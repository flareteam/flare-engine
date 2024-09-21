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
#include "MenuInventory.h"
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
	: id(0)
	, requires_point(false)
	, requires_level(0)
	, requires_primary(eset->primary_stats.list.size(), 0)
	, requires_power()
	, requires_status()
	, requires_not_status()
	, visible(true)
	, visible_check_locked(false)
	, visible_check_status(false)
	, upgrade_level(0)
	, passive_on(false)
	, is_unlocked(false)
	, group(0)
	, next(NULL) {
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
	, points_left(0)
	, default_background("")
	, label_powers(new WidgetLabel)
	, label_unspent(new WidgetLabel)
	, tab_control(NULL)
	, tree_loaded(false)
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
		tab_control->setMainArea(window_area.x + tab_area.x, window_area.y + tab_area.y, tab_area.w);
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
	if (!background)
		setBackground("images/menus/powers.png");
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
					MenuPowersCellGroup cell_group;
					MenuPowersCell base_level;

					base_level.group = power_cell.size();
					cell_group.cells.push_back(base_level);

					power_cell.push_back(cell_group);

					slots.push_back(NULL);
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
				Sprite* tree_sprite = graphics->createSprite();

				if (background && (background->getGraphicsWidth() != tree_sprite->getGraphicsWidth() || background->getGraphicsHeight() != tree_sprite->getGraphicsHeight())) {
					tabs[i].background_is_menu_size = false;
				}

				tree_surf.push_back(tree_sprite);
				graphics->unref();
			}
			else {
				tree_surf.push_back(NULL);
			}
		}
	}

	// If we have more than one tab, create tab_control
	if (!tabs.empty()) {
		tablist_pow.resize(tabs.size());

		tab_control = new WidgetTabControl();

		if (tab_control) {
			// Define the header.
			for (size_t i=0; i<tabs.size(); i++) {
				tab_control->setupTab(static_cast<unsigned>(i), msg->get(tabs[i].title), &tablist_pow[i]);
			}

			// Initialize the tab control.
			tab_control->setMainArea(window_area.x + tab_area.x, window_area.y + tab_area.y, tab_area.w);
		}
	}

	// create power slots
	for (size_t i=0; i<slots.size(); i++) {
		if (!power_cell[i].cells.empty() && powers->isValid(power_cell[i].cells[0].id)) {
			slots[i] = new WidgetSlot(powers->powers[power_cell[i].cells[0].id]->icon, WidgetSlot::HIGHLIGHT_POWER_MENU);
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
	MenuPowersCellGroup& cell_group = power_cell.back();

	// base power cell storage hasn't been set up!
	if (cell_group.cells.empty())
		return;

	// @ATTR power.id|power_id|A power id from powers/powers.txt for this slot.
	if (infile.key == "id") {
		PowerID id = powers->verifyID(Parse::toPowerID(Parse::popFirstString(infile.val)), &infile, !PowerManager::ALLOW_ZERO_ID);
		if (id > 0) {
			skip_section = false;
			cell_group.cells[0].id = id;
		}
		return;
	}

	if (cell_group.cells[0].id == 0) {
		skip_section = true;
		power_cell.pop_back();
		slots.pop_back();
		infile.error("MenuPowers: Power without ID as first attribute. Skipping section.");
	}

	if (skip_section)
		return;

	if (infile.key == "tab") {
		// @ATTR power.tab|int|Tab index to place this power on, starting from 0.
		cell_group.tab = Parse::toInt(infile.val);
	}
	else if (infile.key == "position") {
		// @ATTR power.position|point|Position of this power icon; relative to MenuPowers "pos".
		cell_group.pos = Parse::toPoint(infile.val);
	}

	else if (infile.key == "requires_point") {
		// @ATTR power.requires_point|bool|Power requires a power point to unlock.
		cell_group.cells[0].requires_point = Parse::toBool(infile.val);
	}
	else if (infile.key == "requires_primary") {
		// @ATTR power.requires_primary|predefined_string, int : Primary stat name, Required value|Power requires this primary stat to be at least the specificed value.
		std::string prim_stat = Parse::popFirstString(infile.val);
		size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

		if (prim_stat_index != eset->primary_stats.list.size()) {
			cell_group.cells[0].requires_primary[prim_stat_index] = Parse::toInt(infile.val);
		}
		else {
			infile.error("MenuPowers: '%s' is not a valid primary stat.", prim_stat.c_str());
		}
	}
	else if (infile.key == "requires_level") {
		// @ATTR power.requires_level|int|Power requires at least this level for the hero.
		cell_group.cells[0].requires_level = Parse::toInt(infile.val);
	}
	else if (infile.key == "requires_power") {
		// @ATTR power.requires_power|power_id|Power requires another power id.
		PowerID power_id = powers->verifyID(Parse::toPowerID(infile.val), &infile, !PowerManager::ALLOW_ZERO_ID);
		if (power_id != 0)
			cell_group.cells[0].requires_power.push_back(power_id);
	}
	else if (infile.key == "requires_status") {
		// @ATTR power.requires_status|repeatable(string)|Power requires this campaign status.
		cell_group.cells[0].requires_status.push_back(camp->registerStatus(infile.val));
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR power.requires_not_status|repeatable(string)|Power requires not having this campaign status.
		cell_group.cells[0].requires_not_status.push_back(camp->registerStatus(infile.val));
	}
	else if (infile.key == "visible_requires_status") {
		// @ATTR power.visible_requires_status|repeatable(string)|(Deprecated as of v1.11.75) Hide the power if we don't have this campaign status.
		infile.error("MenuPowers: visible_requires_status is deprecated. Use requires_status and visible_check_status=true instead.");
		cell_group.cells[0].requires_status.push_back(camp->registerStatus(infile.val));
		cell_group.cells[0].visible_check_status = true;
	}
	else if (infile.key == "visible_requires_not_status") {
		// @ATTR power.visible_requires_not_status|repeatable(string)|(Deprecated as of v1.11.75) Hide the power if we have this campaign status.
		infile.error("MenuPowers: visible_requires_not_status is deprecated. Use requires_not_status and visible_check_status=true instead.");
		cell_group.cells[0].requires_not_status.push_back(camp->registerStatus(infile.val));
		cell_group.cells[0].visible_check_status = true;
	}
	else if (infile.key == "upgrades") {
		// @ATTR power.upgrades|list(power_id)|A list of upgrade power ids that this power slot can upgrade to. Each of these powers should have a matching upgrade section.
		std::string repeat_val = Parse::popFirstString(infile.val);
		while (repeat_val != "") {
			PowerID test_id = powers->verifyID(Parse::toPowerID(repeat_val), &infile, !PowerManager::ALLOW_ZERO_ID);
			if (test_id != 0) {
				if (test_id == cell_group.cells[0].id) {
					infile.error("MenuPowers: Upgrade ID '%d' is the same as the base ID. Ignoring.", test_id);
				}
				else {
					MenuPowersCell upgrade_cell;
					upgrade_cell.id = test_id;
					cell_group.cells.push_back(upgrade_cell);
				}
			}
			repeat_val = Parse::popFirstString(infile.val);
		}

		if (cell_group.cells.size() > 1) {
			cell_group.cells[0].upgrade_level = 1;
			if (!cell_group.upgrade_button)
				cell_group.upgrade_button = new WidgetButton("images/menus/buttons/button_plus.png");
		}
	}
	else if (infile.key == "visible") {
		// @ATTR power.visible|bool|Controls whether or not a power is visible or hidden regardless of unlocked state. Defaults to true.
		cell_group.cells[0].visible = Parse::toBool(infile.val);
	}
	else if (infile.key == "visible_check_locked") {
		// @ATTR power.visible_check_locked|bool|When set to true, the power will be hidden if it is locked. Defaults to false.
		cell_group.cells[0].visible_check_locked = Parse::toBool(infile.val);
	}
	else if (infile.key == "visible_check_status") {
		// @ATTR power.visible_check_status|bool|When set to true, the power will be hidden if its status requirements are not met. Defaults to false.
		cell_group.cells[0].visible_check_status = Parse::toBool(infile.val);
	}
	else {
		infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
	}
}

void MenuPowers::loadUpgrade(FileParser &infile, std::vector<MenuPowersCell>& power_cell_upgrade) {
	if (power_cell_upgrade.empty())
		return;

	MenuPowersCell& cell = power_cell_upgrade.back();

	// @ATTR upgrade.id|power_id|A power id from powers/powers.txt for this upgrade.
	if (infile.key == "id") {
		PowerID id = powers->verifyID(Parse::toPowerID(Parse::popFirstString(infile.val)), &infile, !PowerManager::ALLOW_ZERO_ID);
		if (id > 0) {
			skip_section = false;
			cell.id = id;
		}
		return;
	}

	if (cell.id == 0) {
		skip_section = true;
		power_cell_upgrade.pop_back();
		infile.error("MenuPowers: Upgrade without ID as first attribute. Skipping section.");
	}

	if (skip_section)
		return;

	// @ATTR upgrade.requires_primary|predefined_string, int : Primary stat name, Required value|Upgrade requires this primary stat to be at least the specificed value.
	if (infile.key == "requires_primary") {
		std::string prim_stat = Parse::popFirstString(infile.val);
		size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

		if (prim_stat_index != eset->primary_stats.list.size()) {
			cell.requires_primary[prim_stat_index] = Parse::toInt(infile.val);
		}
		else {
			infile.error("MenuPowers: '%s' is not a valid primary stat.", prim_stat.c_str());
		}
	}
	else if (infile.key == "requires_point") {
		// @ATTR upgrade.requires_point|bool|Upgrade requires a power point to unlock.
		cell.requires_point = Parse::toBool(infile.val);
	}
	else if (infile.key == "requires_level") {
		// @ATTR upgrade.requires_level|int|Upgrade requires at least this level for the hero.
		cell.requires_level = Parse::toInt(infile.val);
	}
	else if (infile.key == "requires_power") {
		// @ATTR upgrade.requires_power|int|Upgrade requires another power id.
		PowerID power_id = powers->verifyID(Parse::toPowerID(infile.val), &infile, !PowerManager::ALLOW_ZERO_ID);
		if (power_id != 0)
			cell.requires_power.push_back(power_id);
	}
	else if (infile.key == "requires_status") {
		// @ATTR upgrade.requires_status|repeatable(string)|Upgrade requires this campaign status.
		cell.requires_status.push_back(camp->registerStatus(infile.val));
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR upgrade.requires_not_status|repeatable(string)|Upgrade requires not having this campaign status.
		cell.requires_not_status.push_back(camp->registerStatus(infile.val));
	}
	else if (infile.key == "visible_requires_status") {
		// @ATTR upgrade.visible_requires_status|repeatable(string)|(Deprecated as of v1.11.75) Hide the upgrade if we don't have this campaign status.
		infile.error("MenuPowers: visible_requires_status is deprecated. Use requires_status and visible_check_status=true instead.");
		cell.requires_status.push_back(camp->registerStatus(infile.val));
		cell.visible_check_status = true;
	}
	else if (infile.key == "visible_requires_not_status") {
		// @ATTR upgrade.visible_requires_not_status|repeatable(string)|(Deprecated as of v1.11.75) Hide the upgrade if we have this campaign status.
		infile.error("MenuPowers: visible_requires_not_status is deprecated. Use requires_not_status and visible_check_status=true instead.");
		cell.requires_not_status.push_back(camp->registerStatus(infile.val));
		cell.visible_check_status = true;
	}
	else if (infile.key == "visible") {
		// @ATTR upgrade.visible|bool|Controls whether or not a power is visible or hidden regardless of unlocked state. Defaults to true.
		cell.visible = Parse::toBool(infile.val);
	}
	else if (infile.key == "visible_check_locked") {
		// @ATTR upgrade.visible_check_locked|bool|When set to true, the power will be hidden if it is locked. Defaults to false.
		cell.visible_check_locked = Parse::toBool(infile.val);
	}
	else if (infile.key == "visible_check_status") {
		// @ATTR upgrade.visible_check_status|bool|When set to true, the power will be hidden if its status requirements are not met. Defaults to false.
		cell.visible_check_status = Parse::toBool(infile.val);
	}
	else {
		infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
	}
}

bool MenuPowers::checkRequirements(MenuPowersCell* pcell) {
	if (!pcell)
		return false;

	if (pc->stats.level < pcell->requires_level)
		return false;

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (pc->stats.get_primary(i) < pcell->requires_primary[i])
			return false;
	}

	for (size_t i = 0; i < pcell->requires_status.size(); ++i)
		if (!camp->checkStatus(pcell->requires_status[i]))
			return false;

	for (size_t i = 0; i < pcell->requires_not_status.size(); ++i)
		if (camp->checkStatus(pcell->requires_not_status[i]))
			return false;

	for (size_t i = 0; i < pcell->requires_power.size(); ++i) {
		if (!checkUnlocked(getCellByPowerIndex(pcell->requires_power[i])))
			return false;
	}

	// NOTE if the player is dies, canUsePower() fails and causes passive powers to be locked
	// so we can guard against this be checking player HP > 0
	if (powers->isValid(pcell->id) && powers->powers[pcell->id]->passive && pc->stats.hp > 0) {
		if (!pc->stats.canUsePower(pcell->id, StatBlock::CAN_USE_PASSIVE))
			return false;
	}

	return true;
}

