/*
Copyright © 2011-2012 kitano
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert

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

#include "Menu.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

Menu::Menu()
	: visible(false)
	, sfx_open(0)
	, sfx_close(0)
	, background(NULL) {
}

Menu::~Menu() {
	if (background) delete background;
}

void Menu::setBackground(std::string background_image) {
	Image *graphics;

	if (background) {
		delete background;
		background = NULL;
	}

	graphics = render_device->loadImage(background_image);
	if (graphics) {
		background = graphics->createSprite();
		background->setClip(0,0,window_area.w,window_area.h);
		background->setDest(window_area);
		graphics->unref();
	}
}

void Menu::setBackgroundDest(Rect &dest) {
	if (background) background->setDest(dest);
}

void Menu::setBackgroundClip(Rect &clip) {
	if (background) background->setClip(clip);
}

void Menu::render() {
	if (background)
		render_device->render(background);
}

/**
 * Aligns the menu relative to one of these positions:
 * topleft, top, topright, left, center, right, bottomleft, bottom, bottomright
 */
void Menu::align() {
	window_area.x = window_area_base.x;
	window_area.y = window_area_base.y;

	alignToScreenEdge(alignment, &window_area);

	if (background) {
		background->setClip(
			0,
			0,
			window_area.w,
			window_area.h
		);
		background->setDest(window_area);
	}
}

void Menu::setWindowPos(int x, int y) {
	window_area_base.x = x;
	window_area_base.y = y;
}

/**
 * When reading menu config files, we use this to set common variables
 */
bool Menu::parseMenuKey(const std::string &key, const std::string &val) {
	// @CLASS Menu|Description of menus in menus/
	std::string value = val;

	if (key == "pos") {
		// @ATTR pos|x (integer), y (integer), w (integer), h (integer)|Menu position and dimensions
		value = value + ',';
		window_area = toRect(value);
		setWindowPos(window_area.x, window_area.y);
	}
	else if (key == "align") {
		// @ATTR align|alignment|Position relative to screen edges
		alignment = parse_alignment(value);
	}
	else if (key == "soundfx_open") {
		// @ATTR soundfx_open|string|Filename of a sound to play when opening this menu.
		sfx_open = snd->load(value, "Menu open tab");
	}
	else if (key == "soundfx_close") {
		// @ATTR soundfx_close|string|Filename of a sound to play when closing this menu.
		sfx_close = snd->load(value, "Menu close tab");
	}
	else {
		//not a common key
		return false;
	}

	return true;
}
