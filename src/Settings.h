/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller

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

const std::string VERSION_NAME = "Flare Alpha";
const int VERSION_MAJOR = 0;
const int VERSION_MINOR = 19;

class Element {
public:
	std::string name;
	std::string description;
};

const int ACTIONBAR_MAX = 12; // maximum number of slots in MenuActionBar

class HeroClass {
public:
	std::string name;
	std::string description;
	int currency;
	std::string equipment;
	int physical;
	int mental;
	int offense;
	int defense;
	std::vector<int> hotkeys;
	std::vector<int> powers;
	std::vector<std::string> statuses;
	std::string power_tree;

	HeroClass()
		: name("")
		, description("")
		, currency(0)
		, equipment("")
		, physical(0)
		, mental(0)
		, offense(0)
		, defense(0)
		, hotkeys(std::vector<int>(ACTIONBAR_MAX, 0))
		, power_tree("") {
	}
};

// Path info
extern std::string PATH_CONF; // user-configurable settings files
extern std::string PATH_USER; // important per-user data (saves)
extern std::string PATH_DATA; // common game data
extern std::string CUSTOM_PATH_DATA; // user-defined replacement for PATH_DATA

// Filenames
extern std::string FILE_SETTINGS;     // Name of the settings file (e.g. "settings.txt").
extern std::string FILE_KEYBINDINGS;  // Name of the key bindings file (e.g. "keybindings.txt").

// Main Menu frame size
extern unsigned short FRAME_W;
extern unsigned short FRAME_H;

extern unsigned short ICON_SIZE;

// Audio and Video Settings
extern bool AUDIO;					// initialize the audio subsystem at all?
extern unsigned short MUSIC_VOLUME;
extern unsigned short SOUND_VOLUME;
extern bool FULLSCREEN;
extern unsigned char BITS_PER_PIXEL;
extern unsigned short MAX_FRAMES_PER_SEC;
extern unsigned short VIEW_W;
extern unsigned short VIEW_H;
extern unsigned short VIEW_W_HALF;
extern unsigned short VIEW_H_HALF;
extern short MIN_VIEW_W;
extern short MIN_VIEW_H;
extern bool DOUBLEBUF;
extern bool HWSURFACE;
extern bool CHANGE_GAMMA;
extern float GAMMA;

// Input Settings
extern bool MOUSE_MOVE;
extern bool ENABLE_JOYSTICK;
extern int JOYSTICK_DEVICE;
extern bool MOUSE_AIM;
extern bool NO_MOUSE;
extern int JOY_DEADZONE;
extern bool TOUCHSCREEN;

// Interface Settings
extern bool COMBAT_TEXT;
extern bool SHOW_FPS;
extern bool SHOW_HOTKEYS;
extern bool COLORBLIND;
extern bool HARDWARE_CURSOR;
extern bool DEV_MODE;
extern bool DEV_HUD;
extern bool SHOW_TARGET;
extern bool LOOT_TOOLTIPS;

// Engine Settings
extern bool MENUS_PAUSE;
extern bool SAVE_HPMP;
extern bool ENABLE_PLAYGAME;
extern int CORPSE_TIMEOUT;
extern bool SELL_WITHOUT_VENDOR;
extern int AIM_ASSIST;
extern std::string WINDOW_TITLE;
extern std::string SAVE_PREFIX;
extern int SOUND_FALLOFF;
extern int PARTY_EXP_PERCENTAGE;
extern bool ENABLE_ALLY_COLLISION_AI;
extern bool ENABLE_ALLY_COLLISION;
extern int CURRENCY_ID;
extern float INTERACT_RANGE;
extern bool SAVE_ONLOAD;
extern bool SAVE_ONEXIT;

// Tile Settings
extern float UNITS_PER_PIXEL_X;
extern float UNITS_PER_PIXEL_Y;
extern unsigned short TILE_W;
extern unsigned short TILE_H;
extern unsigned short TILE_W_HALF;
extern unsigned short TILE_H_HALF;
extern unsigned short TILESET_ORIENTATION;
extern unsigned short TILESET_ISOMETRIC;
extern unsigned short TILESET_ORTHOGONAL;

// Language Settings
extern std::string LANGUAGE;

// Autopickup Settings
extern bool AUTOPICKUP_CURRENCY;

// Combat calculation caps
extern short MAX_ABSORB;
extern short MAX_RESIST;
extern short MAX_BLOCK;
extern short MAX_AVOIDANCE;
extern short MIN_ABSORB;
extern short MIN_RESIST;
extern short MIN_BLOCK;
extern short MIN_AVOIDANCE;

// Elemental types
extern std::vector<Element> ELEMENTS;

// Equip flags
extern std::map<std::string,std::string> EQUIP_FLAGS;

// Hero classes
extern std::vector<HeroClass> HERO_CLASSES;

// Currency settings
extern std::string CURRENCY;
extern float VENDOR_RATIO;

// Death penalty settings
extern bool DEATH_PENALTY;
extern bool DEATH_PENALTY_PERMADEATH;
extern int DEATH_PENALTY_CURRENCY;
extern int DEATH_PENALTY_XP;
extern int DEATH_PENALTY_XP_CURRENT;
extern bool DEATH_PENALTY_ITEM;

void setPaths();
void loadTilesetSettings();
void loadMiscSettings();
bool loadSettings();
bool saveSettings();
bool loadDefaults();
void loadAndroidDefaults();

// version information
std::string getVersionString();
bool compareVersions(int maj0, int min0, int maj1, int min1);

#endif
