/*
Copyright © 2011-2012 Clint Bellanger
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>

#include "Settings.h"
#include "Stats.h"
#include "GameSwitcher.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"

GameSwitcher *gswitch;

/**
 * Game initialization.
 */
static void init(const std::string render_device_name) {

	setPaths();
	setStatNames();

	// SDL Inits
	if ( SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0 ) {
		logError("main: Could not initialize SDL: %s", SDL_GetError());
		exit(1);
	}

	// Shared Resources set-up

	mods = new ModManager();

	if (!mods->haveFallbackMod()) {
		logError("main: Could not find the default mod in the following locations:");
		if (dirExists(PATH_DATA + "mods")) logError("%smods/", PATH_DATA.c_str());
		if (dirExists(PATH_USER + "mods")) logError("%smods/", PATH_USER.c_str());
		logError("A copy of the default mod is in the \"mods\" directory of the flare-engine repo.");
		logError("The repo is located at: https://github.com/clintbellanger/flare-engine");
		logError("Try again after copying the default mod to one of the above directories. Exiting.");
		exit(1);
	}

	if (!loadSettings()) {
		logError("%s", ("main: Could not load settings file: ‘" + PATH_CONF + FILE_SETTINGS + "’.").c_str());
		exit(1);
	}

	msg = new MessageEngine();
	font = new FontEngine();
	anim = new AnimationManager();
	comb = new CombatText();
	inpt = new InputState();
	icons = NULL;

	// Load tileset options (must be after ModManager is initialized)
	loadTilesetSettings();

	// Load miscellaneous settings
	loadMiscSettings();

	// Create render Device and Rendering Context.
	render_device = getRenderDevice(render_device_name);
	int status = render_device->createContext(VIEW_W, VIEW_H);

	if (status == -1) {

		logError("main: Error during SDL_SetVideoMode: %s", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	// initialize share icons resource
	SharedResources::loadIcons();

	// Set Gamma
	if (CHANGE_GAMMA)
		render_device->setGamma(GAMMA);

	if (AUDIO && Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024)) {
		logError("main: Error during Mix_OpenAudio: %s", SDL_GetError());
		AUDIO = false;
	}

	snd = new SoundManager();

	// initialize Joysticks
	if(SDL_NumJoysticks() == 1) {
		logInfo("1 joystick was found:");
	}
	else if(SDL_NumJoysticks() > 1) {
		logInfo("%d joysticks were found:", SDL_NumJoysticks());
	}
	else {
		logInfo("No joysticks were found.");
		ENABLE_JOYSTICK = false;
	}
	for(int i = 0; i < SDL_NumJoysticks(); i++) {
		logInfo("  Joy %d) %s", i, inpt->getJoystickName(i).c_str());
	}
	if ((ENABLE_JOYSTICK) && (SDL_NumJoysticks() > 0)) {
		joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
		logInfo("Using joystick #%d.", JOYSTICK_DEVICE);
	}

	// Set sound effects volume from settings file
	if (AUDIO)
		Mix_Volume(-1, SOUND_VOLUME);

	gswitch = new GameSwitcher();

	curs = new CursorManager();
}

static void mainLoop (bool debug_event) {
	bool done = false;
	int delay = int(floor((1000.f/MAX_FRAMES_PER_SEC)+0.5f));
	int logic_ticks = SDL_GetTicks();

	while ( !done ) {
		int loops = 0;
		int now_ticks = SDL_GetTicks();
		int prev_ticks = now_ticks;

		while (now_ticks > logic_ticks && loops < MAX_FRAMES_PER_SEC) {
			// Frames where data loading happens (GameState switching and map loading)
			// take a long time, so our loop here will think that the game "lagged" and
			// try to compensate. To prevent this compensation, we mark those frames as
			// "loading frames" and update the logic ticker without actually executing logic.
			if (gswitch->isLoadingFrame()) {
				logic_ticks = now_ticks;
				break;
			}

			SDL_PumpEvents();
			inpt->handle(debug_event);

			// Skip game logic when minimized on Android
			if (inpt->window_minimized && !inpt->window_restored)
				break;

			gswitch->logic();
			inpt->resetScroll();

			// Engine done means the user escapes the main game menu.
			// Input done means the user closes the window.
			done = gswitch->done || inpt->done;

			logic_ticks += delay;
			loops++;

			// Android only
			// When the app is minimized on Android, no logic gets processed.
			// As a result, the delta time when restoring the app is large, so the game will skip frames and appear to be running fast.
			// To counter this, we reset our delta time here when restoring the app
			if (inpt->window_minimized && inpt->window_restored) {
				logic_ticks = now_ticks;
				inpt->window_minimized = inpt->window_restored = false;
				break;
			}

			// don't skip frames if the game is paused
			if (gswitch->isPaused()) {
				logic_ticks = now_ticks;
				break;
			}
		}

		render_device->blankScreen();
		gswitch->render();

		// display the FPS counter
		// if the frame completed quickly, we estimate the delay here
		now_ticks = SDL_GetTicks();
		int delay_ticks = 0;
		if (now_ticks - prev_ticks < delay) {
			delay_ticks = delay - (now_ticks - prev_ticks);
		}
		if (now_ticks+delay_ticks - prev_ticks != 0) {
			gswitch->showFPS(1000 / (now_ticks+delay_ticks - prev_ticks));
		}

		render_device->commitFrame();

		// delay quick frames
		now_ticks = SDL_GetTicks();
		if (now_ticks - prev_ticks < delay) {
			SDL_Delay(delay - (now_ticks - prev_ticks));
		}

	}
}

static void cleanup() {
	delete gswitch;

	delete anim;
	delete comb;
	delete font;
	delete inpt;
	delete mods;
	delete msg;
	delete snd;
	delete curs;

	Mix_CloseAudio();

	if (render_device)
		render_device->destroyContext();
	delete render_device;

	SDL_Quit();
}

std::string parseArg(const std::string &arg) {
	std::string result = "";

	// arguments must start with '--'
	if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
		for (unsigned i = 2; i < arg.length(); ++i) {
			if (arg[i] == '=') break;
			result += arg[i];
		}
	}

	return result;
}

