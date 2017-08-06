/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <limits.h>

#include "Settings.h"
#include "Stats.h"
#include "GameSwitcher.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "SDLFontEngine.h"
#include "UtilsParsing.h"

GameSwitcher *gswitch;

#define PLATFORM_CPP_INCLUDE

#ifdef _WIN32
#include "PlatformWin32.cpp"
#elif __ANDROID__
#include "PlatformAndroid.cpp"
#elif __IPHONEOS__
#include "PlatformIPhoneOS.cpp"
#elif __GCW0__
#include "PlatformGCW0.cpp"
#else
// Linux stuff should work on Mac OSX/BSD/etc, too
#include "PlatformLinux.cpp"
#endif

class CmdLineArgs {
public:
	std::string render_device_name;
	std::vector<std::string> mod_list;
};

/**
 * Game initialization.
 */
static void init(const CmdLineArgs& cmd_line_args) {
	PlatformInit(&PlatformOptions);

	/**
	 * Set system paths
	 * PATH_CONF is for user-configurable settings files (e.g. keybindings)
	 * PATH_USER is for user-specific data (e.g. save games)
	 * PATH_DATA is for common game data (e.g. images, music)
	 */
	PlatformSetPaths();

	// SDL Inits
	if ( SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0 ) {
		logError("main: Could not initialize SDL: %s", SDL_GetError());
		logErrorDialog("ERROR: Could not initialize SDL.");
		Exit(1);
	}

	// Shared Resources set-up

	mods = new ModManager(&(cmd_line_args.mod_list));

	if (!mods->haveFallbackMod()) {
		logError("main: Could not find the default mod in the following locations:");
		if (dirExists(PATH_DATA + "mods")) logError("%smods/", PATH_DATA.c_str());
		if (dirExists(PATH_USER + "mods")) logError("%smods/", PATH_USER.c_str());
		logError("A copy of the default mod is in the \"mods\" directory of the flare-engine repo.");
		logError("The repo is located at: https://github.com/clintbellanger/flare-engine");
		logError("Try again after copying the default mod to one of the above directories. Exiting.");
		logErrorDialog("ERROR: No \"default\" mod found.");
		Exit(1);
	}

	if (!loadSettings()) {
		logError("main: Could not load settings file: '%s'.", (PATH_CONF + FILE_SETTINGS).c_str());
		logErrorDialog("ERROR: Could not load settings file.");
		Exit(1);
	}

	save_load = new SaveLoad();
	msg = new MessageEngine();
	font = getFontEngine();
	anim = new AnimationManager();
	comb = new CombatText();
	inpt = getInputManager();
	icons = NULL;

	// Load tileset options (must be after ModManager is initialized)
	loadTilesetSettings();

	// Load miscellaneous settings
	loadMiscSettings();
	setStatNames();

	// Create render Device and Rendering Context.
	if (PlatformOptions.default_renderer != "")
		render_device = getRenderDevice(PlatformOptions.default_renderer);
	else if (cmd_line_args.render_device_name != "")
		render_device = getRenderDevice(cmd_line_args.render_device_name);
	else
		render_device = getRenderDevice(RENDER_DEVICE);

	int status = render_device->createContext();

	if (status == -1) {
		logError("main: Could not create rendering context: %s", SDL_GetError());
		logErrorDialog("ERROR: Could not create rendering context");
		Exit(1);
	}

	snd = getSoundManager();

	inpt->initJoystick();

	gswitch = new GameSwitcher();
}

static float getSecondsElapsed(uint64_t prev_ticks, uint64_t now_ticks) {
	return (static_cast<float>(now_ticks - prev_ticks) / static_cast<float>(SDL_GetPerformanceFrequency()));
}

