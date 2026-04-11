/*
Copyright © 2026 Justin Jacobs

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

#ifndef MENU_REGIONTITLE_H
#define MENU_REGIONTITLE_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetLabel.h"

class MenuRegionTitle : public Menu {
protected:
	std::string title;
	Timer timer;
	Timer fade_in_timer;
	Timer fade_out_timer;
	WidgetLabel label;

public:
	MenuRegionTitle();
	~MenuRegionTitle();

	void logic();
	void align();
	void setTitle(const std::string& new_title);
	virtual void render();
};

#endif