bool MenuPowers::checkRequirementStatus(MenuPowersCell* pcell) {
	if (!pcell)
		return false;

	for (size_t i = 0; i < pcell->requires_status.size(); ++i)
		if (!camp->checkStatus(pcell->requires_status[i]))
			return false;

	for (size_t i = 0; i < pcell->requires_not_status.size(); ++i)
		if (camp->checkStatus(pcell->requires_not_status[i]))
			return false;

	return true;
}

bool MenuPowers::checkUnlocked(MenuPowersCell* pcell) {
	// If this power is not in the menu, than it has no requirements
	if (!pcell)
		return true;

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
	if (powers->powers[pcell->id]->passive && pcell->passive_on) {
		std::vector<PowerID>::iterator passive_it = std::find(pc->stats.powers_passive.begin(), pc->stats.powers_passive.end(), pcell->id);
		if (passive_it != pc->stats.powers_passive.end())
			pc->stats.powers_passive.erase(passive_it);

		pc->stats.effects.removeEffectPassive(pcell->id);
		pcell->passive_on = false;
		pc->stats.refresh_stats = true;
	}

	// remove from player's power list
	std::vector<PowerID>::iterator it = std::find(pc->stats.powers_list.begin(), pc->stats.powers_list.end(), pcell->id);
	if (it != pc->stats.powers_list.end())
		pc->stats.powers_list.erase(it);

	// remove from action bar
	menu->act->addPower(0, pcell->id);

	// lock higher levels as well (careful: recursion)
	if (pcell->next) {
		lockCell(pcell->next);
	}
}

