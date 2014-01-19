/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller

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

#pragma once
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

class Power_Menu_Cell {
public:
	short id;
	short tab;
	Point pos;
	short requires_physoff;
	short requires_physdef;
	short requires_mentoff;
	short requires_mentdef;
	short requires_defense;
	short requires_offense;
	short requires_physical;
	short requires_mental;
	short requires_level;
	std::vector<short> upgrades;

	std::vector<short> requires_power;

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
		, upgrades()
		, requires_power()
		, requires_point(false)
		, passive_on(false)
	{
	}
};

class MenuPowers : public Menu {
private:
	StatBlock *stats;
	MenuActionBar *action_bar;
	std::vector<Power_Menu_Cell> power_cell;
	std::vector<Power_Menu_Cell> upgrade;
	std::vector<WidgetButton*> upgradeButtons;
	bool skip_section;

	Sprite powers_unlock;
	Sprite overlay_disabled;
	std::vector<Sprite> tree_surf;
	WidgetButton *closeButton;
	bool pressed;

	LabelInfo title;
	LabelInfo unspent_points;
	Point close_pos;
	Rect tab_area;

	short points_left;
	short tabs_count;
	std::vector<std::string> tab_titles;
	std::vector<std::string> tree_image_files;

	WidgetLabel label_powers;
	WidgetLabel stat_up;
	WidgetTabControl *tabControl;

	void loadGraphics();
	void displayBuild(int power_id);
	bool powerUnlockable(int power_index, const std::vector<Power_Menu_Cell>& power_cells);
	void renderPowers(int tab_num);

	SDL_Color color_bonus;
	SDL_Color color_penalty;

	short id_by_powerIndex(short power_index, const std::vector<Power_Menu_Cell>& cell);
	short nextLevel(short power_cell_index);
	void upgradePower(short power_cell_index);
	void replacePowerCellDataByUpgrade(short power_cell_index, short upgrade_cell_index);

	bool powerIsVisible(short power_index, const std::vector<Power_Menu_Cell>& power_cells);
	void loadHeader(FileParser &infile);
	void loadPower(FileParser &infile);
	void loadUpgrade(FileParser &infile);

public:
	MenuPowers(StatBlock *_stats, MenuActionBar *_action_bar);
	~MenuPowers();
	void update();
	void logic();
	void render();
	TooltipData checkTooltip(Point mouse);
	void generatePowerDescription(TooltipData* tip, int slot_num, const std::vector<Power_Menu_Cell>& power_cells);
	bool baseRequirementsMet(int power_index, const std::vector<Power_Menu_Cell>& power_cells);
	bool requirementsMet(int power_index, const std::vector<Power_Menu_Cell>& power_cells);
	int click(Point mouse);
	bool unlockClick(Point mouse);
	void applyPowerUpgrades();
	bool meetsUsageStats(unsigned powerid);
	short getUnspent() { return points_left; }

	std::vector<WidgetSlot*> slots; // power slot Widgets

	TabList tablist;

};
#endif
