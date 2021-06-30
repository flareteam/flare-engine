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

#include <SDL.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <jni.h>

Platform platform;

namespace PlatformAndroid {
	std::string getPackageName();
	int isExitEvent(void *userdata, SDL_Event* event);
};

std::string PlatformAndroid::getPackageName()
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

int PlatformAndroid::isExitEvent(void* userdata, SDL_Event* event) {
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
	: has_exit_button(true)
	, is_mobile_device(true)
	, force_hardware_cursor(true)
	, has_lock_file(false)
	, config_menu_type(CONFIG_MENU_TYPE_BASE)
	, default_renderer("")
	, config_video(Platform::Video::COUNT, true)
	, config_audio(Platform::Audio::COUNT, true)
	, config_interface(Platform::Interface::COUNT, true)
	, config_input(Platform::Input::COUNT, true)
	, config_misc(Platform::Misc::COUNT, true)
{
	config_video[Platform::Video::RENDERER] = true;
	config_video[Platform::Video::FULLSCREEN] = false;
	config_video[Platform::Video::HWSURFACE] = false;
	config_video[Platform::Video::VSYNC] = false;
	config_video[Platform::Video::TEXTURE_FILTER] = false;
	config_video[Platform::Video::ENABLE_GAMMA] = false;
	config_video[Platform::Video::GAMMA] = false;

	config_interface[Platform::Interface::HARDWARE_CURSOR] = false;

	config_input[Platform::Input::JOYSTICK] = false;
	config_input[Platform::Input::MOUSE_MOVE] = false;
	config_input[Platform::Input::MOUSE_AIM] = false;
	config_input[Platform::Input::NO_MOUSE] = false;
	config_input[Platform::Input::MOUSE_MOVE_SWAP] = false;
	config_input[Platform::Input::MOUSE_MOVE_ATTACK] = false;
	config_input[Platform::Input::JOYSTICK_DEADZONE] = false;
	config_input[Platform::Input::TOUCH_CONTROLS] = false;

	config_misc[Platform::Misc::KEYBINDS] = false;
}

Platform::~Platform() {
}

void Platform::setPaths() {
	/*
	 * settings->path_conf
	 * 1. INTERNAL_SD_CARD/Flare
	 * 2. EXTERNAL_SD_CARD/Flare
	 * 3. App internal storage (/data/...)
	 *
	 * settings->path_data
	 * 1. App external storage (usually internal sd card)
	 * 2. INTERNAL_SD_CARD/Flare
	 *
	 * settings->path_user
	 * 1. INTERNAL_SD_CARD/Flare
	 * 2. EXTERNAL_SD_CARD/Flare
	 */
	std::vector<std::string> internalSDList;
	internalSDList.push_back("/sdcard");
	internalSDList.push_back("/mnt/sdcard");
	internalSDList.push_back("/storage/sdcard0");
	internalSDList.push_back("/storage/emulated/0");
	internalSDList.push_back("/storage/emulated/legacy");
	internalSDList.push_back("/mnt/m_internal_storage");

	std::vector<std::string> externalSDList;
	externalSDList.push_back("/mnt/extSdCard");
	externalSDList.push_back("/storage/extSdCard");
	externalSDList.push_back("/mnt/m_external_sd");

	settings->path_conf = std::string(SDL_AndroidGetInternalStoragePath()) + "/config";

	const std::string package_name = PlatformAndroid::getPackageName();
	const std::string user_folder = "Android/data/" + package_name + "/files";

	if (SDL_AndroidGetExternalStorageState() != 0) {
		settings->path_data = std::string(SDL_AndroidGetExternalStoragePath());
	}

	for (int i = 0; i < internalSDList.size(); i++) {
		if (Filesystem::pathExists(internalSDList[i])) {
			settings->path_user = internalSDList[i] + "/Flare";
			settings->path_conf = settings->path_user + "/config";

			if (settings->path_data.empty())
				settings->path_data = internalSDList[i] + "/" + user_folder;

			break;
		}
	}

	if (settings->path_data.empty()) {
		Utils::logError("Platform: Android external storage unavailable: %s", SDL_GetError());
	}

	if (settings->path_user.empty() || !Filesystem::pathExists(settings->path_user)) {
		for (int i = 0; i < externalSDList.size(); i++) {
			if (Filesystem::pathExists(externalSDList[i])) {
				settings->path_user = externalSDList[i] + "/Flare";
				settings->path_conf = settings->path_user + "/config";

				break;
			}
		}
	}

	Filesystem::createDir(settings->path_user);
	Filesystem::createDir(settings->path_conf);
	Filesystem::createDir(settings->path_user + "/mods");
	Filesystem::createDir(settings->path_user + "/saves");

	settings->path_conf += "/";
	settings->path_user += "/";
	settings->path_data += "/";

	// create a .nomedia file to prevent game data being added to the Android media library
	std::ofstream nomedia;
	nomedia.open(settings->path_user + ".nomedia", std::ios::out);
	if (nomedia.bad()) {
		Utils::logError("Platform: Unable to create Android .nomedia file.");
	}
	nomedia.close();
	nomedia.clear();
}

void Platform::setExitEventFilter() {
	SDL_SetEventFilter(PlatformAndroid::isExitEvent, NULL);
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
