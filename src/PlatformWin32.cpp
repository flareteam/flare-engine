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
#include "Utils.h"
#include "UtilsFileSystem.h"

#include <stdlib.h>

#include <direct.h>

PlatformOptions platform_options;

void PlatformInit() {
	// defaults
}

void PlatformSetPaths() {
	// handle Windows-specific path options
	if (getenv("APPDATA") != NULL) {
		PATH_CONF = PATH_USER = (std::string)getenv("APPDATA") + "\\flare";
		createDir(PATH_CONF);
		createDir(PATH_USER);

		PATH_CONF += "\\config";
		PATH_USER += "\\userdata";
		createDir(PATH_CONF);
		createDir(PATH_USER);
	}
	else {
		PATH_CONF = "config";
		PATH_USER = "userdata";
		createDir(PATH_CONF);
		createDir(PATH_USER);
	}

	createDir(PATH_USER + "\\mods");
	createDir(PATH_USER + "\\saves");

	PATH_DATA = "";
	if (dirExists(CUSTOM_PATH_DATA)) PATH_DATA = CUSTOM_PATH_DATA;
	else if (!CUSTOM_PATH_DATA.empty()) {
		logError("Settings: Could not find specified game data directory.");
		CUSTOM_PATH_DATA = "";
	}

	PATH_CONF = PATH_CONF + "/";
	PATH_USER = PATH_USER + "/";
}

void PlatformSetExitEventFilter() {
}

bool PlatformDirCreate(const std::string& path) {
	if (_mkdir(path.c_str()) != 0) {
		std::string error_msg = "createDir (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

bool PlatformDirRemove(const std::string& path) {
	if (_rmdir(path.c_str()) != 0) {
		std::string error_msg = "removeDir (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

// unused
void PlatformFSInit() {}
bool PlatformFSCheckReady() { return true; }
void PlatformFSCommit() {}

#endif // PLATFORM_CPP
#endif // PLATFORM_CPP_INCLUDE
