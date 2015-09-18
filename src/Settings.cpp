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
#include <iomanip>

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

#if defined(__ANDROID__) || defined (__IPHONEOS__)
#include <SDL.h>
#endif

class ConfigEntry {
public:
	const char * name;
	const std::type_info * type;
	const char * default_val;
	void * storage;
	const char * comment;
};

ConfigEntry config[] = {
	{ "fullscreen",       &typeid(FULLSCREEN),      "0",   &FULLSCREEN,      "fullscreen mode. 1 enable, 0 disable."},
	{ "resolution_w",     &typeid(SCREEN_W),        "640", &SCREEN_W,        "display resolution. 640x480 minimum."},
	{ "resolution_h",     &typeid(SCREEN_H),        "480", &SCREEN_H,        NULL},
	{ "music_volume",     &typeid(MUSIC_VOLUME),    "96",  &MUSIC_VOLUME,    "music and sound volume (0 = silent, 128 = max)"},
	{ "sound_volume",     &typeid(SOUND_VOLUME),    "128", &SOUND_VOLUME,    NULL},
	{ "combat_text",      &typeid(COMBAT_TEXT),     "0",   &COMBAT_TEXT,     "display floating damage text. 1 enable, 0 disable."},
	{ "mouse_move",       &typeid(MOUSE_MOVE),      "0",   &MOUSE_MOVE,      "use mouse to move (experimental). 1 enable, 0 disable."},
	{ "hwsurface",        &typeid(HWSURFACE),       "1",   &HWSURFACE,       "hardware surfaces, v-sync. Try disabling for performance. 1 enable, 0 disable."},
	{ "vsync",            &typeid(VSYNC),           "1",   &VSYNC,           NULL},
	{ "texture_filter",   &typeid(TEXTURE_FILTER),  "1",   &TEXTURE_FILTER,  "texture filter quality. 0 nearest neighbor (worst), 1 linear (best)"},
	{ "enable_joystick",  &typeid(ENABLE_JOYSTICK), "0",   &ENABLE_JOYSTICK, "joystick settings."},
	{ "joystick_device",  &typeid(JOYSTICK_DEVICE), "0",   &JOYSTICK_DEVICE, NULL},
	{ "joystick_deadzone",&typeid(JOY_DEADZONE),    "100", &JOY_DEADZONE,    NULL},
	{ "language",         &typeid(LANGUAGE),        "en",  &LANGUAGE,        "2-letter language code."},
	{ "change_gamma",     &typeid(CHANGE_GAMMA),    "0",   &CHANGE_GAMMA,    "allow changing gamma (experimental). 1 enable, 0 disable."},
	{ "gamma",            &typeid(GAMMA),           "1.0", &GAMMA,           "screen gamma (0.5 = darkest, 2.0 = lightest)"},
	{ "mouse_aim",        &typeid(MOUSE_AIM),       "1",   &MOUSE_AIM,       "use mouse to aim. 1 enable, 0 disable."},
	{ "no_mouse",         &typeid(NO_MOUSE),        "0",   &NO_MOUSE,        "make using mouse secondary, give full control to keyboard. 1 enable, 0 disable."},
	{ "show_fps",         &typeid(SHOW_FPS),        "0",   &SHOW_FPS,        "show frames per second. 1 enable, 0 disable."},
	{ "colorblind",       &typeid(COLORBLIND),      "0",   &COLORBLIND,      "enable colorblind tooltips. 1 enable, 0 disable"},
	{ "hardware_cursor",  &typeid(HARDWARE_CURSOR), "0",   &HARDWARE_CURSOR, "use the system mouse cursor. 1 enable, 0 disable"},
	{ "dev_mode",         &typeid(DEV_MODE),        "0",   &DEV_MODE,        "allow opening the developer console. 1 enable, 0 disable"},
	{ "dev_hud",          &typeid(DEV_HUD),         "1",   &DEV_HUD,         "shows some additional information on-screen when developer mode is enabled. 1 enable, 0 disable"},
	{ "show_target",      &typeid(SHOW_TARGET),     "0",   &SHOW_TARGET,     "show the targeting reticle on the ground when attacking. 1 enable, 0 disable"},
	{ "loot_tooltips",    &typeid(LOOT_TOOLTIPS),   "1",   &LOOT_TOOLTIPS,   "always show loot tooltips. 1 enable, 0 disable"}
};
const int config_size = sizeof(config) / sizeof(ConfigEntry);

