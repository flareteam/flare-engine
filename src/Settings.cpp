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

Settings::Settings()
	: path_conf("")
	, path_user("")
	, path_data("")
	, custom_path_data("")
	, load_slot("")
	, load_script("")
	, view_w(0)
	, view_h(0)
	, view_w_half(0)
	, view_h_half(0)
	, view_scaling(1.0f)
	, audio(true)
	, touchscreen(false)
	, mouse_scaled(true)
	, show_hud(true)
	, encounter_dist(0) // set in updateScreenVars()
	, soft_reset(false)
{
	config.resize(36);
	setConfigDefault(0,  "fullscreen",        &typeid(fullscreen),         "0",            &fullscreen,         "fullscreen mode. 1 enable, 0 disable.");
	setConfigDefault(1,  "resolution_w",      &typeid(screen_w),           "640",          &screen_w,           "display resolution. 640x480 minimum.");
	setConfigDefault(2,  "resolution_h",      &typeid(screen_h),           "480",          &screen_h,           "");
	setConfigDefault(3,  "music_volume",      &typeid(music_volume),       "96",           &music_volume,       "music and sound volume (0 = silent, 128 = max)");
	setConfigDefault(4,  "sound_volume",      &typeid(sound_volume),       "128",          &sound_volume,       "");
	setConfigDefault(5,  "combat_text",       &typeid(combat_text),        "1",            &combat_text,        "display floating damage text. 1 enable, 0 disable.");
	setConfigDefault(6,  "mouse_move",        &typeid(mouse_move),         "0",            &mouse_move,         "use mouse to move (experimental). 1 enable, 0 disable.");
	setConfigDefault(7,  "hwsurface",         &typeid(hwsurface),          "1",            &hwsurface,          "hardware surfaces, v-sync. Try disabling for performance. 1 enable, 0 disable.");
	setConfigDefault(8,  "vsync",             &typeid(vsync),              "1",            &vsync,              "");
	setConfigDefault(9,  "texture_filter",    &typeid(texture_filter),     "1",            &texture_filter,     "texture filter quality. 0 nearest neighbor (worst), 1 linear (best)");
	setConfigDefault(10, "dpi_scaling",       &typeid(dpi_scaling),        "0",            &dpi_scaling,        "toggle DPI-based render scaling. 1 enable, 0 disable");
	setConfigDefault(11, "parallax_layers",   &typeid(parallax_layers),    "1",            &parallax_layers,    "toggle rendering of parallax map layers. 1 enable, 0 disable");
	setConfigDefault(12, "max_fps",           &typeid(max_frames_per_sec), "60",           &max_frames_per_sec, "maximum frames per second. default is 60");
	setConfigDefault(13, "renderer",          &typeid(render_device_name), "sdl_hardware", &render_device_name, "default render device. 'sdl' is the default setting");
	setConfigDefault(14, "enable_joystick",   &typeid(enable_joystick),    "0",            &enable_joystick,    "joystick settings.");
	setConfigDefault(15, "joystick_device",   &typeid(joystick_device),    "0",            &joystick_device,    "");
	setConfigDefault(16, "joystick_deadzone", &typeid(joy_deadzone),       "100",          &joy_deadzone,       "");
	setConfigDefault(17, "language",          &typeid(language),           "en",           &language,           "2-letter language code.");
	setConfigDefault(18, "change_gamma",      &typeid(change_gamma),       "0",            &change_gamma,       "allow changing gamma (experimental). 1 enable, 0 disable.");
	setConfigDefault(19, "gamma",             &typeid(gamma),              "1.0",          &gamma,              "screen gamma (0.5 = darkest, 2.0 = lightest)");
	setConfigDefault(20, "mouse_aim",         &typeid(mouse_aim),          "1",            &mouse_aim,          "use mouse to aim. 1 enable, 0 disable.");
	setConfigDefault(21, "no_mouse",          &typeid(no_mouse),           "0",            &no_mouse,           "make using mouse secondary, give full control to keyboard. 1 enable, 0 disable.");
	setConfigDefault(22, "show_fps",          &typeid(show_fps),           "0",            &show_fps,           "show frames per second. 1 enable, 0 disable.");
	setConfigDefault(23, "colorblind",        &typeid(colorblind),         "0",            &colorblind,         "enable colorblind tooltips. 1 enable, 0 disable");
	setConfigDefault(24, "hardware_cursor",   &typeid(hardware_cursor),    "0",            &hardware_cursor,    "use the system mouse cursor. 1 enable, 0 disable");
	setConfigDefault(25, "dev_mode",          &typeid(dev_mode),           "0",            &dev_mode,           "allow opening the developer console. 1 enable, 0 disable");
	setConfigDefault(26, "dev_hud",           &typeid(dev_hud),            "1",            &dev_hud,            "shows some additional information on-screen when developer mode is enabled. 1 enable, 0 disable");
	setConfigDefault(27, "loot_tooltips",     &typeid(loot_tooltips),      "0",            &loot_tooltips,      "loot tooltip mode. 0 normal, 1 show all, 2 hide all");
	setConfigDefault(28, "statbar_labels",    &typeid(statbar_labels),     "0",            &statbar_labels,     "always show labels on HP/MP/XP bars. 1 enable, 0 disable");
	setConfigDefault(29, "auto_equip",        &typeid(auto_equip),         "1",            &auto_equip,         "automatically equip items. 1 enable, 0 disable");
	setConfigDefault(30, "subtitles",         &typeid(subtitles),          "0",            &subtitles,          "displays subtitles. 1 enable, 0 disable");
	setConfigDefault(31, "minimap_mode",      &typeid(minimap_mode),       "0",            &minimap_mode,       "mini-map display mode. 0 is normal, 1 is 2x zoom, 2 is hidden");
	setConfigDefault(32, "mouse_move_swap",   &typeid(mouse_move_swap),    "0",            &mouse_move_swap,    "use 'Main2' as the movement action when using mouse movement. 1 enable, 0 disable.");
	setConfigDefault(33, "mouse_move_attack", &typeid(mouse_move_attack),  "1",            &mouse_move_attack,  "allows attacking with the mouse movement button if an enemy is targeted and in range. 1 enable, 0 disable.");
	setConfigDefault(34, "entity_markers",    &typeid(entity_markers),     "1",            &entity_markers,     "shows are marker above entities that are hidden behind tall tiles. 1 enable, 0 disable.");
	setConfigDefault(35, "prev_save_slot",    &typeid(prev_save_slot),     "-1",           &prev_save_slot,     "index of the last used save slot");
}

