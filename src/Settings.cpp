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

#include <cstring>
#include <typeinfo>
#include <cmath>
using namespace std;

#include "CommonIncludes.h"
#include "FileParser.h"
#include "Settings.h"
#include "Utils.h"
#include "UtilsParsing.h"
#include "UtilsFileSystem.h"
#include "SharedResources.h"

#ifdef _MSC_VER
#define log2(x)	logf(x)/logf(2)
#endif

class ConfigEntry {
public:
	const char * name;
	const type_info * type;
	const char * default_val;
	void * storage;
	const char * comment;
};

ConfigEntry config[] = {
	{ "fullscreen",       &typeid(FULLSCREEN),      "0",   &FULLSCREEN,      "fullscreen mode. 1 enable, 0 disable."},
	{ "resolution_w",     &typeid(VIEW_W),          "640", &VIEW_W,          "display resolution. 640x480 minimum."},
	{ "resolution_h",     &typeid(VIEW_H),          "480", &VIEW_H,          NULL},
	{ "audio",            &typeid(AUDIO),           "1",   &AUDIO,           "Enable music and sound subsystem."},
	{ "music_volume",     &typeid(MUSIC_VOLUME),    "96",  &MUSIC_VOLUME,    "music and sound volume (0 = silent, 128 = max)"},
	{ "sound_volume",     &typeid(SOUND_VOLUME),    "128", &SOUND_VOLUME,    NULL},
	{ "combat_text",      &typeid(COMBAT_TEXT),     "0",   &COMBAT_TEXT,     "display floating damage text. 1 enable, 0 disable."},
	{ "mouse_move",       &typeid(MOUSE_MOVE),      "0",   &MOUSE_MOVE,      "use mouse to move (experimental). 1 enable, 0 disable."},
	{ "hwsurface",        &typeid(HWSURFACE),       "1",   &HWSURFACE,       "hardware surfaces, double buffering. Try disabling for performance. 1 enable, 0 disable."},
	{ "doublebuf",        &typeid(DOUBLEBUF),       "1",   &DOUBLEBUF,       NULL},
	{ "enable_joystick",  &typeid(ENABLE_JOYSTICK), "0",   &ENABLE_JOYSTICK, "joystick settings."},
	{ "joystick_device",  &typeid(JOYSTICK_DEVICE), "0",   &JOYSTICK_DEVICE, NULL},
	{ "joystick_deadzone",&typeid(JOY_DEADZONE),    "100", &JOY_DEADZONE,    NULL},
	{ "language",         &typeid(LANGUAGE),        "en",  &LANGUAGE,        "2-letter language code."},
	{ "change_gamma",     &typeid(CHANGE_GAMMA),    "0",   &CHANGE_GAMMA,    "allow changing gamma (experimental). 1 enable, 0 disable."},
	{ "gamma",            &typeid(GAMMA),           "1.0", &GAMMA,           "screen gamma (0.5 = darkest, 2.0 = lightest)"},
	{ "texture_quality",  &typeid(TEXTURE_QUALITY), "1",   &TEXTURE_QUALITY, "texture quality (0 = low quality, 1 = high quality)"},
	{ "mouse_aim",        &typeid(MOUSE_AIM),       "1",   &MOUSE_AIM,       "use mouse to aim. 1 enable, 0 disable."},
	{ "no_mouse",         &typeid(NO_MOUSE),        "0",   &NO_MOUSE,        "make using mouse secondary, give full control to keyboard. 1 enable, 0 disable."},
	{ "show_fps",         &typeid(SHOW_FPS),        "0",   &SHOW_FPS,        "show frames per second. 1 enable, 0 disable."},
	{ "show_hotkeys",     &typeid(SHOW_HOTKEYS),    "1",   &SHOW_HOTKEYS,    "show hotkeys names on power bar. 1 enable, 0 disable."},
	{ "colorblind",       &typeid(COLORBLIND),      "0",   &COLORBLIND,      "enable colorblind tooltips. 1 enable, 0 disable"}
};
const int config_size = sizeof(config) / sizeof(ConfigEntry);

// Paths
string GAME_FOLDER = "default";
string DEFAULT_FOLDER = "default";
string PATH_CONF = "";
string PATH_USER = "";
string PATH_DATA = "";
string CUSTOM_PATH_DATA = "";
string PATH_DEFAULT_USER = "";
string PATH_DEFAULT_DATA = "";

