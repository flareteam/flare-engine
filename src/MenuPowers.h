/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2014 Henrik Andersson
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

#ifndef MENU_POWERS_H
#define MENU_POWERS_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "Utils.h"

class MenuActionBar;
class StatBlock;
class TooltipData;
class WidgetButton;
class WidgetLabel;
class WidgetSlot;
class WidgetTabControl;

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
	Power_Menu_Cell();

	int id;
	int tab;
	Point pos;
	std::vector<int> requires_primary;
	int requires_level;
	int upgrade_level;
	std::vector<int> upgrades;

	std::vector<int> requires_power;
	std::vector<int> requires_power_cell;

	bool requires_point;
	bool passive_on;
	std::vector<StatusID> visible_requires_status;
	std::vector<StatusID> visible_requires_not;
};

class MenuPowers : public Menu {
private:
	static const bool UPGRADE_POWER_ALL_TABS = true;

	void loadGraphics();
	void loadTab(FileParser &infile);
	void loadPower(FileParser &infile);
	void loadUpgrade(FileParser &infile);

	bool checkRequirements(int pci);
	bool checkUnlocked(int pci);
	bool checkCellVisible(int pci);
	bool checkUnlock(int pci);
	bool checkUpgrade(int pci);

	int getCellByPowerIndex(int power_index, const std::vector<Power_Menu_Cell>& cell);
	int getNextLevelCell(int pci);

	void replaceCellWithUpgrade(int pci, int uci);
	void upgradePower(int pci, bool ignore_tab);

	void setUnlockedPowers();
	int getPointsUsed();

	void createTooltip(TooltipData* tip_data, int slot_num, const std::vector<Power_Menu_Cell>& power_cells, bool show_unlock_prompt);
	void renderPowers(int tab_num);

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

	Point close_pos;
	Rect tab_area;

	int points_left;
	std::vector<Power_Menu_Tab> tabs;
	std::string default_background;

	WidgetLabel *label_powers;
	WidgetLabel *label_unspent;
	WidgetTabControl *tab_control;

	bool tree_loaded;

	size_t prev_powers_list_size;

	int default_power_tab;

public:
	MenuPowers(StatBlock *_stats, MenuActionBar *_action_bar);
	~MenuPowers();
	void align();

	void loadPowerTree(const std::string &filename);

	void logic();
	void render();

	void renderTooltips(const Point& position);
	int click(const Point& mouse);
	void upgradeByCell(int pci);

	void applyPowerUpgrades();
	void resetToBasePowers();

	bool meetsUsageStats(int power_index);

	std::vector<WidgetSlot*> slots; // power slot Widgets

	bool newPowerNotification;


	std::vector<TabList> tablist_pow;

	bool isTabListSelected();
	int getSelectedCellIndex();
	void setNextTabList(TabList *tl);
	TabList* getCurrentTabList();
	void defocusTabLists();
};
#endif
