/*
Copyright © 2011-2012 Clint Bellanger
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>

using namespace std;

#include "Settings.h"
#include "Stats.h"
#include "GameSwitcher.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "SDLRenderDevice.h"

GameSwitcher *gswitch;
SDL_Surface *titlebar_icon;

/**
 * Game initialization.
 */
static void init() {

	setPaths();
	setStatNames();

	// SDL Inits
	if ( SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0 ) {
		fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	// Shared Resources set-up

	mods = new ModManager();

	if (find(mods->mod_list.begin(), mods->mod_list.end(), FALLBACK_MOD) == mods->mod_list.end()) {
		fprintf(stderr, "Could not find the default mod in the following locations:\n");
		if (dirExists(PATH_DATA + "mods")) fprintf(stderr, "%smods/\n", PATH_DATA.c_str());
		if (dirExists(PATH_DEFAULT_DATA + "mods")) fprintf(stderr, "%smods/\n", PATH_DEFAULT_DATA.c_str());
		if (dirExists(PATH_USER + "mods")) fprintf(stderr, "%smods/\n", PATH_USER.c_str());
		if (dirExists(PATH_DEFAULT_USER + "mods")) fprintf(stderr, "%smods/\n", PATH_DEFAULT_USER.c_str());
		fprintf(stderr, "\nA copy of the default mod is in the \"mods\" directory of the flare-engine repo.\n");
		fprintf(stderr, "The repo is located at: https://github.com/clintbellanger/flare-engine\n");
		fprintf(stderr, "Try again after copying the default mod to one of the above directories.\nExiting.\n");
		exit(1);
	}

	if (!loadSettings()) {
		fprintf(stderr, "%s",
				("Could not load settings file: ‘" + PATH_CONF + FILE_SETTINGS + "’.\n").c_str());
		exit(1);
	}

	msg = new MessageEngine();
	font = new FontEngine();
	anim = new AnimationManager();
	comb = new CombatText();
	imag = new ImageManager();
	inpt = new InputState();

	// Load tileset options (must be after ModManager is initialized)
	loadTilesetSettings();

	// Load miscellaneous settings
	loadMiscSettings();

	// Add Window Titlebar Icon
	titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());
	SDL_WM_SetIcon(titlebar_icon, NULL);

	// Create render Device and Rendering Context.
	render_device = new SDLRenderDevice();
	int status = render_device->createContext(VIEW_W, VIEW_H);

	if (status == -1) {

		fprintf (stderr, "Error during SDL_SetVideoMode: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	loadIcons();
	// Set Gamma
	if (CHANGE_GAMMA)
		SDL_SetGamma(GAMMA,GAMMA,GAMMA);

	if (AUDIO && Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024)) {
		fprintf (stderr, "Error during Mix_OpenAudio: %s\n", SDL_GetError());
		AUDIO = false;
	}

	snd = new SoundManager();

	// initialize Joysticks
	if(SDL_NumJoysticks() == 1) {
		printf("1 joystick was found:\n");
	}
	else if(SDL_NumJoysticks() > 1) {
		printf("%d joysticks were found:\n", SDL_NumJoysticks());
	}
	else {
		printf("No joysticks were found.\n");
	}
	for(int i = 0; i < SDL_NumJoysticks(); i++) {
		printf("  Joy %d) %s\n", i, SDL_JoystickName(i));
	}
	if ((ENABLE_JOYSTICK) && (SDL_NumJoysticks() > 0)) {
		joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
		printf("Using joystick #%d.\n", JOYSTICK_DEVICE);
	}

	// Set sound effects volume from settings file
	if (AUDIO)
		Mix_Volume(-1, SOUND_VOLUME);

	// Window title
	const char* title = msg->get(WINDOW_TITLE).c_str();
	SDL_WM_SetCaption(title, title);


	gswitch = new GameSwitcher();
}

static void mainLoop (bool debug_event) {

	bool done = false;
	int delay = 1000/MAX_FRAMES_PER_SEC;
	int prevTicks = SDL_GetTicks();

	while ( !done ) {

		SDL_PumpEvents();
		inpt->handle(debug_event);
		gswitch->logic();

		// black out
		render_device->blankScreen();

		gswitch->render();

		// Engine done means the user escapes the main game menu.
		// Input done means the user closes the window.
		done = gswitch->done || inpt->done;

		int nowTicks = SDL_GetTicks();
		if (nowTicks - prevTicks < delay) SDL_Delay(delay - (nowTicks - prevTicks));
		gswitch->showFPS(1000 / (SDL_GetTicks() - prevTicks));
		prevTicks = SDL_GetTicks();

		render_device->commitFrame();
	}
}

static void cleanup() {
	delete gswitch;

	delete anim;
	delete comb;
	delete font;
	delete imag;
	delete inpt;
	delete mods;
	delete msg;
	delete snd;

	SDL_FreeSurface(titlebar_icon);

	Mix_CloseAudio();

	render_device->destroyContext();
	delete render_device;

	SDL_Quit();
}

string parseArg(const string &arg) {
	string result = "";

	// arguments must start with '--'
	if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
		for (unsigned i = 2; i < arg.length(); ++i) {
			if (arg[i] == '=') break;
			result += arg[i];
		}
	}

	return result;
}

string parseArgValue(const string &arg) {
	string result = "";
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
	bool game_warning = true;

	for (int i = 1 ; i < argc; i++) {
		string arg = string(argv[i]);
		if (parseArg(arg) == "debug-event") {
			debug_event = true;
		}
		else if (parseArg(arg) == "game") {
			GAME_FOLDER = parseArgValue(arg);
			game_warning = false;
		}
		else if (parseArg(arg) == "data-path") {
			CUSTOM_PATH_DATA = parseArgValue(arg);
			if (!CUSTOM_PATH_DATA.empty() && CUSTOM_PATH_DATA.at(CUSTOM_PATH_DATA.length()-1) != '/')
				CUSTOM_PATH_DATA += "/";
		}
		else if (parseArg(arg) == "version") {
			printf("%s\n", RELEASE_VERSION.c_str());
			done = true;
		}
		else if (parseArg(arg) == "help") {
			printf("\
--help           Prints this message.\n\n\
--version        Prints the release version.\n\n\
--game           Specifies which 'game' to use when launching. A game\n\
                 determines which parent folder to look for mods in, as well\n\
                 as where user settings and save data are stored.\n\n\
--data-path      Specifies an exact path to look for mod data.\n\n\
--debug-event    Prints verbose hardware input information.\n");
			done = true;
		}
	}

	if (!done) {
		// if a game isn't specified, display a warning
		if (game_warning) {
			printf("Warning: A game wasn't specified, falling back to the 'default' game.\nDid you forget the --game flag? (e.g. --game=flare-game).\nSee --help for more details.\n\n");
		}

		srand((unsigned int)time(NULL));
		init();
		mainLoop(debug_event);
	}
	cleanup();

	return 0;
}
