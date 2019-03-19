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
#include "UtilsParsing.h"
#include "Version.h"

#include <math.h>

InputState::InputState(void)
	: binding()
	, binding_alt()
	, binding_joy()
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
	, pressing_up(false)
	, pressing_down(false)
	, joysticks_changed(false)
	, un_press()
	, current_touch()
	, dump_event(false)
	, file_version(new Version())
	, file_version_min(new Version(1, 9, 20))
{
}

InputState::~InputState() {
	delete file_version;
	delete file_version_min;
}

void InputState::defaultJoystickBindings () {
	// most joystick buttons are unbound by default
	for (int key=0; key<KEY_COUNT; key++) {
		binding_joy[key] = -1;
	}

	binding_joy[Input::CANCEL] = 0;
	binding_joy[Input::ACCEPT] = 1;
	binding_joy[Input::ACTIONBAR] = 2;
	binding_joy[Input::ACTIONBAR_USE] = 3;
	binding_joy[Input::ACTIONBAR_BACK] = 4;
	binding_joy[Input::ACTIONBAR_FORWARD] = 5;

	// axis 0
	binding_joy[Input::LEFT] = JOY_AXIS_OFFSET * (-1);
	binding_joy[Input::RIGHT] = (JOY_AXIS_OFFSET+1) * (-1);

	// axis 1
	binding_joy[Input::UP] = (JOY_AXIS_OFFSET+2) * (-1);
	binding_joy[Input::DOWN] = (JOY_AXIS_OFFSET+3) * (-1);
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
	// if there are no mod keybinds, fall back to global config
	else if (infile.open(settings->path_conf + "keybindings.txt", !FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		opened_file = true;
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

		int key1 = -1;
		int key2 = -1;
		int key3 = -1;

		if (infile.section == "user") {
			// this is a traditional keybindings file that has been written by the engine via saveKeyBindings()
			key1 = Parse::toInt(Parse::popFirstString(infile.val), -1);
			key2 = Parse::toInt(Parse::popFirstString(infile.val), -1);
			key3 = Parse::toInt(Parse::popFirstString(infile.val), -1);
		}
		else if (infile.section == "default") {
			// this is a set of default keybindings located in a mod
			std::string str1 = Parse::popFirstString(infile.val);
			std::string str2 = Parse::popFirstString(infile.val);
			std::string str3 = Parse::popFirstString(infile.val);

			if (str1.length() > 6 && str1.substr(0, 6) == "mouse_")
				key1 = (Parse::toInt(str1.substr(6))+ 1 + MOUSE_BIND_OFFSET) * (-1);
			else if (str1 != "-1")
				key1 = getKeyFromName(str1);

			if (str2.length() > 6 && str2.substr(0, 6) == "mouse_")
				key2 = (Parse::toInt(str1.substr(6))+ 1 + MOUSE_BIND_OFFSET) * (-1);
			else if (str2 != "-1")
				key2 = getKeyFromName(str2);

			if (str3.length() > 5 && str3.substr(0, 5) == "axis_") {
				size_t pos_minus = str3.find('-');
				size_t pos_plus = str3.find('+');
				if (pos_minus != std::string::npos) {
					key3 = ((Parse::toInt(str3.substr(5, pos_minus)) * 2) + JOY_AXIS_OFFSET) * (-1);
				}
				else if (pos_plus != std::string::npos) {
					key3 = ((Parse::toInt(str3.substr(5, pos_plus)) * 2) + 1 + JOY_AXIS_OFFSET) * (-1);
				}
			}
			else if (str3.length() > 4 && str3.substr(0, 4) == "joy_")
				key3 = Parse::toInt(str3.substr(4));
		}
		else
			continue;

		// @CLASS InputState: Default Keybindings|Description of engine/default_keybindings.txt. Use **-1** for no binding. Keyboard values can be any of the key names listed in the [SDL docs](https://wiki.libsdl.org/SDL_Keycode). Mouse values are in the format **mouse_0**. Joystick buttons are in the format **joy_0**. Joystick axis are in the format **axis_0-** or **axis_0+**.
		// @ATTR default.cancel|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Cancel".
		// @ATTR default.accept|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Accept".
		// @ATTR default.up|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Up".
		// @ATTR default.down|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Down".
		// @ATTR default.left|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Left".
		// @ATTR default.right|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Right".
		// @ATTR default.bar1|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar1".
		// @ATTR default.bar2|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar2".
		// @ATTR default.bar3|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar3".
		// @ATTR default.bar4|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar4".
		// @ATTR default.bar5|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar5".
		// @ATTR default.bar6|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar6".
		// @ATTR default.bar7|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar7".
		// @ATTR default.bar8|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar8".
		// @ATTR default.bar9|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar9".
		// @ATTR default.bar0|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Bar0".
		// @ATTR default.main1|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Main1".
		// @ATTR default.main2|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Main2".
		// @ATTR default.character|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Character".
		// @ATTR default.inventory|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Inventory".
		// @ATTR default.powers|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Powers".
		// @ATTR default.log|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Log".
		// @ATTR default.ctrl|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Ctrl".
		// @ATTR default.shift|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Shift".
		// @ATTR default.alt|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Alt".
		// @ATTR default.delete|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Delete".
		// @ATTR default.actionbar|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Actionbar Accept".
		// @ATTR default.actionbar_back|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Actionbar Left".
		// @ATTR default.actionbar_forward|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Actionbar Right".
		// @ATTR default.actionbar_use|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Actionbar Use".
		// @ATTR default.developer_menu|string, string, string : Keyboard/mouse 1, Keyboard/mouse 2, Joystick|Bindings for "Developer Menu".

		int cursor = -1;

		if (infile.key == "cancel") cursor = Input::CANCEL;
		else if (infile.key == "accept") cursor = Input::ACCEPT;
		else if (infile.key == "up") cursor = Input::UP;
		else if (infile.key == "down") cursor = Input::DOWN;
		else if (infile.key == "left") cursor = Input::LEFT;
		else if (infile.key == "right") cursor = Input::RIGHT;
		else if (infile.key == "bar1") cursor = Input::BAR_1;
		else if (infile.key == "bar2") cursor = Input::BAR_2;
		else if (infile.key == "bar3") cursor = Input::BAR_3;
		else if (infile.key == "bar4") cursor = Input::BAR_4;
		else if (infile.key == "bar5") cursor = Input::BAR_5;
		else if (infile.key == "bar6") cursor = Input::BAR_6;
		else if (infile.key == "bar7") cursor = Input::BAR_7;
		else if (infile.key == "bar8") cursor = Input::BAR_8;
		else if (infile.key == "bar9") cursor = Input::BAR_9;
		else if (infile.key == "bar0") cursor = Input::BAR_0;
		else if (infile.key == "main1") cursor = Input::MAIN1;
		else if (infile.key == "main2") cursor = Input::MAIN2;
		else if (infile.key == "character") cursor = Input::CHARACTER;
		else if (infile.key == "inventory") cursor = Input::INVENTORY;
		else if (infile.key == "powers") cursor = Input::POWERS;
		else if (infile.key == "log") cursor = Input::LOG;
		else if (infile.key == "ctrl") cursor = Input::CTRL;
		else if (infile.key == "shift") cursor = Input::SHIFT;
		else if (infile.key == "alt") cursor = Input::ALT;
		else if (infile.key == "delete") cursor = Input::DEL;
		else if (infile.key == "actionbar") cursor = Input::ACTIONBAR;
		else if (infile.key == "actionbar_back") cursor = Input::ACTIONBAR_BACK;
		else if (infile.key == "actionbar_forward") cursor = Input::ACTIONBAR_FORWARD;
		else if (infile.key == "actionbar_use") cursor = Input::ACTIONBAR_USE;
		else if (infile.key == "developer_menu") cursor = Input::DEVELOPER_MENU;

		if (cursor != -1) {
			binding[cursor] = key1;
			binding_alt[cursor] = key2;
			binding_joy[cursor] = key3;
		}

	}
	infile.close();

	setFixedKeyBindings();
}

/**
 * Write current key bindings to config file
 */
void InputState::saveKeyBindings() {
	std::string out_path;
	if (mods->locate("engine/default_keybindings.txt") != "") {
		out_path = settings->path_user + "saves/" + eset->misc.save_prefix + "/keybindings.txt";
	}
	else {
		out_path = settings->path_conf + "keybindings.txt";
	}
	std::ofstream outfile;
	outfile.open(out_path.c_str(), std::ios::out);

	if (outfile.is_open()) {
		outfile << "# Keybindings\n";
		outfile << "# FORMAT: {ACTION}={BIND},{BIND_ALT},{BIND_JOY}\n";
		outfile << "# A bind value of -1 means unbound\n";
		outfile << "# For BIND and BIND_ALT, a value of 0 is also unbound\n";
		outfile << "# For BIND and BIND_ALT, any value less than -1 is a mouse button\n";
		outfile << "# As an example, mouse button 1 would be -3 here. Button 2 would be -4, etc.\n\n";

		*file_version = *file_version_min;
		outfile << "file_version=" << file_version->getString() << "\n\n";

		outfile << "[user]\n";
		outfile << "cancel=" << binding[Input::CANCEL] << "," << binding_alt[Input::CANCEL] << "," << binding_joy[Input::CANCEL] << "\n";
		outfile << "accept=" << binding[Input::ACCEPT] << "," << binding_alt[Input::ACCEPT] << "," << binding_joy[Input::ACCEPT] << "\n";
		outfile << "up=" << binding[Input::UP] << "," << binding_alt[Input::UP] << "," << binding_joy[Input::UP] << "\n";
		outfile << "down=" << binding[Input::DOWN] << "," << binding_alt[Input::DOWN] << "," << binding_joy[Input::DOWN] << "\n";
		outfile << "left=" << binding[Input::LEFT] << "," << binding_alt[Input::LEFT] << "," << binding_joy[Input::LEFT] << "\n";
		outfile << "right=" << binding[Input::RIGHT] << "," << binding_alt[Input::RIGHT] << "," << binding_joy[Input::RIGHT] << "\n";
		outfile << "bar1=" << binding[Input::BAR_1] << "," << binding_alt[Input::BAR_1] << "," << binding_joy[Input::BAR_1] << "\n";
		outfile << "bar2=" << binding[Input::BAR_2] << "," << binding_alt[Input::BAR_2] << "," << binding_joy[Input::BAR_2] << "\n";
		outfile << "bar3=" << binding[Input::BAR_3] << "," << binding_alt[Input::BAR_3] << "," << binding_joy[Input::BAR_3] << "\n";
		outfile << "bar4=" << binding[Input::BAR_4] << "," << binding_alt[Input::BAR_4] << "," << binding_joy[Input::BAR_4] << "\n";
		outfile << "bar5=" << binding[Input::BAR_5] << "," << binding_alt[Input::BAR_5] << "," << binding_joy[Input::BAR_5] << "\n";
		outfile << "bar6=" << binding[Input::BAR_6] << "," << binding_alt[Input::BAR_6] << "," << binding_joy[Input::BAR_6] << "\n";
		outfile << "bar7=" << binding[Input::BAR_7] << "," << binding_alt[Input::BAR_7] << "," << binding_joy[Input::BAR_7] << "\n";
		outfile << "bar8=" << binding[Input::BAR_8] << "," << binding_alt[Input::BAR_8] << "," << binding_joy[Input::BAR_8] << "\n";
		outfile << "bar9=" << binding[Input::BAR_9] << "," << binding_alt[Input::BAR_9] << "," << binding_joy[Input::BAR_9] << "\n";
		outfile << "bar0=" << binding[Input::BAR_0] << "," << binding_alt[Input::BAR_0] << "," << binding_joy[Input::BAR_0] << "\n";
		outfile << "main1=" << binding[Input::MAIN1] << "," << binding_alt[Input::MAIN1] << "," << binding_joy[Input::MAIN1] << "\n";
		outfile << "main2=" << binding[Input::MAIN2] << "," << binding_alt[Input::MAIN2] << "," << binding_joy[Input::MAIN2] << "\n";
		outfile << "character=" << binding[Input::CHARACTER] << "," << binding_alt[Input::CHARACTER] << "," << binding_joy[Input::CHARACTER] << "\n";
		outfile << "inventory=" << binding[Input::INVENTORY] << "," << binding_alt[Input::INVENTORY] << "," << binding_joy[Input::INVENTORY] << "\n";
		outfile << "powers=" << binding[Input::POWERS] << "," << binding_alt[Input::POWERS] << "," << binding_joy[Input::POWERS] << "\n";
		outfile << "log=" << binding[Input::LOG] << "," << binding_alt[Input::LOG] << "," << binding_joy[Input::LOG] << "\n";
		outfile << "ctrl=" << binding[Input::CTRL] << "," << binding_alt[Input::CTRL] << "," << binding_joy[Input::CTRL] << "\n";
		outfile << "shift=" << binding[Input::SHIFT] << "," << binding_alt[Input::SHIFT] << "," << binding_joy[Input::SHIFT] << "\n";
		outfile << "alt=" << binding[Input::ALT] << "," << binding_alt[Input::ALT] << "," << binding_joy[Input::ALT] << "\n";
		outfile << "delete=" << binding[Input::DEL] << "," << binding_alt[Input::DEL] << "," << binding_joy[Input::DEL] << "\n";
		outfile << "actionbar=" << binding[Input::ACTIONBAR] << "," << binding_alt[Input::ACTIONBAR] << "," << binding_joy[Input::ACTIONBAR] << "\n";
		outfile << "actionbar_back=" << binding[Input::ACTIONBAR_BACK] << "," << binding_alt[Input::ACTIONBAR_BACK] << "," << binding_joy[Input::ACTIONBAR_BACK] << "\n";
		outfile << "actionbar_forward=" << binding[Input::ACTIONBAR_FORWARD] << "," << binding_alt[Input::ACTIONBAR_FORWARD] << "," << binding_joy[Input::ACTIONBAR_FORWARD] << "\n";
		outfile << "actionbar_use=" << binding[Input::ACTIONBAR_USE] << "," << binding_alt[Input::ACTIONBAR_USE] << "," << binding_joy[Input::ACTIONBAR_USE] << "\n";
		outfile << "developer_menu=" << binding[Input::DEVELOPER_MENU] << "," << binding_alt[Input::DEVELOPER_MENU] << "," << binding_joy[Input::DEVELOPER_MENU] << "\n";

		if (outfile.bad()) Utils::logError("InputState: Unable to write keybindings config file. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		platform.FSCommit();
	}

}

void InputState::handle() {
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
	pressing[Input::ACTIONBAR_USE] = false;
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
	lock[Input::ACTIONBAR_USE] = true;
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
	lock[Input::ACTIONBAR_USE] = false;
}

void InputState::setKeybindNames() {
	binding_name[Input::CANCEL] = msg->get("Cancel");
	binding_name[Input::ACCEPT] = msg->get("Accept");
	binding_name[Input::UP] = msg->get("Up");
	binding_name[Input::DOWN] = msg->get("Down");
	binding_name[Input::LEFT] = msg->get("Left");
	binding_name[Input::RIGHT] = msg->get("Right");
	binding_name[Input::BAR_1] = msg->get("Bar1");
	binding_name[Input::BAR_2] = msg->get("Bar2");
	binding_name[Input::BAR_3] = msg->get("Bar3");
	binding_name[Input::BAR_4] = msg->get("Bar4");
	binding_name[Input::BAR_5] = msg->get("Bar5");
	binding_name[Input::BAR_6] = msg->get("Bar6");
	binding_name[Input::BAR_7] = msg->get("Bar7");
	binding_name[Input::BAR_8] = msg->get("Bar8");
	binding_name[Input::BAR_9] = msg->get("Bar9");
	binding_name[Input::BAR_0] = msg->get("Bar0");
	binding_name[Input::CHARACTER] = msg->get("Character");
	binding_name[Input::INVENTORY] = msg->get("Inventory");
	binding_name[Input::POWERS] = msg->get("Powers");
	binding_name[Input::LOG] = msg->get("Log");
	binding_name[Input::MAIN1] = msg->get("Main1");
	binding_name[Input::MAIN2] = msg->get("Main2");
	binding_name[Input::CTRL] = msg->get("Ctrl");
	binding_name[Input::SHIFT] = msg->get("Shift");
	binding_name[Input::ALT] = msg->get("Alt");
	binding_name[Input::DEL] = msg->get("Delete");
	binding_name[Input::ACTIONBAR] = msg->get("ActionBar Accept");
	binding_name[Input::ACTIONBAR_BACK] = msg->get("ActionBar Left");
	binding_name[Input::ACTIONBAR_FORWARD] = msg->get("ActionBar Right");
	binding_name[Input::ACTIONBAR_USE] = msg->get("ActionBar Use");
	binding_name[Input::DEVELOPER_MENU] = msg->get("Developer Menu");

	mouse_button[0] = msg->get("Left Mouse");
	mouse_button[1] = msg->get("Middle Mouse");
	mouse_button[2] = msg->get("Right Mouse");
	mouse_button[3] = msg->get("Wheel Up");
	mouse_button[4] = msg->get("Wheel Down");
	mouse_button[5] = msg->get("Mouse X1");
	mouse_button[6] = msg->get("Mouse X2");
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
