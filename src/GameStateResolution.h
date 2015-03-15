/*
Copyright © 2014 Justin Jacobs

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

#ifndef GAMESTATERESOLUTION_H
#define GAMESTATERESOLUTION_H

#include "GameState.h"

class MenuConfirm;

class GameStateResolution : public GameState {
private:
	bool applyVideoSettings();
	bool compareVideoSettings();
	void cleanup();
	MenuConfirm *confirm;
	int confirm_ticks;
	bool old_fullscreen;
	bool old_hwsurface;
	bool old_doublebuf;
	bool updated_min_screen;
	bool initialized;

public:
	GameStateResolution(bool fullscreen, bool hwsurface, bool doublebuf, bool _updated_min_screen);
	~GameStateResolution();
	void logic();
	void render();
};

#endif

