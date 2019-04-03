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
		static const int COUNT = 9;
		enum {
			RENDERER,
			FULLSCREEN,
			HWSURFACE,
			VSYNC,
			TEXTURE_FILTER,
			DPI_SCALING,
			PARALLAX_LAYERS,
			ENABLE_GAMMA,
			GAMMA
		};
	};

	class Audio {
	public:
		static const int COUNT = 2;
		enum {
			SFX,
			MUSIC
		};
	};

	class Interface {
	public:
		static const int COUNT = 15;
		enum {
			LANGUAGE,
			MINIMAP_MODE,
			LOOT_TOOLTIPS,
			SUBTITLES,
			COLORBLIND,
			STATBAR_LABELS,
			STATBAR_AUTOHIDE,
			COMBAT_TEXT,
			AUTO_EQUIP,
			HARDWARE_CURSOR,
			ENTITY_MARKERS,
			LOW_HP_WARNING_TYPE,
			LOW_HP_THRESHOLD,
			SHOW_FPS,
			DEV_MODE
		};
	};

	class Input {
	public:
		static const int COUNT = 7;
		enum {
			MOUSE_MOVE,
			MOUSE_MOVE_SWAP,
			MOUSE_MOVE_ATTACK,
			MOUSE_AIM,
			NO_MOUSE,
			JOYSTICK,
			JOYSTICK_DEADZONE
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

	bool has_exit_button;
	bool is_mobile_device;
	bool force_hardware_cursor;
	bool has_lock_file;
	unsigned char config_menu_type;
	std::string default_renderer;

	std::vector<bool> config_video;
	std::vector<bool> config_audio;
	std::vector<bool> config_interface;
	std::vector<bool> config_input;
	std::vector<bool> config_misc;
};

extern Platform platform;

#endif
