/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2014 Henrik Andersson

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

#ifndef MENU_POWERS_H
#define MENU_POWERS_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "Utils.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetTabControl.h"
#include "FileParser.h"

class StatBlock;
class TooltipData;
class WidgetSlot;
class MenuActionBar;

class Power_Menu_Tab {
public:
	std::string title;
	std::string background;

	Power_Menu_Tab()
		: title("")
		, background("") {
	}
};

class Power_Menu_Cell {
public:
	int id;
	int tab;
	Point pos;
	int requires_physoff;
	int requires_physdef;
	int requires_mentoff;
	int requires_mentdef;
	int requires_defense;
	int requires_offense;
	int requires_physical;
	int requires_mental;
	int requires_level;
	int upgrade_level;
	std::vector<int> upgrades;

	std::vector<int> requires_power;
	std::vector<int> requires_power_cell;

	bool requires_point;
	bool passive_on;
	std::vector<std::string> visible_requires_status;
	std::vector<std::string> visible_requires_not;
	Power_Menu_Cell()
		: id(-1)
		, tab(0)
		, pos(Point())
		, requires_physoff(0)
		, requires_physdef(0)
		, requires_mentoff(0)
		, requires_mentdef(0)
		, requires_defense(0)
		, requires_offense(0)
		, requires_physical(0)
		, requires_mental(0)
		, requires_level(0)
		, upgrade_level(0)
		, upgrades()
		, requires_power()
		, requires_point(false)
		, passive_on(false) {
	}
};

class MenuPowers : public Menu {
private:
	StatBlock *stats;
	MenuActionBar *action_bar;
	std::vector<Power_Menu_Cell> power_cell;           // the current visible power cells
	std::vector<Power_Menu_Cell> power_cell_base;      // only base powers
	std::vector<Power_Menu_Cell> power_cell_upgrade;   // only upgrades
	std::vector<Power_Menu_Cell> power_cell_all;       // power_cell_base + power_cell_upgrade
	std::vector<Power_Menu_Cell> power_cell_unlocked;  // only base powers and upgrades that have been unlocked
	std::vector<WidgetButton*> upgradeButtons;
	bool skip_section;

	Sprite *powers_unlock;
	Sprite *overlay_disabled;
	std::vector<Sprite *> tree_surf;
	WidgetButton *closeButton;

	LabelInfo title;
	LabelInfo unspent_points;
	Point close_pos;
	Rect tab_area;

	int points_left;
	std::vector<Power_Menu_Tab> tabs;
	std::string default_background;

	WidgetLabel label_powers;
	WidgetLabel stat_up;
	WidgetTabControl *tab_control;

	void loadGraphics();
	void displayBuild(int power_id);
	bool powerUnlockable(int id);
	void renderPowers(int tab_num);

	Color color_bonus;
	Color color_penalty;
	Color color_flavor;

	int id_by_powerIndex(int power_index, const std::vector<Power_Menu_Cell>& cell);
	int nextLevel(int power_cell_index);
	void replacePowerCellDataByUpgrade(int power_cell_index, int upgrade_cell_index);
	int getPointsUsed();
	void setUnlockedPowers();

	bool powerIsVisible(int id);
	void loadTab(FileParser &infile);
	void loadPower(FileParser &infile);
	void loadUpgrade(FileParser &infile);

	bool tree_loaded;

public:
	MenuPowers(StatBlock *_stats, MenuActionBar *_action_bar);
	~MenuPowers();
	void align();

	void loadPowerTree(const std::string &filename);
	void logic();
	void render();
	TooltipData checkTooltip(Point mouse);
	void generatePowerDescription(TooltipData* tip, int slot_num, const std::vector<Power_Menu_Cell>& power_cells, bool show_unlock_prompt);
	bool baseRequirementsMet(int id);
	bool requirementsMet(int id);
	int click(Point mouse);
	void upgradePower(int power_cell_index);
	bool canUpgrade(int power_cell_index);
	void applyPowerUpgrades();
	bool meetsUsageStats(int powerid);
	int getUnspent() {
		return points_left;
	}
	void resetToBasePowers();

	std::vector<WidgetSlot*> slots; // power slot Widgets

	bool newPowerNotification;

	TabList tablist;

};
#endif
