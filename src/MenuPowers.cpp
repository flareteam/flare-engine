/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

/**
 * class MenuPowers
 */

#include "Avatar.h"
#include "EffectManager.h"
#include "EngineSettings.h"
#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "MenuPowers.h"
#include "MessageEngine.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "StatBlock.h"
#include "TooltipManager.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetSlot.h"
#include "WidgetTabControl.h"

#include <climits>

Power_Menu_Cell::Power_Menu_Cell()
	: id(-1)
	, tab(0)
	, pos(Point())
	, requires_primary(eset->primary_stats.list.size(), 0)
	, requires_level(0)
	, upgrade_level(0)
	, upgrades()
	, requires_power()
	, requires_point(false)
	, passive_on(false) {
}

MenuPowers::MenuPowers(StatBlock *_stats, MenuActionBar *_action_bar)
	: stats(_stats)
	, action_bar(_action_bar)
	, skip_section(false)
	, powers_unlock(NULL)
	, overlay_disabled(NULL)
	, points_left(0)
	, default_background("")
	, label_powers(new WidgetLabel)
	, label_unspent(new WidgetLabel)
	, tab_control(NULL)
	, tree_loaded(false)
	, prev_powers_list_size(0)
	, default_power_tab(-1)
	, newPowerNotification(false)
{

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Read powers data from config file
	FileParser infile;
	// @CLASS MenuPowers: Menu layout|Description of menus/powers.txt
	if (infile.open("menus/powers.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR label_title|label|Position of the "Powers" text.
			if (infile.key == "label_title") {
				label_powers->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR unspent_points|label|Position of the text that displays the amount of unused power points.
			else if (infile.key == "unspent_points") {
				label_unspent->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR close|point|Position of the close button.
			else if (infile.key == "close") close_pos = Parse::toPoint(infile.val);
			// @ATTR tab_area|rectangle|Position and dimensions of the tree pages.
			else if (infile.key == "tab_area") tab_area = Parse::toRect(infile.val);

			else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	label_powers->setText(msg->get("Powers"));
	label_powers->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	label_unspent->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	loadGraphics();

	menu_powers = this;

	align();
}

MenuPowers::~MenuPowers() {
	if (powers_unlock) delete powers_unlock;
	if (overlay_disabled) delete overlay_disabled;

	for (size_t i=0; i<tree_surf.size(); i++) {
		if (tree_surf[i]) delete tree_surf[i];
	}
	for (size_t i=0; i<slots.size(); i++) {
		delete slots.at(i);
		delete upgradeButtons.at(i);
	}
	slots.clear();
	upgradeButtons.clear();

	delete closeButton;
	if (tab_control) delete tab_control;
	menu_powers = NULL;

	delete label_powers;
	delete label_unspent;
}

void MenuPowers::align() {
	Menu::align();

	label_powers->setPos(window_area.x, window_area.y);
	label_unspent->setPos(window_area.x, window_area.y);

	closeButton->pos.x = window_area.x+close_pos.x;
	closeButton->pos.y = window_area.y+close_pos.y;

	if (tab_control) {
		tab_control->setMainArea(window_area.x + tab_area.x, window_area.y + tab_area.y);
	}

	for (size_t i = 0; i < slots.size(); i++) {
		if (!slots[i]) continue;

		slots[i]->setPos(window_area.x, window_area.y);

		if (upgradeButtons[i] != NULL) {
			upgradeButtons[i]->setPos(window_area.x, window_area.y);
		}
	}

}

void MenuPowers::loadGraphics() {

	Image *graphics;

	setBackground("images/menus/powers.png");

	graphics = render_device->loadImage("images/menus/powers_unlock.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		powers_unlock = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/disabled.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		overlay_disabled = graphics->createSprite();
		graphics->unref();
	}
}

/**
 * Loads a given power tree and sets up the menu accordingly
 *
 * @param filename Path to the file that will be loaded
 */
void MenuPowers::loadPowerTree(const std::string &filename) {
	// only load the power tree once per instance
	if (tree_loaded) return;

	// First, parse the power tree file

	FileParser infile;
	// @CLASS MenuPowers: Power tree layout|Description of powers/trees/
	if (infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				// for sections that are stored in collections, add a new object here
				if (infile.section == "power") {
					slots.push_back(NULL);
					upgradeButtons.push_back(NULL);
					power_cell.push_back(Power_Menu_Cell());
				}
				else if (infile.section == "upgrade")
					power_cell_upgrade.push_back(Power_Menu_Cell());
				else if (infile.section == "tab")
					tabs.push_back(Power_Menu_Tab());
			}

			if (infile.section == "") {
				// @ATTR background|filename|Filename of the default background image
				if (infile.key == "background") default_background = infile.val;
			}
			else if (infile.section == "tab")
				loadTab(infile);
			else if (infile.section == "power")
				loadPower(infile);
			else if (infile.section == "upgrade")
				loadUpgrade(infile);
		}
		infile.close();
	}

	// save a copy of the base level powers, as they get overwritten during upgrades
	power_cell_base = power_cell;

	// store the appropriate level for all upgrades
	for (size_t i=0; i<power_cell_upgrade.size(); ++i) {
		for (size_t j=0; j<power_cell_base.size(); j++) {
			std::vector<int>::iterator it = std::find(power_cell_base[j].upgrades.begin(), power_cell_base[j].upgrades.end(), power_cell_upgrade[i].id);
			if (it != power_cell_base[j].upgrades.end()) {
				power_cell_upgrade[i].upgrade_level = static_cast<int>(std::distance(power_cell_base[j].upgrades.begin(), it) + 2);
				break;
			}
		}
	}

	// combine base and upgrade powers into a single list
	for (size_t i=0; i<power_cell_base.size(); ++i) {
		power_cell_all.push_back(power_cell_base[i]);
	}
	for (size_t i=0; i<power_cell_upgrade.size(); ++i) {
		power_cell_all.push_back(power_cell_upgrade[i]);
	}

	// save cell indexes for required powers
	for (size_t i=0; i<power_cell_all.size(); ++i) {
		for (size_t j=0; j<power_cell_all[i].requires_power.size(); ++j) {
			int cell_index = getCellByPowerIndex(power_cell_all[i].requires_power[j], power_cell_all);
			power_cell_all[i].requires_power_cell.push_back(cell_index);
		}
	}

	// load any specified graphics into the tree_surf vector
	Image *graphics;
	if (tabs.empty() && default_background != "") {
		graphics = render_device->loadImage(default_background, RenderDevice::ERROR_NORMAL);
		if (graphics) {
			tree_surf.push_back(graphics->createSprite());
			graphics->unref();
		}
	}
	else {
		for (size_t i=0; i<tabs.size(); ++i) {
			if (tabs[i].background == "")
				tabs[i].background = default_background;

			if (tabs[i].background == "") {
				tree_surf.push_back(NULL);
				continue;
			}

			graphics = render_device->loadImage(tabs[i].background, RenderDevice::ERROR_NORMAL);
			if (graphics) {
				tree_surf.push_back(graphics->createSprite());
				graphics->unref();
			}
			else {
				tree_surf.push_back(NULL);
			}
		}
	}

	// If we have more than one tab, create tab_control
	if (!tabs.empty()) {
		tab_control = new WidgetTabControl();

		if (tab_control) {
			// Initialize the tab control.
			tab_control->setMainArea(window_area.x+tab_area.x, window_area.y+tab_area.y);

			// Define the header.
			for (size_t i=0; i<tabs.size(); i++)
				tab_control->setTabTitle(static_cast<unsigned>(i), msg->get(tabs[i].title));
			tab_control->updateHeader();

			tablist.add(tab_control);
		}

		tablist_pow.resize(tabs.size());
	}

	// create power slots
	for (size_t i=0; i<slots.size(); i++) {
		if (static_cast<size_t>(power_cell[i].id) < powers->powers.size()) {
			slots[i] = new WidgetSlot(powers->powers[power_cell[i].id].icon, Input::ACCEPT);
			slots[i]->setBasePos(power_cell[i].pos.x, power_cell[i].pos.y, Utils::ALIGN_TOPLEFT);

			if (!tablist_pow.empty()) {
				tablist_pow[power_cell[i].tab].add(slots[i]);
				tablist_pow[power_cell[i].tab].setPrevTabList(&tablist);
				tablist_pow[power_cell[i].tab].lock();
			}
			else {
				tablist.add(slots[i]);
			}

			if (upgradeButtons[i] != NULL) {
				upgradeButtons[i]->setBasePos(power_cell[i].pos.x + eset->resolutions.icon_size, power_cell[i].pos.y, Utils::ALIGN_TOPLEFT);
			}
		}
	}

	applyPowerUpgrades();

	// set the default tab from character class setting
	EngineSettings::HeroClasses::HeroClass* pc_class;
	pc_class = eset->hero_classes.getByName(pc->stats.character_class);
	if (pc_class) {
		default_power_tab = pc_class->default_power_tab;
	}

	tree_loaded = true;

	align();
}

void MenuPowers::loadTab(FileParser &infile) {
	// @ATTR tab.title|string|The name of this power tree tab
	if (infile.key == "title") tabs.back().title = infile.val;
	// @ATTR tab.background|filename|Filename of the background image for this tab's power tree
	else if (infile.key == "background") tabs.back().background = infile.val;
}

void MenuPowers::loadPower(FileParser &infile) {
	// @ATTR power.id|int|A power id from powers/powers.txt for this slot.
	if (infile.key == "id") {
		int id = Parse::popFirstInt(infile.val);
		if (id > 0) {
			skip_section = false;
			power_cell.back().id = id;
		}
		else {
			infile.error("MenuPowers: Power index out of bounds 1-%d, skipping power.", INT_MAX);
		}
		return;
	}

	if (power_cell.back().id <= 0) {
		skip_section = true;
		power_cell.pop_back();
		slots.pop_back();
		upgradeButtons.pop_back();
		Utils::logError("MenuPowers: There is a power without a valid id as the first attribute. IDs must be the first attribute in the power menu definition.");
	}

	if (skip_section)
		return;

	// @ATTR power.tab|int|Tab index to place this power on, starting from 0.
	if (infile.key == "tab") power_cell.back().tab = Parse::toInt(infile.val);
	// @ATTR power.position|point|Position of this power icon; relative to MenuPowers "pos".
	else if (infile.key == "position") power_cell.back().pos = Parse::toPoint(infile.val);

	// @ATTR power.requires_primary|predefined_string, int : Primary stat name, Required value|Power requires this primary stat to be at least the specificed value.
	else if (infile.key == "requires_primary") {
		std::string prim_stat = Parse::popFirstString(infile.val);
		size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

		if (prim_stat_index != eset->primary_stats.list.size()) {
			power_cell.back().requires_primary[prim_stat_index] = Parse::toInt(infile.val);
		}
		else {
			infile.error("MenuPowers: '%s' is not a valid primary stat.", prim_stat.c_str());
		}
	}
	// @ATTR power.requires_point|bool|Power requires a power point to unlock.
	else if (infile.key == "requires_point") power_cell.back().requires_point = Parse::toBool(infile.val);
	// @ATTR power.requires_level|int|Power requires at least this level for the hero.
	else if (infile.key == "requires_level") power_cell.back().requires_level = Parse::toInt(infile.val);
	// @ATTR power.requires_power|power_id|Power requires another power id.
	else if (infile.key == "requires_power") power_cell.back().requires_power.push_back(Parse::toInt(infile.val));

	// @ATTR power.visible_requires_status|repeatable(string)|Hide the power if we don't have this campaign status.
	else if (infile.key == "visible_requires_status") power_cell.back().visible_requires_status.push_back(infile.val);
	// @ATTR power.visible_requires_not_status|repeatable(string)|Hide the power if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell.back().visible_requires_not.push_back(infile.val);

	// @ATTR power.upgrades|list(power_id)|A list of upgrade power ids that this power slot can upgrade to. Each of these powers should have a matching upgrade section.
	else if (infile.key == "upgrades") {
		if (power_cell.back().upgrades.empty()) {
			upgradeButtons.back() = new WidgetButton("images/menus/buttons/button_plus.png");
		}

		std::string repeat_val = Parse::popFirstString(infile.val);
		while (repeat_val != "") {
			power_cell.back().upgrades.push_back(Parse::toInt(repeat_val));
			repeat_val = Parse::popFirstString(infile.val);
		}

		if (!power_cell.back().upgrades.empty())
			power_cell.back().upgrade_level = 1;
	}

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

void MenuPowers::loadUpgrade(FileParser &infile) {
	// @ATTR upgrade.id|int|A power id from powers/powers.txt for this upgrade.
	if (infile.key == "id") {
		int id = Parse::popFirstInt(infile.val);
		if (id > 0) {
			skip_section = false;
			power_cell_upgrade.back().id = (id);
		}
		else {
			skip_section = true;
			power_cell_upgrade.pop_back();
			infile.error("MenuPowers: Power index out of bounds 1-%d, skipping power.", INT_MAX);
		}
		return;
	}

	if (skip_section)
		return;

	// @ATTR upgrade.requires_primary|predefined_string, int : Primary stat name, Required value|Upgrade requires this primary stat to be at least the specificed value.
	if (infile.key == "requires_primary") {
		std::string prim_stat = Parse::popFirstString(infile.val);
		size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

		if (prim_stat_index != eset->primary_stats.list.size()) {
			power_cell_upgrade.back().requires_primary[prim_stat_index] = Parse::toInt(infile.val);
		}
		else {
			infile.error("MenuPowers: '%s' is not a valid primary stat.", prim_stat.c_str());
		}
	}
	// @ATTR upgrade.requires_point|bool|Upgrade requires a power point to unlock.
	else if (infile.key == "requires_point") power_cell_upgrade.back().requires_point = Parse::toBool(infile.val);
	// @ATTR upgrade.requires_level|int|Upgrade requires at least this level for the hero.
	else if (infile.key == "requires_level") power_cell_upgrade.back().requires_level = Parse::toInt(infile.val);
	// @ATTR upgrade.requires_power|int|Upgrade requires another power id.
	else if (infile.key == "requires_power") power_cell_upgrade.back().requires_power.push_back(Parse::toInt(infile.val));

	// @ATTR upgrade.visible_requires_status|repeatable(string)|Hide the upgrade if we don't have this campaign status.
	else if (infile.key == "visible_requires_status") power_cell_upgrade.back().visible_requires_status.push_back(infile.val);
	// @ATTR upgrade.visible_requires_not_status|repeatable(string)|Hide the upgrade if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell_upgrade.back().visible_requires_not.push_back(infile.val);

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

bool MenuPowers::checkRequirements(int pci) {
	if (pci == -1)
		return false;

	for (size_t i = 0; i < power_cell_all[pci].requires_power_cell.size(); ++i)
		if (!checkUnlocked(power_cell_all[pci].requires_power_cell[i]))
			return false;

	if (stats->level < power_cell_all[pci].requires_level)
		return false;

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (stats->get_primary(i) < power_cell_all[pci].requires_primary[i])
			return false;
	}

	const Power& power = powers->powers[power_cell_all[pci].id];
	if (power.passive) {
		if (!stats->canUsePower(power_cell_all[pci].id, StatBlock::CAN_USE_PASSIVE))
			return false;
	}

	return true;
}

bool MenuPowers::checkUnlocked(int pci) {
	// If we didn't find power in power_menu, than it has no requirements
	if (pci == -1) return true;

	if (!checkCellVisible(pci))
		return false;

	// If power_id is saved into vector, it's unlocked anyway
	// check power_cell_unlocked and stats->powers_list
	for (size_t i=0; i<power_cell_unlocked.size(); ++i) {
		if (power_cell_unlocked[i].id == power_cell_all[pci].id)
			return true;
	}
	if (std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell_all[pci].id) != stats->powers_list.end())
		return true;

	// Check the rest requirements
	// only check base level; upgrades are checked in logic()
	if (!power_cell_all[pci].requires_point && power_cell_all[pci].upgrade_level <= 1 && checkRequirements(pci))
		return true;

	return false;
}

bool MenuPowers::checkCellVisible(int pci) {
	// If we didn't find power in power_menu, than it has no requirements
	if (pci == -1) return true;

	for (size_t i=0; i<power_cell_all[pci].visible_requires_status.size(); ++i)
		if (!camp->checkStatus(power_cell_all[pci].visible_requires_status[i]))
			return false;

	for (size_t i=0; i<power_cell_all[pci].visible_requires_not.size(); ++i)
		if (camp->checkStatus(power_cell_all[pci].visible_requires_not[i]))
			return false;

	return true;
}

/**
 * Check if we can unlock power.
 */
bool MenuPowers::checkUnlock(int pci) {
	// If we didn't find power in power_menu, than it has no requirements
	if (pci == -1) return true;

	if (!checkCellVisible(pci)) return false;

	// If we already have a power, don't try to unlock it
	if (checkUnlocked(pci)) return false;

	// Check base requirements
	if (checkRequirements(pci)) return true;
	return false;
}

bool MenuPowers::checkUpgrade(int pci) {
	int id = getCellByPowerIndex(power_cell[pci].id, power_cell_all);
	if (!checkUnlocked(id))
		return false;

	int next_index = getNextLevelCell(pci);
	if (next_index == -1)
		return false;
	if (power_cell_upgrade[next_index].requires_point && points_left < 1)
		return false;

	int id_upgrade = getCellByPowerIndex(power_cell_upgrade[next_index].id, power_cell_all);
	if (!checkUnlock(id_upgrade))
		return false;

	return true;
}

int MenuPowers::getCellByPowerIndex(int power_index, const std::vector<Power_Menu_Cell>& cell) {
	// Powers can not have an id of 0
	if (power_index == 0) return -1;

	// Find cell with our power
	for (size_t i=0; i<cell.size(); i++)
		if (cell[i].id == power_index)
			return static_cast<int>(i);

	return -1;
}

/**
 * Find cell in upgrades with next upgrade for current power_cell
 */
int MenuPowers::getNextLevelCell(int pci) {
	if (power_cell[pci].upgrades.empty()) {
		return -1;
	}

	std::vector<int>::iterator level_it;
	level_it = std::find(power_cell[pci].upgrades.begin(),
					power_cell[pci].upgrades.end(),
					power_cell[pci].id);

	if (level_it == power_cell[pci].upgrades.end()) {
		// current power is base power, take first upgrade
		int index = power_cell[pci].upgrades[0];
		return getCellByPowerIndex(index, power_cell_upgrade);
	}
	// current power is an upgrade, take next upgrade if avaliable
	size_t index = std::distance(power_cell[pci].upgrades.begin(), level_it);
	if (power_cell[pci].upgrades.size() > index + 1) {
		return getCellByPowerIndex(*(++level_it), power_cell_upgrade);
	}
	else {
		return -1;
	}
}

void MenuPowers::replaceCellWithUpgrade(int pci, int uci) {
	power_cell[pci].id = power_cell_upgrade[uci].id;
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		power_cell[pci].requires_primary[i] = power_cell_upgrade[uci].requires_primary[i];
	}
	power_cell[pci].requires_level = power_cell_upgrade[uci].requires_level;
	power_cell[pci].requires_power = power_cell_upgrade[uci].requires_power;
	power_cell[pci].requires_power_cell = power_cell_upgrade[uci].requires_power_cell;
	power_cell[pci].requires_point = power_cell_upgrade[uci].requires_point;
	power_cell[pci].passive_on = power_cell_upgrade[uci].passive_on;
	power_cell[pci].upgrade_level = power_cell_upgrade[uci].upgrade_level;

	if (slots[pci])
		slots[pci]->setIcon(powers->powers[power_cell_upgrade[uci].id].icon);
}

/**
 * Upgrade power cell "pci" to the next level
 */
void MenuPowers::upgradePower(int pci, bool ignore_tab) {
	int i = getNextLevelCell(pci);
	if (i == -1)
		return;

	// if power was present in ActionBar, update it there
	action_bar->addPower(power_cell_upgrade[i].id, power_cell[pci].id);

	// if we have tab_control
	if (tab_control) {
		int active_tab = tab_control->getActiveTab();
		if (power_cell[pci].tab == active_tab || ignore_tab) {
			replaceCellWithUpgrade(pci, i);
			stats->powers_list.push_back(power_cell_upgrade[i].id);
			stats->check_title = true;
		}
	}
	// if have don't have tabs
	else {
		replaceCellWithUpgrade(pci, i);
		stats->powers_list.push_back(power_cell_upgrade[i].id);
		stats->check_title = true;
	}
	setUnlockedPowers();
}

void MenuPowers::setUnlockedPowers() {
	std::vector<int> power_ids;
	bool unlocked_cleared = false;

	// only clear/repopulate power_cell_unlocked if the size of the hero's powers_list has changed
	if (prev_powers_list_size != stats->powers_list.size() || power_cell_unlocked.empty()) {
		prev_powers_list_size = stats->powers_list.size();
		power_cell_unlocked.clear();
		unlocked_cleared = true;
	}

	for (size_t i=0; i<power_cell.size(); ++i) {
		if (std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end()) {
			if (!unlocked_cleared)
				continue;

			// base power
			if (std::find(power_ids.begin(), power_ids.end(), power_cell[i].id) == power_ids.end()) {
				power_ids.push_back(power_cell_base[i].id);
				power_cell_unlocked.push_back(power_cell_base[i]);
			}
			if (power_cell[i].id == power_cell_base[i].id)
				continue;

			//upgrades
			for (size_t j=0; j<power_cell[i].upgrades.size(); ++j) {
				if (std::find(power_ids.begin(), power_ids.end(), power_cell[i].upgrades[j]) == power_ids.end()) {
					int id = getCellByPowerIndex(power_cell[i].upgrades[j], power_cell_upgrade);
					if (id != -1) {
						power_ids.push_back(power_cell[i].upgrades[j]);
						power_cell_unlocked.push_back(power_cell_upgrade[id]);

						if (power_cell[i].id == power_cell[i].upgrades[j])
							break;
					}
					else {
						break;
					}
				}
			}
		}
		else {
			// power is unlocked, but not in the player's powers_list
			int pci = getCellByPowerIndex(power_cell[i].id, power_cell_all);
			if (checkUnlocked(pci)) {
				stats->powers_list.push_back(power_cell[i].id);
			}
		}
	}

	// the hero's powers_list may have grown, so we need to re-check unlocked powers
	if (prev_powers_list_size != stats->powers_list.size()) {
		setUnlockedPowers();
	}
}

int MenuPowers::getPointsUsed() {
	int used = 0;
	for (size_t i=0; i<power_cell_unlocked.size(); ++i) {
		if (power_cell_unlocked[i].requires_point)
			used++;
	}

	return used;
}

void MenuPowers::createTooltip(TooltipData* tip_data, int slot_num, const std::vector<Power_Menu_Cell>& power_cells, bool show_unlock_prompt) {
	if (power_cells[slot_num].upgrade_level > 0)
		tip_data->addText(powers->powers[power_cells[slot_num].id].name + " (" + msg->get("Level %d", power_cells[slot_num].upgrade_level) + ")");
	else
		tip_data->addText(powers->powers[power_cells[slot_num].id].name);

	if (powers->powers[power_cells[slot_num].id].passive) tip_data->addText(msg->get("Passive"));
	tip_data->addColoredText(Utils::substituteVarsInString(powers->powers[power_cells[slot_num].id].description, pc), font->getColor(FontEngine::COLOR_ITEM_FLAVOR));

	// add mana cost
	if (powers->powers[power_cells[slot_num].id].requires_mp > 0) {
		tip_data->addText(msg->get("Costs %d MP", powers->powers[power_cells[slot_num].id].requires_mp));
	}
	// add health cost
	if (powers->powers[power_cells[slot_num].id].requires_hp > 0) {
		tip_data->addText(msg->get("Costs %d HP", powers->powers[power_cells[slot_num].id].requires_hp));
	}
	// add cooldown time
	if (powers->powers[power_cells[slot_num].id].cooldown > 0) {
		std::stringstream ss;
		ss << msg->get("Cooldown:") << " " << Utils::getDurationString(powers->powers[power_cells[slot_num].id].cooldown);
		tip_data->addText(ss.str());
	}

	const Power &pwr = powers->powers[power_cells[slot_num].id];
	for (size_t i=0; i<pwr.post_effects.size(); ++i) {
		std::stringstream ss;
		EffectDef* effect_ptr = powers->getEffectDef(pwr.post_effects[i].id);

		// base stats
		if (effect_ptr == NULL) {
			if (pwr.post_effects[i].magnitude > 0) {
				ss << "+";
			}

			ss << pwr.post_effects[i].magnitude;
			bool found_key = false;

			for (size_t j=0; j<Stats::COUNT; ++j) {
				if (pwr.post_effects[i].id == Stats::KEY[j]) {
					if (Stats::PERCENT[j])
						ss << "%";

					ss << " " << Stats::NAME[j];

					found_key = true;
					break;
				}
			}

			if (!found_key) {
				for (size_t j=0; j<eset->elements.list.size(); ++j) {
					if (pwr.post_effects[i].id == eset->elements.list[j].id + "_resist") {
						ss << "% " << msg->get("%s Resistance", eset->elements.list[j].name.c_str());
						found_key = true;
						break;
					}
				}
			}

			if (!found_key) {
				for (size_t j=0; j<eset->primary_stats.list.size(); ++j) {
					if (pwr.post_effects[i].id == eset->primary_stats.list[j].id) {
						ss << " " << eset->primary_stats.list[j].name;
						found_key = true;
						break;
					}
				}
			}

			if (!found_key) {
				for (size_t j=0; j<eset->damage_types.list.size(); ++j) {
					if (pwr.post_effects[i].id == eset->damage_types.list[j].min) {
						ss << " " << eset->damage_types.list[j].name_min;
						found_key = true;
						break;
					}
					else if (pwr.post_effects[i].id == eset->damage_types.list[j].max) {
						ss << " " << eset->damage_types.list[j].name_max;
						found_key = true;
						break;
					}
				}
			}
		}
		else {
			if (effect_ptr->type == "damage") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("Damage per second");
			}
			else if (effect_ptr->type == "damage_percent") {
				ss << pwr.post_effects[i].magnitude << "% " << msg->get("Damage per second");
			}
			else if (effect_ptr->type == "hpot") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("HP per second");
			}
			else if (effect_ptr->type == "hpot_percent") {
				ss << pwr.post_effects[i].magnitude << "% " << msg->get("HP per second");
			}
			else if (effect_ptr->type == "mpot") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("MP per second");
			}
			else if (effect_ptr->type == "mpot_percent") {
				ss << pwr.post_effects[i].magnitude << "% " << msg->get("MP per second");
			}
			else if (effect_ptr->type == "speed") {
				if (pwr.post_effects[i].magnitude == 0)
					ss << msg->get("Immobilize");
				else
					ss << msg->get("%d%% Speed", pwr.post_effects[i].magnitude);
			}
			else if (effect_ptr->type == "attack_speed") {
				ss << msg->get("%d%% Attack Speed", pwr.post_effects[i].magnitude);
			}
			else if (effect_ptr->type == "immunity") {
				ss << msg->get("Immunity");
			}
			else if (effect_ptr->type == "immunity_damage") {
				ss << msg->get("Immunity to damage over time");
			}
			else if (effect_ptr->type == "immunity_slow") {
				ss << msg->get("Immunity to slow");
			}
			else if (effect_ptr->type == "immunity_stun") {
				ss << msg->get("Immunity to stun");
			}
			else if (effect_ptr->type == "immunity_hp_steal") {
				ss << msg->get("Immunity to HP steal");
			}
			else if (effect_ptr->type == "immunity_mp_steal") {
				ss << msg->get("Immunity to MP steal");
			}
			else if (effect_ptr->type == "immunity_knockback") {
				ss << msg->get("Immunity to knockback");
			}
			else if (effect_ptr->type == "immunity_damage_reflect") {
				ss << msg->get("Immunity to damage reflection");
			}
			else if (effect_ptr->type == "stun") {
				ss << msg->get("Stun");
			}
			else if (effect_ptr->type == "revive") {
				ss << msg->get("Automatic revive on death");
			}
			else if (effect_ptr->type == "convert") {
				ss << msg->get("Convert");
			}
			else if (effect_ptr->type == "fear") {
				ss << msg->get("Fear");
			}
			else if (effect_ptr->type == "death_sentence") {
				ss << msg->get("Lifespan");
			}
			else if (effect_ptr->type == "shield") {
				if (pwr.base_damage == eset->damage_types.list.size())
					continue;

				if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
					int magnitude = stats->getDamageMax(pwr.base_damage) * pwr.mod_damage_value_min / 100;
					ss << magnitude;
				}
				else if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_ADD) {
					int magnitude = stats->getDamageMax(pwr.base_damage) + pwr.mod_damage_value_min;
					ss << magnitude;
				}
				else if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_ABSOLUTE) {
					if (pwr.mod_damage_value_max == 0 || pwr.mod_damage_value_min == pwr.mod_damage_value_max)
						ss << pwr.mod_damage_value_min;
					else
						ss << pwr.mod_damage_value_min << "-" << pwr.mod_damage_value_max;
				}
				else {
					ss << stats->getDamageMax(pwr.base_damage);
				}

				ss << " " << msg->get("Magical Shield");
			}
			else if (effect_ptr->type == "heal") {
				if (pwr.base_damage == eset->damage_types.list.size())
					continue;

				int mag_min = stats->getDamageMin(pwr.base_damage);
				int mag_max = stats->getDamageMax(pwr.base_damage);

				if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
					mag_min = mag_min * pwr.mod_damage_value_min / 100;
					mag_max = mag_max * pwr.mod_damage_value_min / 100;
					ss << mag_min << "-" << mag_max;
				}
				else if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_ADD) {
					mag_min = mag_min + pwr.mod_damage_value_min;
					mag_max = mag_max + pwr.mod_damage_value_min;
					ss << mag_min << "-" << mag_max;
				}
				else if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_ABSOLUTE) {
					if (pwr.mod_damage_value_max == 0 || pwr.mod_damage_value_min == pwr.mod_damage_value_max)
						ss << pwr.mod_damage_value_min;
					else
						ss << pwr.mod_damage_value_min << "-" << pwr.mod_damage_value_max;
				}
				else {
					ss << mag_min << "-" << mag_max;
				}

				ss << " " << msg->get("Healing");
			}
			else if (effect_ptr->type == "knockback") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("Knockback");
			}
			else if (!effect_ptr->name.empty() && pwr.post_effects[i].magnitude > 0) {
				if (effect_ptr->can_stack)
					ss << "+";
				ss << pwr.post_effects[i].magnitude << " " << msg->get(effect_ptr->name);
			}
			else if (pwr.post_effects[i].magnitude == 0) {
				// nothing
			}
		}

		if (!ss.str().empty()) {
			if (pwr.post_effects[i].duration > 0) {
				if (effect_ptr && effect_ptr->type == "death_sentence") {
					ss << ": " << Utils::getDurationString(pwr.post_effects[i].duration);
				}
				else {
					ss << " (" << Utils::getDurationString(pwr.post_effects[i].duration) << ")";
				}

				if (pwr.post_effects[i].chance != 100)
					ss << " ";
			}
			if (pwr.post_effects[i].chance != 100) {
				ss << "(" << msg->get("%d%% chance", pwr.post_effects[i].chance) << ")";
			}

			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
	}

	if (pwr.use_hazard || pwr.type == Power::TYPE_REPEATER) {
		std::stringstream ss;

		// modifier_damage
		if (pwr.mod_damage_mode > -1) {
			if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_ADD && pwr.mod_damage_value_min > 0)
				ss << "+";

			if (pwr.mod_damage_value_max == 0 || pwr.mod_damage_value_min == pwr.mod_damage_value_max) {
				ss << pwr.mod_damage_value_min;
			}
			else {
				ss << pwr.mod_damage_value_min << "-" << pwr.mod_damage_value_max;
			}

			if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			if (pwr.base_damage != eset->damage_types.list.size()) {
				ss << eset->damage_types.list[pwr.base_damage].name;
			}

			if (pwr.count > 1 && pwr.type != Power::TYPE_REPEATER)
				ss << " (x" << pwr.count << ")";

			if (!ss.str().empty())
				tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}

		// modifier_accuracy
		if (pwr.mod_accuracy_mode > -1) {
			ss.str("");

			if (pwr.mod_accuracy_mode == Power::STAT_MODIFIER_MODE_ADD && pwr.mod_accuracy_value > 0)
				ss << "+";

			ss << pwr.mod_accuracy_value;

			if (pwr.mod_accuracy_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			ss << msg->get("Base Accuracy");

			if (!ss.str().empty())
				tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}

		// modifier_critical
		if (pwr.mod_crit_mode > -1) {
			ss.str("");

			if (pwr.mod_crit_mode == Power::STAT_MODIFIER_MODE_ADD && pwr.mod_crit_value > 0)
				ss << "+";

			ss << pwr.mod_crit_value;

			if (pwr.mod_crit_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			ss << msg->get("Base Critical Chance");

			if (!ss.str().empty())
				tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}

		if (pwr.trait_armor_penetration) {
			ss.str("");
			ss << msg->get("Ignores Absorbtion");
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		if (pwr.trait_avoidance_ignore) {
			ss.str("");
			ss << msg->get("Ignores Avoidance");
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		if (pwr.trait_crits_impaired > 0) {
			ss.str("");
			ss << msg->get("%d%% Chance to crit slowed targets", pwr.trait_crits_impaired);
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		if (pwr.trait_elemental > -1) {
			ss.str("");
			ss << msg->get("%s Elemental Damage", eset->elements.list[pwr.trait_elemental].name.c_str());
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
	}

	std::set<std::string>::iterator it;
	for (it = powers->powers[power_cells[slot_num].id].requires_flags.begin(); it != powers->powers[power_cells[slot_num].id].requires_flags.end(); ++it) {
		for (size_t i = 0; i < eset->equip_flags.list.size(); ++i) {
			if ((*it) == eset->equip_flags.list[i].id) {
				tip_data->addText(msg->get("Requires a %s", msg->get(eset->equip_flags.list[i].name).c_str()));
			}
		}
	}

	// add requirement
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (power_cells[slot_num].requires_primary[i] > 0) {
			if (stats->get_primary(i) < power_cells[slot_num].requires_primary[i])
				tip_data->addColoredText(msg->get("Requires %s %d", eset->primary_stats.list[i].name.c_str(), power_cells[slot_num].requires_primary[i]), font->getColor(FontEngine::COLOR_MENU_PENALTY));
			else
				tip_data->addText(msg->get("Requires %s %d", eset->primary_stats.list[i].name.c_str(), power_cells[slot_num].requires_primary[i]));
		}
	}

	// Draw required Level Tooltip
	if ((power_cells[slot_num].requires_level > 0) && stats->level < power_cells[slot_num].requires_level) {
		tip_data->addColoredText(msg->get("Requires Level %d", power_cells[slot_num].requires_level), font->getColor(FontEngine::COLOR_MENU_PENALTY));
	}
	else if ((power_cells[slot_num].requires_level > 0) && stats->level >= power_cells[slot_num].requires_level) {
		tip_data->addText(msg->get("Requires Level %d", power_cells[slot_num].requires_level));
	}

	for (size_t j=0; j < power_cells[slot_num].requires_power.size(); ++j) {
		if (power_cells[slot_num].requires_power[j] == 0) continue;

		int req_index = getCellByPowerIndex(power_cells[slot_num].requires_power[j], power_cell_all);
		if (req_index == -1) continue;

		std::string req_power_name;
		if (power_cell_all[req_index].upgrade_level > 0)
			req_power_name = powers->powers[power_cell_all[req_index].id].name + " (" + msg->get("Level %d", power_cell_all[req_index].upgrade_level) + ")";
		else
			req_power_name = powers->powers[power_cell_all[req_index].id].name;


		// Required Power Tooltip
		int req_cell_index = getCellByPowerIndex(power_cells[slot_num].requires_power[j], power_cell_all);
		if (!checkUnlocked(req_cell_index)) {
			tip_data->addColoredText(msg->get("Requires Power: %s", req_power_name.c_str()), font->getColor(FontEngine::COLOR_MENU_PENALTY));
		}
		else {
			tip_data->addText(msg->get("Requires Power: %s", req_power_name.c_str()));
		}

	}

	// Draw unlock power Tooltip
	if (power_cells[slot_num].requires_point && !(std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cells[slot_num].id) != stats->powers_list.end())) {
		int unlock_id = getCellByPowerIndex(power_cells[slot_num].id, power_cell_all);
		if (show_unlock_prompt && points_left > 0 && checkUnlock(unlock_id)) {
			tip_data->addColoredText(msg->get("Click to Unlock (uses 1 Skill Point)"), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		else {
			if (power_cells[slot_num].requires_point && points_left < 1)
				tip_data->addColoredText(msg->get("Requires 1 Skill Point"), font->getColor(FontEngine::COLOR_MENU_PENALTY));
			else
				tip_data->addText(msg->get("Requires 1 Skill Point"));
		}
	}
}

void MenuPowers::renderPowers(int tab_num) {

	Rect disabled_src;
	disabled_src.x = disabled_src.y = 0;
	disabled_src.w = disabled_src.h = eset->resolutions.icon_size;

	for (size_t i=0; i<power_cell.size(); i++) {
		bool power_in_vector = false;

		// Continue if slot is not filled with data
		if (power_cell[i].tab != tab_num) continue;

		int cell_index = getCellByPowerIndex(power_cell[i].id, power_cell_all);
		if (!checkCellVisible(cell_index)) continue;

		if (std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end()) power_in_vector = true;

		if (slots[i])
			slots[i]->render();

		// highlighting
		if (power_in_vector || checkUnlocked(cell_index)) {
			Rect src_unlock;

			src_unlock.x = 0;
			src_unlock.y = 0;
			src_unlock.w = eset->resolutions.icon_size;
			src_unlock.h = eset->resolutions.icon_size;

			int selected_slot = -1;
			if (isTabListSelected()) {
				selected_slot = getSelectedCellIndex();
			}

			for (size_t j=0; j<power_cell.size(); j++) {
				if (selected_slot == static_cast<int>(j))
					continue;

				if (power_cell[j].id == power_cell[i].id && powers_unlock && slots[j]) {
					powers_unlock->setClip(src_unlock);
					powers_unlock->setDest(slots[j]->pos);
					render_device->render(powers_unlock);
				}
			}
		}
		else {
			if (overlay_disabled && slots[i]) {
				overlay_disabled->setClip(disabled_src);
				overlay_disabled->setDest(slots[i]->pos);
				render_device->render(overlay_disabled);
			}
		}

		if (slots[i])
			slots[i]->renderSelection();

		// upgrade buttons
		if (upgradeButtons[i])
			upgradeButtons[i]->render();
	}
}

void MenuPowers::logic() {
	if (!visible && tab_control && default_power_tab > -1) {
		tab_control->setActiveTab(static_cast<unsigned>(default_power_tab));
	}

	setUnlockedPowers();

	for (size_t i=0; i<power_cell_unlocked.size(); i++) {
		if (static_cast<size_t>(power_cell_unlocked[i].id) < powers->powers.size() && powers->powers[power_cell_unlocked[i].id].passive) {
			std::vector<int>::iterator passive_it = std::find(stats->powers_passive.begin(), stats->powers_passive.end(), power_cell_unlocked[i].id);

			int cell_index = getCellByPowerIndex(power_cell_unlocked[i].id, power_cell_all);
			bool is_current_upgrade_max = (getCellByPowerIndex(power_cell_unlocked[i].id, power_cell) != -1);

			if (passive_it != stats->powers_passive.end()) {
				if (!is_current_upgrade_max || (!checkRequirements(cell_index) && power_cell_unlocked[i].passive_on)) {
					// passive power is activated, but does not meet requirements, so remove it
					stats->powers_passive.erase(passive_it);
					stats->effects.removeEffectPassive(power_cell_unlocked[i].id);
					power_cell[i].passive_on = false;
					power_cell_unlocked[i].passive_on = false;
					stats->refresh_stats = true;
				}
			}
			else if (is_current_upgrade_max && checkRequirements(cell_index) && !power_cell_unlocked[i].passive_on) {
				// passive power has not been activated, so activate it here
				stats->powers_passive.push_back(power_cell_unlocked[i].id);
				power_cell_unlocked[i].passive_on = true;
				// for passives without special triggers, we need to trigger them here
				if (stats->effects.triggered_others)
					powers->activateSinglePassive(stats, power_cell_unlocked[i].id);
			}
		}
	}

	for (size_t i=0; i<power_cell.size(); i++) {
		//upgrade buttons logic
		if (upgradeButtons[i] != NULL) {
			upgradeButtons[i]->enabled = false;
			// enable button only if current level is unlocked and next level can be unlocked
			if (checkUpgrade(static_cast<int>(i))) {
				upgradeButtons[i]->enabled = true;
			}
			if ((!tab_control || power_cell[i].tab == tab_control->getActiveTab()) && upgradeButtons[i]->checkClick()) {
				upgradePower(static_cast<int>(i), !UPGRADE_POWER_ALL_TABS);
			}

			// automatically apply upgrades when requires_point = false
			if (upgradeButtons[i]->enabled) {
				int next_index;
				int prev_index = -1;
				while ((next_index = getNextLevelCell(static_cast<int>(i))) != -1) {
					if (prev_index == next_index) {
						// this should never happen, but if it ever does, it would be an infinite loop
						break;
					}

					if (!power_cell_upgrade[next_index].requires_point && checkUpgrade(static_cast<int>(i))) {
						upgradePower(static_cast<int>(i), UPGRADE_POWER_ALL_TABS);
					}
					else {
						break;
					}

					prev_index = next_index;
				}
			}
		}
	}

	points_left = (stats->level * stats->power_points_per_level) - getPointsUsed();
	if (points_left > 0) {
		newPowerNotification = true;
	}

	if (!visible) return;

	tablist.logic();
	if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); i++) {
			if (tab_control && tab_control->getActiveTab() == static_cast<int>(i)) {
				tablist.setNextTabList(&tablist_pow[i]);
			}
			tablist_pow[i].logic();
		}
	}

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}

	if (tab_control) {
		// make shure keyboard navigation leads us to correct tab
		for (size_t i=0; i<slots.size(); i++) {
			if (power_cell[i].tab == tab_control->getActiveTab())
				continue;

			if (slots[i] && slots[i]->in_focus)
				slots[i]->defocus();
		}

		tab_control->logic();
	}
}

