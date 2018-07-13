/*
Copyright © 2016 Justin Jacobs

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

#ifdef PLATFORM_CPP_INCLUDE
#ifndef PLATFORM_CPP
#define PLATFORM_CPP

#include "Platform.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"

#include <SDL.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

Platform platform;

namespace PlatformIPhoneOS {
	int isExitEvent(void* userdata, SDL_Event* event);
};

int PlatformIPhoneOS::isExitEvent(void* userdata, SDL_Event* event) {
	if (userdata) {}; // avoid unused var compile warning

	if (event->type == SDL_APP_TERMINATING) {
		Utils::logInfo("Terminating app, saving...");
		save_load->saveGame();
		Utils::logInfo("Saved, ready to exit.");
		return 0;
	}
	return 1;
}

Platform::Platform()
	: has_exit_button(false)
	, is_mobile_device(true)
	, force_hardware_cursor(true)
	, has_lock_file(false)
	, config_menu_type(CONFIG_MENU_TYPE_BASE)
	, default_renderer("sdl_hardware") {
}

Platform::~Platform() {
}

// NOTE This was copied from the Linux platform
// That probably isn't right, but its what the codepath was before in Settings.cpp
void Platform::setPaths() {

	// attempting to follow this spec:
	// http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

	// set config path (settings, keybindings)
	// $XDG_CONFIG_HOME/flare/
	if (getenv("XDG_CONFIG_HOME") != NULL) {
		settings->path_conf = (std::string)getenv("XDG_CONFIG_HOME") + "/flare/";
	}
	// $HOME/.config/flare/
	else if (getenv("HOME") != NULL) {
		settings->path_conf = (std::string)getenv("HOME") + "/.config/";
		Filesystem::createDir(settings->path_conf);
		settings->path_conf += "flare/";
	}
	// ./config/
	else {
		settings->path_conf = "./config/";
	}

	Filesystem::createDir(settings->path_conf);

	// set user path (save games)
	// $XDG_DATA_HOME/flare/
	if (getenv("XDG_DATA_HOME") != NULL) {
		settings->path_user = (std::string)getenv("XDG_DATA_HOME") + "/flare/";
	}
	// $HOME/.local/share/flare/
	else if (getenv("HOME") != NULL) {
		settings->path_user = (std::string)getenv("HOME") + "/.local/";
		Filesystem::createDir(settings->path_user);
		settings->path_user += "share/";
		Filesystem::createDir(settings->path_user);
		settings->path_user += "flare/";
	}
	// ./saves/
	else {
		settings->path_user = "./userdata/";
	}

	Filesystem::createDir(settings->path_user);
	Filesystem::createDir(settings->path_user + "mods/");
	Filesystem::createDir(settings->path_user + "saves/");

	// data folder
	// while settings->path_conf and settings->path_user are created if not found,
	// settings->path_data must already have the game data for the game to work.
	// in most releases the data will be in the same folder as the executable
	// - Windows apps are released as a simple folder
	// - OSX apps are released in a .app folder
	// Official linux distros might put the executable and data files
	// in a more standard location.

	// these flags are set to true when a valid directory is found
	bool path_data = false;

	// if the user specified a data path, try to use it
	if (Filesystem::pathExists(settings->custom_path_data)) {
		if (!path_data) settings->path_data = settings->custom_path_data;
		path_data = true;
	}
	else if (!settings->custom_path_data.empty()) {
		Utils::logError("Settings: Could not find specified game data directory.");
		settings->custom_path_data = "";
	}

	// Check for the local data before trying installed ones.
	if (Filesystem::pathExists("./mods")) {
		if (!path_data) settings->path_data = "./";
		path_data = true;
	}

	// check $XDG_DATA_DIRS options
	// a list of directories in preferred order separated by :
	if (getenv("XDG_DATA_DIRS") != NULL) {
		std::string pathlist = (std::string)getenv("XDG_DATA_DIRS");
		std::string pathtest;
		pathtest = Parse::popFirstString(pathlist,':');
		while (pathtest != "") {
			if (!path_data) {
				settings->path_data = pathtest + "/flare/";
				if (Filesystem::pathExists(settings->path_data)) path_data = true;
			}
			if (path_data) break;
			pathtest = Parse::popFirstString(pathlist,':');
		}
	}

#if defined DATA_INSTALL_DIR
	if (!path_data) settings->path_data = DATA_INSTALL_DIR "/";
	if (!path_data && Filesystem::pathExists(settings->path_data)) path_data = true;
#endif

	// check /usr/local/share/flare/ and /usr/share/flare/ next
	if (!path_data) settings->path_data = "/usr/local/share/flare/";
	if (!path_data && Filesystem::pathExists(settings->path_data)) path_data = true;

	if (!path_data) settings->path_data = "/usr/share/flare/";
	if (!path_data && Filesystem::pathExists(settings->path_data)) path_data = true;

	// check "games" variants of these
	if (!path_data) settings->path_data = "/usr/local/share/games/flare/";
	if (!path_data && Filesystem::pathExists(settings->path_data)) path_data = true;

	if (!path_data) settings->path_data = "/usr/share/games/flare/";
	if (!path_data && Filesystem::pathExists(settings->path_data)) path_data = true;

	// finally assume the local folder
	if (!path_data)	settings->path_data = "./";
}

void Platform::setExitEventFilter() {
	SDL_SetEventFilter(PlatformIPhoneOS::isExitEvent, NULL);
}

bool Platform::dirCreate(const std::string& path) {
	if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		std::string error_msg = "Platform::dirCreate (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

bool Platform::dirRemove(const std::string& path) {
	if (rmdir(path.c_str()) == -1) {
		std::string error_msg = "Platform::dirRemove (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

// unused
void Platform::FSInit() {}
bool Platform::FSCheckReady() { return true; }
void Platform::FSCommit() {}
void Platform::setScreenSize() {}

#endif // PLATFORM_CPP
#endif // PLATFORM_CPP_INCLUDE
