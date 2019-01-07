/*
Copyright © 2011-2012 kitano
Copyright © 2014 Henrik Andersson
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
 * class MenuExit
 */

#ifndef MENU_EXIT_H
#define MENU_EXIT_H

#include "CommonIncludes.h"
#include "Menu.h"

class MenuConfig;

class MenuExit : public Menu {
protected:
	MenuConfig* menu_config;
	bool exitClicked;

public:
	MenuExit();
	~MenuExit();
	void align();
	void logic();
	virtual void render();

	bool isExitRequested() {
		return exitClicked;
	}

	void disableSave();
	void handleCancel();

	bool reload_music;
};

#endif