static void mainLoop () {
	bool done = false;

	float seconds_per_frame = 1.f/static_cast<float>(MAX_FRAMES_PER_SEC);

	uint64_t prev_ticks = SDL_GetPerformanceCounter();
	uint64_t logic_ticks = SDL_GetPerformanceCounter();

	float last_fps = -1;

	while ( !done ) {
		int loops = 0;
		uint64_t now_ticks = SDL_GetPerformanceCounter();

		while (now_ticks >= logic_ticks && loops < MAX_FRAMES_PER_SEC) {
			// Frames where data loading happens (GameState switching and map loading)
			// take a long time, so our loop here will think that the game "lagged" and
			// try to compensate. To prevent this compensation, we mark those frames as
			// "loading frames" and update the logic ticker without actually executing logic.
			if (gswitch->isLoadingFrame()) {
				logic_ticks = now_ticks;
				break;
			}

			SDL_PumpEvents();
			inpt->handle();

			// Skip game logic when minimized on Mobile device
			if (inpt->window_minimized && !inpt->window_restored)
				break;

			gswitch->logic();
			inpt->resetScroll();

			// Engine done means the user escapes the main game menu.
			// Input done means the user closes the window.
			done = gswitch->done || inpt->done;

			logic_ticks += static_cast<uint64_t>(seconds_per_frame * static_cast<float>(SDL_GetPerformanceFrequency()));
			loops++;

			// Android and IOS only
			// When the app is minimized on Mobile device, no logic gets processed.
			// As a result, the delta time when restoring the app is large, so the game will skip frames and appear to be running fast.
			// To counter this, we reset our delta time here when restoring the app
			if (inpt->window_minimized && inpt->window_restored) {
				logic_ticks = now_ticks = SDL_GetPerformanceCounter();
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
		if (last_fps != -1) {
		    gswitch->showFPS(last_fps);
		}

		render_device->commitFrame();

		// calculate the FPS
		// if the frame completed quickly, we estimate the delay here
		float fps_delay;
		if (getSecondsElapsed(prev_ticks, SDL_GetPerformanceCounter()) < seconds_per_frame) {
			fps_delay = seconds_per_frame;
		} else {
			fps_delay = getSecondsElapsed(prev_ticks, SDL_GetPerformanceCounter());
		}
		if (fps_delay != 0) {
			last_fps = (1000.f / fps_delay) / 1000.f;
		} else {
			last_fps = -1;
		}

		// delay quick frames
		// thanks to David Gow: https://davidgow.net/handmadepenguin/ch18.html
		if (getSecondsElapsed(prev_ticks, SDL_GetPerformanceCounter()) < seconds_per_frame) {
			int32_t delay_ms = static_cast<int32_t>((seconds_per_frame - getSecondsElapsed(prev_ticks, SDL_GetPerformanceCounter())) * 1000.f) - 1;
			if (delay_ms > 0) {
				SDL_Delay(delay_ms);
			}
			while (getSecondsElapsed(prev_ticks, SDL_GetPerformanceCounter()) < seconds_per_frame) {
				// Waiting...
			}
		}
		prev_ticks = SDL_GetPerformanceCounter();
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
	delete save_load;

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
	CmdLineArgs cmd_line_args;

	for (int i = 1 ; i < argc; i++) {
		std::string arg_full = std::string(argv[i]);
		std::string arg = parseArg(arg_full);
		if (arg == "debug-event") {
			debug_event = true;
		}
		else if (arg == "data-path") {
			CUSTOM_PATH_DATA = parseArgValue(arg_full);
			if (!CUSTOM_PATH_DATA.empty() && CUSTOM_PATH_DATA.at(CUSTOM_PATH_DATA.length()-1) != '/')
				CUSTOM_PATH_DATA += "/";
		}
		else if (arg == "version") {
			printf("%s\n", getVersionString().c_str());
			done = true;
		}
		else if (arg == "renderer") {
			cmd_line_args.render_device_name = parseArgValue(arg_full);
		}
		else if (arg == "no-audio") {
			AUDIO = false;
		}
		else if (arg == "mods") {
			std::string mod_list_str = parseArgValue(arg_full);
			while (!mod_list_str.empty()) {
				cmd_line_args.mod_list.push_back(popFirstString(mod_list_str));
			}
		}
		else if (arg == "load-slot") {
			LOAD_SLOT = parseArgValue(arg_full);
		}
		else if (arg == "load-script") {
			LOAD_SCRIPT = parseArgValue(arg_full);
		}
		else if (arg == "help") {
			printf("\
--help                   Prints this message.\n\
--version                Prints the release version.\n\
--data-path=<PATH>       Specifies an exact path to look for mod data.\n\
--debug-event            Prints verbose hardware input information.\n\
--renderer=<RENDERER>    Specifies the rendering backend to use.\n\
                         The default is 'sdl'.\n\
--no-audio               Disables sound effects and music.\n\
--mods=<MOD>,...         Starts the game with only these mods enabled.\n\
--load-slot=<SLOT>       Loads a save slot by numerical index.\n\
--load-script=<SCRIPT>   Execute's a script upon loading a saved game.\n\
                         The script path is mod-relative.\n");
			done = true;
		}
		else {
			logError("'%s' is not a valid command line option. Try '--help' for a list of valid options.", argv[i]);
		}
	}

	if (!done) {
		srand(static_cast<unsigned int>(time(NULL)));
		init(cmd_line_args);

		if (debug_event)
			inpt->enableEventLog();

		mainLoop();

		if (gswitch)
			gswitch->saveUserSettings();

		cleanup();
	}

	return 0;
}
