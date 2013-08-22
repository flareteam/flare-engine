/*
Copyright © 2011-2012 Pavel Kirpichyov (Cheshire)

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
 * MenuEnemy
 *
 * Handles the display of the Enemy info of the screen
 */

#pragma once
#ifndef MENU_ENEMY_H
#define MENU_ENEMY_H

#include "CommonIncludes.h"
#include "Settings.h"
#include "WidgetLabel.h"

class Enemy;

class MenuEnemy : public Menu {
private:
	SDL_Surface *bar_hp;
	SDL_Rect bar_pos;
	LabelInfo text_pos;
	bool custom_text_pos;
	SDL_Color color_normal;
public:
	MenuEnemy();
	~MenuEnemy();
	Enemy *enemy;
	void loadGraphics();
	void handleNewMap();
	void logic();
	void render();
	int timeout;
};

#endif