bool MenuPowers::isBonusCell(MenuPowersCell* pcell) {
	if (!pcell)
		return false;

	if (power_cell[pcell->group].getBonusLevels() <= 0)
		return false;

	return pcell == power_cell[pcell->group].getBonusCurrent(power_cell[pcell->group].getCurrent());
}

bool MenuPowers::isCellVisible(MenuPowersCell* pcell) {
	if (!pcell)
		return false;

	if (!pcell->visible)
		return false;

	if (pcell->visible_check_status && !checkRequirementStatus(pcell))
		return false;
	else if (pcell->visible_check_locked && !checkUnlocked(pcell))
		return false;

	return true;
}

MenuPowersCell* MenuPowers::getCellByPowerIndex(PowerID power_index) {
	// Powers can not have an id of 0
	if (power_index == 0)
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

	if (!tab_control || ignore_tab || tab_control->getActiveTab() == power_cell[pcell->group].tab) {
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
			if (std::find(recently_locked_cells.begin(), recently_locked_cells.end(), &power_cell[i].cells[j]) == recently_locked_cells.end()) {
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
			}

			if (power_cell[i].cells[j].is_unlocked) {
				if (!checkRequirements(&power_cell[i].cells[j])) {
					lockCell(&power_cell[i].cells[j]);
					did_cell_lock = true;

					// We're going to recursively call setUnlockedPowers() at this point.
					// We save a list of powers locked here so that we don't unlock them again in this cycle,
					// which would result in an infinite loop.
					recently_locked_cells.push_back(&power_cell[i].cells[j]);
				}
				else {
					// if power was present in ActionBar, update it there
					if (power_cell[i].current_cell != j)
						menu->act->addPower(power_cell[i].cells[j].id, power_cell[i].getCurrent()->id);

					power_cell[i].current_cell = j;
					if (slots[i])
						slots[i]->setIcon(powers->powers[power_cell[i].cells[j].id]->icon, WidgetSlot::NO_OVERLAY);
				}
			}
		}
	}

	// if we locked a cell, we need to re-run this function to make sure the proper current_cell is set
	if (did_cell_lock) {
		setUnlockedPowers();
		return;
	}

	recently_locked_cells.clear();

	for (size_t i = 0; i < power_cell.size(); ++i) {
		// handle passive powers
		MenuPowersCell* current_pcell = power_cell[i].getCurrent();
		if (!current_pcell->is_unlocked)
			continue;

		MenuPowersCell* bonus_pcell = power_cell[i].getBonusCurrent(current_pcell);

		for (size_t j = 0; j < power_cell[i].cells.size(); ++j) {
			MenuPowersCell* pcell = &power_cell[i].cells[j];

			if (pcell != bonus_pcell || (pcell->passive_on && powers->powers[pcell->id]->passive && (!checkRequirements(current_pcell) || (!pcell->is_unlocked && !isBonusCell(pcell))))) {
				// passive power is activated, but does not meet requirements, so remove it
				std::vector<PowerID>::iterator passive_it = std::find(pc->stats.powers_passive.begin(), pc->stats.powers_passive.end(), pcell->id);
				if (passive_it != pc->stats.powers_passive.end()) {
					pc->stats.powers_passive.erase(passive_it);

					pc->stats.effects.removeEffectPassive(pcell->id);
					pcell->passive_on = false;
					pc->stats.refresh_stats = true;

					// passive powers can lock equipment slots, so update equipment here
					menu->inv->applyEquipment();
				}
			}
			else if (pcell == bonus_pcell && !pcell->passive_on && powers->powers[pcell->id]->passive && checkRequirements(current_pcell)) {
				// passive power has not been activated, so activate it here
				std::vector<PowerID>::iterator passive_it = std::find(pc->stats.powers_passive.begin(), pc->stats.powers_passive.end(), pcell->id);
				if (passive_it == pc->stats.powers_passive.end()) {
					pc->stats.powers_passive.push_back(pcell->id);

					pcell->passive_on = true;
					// for passives without special triggers, we need to trigger them here
					if (pc->stats.effects.triggered_others)
						powers->activateSinglePassive(&pc->stats, pcell->id);

					// passive powers can lock equipment slots, so update equipment here
					menu->inv->applyEquipment();
				}
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

void MenuPowers::createTooltipFromActionBar(TooltipData* tip_data, unsigned slot, int tooltip_length) {
	if (slot >= menu->act->hotkeys.size() || slot >= menu->act->hotkeys_mod.size())
		return;

	PowerID power_index = menu->act->hotkeys[slot];
	PowerID mod_power_index = menu->act->hotkeys_mod[slot];

	PowerID pindex = mod_power_index;
	MenuPowersCell* pcell = getCellByPowerIndex(pindex);

	// action bar slot is modded and not found in the menu
	if (power_index != mod_power_index && !pcell) {
		PowerID test_pindex = power_index;
		MenuPowersCell* test_pcell = getCellByPowerIndex(test_pindex);

		// non-modded power found in the menu; use it instead
		if (test_pcell) {
			pindex = test_pindex;
			pcell = test_pcell;
		}
		// else, neither is found in the menu, so default to the modded power
	}

	// if the power is upgraded, we need to get the base power
	MenuPowersCell* pcell_base = NULL;
	if (pcell) {
		pcell_base = power_cell[pcell->group].getCurrent();
	}

	createTooltip(tip_data, pcell_base, pindex, false, tooltip_length);
	createTooltipInputHint(tip_data, TOOLTIP_SHOW_ACTIVATE_HINT);
}

void MenuPowers::createTooltip(TooltipData* tip_data, MenuPowersCell* pcell, PowerID power_index, bool show_unlock_prompt, int tooltip_length) {

	MenuPowersCell* pcell_bonus = NULL;
	if (pcell) {
		pcell_bonus = power_cell[pcell->group].getBonusCurrent(pcell);
	}
	const Power* pwr = pcell_bonus ? powers->powers[pcell_bonus->id] : powers->powers[power_index];

	{
		std::stringstream ss;
		ss << pwr->name;
		if (pcell && pcell->upgrade_level > 0) {
			ss << " (" << msg->getv("Level %d", pcell->upgrade_level);
			int bonus_levels = power_cell[pcell->group].getBonusLevels();
			if (bonus_levels > 0)
				ss << ", +" << bonus_levels;
			ss << ")";
		}
		tip_data->addText(ss.str());
	}

	if (tooltip_length == MenuPowers::TOOLTIP_SHORT || (!pcell && tooltip_length != MenuPowers::TOOLTIP_LONG_ALL))
		return;

	if (pwr->passive) tip_data->addText(msg->get("Passive"));
	if (pwr->description != "") {
		tip_data->addColoredText(Utils::substituteVarsInString(pwr->description, pc), font->getColor(FontEngine::COLOR_ITEM_FLAVOR));
	}

	// add mana cost
	if (pwr->requires_mp > 0) {
		tip_data->addText(msg->getv("Costs %s MP", Utils::floatToString(pwr->requires_mp, eset->number_format.power_tooltips).c_str()));
	}
	// add health cost
	if (pwr->requires_hp > 0) {
		tip_data->addText(msg->getv("Costs %s HP", Utils::floatToString(pwr->requires_hp, eset->number_format.power_tooltips).c_str()));
	}
	// add resource stat cost
	for (size_t i = 0; i < pwr->requires_resource_stat.size(); ++i) {
		if (pwr->requires_resource_stat[i] > 0) {
			tip_data->addText(eset->resource_stats.list[i].text_tooltip_cost + ": " + Utils::floatToString(pwr->requires_resource_stat[i], eset->number_format.power_tooltips));
		}
	}
	// add cooldown time
	if (pwr->cooldown > 0) {
		tip_data->addText(msg->get("Cooldown:") + " " + Utils::getDurationString(pwr->cooldown, eset->number_format.durations));
	}

	if (pwr->use_hazard || pwr->type == Power::TYPE_REPEATER) {
		std::stringstream ss;

		// modifier_damage
		if (pwr->mod_damage_mode > -1) {
			if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_ADD && pwr->mod_damage_value_min > 0)
				ss << "+";

			if (pwr->mod_damage_value_max == 0 || pwr->mod_damage_value_min == pwr->mod_damage_value_max) {
				ss << Utils::floatToString(pwr->mod_damage_value_min, eset->number_format.power_tooltips);
			}
			else {
				ss << Utils::floatToString(pwr->mod_damage_value_min, eset->number_format.power_tooltips) << "-" << Utils::floatToString(pwr->mod_damage_value_max, eset->number_format.power_tooltips);
			}

			if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			if (pwr->base_damage != eset->damage_types.list.size()) {
				ss << eset->damage_types.list[pwr->base_damage].name;
			}

			if (pwr->count > 1 && pwr->type != Power::TYPE_REPEATER)
				ss << " (x" << pwr->count << ")";

			if (!ss.str().empty())
				tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		else {
			if (pwr->base_damage != eset->damage_types.list.size()) {
				tip_data->addText(eset->damage_types.list[pwr->base_damage].name);
			}
		}
	}

	for (size_t i=0; i<pwr->post_effects.size(); ++i) {
		std::stringstream ss;
		EffectDef* effect_ptr = powers->getEffectDef(pwr->post_effects[i].id);

		int effect_type = Effect::NONE;
		if (effect_ptr) {
			effect_type = effect_ptr->type;
		}
		else {
			effect_type = Effect::getTypeFromString(pwr->post_effects[i].id);
		}

		if (Effect::typeIsStat(effect_type) ||
		    Effect::typeIsDmgMin(effect_type) ||
		    Effect::typeIsDmgMax(effect_type) ||
		    Effect::typeIsResist(effect_type) ||
		    Effect::typeIsPrimary(effect_type) ||
		    Effect::typeIsResourceStat(effect_type))
		{
			if (pwr->post_effects[i].is_multiplier)
				ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips + 2) << "×";
			else if (pwr->post_effects[i].magnitude > 0)
				ss << "+" << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips);
			else
				ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips);
		}

		if (Effect::typeIsStat(effect_type)) {
			int index = Effect::getStatFromType(effect_type);
			if (Stats::PERCENT[index] && !pwr->post_effects[i].is_multiplier) {
				ss << "%";
			}
			ss << " " << Stats::NAME[index];
		}
		else if (Effect::typeIsDmgMin(effect_type)) {
			size_t index = Effect::getDmgFromType(effect_type);
			ss << " " << eset->damage_types.list[index].name_min;
		}
		else if (Effect::typeIsDmgMax(effect_type)) {
			size_t index = Effect::getDmgFromType(effect_type);
			ss << " " << eset->damage_types.list[index].name_max;
		}
		else if (Effect::typeIsResist(effect_type)) {
			size_t index = Effect::getResistFromType(effect_type);
			if (!pwr->post_effects[i].is_multiplier) {
				ss << "%";
			}
			ss << " " << msg->getv("Resistance (%s)", eset->elements.list[index].name.c_str());
		}
		else if (Effect::typeIsPrimary(effect_type)) {
			size_t index = Effect::getPrimaryFromType(effect_type);
			ss << " " << eset->primary_stats.list[index].name;
		}
		else if (Effect::typeIsResourceStat(effect_type)) {
			size_t index = Effect::getResourceStatFromType(effect_type);
			size_t sub_index = Effect::getResourceStatSubIndexFromType(effect_type);

			if (!pwr->post_effects[i].is_multiplier && (sub_index == EngineSettings::ResourceStats::STAT_STEAL || sub_index == EngineSettings::ResourceStats::STAT_RESIST_STEAL)) {
				ss << "%";
			}
			ss << " " << eset->resource_stats.list[index].text[sub_index];
		}
		else if (Effect::typeIsResourceEffect(effect_type)) {
			size_t index = Effect::getResourceStatFromType(effect_type);
			size_t sub_index = Effect::getResourceStatSubIndexFromType(effect_type);

			if (sub_index == EngineSettings::ResourceStats::STAT_HEAL) {
				ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << " " << eset->resource_stats.list[index].text_tooltip_heal;
			}
			else if (sub_index == EngineSettings::ResourceStats::STAT_HEAL_PERCENT) {
				ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << "% " << eset->resource_stats.list[index].text_tooltip_heal;
			}
		}
		else if (effect_type == Effect::DAMAGE) {
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << " " << msg->get("Damage per second");
		}
		else if (effect_type == Effect::DAMAGE_PERCENT) {
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << "% " << msg->get("Damage per second");
		}
		else if (effect_type == Effect::HPOT) {
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << " " << msg->get("HP per second");
		}
		else if (effect_type == Effect::HPOT_PERCENT) {
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << "% " << msg->get("HP per second");
		}
		else if (effect_type == Effect::MPOT) {
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << " " << msg->get("MP per second");
		}
		else if (effect_type == Effect::MPOT_PERCENT) {
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << "% " << msg->get("MP per second");
		}
		else if (effect_type == Effect::SPEED) {
			if (pwr->post_effects[i].magnitude == 0)
				ss << msg->get("Immobilize");
			else
				ss << msg->getv("%s%% Speed", Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips).c_str());
		}
		else if (effect_type == Effect::ATTACK_SPEED) {
			ss << msg->getv("%s%% Attack Speed", Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips).c_str());
		}
		else if (effect_type == Effect::RESIST_ALL) {
			ss << "+" << pwr->post_effects[i].magnitude << "% " << msg->get("Resistance to all negative effects");
		}

		else if (effect_type == Effect::STUN) {
			ss << msg->get("Stun");
		}
		else if (effect_type == Effect::REVIVE) {
			ss << msg->get("Automatic revive on death");
		}
		else if (effect_type == Effect::CONVERT) {
			ss << msg->get("Convert");
		}
		else if (effect_type == Effect::FEAR) {
			ss << msg->get("Fear");
		}
		else if (effect_type == Effect::DEATH_SENTENCE) {
			ss << msg->get("Lifespan");
		}
		else if (effect_type == Effect::SHIELD) {
			if (pwr->base_damage == eset->damage_types.list.size())
				continue;

			if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				float magnitude = pc->stats.getDamageMax(pwr->base_damage) * pwr->mod_damage_value_min / 100;
				ss << Utils::floatToString(magnitude, eset->number_format.power_tooltips);
			}
			else if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_ADD) {
				float magnitude = pc->stats.getDamageMax(pwr->base_damage) + pwr->mod_damage_value_min;
				ss << Utils::floatToString(magnitude, eset->number_format.power_tooltips);
			}
			else if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_ABSOLUTE) {
				if (pwr->mod_damage_value_max == 0 || pwr->mod_damage_value_min == pwr->mod_damage_value_max)
					ss << Utils::floatToString(pwr->mod_damage_value_min, eset->number_format.power_tooltips);
				else
					ss << Utils::floatToString(pwr->mod_damage_value_min, eset->number_format.power_tooltips) << "-" << Utils::floatToString(pwr->mod_damage_value_max, eset->number_format.power_tooltips);
			}
			else {
				ss << Utils::floatToString(pc->stats.getDamageMax(pwr->base_damage), eset->number_format.power_tooltips);
			}

			ss << " " << msg->get("Magical Shield");
		}
		else if (effect_type == Effect::HEAL) {
			if (pwr->base_damage == eset->damage_types.list.size())
				continue;

			float mag_min = pc->stats.getDamageMin(pwr->base_damage);
			float mag_max = pc->stats.getDamageMax(pwr->base_damage);

			if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				mag_min = mag_min * pwr->mod_damage_value_min / 100;
				mag_max = mag_max * pwr->mod_damage_value_min / 100;
				ss << Utils::floatToString(mag_min, eset->number_format.power_tooltips) << "-" << Utils::floatToString(mag_max, eset->number_format.power_tooltips);
			}
			else if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_ADD) {
				mag_min = mag_min + pwr->mod_damage_value_min;
				mag_max = mag_max + pwr->mod_damage_value_min;
				ss << Utils::floatToString(mag_min, eset->number_format.power_tooltips) << "-" << Utils::floatToString(mag_max, eset->number_format.power_tooltips);
			}
			else if (pwr->mod_damage_mode == Power::STAT_MODIFIER_MODE_ABSOLUTE) {
				if (pwr->mod_damage_value_max == 0 || pwr->mod_damage_value_min == pwr->mod_damage_value_max)
					ss << Utils::floatToString(pwr->mod_damage_value_min, eset->number_format.power_tooltips);
				else
					ss << Utils::floatToString(pwr->mod_damage_value_min, eset->number_format.power_tooltips) << "-" << Utils::floatToString(pwr->mod_damage_value_max, eset->number_format.power_tooltips);
			}
			else {
				ss << Utils::floatToString(mag_min, eset->number_format.power_tooltips) << "-" << Utils::floatToString(mag_max, eset->number_format.power_tooltips);
			}

			ss << " " << msg->get("Healing");
		}
		else if (effect_type == Effect::KNOCKBACK) {
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << " " << msg->get("Knockback");
		}
		else if (effect_ptr && !effect_ptr->name.empty() && pwr->post_effects[i].magnitude > 0) {
			if (effect_ptr->can_stack)
				ss << "+";
			ss << Utils::floatToString(pwr->post_effects[i].magnitude, eset->number_format.power_tooltips) << " " << msg->get(effect_ptr->name);
		}
		else if (pwr->post_effects[i].magnitude == 0) {
			// nothing
		}

		if (!ss.str().empty()) {
			if (pwr->post_effects[i].duration > 0) {
				if (effect_type == Effect::DEATH_SENTENCE) {
					ss << ": " << Utils::getDurationString(pwr->post_effects[i].duration, eset->number_format.durations);
				}
				else {
					ss << " (" << Utils::getDurationString(pwr->post_effects[i].duration, eset->number_format.durations) << ")";
				}

				if (pwr->post_effects[i].chance != 100)
					ss << " ";
			}
			if (pwr->post_effects[i].chance != 100) {
				ss << "(" << msg->getv("%s%% chance", Utils::floatToString(pwr->post_effects[i].chance, eset->number_format.power_tooltips).c_str()) << ")";
			}

			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
	}

	if (pwr->use_hazard || pwr->type == Power::TYPE_REPEATER) {
		std::stringstream ss;

		// modifier_accuracy
		if (pwr->mod_accuracy_mode > -1) {
			ss.str("");

			if (pwr->mod_accuracy_mode == Power::STAT_MODIFIER_MODE_ADD && pwr->mod_accuracy_value > 0)
				ss << "+";

			ss << Utils::floatToString(pwr->mod_accuracy_value, eset->number_format.power_tooltips);

			if (pwr->mod_accuracy_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			ss << msg->get("Base Accuracy");

			if (!ss.str().empty())
				tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}

		// modifier_critical
		if (pwr->mod_crit_mode > -1) {
			ss.str("");

			if (pwr->mod_crit_mode == Power::STAT_MODIFIER_MODE_ADD && pwr->mod_crit_value > 0)
				ss << "+";

			ss << Utils::floatToString(pwr->mod_crit_value, eset->number_format.power_tooltips);

			if (pwr->mod_crit_mode == Power::STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			ss << msg->get("Base Critical Chance");

			if (!ss.str().empty())
				tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}

		if (pwr->trait_armor_penetration) {
			ss.str("");
			ss << msg->get("Ignores Absorbtion");
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		if (pwr->trait_avoidance_ignore) {
			ss.str("");
			ss << msg->get("Ignores Avoidance");
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		if (pwr->trait_crits_impaired > 0) {
			ss.str("");
			ss << msg->getv("%s%% Chance to crit slowed targets", Utils::floatToString(pwr->trait_crits_impaired, eset->number_format.power_tooltips).c_str());
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
		if (pwr->trait_elemental > -1) {
			ss.str("");
			ss << msg->getv("Elemental Damage (%s)", eset->elements.list[pwr->trait_elemental].name.c_str());
			tip_data->addColoredText(ss.str(), font->getColor(FontEngine::COLOR_MENU_BONUS));
		}
	}

	std::set<std::string>::iterator it;
	for (it = pwr->requires_flags.begin(); it != pwr->requires_flags.end(); ++it) {
		for (size_t i = 0; i < eset->equip_flags.list.size(); ++i) {
			if ((*it) == eset->equip_flags.list[i].id) {
				tip_data->addText(msg->getv("Requires a %s", msg->get(eset->equip_flags.list[i].name).c_str()));
			}
		}
	}

	if (pcell) {
		// add requirement
		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			if (pcell->requires_primary[i] > 0) {
				if (pc->stats.get_primary(i) < pcell->requires_primary[i])
					tip_data->addColoredText(msg->getv("Requires %s %d", eset->primary_stats.list[i].name.c_str(), pcell->requires_primary[i]), font->getColor(FontEngine::COLOR_MENU_PENALTY));
				else
					tip_data->addText(msg->getv("Requires %s %d", eset->primary_stats.list[i].name.c_str(), pcell->requires_primary[i]));
			}
		}

		// Draw required Level Tooltip
		if ((pcell->requires_level > 0) && pc->stats.level < pcell->requires_level) {
			tip_data->addColoredText(msg->getv("Requires Level %d", pcell->requires_level), font->getColor(FontEngine::COLOR_MENU_PENALTY));
		}
		else if ((pcell->requires_level > 0) && pc->stats.level >= pcell->requires_level) {
			tip_data->addText(msg->getv("Requires Level %d", pcell->requires_level));
		}

		for (size_t j=0; j < pcell->requires_power.size(); ++j) {
			MenuPowersCell* req_cell = getCellByPowerIndex(pcell->requires_power[j]);
			if (!req_cell)
				continue;

			std::string req_power_name;
			if (req_cell->upgrade_level > 0)
				req_power_name = powers->powers[req_cell->id]->name + " (" + msg->getv("Level %d", req_cell->upgrade_level) + ")";
			else
				req_power_name = powers->powers[req_cell->id]->name;


			// Required Power Tooltip
			if (!checkUnlocked(req_cell)) {
				tip_data->addColoredText(msg->getv("Requires Power: %s", req_power_name.c_str()), font->getColor(FontEngine::COLOR_MENU_PENALTY));
			}
			else {
				tip_data->addText(msg->getv("Requires Power: %s", req_power_name.c_str()));
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
}

void MenuPowers::createTooltipInputHint(TooltipData* tip_data, bool enable_activate_msg) {
	bool show_activate_msg = false;
	std::string activate_bind_str;

	bool show_more_msg = false;
	std::string more_bind_str;

	if (inpt->mode == InputState::MODE_TOUCHSCREEN) {
		tip_data->addColoredText('\n' + msg->get("Tap icon again for more options"), font->getColor(FontEngine::COLOR_ITEM_BONUS));
	}
	else if (inpt->mode == InputState::MODE_JOYSTICK) {
		if (enable_activate_msg) {
			show_activate_msg = true;
			activate_bind_str = inpt->getGamepadBindingString(Input::MENU_ACTIVATE);
		}

		show_more_msg = true;
		more_bind_str = inpt->getGamepadBindingString(Input::ACCEPT);
	}
	else if (!inpt->usingMouse()) {
		if (enable_activate_msg) {
			show_activate_msg = true;
			activate_bind_str = inpt->getBindingString(Input::MENU_ACTIVATE);
		}

		show_more_msg = true;
		more_bind_str = inpt->getBindingString(Input::ACCEPT);
	}
	else {
		show_activate_msg = enable_activate_msg;
		activate_bind_str = inpt->getBindingString(Input::MAIN2);
	}

	if (show_activate_msg || show_more_msg) {
		tip_data->addText("");
	}

	if (show_activate_msg) {
		tip_data->addColoredText(msg->getv("Press [%s] to use", activate_bind_str.c_str()), font->getColor(FontEngine::COLOR_ITEM_BONUS));
	}

	if (show_more_msg) {
		tip_data->addColoredText(msg->getv("Press [%s] for more options", more_bind_str.c_str()), font->getColor(FontEngine::COLOR_ITEM_BONUS));
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
		if (!slot_cell || !isCellVisible(slot_cell))
			continue;

		if (slots[i]) {
			slots[i]->enabled = true;

			// highlighting
			if (checkUnlocked(slot_cell)) {
				int selected_slot = -1;
				if (isTabListSelected()) {
					selected_slot = getSelectedCellIndex();
				}

				if (selected_slot == static_cast<int>(i))
					continue;

				slots[i]->highlight = true;
			}
			else {
				slots[i]->highlight = false;
				slots[i]->enabled = false;
			}

			slots[i]->render();
		}

		// upgrade buttons
		if (power_cell[i].upgrade_button)
			power_cell[i].upgrade_button->render();
	}
}

void MenuPowers::logic() {
	if (!visible && tab_control && default_power_tab > -1) {
		tab_control->setActiveTab(static_cast<unsigned>(default_power_tab));
		tablist.setNextTabList(&tablist_pow[default_power_tab]);
	}

	setUnlockedPowers();

	points_left = (pc->stats.level * pc->stats.power_points_per_level) - getPointsUsed();
	if (points_left > 0) {
		newPowerNotification = true;
	}

	for (size_t i=0; i<power_cell.size(); i++) {
		// make sure invisible cells are skipped in the tablist
		if (visible && slots[i])
			slots[i]->enable_tablist_nav = isCellVisible(power_cell[i].getCurrent());

		// disable upgrade buttons by default
		if (power_cell[i].upgrade_button != NULL) {
			power_cell[i].upgrade_button->enabled = false;
		}

		// try to automatically upgrade powers is no power point is required
		MenuPowersCell* pcell = power_cell[i].getCurrent();
		while (checkUpgrade(pcell)) {
			if (pcell->next && !pcell->next->requires_point) {
				// automatic upgrade possible; do upgrade and re-check upgrade possibility
				upgradePower(pcell, UPGRADE_POWER_ALL_TABS);
				pcell = power_cell[i].getCurrent();
				if (power_cell[i].upgrade_button != NULL)
					power_cell[i].upgrade_button->enabled = (pc->stats.hp > 0 && isCellVisible(pcell) && checkUpgrade(pcell));
			}
			else {
				// power point required or no upgrade available; stop trying to upgrade
				if (power_cell[i].upgrade_button != NULL)
					power_cell[i].upgrade_button->enabled = (pc->stats.hp > 0 && isCellVisible(pcell));
				break;
			}
		}

		// handle clicking of upgrade button
		if (visible && pc->stats.hp > 0 && power_cell[i].upgrade_button != NULL) {
			if ((!tab_control || power_cell[i].tab == tab_control->getActiveTab()) && power_cell[i].upgrade_button->checkClick()) {
				upgradePower(power_cell[i].getCurrent(), !UPGRADE_POWER_ALL_TABS);
			}
		}
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
	Rect dest, tab_dest;

	// background
	dest = window_area;
	src.x = 0;
	src.y = 0;
	src.w = window_area.w;
	src.h = window_area.h;

	// tab background (if not menu-sized)
	tab_dest = window_area;
	tab_dest.x += tab_area.x;
	tab_dest.y += tab_area.y;
	if (tab_control)
		tab_area.y += tab_control->getTabHeight();

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
					if (tabs[i].background_is_menu_size) {
						r->setClipFromRect(src);
						r->setDestFromRect(dest);
					}
					else {
						r->setDestFromRect(tab_dest);
					}
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
		if (points_left >= 1) {
			label_unspent->setText(msg->getv("Available skill points: %d", points_left));
		}
		else {
			label_unspent->setText("");
		}
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
		if (!isCellVisible(tip_cell))
			continue;

		if (slots[i] && Utils::isWithinRect(slots[i]->pos, position)) {
			bool base_unlocked = checkUnlocked(tip_cell);

			createTooltip(&tip_data, tip_cell, tip_cell->id, !base_unlocked, MenuPowers::TOOLTIP_LONG_MENU);
			if (tip_cell->next) {
				tip_data.addText("\n" + msg->get("Next Level:"));
				createTooltip(&tip_data, tip_cell->next, tip_cell->next->id, base_unlocked, MenuPowers::TOOLTIP_LONG_MENU);
				createTooltipInputHint(&tip_data, !TOOLTIP_SHOW_ACTIVATE_HINT);
			}
			else {
				createTooltipInputHint(&tip_data, !TOOLTIP_SHOW_ACTIVATE_HINT);
			}

			tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
			break;
		}
	}
}

/**
 * Click-to-drag a power (to the action bar)
 */
MenuPowersClick MenuPowers::click(const Point& mouse) {
	MenuPowersClick result;

	int active_tab = (tab_control) ? tab_control->getActiveTab() : 0;

	for (size_t i=0; i<power_cell.size(); i++) {
		if (slots[i] && Utils::isWithinRect(slots[i]->pos, mouse) && (power_cell[i].tab == active_tab)) {
			if (inpt->mode == InputState::MODE_TOUCHSCREEN) {
				bool slot_had_focus = slots[i]->in_focus;

				if (!tabs.empty()) {
					tablist_pow[active_tab].setCurrent(slots[i]);
				}
				else {
					tablist.setCurrent(slots[i]);
				}

				if (!slot_had_focus) {
					return result;
				}
			}

			MenuPowersCell* pcell = power_cell[i].getCurrent();
			if (!pcell || !isCellVisible(pcell))
				return result;

			if (checkUnlock(pcell) && points_left > 0 && pcell->requires_point) {
				// unlock base power
				result.unlock = pcell->id;
			}
			else if (pcell->next && checkUpgrade(pcell)) {
				// unlock upgrade
				result.unlock = pcell->id;
			}

			if (checkUnlocked(pcell) && !powers->powers[pcell->id]->passive) {
				// pick up and drag power
				if (inpt->usingMouse() && inpt->mode != InputState::MODE_TOUCHSCREEN) {
					slots[i]->defocus();
					if (!tabs.empty()) {
						tablist_pow[active_tab].setCurrent(NULL);
					}
					else {
						tablist.setCurrent(NULL);
					}
				}
				result.drag = power_cell[i].getBonusCurrent(pcell)->id;
			}

			return result;
		}
	}

	// nothing selected, defocus everything
	defocusTabLists();

	return result;
}

void MenuPowers::clickUnlock(PowerID power_index) {
	MenuPowersCell* pcell = getCellByPowerIndex(power_index);
	if (!pcell)
		return;

	if (!checkUnlocked(pcell)) {
		// unlock base power
		pc->stats.powers_list.push_back(power_index);
		pc->stats.check_title = true;
		setUnlockedPowers();
		menu->act->addPower(power_index, 0);
	}
	else {
		// base power is already unlocked, so upgrade instead
		upgradePower(pcell, !UPGRADE_POWER_ALL_TABS);
	}
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
bool MenuPowers::meetsUsageStats(PowerID power_index) {
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

void MenuPowers::addBonusLevels(PowerID power_index, int bonus_levels) {
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

std::string MenuPowers::getItemBonusPowerReqString(PowerID power_index) {
	MenuPowersCell* pcell = getCellByPowerIndex(power_index);

	if (!pcell)
		return "";

	std::string output = powers->powers[power_index]->name;
	if (pcell->upgrade_level > 0) {
		output += " (" + msg->getv("Level %d", pcell->upgrade_level) + ")";
	}

	return output;
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

