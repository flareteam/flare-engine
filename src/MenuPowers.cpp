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
#include "MenuManager.h"
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

MenuPowersCell::MenuPowersCell()
	: id(-1)
	, requires_level(0)
	, requires_point(false)
	, requires_primary(eset->primary_stats.list.size(), 0)
	, requires_power()
	, visible_requires_status()
	, visible_requires_not()
	, upgrade_level(0)
	, passive_on(false)
	, is_unlocked(false)
	, group(0)
	, next(NULL) {
}

bool MenuPowersCell::isVisible() {
	for (size_t i = 0; i < visible_requires_status.size(); ++i)
		if (!camp->checkStatus(visible_requires_status[i]))
			return false;

	for (size_t i = 0; i < visible_requires_not.size(); ++i)
		if (camp->checkStatus(visible_requires_not[i]))
			return false;

	return true;
}

MenuPowersCellGroup::MenuPowersCellGroup()
	: tab(0)
	, pos()
	, current_cell(0)
	, cells()
	, upgrade_button(NULL) {
}

MenuPowersCell* MenuPowersCellGroup::getCurrent() {
	return &cells[current_cell];
}

MenuPowersCell* MenuPowersCellGroup::getBonusCurrent(MenuPowersCell* pcell) {
	if (bonus_levels.empty())
		return pcell;

	size_t current = current_cell;

	for (size_t i = 0; i < cells.size(); ++i) {
		if (pcell == &cells[i]) {
			current = i;
			break;
		}
	}

	int current_bonus_levels = getBonusLevels();
	size_t bonus_cell = current + static_cast<size_t>(current_bonus_levels);

	if (bonus_cell >= cells.size())
		return &cells[cells.size() - 1];

	return &cells[bonus_cell];
}

int MenuPowersCellGroup::getBonusLevels() {
	int blevel = 0;
	for (size_t i = 0; i < bonus_levels.size(); ++i) {
		if (current_cell >= bonus_levels[i].first)
			blevel += bonus_levels[i].second;
	}
	return blevel;
}

