/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
Copyright © 2014-2016 Justin Jacobs

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
 * GameStateConfig
 *
 * Handle game Settings Menu
 */

#ifndef GAMESTATECONFIG_H
#define GAMESTATECONFIG_H

#include "CommonIncludes.h"
#include "GameState.h"

class MenuConfig;

class GameStateConfig : public GameState {
private:
	MenuConfig* menu_config;

public:
	explicit GameStateConfig();
	~GameStateConfig();

	void logic();
	void logicAccept();
	void logicCancel();
	void refreshFont();

	void render();
};

#endif

