/*
Copyright © 2011-2012 kitano
Copyright © 2013 Henrik Andersson
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
#include "Utils.h"

Menu::Menu()
	: visible(false)
	, sfx_open(0)
	, sfx_close(0) {
}

Menu::~Menu() {
	background.clearGraphics();
}

void Menu::align() {
	alignToScreenEdge(alignment, &window_area);

	if (!background.graphicsIsNull()) {
		background.setClip(
			0,
			0,
			window_area.w,
			window_area.h
		);
		background.setDest(window_area);
	}
}
