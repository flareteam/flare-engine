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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

class Platform {
public:
	enum {
		CONFIG_MENU_TYPE_BASE = 0,
		CONFIG_MENU_TYPE_DESKTOP = 1,
		CONFIG_MENU_TYPE_DESKTOP_NO_VIDEO = 2
	};

	class Video {
	public:
		static const int COUNT = 11;
		enum {
			RENDERER,
			FULLSCREEN,
			HWSURFACE,
			VSYNC,
			TEXTURE_FILTER,
			DPI_SCALING,
			PARALLAX_LAYERS,
			MAX_RENDER_SIZE,
			FRAME_LIMIT,
			THREADED_IMAGE_LOAD,
			FADE_WALLS,
		};
	};

	class Audio {
	public:
		static const int COUNT = 3;
		enum {
			SFX,
			MUSIC,
			MUTE_ON_FOCUS_LOSS,
		};
	};

	class Game {
	public:
		static const int COUNT = 4;
		enum {
			AUTO_EQUIP,
			AUTO_LOOT,
			LOW_HP_WARNING_TYPE,
			LOW_HP_THRESHOLD,
		};
	};

	class Interface {
	public:
		static const int COUNT = 13;
		enum {
			LANGUAGE,
			SUBTITLES,
			COLORBLIND,
			MINIMAP_MODE,
			LOOT_TOOLTIPS,
			ITEM_COMPARE_TIPS,
			COMBAT_TEXT,
			STATBAR_LABELS,
			STATBAR_AUTOHIDE,
			HARDWARE_CURSOR,
			PAUSE_ON_FOCUS_LOSS,
			SHOW_FPS,
			DEV_MODE
		};
	};

	class Input {
	public:
		static const int COUNT = 10;
		enum {
			MOUSE_MOVE,
			MOUSE_MOVE_SWAP,
			MOUSE_MOVE_ATTACK,
			MOUSE_AIM,
			NO_MOUSE,
			JOYSTICK,
			JOYSTICK_DEADZONE,
			JOYSTICK_RUMBLE,
			TOUCH_CONTROLS,
			TOUCH_SCALE
		};
	};

	class Misc {
	public:
		static const int COUNT = 2;
		enum {
			KEYBINDS,
			MODS
		};
	};

	Platform();
	~Platform();

	void setPaths();
	void setExitEventFilter();
	bool dirCreate(const std::string& path);
	bool dirRemove(const std::string& path);

	void FSInit();
	bool FSCheckReady();
	void FSCommit();

	void setScreenSize();
	void setFullscreen(bool enable);

	bool has_exit_button;
	bool is_mobile_device;
	bool force_hardware_cursor;
	bool has_lock_file;
	bool needs_alt_escape_key;
	bool fullscreen_bypass;
	unsigned char config_menu_type;
	std::string default_renderer;

	std::vector<bool> config_video;
	std::vector<bool> config_audio;
	std::vector<bool> config_game;
	std::vector<bool> config_interface;
	std::vector<bool> config_input;
	std::vector<bool> config_misc;
};

extern Platform platform;

#endif
