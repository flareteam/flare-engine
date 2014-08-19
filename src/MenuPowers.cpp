/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson

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

#include "CommonIncludes.h"
#include "Menu.h"
#include "MenuPowers.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"
#include "WidgetSlot.h"
#include "TooltipData.h"
#include "MenuActionBar.h"

#include <climits>

using namespace std;

MenuPowers::MenuPowers(StatBlock *_stats, MenuActionBar *_action_bar)
	: stats(_stats)
	, action_bar(_action_bar)
	, skip_section(false)
	, powers_unlock(NULL)
	, overlay_disabled(NULL)
	, pressed(false)
	, points_left(0)
	, tabs_count(1)
	, tabControl(NULL)
	, newPowerNotification(false)
{

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Read powers data from config file
	FileParser infile;
	// @CLASS MenuPowers|Description of menus/powers.txt
	if (infile.open("menus/powers.txt")) {
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
			}
			if (infile.section == "header")
				loadHeader(infile);
			else if (infile.section == "power")
				loadPower(infile);
			else if (infile.section == "upgrade")
				loadUpgrade(infile);
		}
		infile.close();
	}

	// save a copy of the base level powers, as they get overwritten during upgrades
	power_cell_base = power_cell;

	// combine base and upgrade powers into a single list
	for (unsigned i=0; i<power_cell_base.size(); ++i) {
		power_cell_all.push_back(power_cell_base[i]);
	}
	for (unsigned i=0; i<power_cell_upgrade.size(); ++i) {
		power_cell_all.push_back(power_cell_upgrade[i]);
	}

	loadGraphics();

	// check for errors in config file
	if((tabs_count == 1) && (!tree_image_files.empty() || !tab_titles.empty())) {
		logError("MenuPowers: menu/powers.txt error: you don't have tabs, but tab_tree_image and tab_title counts are not 0\n");
		SDL_Quit();
		exit(1);
	}
	else if((tabs_count > 1) && (tree_image_files.size() != (unsigned)tabs_count || tab_titles.size() != (unsigned)tabs_count)) {
		logError("MenuPowers: menu/powers.txt error: tabs count, tab_tree_image and tab_name counts do not match\n");
		SDL_Quit();
		exit(1);
	}

	menu_powers = this;

	color_bonus = font->getColor("menu_bonus");
	color_penalty = font->getColor("menu_penalty");

	align();
	alignElements();
}

void MenuPowers::alignElements() {
	for (unsigned i=0; i<power_cell.size(); i++) {
		slots[i]->pos.x = window_area.x + power_cell[i].pos.x;
		slots[i]->pos.y = window_area.y + power_cell[i].pos.y;
		if (upgradeButtons[i] != NULL) {
			upgradeButtons[i]->pos.x = slots[i]->pos.x + ICON_SIZE;
			upgradeButtons[i]->pos.y = slots[i]->pos.y;
		}
	}

	label_powers.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Powers"), font->getColor("menu_normal"), title.font_style);

	closeButton->pos.x = window_area.x+close_pos.x;
	closeButton->pos.y = window_area.y+close_pos.y;

	stat_up.set(window_area.x+unspent_points.x, window_area.y+unspent_points.y, unspent_points.justify, unspent_points.valign, "", font->getColor("menu_bonus"), unspent_points.font_style);

	// If we have more than one tab, create TabControl
	if (tabs_count > 1) {
		tabControl = new WidgetTabControl(tabs_count);

		// Initialize the tab control.
		tabControl->setMainArea(window_area.x+tab_area.x, window_area.y+tab_area.y, tab_area.w, tab_area.h);

		// Define the header.
		for (int i=0; i<tabs_count; i++)
			tabControl->setTabTitle(i, msg->get(tab_titles[i]));
		tabControl->updateHeader();
	}
}