// Paths
std::string PATH_CONF = "";
std::string PATH_USER = "";
std::string PATH_DATA = "";
std::string CUSTOM_PATH_DATA = "";

// Filenames
std::string FILE_SETTINGS	= "settings.txt";
std::string FILE_KEYBINDINGS = "keybindings.txt";

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
unsigned short MAX_FRAMES_PER_SEC = 60;
unsigned char BITS_PER_PIXEL = 32;
unsigned short VIEW_W = 0;
unsigned short VIEW_H = 0;
unsigned short VIEW_W_HALF = 0;
unsigned short VIEW_H_HALF = 0;
short MIN_SCREEN_W = 640;
short MIN_SCREEN_H = 480;
unsigned short SCREEN_W = 640;
unsigned short SCREEN_H = 480;
bool VSYNC;
bool HWSURFACE;
bool TEXTURE_FILTER;
bool IGNORE_TEXTURE_FILTER = false;
bool CHANGE_GAMMA;
float GAMMA;

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
bool SHOW_TARGET;
bool LOOT_TOOLTIPS;

// Input Settings
bool MOUSE_MOVE;
bool ENABLE_JOYSTICK;
int JOYSTICK_DEVICE;
bool MOUSE_AIM;
bool NO_MOUSE;
int JOY_DEADZONE;
bool TOUCHSCREEN = false;

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
std::vector<EquipFlag> EQUIP_FLAGS;

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

// Tooltip settings
int TOOLTIP_OFFSET;
int TOOLTIP_WIDTH;
int TOOLTIP_MARGIN;
int TOOLTIP_MARGIN_NPC;

