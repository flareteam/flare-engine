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

const float Settings::LOGIC_FPS = 60.f;

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
	, safe_video(false)
{
	config.resize(51);
	setConfigDefault(0,  "move_type_dimissed",  &typeid(move_type_dimissed),  "0",            &move_type_dimissed,  "One time flag for initial movement type dialog | 0 = show dialog, 1 = no dialog");
	setConfigDefault(1,  "fullscreen",          &typeid(fullscreen),          "0",            &fullscreen,          "Fullscreen mode | 0 = disable, 1 = enable");
	setConfigDefault(2,  "resolution_w",        &typeid(screen_w),            "640",          &screen_w,            "Window size");
	setConfigDefault(3,  "resolution_h",        &typeid(screen_h),            "480",          &screen_h,            "");
	setConfigDefault(4,  "music_volume",        &typeid(music_volume),        "96",           &music_volume,        "Music and sound volume | 0 = silent, 128 = maximum");
	setConfigDefault(5,  "sound_volume",        &typeid(sound_volume),        "128",          &sound_volume,        "");
	setConfigDefault(6,  "combat_text",         &typeid(combat_text),         "1",            &combat_text,         "Display floating damage text | 0 = disable, 1 = enable");
	setConfigDefault(7,  "mouse_move",          &typeid(mouse_move),          "0",            &mouse_move,          "Use mouse to move | 0 = disable, 1 = enable");
	setConfigDefault(8,  "hwsurface",           &typeid(hwsurface),           "1",            &hwsurface,           "Hardware surfaces & V-sync. Try disabling for performance. | 0 = disable, 1 = enable");
	setConfigDefault(9,  "vsync",               &typeid(vsync),               "1",            &vsync,               "");
	setConfigDefault(10, "texture_filter",      &typeid(texture_filter),      "1",            &texture_filter,      "Texture filter quality | 0 = nearest neighbor (worst), 1 = linear (best)");
	setConfigDefault(11, "dpi_scaling",         &typeid(dpi_scaling),         "0",            &dpi_scaling,         "DPI-based render scaling | 0 = disable, 1 = enable");
	setConfigDefault(12, "parallax_layers",     &typeid(parallax_layers),     "1",            &parallax_layers,     "Rendering of parallax map layers | 0 = disable, 1 = enable");
	setConfigDefault(13, "max_fps",             &typeid(max_frames_per_sec),  "60",           &max_frames_per_sec,  "Maximum frames per second | 60 = default");
	setConfigDefault(14, "renderer",            &typeid(render_device_name),  "sdl_hardware", &render_device_name,  "Default render device. | sdl_hardware = default, Try sdl for compatibility");
	setConfigDefault(15, "enable_joystick",     &typeid(enable_joystick),     "0",            &enable_joystick,     "Joystick settings.");
	setConfigDefault(16, "joystick_device",     &typeid(joystick_device),     "-1",           &joystick_device,     "");
	setConfigDefault(17, "joystick_deadzone",   &typeid(joy_deadzone),        "8000",          &joy_deadzone,        "");
	setConfigDefault(18, "language",            &typeid(language),            "en",           &language,            "2-letter language code.");
	setConfigDefault(19, "change_gamma",        &typeid(change_gamma),        "0",            &change_gamma,        "Allow changing screen gamma (experimental) | 0 = disable, 1 = enable");
	setConfigDefault(20, "gamma",               &typeid(gamma),               "1.0",          &gamma,               "Screen gamma. Requires change_gamma=1 | 0.5 = darkest, 2.0 = lightest");
	setConfigDefault(21, "mouse_aim",           &typeid(mouse_aim),           "1",            &mouse_aim,           "Use mouse to aim | 0 = disable, 1 = enable");
	setConfigDefault(22, "no_mouse",            &typeid(no_mouse),            "0",            &no_mouse,            "Make using mouse secondary, give full control to keyboard | 0 = disable, 1 = enable");
	setConfigDefault(23, "show_fps",            &typeid(show_fps),            "0",            &show_fps,            "Show frames per second | 0 = disable, 1 = enable");
	setConfigDefault(24, "colorblind",          &typeid(colorblind),          "0",            &colorblind,          "Enable colorblind help text | 0 = disable, 1 = enable");
	setConfigDefault(25, "hardware_cursor",     &typeid(hardware_cursor),     "0",            &hardware_cursor,     "Use the system mouse cursor | 0 = disable, 1 = enable");
	setConfigDefault(26, "dev_mode",            &typeid(dev_mode),            "0",            &dev_mode,            "Developer mode | 0 = disable, 1 = enable");
	setConfigDefault(27, "dev_hud",             &typeid(dev_hud),             "1",            &dev_hud,             "Show additional information on-screen when dev_mode=1 | 0 = disable, 1 = enable");
	setConfigDefault(28, "loot_tooltips",       &typeid(loot_tooltips),       "0",            &loot_tooltips,       "Loot tooltip mode | 0 = normal, 1 = show all, 2 = hide all");
	setConfigDefault(29, "statbar_labels",      &typeid(statbar_labels),      "0",            &statbar_labels,      "Always show labels on HP/MP/XP bars | 0 = disable, 1 = enable");
	setConfigDefault(30, "statbar_autohide",    &typeid(statbar_autohide),    "1",            &statbar_autohide,    "Allow the HP/MP/XP bars to auto-hide on inactivity | 0 = disable, 1 = enable");
	setConfigDefault(31, "auto_equip",          &typeid(auto_equip),          "1",            &auto_equip,          "Automatically equip items | 0 = disable, 1 = enable");
	setConfigDefault(32, "subtitles",           &typeid(subtitles),           "0",            &subtitles,           "Subtitles | 0 = disable, 1 = enable");
	setConfigDefault(33, "minimap_mode",        &typeid(minimap_mode),        "0",            &minimap_mode,        "Mini-map display mode | 0 = normal, 1 = 2x zoom, 2 = hidden");
	setConfigDefault(34, "mouse_move_swap",     &typeid(mouse_move_swap),     "0",            &mouse_move_swap,     "Use 'Main2' as the movement action when mouse_move=1 | 0 = disable, 1 = enable");
	setConfigDefault(35, "mouse_move_attack",   &typeid(mouse_move_attack),   "1",            &mouse_move_attack,   "Allow attacking with the mouse movement button if an enemy is targeted and in range | 0 = disable, 1 = enable");
	setConfigDefault(36, "entity_markers",      &typeid(entity_markers),      "1",            &entity_markers,      "Shows a marker above entities that are hidden behind tall tiles | 0 = disable, 1 = enable");
	setConfigDefault(37, "prev_save_slot",      &typeid(prev_save_slot),      "-1",           &prev_save_slot,      "Index of the last used save slot");
	setConfigDefault(38, "low_hp_warning_type", &typeid(low_hp_warning_type), "1",            &low_hp_warning_type, "Low health warning type settings | 0 = disable, 1 = all, 2 = message & cursor, 3 = message & sound, 4 = cursor & sound , 5 = message, 6 = cursor, 7 = sound");
	setConfigDefault(39, "low_hp_threshold",    &typeid(low_hp_threshold),    "20",           &low_hp_threshold,    "Low HP warning threshold percentage");
	setConfigDefault(40, "item_compare_tips",   &typeid(item_compare_tips),   "1",            &item_compare_tips,   "Show comparison tooltips for equipped items of the same type | 0 = disable, 1 = enable");
	setConfigDefault(41, "max_render_size",     &typeid(max_render_size),     "0",            &max_render_size,     "Overrides the maximum height (in pixels) of the internal render surface | 0 = ignore this setting");
	setConfigDefault(42, "touch_controls",      &typeid(touchscreen),         "0",            &touchscreen,         "Enables touch screen controls | 0 = disable, 1 = enable");
	setConfigDefault(43, "touch_scale",         &typeid(touch_scale),         "1.0",          &touch_scale,         "Factor used to scale the touch controls | 1.0 = 100 percent scale");
	setConfigDefault(44, "mute_on_focus_loss",  &typeid(mute_on_focus_loss),  "1",            &mute_on_focus_loss,  "Mute game audio when the game window loses focus | 0 = disable, 1 = enable");
	setConfigDefault(45, "pause_on_focus_loss", &typeid(pause_on_focus_loss), "1",            &pause_on_focus_loss, "Pause game when the game window loses focus | 0 = disable, 1 = enable");
	setConfigDefault(46, "audio_freq",          &typeid(audio_freq),          "44100",        &audio_freq,          "Audio playback frequency in Hz. Default is 44100");
	setConfigDefault(47, "dev_cmd_1",           &typeid(dev_cmd_1),           "toggle_fps",    &dev_cmd_1,           "Custom developer console shortcut command");
	setConfigDefault(48, "dev_cmd_2",           &typeid(dev_cmd_2),           "toggle_devhud", &dev_cmd_2,           "Custom developer console shortcut command");
	setConfigDefault(49, "dev_cmd_3",           &typeid(dev_cmd_3),           "toggle_hud",    &dev_cmd_3,           "Custom developer console shortcut command");
	setConfigDefault(50, "auto_loot",           &typeid(auto_loot),           "1",             &auto_loot,           "Automatically pick up loot | 0 = disable, 1 = enable, 2 = currency only");
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
	bool found_settings = false;

	FileParser infile;
	if (infile.open(settings->path_conf + "settings.txt", !FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		found_settings = true;
	}
	else if (infile.open("engine/default_settings.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		found_settings = true;
	}

	if (!found_settings) {
		saveSettings();
	}
	else {
		while (infile.next()) {
			size_t entry = getConfigEntry(infile.key);
			if (entry != config.size()) {
				Parse::tryParseValue(*config[entry].type, infile.val, config[entry].storage);
			}
		}
		infile.close();

		// validate joystick deadzone value
		// TODO validation for all setting vars?
		if (joy_deadzone < JOY_DEADZONE_MIN || joy_deadzone > JOY_DEADZONE_MAX) {
			joy_deadzone = JOY_DEADZONE_MIN;
		}
	}

	// Force using the software renderer if safe mode is enabled
	if (safe_video) {
		render_device_name = "sdl";
	}
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
	// HACK init defaults except video and one-time flags
	for (size_t i = 4; i < config.size(); i++) {
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

void Settings::logSettings() {
	for (size_t i = 0; i < config.size(); ++i) {
		Utils::logInfo("Settings: %s=%s", config[i].name.c_str(), configValueToString(*config[i].type, config[i].storage).c_str());
	}
}

std::string Settings::configValueToString(const std::type_info &type, void *storage) {
	std::stringstream stream;

	if (type == typeid(bool)) {
		stream << *(static_cast<bool*>(storage));
	}
	else if (type == typeid(int)) {
		stream << *(static_cast<int*>(storage));
	}
	else if (type == typeid(unsigned int)) {
		stream << *(static_cast<unsigned int*>(storage));
	}
	else if (type == typeid(short)) {
		stream << *(static_cast<short*>(storage));
	}
	else if (type == typeid(unsigned short)) {
		stream << *(static_cast<unsigned short*>(storage));
	}
	else if (type == typeid(char)) {
		stream << *(static_cast<char*>(storage));
	}
	else if (type == typeid(unsigned char)) {
		stream << *(static_cast<unsigned char*>(storage));
	}
	else if (type == typeid(float)) {
		stream << *(static_cast<float*>(storage));
	}
	else if (type == typeid(std::string)) {
		stream << *(static_cast<std::string*>(storage));
	}

	return stream.str();
}

