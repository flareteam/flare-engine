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

#include "FileParser.h"
#include "MenuDevHUD.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

MenuDevHUD::MenuDevHUD() : Menu() {

	// Load config settings
	FileParser infile;
	// @CLASS MenuDevHUD|Description of menus/devhud.txt
	if(infile.open("menus/devhud.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else
				infile.error("MenuDevHUD: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	original_area = window_area;

	align();
}

void MenuDevHUD::align() {
	std::stringstream ss;
	int line_width = 0;
	int line_height = font->getLineHeight();

	ss.str("");
	ss << msg->get("Player (x,y): ") << pc->stats.pos.x << ", " << pc->stats.pos.y;
	player_pos.set(window_area.x, window_area.y, JUSTIFY_LEFT, VALIGN_TOP, ss.str(), font->getColor("menu_normal"));
	line_width = std::max(line_width, player_pos.bounds.w);

	FPoint target = screen_to_map(inpt->mouse.x,  inpt->mouse.y, pc->stats.pos.x, pc->stats.pos.y);
	ss.str("");
	ss << msg->get("Target (x,y): ") << target.x << ", " << target.y;
	target_pos.set(window_area.x, window_area.y+line_height, JUSTIFY_LEFT, VALIGN_TOP, ss.str(), font->getColor("menu_normal"));
	line_width = std::max(line_width, target_pos.bounds.w);

	ss.str("");
	ss << msg->get("Mouse (x,y): ") << inpt->mouse.x << ", " << inpt->mouse.y;
	mouse_pos.set(window_area.x, window_area.y+line_height*2, JUSTIFY_LEFT, VALIGN_TOP, ss.str(), font->getColor("menu_normal"));
	line_width = std::max(line_width, mouse_pos.bounds.w);

	ss.str("");
	ss << msg->get("Target distance: ") << calcDist(pc->stats.pos, target);
	target_distance.set(window_area.x, window_area.y+line_height*3, JUSTIFY_LEFT, VALIGN_TOP, ss.str(), font->getColor("menu_normal"));
	line_width = std::max(line_width, mouse_pos.bounds.w);

	window_area = original_area;
	window_area.w = line_width;
	window_area.h = line_height*4;

	Menu::align();
}

void MenuDevHUD::logic() {
	if (visible) {
		align();
	}
}

void MenuDevHUD::render() {
	if (visible) {
		// background
		Menu::render();

		player_pos.render();
		mouse_pos.render();
		target_pos.render();
		target_distance.render();
	}
}

MenuDevHUD::~MenuDevHUD() {
}