// Other Settings
bool MENUS_PAUSE;
bool SAVE_HPMP;
bool ENABLE_PLAYGAME;
int CORPSE_TIMEOUT;
bool SELL_WITHOUT_VENDOR;
int AIM_ASSIST;
std::string SAVE_PREFIX = "";
std::string WINDOW_TITLE;
int SOUND_FALLOFF;
int PARTY_EXP_PERCENTAGE;
bool ENABLE_ALLY_COLLISION_AI;
bool ENABLE_ALLY_COLLISION;
int CURRENCY_ID;
float INTERACT_RANGE;
bool SAVE_ONLOAD = true;
bool SAVE_ONEXIT = true;
float ENCOUNTER_DIST;

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
	if (getenv("APPDATA") != NULL) {
		PATH_CONF = PATH_USER = (std::string)getenv("APPDATA") + "\\flare";
		createDir(PATH_CONF);
		createDir(PATH_USER);

		PATH_CONF += "\\config";
		PATH_USER += "\\userdata";
		createDir(PATH_CONF);
		createDir(PATH_USER);
	}
	else {
		PATH_CONF = "config";
		PATH_USER = "userdata";
		createDir(PATH_CONF);
		createDir(PATH_USER);
	}

	createDir(PATH_USER + "\\mods");
	createDir(PATH_USER + "\\saves");

	PATH_DATA = "";
	if (dirExists(CUSTOM_PATH_DATA)) PATH_DATA = CUSTOM_PATH_DATA;
	else if (!CUSTOM_PATH_DATA.empty()) logError("Settings: Could not find specified game data directory.");

	PATH_CONF = PATH_CONF + "/";
	PATH_USER = PATH_USER + "/";
}
#elif __ANDROID__
// Android paths
void setPaths() {

	PATH_CONF = std::string(SDL_AndroidGetInternalStoragePath()) + "/config";
	PATH_USER = std::string(SDL_AndroidGetInternalStoragePath()) + "/userdata";
	createDir(PATH_CONF);
	createDir(PATH_USER);
	createDir(PATH_USER + "/mods");
	createDir(PATH_USER + "/saves");

	std::string mods_folder = "data/org.flare.app/files";

	if (SDL_AndroidGetExternalStorageState() != 0)
	{
		PATH_DATA = std::string(SDL_AndroidGetExternalStoragePath());
	}
	else if (dirExists("/sdcard/Android"))
	{
		PATH_DATA = "/sdcard/Android/" + mods_folder;
	}
	else if (dirExists("/mnt/sdcard/Android"))
	{
		PATH_DATA = "/mnt/sdcard/Android/" + mods_folder;
	}
	else if (dirExists("storage/sdcard0/Android"))
	{
		PATH_DATA = "/storage/sdcard0/Android/" + mods_folder;
	}
	else if (dirExists("/storage/emulated/0/Android"))
	{
		PATH_DATA = "/storage/emulated/0/Android/" + mods_folder;
	}
	else if (dirExists("/storage/emulated/legacy/Android"))
	{
		PATH_DATA = "/storage/emulated/legacy/Android/" + mods_folder;
	}
	else
	{
		logError("Settings: Android external storage unavailable: %s", SDL_GetError());
	}

	PATH_CONF = PATH_CONF + "/";
	PATH_USER = PATH_USER + "/";
	PATH_DATA = PATH_DATA + "/";
}
#else
void setPaths() {

	// attempting to follow this spec:
	// http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

	// set config path (settings, keybindings)
	// $XDG_CONFIG_HOME/flare/
	if (getenv("XDG_CONFIG_HOME") != NULL) {
		PATH_CONF = (std::string)getenv("XDG_CONFIG_HOME") + "/flare/";
	}
	// $HOME/.config/flare/
	else if (getenv("HOME") != NULL) {
		PATH_CONF = (std::string)getenv("HOME") + "/.config/";
		createDir(PATH_CONF);
		PATH_CONF += "flare/";
	}
	// ./config/
	else {
		PATH_CONF = "./config/";
	}

	createDir(PATH_CONF);

	// set user path (save games)
	// $XDG_DATA_HOME/flare/
	if (getenv("XDG_DATA_HOME") != NULL) {
		PATH_USER = (std::string)getenv("XDG_DATA_HOME") + "/flare/";
	}
	// $HOME/.local/share/flare/
	else if (getenv("HOME") != NULL) {
		PATH_USER = (std::string)getenv("HOME") + "/.local/";
		createDir(PATH_USER);
		PATH_USER += "share/";
		createDir(PATH_USER);
		PATH_USER += "flare/";
	}
	// ./saves/
	else {
		PATH_USER = "./userdata/";
	}

	createDir(PATH_USER);
	createDir(PATH_USER + "mods/");
	createDir(PATH_USER + "saves/");

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

	// if the user specified a data path, try to use it
	if (dirExists(CUSTOM_PATH_DATA)) {
		if (!path_data) PATH_DATA = CUSTOM_PATH_DATA;
		path_data = true;
	}
	else if (!CUSTOM_PATH_DATA.empty()) logError("Settings: Could not find specified game data directory.");

	// Check for the local data before trying installed ones.
	if (dirExists("./mods")) {
		if (!path_data) PATH_DATA = "./";
		path_data = true;
	}

	// check $XDG_DATA_DIRS options
	// a list of directories in preferred order separated by :
	if (getenv("XDG_DATA_DIRS") != NULL) {
		std::string pathlist = (std::string)getenv("XDG_DATA_DIRS");
		std::string pathtest;
		pathtest = popFirstString(pathlist,':');
		while (pathtest != "") {
			if (!path_data) {
				PATH_DATA = pathtest + "/flare/";
				if (dirExists(PATH_DATA)) path_data = true;
			}
			if (path_data) break;
			pathtest = popFirstString(pathlist,':');
		}
	}

#if defined DATA_INSTALL_DIR
	if (!path_data) PATH_DATA = DATA_INSTALL_DIR "/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;
#endif

	// check /usr/local/share/flare/ and /usr/share/flare/ next
	if (!path_data) PATH_DATA = "/usr/local/share/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	if (!path_data) PATH_DATA = "/usr/share/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	// check "games" variants of these
	if (!path_data) PATH_DATA = "/usr/local/share/games/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	if (!path_data) PATH_DATA = "/usr/share/games/flare/";
	if (!path_data && dirExists(PATH_DATA)) path_data = true;

	// finally assume the local folder
	if (!path_data)	PATH_DATA = "./";
}
#endif

