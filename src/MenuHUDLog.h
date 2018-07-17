/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MenuHUDLog
 */

#ifndef MENU_HUD_LOG_H
#define MENU_HUD_LOG_H

#include "CommonIncludes.h"
#include "Utils.h"

class MenuHUDLog : public Menu {
private:

	int calcDuration(const std::string& s);

	std::vector<std::string> log_msg;
	std::vector<int> msg_age;
	std::vector<Sprite *> msg_buffer;

	int paragraph_spacing;

	Sprite *overlay_bg;
	bool enable_overlay;
	bool click_to_dismiss;
	bool start_at_bottom;
	bool overlay_at_bottom;

public:
	enum {
		MSG_NORMAL = 0,
		MSG_UNIQUE = 1
	};

	MenuHUDLog();
	~MenuHUDLog();
	void logic();
	void render();
	void add(const std::string& s, int type);
	void remove(int msg_index);
	void clear();
	void renderOverlay();

	bool hide_overlay;
};

#endif
