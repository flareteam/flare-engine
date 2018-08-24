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

class MenuPowersTab {
public:
	std::string title;
	std::string background;

	MenuPowersTab()
		: title("")
		, background("") {
	}
};

class MenuPowersCell {
public:
	MenuPowersCell();
	bool isVisible();

	int id;
	int requires_level;
	bool requires_point;
	std::vector<int> requires_primary;
	std::vector<int> requires_power;
	std::vector<StatusID> visible_requires_status;
	std::vector<StatusID> visible_requires_not;

	int upgrade_level;
	bool passive_on;
	bool is_unlocked;

	size_t group;
	MenuPowersCell* next; // TODO should we also have "parent"?
};

class MenuPowersCellGroup {
public:
	MenuPowersCellGroup();
	MenuPowersCell* getCurrent();
	MenuPowersCell* getBonusCurrent(MenuPowersCell* pcell);
	int getBonusLevels();

	int tab;
	Point pos;

	size_t current_cell;
	std::vector<MenuPowersCell> cells;

	WidgetButton* upgrade_button;

	std::vector< std::pair<size_t, int> > bonus_levels;
};

class MenuPowers : public Menu {
private:
	static const bool UPGRADE_POWER_ALL_TABS = true;

	void loadGraphics();
	void loadTab(FileParser &infile);
	void loadPower(FileParser &infile);
	void loadUpgrade(FileParser &infile, std::vector<MenuPowersCell>& power_cell_upgrade);

	bool checkRequirements(MenuPowersCell* pcell);
	bool checkUnlocked(MenuPowersCell* pcell);
	bool checkUnlock(MenuPowersCell* pcell);
	bool checkUpgrade(MenuPowersCell* pcell);
	void lockCell(MenuPowersCell* pcell);
	bool isBonusCell(MenuPowersCell* pcell);

	MenuPowersCell* getCellByPowerIndex(int power_index);

	void upgradePower(MenuPowersCell* pcell, bool ignore_tab);

	int getPointsUsed();

	void createTooltip(TooltipData* tip_data, MenuPowersCell* pcell, bool show_unlock_prompt);
	void renderPowers(int tab_num);

	std::vector<MenuPowersCellGroup> power_cell;
	bool skip_section;

	Sprite *powers_unlock;
	Sprite *overlay_disabled;
	std::vector<Sprite *> tree_surf;
	WidgetButton *closeButton;

	Point close_pos;
	Rect tab_area;

	int points_left;
	std::vector<MenuPowersTab> tabs;
	std::string default_background;

	WidgetLabel *label_powers;
	WidgetLabel *label_unspent;
	WidgetTabControl *tab_control;

	bool tree_loaded;

	size_t prev_powers_list_size;

	int default_power_tab;

public:
	MenuPowers();
	~MenuPowers();
	void align();

	void loadPowerTree(const std::string &filename);

	void logic();
	void render();

	void renderTooltips(const Point& position);
	int click(const Point& mouse);
	void upgradeBySlotIndex(int slot_index);

	void setUnlockedPowers();
	void resetToBasePowers();

	bool meetsUsageStats(int power_index);

	void clearActionBarBonusLevels();
	void clearBonusLevels();
	void addBonusLevels(int power_index, int bonus_levels);
	std::string getItemBonusPowerReqString(int power_index);

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
