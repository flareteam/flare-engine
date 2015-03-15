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

#include "CommonIncludes.h"
#include "GameStateConfigBase.h"
#include "GameStateConfigDesktop.h"
#include "GameStateResolution.h"
#include "GameStateTitle.h"
#include "MenuConfirm.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

GameStateResolution::GameStateResolution(bool fullscreen, bool hwsurface, bool doublebuf, bool _updated_min_screen)
	: GameState()
	, confirm(NULL)
	, confirm_ticks(0)
	, old_fullscreen(fullscreen)
	, old_hwsurface(hwsurface)
	, old_doublebuf(doublebuf)
	, updated_min_screen(_updated_min_screen)
	, initialized(false) {
}

void GameStateResolution::logic() {
	if (!initialized) {
		initialized = true;
		bool settings_changed = compareVideoSettings();

		// Clean up shared resources
		if (settings_changed) {
			delete icons;
			icons = NULL;

			delete curs;
			curs = NULL;
		}

		// Apply the new resolution
		// if it fails, don't create the dialog box (this will make the game continue straight to the title screen)
		if (settings_changed && applyVideoSettings())
			confirm = new MenuConfirm(msg->get("OK"),msg->get("Use this resolution?"));

		if (confirm) {
			confirm->visible = true;
			confirm_ticks = MAX_FRAMES_PER_SEC * 10;
		}
		else {
			if (updated_min_screen)
				render_device->windowUpdateMinSize();
			render_device->windowResize();
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
			return;
		}
	}

	if (confirm) {
		confirm->logic();

		if (confirm_ticks > 0) confirm_ticks--;

		if (confirm->confirmClicked) {
			saveSettings();
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
		}
		else if (confirm->cancelClicked || confirm_ticks == 0) {
			cleanup();
			FULLSCREEN = old_fullscreen;
			HWSURFACE = old_hwsurface;
			DOUBLEBUF = old_doublebuf;
			if (applyVideoSettings()) {
				saveSettings();
			}
			delete requestedGameState;
#ifdef __ANDROID__
			requestedGameState = new GameStateConfigBase();
#else
			requestedGameState = new GameStateConfigDesktop();
#endif
		}
	}
	else {
		if (updated_min_screen)
			render_device->windowUpdateMinSize();
		render_device->windowResize();
		delete requestedGameState;
		requestedGameState = new GameStateTitle();
	}
}

void GameStateResolution::render() {
	if (confirm)
		confirm->render();
}

/**
 * Tries to apply the selected video settings, reverting back to the old settings upon failure
 */
bool GameStateResolution::applyVideoSettings() {
	// Attempt to apply the new settings
	int status = render_device->createContext(SCREEN_W, SCREEN_H);

	// If the new settings fail, revert to the old ones
	if (status == -1) {
		logError("GameStateResolution: Error during SDL_SetVideoMode: %s", SDL_GetError());
		FULLSCREEN = old_fullscreen;
		HWSURFACE = old_hwsurface;
		DOUBLEBUF = old_doublebuf;
		render_device->createContext(SCREEN_W, SCREEN_H);
		SharedResources::loadIcons();
		curs = new CursorManager();
		return false;

	}
	else {
		// If the new settings succeed, finish loading resources
		SharedResources::loadIcons();
		curs = new CursorManager();
		return true;
	}
}

/**
 * Checks if the video settings have changed
 * Returns true if they have, otherwise returns false
 */
bool GameStateResolution::compareVideoSettings() {
	return (FULLSCREEN != old_fullscreen ||
			HWSURFACE != old_hwsurface ||
			DOUBLEBUF != old_doublebuf);
}

void GameStateResolution::cleanup() {
	if (confirm) {
		delete confirm;
		confirm = NULL;
	}
}

GameStateResolution::~GameStateResolution() {
	cleanup();
}