// Filenames
string FILE_SETTINGS	= "settings.txt";
string FILE_KEYBINDINGS = "keybindings.txt";

// Tile Settings
float UNITS_PER_PIXEL_X;
float UNITS_PER_PIXEL_Y;
unsigned short TILE_W;
unsigned short TILE_H;
unsigned short TILE_W_HALF;
unsigned short TILE_H_HALF;
unsigned short TILESET_ISOMETRIC;
unsigned short TILESET_ORTHOGONAL;
unsigned short TILESET_ORIENTATION;

// Main Menu frame size
unsigned short FRAME_W;
unsigned short FRAME_H;

unsigned short ICON_SIZE;

// Video Settings
bool FULLSCREEN;
unsigned short MAX_FRAMES_PER_SEC = 30;
unsigned char BITS_PER_PIXEL = 32;
unsigned short VIEW_W;
unsigned short VIEW_H;
unsigned short VIEW_W_HALF = VIEW_W/2;
unsigned short VIEW_H_HALF = VIEW_H/2;
short MIN_VIEW_W = -1;
short MIN_VIEW_H = -1;
bool DOUBLEBUF;
bool HWSURFACE;
bool CHANGE_GAMMA;
float GAMMA;
bool TEXTURE_QUALITY;

// Audio Settings
bool AUDIO;
unsigned short MUSIC_VOLUME;
unsigned short SOUND_VOLUME;

// User Preferences
bool COMBAT_TEXT;

// Input Settings
bool MOUSE_MOVE;
bool ENABLE_JOYSTICK;
int JOYSTICK_DEVICE;
bool MOUSE_AIM;
bool NO_MOUSE;
int JOY_DEADZONE;

// Language Settings
std::string LANGUAGE = "en";

// Autopickup Settings
bool AUTOPICKUP_CURRENCY;

// Combat calculation caps (percentage)
short MAX_ABSORB;
short MAX_RESIST;
short MAX_BLOCK;
short MAX_AVOIDANCE;
short MIN_ABSORB;
short MIN_RESIST;
short MIN_BLOCK;
short MIN_AVOIDANCE;

// Elemental types
std::vector<Element> ELEMENTS;

// Equipment flags
std::map<std::string,std::string> EQUIP_FLAGS;

// Hero classes
std::vector<HeroClass> HERO_CLASSES;

// Currency settings
std::string CURRENCY;
float VENDOR_RATIO;

// Death penalty settings
bool DEATH_PENALTY;
bool DEATH_PENALTY_PERMADEATH;
int DEATH_PENALTY_CURRENCY;
int DEATH_PENALTY_XP;
int DEATH_PENALTY_XP_CURRENT;
bool DEATH_PENALTY_ITEM;

// Other Settings
bool MENUS_PAUSE;
bool SAVE_HPMP;
bool ENABLE_PLAYGAME;
bool SHOW_FPS = false;
bool SHOW_HOTKEYS = true;
bool COLORBLIND = false;
int CORPSE_TIMEOUT;
bool SELL_WITHOUT_VENDOR;
int AIM_ASSIST;
std::string GAME_PREFIX = "";
std::string WINDOW_TITLE;
int SOUND_FALLOFF;
int PARTY_EXP_PERCENTAGE;
bool ENABLE_ALLY_COLLISION_AI;
bool ENABLE_ALLY_COLLISION;
int CURRENCY_ID;
float INTERACT_RANGE;

/**
 * Set system paths
 * PATH_CONF is for user-configurable settings files (e.g. keybindings)
 * PATH_USER is for user-specific data (e.g. save games)
 * PATH_DATA is for common game data (e.g. images, music)
 */

