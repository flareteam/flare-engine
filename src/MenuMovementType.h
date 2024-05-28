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

#ifndef MENU_MOVEMENT_TYPE_H
#define MENU_MOVEMENT_TYPE_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "Utils.h"
#include "WidgetLabel.h"

class WidgetButton;

class MenuMovementType : public Menu {
protected:
	WidgetButton *button_keyboard;
	WidgetButton *button_mouse;
	WidgetButton *button_joystick;
	WidgetLabel label_title;
	WidgetLabel label_config_hint;

	Sprite *icon_keyboard;
	Sprite *icon_mouse;
	Sprite *icon_joystick;

	Point icon_keyboard_pos;
	Point icon_mouse_pos;
	Point icon_joystick_pos;

public:
	MenuMovementType();
	~MenuMovementType();

	void logic();
	void align();
	void close();
	virtual void render();
};

#endif
