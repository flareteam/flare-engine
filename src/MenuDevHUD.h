/*
Copyright © 2014-2015 Justin Jacobs

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
 * class MenuDevHUD
 */

#ifndef MENU_DEVHUD_H
#define MENU_DEVHUD_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetLabel.h"

class MenuDevHUD : public Menu {
protected:
	void loadGraphics();

	Rect original_area;
	WidgetLabel player_pos;
	WidgetLabel mouse_pos;
	WidgetLabel target_pos;

public:
	MenuDevHUD();
	~MenuDevHUD();
	void align();

	void logic();
	virtual void render();
};

#endif
