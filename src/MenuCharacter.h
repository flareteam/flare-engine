/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MenuCharacter
 */

#ifndef MENU_CHARACTER_H
#define MENU_CHARACTER_H

#include "CommonIncludes.h"
#include "TooltipData.h"
#include "WidgetLabel.h"

class StatBlock;
class WidgetButton;
class WidgetLabel;
class WidgetListBox;

class MenuCharacter : public Menu {
private:
	class CharStat {
	public:
		WidgetLabel *label;
		WidgetLabel *value;
		Rect hover;
		TooltipData tip;
		Rect value_pos;

		void setHover(int x, int y) {
			hover.x = x + value_pos.x;
			hover.y = y + value_pos.y;
			hover.w = value_pos.w;
			hover.h = value_pos.h;
		}
	};

	enum {
		CSTAT_NAME = 0,
		CSTAT_LEVEL = 1
	};

	WidgetButton *closeButton;
	WidgetLabel *labelCharacter;
	WidgetLabel *labelUnspent;
	WidgetListBox *statList;
	std::vector<WidgetButton*> upgradeButton;

	void loadGraphics();
	Color bonusColor(int stat);
	std::string statTooltip(int stat);
	std::string damageTooltip(size_t dmg_type);
	bool checkSkillPoints();
	int skill_points;
	std::vector<bool> primary_up;

	// label and widget positions
	Point statlist_pos;
	int statlist_rows;
	int statlist_scrollbar_offset;
	std::vector<bool> show_stat;
	bool show_resists;

	std::vector<CharStat> cstat;

	std::vector<int*> base_stats;
	std::vector<int*> base_stats_add;
	std::vector< std::vector<int>* > base_bonus;

	int name_max_width;

public:
	explicit MenuCharacter();
	~MenuCharacter();
	void align();

	void logic();
	void render();
	void refreshStats();
	void renderTooltips(const Point& position);
	bool checkUpgrade();
	int getUnspent() {
		return skill_points;
	}
};

#endif