void MenuPowers::render() {
	if (!visible) return;

	Rect src;
	Rect dest;

	// background
	dest = window_area;
	src.x = 0;
	src.y = 0;
	src.w = window_area.w;
	src.h = window_area.h;

	setBackgroundClip(src);
	setBackgroundDest(dest);
	Menu::render();


	if (tab_control) {
		tab_control->render();
		int active_tab = tab_control->getActiveTab();
		for (size_t i=0; i<tabs.size(); i++) {
			if (active_tab == static_cast<int>(i)) {
				// power tree
				Sprite *r = tree_surf[i];
				if (r) {
					r->setClip(src);
					r->setDest(dest);
					render_device->render(r);
				}

				// power icons
				renderPowers(active_tab);
			}
		}
	}
	else if (!tree_surf.empty()) {
		Sprite *r = tree_surf[0];
		if (r) {
			r->setClip(src);
			r->setDest(dest);
			render_device->render(r);
		}
		renderPowers(0);
	}
	else {
		renderPowers(0);
	}

	// close button
	closeButton->render();

	// text overlay
	label_powers->render();

	// stats
	if (!label_unspent->isHidden()) {
		std::stringstream ss;

		ss.str("");
		if (points_left == 1) {
			ss << msg->get("%d unspent skill point", points_left);
		}
		else if (points_left > 1) {
			ss << msg->get("%d unspent skill points", points_left);
		}
		label_unspent->setText(ss.str());
		label_unspent->render();
	}
}