std::string parseArgValue(const std::string &arg) {
	std::string result = "";
	bool found_equals = false;

	for (unsigned i = 0; i < arg.length(); ++i) {
		if (found_equals) {
			result += arg[i];
		}
		if (arg[i] == '=') found_equals = true;
	}

	return result;
}

int main(int argc, char *argv[]) {
	bool debug_event = false;
	bool done = false;
	std::string render_device_name = "";

	for (int i = 1 ; i < argc; i++) {
		std::string arg = std::string(argv[i]);
		if (parseArg(arg) == "debug-event") {
			debug_event = true;
		}
		else if (parseArg(arg) == "data-path") {
			CUSTOM_PATH_DATA = parseArgValue(arg);
			if (!CUSTOM_PATH_DATA.empty() && CUSTOM_PATH_DATA.at(CUSTOM_PATH_DATA.length()-1) != '/')
				CUSTOM_PATH_DATA += "/";
		}
		else if (parseArg(arg) == "version") {
			printf("%s\n", getVersionString().c_str());
			done = true;
		}
		else if (parseArg(arg) == "renderer") {
			render_device_name = parseArgValue(arg);
		}
		else if (parseArg(arg) == "help") {
			printf("\
--help           Prints this message.\n\n\
--version        Prints the release version.\n\n\
--data-path      Specifies an exact path to look for mod data.\n\n\
--debug-event    Prints verbose hardware input information.\n\n\
--renderer       Specifies the rendering backend to use. The default is 'sdl'.\n");
			done = true;
		}
	}

	if (!done) {
		srand((unsigned int)time(NULL));
		init(render_device_name);
		mainLoop(debug_event);
		cleanup();
	}

	return 0;
}
