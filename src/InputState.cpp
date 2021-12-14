/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2012-2015 Justin Jacobs

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
 * class InputState
 *
 * Handles keyboard and mouse states
 */

#include "EngineSettings.h"
#include "FileParser.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "Platform.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "Version.h"

#include <math.h>

InputState::InputState(void)
	: binding()
	, pressing()
	, lock()
	, slow_repeat()
	, repeat_cooldown()
	, done(false)
	, mouse()
	, last_key(-1)
	, last_button(-1)
	, last_joybutton(-1)
	, last_joyaxis(-1)
	, last_is_joystick(false)
	, scroll_up(false)
	, scroll_down(false)
	, lock_scroll(false)
	, touch_locked(false)
	, lock_all(false)
	, window_minimized(false)
	, window_restored(false)
	, window_resized(false)
	, joysticks_changed(false)
	, refresh_hotkeys(false)
	, un_press()
	, current_touch()
	, dump_event(false)
	, file_version(new Version())
	, file_version_min(new Version(1, 12, 38))
{
	config_keys[Input::CANCEL] = "cancel";
	config_keys[Input::ACCEPT] = "accept";
	config_keys[Input::UP] = "up";
	config_keys[Input::DOWN] = "down";
	config_keys[Input::LEFT] = "left";
	config_keys[Input::RIGHT] = "right";
	config_keys[Input::BAR_1] = "bar1";
	config_keys[Input::BAR_2] = "bar2";
	config_keys[Input::BAR_3] = "bar3";
	config_keys[Input::BAR_4] = "bar4";
	config_keys[Input::BAR_5] = "bar5";
	config_keys[Input::BAR_6] = "bar6";
	config_keys[Input::BAR_7] = "bar7";
	config_keys[Input::BAR_8] = "bar8";
	config_keys[Input::BAR_9] = "bar9";
	config_keys[Input::BAR_0] = "bar0";
	config_keys[Input::MAIN1] = "main1";
	config_keys[Input::MAIN2] = "main2";
	config_keys[Input::CHARACTER] = "character";
	config_keys[Input::INVENTORY] = "inventory";
	config_keys[Input::POWERS] = "powers";
	config_keys[Input::LOG] = "log";
	config_keys[Input::ACTIONBAR] = "actionbar";
	config_keys[Input::MENU_PAGE_NEXT] = "menu_page_next";
	config_keys[Input::MENU_PAGE_PREV] = "menu_page_prev";
	config_keys[Input::MENU_ACTIVATE] = "menu_activate";
	config_keys[Input::PAUSE] = "pause";
	config_keys[Input::AIM_UP] = "aim_up";
	config_keys[Input::AIM_DOWN] = "aim_down";
	config_keys[Input::AIM_LEFT] = "aim_left";
	config_keys[Input::AIM_RIGHT] = "aim_right";
	config_keys[Input::DEVELOPER_MENU] = "developer_menu";
}

InputState::~InputState() {
	delete file_version;
	delete file_version_min;
}

/**
 * Key bindings are found in config/keybindings.txt
 */