/**
 * Show mouseover descriptions of disciplines and powers
 */
void MenuPowers::renderTooltips(const Point& position) {
	if (!visible || !Utils::isWithinRect(window_area, position))
		return;

	TooltipData tip_data;

	for (size_t i=0; i<power_cell.size(); i++) {

		if (tab_control && (tab_control->getActiveTab() != power_cell[i].tab)) continue;

		int cell_index = getCellByPowerIndex(power_cell[i].id, power_cell_all);
		if (!checkCellVisible(cell_index)) continue;

		if (slots[i] && Utils::isWithinRect(slots[i]->pos, position)) {
			bool base_unlocked = checkUnlocked(cell_index) || std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end();

			createTooltip(&tip_data, static_cast<int>(i), power_cell, !base_unlocked);
			if (!power_cell[i].upgrades.empty()) {
				int next_level = getNextLevelCell(static_cast<int>(i));
				if (next_level != -1) {
					tip_data.addText("\n" + msg->get("Next Level:"));
					createTooltip(&tip_data, next_level, power_cell_upgrade, base_unlocked);
				}
			}

			tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
			break;
		}
	}
}

/**
 * Click-to-drag a power (to the action bar)
 */
int MenuPowers::click(const Point& mouse) {
	int active_tab = (tab_control) ? tab_control->getActiveTab() : 0;

	for (size_t i=0; i<power_cell.size(); i++) {
		if (slots[i] && Utils::isWithinRect(slots[i]->pos, mouse) && (power_cell[i].tab == active_tab)) {
			if (settings->touchscreen) {
				if (!slots[i]->in_focus) {
					slots[i]->in_focus = true;
					if (!tabs.empty()) {
						tablist_pow[active_tab].setCurrent(slots[i]);
					}
					else {
						tablist.setCurrent(slots[i]);
					}
					return 0;
				}
			}

			int cell_index = getCellByPowerIndex(power_cell[i].id, power_cell_all);
			if (checkUnlock(cell_index) && points_left > 0 && power_cell[i].requires_point) {
				// unlock power
				stats->powers_list.push_back(power_cell[i].id);
				stats->check_title = true;
				setUnlockedPowers();
				action_bar->addPower(power_cell[i].id, 0);
				return 0;
			}
			else if (checkUnlocked(cell_index) && !powers->powers[power_cell[i].id].passive) {
				// pick up and drag power
				slots[i]->defocus();
				if (!tabs.empty()) {
					tablist_pow[active_tab].setCurrent(NULL);
				}
				else {
					tablist.setCurrent(NULL);
				}
				return power_cell[i].id;
			}
			else
				return 0;
		}
	}

	// nothing selected, defocus everything
	defocusTabLists();

	return 0;
}

