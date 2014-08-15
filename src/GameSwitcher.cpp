/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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

/*
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

#include "GameSwitcher.h"
#include "GameStateTitle.h"
#include "GameStateCutscene.h"
#include "SharedResources.h"
#include "Settings.h"
#include "FileParser.h"
#include "UtilsParsing.h"

#include <typeinfo>

using namespace std;

GameSwitcher::GameSwitcher() {

	// The initial state is the intro cutscene and then title screen
	GameStateTitle *title=new GameStateTitle();
	GameStateCutscene *intro = new GameStateCutscene(title);

	currentState = intro;

	if (!intro->load("cutscenes/intro.txt")) {
		delete intro;
		currentState = title;
	}

	label_fps = new WidgetLabel();
	done = false;
	music = NULL;
	loadMusic();
	loadFPS();
}

void GameSwitcher::loadMusic() {
	if (AUDIO && MUSIC_VOLUME) {
		Mix_FreeMusic(music);
		music = NULL;

		std::string music_filename = "";
		FileParser infile;
		// @CLASS GameSwitcher: Default music|Description of engine/default_music.txt
		if (infile.open("engine/default_music.txt", true, "")) {
			while (infile.next()) {
				// @ATTR music|string|Filename of a music file to play during game states that don't already have music.
				if (infile.key == "music") music_filename = infile.val;
			}
			infile.close();
		}

		if (music_filename != "") {
			music = Mix_LoadMUS((mods->locate(music_filename)).c_str());
			if (!music)
				logError("GameSwitcher: Mix_LoadMUS: %s\n", Mix_GetError());
		}
	}

	if (music) {
		Mix_VolumeMusic(MUSIC_VOLUME);
		Mix_PlayMusic(music, -1);
	}
}

void GameSwitcher::logic() {
	// reset the mouse cursor
	curs->logic();

	// Check if a the game state is to be changed and change it if necessary, deleting the old state
	GameState* newState = currentState->getRequestedGameState();
	if (newState != NULL) {
		delete currentState;
		currentState = newState;
		currentState->load_counter++;

		// reload the fps meter position
		loadFPS();

		// if this game state does not provide music, use the title theme
		if (!currentState->hasMusic)
			if (!Mix_PlayingMusic())
				if (music)
					Mix_PlayMusic(music, -1);
	}

	currentState->logic();

	// Check if the GameState wants to quit the application
	done = currentState->isExitRequested();

	if (currentState->reload_music) {
		loadMusic();
		currentState->reload_music = false;
	}
}

void GameSwitcher::showFPS(int fps) {
	if (SHOW_FPS) {
		if (!label_fps) label_fps = new WidgetLabel();
		string sfps = toString(typeid(fps), &fps) + string(" fps");
		label_fps->set(fps_position.x, fps_position.y, JUSTIFY_LEFT, VALIGN_TOP, sfps, fps_color);
		label_fps->render();
	}
}

void GameSwitcher::loadFPS() {
	// Load FPS rendering settings
	FileParser infile;
	// @CLASS GameSwitcher: FPS counter|Description of menus/fps.txt
	if (infile.open("menus/fps.txt")) {
		while(infile.next()) {
			// @ATTR position|x (integer), y (integer), align (alignment)|Position of the fps counter.
			if(infile.key == "position") {
				fps_position.x = popFirstInt(infile.val);
				fps_position.y = popFirstInt(infile.val);
				fps_corner = popFirstString(infile.val);
			}
			// @ATTR color|r (integer), g (integer), b (integer)|Color of the fps counter text.
			else if(infile.key == "color") {
				fps_color = toRGB(infile.val);
			}
		}
		infile.close();
	}

	// this is a dummy string used to approximate the fps position when aligned to the right
	font->setFont("font_regular");
	fps_position.w = font->calc_width("00 fps");
	fps_position.h = font->getLineHeight();

	alignToScreenEdge(fps_corner, &fps_position);

	// Delete the label object if it exists (we'll recreate this with showFPS())
	if (label_fps) {
		delete label_fps;
		label_fps = NULL;
	}
}

bool GameSwitcher::isLoadingFrame() {
	if (currentState->load_counter > 0) {
		currentState->load_counter--;
		return true;
	}

	return false;
}

void GameSwitcher::render() {
	currentState->render();
	curs->render();
}

GameSwitcher::~GameSwitcher() {
	delete currentState;
	delete label_fps;
	Mix_FreeMusic(music);
}

