/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk

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

#ifndef GAME_SWITCHER_H
#define GAME_SWITCHER_H

#include "CommonIncludes.h"
#include "Utils.h"

const int GAME_STATE_TITLE = 0;
const int GAME_STATE_PLAY = 1;
const int GAME_STATE_LOAD = 2;
const int GAME_STATE_NEW = 3;

class GameState;
class WidgetLabel;
/**
 * class GameSwitcher
 *
 * State machine handler between main game modes that take up the entire view/control
 *
 * Examples:
 * - the main gameplay (GameEngine class)
 * - title screen
 * - new game screen (character create)
 * - load game screen
 * - maybe full-video cutscenes
 */

class GameSwitcher {
private:
	void loadBackgroundList();
	void refreshBackground();
	void freeBackground();

	GameState *currentState;

	WidgetLabel *label_fps;
	Rect fps_position;
	Color fps_color;
	ALIGNMENT fps_corner;

	Sprite *background;
	Image *background_image;
	std::string background_filename;
	std::vector<std::string> background_list;

	int fps_ticks;
	int last_fps;

public:
	GameSwitcher();
	GameSwitcher(const GameSwitcher &copy); // not implemented.
	~GameSwitcher();

	void loadMusic();
	void loadBackgroundImage();
	void loadFPS();
	bool isLoadingFrame();
	bool isPaused();
	void logic();
	void render();
	void showFPS(int fps);
	void saveUserSettings();
	bool done;
};

#endif

