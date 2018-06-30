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

// Path info
extern std::string PATH_CONF; // user-configurable settings files
extern std::string PATH_USER; // important per-user data (saves)
extern std::string PATH_DATA; // common game data
extern std::string CUSTOM_PATH_DATA; // user-defined replacement for PATH_DATA

// Audio and Video Settings
extern bool AUDIO;					// initialize the audio subsystem at all?
extern unsigned short MUSIC_VOLUME;
extern unsigned short SOUND_VOLUME;
extern bool FULLSCREEN;
extern unsigned short MAX_FRAMES_PER_SEC;
extern unsigned short VIEW_W;
extern unsigned short VIEW_H;
extern unsigned short VIEW_W_HALF;
extern unsigned short VIEW_H_HALF;
extern float VIEW_SCALING;
extern unsigned short SCREEN_W;
extern unsigned short SCREEN_H;
extern bool VSYNC;
extern bool HWSURFACE;
extern bool TEXTURE_FILTER;
extern bool DPI_SCALING;
extern bool CHANGE_GAMMA;
extern float GAMMA;
extern std::string RENDER_DEVICE;

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
extern float ENCOUNTER_DIST;

// Language Settings
extern std::string LANGUAGE;

// Command-line settings
extern std::string LOAD_SLOT;
extern std::string LOAD_SCRIPT;

// Misc
extern int PREV_SAVE_SLOT;
extern bool SOFT_RESET;

void loadSettings();
bool saveSettings();
bool loadDefaults();
void loadMobileDefaults();
void updateScreenVars();

#endif
