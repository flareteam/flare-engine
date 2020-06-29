/*
Copyright © 2020 Justin Jacobs

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

#ifndef MENU_GAMEOVER_H
#define MENU_GAMEOVER_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetLabel.h"

class WidgetButton;

class MenuGameOver : public Menu {
protected:
	WidgetButton *button_continue;
	WidgetButton *button_exit;
	WidgetLabel label;

public:
	MenuGameOver();
	~MenuGameOver();

	void logic();
	void align();
	void close();
	void disableSave();
	virtual void render();

	bool continue_clicked;
	bool exit_clicked;
};

#endif
