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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "CommonIncludes.h"

#include <typeinfo>

class Settings {
public:
	static const float LOGIC_FPS;

	enum {
		LOOT_TIPS_DEFAULT = 0,
		LOOT_TIPS_SHOW_ALL = 1,
		LOOT_TIPS_HIDE_ALL = 2
	};

	enum {
		MINIMAP_NORMAL = 0,
		MINIMAP_2X = 1,
		MINIMAP_HIDDEN = 2
	};

	enum {
		LHP_WARN_NONE = 0,
		LHP_WARN_ALL = 1,
		LHP_WARN_TEXT_CURSOR = 2,
		LHP_WARN_TEXT_SOUND = 3,
		LHP_WARN_CURSOR_SOUND = 4,
		LHP_WARN_TEXT = 5,
		LHP_WARN_CURSOR = 6,
		LHP_WARN_SOUND = 7
	};

	static const int JOY_DEADZONE_MIN = 8000;
	static const int JOY_DEADZONE_MAX = 32767;

	Settings();
	void loadSettings();
	void saveSettings();
	void loadDefaults();
	void updateScreenVars();
	void logSettings();

	// Video Settings
	bool fullscreen;
	unsigned short screen_w;
	unsigned short screen_h;
	bool hwsurface;
	bool vsync;
	bool texture_filter;
	bool dpi_scaling;
	unsigned short max_frames_per_sec;
	std::string render_device_name;
	bool change_gamma;
	float gamma;
	bool parallax_layers;
	unsigned short max_render_size;

	// Audio Settings
	unsigned short music_volume;
	unsigned short sound_volume;
	bool mute_on_focus_loss;
	unsigned int audio_freq;

	// Input Settings
	bool mouse_move;
	bool mouse_move_swap;
	bool mouse_move_attack;
	bool enable_joystick;
	int joystick_device;
	bool mouse_aim;
	bool no_mouse;
	int joy_deadzone;
	float touch_scale;
	bool joystick_rumble;

	// Game Settings
	bool auto_equip;
	int auto_loot;
	int low_hp_warning_type;
	int low_hp_threshold;

	// Interface Settings
	bool combat_text;
	bool show_fps;
	bool colorblind;
	bool hardware_cursor;
	bool dev_mode;
	bool dev_hud;
	int loot_tooltips;
	bool statbar_labels;
	bool statbar_autohide;
	bool subtitles;
	int minimap_mode;
	bool entity_markers;
	bool item_compare_tips;
	bool pause_on_focus_loss;

	// Language Settings
	std::string language;

	// Misc
	int prev_save_slot;
	bool setup_language;
	bool setup_mousemove;

	// Dev console: shortcut commands
	std::string dev_cmd_1;
	std::string dev_cmd_2;
	std::string dev_cmd_3;

	/**
	 * NOTE Everything below is not part of the user's settings.txt, but somehow ended up here
	 * TODO Move these to more appropriate locations?
	 */

	// Path info
	std::string path_conf; // user-configurable settings files
	std::string path_user; // important per-user data (saves)
	std::string path_data; // common game data
	std::string custom_path_data; // user-defined replacement for PATH_DATA

	// Command-line settings
	std::string load_slot;
	std::string load_script;

	// Misc
	unsigned short view_w;
	unsigned short view_h;
	unsigned short view_w_half;
	unsigned short view_h_half;
	float view_scaling;

	bool audio;

	bool touchscreen;
	bool mouse_scaled; // mouse position is automatically scaled to view_w * view_h resolution

	bool show_hud;

	float encounter_dist;

	bool soft_reset;

	bool safe_video;

private:
	class ConfigEntry {
	public:
		std::string name;
		const std::type_info *type;
		std::string default_val;
		void *storage;
		std::string comment;
	};
	std::vector<ConfigEntry> config;

	void setConfigDefault(size_t index, const std::string& name, const std::type_info *type, const std::string& default_val, void *storage, const std::string& comment);
	size_t getConfigEntry(const std::string& name);
	void loadMobileDefaults();
	std::string configValueToString(const std::type_info &type, void *storage);
};
#endif
