/*
Copyright © 2018 Justin Jacobs

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

#ifndef MENU_TOUCHCONTROLS_H
#define MENU_TOUCHCONTROLS_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "Utils.h"

class MenuTouchControls : public Menu {
private:
	void alignInput(Point& center, const Point& center_base, const int radius, const int _align);
	void renderInput(const Point& center, const int radius, const Color& color);

	int move_radius;
	Point move_center;
	Point move_center_base;
	int move_align;
	int move_deadzone;

	int main1_radius;
	Point main1_center;
	Point main1_center_base;
	int main1_align;

	int main2_radius;
	Point main2_center;
	Point main2_center_base;
	int main2_align;

	int radius_padding;
public:
	MenuTouchControls();
	~MenuTouchControls();

	void logic();
	void align();
	void render();

	bool checkAllowMain1();
};

#endif