#ifdef _WIN32
// Windows paths
void setPaths() {

	// handle Windows-specific path options
	PATH_CONF = "config";
	PATH_USER = "saves";
	PATH_DATA = "";
	PATH_DEFAULT_USER = "";
	PATH_DEFAULT_DATA = "";
	if (dirExists(CUSTOM_PATH_DATA)) PATH_DATA = CUSTOM_PATH_DATA;
	else if (!CUSTOM_PATH_DATA.empty()) fprintf(stderr, "Error: Could not find specified game data directory.\n");

	// TODO: place config and save data in the user's home, windows style
	createDir(PATH_CONF);
	createDir(PATH_USER);

	PATH_CONF = PATH_CONF + "/";
	PATH_USER = PATH_USER + "/";
}
#elif __amigaos4__
// AmigaOS paths
void setPaths() {
	PATH_CONF = "PROGDIR:";
	PATH_USER = "PROGDIR:";
	PATH_DATA = "PROGDIR:";
	PATH_DEFAULT_USER = "PROGDIR:";
	PATH_DEFAULT_DATA = "PROGDIR:";
	if (dirExists(CUSTOM_PATH_DATA)) PATH_DATA = CUSTOM_PATH_DATA;
	else if (!CUSTOM_PATH_DATA.empty()) fprintf(stderr, "Error: Could not find specified game data directory.\n");
}
#else
void setPaths() {

	// attempting to follow this spec:
	// http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

	// Note: If the GAME_FOLDER isn't defined, we fall back to using the current directory

	// set config path (settings, keybindings)
	// $XDG_CONFIG_HOME/flare/
	if (getenv("XDG_CONFIG_HOME") != NULL) {
		PATH_CONF = (string)getenv("XDG_CONFIG_HOME") + "/flare/";
		createDir(PATH_CONF);
		PATH_CONF += GAME_FOLDER + "/";
		createDir(PATH_CONF);
	}
	// $HOME/.config/flare/
	else if (getenv("HOME") != NULL) {
		PATH_CONF = (string)getenv("HOME") + "/.config/";
		createDir(PATH_CONF);
		PATH_CONF += "flare/";
		createDir(PATH_CONF);
		PATH_CONF += GAME_FOLDER + "/";
		createDir(PATH_CONF);
	}
	// ./config/
	else {
		PATH_CONF = "./config/";
		createDir(PATH_CONF);
	}

	// set user path (save games)
	// $XDG_DATA_HOME/flare/
	if (getenv("XDG_DATA_HOME") != NULL) {
		PATH_USER = (string)getenv("XDG_DATA_HOME") + "/flare/";
		createDir(PATH_USER);
		PATH_DEFAULT_USER = PATH_USER + DEFAULT_FOLDER + "/";
		PATH_USER += GAME_FOLDER + "/";
		createDir(PATH_USER);
		createDir(PATH_USER + "mods/");
		createDir(PATH_DEFAULT_USER);
		createDir(PATH_DEFAULT_USER + "mods/");
	}
	// $HOME/.local/share/flare/
	else if (getenv("HOME") != NULL) {
		PATH_USER = (string)getenv("HOME") + "/.local/";
		createDir(PATH_USER);
		PATH_USER += "share/";
		createDir(PATH_USER);
		PATH_USER += "flare/";
		createDir(PATH_USER);
		PATH_DEFAULT_USER = PATH_USER + DEFAULT_FOLDER + "/";
		PATH_USER += GAME_FOLDER + "/";
		createDir(PATH_USER);
		createDir(PATH_USER + "mods/");
		createDir(PATH_DEFAULT_USER);
		createDir(PATH_DEFAULT_USER + "mods/");
	}
	// ./saves/
	else {
		PATH_USER = "./saves/";
		createDir(PATH_USER);
		PATH_DEFAULT_USER = "./saves/";
		createDir(PATH_DEFAULT_USER);
	}

	// data folder
	// while PATH_CONF and PATH_USER are created if not found,
	// PATH_DATA must already have the game data for the game to work.
	// in most releases the data will be in the same folder as the executable
	// - Windows apps are released as a simple folder
	// - OSX apps are released in a .app folder
	// Official linux distros might put the executable and data files
	// in a more standard location.

	// these flags are set to true when a valid directory is found
	bool path_data = false;
	bool path_default_data = false;

	// if the user specified a data path, try to use it
	if (dirExists(CUSTOM_PATH_DATA)) {
		if (!path_data) PATH_DATA = CUSTOM_PATH_DATA;
		if (!path_default_data) PATH_DEFAULT_DATA = CUSTOM_PATH_DATA;
		path_data = path_default_data = true;
	}
	else if (!CUSTOM_PATH_DATA.empty()) fprintf(stderr, "Error: Could not find specified game data directory.\n");

	// Check for the local data before trying installed ones.
	if (dirExists("./mods")) {
		if (!path_data) PATH_DATA = "./";
		if (!path_default_data) PATH_DEFAULT_DATA = "./";
		path_data = path_default_data = true;
	}

	// check $XDG_DATA_DIRS options
	// a list of directories in preferred order separated by :
	if (getenv("XDG_DATA_DIRS") != NULL) {
		string pathlist = (string)getenv("XDG_DATA_DIRS");
		string pathtest;
		pathtest = eatFirstString(pathlist,':');
		while (pathtest != "") {
			if (!path_data) {
				PATH_DATA = pathtest + "/flare/" + GAME_FOLDER + "/";
				if (dirExists(PATH_DATA)) path_data = true;
			}
			if (!path_default_data) {
				PATH_DEFAULT_DATA = pathtest + "/flare/" + DEFAULT_FOLDER + "/";
				if (dirExists(PATH_DEFAULT_DATA)) path_default_data = true;
			}
			if (path_data && path_default_data) break;
			pathtest = eatFirstString(pathlist,':');
		}
	}

#if defined DATA_INSTALL_DIR
	if (!path_data) PATH_DATA = DATA_INSTALL_DIR "/" + GAME_FOLDER + "/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;
	if (!path_default_data) PATH_DEFAULT_DATA = DATA_INSTALL_DIR "/" + DEFAULT_FOLDER + "/";
	if (!path_default_data && dirExists(PATH_DEFAULT_DATA)) path_default_data = true;
#endif

	// check /usr/local/share/flare/ and /usr/share/flare/ next
	if (!path_data) PATH_DATA = "/usr/local/share/flare/" + GAME_FOLDER + "/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;
	if (!path_default_data) PATH_DEFAULT_DATA = "/usr/local/share/flare/" + DEFAULT_FOLDER + "/";
	if (!path_default_data && dirExists(PATH_DEFAULT_DATA)) path_default_data = true;

	if (!path_data) PATH_DATA = "/usr/share/flare/" + GAME_FOLDER + "/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;
	if (!path_default_data) PATH_DEFAULT_DATA = "/usr/share/flare/" + DEFAULT_FOLDER + "/";
	if (!path_default_data && dirExists(PATH_DEFAULT_DATA)) path_default_data = true;

	// check "games" variants of these
	if (!path_data) PATH_DATA = "/usr/local/share/games/flare/" + GAME_FOLDER + "/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;
	if (!path_default_data) PATH_DEFAULT_DATA = "/usr/local/share/games/flare/" + DEFAULT_FOLDER + "/";
	if (!path_default_data && dirExists(PATH_DEFAULT_DATA)) path_default_data = true;

	if (!path_data) PATH_DATA = "/usr/share/games/flare/" + GAME_FOLDER + "/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;
	if (!path_default_data) PATH_DEFAULT_DATA = "/usr/share/games/flare/" + DEFAULT_FOLDER + "/";
	if (!path_default_data && dirExists(PATH_DEFAULT_DATA)) path_default_data = true;

	// finally assume the local folder
	if (!path_data)	PATH_DATA = "./";
	if (!path_default_data) PATH_DEFAULT_DATA = "./";
}
#endif

