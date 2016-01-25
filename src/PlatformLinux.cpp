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

#include "Platform.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

#include <SDL.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

PlatformOptions_t PlatformOptions = {};

void PlatformInit(struct PlatformOptions_t *options) {
	options->has_exit_button = true;
	options->is_mobile_device = false;
	options->config_menu_type = CONFIG_MENU_TYPE_DESKTOP;
	options->default_renderer="";
}

void PlatformSetPaths() {

	// attempting to follow this spec:
	// http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

	// set config path (settings, keybindings)
	// $XDG_CONFIG_HOME/flare/
	if (getenv("XDG_CONFIG_HOME") != NULL) {
		PATH_CONF = (std::string)getenv("XDG_CONFIG_HOME") + "/flare/";
	}
	// $HOME/.config/flare/
	else if (getenv("HOME") != NULL) {
		PATH_CONF = (std::string)getenv("HOME") + "/.config/";
		createDir(PATH_CONF);
		PATH_CONF += "flare/";
	}
	// ./config/
	else {
		PATH_CONF = "./config/";
	}

	createDir(PATH_CONF);

	// set user path (save games)
	// $XDG_DATA_HOME/flare/
	if (getenv("XDG_DATA_HOME") != NULL) {
		PATH_USER = (std::string)getenv("XDG_DATA_HOME") + "/flare/";
	}
	// $HOME/.local/share/flare/
	else if (getenv("HOME") != NULL) {
		PATH_USER = (std::string)getenv("HOME") + "/.local/";
		createDir(PATH_USER);
		PATH_USER += "share/";
		createDir(PATH_USER);
		PATH_USER += "flare/";
	}
	// ./saves/
	else {
		PATH_USER = "./userdata/";
	}

	createDir(PATH_USER);
	createDir(PATH_USER + "mods/");
	createDir(PATH_USER + "saves/");

	// data folder
	// while PATH_CONF and PATH_USER are created if not found,
	// PATH_DATA must already have the game data for the game to work.
	// in most releases the data will be in the same folder as the executable
	// - Windows apps are released as a simple folder
	// - OSX apps are released in a .app folder
	// Official linux distros might put the executable and data files
	// in a more standard location.

	// these flags are set to true when a valid directory is found
	bool path_data = false;

	// if the user specified a data path, try to use it
	if (dirExists(CUSTOM_PATH_DATA)) {
		if (!path_data) PATH_DATA = CUSTOM_PATH_DATA;
		path_data = true;
	}
	else if (!CUSTOM_PATH_DATA.empty()) {
		logError("Settings: Could not find specified game data directory.");
		CUSTOM_PATH_DATA = "";
	}

	// Check for the local data before trying installed ones.
	if (dirExists("./mods")) {
		if (!path_data) PATH_DATA = "./";
		path_data = true;
	}

	// check $XDG_DATA_DIRS options
	// a list of directories in preferred order separated by :
	if (getenv("XDG_DATA_DIRS") != NULL) {
		std::string pathlist = (std::string)getenv("XDG_DATA_DIRS");
		std::string pathtest;
		pathtest = popFirstString(pathlist,':');
		while (pathtest != "") {
			if (!path_data) {
				PATH_DATA = pathtest + "/flare/";
				if (dirExists(PATH_DATA)) path_data = true;
			}
			if (path_data) break;
			pathtest = popFirstString(pathlist,':');
		}
	}

#if defined DATA_INSTALL_DIR
	if (!path_data) PATH_DATA = DATA_INSTALL_DIR "/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;
#endif

	// check /usr/local/share/flare/ and /usr/share/flare/ next
	if (!path_data) PATH_DATA = "/usr/local/share/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	if (!path_data) PATH_DATA = "/usr/share/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	// check "games" variants of these
	if (!path_data) PATH_DATA = "/usr/local/share/games/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	if (!path_data) PATH_DATA = "/usr/share/games/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	// finally assume the local folder
	if (!path_data)	PATH_DATA = "./";
}

void PlatformSetExitEventFilter() {
}

bool PlatformDirCreate(const std::string& path) {
	if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		perror("createDir");
		return false;
	}
	return true;
}

bool PlatformDirRemove(const std::string& path) {
	if (rmdir(path.c_str()) == -1) {
		perror("removeDir");
		return false;
	}
	return true;
}