void MenuPowers::upgradeByCell(int pci) {
	if (checkUpgrade(pci))
		upgradePower(pci, !UPGRADE_POWER_ALL_TABS);
}

/**
 * Apply power upgrades on savegame loading
 */
void MenuPowers::applyPowerUpgrades() {
	for (size_t i=0; i<power_cell.size(); i++) {
		if (!power_cell[i].upgrades.empty()) {
			std::vector<int>::iterator it;
			for (it = power_cell[i].upgrades.end(); it != power_cell[i].upgrades.begin(); ) {
				--it;
				std::vector<int>::iterator upgrade_it;
				upgrade_it = std::find(stats->powers_list.begin(), stats->powers_list.end(), *it);
				if (upgrade_it != stats->powers_list.end()) {
					int upgrade_index = getCellByPowerIndex((*upgrade_it), power_cell_upgrade);
					if (upgrade_index != -1)
						replaceCellWithUpgrade(static_cast<int>(i), upgrade_index);
					break;
				}
			}
		}
	}
	setUnlockedPowers();
}

void MenuPowers::resetToBasePowers() {
	for (size_t i=0; i<power_cell.size(); ++i) {
		power_cell[i] = power_cell_base[i];
	}
}

/**
 * Return true if required stats for power usage are met. Else return false.
 */