void Settings::setConfigDefault(size_t index, const std::string& name, const std::type_info *type, const std::string& default_val, void *storage, const std::string& comment) {
	if (index >= config.size()) {
		Utils::logError("Settings: Can't set default config value; %u is not a valid index.", index);
		return;
	}

	config[index].name = name;
	config[index].type = type;
	config[index].default_val = default_val;
	config[index].storage = storage;
	config[index].comment = comment;
}

size_t Settings::getConfigEntry(const std::string& name) {
	for (size_t i = 0; i < config.size(); i++) {
		if (config[i].name == name)
			return i;
	}

	Utils::logError("Settings: '%s' is not a valid configuration key.", name.c_str());
	return config.size();
}

void Settings::loadSettings() {
	// init defaults
	for (size_t i = 0; i < config.size(); i++) {
		Parse::tryParseValue(*config[i].type, config[i].default_val, config[i].storage);
	}

	// try read from file
	FileParser infile;
	if (!infile.open(settings->path_conf + "settings.txt", !FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		loadMobileDefaults();
		if (!infile.open("engine/default_settings.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
			saveSettings();
			return;
		}
		else saveSettings();
	}

	while (infile.next()) {
		size_t entry = getConfigEntry(infile.key);
		if (entry != config.size()) {
			Parse::tryParseValue(*config[entry].type, infile.val, config[entry].storage);
		}
	}
	infile.close();

	loadMobileDefaults();
}

/**
 * Save the current main settings (primary video and audio settings)
 */
void Settings::saveSettings() {
	std::ofstream outfile;
	outfile.open((settings->path_conf + "settings.txt").c_str(), std::ios::out);

	if (outfile.is_open()) {

		// comment
		outfile << "## flare-engine settings file ##" << "\n";

		for (size_t i = 0; i < config.size(); i++) {

			// write additional newline before the next section
			if (i != 0 && !config[i].comment.empty())
				outfile<<"\n";

			if (!config[i].comment.empty()) {
				outfile<<"# "<<config[i].comment<<"\n";
			}
			outfile << config[i].name << "=" << Parse::toString(*config[i].type, config[i].storage) << "\n";
		}

		if (outfile.bad()) Utils::logError("Settings: Unable to write settings file. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		platform.FSCommit();
	}
}

/**
 * Load all default settings, except video settings.
 */
void Settings::loadDefaults() {
	// HACK init defaults except video
	for (size_t i = 3; i < config.size(); i++) {
		Parse::tryParseValue(*config[i].type, config[i].default_val, config[i].storage);
	}

	loadMobileDefaults();
}

/**
 * Set required settings for Mobile devices
 */
void Settings::loadMobileDefaults() {
	if (platform.is_mobile_device) {
		mouse_move = false;
		mouse_aim = false;
		no_mouse = false;
		enable_joystick = false;
		hardware_cursor = true;
		touchscreen = true;
		fullscreen = true;
		dpi_scaling = true;
	}
}

/**
 * Some variables depend on VIEW_W and VIEW_H. Update them here.
 */
void Settings::updateScreenVars() {
	if (eset->tileset.tile_w > 0 && eset->tileset.tile_h > 0) {
		if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC)
			encounter_dist = sqrtf(powf(static_cast<float>(view_w/eset->tileset.tile_w), 2.f) + powf(static_cast<float>(view_h/eset->tileset.tile_h_half), 2.f)) / 2.f;
		else if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL)
			encounter_dist = sqrtf(powf(static_cast<float>(view_w/eset->tileset.tile_w), 2.f) + powf(static_cast<float>(view_h/eset->tileset.tile_h), 2.f)) / 2.f;
	}
}