MenuPowers::MenuPowers()
	: skip_section(false)
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

	label_unspent->setColor(font->getColor(FontEngine::COLOR_MENU_BONUS));

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
	}
	slots.clear();

	for (size_t i=0; i<power_cell.size(); i++) {
		delete power_cell[i].upgrade_button;
	}

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

		if (power_cell[i].upgrade_button != NULL) {
			power_cell[i].upgrade_button->setPos(window_area.x, window_area.y);
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
	std::vector<MenuPowersCell> power_cell_upgrade;

	FileParser infile;
	// @CLASS MenuPowers: Power tree layout|Description of powers/trees/
	if (infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				// for sections that are stored in collections, add a new object here
				if (infile.section == "power") {
					slots.push_back(NULL);
					power_cell.push_back(MenuPowersCellGroup());
					power_cell.back().cells.push_back(MenuPowersCell());
					power_cell.back().cells.back().group = power_cell.size() - 1;
				}
				else if (infile.section == "upgrade")
					power_cell_upgrade.push_back(MenuPowersCell());
				else if (infile.section == "tab")
					tabs.push_back(MenuPowersTab());
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
				loadUpgrade(infile, power_cell_upgrade);
		}
		infile.close();
	}

	// fill cell groups with upgrades
	for (size_t i = 0; i < power_cell.size(); ++i) {
		for (size_t j = 1; j < power_cell[i].cells.size(); ++j) {
			for (size_t k = 0; k < power_cell_upgrade.size(); ++k) {
				if (power_cell_upgrade[k].id == power_cell[i].cells[j].id) {
					power_cell[i].cells[j] = power_cell_upgrade[k];
					power_cell[i].cells[j].upgrade_level = static_cast<int>(j) + 1;
					power_cell[i].cells[j].group = i;
					power_cell[i].cells[j-1].next = &power_cell[i].cells[j];
				}
			}
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
			// Define the header.
			for (size_t i=0; i<tabs.size(); i++)
				tab_control->setTabTitle(static_cast<unsigned>(i), msg->get(tabs[i].title));

			// Initialize the tab control.
			tab_control->setMainArea(window_area.x + tab_area.x, window_area.y + tab_area.y);

			tablist.add(tab_control);
		}

		tablist_pow.resize(tabs.size());
	}

	// create power slots
	for (size_t i=0; i<slots.size(); i++) {
		if (!power_cell[i].cells.empty() && static_cast<size_t>(power_cell[i].cells[0].id) < powers->powers.size()) {
			slots[i] = new WidgetSlot(powers->powers[power_cell[i].cells[0].id].icon, Input::ACCEPT);
			slots[i]->setBasePos(power_cell[i].pos.x, power_cell[i].pos.y, Utils::ALIGN_TOPLEFT);

			if (!tablist_pow.empty()) {
				tablist_pow[power_cell[i].tab].add(slots[i]);
				tablist_pow[power_cell[i].tab].setPrevTabList(&tablist);
				tablist_pow[power_cell[i].tab].lock();
			}
			else {
				tablist.add(slots[i]);
			}

			if (power_cell[i].upgrade_button != NULL) {
				power_cell[i].upgrade_button->setBasePos(power_cell[i].pos.x + eset->resolutions.icon_size, power_cell[i].pos.y, Utils::ALIGN_TOPLEFT);
			}
		}
	}

	setUnlockedPowers();

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
			power_cell.back().cells.back().id = id;
		}
		else {
			infile.error("MenuPowers: Power index out of bounds 1-%d, skipping power.", INT_MAX);
		}
		return;
	}

	if (power_cell.back().cells.back().id <= 0) {
		skip_section = true;
		power_cell.pop_back();
		slots.pop_back();
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
			power_cell.back().cells.back().requires_primary[prim_stat_index] = Parse::toInt(infile.val);
		}
		else {
			infile.error("MenuPowers: '%s' is not a valid primary stat.", prim_stat.c_str());
		}
	}
	// @ATTR power.requires_point|bool|Power requires a power point to unlock.
	else if (infile.key == "requires_point") power_cell.back().cells.back().requires_point = Parse::toBool(infile.val);
	// @ATTR power.requires_level|int|Power requires at least this level for the hero.
	else if (infile.key == "requires_level") power_cell.back().cells.back().requires_level = Parse::toInt(infile.val);
	// @ATTR power.requires_power|power_id|Power requires another power id.
	else if (infile.key == "requires_power") power_cell.back().cells.back().requires_power.push_back(Parse::toInt(infile.val));

	// @ATTR power.visible_requires_status|repeatable(string)|Hide the power if we don't have this campaign status.
	else if (infile.key == "visible_requires_status") power_cell.back().cells.back().visible_requires_status.push_back(camp->registerStatus(infile.val));
	// @ATTR power.visible_requires_not_status|repeatable(string)|Hide the power if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell.back().cells.back().visible_requires_not.push_back(camp->registerStatus(infile.val));

	// @ATTR power.upgrades|list(power_id)|A list of upgrade power ids that this power slot can upgrade to. Each of these powers should have a matching upgrade section.
	else if (infile.key == "upgrades") {
		std::string repeat_val = Parse::popFirstString(infile.val);
		while (repeat_val != "") {
			power_cell.back().cells.push_back(MenuPowersCell());
			power_cell.back().cells.back().id = Parse::toInt(repeat_val);
			repeat_val = Parse::popFirstString(infile.val);
		}

		if (power_cell.back().cells.size() > 1) {
			power_cell.back().cells[0].upgrade_level = 1;
			if (!power_cell.back().upgrade_button)
				power_cell.back().upgrade_button = new WidgetButton("images/menus/buttons/button_plus.png");
		}
	}

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

void MenuPowers::loadUpgrade(FileParser &infile, std::vector<MenuPowersCell>& power_cell_upgrade) {
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
	else if (infile.key == "visible_requires_status") power_cell_upgrade.back().visible_requires_status.push_back(camp->registerStatus(infile.val));
	// @ATTR upgrade.visible_requires_not_status|repeatable(string)|Hide the upgrade if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell_upgrade.back().visible_requires_not.push_back(camp->registerStatus(infile.val));

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

bool MenuPowers::checkRequirements(MenuPowersCell* pcell) {
	if (!pcell)
		return false;

	for (size_t i = 0; i < pcell->requires_power.size(); ++i) {
		if (!checkUnlocked(getCellByPowerIndex(pcell->requires_power[i])))
			return false;
	}

	if (pc->stats.level < pcell->requires_level)
		return false;

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (pc->stats.get_primary(i) < pcell->requires_primary[i])
			return false;
	}

	// NOTE if the player is dies, canUsePower() fails and causes passive powers to be locked
	// so we can guard against this be checking player HP > 0
	if (powers->powers[pcell->id].passive && pc->stats.hp > 0) {
		if (!pc->stats.canUsePower(pcell->id, StatBlock::CAN_USE_PASSIVE))
			return false;
	}

	return true;
}