bool MenuPowers::meetsUsageStats(int power_index) {
	// Find cell with our power
	int id = getCellByPowerIndex(power_index, power_cell);

	// If we didn't find power in power_menu, than it has no stats requirements
	if (id == -1) return true;

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (stats->get_primary(i) < power_cell[id].requires_primary[i])
			return false;
	}

	return true;
}

bool MenuPowers::isTabListSelected() {
	return (getCurrentTabList() && (tabs.empty() || (tabs.size() > 0 && getCurrentTabList() != (&tablist))));
}

int MenuPowers::getSelectedCellIndex() {
	TabList* cur_tablist = getCurrentTabList();
	int current = cur_tablist->getCurrent();

	if (tabs.empty()) {
		return current;
	}
	else {
		WidgetSlot *cur_slot = static_cast<WidgetSlot*>(cur_tablist->getWidgetByIndex(current));

		for (size_t i = 0; i < slots.size(); ++i) {
			if (slots[i] == cur_slot)
				return static_cast<int>(i);
		}

		// we should never hit this return statement
		return 0;
	}
}

void MenuPowers::setNextTabList(TabList *tl) {
	if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); ++i) {
			tablist_pow[i].setNextTabList(tl);
		}
	}
}

TabList* MenuPowers::getCurrentTabList() {
	if (tablist.getCurrent() != -1) {
		return (&tablist);
	}
	else if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); ++i) {
			if (tablist_pow[i].getCurrent() != -1)
				return (&tablist_pow[i]);
		}
	}

	return NULL;
}

void MenuPowers::defocusTabLists() {
	tablist.defocus();

	if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); ++i) {
			tablist_pow[i].defocus();
		}
	}
}

