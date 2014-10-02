/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Justin Jacobs
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
	Sprite *bar;
	WidgetLabel *label;
	int stat_cur;
	int stat_max;
	Point mouse;
	Rect bar_pos;
	LabelInfo text_pos;
	bool orientation;
	bool custom_text_pos;
	std::string custom_string;
	Color color_normal;
	std::string bar_gfx;
	std::string bar_gfx_background;

public:
	MenuStatBar(std::string type);
	~MenuStatBar();
	void loadGraphics();
	void update(int _stat_cur, int _stat_max, Point _mouse, std::string _custom_string = "");
	void render();
};

#endif
