/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2016 Justin Jacobs
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
 * MenuStatBar
 *
 * Handles the display of a status bar
 */

#ifndef MENU_STATBAR_H
#define MENU_STATBAR_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "WidgetLabel.h"

class WidgetLabel;

class MenuStatBar : public Menu {
private:
	bool disappear();

	enum {
		HORIZONTAL = 0,
		VERTICAL = 1
	};

	union MenuStatBarValue {
		float Float;
		unsigned long Unsigned;
	};

	Sprite *bar;
	WidgetLabel *label;
	MenuStatBarValue stat_min;
	MenuStatBarValue stat_cur;
	MenuStatBarValue stat_cur_prev;
	MenuStatBarValue stat_max;
	Rect bar_pos;
	LabelInfo text_pos;
	bool orientation;
	bool custom_text_pos;
	std::string custom_string;
	std::string bar_gfx;
	std::string bar_gfx_background;
	short type;
	Timer timeout;
	Point bar_fill_offset;
	Point bar_fill_size;


public:
	enum {
		TYPE_HP = 0,
		TYPE_MP = 1,
		TYPE_XP = 2
	};

	explicit MenuStatBar(short _type);
	~MenuStatBar();
	void loadGraphics();
	void update();
	void render();
};

#endif
