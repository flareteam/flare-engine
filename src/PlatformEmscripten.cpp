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
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

#include <SDL.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <emscripten.h>

Platform PLATFORM;

Platform::Platform()
	: has_exit_button(false)
	, is_mobile_device(false)
	, force_hardware_cursor(false)
	, has_lock_file(false)
	, config_menu_type(CONFIG_MENU_TYPE_DESKTOP_NO_VIDEO)
	, default_renderer("sdl_hardware") {
}

Platform::~Platform() {
}

void Platform::setPaths() {
	settings->path_conf = "/flare_data/config/";
	createDir(settings->path_conf);

	settings->path_user = "/flare_data/userdata/";
	createDir(settings->path_user);
	createDir(settings->path_user + "mods/");
	createDir(settings->path_user + "saves/");

	// data folder

	// these flags are set to true when a valid directory is found
	bool path_data = false;

	// Check for the local data before trying installed ones.
	if (pathExists("./mods")) {
		if (!path_data) settings->path_data = "./";
		path_data = true;
	}

	// finally assume the local folder
	if (!path_data)	settings->path_data = "./";
}

void Platform::setExitEventFilter() {
}

bool Platform::dirCreate(const std::string& path) {
	if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		std::string error_msg = "createDir (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

bool Platform::dirRemove(const std::string& path) {
	if (rmdir(path.c_str()) == -1) {
		std::string error_msg = "removeDir (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

void Platform::FSInit() {
    EM_ASM(
        FS.mkdir('/flare_data');
        FS.mount(IDBFS,{},'/flare_data');

        Module.print("start file sync..");
        Module.syncdone = 0;

        FS.syncfs(true, function(err) {
                       assert(!err);
                       Module.print("end file sync..");
                       Module.syncdone = 1;
        });
    );
}

bool Platform::FSCheckReady() {
    if(emscripten_run_script_int("Module.syncdone") == 1) {
        FILE *config_file = fopen(std::string(settings->path_conf + "settings.txt").c_str(),"r");
        if (config_file == NULL) {
            //persist Emscripten current data to Indexed Db
            EM_ASM(
                Module.print("Start File sync..");
                Module.syncdone = 0;
                FS.syncfs(false, function(err) {
                    assert(!err);
                    Module.print("End File sync..");
                    Module.syncdone = 1;
                });
            );
			settings->saveSettings();
			return false;
        }
        else {
            fclose(config_file);
			return true;
        }
    }
	return false;
}

void Platform::FSCommit() {
    EM_ASM(
        //persist changes
        FS.syncfs(false,function (err) {
                          assert(!err);
        });
    );
}

void Platform::setScreenSize() {
	// can't change window size dynamically with Emscripten, so default to 16:9 aspect ratio
	settings->screen_w = 854;
	settings->screen_h = 480;
}

#endif // PLATFORM_CPP
#endif // PLATFORM_CPP_INCLUDE