static ConfigEntry * getConfigEntry(const char * name) {

	for (int i = 0; i < config_size; i++) {
		if (std::strcmp(config[i].name, name) == 0) return config + i;
	}
	return NULL;
}

static ConfigEntry * getConfigEntry(const std::string & name) {
	return getConfigEntry(name.c_str());
}

void loadTilesetSettings() {
	// reset defaults
	UNITS_PER_PIXEL_X = 2;
	UNITS_PER_PIXEL_Y = 4;
	TILE_W = 64;
	TILE_H = 32;
	TILE_W_HALF = TILE_W/2;
	TILE_H_HALF = TILE_H/2;
	TILESET_ISOMETRIC = 0;
	TILESET_ORTHOGONAL = 1;
	TILESET_ORIENTATION = TILESET_ISOMETRIC;

	FileParser infile;
	// load tileset settings from engine config
	if (infile.open("engine/tileset_config.txt", true, true, "Unable to open engine/tileset_config.txt! Defaulting to 64x32 isometric tiles.\n")) {
		while (infile.next()) {
			if (infile.key == "tile_size") {
				TILE_W = toInt(infile.nextValue());
				TILE_H = toInt(infile.nextValue());
				TILE_W_HALF = TILE_W /2;
				TILE_H_HALF = TILE_H /2;
			}
			else if (infile.key == "orientation") {
				if (infile.val == "isometric")
					TILESET_ORIENTATION = TILESET_ISOMETRIC;
				else if (infile.val == "orthogonal")
					TILESET_ORIENTATION = TILESET_ORTHOGONAL;
			}
		}
		infile.close();
	}

	// Init automatically calculated parameters
	VIEW_W_HALF = VIEW_W / 2;
	VIEW_H_HALF = VIEW_H / 2;
	if (TILESET_ORIENTATION == TILESET_ISOMETRIC) {
		if (TILE_W > 0 && TILE_H > 0) {
			UNITS_PER_PIXEL_X = 2.0f / TILE_W;
			UNITS_PER_PIXEL_Y = 2.0f / TILE_H;
		}
		else {
			fprintf(stderr, "Error: Tile dimensions must be greater than 0. Resetting to the default size of 64x32.\n");
			TILE_W = 64;
			TILE_H = 32;
		}
	}
	else { // TILESET_ORTHOGONAL
		if (TILE_W > 0 && TILE_H > 0) {
			UNITS_PER_PIXEL_X = 1.0f / TILE_W;
			UNITS_PER_PIXEL_Y = 1.0f / TILE_H;
		}
		else {
			fprintf(stderr, "Error: Tile dimensions must be greater than 0. Resetting to the default size of 64x32.\n");
			TILE_W = 64;
			TILE_H = 32;
		}
	}
	if (UNITS_PER_PIXEL_X == 0 || UNITS_PER_PIXEL_Y == 0) {
		fprintf(stderr, "One of UNITS_PER_PIXEL values is zero! %dx%d\n", (int)UNITS_PER_PIXEL_X, (int)UNITS_PER_PIXEL_Y);
		SDL_Quit();
		exit(1);
	}
}

