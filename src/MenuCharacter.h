/*
Copyright © 2011-2012 Clint Bellanger

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

#pragma once
#ifndef MENU_CHARACTER_H
#define MENU_CHARACTER_H

#include "CommonIncludes.h"
#include "WidgetLabel.h"
#include "TooltipData.h"

class StatBlock;
class WidgetButton;
class WidgetLabel;
class WidgetListBox;

const int CSTAT_NAME = 0;
const int CSTAT_LEVEL = 1;
const int CSTAT_PHYSICAL = 2;
const int CSTAT_MENTAL = 3;
const int CSTAT_OFFENSE = 4;
const int CSTAT_DEFENSE = 5;
const int CSTAT_COUNT = 6;
const int STATLIST_COUNT = 17;

class CharStat {
public:
	WidgetLabel *label;
	WidgetLabel *value;
	SDL_Rect hover;
	TooltipData tip;
	bool visible;

	void setHover(int x, int y, int w, int h) {
		hover.x=x;
		hover.y=y;
		hover.w=w;
		hover.h=h;
	}
};

class MenuCharacter : public Menu {
private:
	StatBlock *stats;

	WidgetButton *closeButton;
	WidgetButton *upgradeButton[4];
	WidgetLabel *labelCharacter;
	WidgetLabel *labelUnspent;
	WidgetListBox *statList;
	CharStat cstat[CSTAT_COUNT];

	void loadGraphics();
	SDL_Color bonusColor(int stat);
	std::string statTooltip(int stat);
	int skill_points;
	bool physical_up;
	bool mental_up;
	bool offense_up;
	bool defense_up;

	// label and widget positions
	Point close_pos;
	LabelInfo title;
	Point upgrade_pos[4];
	Point statlist_pos;
	int statlist_rows;
	int statlist_scrollbar_offset;
	LabelInfo unspent_pos;
	LabelInfo label_pos[CSTAT_COUNT];
	SDL_Rect value_pos[CSTAT_COUNT];
	bool show_upgrade[4];
	bool show_stat[STATLIST_COUNT];

public:
	MenuCharacter(StatBlock *stats);
	~MenuCharacter();
	void update();
	void logic();
	void render();
	void refreshStats();
	TooltipData checkTooltip();
	bool checkUpgrade();
	int getUnspent() { return skill_points; }

	bool newPowerNotification;

	TabList tablist;

};

#endif