void InputState::loadKeyBindings() {

	FileParser infile;
	bool opened_file = false;
	*file_version = VersionInfo::MIN;

	// first check for mod keybinds
	if (mods->locate("engine/default_keybindings.txt") != "") {
		if (infile.open(settings->path_user + "saves/" + eset->misc.save_prefix + "/keybindings.txt", !FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
			opened_file = true;
		}
		else if (infile.open("engine/default_keybindings.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
			opened_file = true;
		}
	}
	else {
		// if there are no mod keybinds, fall back to global config
		if (infile.open(settings->path_conf + "keybindings.txt", !FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
			opened_file = true;
		}

		// clean up mod keybinds if engine/default_keybindings.txt is not present
		if (Filesystem::fileExists(settings->path_user + "saves/" + eset->misc.save_prefix + "/keybindings.txt")) {
			Utils::logInfo("InputState: Found unexpected save prefix keybinding file. Removing it now.");
			Filesystem::removeFile(settings->path_user + "saves/" + eset->misc.save_prefix + "/keybindings.txt");
		}
	}

	if (!opened_file) {
		saveKeyBindings();
		return;
	}

	while (infile.next()) {
		if (infile.section.empty()) {
			if (infile.key == "file_version")
				file_version->setFromString(infile.val);

			if (*file_version < *file_version_min) {
				Utils::logError("InputState: Keybindings configuration file is out of date (%s < %s). Resetting to engine defaults.", file_version->getString().c_str(), file_version_min->getString().c_str());
				if (platform.config_menu_type != Platform::CONFIG_MENU_TYPE_BASE) {
					Utils::logErrorDialog("InputState: Keybindings configuration file is out of date. Resetting to engine defaults.", file_version->getString().c_str(), file_version_min->getString().c_str());
				}
				saveKeyBindings();
				break;
			}

			continue;
		}

		if (infile.new_section && infile.section == "user") {
			for (int key = 0; key < KEY_COUNT_USER; ++key) {
				binding[key].clear();
			}
		}

		// @CLASS InputState: Default Keybindings|Description of engine/default_keybindings.txt. Use a bind value of '-1' to clear all bindings for an action. Type may be any of the follwing: 0 = Keyboard, 1 = Mouse, 2 = Gamepad button, 3 = Gamepad Axis. Human-readable key and gamepad mapping names may be used by prefixing the bind with "SDL:" (e.g. "SDL:space" or "SDL:leftstick"). If using the "SDL:" prefix for a gamepad axis, add ":-" to the end of the bind to get the negative direction (e.g. "SDL:leftx:-").
		// @ATTR default.cancel|[int, string], int : Bind, Type|Bindings for "Cancel".
		// @ATTR default.accept|[int, string], int : Bind, Type|Bindings for "Accept".
		// @ATTR default.up|[int, string], int : Bind, Type|Bindings for "Up".
		// @ATTR default.down|[int, string], int : Bind, Type|Bindings for "Down".
		// @ATTR default.left|[int, string], int : Bind, Type|Bindings for "Left".
		// @ATTR default.right|[int, string], int : Bind, Type|Bindings for "Right".
		// @ATTR default.bar1|[int, string], int : Bind, Type|Bindings for "Bar1".
		// @ATTR default.bar2|[int, string], int : Bind, Type|Bindings for "Bar2".
		// @ATTR default.bar3|[int, string], int : Bind, Type|Bindings for "Bar3".
		// @ATTR default.bar4|[int, string], int : Bind, Type|Bindings for "Bar4".
		// @ATTR default.bar5|[int, string], int : Bind, Type|Bindings for "Bar5".
		// @ATTR default.bar6|[int, string], int : Bind, Type|Bindings for "Bar6".
		// @ATTR default.bar7|[int, string], int : Bind, Type|Bindings for "Bar7".
		// @ATTR default.bar8|[int, string], int : Bind, Type|Bindings for "Bar8".
		// @ATTR default.bar9|[int, string], int : Bind, Type|Bindings for "Bar9".
		// @ATTR default.bar0|[int, string], int : Bind, Type|Bindings for "Bar0".
		// @ATTR default.main1|[int, string], int : Bind, Type|Bindings for "Main1".
		// @ATTR default.main2|[int, string], int : Bind, Type|Bindings for "Main2".
		// @ATTR default.character|[int, string], int : Bind, Type|Bindings for "Character".
		// @ATTR default.inventory|[int, string], int : Bind, Type|Bindings for "Inventory".
		// @ATTR default.powers|[int, string], int : Bind, Type|Bindings for "Powers".
		// @ATTR default.log|[int, string], int : Bind, Type|Bindings for "Log".
		// @ATTR default.actionbar|[int, string], int : Bind, Type|Bindings for "Actionbar Accept".
		// @ATTR default.actionbar_back|[int, string], int : Bind, Type|Bindings for "Actionbar Left".
		// @ATTR default.actionbar_forward|[int, string], int : Bind, Type|Bindings for "Actionbar Right".
		// @ATTR default.actionbar_use|[int, string], int : Bind, Type|Bindings for "Actionbar Use".
		// @ATTR default.developer_menu|[int, string], int : Bind, Type|Bindings for "Developer Menu".

		if (infile.section == "user" || infile.section == "default") {
			for (int key = 0; key < KEY_COUNT_USER; ++key) {
				if (infile.key == config_keys[key]) {
					std::string bind = Parse::popFirstString(infile.val);
					int type = Parse::popFirstInt(infile.val);

					InputBind input_bind;
					input_bind.type = type;

					input_bind.bind = getBindFromString(bind, input_bind.type);

					if (input_bind.bind == -1) {
						binding[key].clear();
					}
					else {
						binding[key].push_back(input_bind);
					}
				}
			}
		}
	}
	infile.close();
}

/**
 * Write current key bindings to config file
 */
void InputState::saveKeyBindings() {
	std::string out_path;
	if (mods->locate("engine/default_keybindings.txt") != "") {
		Filesystem::createDir(settings->path_user + "saves/" + eset->misc.save_prefix);
		out_path = settings->path_user + "saves/" + eset->misc.save_prefix + "/keybindings.txt";
	}
	else {
		out_path = settings->path_conf + "keybindings.txt";
	}
	std::ofstream outfile;
	outfile.open(out_path.c_str(), std::ios::out);

	if (outfile.is_open()) {
		outfile << "# Keybindings\n";
		outfile << "# FORMAT: {ACTION}={BIND},{TYPE}\n";
		outfile << "# A bind value of -1 means unbound and will clear any existing bindings for that action\n";
		outfile << "# Type may be any of the follwing: 0 = Keyboard, 1 = Mouse, 2 = Gamepad button, 3 = Gamepad Axis.\n";
		outfile << "# Human-readable key and gamepad mapping names may be used by prefixing the bind with \"SDL:\" (e.g. \"SDL:space\" or \"SDL:leftstick\").\n";
		outfile << "# If using the \"SDL:\" prefix for a gamepad axis, add \":-\" to the end of the bind to get the negative direction (e.g. \"SDL:leftx:-\").\n\n";

		*file_version = *file_version_min;
		outfile << "file_version=" << file_version->getString() << "\n\n";

		outfile << "[user]\n";
		for (int key = 0; key < KEY_COUNT_USER; ++key) {
			if (binding[key].empty()) {
				outfile << config_keys[key] << "=-1\n";
			}
			else {
				for (size_t i = 0; i < binding[key].size(); ++i) {
					outfile << config_keys[key] << "=" << binding[key][i].bind << "," << binding[key][i].type << "\n";
				}
			}
		}

		if (outfile.bad()) Utils::logError("InputState: Unable to write keybindings config file. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		platform.FSCommit();
	}

}

void InputState::handle() {
	refresh_hotkeys = false;

	if (lock_all) return;

	inkeys = "";

	// sometimes buttons are pressed and released in a single event window.
	// in order to properly read these events in game logic, we delay the
	// resetting of their states (done here) until the next frame. this
	// loop also resets the states of other inputs that are no longer being
	// pressed. (joysticks are a little more complex.)
	for (int key=0; key < KEY_COUNT; key++) {
		if (un_press[key] == true) {
			pressing[key] = false;
			un_press[key] = false;
			lock[key] = false;
		}
	}
}

void InputState::resetScroll() {
	scroll_up = false;
	scroll_down = false;
}

void InputState::lockActionBar() {
	pressing[Input::BAR_1] = false;
	pressing[Input::BAR_2] = false;
	pressing[Input::BAR_3] = false;
	pressing[Input::BAR_4] = false;
	pressing[Input::BAR_5] = false;
	pressing[Input::BAR_6] = false;
	pressing[Input::BAR_7] = false;
	pressing[Input::BAR_8] = false;
	pressing[Input::BAR_9] = false;
	pressing[Input::BAR_0] = false;
	pressing[Input::MAIN1] = false;
	pressing[Input::MAIN2] = false;
	pressing[Input::MENU_ACTIVATE] = false;
	lock[Input::BAR_1] = true;
	lock[Input::BAR_2] = true;
	lock[Input::BAR_3] = true;
	lock[Input::BAR_4] = true;
	lock[Input::BAR_5] = true;
	lock[Input::BAR_6] = true;
	lock[Input::BAR_7] = true;
	lock[Input::BAR_8] = true;
	lock[Input::BAR_9] = true;
	lock[Input::BAR_0] = true;
	lock[Input::MAIN1] = true;
	lock[Input::MAIN2] = true;
	lock[Input::MENU_ACTIVATE] = true;
}

void InputState::unlockActionBar() {
	lock[Input::BAR_1] = false;
	lock[Input::BAR_2] = false;
	lock[Input::BAR_3] = false;
	lock[Input::BAR_4] = false;
	lock[Input::BAR_5] = false;
	lock[Input::BAR_6] = false;
	lock[Input::BAR_7] = false;
	lock[Input::BAR_8] = false;
	lock[Input::BAR_9] = false;
	lock[Input::BAR_0] = false;
	lock[Input::MAIN1] = false;
	lock[Input::MAIN2] = false;
	lock[Input::MENU_ACTIVATE] = false;
}

void InputState::enableEventLog() {
	dump_event = true;
}

Point InputState::scaleMouse(unsigned int x, unsigned int y) {
	if (settings->mouse_scaled) {
		return Point(x,y);
	}

	Point scaled_mouse;
	int offsetY = static_cast<int>(((settings->screen_h - settings->view_h / settings->view_scaling) / 2) * settings->view_scaling);

	scaled_mouse.x = static_cast<int>(static_cast<float>(x) * settings->view_scaling);
	scaled_mouse.y = static_cast<int>(static_cast<float>(y) * settings->view_scaling) - offsetY;

	return scaled_mouse;
}
