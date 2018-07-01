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

Platform PLATFORM;

Platform::Platform()
	: has_exit_button(true)
	, is_mobile_device(false)
	, force_hardware_cursor(false)
	, has_lock_file(true)
	, config_menu_type(CONFIG_MENU_TYPE_DESKTOP)
	, default_renderer("") {
}

Platform::~Platform() {
}

void Platform::setPaths() {
	// handle Windows-specific path options
	if (getenv("APPDATA") != NULL) {
		settings->path_conf = settings->path_user = (std::string)getenv("APPDATA") + "\\flare";
		createDir(settings->path_conf);
		createDir(settings->path_user);

		settings->path_conf += "\\config";
		settings->path_user += "\\userdata";
		createDir(settings->path_conf);
		createDir(settings->path_user);
	}
	else {
		settings->path_conf = "config";
		settings->path_user = "userdata";
		createDir(settings->path_conf);
		createDir(settings->path_user);
	}

	createDir(settings->path_user + "\\mods");
	createDir(settings->path_user + "\\saves");

	settings->path_data = "";
	if (pathExists(settings->custom_path_data)) settings->path_data = settings->custom_path_data;
	else if (!settings->custom_path_data.empty()) {
		logError("Settings: Could not find specified game data directory.");
		settings->custom_path_data = "";
	}

	settings->path_conf = settings->path_conf + "/";
	settings->path_user = settings->path_user + "/";
}

bool Platform::dirCreate(const std::string& path) {
	if (_mkdir(path.c_str()) != 0) {
		std::string error_msg = "createDir (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

bool Platform::dirRemove(const std::string& path) {
	if (_rmdir(path.c_str()) != 0) {
		std::string error_msg = "removeDir (" + path + ")";
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
void Platform::setExitEventFilter() {}


#endif // PLATFORM_CPP
#endif // PLATFORM_CPP_INCLUDE