bool MenuPowers::checkUnlocked(MenuPowersCell* pcell) {
	// If this power is not in the menu, than it has no requirements
	if (!pcell)
		return true;

	if (!pcell->isVisible())
		return false;

	// If power_id is saved into vector, it's unlocked anyway
	// check if the unlocked flag is set and check the player's power list
	if (pcell->is_unlocked)
		return true;
	if (std::find(pc->stats.powers_list.begin(), pc->stats.powers_list.end(), pcell->id) != pc->stats.powers_list.end())
		return true;

	// Check the rest of the requirements
	// only check base level; upgrades are checked in logic()
	if (!pcell->requires_point && pcell->upgrade_level <= 1 && checkRequirements(pcell))
		return true;

	return false;
}

/**
 * Check if we can unlock power.
 */
bool MenuPowers::checkUnlock(MenuPowersCell* pcell) {
	// If this power is not in the menu, than it has no requirements
	if (!pcell)
		return true;

	if (!pcell->isVisible())
		return false;

	// If we already have a power, don't try to unlock it
	if (checkUnlocked(pcell))
		return false;

	// Check base requirements
	if (checkRequirements(pcell))
		return true;

	return false;
}

bool MenuPowers::checkUpgrade(MenuPowersCell* pcell) {
	if (!checkUnlocked(pcell))
		return false;

	if (!pcell->next || (pcell->next->requires_point && points_left < 1))
		return false;

	if (!checkUnlock(pcell->next))
		return false;

	return true;
}

void MenuPowers::lockCell(MenuPowersCell* pcell) {
	pcell->is_unlocked = false;

	// remove passive effects
	if (powers->powers[pcell->id].passive && pcell->passive_on) {
		std::vector<int>::iterator passive_it = std::find(pc->stats.powers_passive.begin(), pc->stats.powers_passive.end(), pcell->id);
		if (passive_it != pc->stats.powers_passive.end())
			pc->stats.powers_passive.erase(passive_it);

		pc->stats.effects.removeEffectPassive(pcell->id);
		pcell->passive_on = false;
		pc->stats.refresh_stats = true;
	}

	// remove from player's power list
	std::vector<int>::iterator it = std::find(pc->stats.powers_list.begin(), pc->stats.powers_list.end(), pcell->id);
	if (it != pc->stats.powers_list.end())
		pc->stats.powers_list.erase(it);

	// remove from action bar
	menu->act->addPower(0, pcell->id);

	// lock higher levels as well
	MenuPowersCell* next_cell = pcell->next;
	while (next_cell) {
		lockCell(next_cell);
		next_cell = next_cell->next;
	}
}

bool MenuPowers::isBonusCell(MenuPowersCell* pcell) {
	if (!pcell)
		return false;

	if (power_cell[pcell->group].getBonusLevels() <= 0)
		return false;

	return pcell == power_cell[pcell->group].getBonusCurrent(power_cell[pcell->group].getCurrent());
}

MenuPowersCell* MenuPowers::getCellByPowerIndex(int power_index) {
	// Powers can not have an id of 0
	if (power_index <= 0)
		return NULL;

	// Find cell with our power
	for (size_t i = 0; i < power_cell.size(); ++i) {
		for (size_t j = 0; j < power_cell[i].cells.size(); ++j) {
			if (power_cell[i].cells[j].id == power_index)
				return &power_cell[i].cells[j];
		}
	}

	return NULL;
}

/**
 * Upgrade power cell "pci" to the next level
 */
void MenuPowers::upgradePower(MenuPowersCell* pcell, bool ignore_tab) {
	if (!pcell || !pcell->next)
		return;

	if (!tab_control || ignore_tab || (tab_control && tab_control->getActiveTab() == power_cell[pcell->group].tab)) {
		pcell->next->is_unlocked = true;
		pc->stats.powers_list.push_back(pcell->next->id);
		pc->stats.check_title = true;
	}
	setUnlockedPowers();
}