void loadMiscSettings() {
	// reset to defaults
	ELEMENTS.clear();
	EQUIP_FLAGS.clear();
	HERO_CLASSES.clear();
	FRAME_W = 0;
	FRAME_H = 0;
	ICON_SIZE = 0;
	AUTOPICKUP_CURRENCY = false;
	MAX_ABSORB = 90;
	MAX_RESIST = 90;
	MAX_BLOCK = 100;
	MAX_AVOIDANCE = 99;
	MIN_ABSORB = 0;
	MIN_RESIST = 0;
	MIN_BLOCK = 0;
	MIN_AVOIDANCE = 0;
	CURRENCY = "Gold";
	VENDOR_RATIO = 0.25;
	DEATH_PENALTY = true;
	DEATH_PENALTY_PERMADEATH = false;
	DEATH_PENALTY_CURRENCY = 50;
	DEATH_PENALTY_XP = 0;
	DEATH_PENALTY_XP_CURRENT = 0;
	DEATH_PENALTY_ITEM = false;
	MENUS_PAUSE = false;
	SAVE_HPMP = false;
	ENABLE_PLAYGAME = false;
	CORPSE_TIMEOUT = 1800;
	SELL_WITHOUT_VENDOR = true;
	AIM_ASSIST = 0;
	WINDOW_TITLE = "Flare";
	SOUND_FALLOFF = 15;
	PARTY_EXP_PERCENTAGE = 100;
	ENABLE_ALLY_COLLISION_AI = true;
	ENABLE_ALLY_COLLISION = true;
	CURRENCY_ID = 1;
	INTERACT_RANGE = 3;

	FileParser infile;
	// load miscellaneous settings from engine config
	// misc.txt
	if (infile.open("engine/misc.txt")) {
		while (infile.next()) {
			if (infile.key == "save_hpmp")
				SAVE_HPMP = toBool(infile.val);
			else if (infile.key == "corpse_timeout")
				CORPSE_TIMEOUT = toInt(infile.val);
			else if (infile.key == "sell_without_vendor")
				SELL_WITHOUT_VENDOR = toBool(infile.val);
			else if (infile.key == "aim_assist")
				AIM_ASSIST = toInt(infile.val);
			else if (infile.key == "window_title")
				WINDOW_TITLE = infile.val;
			else if (infile.key == "game_prefix")
				GAME_PREFIX = infile.val;
			else if (infile.key == "sound_falloff")
				SOUND_FALLOFF = toInt(infile.val);
			else if (infile.key == "party_exp_percentage")
				PARTY_EXP_PERCENTAGE = toInt(infile.val);
			else if (infile.key == "enable_ally_collision")
				ENABLE_ALLY_COLLISION = toBool(infile.val);
			else if (infile.key == "enable_ally_collision_ai")
				ENABLE_ALLY_COLLISION_AI = toBool(infile.val);
			else if (infile.key == "currency_id") {
				CURRENCY_ID = toInt(infile.val);
				if (CURRENCY_ID < 1) {
					CURRENCY_ID = 1;
					fprintf(stderr, "Currency ID below the minimum allowed value. Resetting it to %d\n", CURRENCY_ID);
				}
			}
			else if (infile.key == "interact_range")
				INTERACT_RANGE = toFloat(infile.val);
			else if (infile.key == "menus_pause")
				MENUS_PAUSE = toBool(infile.val);

		}
		infile.close();
	}
	// resolutions.txt
	if (infile.open("engine/resolutions.txt")) {
		while (infile.next()) {
			if (infile.key == "menu_frame_width")
				FRAME_W = toInt(infile.val);
			else if (infile.key == "menu_frame_height")
				FRAME_H = toInt(infile.val);
			else if (infile.key == "icon_size")
				ICON_SIZE = toInt(infile.val);
			else if (infile.key == "required_width") {
				MIN_VIEW_W = toInt(infile.val);
				if (VIEW_W < MIN_VIEW_W) VIEW_W = MIN_VIEW_W;
				VIEW_W_HALF = VIEW_W/2;
			}
			else if (infile.key == "required_height") {
				MIN_VIEW_H = toInt(infile.val);
				if (VIEW_H < MIN_VIEW_H) VIEW_H = MIN_VIEW_H;
				VIEW_H_HALF = VIEW_H/2;
			}
		}
		infile.close();
	}
	// gameplay.txt
	if (infile.open("engine/gameplay.txt")) {
		while (infile.next()) {
			if (infile.key == "enable_playgame") {
				ENABLE_PLAYGAME = toBool(infile.val);
			}
		}
		infile.close();
	}
	// combat.txt
	if (infile.open("engine/combat.txt")) {
		while (infile.next()) {
			if (infile.key == "max_absorb_percent") MAX_ABSORB = toInt(infile.val);
			else if (infile.key == "max_resist_percent") MAX_RESIST = toInt(infile.val);
			else if (infile.key == "max_block_percent") MAX_BLOCK = toInt(infile.val);
			else if (infile.key == "max_avoidance_percent") MAX_AVOIDANCE = toInt(infile.val);
			else if (infile.key == "min_absorb_percent") MIN_ABSORB = toInt(infile.val);
			else if (infile.key == "min_resist_percent") MIN_RESIST = toInt(infile.val);
			else if (infile.key == "min_block_percent") MIN_BLOCK = toInt(infile.val);
			else if (infile.key == "min_avoidance_percent") MIN_AVOIDANCE = toInt(infile.val);
		}
		infile.close();
	}
	// elements.txt
	if (infile.open("engine/elements.txt")) {
		Element e;
		while (infile.next()) {
			if (infile.key == "name") e.name = infile.val;
			else if (infile.key == "description") e.description = infile.val;

			if (e.name != "" && e.description != "") {
				ELEMENTS.push_back(e);
				e.name = e.description = "";
			}
		}
		infile.close();
	}
	// equip_flags.txt
	if (infile.open("engine/equip_flags.txt", true, false)) {
		string type,description;
		type = description = "";

		while (infile.next()) {
			if (infile.key == "name") type = infile.val;
			else if (infile.key == "description") description = infile.val;

			if (type != "" && description != "") {
				EQUIP_FLAGS[type] = description;
				type = description = "";
			}
		}
		infile.close();
	}
	// classes.txt
	if (infile.open("engine/classes.txt")) {
		HeroClass c;
		while (infile.next()) {
			if (infile.key == "name") c.name = infile.val;

			if (c.name != "") {
				HERO_CLASSES.push_back(c);
				c.name = "";
			}

			if (!HERO_CLASSES.empty()) {
				if (infile.key == "description") HERO_CLASSES.back().description = infile.val;
				else if (infile.key == "currency") HERO_CLASSES.back().currency = toInt(infile.val);
				else if (infile.key == "equipment") HERO_CLASSES.back().equipment = infile.val;
				else if (infile.key == "physical") HERO_CLASSES.back().physical = toInt(infile.val);
				else if (infile.key == "mental") HERO_CLASSES.back().mental = toInt(infile.val);
				else if (infile.key == "offense") HERO_CLASSES.back().offense = toInt(infile.val);
				else if (infile.key == "defense") HERO_CLASSES.back().defense = toInt(infile.val);
				else if (infile.key == "actionbar") {
					for (int i=0; i<12; i++) {
						HERO_CLASSES.back().hotkeys[i] = toInt(infile.nextValue());
					}
				}
				else if (infile.key == "powers") {
					string power;
					while ( (power = infile.nextValue()) != "") {
						HERO_CLASSES.back().powers.push_back(toInt(power));
					}
				}
				else if (infile.key == "campaign") {
					string status;
					while ( (status = infile.nextValue()) != "") {
						HERO_CLASSES.back().statuses.push_back(status);
					}
				}
			}
		}
		infile.close();
	}
	// Make a default hero class if none were found
	if (HERO_CLASSES.empty()) {
		HeroClass c;
		c.name = msg->get("Adventurer");
		HERO_CLASSES.push_back(c);
	}

	// death_penalty.txt
	if (infile.open("engine/death_penalty.txt")) {
		while (infile.next()) {
			if (infile.key == "enable") DEATH_PENALTY = toBool(infile.val);
			else if (infile.key == "permadeath") DEATH_PENALTY_PERMADEATH = toBool(infile.val);
			else if (infile.key == "currency") DEATH_PENALTY_CURRENCY = toInt(infile.val);
			else if (infile.key == "xp_total") DEATH_PENALTY_XP = toInt(infile.val);
			else if (infile.key == "xp_current_level") DEATH_PENALTY_XP_CURRENT = toInt(infile.val);
			else if (infile.key == "random_item") DEATH_PENALTY_ITEM = toBool(infile.val);
		}
		infile.close();
	}
}

