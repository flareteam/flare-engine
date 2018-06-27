/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
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

/**
 * Settings
 */

#include <cstring>
#include <typeinfo>
#include <cmath>
#include <iomanip>

#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "MenuActionBar.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "Platform.h"
#include "Settings.h"
#include "Utils.h"
#include "UtilsParsing.h"
#include "UtilsFileSystem.h"
#include "SharedResources.h"
#include "Version.h"


class ConfigEntry {
public:
	const char * name;
	const std::type_info * type;
	const char * default_val;
	void * storage;
	const char * comment;
};

ConfigEntry config[] = {
	{ "fullscreen",        &typeid(FULLSCREEN),         "0",            &FULLSCREEN,         "fullscreen mode. 1 enable, 0 disable."},
	{ "resolution_w",      &typeid(SCREEN_W),           "640",          &SCREEN_W,           "display resolution. 640x480 minimum."},
	{ "resolution_h",      &typeid(SCREEN_H),           "480",          &SCREEN_H,           NULL},
	{ "music_volume",      &typeid(MUSIC_VOLUME),       "96",           &MUSIC_VOLUME,       "music and sound volume (0 = silent, 128 = max)"},
	{ "sound_volume",      &typeid(SOUND_VOLUME),       "128",          &SOUND_VOLUME,       NULL},
	{ "combat_text",       &typeid(COMBAT_TEXT),        "1",            &COMBAT_TEXT,        "display floating damage text. 1 enable, 0 disable."},
	{ "mouse_move",        &typeid(MOUSE_MOVE),         "0",            &MOUSE_MOVE,         "use mouse to move (experimental). 1 enable, 0 disable."},
	{ "hwsurface",         &typeid(HWSURFACE),          "1",            &HWSURFACE,          "hardware surfaces, v-sync. Try disabling for performance. 1 enable, 0 disable."},
	{ "vsync",             &typeid(VSYNC),              "1",            &VSYNC,              NULL},
	{ "texture_filter",    &typeid(TEXTURE_FILTER),     "1",            &TEXTURE_FILTER,     "texture filter quality. 0 nearest neighbor (worst), 1 linear (best)"},
	{ "dpi_scaling",       &typeid(DPI_SCALING),        "0",            &DPI_SCALING,        "toggle DPI-based render scaling. 1 enable, 0 disable"},
	{ "max_fps",           &typeid(MAX_FRAMES_PER_SEC), "60",           &MAX_FRAMES_PER_SEC, "maximum frames per second. default is 60"},
	{ "renderer",          &typeid(RENDER_DEVICE),      "sdl_hardware", &RENDER_DEVICE,      "default render device. 'sdl' is the default setting"},
	{ "enable_joystick",   &typeid(ENABLE_JOYSTICK),    "0",            &ENABLE_JOYSTICK,    "joystick settings."},
	{ "joystick_device",   &typeid(JOYSTICK_DEVICE),    "0",            &JOYSTICK_DEVICE,    NULL},
	{ "joystick_deadzone", &typeid(JOY_DEADZONE),       "100",          &JOY_DEADZONE,       NULL},
	{ "language",          &typeid(LANGUAGE),           "en",           &LANGUAGE,           "2-letter language code."},
	{ "change_gamma",      &typeid(CHANGE_GAMMA),       "0",            &CHANGE_GAMMA,       "allow changing gamma (experimental). 1 enable, 0 disable."},
	{ "gamma",             &typeid(GAMMA),              "1.0",          &GAMMA,              "screen gamma (0.5 = darkest, 2.0 = lightest)"},
	{ "mouse_aim",         &typeid(MOUSE_AIM),          "1",            &MOUSE_AIM,          "use mouse to aim. 1 enable, 0 disable."},
	{ "no_mouse",          &typeid(NO_MOUSE),           "0",            &NO_MOUSE,           "make using mouse secondary, give full control to keyboard. 1 enable, 0 disable."},
	{ "show_fps",          &typeid(SHOW_FPS),           "0",            &SHOW_FPS,           "show frames per second. 1 enable, 0 disable."},
	{ "colorblind",        &typeid(COLORBLIND),         "0",            &COLORBLIND,         "enable colorblind tooltips. 1 enable, 0 disable"},
	{ "hardware_cursor",   &typeid(HARDWARE_CURSOR),    "0",            &HARDWARE_CURSOR,    "use the system mouse cursor. 1 enable, 0 disable"},
	{ "dev_mode",          &typeid(DEV_MODE),           "0",            &DEV_MODE,           "allow opening the developer console. 1 enable, 0 disable"},
	{ "dev_hud",           &typeid(DEV_HUD),            "1",            &DEV_HUD,            "shows some additional information on-screen when developer mode is enabled. 1 enable, 0 disable"},
	{ "loot_tooltips",     &typeid(LOOT_TOOLTIPS),      "1",            &LOOT_TOOLTIPS,      "always show loot tooltips. 1 enable, 0 disable"},
	{ "statbar_labels",    &typeid(STATBAR_LABELS),     "0",            &STATBAR_LABELS,     "always show labels on HP/MP/XP bars. 1 enable, 0 disable"},
	{ "auto_equip",        &typeid(AUTO_EQUIP),         "1",            &AUTO_EQUIP,         "automatically equip items. 1 enable, 0 disable"},
	{ "subtitles",         &typeid(SUBTITLES),          "0",            &SUBTITLES,          "displays subtitles. 1 enable, 0 disable"},
	{ "prev_save_slot",    &typeid(PREV_SAVE_SLOT),     "-1",           &PREV_SAVE_SLOT,     "index of the last used save slot"}
};
const size_t config_size = sizeof(config) / sizeof(ConfigEntry);