static ConfigEntry * getConfigEntry(const char * name) {

	for (int i = 0; i < config_size; i++) {
		if (std::strcmp(config[i].name, name) == 0) return config + i;
	}

	logError("Settings: '%s' is not a valid configuration key.", name);
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
	// @CLASS Settings: Tileset config|Description of engine/tileset_config.txt
	if (infile.open("engine/tileset_config.txt", true, "Unable to open engine/tileset_config.txt! Defaulting to 64x32 isometric tiles.")) {
		while (infile.next()) {
			if (infile.key == "tile_size") {
				// @ATTR tile_size|w (integet), h (integer)|The width and height of a tile.
				TILE_W = static_cast<unsigned short>(toInt(infile.nextValue()));
				TILE_H = static_cast<unsigned short>(toInt(infile.nextValue()));
				TILE_W_HALF = TILE_W /2;
				TILE_H_HALF = TILE_H /2;
			}
			else if (infile.key == "orientation") {
				// @ATTR orientation|[isometric, orthogonal]|The perspective of tiles; isometric or orthogonal.
				if (infile.val == "isometric")
					TILESET_ORIENTATION = TILESET_ISOMETRIC;
				else if (infile.val == "orthogonal")
					TILESET_ORIENTATION = TILESET_ORTHOGONAL;
			}
			else {
				infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// Init automatically calculated parameters
	if (TILESET_ORIENTATION == TILESET_ISOMETRIC) {
		if (TILE_W > 0 && TILE_H > 0) {
			UNITS_PER_PIXEL_X = 2.0f / TILE_W;
			UNITS_PER_PIXEL_Y = 2.0f / TILE_H;
		}
		else {
			logError("Settings: Tile dimensions must be greater than 0. Resetting to the default size of 64x32.");
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
			logError("Settings: Tile dimensions must be greater than 0. Resetting to the default size of 64x32.");
			TILE_W = 64;
			TILE_H = 32;
		}
	}
	if (UNITS_PER_PIXEL_X == 0 || UNITS_PER_PIXEL_Y == 0) {
		logError("Settings: One of UNITS_PER_PIXEL values is zero! %dx%d", static_cast<int>(UNITS_PER_PIXEL_X), static_cast<int>(UNITS_PER_PIXEL_Y));
		Exit(1);
	}
}

void loadMiscSettings() {
	// reset to defaults
	ELEMENTS.clear();
	EQUIP_FLAGS.clear();
	HERO_CLASSES.clear();
	FRAME_W = 0;
	FRAME_H = 0;
	IGNORE_TEXTURE_FILTER = false;
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
	CORPSE_TIMEOUT = 60*MAX_FRAMES_PER_SEC;
	SELL_WITHOUT_VENDOR = true;
	AIM_ASSIST = 0;
	SAVE_PREFIX = "";
	WINDOW_TITLE = "Flare";
	SOUND_FALLOFF = 15;
	PARTY_EXP_PERCENTAGE = 100;
	ENABLE_ALLY_COLLISION_AI = true;
	ENABLE_ALLY_COLLISION = true;
	CURRENCY_ID = 1;
	INTERACT_RANGE = 3;
	SAVE_ONLOAD = true;
	SAVE_ONEXIT = true;
	TOOLTIP_OFFSET = 0;
	TOOLTIP_WIDTH = 1;
	TOOLTIP_MARGIN = 0;
	TOOLTIP_MARGIN_NPC = 0;

	FileParser infile;
	// @CLASS Settings: Misc|Description of engine/misc.txt
	if (infile.open("engine/misc.txt")) {
		while (infile.next()) {
			// @ATTR save_hpmp|boolean|When saving the game, keep the hero's current HP and MP.
			if (infile.key == "save_hpmp")
				SAVE_HPMP = toBool(infile.val);
			// @ATTR corpse_timeout|duration|Duration that a corpse can exist on the map in 'ms' or 's'.
			else if (infile.key == "corpse_timeout")
				CORPSE_TIMEOUT = parse_duration(infile.val);
			// @ATTR sell_without_vendor|boolean|Allows selling items when not at a vendor via CTRL-Click.
			else if (infile.key == "sell_without_vendor")
				SELL_WITHOUT_VENDOR = toBool(infile.val);
			// @ATTR aim_assist|integer|The pixel offset for powers that use aim_assist.
			else if (infile.key == "aim_assist")
				AIM_ASSIST = toInt(infile.val);
			// @ATTR window_title|string|Sets the text in the window's titlebar.
			else if (infile.key == "window_title")
				WINDOW_TITLE = infile.val;
			// @ATTR save_prefix|string|A string that's prepended to save filenames to prevent conflicts between mods.
			else if (infile.key == "save_prefix")
				SAVE_PREFIX = infile.val;
			// @ATTR sound_falloff|integer|The maximum radius in tiles that any single sound is audible.
			else if (infile.key == "sound_falloff")
				SOUND_FALLOFF = toInt(infile.val);
			// @ATTR party_exp_percentage|integer|The percentage of XP given to allies.
			else if (infile.key == "party_exp_percentage")
				PARTY_EXP_PERCENTAGE = toInt(infile.val);
			// @ATTR enable_ally_collision|boolean|Allows allies to block the player's path.
			else if (infile.key == "enable_ally_collision")
				ENABLE_ALLY_COLLISION = toBool(infile.val);
			// @ATTR enable_ally_collision_ai|boolean|Allows allies to block the path of other AI creatures.
			else if (infile.key == "enable_ally_collision_ai")
				ENABLE_ALLY_COLLISION_AI = toBool(infile.val);
			else if (infile.key == "currency_id") {
				// @ATTR currency_id|integer|An item id that will be used as currency.
				CURRENCY_ID = toInt(infile.val);
				if (CURRENCY_ID < 1) {
					CURRENCY_ID = 1;
					logError("Settings: Currency ID below the minimum allowed value. Resetting it to %d", CURRENCY_ID);
				}
			}
			// @ATTR interact_range|float|Distance where the player can interact with objects and NPCs.
			else if (infile.key == "interact_range")
				INTERACT_RANGE = toFloat(infile.val);
			// @ATTR menus_pause|boolean|Opening any menu will pause the game.
			else if (infile.key == "menus_pause")
				MENUS_PAUSE = toBool(infile.val);
			// @ATTR save_onload|boolean|Save the game upon changing maps.
			else if (infile.key == "save_onload")
				SAVE_ONLOAD = toBool(infile.val);
			// @ATTR save_onexit|boolean|Save the game upon quitting to the title screen or desktop.
			else if (infile.key == "save_onexit")
				SAVE_ONEXIT = toBool(infile.val);

			else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	if (SAVE_PREFIX == "") {
		logError("Settings: save_prefix not found in engine/misc.txt, setting to 'default'. This may cause save file conflicts between games that have no save_prefix.");
		SAVE_PREFIX = "default";
	}

	// @CLASS Settings: Resolution|Description of engine/resolutions.txt
	if (infile.open("engine/resolutions.txt")) {
		while (infile.next()) {
			// @ATTR menu_frame_width|integer|Width of frame for New Game, Configuration, etc. menus.
			if (infile.key == "menu_frame_width")
				FRAME_W = static_cast<unsigned short>(toInt(infile.val));
			// @ATTR menu_frame_height|integer|Height of frame for New Game, Configuration, etc. menus.
			else if (infile.key == "menu_frame_height")
				FRAME_H = static_cast<unsigned short>(toInt(infile.val));
			// @ATTR icon_size|integer|Size of icons.
			else if (infile.key == "icon_size")
				ICON_SIZE = static_cast<unsigned short>(toInt(infile.val));
			// @ATTR required_width|integer|Minimum window/screen resolution width.
			else if (infile.key == "required_width") {
				MIN_SCREEN_W = static_cast<unsigned short>(toInt(infile.val));
			}
			// @ATTR required_height|integer|Minimum window/screen resolution height.
			else if (infile.key == "required_height") {
				MIN_SCREEN_H = static_cast<unsigned short>(toInt(infile.val));
			}
			// @ATTR virtual_height|integer|The height (in pixels) of the game's actual rendering area. The width will be resized to match the window's aspect ration, and everything will be scaled up to fill the window.
			else if (infile.key == "virtual_height") {
				VIEW_H = static_cast<unsigned short>(toInt(infile.val));
				VIEW_H_HALF = VIEW_H / 2;
			}
			// @ATTR ignore_texture_filter|boolean|If true, this ignores the "Texture Filtering" video setting and uses only nearest-neighbor scaling. This is good for games that use pixel art assets.
			else if (infile.key == "ignore_texture_filter") {
				IGNORE_TEXTURE_FILTER = toBool(infile.val);
			}
			else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// prevent the window from being too small
	if (SCREEN_W < MIN_SCREEN_W) SCREEN_W = MIN_SCREEN_W;
	if (SCREEN_H < MIN_SCREEN_H) SCREEN_H = MIN_SCREEN_H;

	// set the default virtual height if it's not defined
	if (VIEW_H == 0) {
		logError("Settings: virtual_height is undefined. Setting it to %d.", MIN_SCREEN_H);
		VIEW_H = MIN_SCREEN_H;
		VIEW_H_HALF = VIEW_H / 2;
	}

	// @CLASS Settings: Gameplay|Description of engine/gameplay.txt
	if (infile.open("engine/gameplay.txt")) {
		while (infile.next()) {
			if (infile.key == "enable_playgame") {
				// @ATTR enable_playgame|boolean|Enables the "Play Game" button on the main menu.
				ENABLE_PLAYGAME = toBool(infile.val);
			}
			else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// @CLASS Settings: Combat|Description of engine/combat.txt
	if (infile.open("engine/combat.txt")) {
		while (infile.next()) {
			// @ATTR max_absorb_percent|integer|Maximum percentage of damage that can be absorbed.
			if (infile.key == "max_absorb_percent") MAX_ABSORB = static_cast<short>(toInt(infile.val));
			// @ATTR max_resist_percent|integer|Maximum percentage of elemental damage that can be resisted.
			else if (infile.key == "max_resist_percent") MAX_RESIST = static_cast<short>(toInt(infile.val));
			// @ATTR max_block_percent|integer|Maximum percentage of damage that can be blocked.
			else if (infile.key == "max_block_percent") MAX_BLOCK = static_cast<short>(toInt(infile.val));
			// @ATTR max_avoidance_percent|integer|Maximum percentage chance that hazards can be avoided.
			else if (infile.key == "max_avoidance_percent") MAX_AVOIDANCE = static_cast<short>(toInt(infile.val));
			// @ATTR min_absorb_percent|integer|Minimum percentage of damage that can be absorbed.
			else if (infile.key == "min_absorb_percent") MIN_ABSORB = static_cast<short>(toInt(infile.val));
			// @ATTR min_resist_percent|integer|Minimum percentage of elemental damage that can be resisted.
			else if (infile.key == "min_resist_percent") MIN_RESIST = static_cast<short>(toInt(infile.val));
			// @ATTR min_block_percent|integer|Minimum percentage of damage that can be blocked.
			else if (infile.key == "min_block_percent") MIN_BLOCK = static_cast<short>(toInt(infile.val));
			// @ATTR min_avoidance_percent|integer|Minimum percentage chance that hazards can be avoided.
			else if (infile.key == "min_avoidance_percent") MIN_AVOIDANCE = static_cast<short>(toInt(infile.val));

			else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// @CLASS Settings: Elements|Description of engine/elements.txt
	if (infile.open("engine/elements.txt")) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "element") {
					// check if the previous element and remove it if there is no identifier
					if (!ELEMENTS.empty() && ELEMENTS.back().id == "") {
						ELEMENTS.pop_back();
					}
					ELEMENTS.resize(ELEMENTS.size()+1);
				}
			}

			if (ELEMENTS.empty() || infile.section != "element")
				continue;

			// @ATTR element.id|string|An identifier for this element.
			if (infile.key == "id") ELEMENTS.back().id = infile.val;
			// @ATTR element.name|string|The displayed name of this element.
			else if (infile.key == "name") ELEMENTS.back().name = msg->get(infile.val);

			else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();

		// check if the last element and remove it if there is no identifier
		if (!ELEMENTS.empty() && ELEMENTS.back().id == "") {
			ELEMENTS.pop_back();
		}
	}

	// @CLASS Settings: Equip flags|Description of engine/equip_flags.txt
	if (infile.open("engine/equip_flags.txt")) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "flag") {
					// check if the previous flag and remove it if there is no identifier
					if (!EQUIP_FLAGS.empty() && EQUIP_FLAGS.back().id == "") {
						EQUIP_FLAGS.pop_back();
					}
					EQUIP_FLAGS.resize(EQUIP_FLAGS.size()+1);
				}
			}

			if (EQUIP_FLAGS.empty() || infile.section != "flag")
				continue;

			// @ATTR flag.id|string|An identifier for this equip flag.
			if (infile.key == "id") EQUIP_FLAGS.back().id = infile.val;
			// @ATTR flag.name|string|The displayed name of this equip flag.
			else if (infile.key == "name") EQUIP_FLAGS.back().name = infile.val;

			else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();

		// check if the last flag and remove it if there is no identifier
		if (!EQUIP_FLAGS.empty() && EQUIP_FLAGS.back().id == "") {
			EQUIP_FLAGS.pop_back();
		}
	}

	// @CLASS Settings: Classes|Description of engine/classes.txt
	if (infile.open("engine/classes.txt")) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "class") {
					// check if the previous class and remove it if there is no name
					if (!HERO_CLASSES.empty() && HERO_CLASSES.back().name == "") {
						HERO_CLASSES.pop_back();
					}
					HERO_CLASSES.resize(HERO_CLASSES.size()+1);
				}
			}

			if (HERO_CLASSES.empty() || infile.section != "class")
				continue;

			if (!HERO_CLASSES.empty()) {
				// @ATTR name|string|The displayed name of this class.
				if (infile.key == "name") HERO_CLASSES.back().name = infile.val;
				// @ATTR description|string|A description of this class.
				else if (infile.key == "description") HERO_CLASSES.back().description = infile.val;
				// @ATTR currency|integer|The amount of currency this class will start with.
				else if (infile.key == "currency") HERO_CLASSES.back().currency = toInt(infile.val);
				// @ATTR equipment|item (integer), ...|A list of items that are equipped when starting with this class.
				else if (infile.key == "equipment") HERO_CLASSES.back().equipment = infile.val;
				// @ATTR carried|item (integer), ...|A list of items that are placed in the normal inventorty when starting with this class.
				else if (infile.key == "carried") HERO_CLASSES.back().carried = infile.val;
				// @ATTR physical|integer|Class starts with this physical stat.
				else if (infile.key == "physical") HERO_CLASSES.back().physical = toInt(infile.val);
				// @ATTR mental|integer|Class starts with this mental stat.
				else if (infile.key == "mental") HERO_CLASSES.back().mental = toInt(infile.val);
				// @ATTR offense|integer|Class starts with this offense stat.
				else if (infile.key == "offense") HERO_CLASSES.back().offense = toInt(infile.val);
				// @ATTR defense|integer|Class starts with this defense stat.
				else if (infile.key == "defense") HERO_CLASSES.back().defense = toInt(infile.val);

				else if (infile.key == "actionbar") {
					// @ATTR actionbar|power (integer), ...|A list of powers to place in the action bar for the class.
					for (int i=0; i<12; i++) {
						HERO_CLASSES.back().hotkeys[i] = toInt(infile.nextValue());
					}
				}
				else if (infile.key == "powers") {
					// @ATTR powers|power (integer), ...|A list of powers that are unlocked when starting this class.
					std::string power;
					while ( (power = infile.nextValue()) != "") {
						HERO_CLASSES.back().powers.push_back(toInt(power));
					}
				}
				else if (infile.key == "campaign") {
					// @ATTR campaign|status (string), ...|A list of campaign statuses that are set when starting this class.
					std::string status;
					while ( (status = infile.nextValue()) != "") {
						HERO_CLASSES.back().statuses.push_back(status);
					}
				}
				// @ATTR power_tree|string|Power tree that will be loaded by MenuPowers
				else if (infile.key == "power_tree") HERO_CLASSES.back().power_tree = infile.val;

				else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();

		// check if the last class and remove it if there is no name
		if (!HERO_CLASSES.empty() && HERO_CLASSES.back().name == "") {
			HERO_CLASSES.pop_back();
		}
	}
	// Make a default hero class if none were found
	if (HERO_CLASSES.empty()) {
		HeroClass c;
		c.name = "Adventurer";
		msg->get("Adventurer"); // this is needed for translation
		HERO_CLASSES.push_back(c);
	}

	// @CLASS Settings: Death penalty|Description of engine/death_penalty.txt
	if (infile.open("engine/death_penalty.txt")) {
		while (infile.next()) {
			// @ATTR enable|boolean|Enable the death penalty.
			if (infile.key == "enable") DEATH_PENALTY = toBool(infile.val);
			// @ATTR permadeath|boolean|Force permadeath for all new saves.
			else if (infile.key == "permadeath") DEATH_PENALTY_PERMADEATH = toBool(infile.val);
			// @ATTR currency|integer|Remove this percentage of currency.
			else if (infile.key == "currency") DEATH_PENALTY_CURRENCY = toInt(infile.val);
			// @ATTR xp_total|integer|Remove this percentage of total XP.
			else if (infile.key == "xp_total") DEATH_PENALTY_XP = toInt(infile.val);
			// @ATTR xp_current_level|integer|Remove this percentage of the XP gained since the last level.
			else if (infile.key == "xp_current_level") DEATH_PENALTY_XP_CURRENT = toInt(infile.val);
			// @ATTR random_item|integer|Removes a random item from the player's inventory.
			else if (infile.key == "random_item") DEATH_PENALTY_ITEM = toBool(infile.val);

			else infile.error("Settings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// @CLASS Settings: Tooltips|Description of engine/tooltips.txt
	if (infile.open("engine/tooltips.txt")) {
		while (infile.next()) {
			// @ATTR tooltip_offset|integer|Offset in pixels from the origin point (usually mouse cursor).
			if (infile.key == "tooltip_offset")
				TOOLTIP_OFFSET = toInt(infile.val);
			// @ATTR tooltip_width|integer|Maximum width of tooltip in pixels.
			else if (infile.key == "tooltip_width")
				TOOLTIP_WIDTH = toInt(infile.val);
			// @ATTR tooltip_margin|integer|Padding between the text and the tooltip borders.
			else if (infile.key == "tooltip_margin")
				TOOLTIP_MARGIN = toInt(infile.val);
			// @ATTR npc_tooltip_margin|integer|Vertical offset for NPC labels.
			else if (infile.key == "npc_tooltip_margin")
				TOOLTIP_MARGIN_NPC = toInt(infile.val);
		}
		infile.close();
	}

	// @CLASS Settings: Loot|Description of engine/loot.txt
	if (infile.open("engine/loot.txt")) {
		while (infile.next()) {
			if (infile.key == "currency_name") {
				// @ATTR currency_name|string|Define the name of currency in game
				CURRENCY = msg->get(infile.val);
			}
		}
		infile.close();
	}
}

bool loadSettings() {

	// init defaults
	for (int i = 0; i < config_size; i++) {
		ConfigEntry * entry = config + i;
		tryParseValue(*entry->type, entry->default_val, entry->storage);
	}

	// try read from file
	FileParser infile;
	if (!infile.open(PATH_CONF + FILE_SETTINGS, false, "")) {
		loadMobileDefaults();
		if (!infile.open("engine/default_settings.txt", true, "")) {
			saveSettings();
			return true;
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

	return true;
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

		for (int i = 0; i < config_size; i++) {

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
	}
	return true;
}

/**
 * Load all default settings, except video settings.
 */
bool loadDefaults() {

	// HACK init defaults except video
	for (int i = 3; i < config_size; i++) {
		ConfigEntry * entry = config + i;
		tryParseValue(*entry->type, entry->default_val, entry->storage);
	}

	loadMobileDefaults();

	return true;
}

/**
 * Return a string of version name + version number
 */
std::string getVersionString() {
	std::stringstream ss;
	if (VERSION_MAJOR > 0 && VERSION_MINOR < 100 && VERSION_MINOR % 10 == 0)
		ss << VERSION_NAME << " v" << VERSION_MAJOR << "." << VERSION_MINOR/10;
	else
		ss << VERSION_NAME << " v" << VERSION_MAJOR << "." << std::setfill('0') << std::setw(2) << VERSION_MINOR;
	return ss.str();
}

/**
 * Compare version numbers. Returns true if the first number is larger than the second
 */
bool compareVersions(int maj0, int min0, int maj1, int min1) {
	if (maj0 == maj1)
		return min0 > min1;
	else
		return maj0 > maj1;
}

/**
 * Set required settings for Mobile devices
 */
void loadMobileDefaults() {
#if defined(__ANDROID__) || defined (__IPHONEOS__)
	MOUSE_MOVE = true;
	MOUSE_AIM = true;
	NO_MOUSE = false;
	ENABLE_JOYSTICK = false;
	HARDWARE_CURSOR = true;
	TOUCHSCREEN = true;
#endif
}

/**
 * Some variables depend on VIEW_W and VIEW_H. Update them here.
 */
void updateScreenVars() {
	if (TILE_W > 0 && TILE_H > 0) {
		if (TILESET_ORIENTATION == TILESET_ISOMETRIC)
			ENCOUNTER_DIST = sqrtf(powf(static_cast<float>(VIEW_W/TILE_W), 2.f) + powf(static_cast<float>(VIEW_H/TILE_H_HALF), 2.f)) / 2.f;
		else if (TILESET_ORIENTATION == TILESET_ORTHOGONAL)
			ENCOUNTER_DIST = sqrtf(powf(static_cast<float>(VIEW_W/TILE_W), 2.f) + powf(static_cast<float>(VIEW_H/TILE_H), 2.f)) / 2.f;
	}
}
