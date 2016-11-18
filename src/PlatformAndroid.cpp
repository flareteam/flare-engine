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

#ifndef PLATFORM_CPP
#define PLATFORM_CPP

#include "Platform.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsFileSystem.h"

#include <SDL.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <jni.h>

PlatformOptions_t PlatformOptions = {true, true, CONFIG_MENU_TYPE_BASE, "sdl_hardware"};

std::string AndroidGetPackageName()
{
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz(env->GetObjectClass(activity));

	jmethodID method_id = env->GetMethodID(clazz, "getPackageName", "()Ljava/lang/String;");
	jstring packageName = (jstring)env->CallObjectMethod(activity,  method_id);
	const char* name = env->GetStringUTFChars(packageName, NULL);
	std::string result(name);
	env->ReleaseStringUTFChars(packageName, name);

	env->DeleteLocalRef(activity);
	env->DeleteLocalRef(clazz);

	return result;
}

int AndroidIsExitEvent(void* userdata, SDL_Event* event) {
	if (userdata) {}; // avoid unused var compile warning

	if (event->type == SDL_APP_TERMINATING) {
		logInfo("Terminating app, saving...");
		save_load->saveGame();
		logInfo("Saved, ready to exit.");
		return 0;
	}
	return 1;
}

void PlatformInit(struct PlatformOptions_t *options) {
	options->has_exit_button = true;
	options->is_mobile_device = true;
	options->config_menu_type = CONFIG_MENU_TYPE_BASE;
	options->default_renderer="sdl_hardware";
}

void PlatformSetPaths() {
	const std::string externalSDList[] = {
		"/mnt/extSdCard/Android",
		"/storage/extSdCard/Android"
		};
	const int externalSDList_size = 2;

	PATH_CONF = std::string(SDL_AndroidGetInternalStoragePath()) + "/config";

	const std::string package_name = AndroidGetPackageName();
	const std::string user_folder = "data/" + package_name + "/files";

	if (SDL_AndroidGetExternalStorageState() != 0)
	{
		PATH_USER = std::string(SDL_AndroidGetExternalStoragePath());
	}
	// NOTE: Next condition shouldn't be needed, but in theory SDL_AndroidGetExternalStoragePath() can fail.
	else
	{
		const std::string internalSDList[] = {
			"/sdcard/Android",
			"/mnt/sdcard/Android",
			"/storage/sdcard0/Android",
			"/storage/emulated/0/Android",
			"/storage/emulated/legacy/Android",
			};
		const int internalSDList_size = 5;

		for (int i = 0; i < internalSDList_size; i++)
		{
			if (dirExists(internalSDList[i]))
			{
				PATH_USER = internalSDList[i] + "/" + user_folder;
				break;
			}
		}
	}
	if (PATH_USER.empty())
	{
		logError("Settings: Android external storage unavailable: %s", SDL_GetError());
	}

	for (int i = 0; i < externalSDList_size; i++)
	{
		if (dirExists(externalSDList[i]))
		{
			PATH_DATA = externalSDList[i] + "/" + user_folder;
			if (!dirExists(PATH_DATA))
			{
				createDir(externalSDList[i] + "/data" + package_name);
				createDir(externalSDList[i] + "/data" + package_name + "/files");
			}
			break;
		}
	}

	PATH_USER += "/userdata";

	createDir(PATH_CONF);
	createDir(PATH_USER);
	createDir(PATH_USER + "/mods");
	createDir(PATH_USER + "/saves");

	PATH_CONF += "/";
	PATH_USER += "/";
	PATH_DATA += "/";
}

void PlatformSetExitEventFilter() {
	SDL_SetEventFilter(AndroidIsExitEvent, NULL);
}

bool PlatformDirCreate(const std::string& path) {
	if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		std::string error_msg = "createDir (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

bool PlatformDirRemove(const std::string& path) {
	if (rmdir(path.c_str()) == -1) {
		std::string error_msg = "removeDir (" + path + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

#endif
