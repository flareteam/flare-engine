/*
Copyright © 2011-2012 kitano
Copyright © 2013-2014 Henrik Andersson

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
#include "SoundManager.h"

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
	ALIGNMENT alignment;

	virtual bool parseMenuKey(const std::string &key, const std::string &val);

	SoundManager::SoundID sfx_open;
	SoundManager::SoundID sfx_close;

private:
	Sprite *background;
	Point window_area_base;
};

#endif

