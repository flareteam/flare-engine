/*
Copyright © 2014 Justin Jacobs

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
 * class MenuDevConsole
 */

#ifndef MENU_DEVCONSOLE_H
#define MENU_DEVCONSOLE_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetButton.h"
#include "WidgetInput.h"
#include "WidgetLabel.h"
#include "WidgetLog.h"

class MenuDevConsole : public Menu {
protected:
	void loadGraphics();
	void execute();

	WidgetButton *button_close;
	WidgetButton *button_confirm;
	WidgetInput *input_box;
	WidgetLog *log_history;

	WidgetLabel label;

	TabList tablist;
	LabelInfo title;
	Rect history_area;

	Color color_echo;
	Color color_error;

	unsigned long input_scrollback_pos;
	std::vector<std::string> input_scrollback;

public:
	MenuDevConsole();
	~MenuDevConsole();
	void align();

	void logic();
	virtual void render();

	bool inputFocus();
	void reset();
};

#endif