void MenuPowers::setUnlockedPowers() {
	bool did_cell_lock = false;

	// restore bonus-modified action bar powers before performing upgrades
	clearActionBarBonusLevels();

	for (size_t i = 0; i<power_cell.size(); ++i) {
		for (size_t j = 0; j < power_cell[i].cells.size(); ++j) {
			if (std::find(pc->stats.powers_list.begin(), pc->stats.powers_list.end(), power_cell[i].cells[j].id) != pc->stats.powers_list.end()) {
				power_cell[i].cells[j].is_unlocked = true;
			}
			else {
				if (checkUnlocked(&power_cell[i].cells[j])) {
					// power is unlocked, but not in the player's powers_list
					pc->stats.powers_list.push_back(power_cell[i].cells[j].id);
					power_cell[i].cells[j].is_unlocked = true;
				}
			}

			if (power_cell[i].cells[j].is_unlocked) {
				if (!power_cell[i].cells[j].isVisible() || !checkRequirements(&power_cell[i].cells[j])) {
					lockCell(&power_cell[i].cells[j]);
					did_cell_lock = true;
				}
				else {
					// if power was present in ActionBar, update it there
					if (power_cell[i].current_cell != j)
						menu->act->addPower(power_cell[i].cells[j].id, power_cell[i].getCurrent()->id);

					power_cell[i].current_cell = j;
					if (slots[i])
						slots[i]->setIcon(powers->powers[power_cell[i].cells[j].id].icon, WidgetSlot::NO_OVERLAY);
				}
			}
		}
	}

	// if we locked a cell, we need to re-run this function to make sure the proper current_cell is set
	if (did_cell_lock) {
		setUnlockedPowers();
		return;
	}

	for (size_t i = 0; i < power_cell.size(); ++i) {
		// handle passive powers
		MenuPowersCell* current_pcell = power_cell[i].getCurrent();
		if (!current_pcell->is_unlocked)
			continue;

		MenuPowersCell* bonus_pcell = power_cell[i].getBonusCurrent(current_pcell);

		for (size_t j = 0; j < power_cell[i].cells.size(); ++j) {
			MenuPowersCell* pcell = &power_cell[i].cells[j];

			if (pcell != bonus_pcell || ((!checkRequirements(current_pcell) || (!pcell->is_unlocked && !isBonusCell(pcell))) && pcell->passive_on)) {
				// passive power is activated, but does not meet requirements, so remove it
				std::vector<int>::iterator passive_it = std::find(pc->stats.powers_passive.begin(), pc->stats.powers_passive.end(), pcell->id);
				if (passive_it != pc->stats.powers_passive.end())
					pc->stats.powers_passive.erase(passive_it);

				pc->stats.effects.removeEffectPassive(pcell->id);
				pcell->passive_on = false;
				pc->stats.refresh_stats = true;
			}
			else if (pcell == bonus_pcell && checkRequirements(current_pcell) && !pcell->passive_on) {
				// passive power has not been activated, so activate it here
				std::vector<int>::iterator passive_it = std::find(pc->stats.powers_passive.begin(), pc->stats.powers_passive.end(), pcell->id);
				if (passive_it == pc->stats.powers_passive.end())
					pc->stats.powers_passive.push_back(pcell->id);

				pcell->passive_on = true;
				// for passives without special triggers, we need to trigger them here
				if (pc->stats.effects.triggered_others)
					powers->activateSinglePassive(&pc->stats, pcell->id);
			}
		}

		// update the action bar for powers upgraded via item bonuses
		if (current_pcell != bonus_pcell) {
			menu->act->addPower(bonus_pcell->id, current_pcell->id);
		}
	}
}

int MenuPowers::getPointsUsed() {
	int used = 0;

	for (size_t i = 0; i < pc->stats.powers_list.size(); ++i) {
		MenuPowersCell* pcell = getCellByPowerIndex(pc->stats.powers_list[i]);
		if (pcell && pcell->requires_point)
			used++;
	}

	return used;
}

