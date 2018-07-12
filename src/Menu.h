/*
Copyright © 2011-2012 kitano
Copyright © 2013-2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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
 * class Menu
 *
 * The base class for Menu objects
 */

#ifndef MENU_H
#define MENU_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "Widget.h"

class Menu {
public:
	Menu();
	virtual ~Menu();

	void setBackground(const std::string& background_image);
	void setBackgroundDest(Rect &dest);
	void setBackgroundClip(Rect &clip);
	virtual void align();
	virtual void render();
	virtual void setWindowPos(int x, int y);

	bool visible;
	Rect window_area;
	int alignment;

	virtual bool parseMenuKey(const std::string &key, const std::string &val);

	SoundID sfx_open;
	SoundID sfx_close;

	TabList tablist;
	virtual TabList* getCurrentTabList();
	virtual void defocusTabLists();

private:
	Sprite *background;
	Point window_area_base;
};

#endif