bool loadSettings() {

	// init defaults
	for (int i = 0; i < config_size; i++) {
		// TODO: handle errors
		ConfigEntry * entry = config + i;
		tryParseValue(*entry->type, entry->default_val, entry->storage);
	}

	// try read from file
	FileParser infile;
	if (!infile.open(PATH_CONF + FILE_SETTINGS, false, true,  "")) {
		if (!infile.open("engine/default_settings.txt", true, true, "")) {
			saveSettings();
			return true;
		}
		else saveSettings();
	}

	while (infile.next()) {

		ConfigEntry * entry = getConfigEntry(infile.key);
		if (entry) {
			// TODO: handle errors
			tryParseValue(*entry->type, infile.val, entry->storage);
		}
	}
	infile.close();

	return true;
}

/**
 * Save the current main settings (primary video and audio settings)
 */
bool saveSettings() {

	ofstream outfile;
	outfile.open((PATH_CONF + FILE_SETTINGS).c_str(), ios::out);

	if (outfile.is_open()) {

		for (int i = 0; i < config_size; i++) {

			// write additional newline before the next section
			if (i != 0 && config[i].comment != NULL)
				outfile<<"\n";

			if (config[i].comment != NULL) {
				outfile<<"# "<<config[i].comment<<"\n";
			}
			outfile<<config[i].name<<"="<<toString(*config[i].type, config[i].storage)<<"\n";
		}

		if (outfile.bad()) fprintf(stderr, "Unable to write settings file. No write access or disk is full!\n");
		outfile.close();
		outfile.clear();
	}
	return true;
}

/**
 * Load all default settings, except video settings.
 */
bool loadDefaults() {

	// HACK init defaults except video
	for (int i = 3; i < config_size; i++) {
		// TODO: handle errors
		ConfigEntry * entry = config + i;
		tryParseValue(*entry->type, entry->default_val, entry->storage);
	}

	// Init automatically calculated parameters
	VIEW_W_HALF = VIEW_W / 2;
	VIEW_H_HALF = VIEW_H / 2;

	return true;
}
