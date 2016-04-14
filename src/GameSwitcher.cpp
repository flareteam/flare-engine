/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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

GameSwitcher::GameSwitcher()
	: background(NULL)
	, background_image(NULL)
	, background_filename("")
	, fps_ticks(0)
	, last_fps(0)
{

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
	loadMusic();
	loadFPS();

	loadBackgroundList();

	if (currentState->has_background)
		loadBackgroundImage();
}

void GameSwitcher::loadMusic() {
	if (!AUDIO) return;

	if (MUSIC_VOLUME > 0) {
		std::string music_filename = "";
		FileParser infile;
		// @CLASS GameSwitcher: Default music|Description of engine/default_music.txt
		if (infile.open("engine/default_music.txt", true, "")) {
			while (infile.next()) {
				// @ATTR music|string|Filename of a music file to play during game states that don't already have music.
				if (infile.key == "music") music_filename = infile.val;
				else infile.error("GameSwitcher: '%s' is not a valid key.", infile.key.c_str());
			}
			infile.close();
		}

		//load and play music
		snd->loadMusic(music_filename);
	}
	else {
		snd->stopMusic();
	}
}

void GameSwitcher::loadBackgroundList() {
	background_list.clear();
	freeBackground();

	FileParser infile;
	// @CLASS GameSwitcher: Background images|Description of engine/menu_backgrounds.txt
	if (infile.open("engine/menu_backgrounds.txt", true, "")) {
		while (infile.next()) {
			// @ATTR background|string|Filename of a background image to be added to the pool of random menu backgrounds
			if (infile.key == "background") background_list.push_back(infile.val);
			else infile.error("GameSwitcher: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
}

void GameSwitcher::loadBackgroundImage() {
	if (background_list.empty()) return;

	if (background_filename != "") return;

	// load the background image
	size_t index = static_cast<size_t>(rand()) % background_list.size();
	background_filename = background_list[index];
	background_image = render_device->loadImage(background_filename);
	refreshBackground();
}

void GameSwitcher::refreshBackground() {
	if (background_image) {
		background_image->ref();
		Rect dest = resizeToScreen(background_image->getWidth(), background_image->getHeight(), true, ALIGN_CENTER);

		Image *resized = background_image->resize(dest.w, dest.h);
		if (resized) {
			if (background)
				delete background;

			background = resized->createSprite();
			resized->unref();
		}

		if (background)
			background->setDest(dest);
	}
}

void GameSwitcher::freeBackground() {
	delete background;
	background = NULL;

	if (background_image) {
		background_image->unref();
		background_image = NULL;
	}

	background_filename = "";
}

void GameSwitcher::logic() {
	// reset the mouse cursor
	curs->logic();

	// Check if a the game state is to be changed and change it if necessary, deleting the old state
	GameState* newState = currentState->getRequestedGameState();
	if (newState != NULL) {
		if (currentState->reload_backgrounds || render_device->reloadGraphics())
			loadBackgroundList();

		delete currentState;
		currentState = newState;
		currentState->load_counter++;

		// reload the fps meter position
		loadFPS();

		// if this game state does not provide music, use the title theme
		if (!currentState->hasMusic)
			if (!snd->isPlayingMusic())
				loadMusic();

		// if this game state shows a background image, load it here
		if (currentState->has_background)
			loadBackgroundImage();
		else
			freeBackground();
	}

	// resize background image when window is resized
	if (inpt->window_resized && currentState->has_background) {
		refreshBackground();
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
	if (SHOW_FPS && SHOW_HUD) {
		if (!label_fps) label_fps = new WidgetLabel();
		if (fps_ticks == 0) {
			fps_ticks = MAX_FRAMES_PER_SEC / 4;

			int avg_fps = (fps + last_fps) / 2;
			last_fps = fps;
			std::string sfps = toString(typeid(avg_fps), &avg_fps) + std::string(" fps");
			Rect pos = fps_position;
			alignToScreenEdge(fps_corner, &pos);
			label_fps->set(pos.x, pos.y, JUSTIFY_LEFT, VALIGN_TOP, sfps, fps_color);
		}
		label_fps->render();
		fps_ticks--;
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
				fps_corner = parse_alignment(popFirstString(infile.val));
			}
			// @ATTR color|r (integer), g (integer), b (integer)|Color of the fps counter text.
			else if(infile.key == "color") {
				fps_color = toRGB(infile.val);
			}
			else {
				infile.error("GameSwitcher: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// this is a dummy string used to approximate the fps position when aligned to the right
	font->setFont("font_regular");
	fps_position.w = font->calc_width("00 fps");
	fps_position.h = font->getLineHeight();

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

bool GameSwitcher::isPaused() {
	return currentState->isPaused();
}

void GameSwitcher::render() {
	// display background
	if (background && currentState->has_background) {
		render_device->render(background);
	}

	currentState->render();
	curs->render();
}

void GameSwitcher::saveUserSettings() {
	if (currentState && currentState->save_settings_on_exit)
		saveSettings();
}

GameSwitcher::~GameSwitcher() {
	delete currentState;
	delete label_fps;
	snd->unloadMusic();
	freeBackground();
	background_list.clear();
}