void MenuPowers::loadGraphics() {

	Image *graphics;

	setBackground("images/menus/powers.png");

	graphics = render_device->loadImage("images/menus/powers_unlock.png");
	if (graphics) {
		powers_unlock = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/disabled.png");
	if (graphics) {
		overlay_disabled = graphics->createSprite();
		graphics->unref();
	}

	if (tree_image_files.empty()) {
		graphics = render_device->loadImage("images/menus/powers_tree.png");
		if (graphics) {
			tree_surf.push_back(graphics->createSprite());
			graphics->unref();
		}
	}
	else {
		for (unsigned int i = 0; i < tree_image_files.size(); ++i) {
			graphics = render_device->loadImage(tree_image_files[i]);
			if (graphics) {
				tree_surf.push_back(graphics->createSprite());
				graphics->unref();
			}
		}
	}
	for (unsigned int i=0; i<slots.size(); i++) {

		slots[i] = new WidgetSlot(powers->powers[power_cell[i].id].icon);
		slots[i]->pos.x = power_cell[i].pos.x;
		slots[i]->pos.y = power_cell[i].pos.y;
		tablist.add(slots[i]);
	}
}

short MenuPowers::id_by_powerIndex(short power_index, const std::vector<Power_Menu_Cell>& cell) {
	// Find cell with our power
	for (unsigned i=0; i<cell.size(); i++)
		if (cell[i].id == power_index)
			return i;

	return -1;
}

/**
 * Apply power upgrades on savegame loading
 */
void MenuPowers::applyPowerUpgrades() {
	for (unsigned i = 0; i < power_cell.size(); i++) {
		if (!power_cell[i].upgrades.empty()) {
			vector<short>::iterator it;
			for (it = power_cell[i].upgrades.end(); it != power_cell[i].upgrades.begin(); ) {
				--it;
				vector<int>::iterator upgrade_it;
				upgrade_it = find(stats->powers_list.begin(), stats->powers_list.end(), *it);
				if (upgrade_it != stats->powers_list.end()) {
					short upgrade_index = id_by_powerIndex(*upgrade_it, power_cell_upgrade);
					replacePowerCellDataByUpgrade(i, upgrade_index);
					break;
				}
			}
		}
	}
	setUnlockedPowers();
}

/**
 * Find cell in upgrades with next upgrade for current power_cell
 */
short MenuPowers::nextLevel(short power_cell_index) {
	vector<short>::iterator level_it;
	level_it = find(power_cell[power_cell_index].upgrades.begin(),
					power_cell[power_cell_index].upgrades.end(),
					power_cell[power_cell_index].id);

	if (level_it == power_cell[power_cell_index].upgrades.end()) {
		// current power is base power, take first upgrade
		short index = power_cell[power_cell_index].upgrades[0];
		return id_by_powerIndex(index, power_cell_upgrade);
	}
	// current power is an upgrade, take next upgrade if avaliable
	short index = distance(power_cell[power_cell_index].upgrades.begin(), level_it);
	if ((short)power_cell[power_cell_index].upgrades.size() > index + 1) {
		return id_by_powerIndex(*(++level_it), power_cell_upgrade);
	}
	else {
		return -1;
	}
}

/**
 * Replace data in power_cell[cell_index] by data in upgrades
 */
void MenuPowers::upgradePower(short power_cell_index) {
	short i = nextLevel(power_cell_index);
	if (i == -1)
		return;

	// if power was present in ActionBar, update it there
	for(int j = 0; j < 12; j++) {
		if(action_bar->hotkeys[j] == power_cell[power_cell_index].id) {
			action_bar->hotkeys[j] = power_cell_upgrade[i].id;
		}
	}
	// if we have tabControl
	if (tabs_count > 1) {
		int active_tab = tabControl->getActiveTab();
		if (power_cell[power_cell_index].tab == active_tab) {
			replacePowerCellDataByUpgrade(power_cell_index, i);
			stats->powers_list.push_back(power_cell_upgrade[i].id);
			stats->check_title = true;
		}
	}
	// if have don't have tabs
	else {
		replacePowerCellDataByUpgrade(power_cell_index, i);
		stats->powers_list.push_back(power_cell_upgrade[i].id);
		stats->check_title = true;
	}
	setUnlockedPowers();
}

void MenuPowers::replacePowerCellDataByUpgrade(short power_cell_index, short upgrade_cell_index) {
	power_cell[power_cell_index].id = power_cell_upgrade[upgrade_cell_index].id;
	power_cell[power_cell_index].requires_physoff = power_cell_upgrade[upgrade_cell_index].requires_physoff;
	power_cell[power_cell_index].requires_physdef = power_cell_upgrade[upgrade_cell_index].requires_physdef;
	power_cell[power_cell_index].requires_mentoff = power_cell_upgrade[upgrade_cell_index].requires_mentoff;
	power_cell[power_cell_index].requires_mentdef = power_cell_upgrade[upgrade_cell_index].requires_mentdef;
	power_cell[power_cell_index].requires_defense = power_cell_upgrade[upgrade_cell_index].requires_defense;
	power_cell[power_cell_index].requires_offense = power_cell_upgrade[upgrade_cell_index].requires_offense;
	power_cell[power_cell_index].requires_physical = power_cell_upgrade[upgrade_cell_index].requires_physical;
	power_cell[power_cell_index].requires_mental = power_cell_upgrade[upgrade_cell_index].requires_mental;
	power_cell[power_cell_index].requires_level = power_cell_upgrade[upgrade_cell_index].requires_level;
	power_cell[power_cell_index].requires_power = power_cell_upgrade[upgrade_cell_index].requires_power;
	power_cell[power_cell_index].requires_point = power_cell_upgrade[upgrade_cell_index].requires_point;
	power_cell[power_cell_index].passive_on = power_cell_upgrade[upgrade_cell_index].passive_on;

	slots[power_cell_index]->setIcon(powers->powers[power_cell_upgrade[upgrade_cell_index].id].icon);
}

bool MenuPowers::baseRequirementsMet(int power_index) {
	int id = id_by_powerIndex(power_index, power_cell_all);

	for (unsigned i = 0; i < power_cell_all[id].requires_power.size(); ++i)
		if (!requirementsMet(power_cell_all[id].requires_power[i]))
			return false;

	if ((stats->physoff() >= power_cell_all[id].requires_physoff) &&
			(stats->physdef() >= power_cell_all[id].requires_physdef) &&
			(stats->mentoff() >= power_cell_all[id].requires_mentoff) &&
			(stats->mentdef() >= power_cell_all[id].requires_mentdef) &&
			(stats->get_defense() >= power_cell_all[id].requires_defense) &&
			(stats->get_offense() >= power_cell_all[id].requires_offense) &&
			(stats->get_physical() >= power_cell_all[id].requires_physical) &&
			(stats->get_mental() >= power_cell_all[id].requires_mental) &&
			(stats->level >= power_cell_all[id].requires_level)) return true;
	return false;
}

/**
 * With great power comes great stat requirements.
 */
bool MenuPowers::requirementsMet(int power_index) {

	// power_index can be 0 during recursive call if requires_power is not defined.
	// Power with index 0 doesn't exist and is always enabled
	if (power_index == 0) return true;

	int id = id_by_powerIndex(power_index, power_cell_all);

	// If we didn't find power in power_menu, than it has no requirements
	if (id == -1) return true;

	if (!powerIsVisible(power_index)) return false;

	// If power_id is saved into vector, it's unlocked anyway
	// check power_cell_unlocked and stats->powers_list
	for (unsigned i=0; i<power_cell_unlocked.size(); ++i) {
		if (power_cell_unlocked[i].id == power_index)
			return true;
	}
	if (find(stats->powers_list.begin(), stats->powers_list.end(), power_index) != stats->powers_list.end()) return true;

	// Check the rest requirements
	if (baseRequirementsMet(power_index) && !power_cell_all[id].requires_point) return true;
	return false;
}

/**
 * Check if we can unlock power.
 */
bool MenuPowers::powerUnlockable(int power_index) {

	// power_index can be 0 during recursive call if requires_power is not defined.
	// Power with index 0 doesn't exist and is always enabled
	if (power_index == 0) return true;

	// Find cell with our power
	int id = id_by_powerIndex(power_index, power_cell_all);

	// If we didn't find power in power_menu, than it has no requirements
	if (id == -1) return true;

	if (!powerIsVisible(power_index)) return false;

	// If we already have a power, don't try to unlock it
	if (requirementsMet(power_index)) return false;

	// Check requirements
	if (baseRequirementsMet(power_index)) return true;
	return false;
}

/**
 * Click-to-drag a power (to the action bar)
 */
int MenuPowers::click(Point mouse) {

	// if we have tabControl
	if (tabs_count > 1) {
		int active_tab = tabControl->getActiveTab();
		for (unsigned i=0; i<power_cell.size(); i++) {
			if (isWithin(slots[i]->pos, mouse) && (power_cell[i].tab == active_tab)) {
				if (requirementsMet(power_cell[i].id) && !powers->powers[power_cell[i].id].passive) return power_cell[i].id;
				else return 0;
			}
		}
		// if have don't have tabs
	}
	else {
		for (unsigned i=0; i<power_cell.size(); i++) {
			if (isWithin(slots[i]->pos, mouse)) {
				if (requirementsMet(power_cell[i].id) && !powers->powers[power_cell[i].id].passive) return power_cell[i].id;
				else return 0;
			}
		}
	}
	return 0;
}

/**
 * Unlock a power
 */
bool MenuPowers::unlockClick(Point mouse) {

	// if we have tabControl
	if (tabs_count > 1) {
		int active_tab = tabControl->getActiveTab();
		for (unsigned i=0; i<power_cell.size(); i++) {
			if (isWithin(slots[i]->pos, mouse)
					&& (powerUnlockable(power_cell[i].id)) && points_left > 0
					&& power_cell[i].requires_point && power_cell[i].tab == active_tab) {
				stats->powers_list.push_back(power_cell[i].id);
				stats->check_title = true;
				setUnlockedPowers();
				return true;
			}
		}
		// if have don't have tabs
	}
	else {
		for (unsigned i=0; i<power_cell.size(); i++) {
			if (isWithin(slots[i]->pos, mouse)
					&& (powerUnlockable(power_cell[i].id))
					&& points_left > 0 && power_cell[i].requires_point) {
				stats->powers_list.push_back(power_cell[i].id);
				stats->check_title = true;
				setUnlockedPowers();
				return true;
			}
		}
	}
	return false;
}

short MenuPowers::getPointsUsed() {
	short used = 0;
	for (unsigned i=0; i<power_cell_unlocked.size(); ++i) {
		if (power_cell_unlocked[i].requires_point)
			used++;
	}

	return used;
}

void MenuPowers::setUnlockedPowers() {
	std::vector<int> power_ids;
	power_cell_unlocked.clear();

	for (unsigned i=0; i<power_cell.size(); ++i) {
		if (find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end()) {
			// base power
			if (find(power_ids.begin(), power_ids.end(), power_cell[i].id) == power_ids.end()) {
				power_ids.push_back(power_cell_base[i].id);
				power_cell_unlocked.push_back(power_cell_base[i]);
			}
			if (power_cell[i].id == power_cell_base[i].id)
				continue;

			//upgrades
			for (unsigned j=0; j<power_cell[i].upgrades.size(); ++j) {
				if (find(power_ids.begin(), power_ids.end(), power_cell[i].upgrades[j]) == power_ids.end()) {
					int id = id_by_powerIndex(power_cell[i].upgrades[j], power_cell_upgrade);
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
	}
}

void MenuPowers::logic() {
	for (unsigned i=0; i<power_cell.size(); i++) {
		if (powers->powers[power_cell[i].id].passive) {
			bool unlocked_power = find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end();
			vector<int>::iterator it = find(stats->powers_passive.begin(), stats->powers_passive.end(), power_cell[i].id);
			if (it != stats->powers_passive.end()) {
				if (!baseRequirementsMet(power_cell[i].id) && power_cell[i].passive_on) {
					stats->powers_passive.erase(it);
					stats->effects.removeEffectPassive(power_cell[i].id);
					power_cell[i].passive_on = false;
					stats->refresh_stats = true;
				}
			}
			else if (((baseRequirementsMet(power_cell[i].id) && !power_cell[i].requires_point) || unlocked_power) && !power_cell[i].passive_on) {
				stats->powers_passive.push_back(power_cell[i].id);
				power_cell[i].passive_on = true;
				// for passives without special triggers, we need to trigger them here
				if (stats->effects.triggered_others)
					powers->activateSinglePassive(stats, power_cell[i].id);
			}
		}

		//upgrade buttons logic
		if (upgradeButtons[i] != NULL && power_cell[i].tab == tabControl->getActiveTab()) {
			if (upgradeButtons[i]->checkClick()) {
				upgradePower(i);
			}
		}
	}

	points_left = (stats->level * stats->power_points_per_level) - getPointsUsed();
	if (points_left > 0) {
		newPowerNotification = true;
	}

	if (!visible) return;

	if (NO_MOUSE) {
		tablist.logic();
	}

	// make shure keyboard navigation leads us to correct tab
	for (unsigned int i = 0; i < slots.size(); i++) {
		if (slots[i]->in_focus) tabControl->setActiveTab(power_cell[i].tab);
	}

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}
	if (tabs_count > 1) tabControl->logic();
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


	if (tabs_count > 1) {
		tabControl->render();
		int active_tab = tabControl->getActiveTab();
		for (int i=0; i<tabs_count; i++) {
			if (active_tab == i) {
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
	else {
		Sprite *r = tree_surf[0];
		if (r) {
			r->setClip(src);
			r->setDest(dest);
			render_device->render(r);
		}
		renderPowers(0);
	}

	// close button
	closeButton->render();

	// text overlay
	if (!title.hidden) label_powers.render();

	// stats
	if (!unspent_points.hidden) {
		stringstream ss;

		ss.str("");
		if (points_left !=0) {
			ss << msg->get("Unspent skill points:") << " " << points_left;
		}
		stat_up.set(ss.str());
		stat_up.render();
	}
}

/**
 * Highlight unlocked powers
 */
void MenuPowers::displayBuild(int power_id) {
	Rect src_unlock;

	src_unlock.x = 0;
	src_unlock.y = 0;
	src_unlock.w = ICON_SIZE;
	src_unlock.h = ICON_SIZE;

	for (unsigned i=0; i<power_cell.size(); i++) {
		if (power_cell[i].id == power_id && powers_unlock) {
			powers_unlock->setClip(src_unlock);
			powers_unlock->setDest(slots[i]->pos);
			render_device->render(powers_unlock);
		}
	}
}

/**
 * Show mouseover descriptions of disciplines and powers
 */
TooltipData MenuPowers::checkTooltip(Point mouse) {

	TooltipData tip;

	for (unsigned i=0; i<power_cell.size(); i++) {

		if ((tabs_count > 1) && (tabControl->getActiveTab() != power_cell[i].tab)) continue;

		if (!powerIsVisible(power_cell[i].id)) continue;

		if (isWithin(slots[i]->pos, mouse)) {
			generatePowerDescription(&tip, i, power_cell);
			if (!power_cell[i].upgrades.empty()) {
				short next_level = nextLevel(i);
				if (next_level != -1) {
					tip.addText("\n" + msg->get("Next Level:"));
					generatePowerDescription(&tip, next_level, power_cell_upgrade);
				}
			}

			return tip;
		}
	}

	return tip;
}

void MenuPowers::generatePowerDescription(TooltipData* tip, int slot_num, const vector<Power_Menu_Cell>& power_cells) {
	tip->addText(powers->powers[power_cells[slot_num].id].name);
	if (powers->powers[power_cells[slot_num].id].passive) tip->addText("Passive");
	tip->addText(powers->powers[power_cells[slot_num].id].description);

	std::set<std::string>::iterator it;
	for (it = powers->powers[power_cells[slot_num].id].requires_flags.begin(); it != powers->powers[power_cells[slot_num].id].requires_flags.end(); ++it) {
		tip->addText(msg->get("Requires a %s", msg->get(EQUIP_FLAGS[(*it)])));
	}

	// add requirement
	if ((power_cells[slot_num].requires_physoff > 0) && (stats->physoff() < power_cells[slot_num].requires_physoff)) {
		tip->addText(msg->get("Requires Physical Offense %d", power_cells[slot_num].requires_physoff), color_penalty);
	}
	else if((power_cells[slot_num].requires_physoff > 0) && (stats->physoff() >= power_cells[slot_num].requires_physoff)) {
		tip->addText(msg->get("Requires Physical Offense %d", power_cells[slot_num].requires_physoff));
	}
	if ((power_cells[slot_num].requires_physdef > 0) && (stats->physdef() < power_cells[slot_num].requires_physdef)) {
		tip->addText(msg->get("Requires Physical Defense %d", power_cells[slot_num].requires_physdef), color_penalty);
	}
	else if ((power_cells[slot_num].requires_physdef > 0) && (stats->physdef() >= power_cells[slot_num].requires_physdef)) {
		tip->addText(msg->get("Requires Physical Defense %d", power_cells[slot_num].requires_physdef));
	}
	if ((power_cells[slot_num].requires_mentoff > 0) && (stats->mentoff() < power_cells[slot_num].requires_mentoff)) {
		tip->addText(msg->get("Requires Mental Offense %d", power_cells[slot_num].requires_mentoff), color_penalty);
	}
	else if ((power_cells[slot_num].requires_mentoff > 0) && (stats->mentoff() >= power_cells[slot_num].requires_mentoff)) {
		tip->addText(msg->get("Requires Mental Offense %d", power_cells[slot_num].requires_mentoff));
	}
	if ((power_cells[slot_num].requires_mentdef > 0) && (stats->mentdef() < power_cells[slot_num].requires_mentdef)) {
		tip->addText(msg->get("Requires Mental Defense %d", power_cells[slot_num].requires_mentdef), color_penalty);
	}
	else if ((power_cells[slot_num].requires_mentdef > 0) && (stats->mentdef() >= power_cells[slot_num].requires_mentdef)) {
		tip->addText(msg->get("Requires Mental Defense %d", power_cells[slot_num].requires_mentdef));
	}
	if ((power_cells[slot_num].requires_offense > 0) && (stats->get_offense() < power_cells[slot_num].requires_offense)) {
		tip->addText(msg->get("Requires Offense %d", power_cells[slot_num].requires_offense), color_penalty);
	}
	else if ((power_cells[slot_num].requires_offense > 0) && (stats->get_offense() >= power_cells[slot_num].requires_offense)) {
		tip->addText(msg->get("Requires Offense %d", power_cells[slot_num].requires_offense));
	}
	if ((power_cells[slot_num].requires_defense > 0) && (stats->get_defense() < power_cells[slot_num].requires_defense)) {
		tip->addText(msg->get("Requires Defense %d", power_cells[slot_num].requires_defense), color_penalty);
	}
	else if ((power_cells[slot_num].requires_defense > 0) && (stats->get_defense() >= power_cells[slot_num].requires_defense)) {
		tip->addText(msg->get("Requires Defense %d", power_cells[slot_num].requires_defense));
	}
	if ((power_cells[slot_num].requires_physical > 0) && (stats->get_physical() < power_cells[slot_num].requires_physical)) {
		tip->addText(msg->get("Requires Physical %d", power_cells[slot_num].requires_physical), color_penalty);
	}
	else if ((power_cells[slot_num].requires_physical > 0) && (stats->get_physical() >= power_cells[slot_num].requires_physical)) {
		tip->addText(msg->get("Requires Physical %d", power_cells[slot_num].requires_physical));
	}
	if ((power_cells[slot_num].requires_mental > 0) && (stats->get_mental() < power_cells[slot_num].requires_mental)) {
		tip->addText(msg->get("Requires Mental %d", power_cells[slot_num].requires_mental), color_penalty);
	}
	else if ((power_cells[slot_num].requires_mental > 0) && (stats->get_mental() >= power_cells[slot_num].requires_mental)) {
		tip->addText(msg->get("Requires Mental %d", power_cells[slot_num].requires_mental));
	}

	// Draw required Level Tooltip
	if ((power_cells[slot_num].requires_level > 0) && stats->level < power_cells[slot_num].requires_level) {
		tip->addText(msg->get("Requires Level %d", power_cells[slot_num].requires_level), color_penalty);
	}
	else if ((power_cells[slot_num].requires_level > 0) && stats->level >= power_cells[slot_num].requires_level) {
		tip->addText(msg->get("Requires Level %d", power_cells[slot_num].requires_level));
	}

	// Draw required Skill Point Tooltip
	if ((power_cells[slot_num].requires_point) &&
			!(find(stats->powers_list.begin(), stats->powers_list.end(), power_cells[slot_num].id) != stats->powers_list.end()) &&
			(points_left < 1)) {
		tip->addText(msg->get("Requires %d Skill Point", power_cells[slot_num].requires_point), color_penalty);
	}
	else if ((power_cells[slot_num].requires_point) &&
			 !(find(stats->powers_list.begin(), stats->powers_list.end(), power_cells[slot_num].id) != stats->powers_list.end()) &&
			 (points_left > 0)) {
		tip->addText(msg->get("Requires %d Skill Point", power_cells[slot_num].requires_point));
	}

	// Draw unlock power Tooltip
	if (power_cells[slot_num].requires_point &&
			!(find(stats->powers_list.begin(), stats->powers_list.end(), power_cells[slot_num].id) != stats->powers_list.end()) &&
			(points_left > 0) &&
			powerUnlockable(power_cells[slot_num].id)) {
		tip->addText(msg->get("Click to Unlock"), color_bonus);
	}



	for (unsigned j = 0; j < power_cells[slot_num].requires_power.size(); ++j) {
		// Required Power Tooltip
		if ((power_cells[slot_num].requires_power[j] != 0) && !(requirementsMet(power_cells[slot_num].requires_power[j]))) {
			tip->addText(msg->get("Requires Power: %s", powers->powers[power_cells[slot_num].requires_power[j]].name), color_penalty);
		}
		else if ((power_cells[slot_num].requires_power[j] != 0) && (requirementsMet(power_cells[slot_num].requires_power[j]))) {
			tip->addText(msg->get("Requires Power: %s", powers->powers[power_cells[slot_num].requires_power[j]].name));
		}

	}

	// add mana cost
	if (powers->powers[power_cells[slot_num].id].requires_mp > 0) {
		tip->addText(msg->get("Costs %d MP", powers->powers[power_cells[slot_num].id].requires_mp));
	}
	// add health cost
	if (powers->powers[power_cells[slot_num].id].requires_hp > 0) {
		tip->addText(msg->get("Costs %d HP", powers->powers[power_cells[slot_num].id].requires_hp));
	}
	// add cooldown time
	if (powers->powers[power_cells[slot_num].id].cooldown > 0) {
		tip->addText(msg->get("Cooldown: %d seconds", powers->powers[power_cells[slot_num].id].cooldown / MAX_FRAMES_PER_SEC));
	}
}

MenuPowers::~MenuPowers() {
	if (powers_unlock) delete powers_unlock;
	if (overlay_disabled) delete overlay_disabled;

	for (unsigned int i=0; i<tree_surf.size(); i++) {
		if (tree_surf[i]) delete tree_surf[i];
	}
	for (unsigned int i=0; i<slots.size(); i++) {
		delete slots.at(i);
		delete upgradeButtons.at(i);
	}
	slots.clear();
	upgradeButtons.clear();

	delete closeButton;
	if (tabs_count > 1) delete tabControl;
	menu_powers = NULL;
}

/**
 * Return true if required stats for power usage are met. Else return false.
 */
bool MenuPowers::meetsUsageStats(unsigned powerid) {

	// Find cell with our power
	int id = id_by_powerIndex(powerid, power_cell);
	// If we didn't find power in power_menu, than it has no stats requirements
	if (id == -1) return true;

	return stats->physoff() >= power_cell[id].requires_physoff
		   && stats->physdef() >= power_cell[id].requires_physdef
		   && stats->mentoff() >= power_cell[id].requires_mentoff
		   && stats->mentdef() >= power_cell[id].requires_mentdef
		   && stats->get_defense() >= power_cell[id].requires_defense
		   && stats->get_offense() >= power_cell[id].requires_offense
		   && stats->get_mental() >= power_cell[id].requires_mental
		   && stats->get_physical() >= power_cell[id].requires_physical;
}

void MenuPowers::renderPowers(int tab_num) {

	Rect disabled_src;
	disabled_src.x = disabled_src.y = 0;
	disabled_src.w = disabled_src.h = ICON_SIZE;

	for (unsigned i=0; i<power_cell.size(); i++) {
		bool power_in_vector = false;

		// Continue if slot is not filled with data
		if (power_cell[i].tab != tab_num) continue;

		if (!powerIsVisible(power_cell[i].id)) continue;

		if (find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end()) power_in_vector = true;

		slots[i]->render();

		// highlighting
		if (power_in_vector || requirementsMet(power_cell[i].id)) {
			displayBuild(power_cell[i].id);
		}
		else {
			if (overlay_disabled) {
				overlay_disabled->setClip(disabled_src);
				overlay_disabled->setDest(slots[i]->pos);
				render_device->render(overlay_disabled);
			}
		}
		slots[i]->renderSelection();
		// upgrade buttons
		if (upgradeButtons[i] != NULL && nextLevel(i) != -1) {
			// draw button only if current level is unlocked and next level can be unlocked
			if (requirementsMet(power_cell[i].id) && powerUnlockable(power_cell_upgrade[nextLevel(i)].id) && points_left > 0 && power_cell_upgrade[nextLevel(i)].requires_point) {
				upgradeButtons[i]->enabled = true;
				upgradeButtons[i]->render();
			}
			else {
				upgradeButtons[i]->enabled = false;
			}
		}
	}
}

bool MenuPowers::powerIsVisible(short power_index) {

	// power_index can be 0 during recursive call if requires_power is not defined.
	// Power with index 0 doesn't exist and is always enabled
	if (power_index == 0) return true;

	// Find cell with our power
	int id = id_by_powerIndex(power_index, power_cell_all);

	// If we didn't find power in power_menu, than it has no requirements
	if (id == -1) return true;

	for (unsigned i = 0; i < power_cell_all[id].visible_requires_status.size(); ++i)
		if (!camp->checkStatus(power_cell_all[id].visible_requires_status[i]))
			return false;

	for (unsigned i = 0; i < power_cell_all[id].visible_requires_not.size(); ++i)
		if (camp->checkStatus(power_cell_all[id].visible_requires_not[i]))
			return false;

	return true;
}

void MenuPowers::loadHeader(FileParser &infile) {
	if (parseMenuKey(infile.key, infile.val))
		return;

	// @ATTR header.tab_title|string|Title to display for a tree page. Use only if tabs > 1.
	if (infile.key == "tab_title") tab_titles.push_back(infile.val);
	// @ATTR header.tab_tree|string|Filename of a background image to use for a tree page. Pair this with tab_title. Use only if tabs > 1.
	else if (infile.key == "tab_tree") tree_image_files.push_back(infile.val);
	// @ATTR header.label_title|label|Position of the "Powers" text.
	else if (infile.key == "label_title") title = eatLabelInfo(infile.val);
	// @ATTR header.unspent_points|label|Position of the text that displays the amount of unused power points.
	else if (infile.key == "unspent_points") unspent_points = eatLabelInfo(infile.val);
	// @ATTR header.close|x (integer), y (integer)|Position of the close button.
	else if (infile.key == "close") close_pos = toPoint(infile.val);
	// @ATTR header.tab_area|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the tree pages.
	else if (infile.key == "tab_area") tab_area = toRect(infile.val);
	// @ATTR header.tabs|integer|The total number of tree pages.
	else if (infile.key == "tabs") {
		tabs_count = toInt(infile.val);
		if (tabs_count < 1) tabs_count = 1;
	}

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

void MenuPowers::loadPower(FileParser &infile) {
	// @ATTR power.id|integer|A power id from powers/powers.txt for this slot.
	if (infile.key == "id") {
		int id = popFirstInt(infile.val);
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
		logError("MenuPowers: There is a power without a valid id as the first attribute.\nIDs must be the first attribute in the power menu definition.\n");
	}

	if (skip_section)
		return;

	// @ATTR power.tab|integer|Tab index to place this power on, starting from 0.
	if (infile.key == "tab") power_cell.back().tab = toInt(infile.val);
	// @ATTR power.position|x (integer), y (integer)|Position of this power icon.
	else if (infile.key == "position") power_cell.back().pos = toPoint(infile.val);

	// @ATTR power.requires_physoff|integer|Power requires Physical and Offense stat of this value.
	else if (infile.key == "requires_physoff") power_cell.back().requires_physoff = toInt(infile.val);
	// @ATTR power.requires_physdef|integer|Power requires Physical and Defense stat of this value.
	else if (infile.key == "requires_physdef") power_cell.back().requires_physdef = toInt(infile.val);
	// @ATTR power.requires_mentoff|integer|Power requires Mental and Offense stat of this value.
	else if (infile.key == "requires_mentoff") power_cell.back().requires_mentoff = toInt(infile.val);
	// @ATTR power.requires_mentdef|integer|Power requires Mental and Defense stat of this value.
	else if (infile.key == "requires_mentdef") power_cell.back().requires_mentdef = toInt(infile.val);

	// @ATTR power.requires_defense|integer|Power requires Defense stat of this value.
	else if (infile.key == "requires_defense") power_cell.back().requires_defense = toInt(infile.val);
	// @ATTR power.requires_offense|integer|Power requires Offense stat of this value.
	else if (infile.key == "requires_offense") power_cell.back().requires_offense = toInt(infile.val);
	// @ATTR power.requires_physical|integer|Power requires Physical stat of this value.
	else if (infile.key == "requires_physical") power_cell.back().requires_physical = toInt(infile.val);
	// @ATTR power.requires_mental|integer|Power requires Mental stat of this value.
	else if (infile.key == "requires_mental") power_cell.back().requires_mental = toInt(infile.val);

	// @ATTR power.requires_point|boolean|Power requires a power point to unlock.
	else if (infile.key == "requires_point") power_cell.back().requires_point = toBool(infile.val);
	// @ATTR power.requires_level|integer|Power requires at least this level for the hero.
	else if (infile.key == "requires_level") power_cell.back().requires_level = toInt(infile.val);
	// @ATTR power.requires_power|integer|Power requires another power id.
	else if (infile.key == "requires_power") power_cell.back().requires_power.push_back(toInt(infile.val));

	// @ATTR power.visible_requires_status|string|Hide the power if we don't have this campaign status.
	else if (infile.key == "visible_requires_status") power_cell.back().visible_requires_status.push_back(infile.val);
	// @ATTR power.visible_requires_status|string|Hide the power if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell.back().visible_requires_not.push_back(infile.val);

	// @ATTR power.upgrades|id (integer), ...|A list of upgrade power ids that this power slot can upgrade to. Each of these powers should have a matching upgrade section.
	else if (infile.key == "upgrades") {
		upgradeButtons.back() = new WidgetButton("images/menus/buttons/button_plus.png");
		string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			power_cell.back().upgrades.push_back(toInt(repeat_val));
			repeat_val = infile.nextValue();
		}
	}

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

void MenuPowers::loadUpgrade(FileParser &infile) {
	// @ATTR upgrade.id|integer|A power id from powers/powers.txt for this upgrade.
	if (infile.key == "id") {
		int id = popFirstInt(infile.val);
		if (id > 0) {
			skip_section = false;
			power_cell_upgrade.back().id = id;
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

	// @ATTR upgrade.requires_physoff|integer|Upgrade requires Physical and Offense stat of this value.
	if (infile.key == "requires_physoff") power_cell_upgrade.back().requires_physoff = toInt(infile.val);
	// @ATTR upgrade.requires_physdef|integer|Upgrade requires Physical and Defense stat of this value.
	else if (infile.key == "requires_physdef") power_cell_upgrade.back().requires_physdef = toInt(infile.val);
	// @ATTR upgrade.requires_mentoff|integer|Upgrade requires Mental and Offense stat of this value.
	else if (infile.key == "requires_mentoff") power_cell_upgrade.back().requires_mentoff = toInt(infile.val);
	// @ATTR upgrade.requires_mentdef|integer|Upgrade requires Mental and Defense stat of this value.
	else if (infile.key == "requires_mentdef") power_cell_upgrade.back().requires_mentdef = toInt(infile.val);

	// @ATTR upgrade.requires_defense|integer|Upgrade requires Defense stat of this value.
	else if (infile.key == "requires_defense") power_cell_upgrade.back().requires_defense = toInt(infile.val);
	// @ATTR upgrade.requires_offense|integer|Upgrade requires Offense stat of this value.
	else if (infile.key == "requires_offense") power_cell_upgrade.back().requires_offense = toInt(infile.val);
	// @ATTR upgrade.requires_physical|integer|Upgrade requires Physical stat of this value.
	else if (infile.key == "requires_physical") power_cell_upgrade.back().requires_physical = toInt(infile.val);
	// @ATTR upgrade.requires_mental|integer|Upgrade requires Mental stat of this value.
	else if (infile.key == "requires_mental") power_cell_upgrade.back().requires_mental = toInt(infile.val);

	// @ATTR upgrade.requires_point|boolean|Upgrade requires a power point to unlock.
	else if (infile.key == "requires_point") power_cell_upgrade.back().requires_point = toBool(infile.val);
	// @ATTR upgrade.requires_level|integer|Upgrade requires at least this level for the hero.
	else if (infile.key == "requires_level") power_cell_upgrade.back().requires_level = toInt(infile.val);
	// @ATTR upgrade.requires_power|integer|Upgrade requires another power id.
	else if (infile.key == "requires_power") power_cell_upgrade.back().requires_power.push_back(toInt(infile.val));

	// @ATTR upgrade.visible_requires_status|string|Hide the upgrade if we don't have this campaign status.
	else if (infile.key == "visible_requires_status") power_cell_upgrade.back().visible_requires_status.push_back(infile.val);
	// @ATTR upgrade.visible_requires_not_status|string|Hide the upgrade if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell_upgrade.back().visible_requires_not.push_back(infile.val);

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}