void MenuPowers::createTooltip(TooltipData* tip_data, MenuPowersCell* pcell, bool show_unlock_prompt) {
	if (!pcell)
		return;

	MenuPowersCell* pcell_bonus = power_cell[pcell->group].getBonusCurrent(pcell);

	const Power &pwr = powers->powers[pcell_bonus->id];

	{
		std::stringstream ss;
		ss << pwr.name;
		if (pcell->upgrade_level > 0) {
			ss << " (" << msg->get("Level %d", pcell->upgrade_level);
			int bonus_levels = power_cell[pcell->group].getBonusLevels();
			if (bonus_levels > 0)
				ss << ", +" << bonus_levels;
			ss << ")";
		}
		tip_data->addText(ss.str());
	}

	if (pwr.passive) tip_data->addText(msg->get("Passive"));
	tip_data->addColoredText(Utils::substituteVarsInString(pwr.description, pc), font->getColor(FontEngine::COLOR_ITEM_FLAVOR));

	// add mana cost
	if (pwr.requires_mp > 0) {
		tip_data->addText(msg->get("Costs %d MP", pwr.requires_mp));
	}
	// add health cost
	if (pwr.requires_hp > 0) {
		tip_data->addText(msg->get("Costs %d HP", pwr.requires_hp));
	}
	// add cooldown time
	if (pwr.cooldown > 0) {
		std::stringstream ss;
		ss << msg->get("Cooldown:") << " " << Utils::getDurationString(pwr.cooldown, 2);
		tip_data->addText(ss.str());
	}

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

			for (int j=0; j<Stats::COUNT; ++j) {
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
						ss << "% " << msg->get("Resistance (%s)", eset->elements.list[j].name);
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

			// NOTE don't need to set found_key after this, since this is the last set of keys to check
			if (!found_key) {
				for (size_t j=0; j<eset->damage_types.list.size(); ++j) {
					if (pwr.post_effects[i].id == eset->damage_types.list[j].min) {
						ss << " " << eset->damage_types.list[j].name_min;
						break;
					}
					else if (pwr.post_effects[i].id == eset->damage_types.list[j].max) {
						ss << " " << eset->damage_types.list[j].name_max;
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
					int magnitude = pc->stats.getDamageMax(pwr.base_damage) * pwr.mod_damage_value_min / 100;
					ss << magnitude;
				}
				else if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_ADD) {
					int magnitude = pc->stats.getDamageMax(pwr.base_damage) + pwr.mod_damage_value_min;
					ss << magnitude;
				}
				else if (pwr.mod_damage_mode == Power::STAT_MODIFIER_MODE_ABSOLUTE) {
					if (pwr.mod_damage_value_max == 0 || pwr.mod_damage_value_min == pwr.mod_damage_value_max)
						ss << pwr.mod_damage_value_min;
					else
						ss << pwr.mod_damage_value_min << "-" << pwr.mod_damage_value_max;
				}
				else {
					ss << pc->stats.getDamageMax(pwr.base_damage);
				}

				ss << " " << msg->get("Magical Shield");
			}
			else if (effect_ptr->type == "heal") {
				if (pwr.base_damage == eset->damage_types.list.size())
					continue;

				int mag_min = pc->stats.getDamageMin(pwr.base_damage);
				int mag_max = pc->stats.getDamageMax(pwr.base_damage);

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
					ss << ": " << Utils::getDurationString(pwr.post_effects[i].duration, 2);
				}
				else {
					ss << " (" << Utils::getDurationString(pwr.post_effects[i].duration, 2) << ")";
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
			ss << msg->get("Elemental Damage (%s)", eset->elements.list[pwr.trait_elemental].name);
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
	}

	std::set<std::string>::iterator it;
	for (it = pwr.requires_flags.begin(); it != pwr.requires_flags.end(); ++it) {
		for (size_t i = 0; i < eset->equip_flags.list.size(); ++i) {
			if ((*it) == eset->equip_flags.list[i].id) {
				tip_data->addText(msg->get("Requires a %s", msg->get(eset->equip_flags.list[i].name)));
			}
		}
	}

	// add requirement
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (pcell->requires_primary[i] > 0) {
			if (pc->stats.get_primary(i) < pcell->requires_primary[i])
				tip_data->addColoredText(msg->get("Requires %s %d", eset->primary_stats.list[i].name, pcell->requires_primary[i]), font->getColor(FontEngine::COLOR_MENU_PENALTY));
			else
				tip_data->addText(msg->get("Requires %s %d", eset->primary_stats.list[i].name, pcell->requires_primary[i]));
		}
	}

	// Draw required Level Tooltip
	if ((pcell->requires_level > 0) && pc->stats.level < pcell->requires_level) {
		tip_data->addColoredText(msg->get("Requires Level %d", pcell->requires_level), font->getColor(FontEngine::COLOR_MENU_PENALTY));
	}
	else if ((pcell->requires_level > 0) && pc->stats.level >= pcell->requires_level) {
		tip_data->addText(msg->get("Requires Level %d", pcell->requires_level));
	}

	for (size_t j=0; j < pcell->requires_power.size(); ++j) {
		MenuPowersCell* req_cell = getCellByPowerIndex(pcell->requires_power[j]);
		if (!req_cell)
			continue;

		std::string req_power_name;
		if (req_cell->upgrade_level > 0)
			req_power_name = powers->powers[req_cell->id].name + " (" + msg->get("Level %d", req_cell->upgrade_level) + ")";
		else
			req_power_name = powers->powers[req_cell->id].name;


		// Required Power Tooltip
		if (!checkUnlocked(req_cell)) {
			tip_data->addColoredText(msg->get("Requires Power: %s", req_power_name), font->getColor(FontEngine::COLOR_MENU_PENALTY));
		}
		else {
			tip_data->addText(msg->get("Requires Power: %s", req_power_name));
		}

	}

	// Draw unlock power Tooltip
	if (pcell->requires_point && !(std::find(pc->stats.powers_list.begin(), pc->stats.powers_list.end(), pcell->id) != pc->stats.powers_list.end())) {
		MenuPowersCell* unlock_cell = getCellByPowerIndex(pcell->id);
		if (show_unlock_prompt && points_left > 0 && checkUnlock(unlock_cell)) {
			tip_data->addColoredText(msg->get("Click to Unlock (uses 1 Skill Point)"), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		else {
			if (pcell->requires_point && points_left < 1)
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
		// Continue if slot is not filled with data
		if (power_cell[i].tab != tab_num) continue;

		MenuPowersCell* slot_cell = power_cell[i].getCurrent();
		if (!slot_cell || !slot_cell->isVisible())
			continue;

		if (slots[i])
			slots[i]->render();

		// upgrade buttons
		if (power_cell[i].upgrade_button)
			power_cell[i].upgrade_button->render();

		// highlighting
		if (checkUnlocked(slot_cell)) {
			Rect src_unlock;

			src_unlock.x = 0;
			src_unlock.y = 0;
			src_unlock.w = eset->resolutions.icon_size;
			src_unlock.h = eset->resolutions.icon_size;

			int selected_slot = -1;
			if (isTabListSelected()) {
				selected_slot = getSelectedCellIndex();
			}

			if (selected_slot == static_cast<int>(i))
				continue;

			if (powers_unlock && slots[i]) {
				powers_unlock->setClipFromRect(src_unlock);
				powers_unlock->setDestFromRect(slots[i]->pos);
				render_device->render(powers_unlock);
			}
		}
		else {
			if (overlay_disabled && slots[i]) {
				overlay_disabled->setClipFromRect(disabled_src);
				overlay_disabled->setDestFromRect(slots[i]->pos);
				render_device->render(overlay_disabled);
			}
		}

		if (slots[i])
			slots[i]->renderSelection();
	}
}

void MenuPowers::logic() {
	if (!visible && tab_control && default_power_tab > -1) {
		tab_control->setActiveTab(static_cast<unsigned>(default_power_tab));
	}

	setUnlockedPowers();

	for (size_t i=0; i<power_cell.size(); i++) {
		// make sure invisible cells are skipped in the tablist
		if (slots[i])
			slots[i]->enable_tablist_nav = power_cell[i].getCurrent()->isVisible();

		//upgrade buttons logic
		if (power_cell[i].upgrade_button != NULL) {
			power_cell[i].upgrade_button->enabled = false;
			if (pc->stats.hp > 0) {
				// enable button only if current level is unlocked and next level can be unlocked
				if (checkUpgrade(power_cell[i].getCurrent())) {
					power_cell[i].upgrade_button->enabled = true;
				}
				if ((!tab_control || power_cell[i].tab == tab_control->getActiveTab()) && power_cell[i].upgrade_button->checkClick()) {
					upgradePower(power_cell[i].getCurrent(), !UPGRADE_POWER_ALL_TABS);
				}
			}
		}
	}

	points_left = (pc->stats.level * pc->stats.power_points_per_level) - getPointsUsed();
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
					r->setClipFromRect(src);
					r->setDestFromRect(dest);
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
			r->setClipFromRect(src);
			r->setDestFromRect(dest);
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

		if (tab_control && (tab_control->getActiveTab() != power_cell[i].tab))
			continue;

		MenuPowersCell* tip_cell = power_cell[i].getCurrent();
		if (!tip_cell->isVisible())
			continue;

		if (slots[i] && Utils::isWithinRect(slots[i]->pos, position)) {
			bool base_unlocked = checkUnlocked(tip_cell);

			createTooltip(&tip_data, tip_cell, !base_unlocked);
			if (tip_cell->next) {
				tip_data.addText("\n" + msg->get("Next Level:"));
				createTooltip(&tip_data, tip_cell->next, base_unlocked);
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

			MenuPowersCell* pcell = power_cell[i].getCurrent();
			if (!pcell)
				return 0;

			if (checkUnlock(pcell) && points_left > 0 && pcell->requires_point) {
				// unlock power
				pc->stats.powers_list.push_back(pcell->id);
				pc->stats.check_title = true;
				setUnlockedPowers();
				menu->act->addPower(pcell->id, 0);
				return 0;
			}
			else if (checkUnlocked(pcell) && !powers->powers[pcell->id].passive) {
				// pick up and drag power
				if (inpt->usingMouse()) {
					slots[i]->defocus();
					if (!tabs.empty()) {
						tablist_pow[active_tab].setCurrent(NULL);
					}
					else {
						tablist.setCurrent(NULL);
					}
				}
				return power_cell[i].getBonusCurrent(pcell)->id;
			}
			else
				return 0;
		}
	}

	// nothing selected, defocus everything
	defocusTabLists();

	return 0;
}

void MenuPowers::upgradeBySlotIndex(int slot_index) {
	MenuPowersCell* pcell = power_cell[slot_index].getCurrent();
	if (checkUpgrade(pcell))
		upgradePower(pcell, !UPGRADE_POWER_ALL_TABS);
}

void MenuPowers::resetToBasePowers() {
	for (size_t i = 0; i < power_cell.size(); ++i) {
		power_cell[i].current_cell = 0;
		for (size_t j = 0; j < power_cell[i].cells.size(); ++j) {
			power_cell[i].cells[j].is_unlocked = false;
			power_cell[i].cells[j].passive_on = false;
		}
	}

	setUnlockedPowers();
}

/**
 * Return true if required stats for power usage are met. Else return false.
 */
bool MenuPowers::meetsUsageStats(int power_index) {
	// Find cell with our power
	MenuPowersCell* pcell = getCellByPowerIndex(power_index);

	// If we didn't find power in power_menu, than it has no stats requirements
	if (!pcell)
		return true;

	// ignore bonuses to power level
	MenuPowersCell* base_pcell = power_cell[pcell->group].getCurrent();

	if (pc->stats.level < base_pcell->requires_level)
		return false;

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (pc->stats.get_primary(i) < base_pcell->requires_primary[i])
			return false;
	}

	return true;
}

void MenuPowers::clearActionBarBonusLevels() {
	for (size_t i = 0; i < power_cell.size(); ++i) {
		if (power_cell[i].getBonusLevels() > 0) {
			MenuPowersCell* pcell = power_cell[i].getCurrent();
			menu->act->addPower(pcell->id, power_cell[i].getBonusCurrent(pcell)->id);
		}
	}
}

void MenuPowers::clearBonusLevels() {
	clearActionBarBonusLevels();

	for (size_t i = 0; i < power_cell.size(); ++i) {
		power_cell[i].bonus_levels.clear();
	}
}

void MenuPowers::addBonusLevels(int power_index, int bonus_levels) {
	MenuPowersCell* pcell = getCellByPowerIndex(power_index);

	if (!pcell)
		return;

	MenuPowersCellGroup* pgroup = &power_cell[pcell->group];

	size_t min_level = pgroup->cells.size() - 1;
	for (size_t i = 0; i < pgroup->cells.size(); ++i) {
		if (pcell == &pgroup->cells[i]) {
			min_level = i;
			break;
		}
	}

	std::pair<size_t, int> bonus(min_level, bonus_levels);
	pgroup->bonus_levels.push_back(bonus);
}

std::string MenuPowers::getItemBonusPowerReqString(int power_index) {
	MenuPowersCell* pcell = getCellByPowerIndex(power_index);

	if (!pcell)
		return "";

	std::stringstream ss;
	ss << powers->powers[power_index].name;
	if (pcell->upgrade_level > 0) {
		ss << " (" << msg->get("Level %d", pcell->upgrade_level) << ")";
	}

	return ss.str();
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

