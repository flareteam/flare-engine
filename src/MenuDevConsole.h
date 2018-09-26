/*
Copyright © 2014-2016 Justin Jacobs

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
#include "Utils.h"
#include "WidgetLabel.h"

class WidgetButton;
class WidgetInput;
class WidgetLog;

class MenuDevConsole : public Menu {
protected:
	void loadGraphics();
	void execute();
	void getPlayerInfo();
	void getTileInfo();
	void getEnemyInfo();
	void reset();

	WidgetButton *button_close;
	WidgetButton *button_confirm;
	WidgetInput *input_box;
	WidgetLog *log_history;

	WidgetLabel label;

	Rect history_area;

	bool first_open;

	size_t input_scrollback_pos;
	std::vector<std::string> input_scrollback;

public:
	MenuDevConsole();
	~MenuDevConsole();
	void align();
	void closeWindow();

	void logic();
	virtual void render();

	bool inputFocus();

	FPoint target;
	Timer distance_timer;
};

#endif
