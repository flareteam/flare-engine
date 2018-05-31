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

class Element {
public:
	std::string id;
	std::string name;
};

class EquipFlag {
public:
	std::string id;
	std::string name;
};

class PrimaryStat {
public:
	std::string id;
	std::string name;
};

class HeroClass {
public:
	std::string name;
	std::string description;
	int currency;
	std::string equipment;
	std::string carried;
	std::vector<int> primary;
	std::vector<int> hotkeys;
	std::vector<int> powers;
	std::vector<std::string> statuses;
	std::string power_tree;
	int default_power_tab;
	std::vector<int> options;

	HeroClass();
};

class DamageType {
public:
	std::string id;
	std::string name;
	std::string name_min;
	std::string name_max;
	std::string description;
	std::string min;
	std::string max;
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
extern float VIEW_SCALING;
extern unsigned short SCREEN_W;
extern unsigned short SCREEN_H;
extern short MIN_SCREEN_W;
extern short MIN_SCREEN_H;
extern bool VSYNC;
extern bool HWSURFACE;
extern bool TEXTURE_FILTER;
extern bool IGNORE_TEXTURE_FILTER;
extern bool DPI_SCALING;
extern bool CHANGE_GAMMA;
extern float GAMMA;
extern std::string RENDER_DEVICE;
extern std::vector<unsigned short> VIRTUAL_HEIGHTS;
extern float VIRTUAL_DPI;

// Input Settings
extern bool MOUSE_MOVE;
extern bool ENABLE_JOYSTICK;
extern int JOYSTICK_DEVICE;
extern bool MOUSE_AIM;
extern bool NO_MOUSE;
extern int JOY_DEADZONE;
extern bool TOUCHSCREEN;
extern bool MOUSE_SCALED; // mouse position is automatically scaled to VIEW_W x VIEW_H resolution

// Interface Settings
extern bool COMBAT_TEXT;
extern bool SHOW_FPS;
extern bool COLORBLIND;
extern bool HARDWARE_CURSOR;
extern bool DEV_MODE;
extern bool DEV_HUD;
extern bool LOOT_TOOLTIPS;
extern bool STATBAR_LABELS;
extern bool AUTO_EQUIP;
extern bool SUBTITLES;
extern bool SHOW_HUD;

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
extern float ENCOUNTER_DIST;
extern float CAMERA_SPEED;
extern bool SAVE_BUYBACK;
extern bool KEEP_BUYBACK_ON_MAP_CHANGE;

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
extern int MAX_ABSORB;
extern int MAX_RESIST;
extern int MAX_BLOCK;
extern int MAX_AVOIDANCE;
extern int MIN_ABSORB;
extern int MIN_RESIST;
extern int MIN_BLOCK;
extern int MIN_AVOIDANCE;
extern int MIN_MISS_DAMAGE;
extern int MAX_MISS_DAMAGE;
extern int MIN_CRIT_DAMAGE;
extern int MAX_CRIT_DAMAGE;
extern int MIN_OVERHIT_DAMAGE;
extern int MAX_OVERHIT_DAMAGE;

// Gameplay attribue definitions
extern std::vector<PrimaryStat> PRIMARY_STATS;
extern std::vector<Element> ELEMENTS;
extern std::vector<EquipFlag> EQUIP_FLAGS;
extern std::vector<HeroClass> HERO_CLASSES;
extern std::vector<DamageType> DAMAGE_TYPES;
extern size_t DAMAGE_TYPES_COUNT; // DAMAGE_TYPES.size() * 2, to account for min & max

// Currency settings
extern std::string CURRENCY;
extern float VENDOR_RATIO;
extern float VENDOR_RATIO_BUYBACK;

// Death penalty settings
extern bool DEATH_PENALTY;
extern bool DEATH_PENALTY_PERMADEATH;
extern int DEATH_PENALTY_CURRENCY;
extern int DEATH_PENALTY_XP;
extern int DEATH_PENALTY_XP_CURRENT;
extern bool DEATH_PENALTY_ITEM;

// Tooltip Settings
extern int TOOLTIP_OFFSET; // distance between cursor and tooltip
extern int TOOLTIP_WIDTH; // max width of tooltips (wrap text)
extern int TOOLTIP_MARGIN; // outer margin between tooltip text and the edge of the tooltip background
extern int TOOLTIP_MARGIN_NPC;
extern int TOOLTIP_BACKGROUND_BORDER;

// Command-line settings
extern std::string LOAD_SLOT;
extern std::string LOAD_SCRIPT;

// Misc
extern int PREV_SAVE_SLOT;
extern bool SOFT_RESET;

void loadTilesetSettings();
void loadMiscSettings();
void loadSettings();
bool saveSettings();
bool loadDefaults();
void loadMobileDefaults();
void updateScreenVars();
size_t getPrimaryStatIndex(const std::string& id_str);
HeroClass* getHeroClassByName(const std::string& name);

// version information
std::string getVersionString();

#endif