// Paths
std::string PATH_CONF = "";
std::string PATH_USER = "";
std::string PATH_DATA = "";
std::string CUSTOM_PATH_DATA = "";

// Filenames
std::string FILE_SETTINGS	= "settings.txt";
std::string FILE_KEYBINDINGS = "keybindings.txt";

// Video Settings
bool FULLSCREEN;
unsigned char BITS_PER_PIXEL = 32;
unsigned short MAX_FRAMES_PER_SEC;
unsigned short VIEW_W = 0;
unsigned short VIEW_H = 0;
unsigned short VIEW_W_HALF = 0;
unsigned short VIEW_H_HALF = 0;
float VIEW_SCALING = 1.0f;
unsigned short SCREEN_W = 640;
unsigned short SCREEN_H = 480;
bool VSYNC;
bool HWSURFACE;
bool TEXTURE_FILTER;
bool DPI_SCALING;
bool CHANGE_GAMMA;
float GAMMA;
std::string RENDER_DEVICE;

// Audio Settings
bool AUDIO = true;
unsigned short MUSIC_VOLUME;
unsigned short SOUND_VOLUME;

// Interface Settings
bool COMBAT_TEXT;
bool SHOW_FPS;
bool COLORBLIND;
bool HARDWARE_CURSOR;
bool DEV_MODE;
bool DEV_HUD;
bool LOOT_TOOLTIPS;
bool STATBAR_LABELS;
bool AUTO_EQUIP;
bool SUBTITLES;
bool SHOW_HUD = true;

// Input Settings
bool MOUSE_MOVE;
bool ENABLE_JOYSTICK;
int JOYSTICK_DEVICE;
bool MOUSE_AIM;
bool NO_MOUSE;
int JOY_DEADZONE;
bool TOUCHSCREEN = false;
bool MOUSE_SCALED = true;

// Language Settings
std::string LANGUAGE = "en";

// Command-line settings
std::string LOAD_SLOT;
std::string LOAD_SCRIPT;

// Other Settings
float ENCOUNTER_DIST;
int PREV_SAVE_SLOT = -1;
bool SOFT_RESET = false;

static ConfigEntry * getConfigEntry(const char * name) {

	for (size_t i = 0; i < config_size; i++) {
		if (std::strcmp(config[i].name, name) == 0) return config + i;
	}

	logError("Settings: '%s' is not a valid configuration key.", name);
	return NULL;
}

static ConfigEntry * getConfigEntry(const std::string & name) {
	return getConfigEntry(name.c_str());
}

void loadSettings() {

	// init defaults
	for (size_t i = 0; i < config_size; i++) {
		ConfigEntry * entry = config + i;
		tryParseValue(*entry->type, entry->default_val, entry->storage);
	}

	// try read from file
	FileParser infile;
	if (!infile.open(PATH_CONF + FILE_SETTINGS, !FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		loadMobileDefaults();
		if (!infile.open("engine/default_settings.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
			saveSettings();
			return;
		}
		else saveSettings();
	}

	while (infile.next()) {

		ConfigEntry * entry = getConfigEntry(infile.key);
		if (entry) {
			tryParseValue(*entry->type, infile.val, entry->storage);
		}
	}
	infile.close();

	loadMobileDefaults();
}

/**
 * Save the current main settings (primary video and audio settings)
 */
bool saveSettings() {

	std::ofstream outfile;
	outfile.open((PATH_CONF + FILE_SETTINGS).c_str(), std::ios::out);

	if (outfile.is_open()) {

		// comment
		outfile << "## flare-engine settings file ##" << "\n";

		for (size_t i = 0; i < config_size; i++) {

			// write additional newline before the next section
			if (i != 0 && config[i].comment != NULL)
				outfile<<"\n";

			if (config[i].comment != NULL) {
				outfile<<"# "<<config[i].comment<<"\n";
			}
			outfile<<config[i].name<<"="<<toString(*config[i].type, config[i].storage)<<"\n";
		}

		if (outfile.bad()) logError("Settings: Unable to write settings file. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		PLATFORM.FSCommit();
	}
	return true;
}

/**
 * Load all default settings, except video settings.
 */
bool loadDefaults() {

	// HACK init defaults except video
	for (size_t i = 3; i < config_size; i++) {
		ConfigEntry * entry = config + i;
		tryParseValue(*entry->type, entry->default_val, entry->storage);
	}

	loadMobileDefaults();

	return true;
}

/**
 * Set required settings for Mobile devices
 */
void loadMobileDefaults() {
	if (PLATFORM.is_mobile_device) {
		MOUSE_MOVE = false;
		MOUSE_AIM = false;
		NO_MOUSE = false;
		ENABLE_JOYSTICK = false;
		HARDWARE_CURSOR = true;
		TOUCHSCREEN = true;
		FULLSCREEN = true;
		DPI_SCALING = true;
	}
}

/**
 * Some variables depend on VIEW_W and VIEW_H. Update them here.
 */
void updateScreenVars() {
	if (eset->tileset.tile_w > 0 && eset->tileset.tile_h > 0) {
		if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC)
			ENCOUNTER_DIST = sqrtf(powf(static_cast<float>(VIEW_W/eset->tileset.tile_w), 2.f) + powf(static_cast<float>(VIEW_H/eset->tileset.tile_h_half), 2.f)) / 2.f;
		else if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL)
			ENCOUNTER_DIST = sqrtf(powf(static_cast<float>(VIEW_W/eset->tileset.tile_w), 2.f) + powf(static_cast<float>(VIEW_H/eset->tileset.tile_h), 2.f)) / 2.f;
	}
}

