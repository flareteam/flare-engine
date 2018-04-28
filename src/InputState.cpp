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

#include "FileParser.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "Platform.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

#include <math.h>

InputState::InputState(void)
	: binding()
	, binding_alt()
	, binding_joy()
	, pressing()
	, lock()
	, slow_repeat()
	, repeat_ticks()
	, max_repeat_ticks()
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
{
}

void InputState::defaultJoystickBindings () {
	// most joystick buttons are unbound by default
	for (int key=0; key<key_count; key++) {
		binding_joy[key] = -1;
	}

	binding_joy[CANCEL] = 0;
	binding_joy[ACCEPT] = 1;
	binding_joy[ACTIONBAR] = 2;
	binding_joy[ACTIONBAR_USE] = 3;
	binding_joy[ACTIONBAR_BACK] = 4;
	binding_joy[ACTIONBAR_FORWARD] = 5;

	// axis 0
	binding_joy[LEFT] = JOY_AXIS_OFFSET * (-1);
	binding_joy[RIGHT] = (JOY_AXIS_OFFSET+1) * (-1);

	// axis 1
	binding_joy[UP] = (JOY_AXIS_OFFSET+2) * (-1);
	binding_joy[DOWN] = (JOY_AXIS_OFFSET+3) * (-1);
}

/**
 * Key bindings are found in config/keybindings.txt
 */
void InputState::loadKeyBindings() {

	FileParser infile;
	bool opened_file = false;

	// first check for mod keybinds
	if (mods->locate("engine/default_keybindings.txt") != "") {
		if (infile.open(PATH_USER + "saves/" + SAVE_PREFIX + "/" + FILE_KEYBINDINGS, false, "")) {
			opened_file = true;
		}
		else if (infile.open("engine/default_keybindings.txt", true, "")) {
			opened_file = true;
		}
	}
	// if there are no mod keybinds, fall back to global config
	else if (infile.open(PATH_CONF + FILE_KEYBINDINGS, false, "")) {
		opened_file = true;
	}

	if (!opened_file) {
		saveKeyBindings();
		return;
	}

	while (infile.next()) {
		int key1 = -1;
		int key2 = -1;
		int key3 = -1;

		if (infile.section.empty()) {
			// this is a traditional keybindings file that has been written by the engine via saveKeyBindings()
			key1 = toInt(popFirstString(infile.val), -1);
			key2 = toInt(popFirstString(infile.val), -1);
			key3 = toInt(popFirstString(infile.val), -1);
		}
		else if (infile.section == "default") {
			// this is a set of default keybindings located in a mod
			std::string str1 = popFirstString(infile.val);
			std::string str2 = popFirstString(infile.val);
			std::string str3 = popFirstString(infile.val);

			if (str1.length() > 6 && str1.substr(0, 6) == "mouse_")
				key1 = (toInt(str1.substr(6))+ 1 + MOUSE_BIND_OFFSET) * (-1);
			else if (str1 != "-1")
				key1 = getKeyFromName(str1);

			if (str2.length() > 6 && str2.substr(0, 6) == "mouse_")
				key2 = (toInt(str1.substr(6))+ 1 + MOUSE_BIND_OFFSET) * (-1);
			else if (str2 != "-1")
				key2 = getKeyFromName(str2);

			if (str3.length() > 5 && str3.substr(0, 5) == "axis_") {
				size_t pos_minus = str3.find('-');
				size_t pos_plus = str3.find('+');
				if (pos_minus != std::string::npos) {
					key3 = ((toInt(str3.substr(5, pos_minus)) * 2) + JOY_AXIS_OFFSET) * (-1);
				}
				else if (pos_plus != std::string::npos) {
					key3 = ((toInt(str3.substr(5, pos_plus)) * 2) + 1 + JOY_AXIS_OFFSET) * (-1);
				}
			}
			else if (str3.length() > 4 && str3.substr(0, 4) == "joy_")
				key3 = toInt(str3.substr(4));
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

		if (infile.key == "cancel") cursor = CANCEL;
		else if (infile.key == "accept") cursor = ACCEPT;
		else if (infile.key == "up") cursor = UP;
		else if (infile.key == "down") cursor = DOWN;
		else if (infile.key == "left") cursor = LEFT;
		else if (infile.key == "right") cursor = RIGHT;
		else if (infile.key == "bar1") cursor = BAR_1;
		else if (infile.key == "bar2") cursor = BAR_2;
		else if (infile.key == "bar3") cursor = BAR_3;
		else if (infile.key == "bar4") cursor = BAR_4;
		else if (infile.key == "bar5") cursor = BAR_5;
		else if (infile.key == "bar6") cursor = BAR_6;
		else if (infile.key == "bar7") cursor = BAR_7;
		else if (infile.key == "bar8") cursor = BAR_8;
		else if (infile.key == "bar9") cursor = BAR_9;
		else if (infile.key == "bar0") cursor = BAR_0;
		else if (infile.key == "main1") cursor = MAIN1;
		else if (infile.key == "main2") cursor = MAIN2;
		else if (infile.key == "character") cursor = CHARACTER;
		else if (infile.key == "inventory") cursor = INVENTORY;
		else if (infile.key == "powers") cursor = POWERS;
		else if (infile.key == "log") cursor = LOG;
		else if (infile.key == "ctrl") cursor = CTRL;
		else if (infile.key == "shift") cursor = SHIFT;
		else if (infile.key == "alt") cursor = ALT;
		else if (infile.key == "delete") cursor = DEL;
		else if (infile.key == "actionbar") cursor = ACTIONBAR;
		else if (infile.key == "actionbar_back") cursor = ACTIONBAR_BACK;
		else if (infile.key == "actionbar_forward") cursor = ACTIONBAR_FORWARD;
		else if (infile.key == "actionbar_use") cursor = ACTIONBAR_USE;
		else if (infile.key == "developer_menu") cursor = DEVELOPER_MENU;

		if (cursor != -1) {
			binding[cursor] = key1;
			binding_alt[cursor] = key2;
			binding_joy[cursor] = key3;
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
		out_path = PATH_USER + "saves/" + SAVE_PREFIX + "/" + FILE_KEYBINDINGS;
	}
	else {
		out_path = PATH_CONF + FILE_KEYBINDINGS;
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

		outfile << "cancel=" << binding[CANCEL] << "," << binding_alt[CANCEL] << "," << binding_joy[CANCEL] << "\n";
		outfile << "accept=" << binding[ACCEPT] << "," << binding_alt[ACCEPT] << "," << binding_joy[ACCEPT] << "\n";
		outfile << "up=" << binding[UP] << "," << binding_alt[UP] << "," << binding_joy[UP] << "\n";
		outfile << "down=" << binding[DOWN] << "," << binding_alt[DOWN] << "," << binding_joy[DOWN] << "\n";
		outfile << "left=" << binding[LEFT] << "," << binding_alt[LEFT] << "," << binding_joy[LEFT] << "\n";
		outfile << "right=" << binding[RIGHT] << "," << binding_alt[RIGHT] << "," << binding_joy[RIGHT] << "\n";
		outfile << "bar1=" << binding[BAR_1] << "," << binding_alt[BAR_1] << "," << binding_joy[BAR_1] << "\n";
		outfile << "bar2=" << binding[BAR_2] << "," << binding_alt[BAR_2] << "," << binding_joy[BAR_2] << "\n";
		outfile << "bar3=" << binding[BAR_3] << "," << binding_alt[BAR_3] << "," << binding_joy[BAR_3] << "\n";
		outfile << "bar4=" << binding[BAR_4] << "," << binding_alt[BAR_4] << "," << binding_joy[BAR_4] << "\n";
		outfile << "bar5=" << binding[BAR_5] << "," << binding_alt[BAR_5] << "," << binding_joy[BAR_5] << "\n";
		outfile << "bar6=" << binding[BAR_6] << "," << binding_alt[BAR_6] << "," << binding_joy[BAR_6] << "\n";
		outfile << "bar7=" << binding[BAR_7] << "," << binding_alt[BAR_7] << "," << binding_joy[BAR_7] << "\n";
		outfile << "bar8=" << binding[BAR_8] << "," << binding_alt[BAR_8] << "," << binding_joy[BAR_8] << "\n";
		outfile << "bar9=" << binding[BAR_9] << "," << binding_alt[BAR_9] << "," << binding_joy[BAR_9] << "\n";
		outfile << "bar0=" << binding[BAR_0] << "," << binding_alt[BAR_0] << "," << binding_joy[BAR_0] << "\n";
		outfile << "main1=" << binding[MAIN1] << "," << binding_alt[MAIN1] << "," << binding_joy[MAIN1] << "\n";
		outfile << "main2=" << binding[MAIN2] << "," << binding_alt[MAIN2] << "," << binding_joy[MAIN2] << "\n";
		outfile << "character=" << binding[CHARACTER] << "," << binding_alt[CHARACTER] << "," << binding_joy[CHARACTER] << "\n";
		outfile << "inventory=" << binding[INVENTORY] << "," << binding_alt[INVENTORY] << "," << binding_joy[INVENTORY] << "\n";
		outfile << "powers=" << binding[POWERS] << "," << binding_alt[POWERS] << "," << binding_joy[POWERS] << "\n";
		outfile << "log=" << binding[LOG] << "," << binding_alt[LOG] << "," << binding_joy[LOG] << "\n";
		outfile << "ctrl=" << binding[CTRL] << "," << binding_alt[CTRL] << "," << binding_joy[CTRL] << "\n";
		outfile << "shift=" << binding[SHIFT] << "," << binding_alt[SHIFT] << "," << binding_joy[SHIFT] << "\n";
		outfile << "alt=" << binding[ALT] << "," << binding_alt[ALT] << "," << binding_joy[ALT] << "\n";
		outfile << "delete=" << binding[DEL] << "," << binding_alt[DEL] << "," << binding_joy[DEL] << "\n";
		outfile << "actionbar=" << binding[ACTIONBAR] << "," << binding_alt[ACTIONBAR] << "," << binding_joy[ACTIONBAR] << "\n";
		outfile << "actionbar_back=" << binding[ACTIONBAR_BACK] << "," << binding_alt[ACTIONBAR_BACK] << "," << binding_joy[ACTIONBAR_BACK] << "\n";
		outfile << "actionbar_forward=" << binding[ACTIONBAR_FORWARD] << "," << binding_alt[ACTIONBAR_FORWARD] << "," << binding_joy[ACTIONBAR_FORWARD] << "\n";
		outfile << "actionbar_use=" << binding[ACTIONBAR_USE] << "," << binding_alt[ACTIONBAR_USE] << "," << binding_joy[ACTIONBAR_USE] << "\n";
		outfile << "developer_menu=" << binding[DEVELOPER_MENU] << "," << binding_alt[DEVELOPER_MENU] << "," << binding_joy[DEVELOPER_MENU] << "\n";

		if (outfile.bad()) logError("InputState: Unable to write keybindings config file. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		PlatformFSCommit();
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
	for (int key=0; key < key_count; key++) {
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
	pressing[BAR_1] = false;
	pressing[BAR_2] = false;
	pressing[BAR_3] = false;
	pressing[BAR_4] = false;
	pressing[BAR_5] = false;
	pressing[BAR_6] = false;
	pressing[BAR_7] = false;
	pressing[BAR_8] = false;
	pressing[BAR_9] = false;
	pressing[BAR_0] = false;
	pressing[MAIN1] = false;
	pressing[MAIN2] = false;
	pressing[ACTIONBAR_USE] = false;
	lock[BAR_1] = true;
	lock[BAR_2] = true;
	lock[BAR_3] = true;
	lock[BAR_4] = true;
	lock[BAR_5] = true;
	lock[BAR_6] = true;
	lock[BAR_7] = true;
	lock[BAR_8] = true;
	lock[BAR_9] = true;
	lock[BAR_0] = true;
	lock[MAIN1] = true;
	lock[MAIN2] = true;
	lock[ACTIONBAR_USE] = true;
}

void InputState::unlockActionBar() {
	lock[BAR_1] = false;
	lock[BAR_2] = false;
	lock[BAR_3] = false;
	lock[BAR_4] = false;
	lock[BAR_5] = false;
	lock[BAR_6] = false;
	lock[BAR_7] = false;
	lock[BAR_8] = false;
	lock[BAR_9] = false;
	lock[BAR_0] = false;
	lock[MAIN1] = false;
	lock[MAIN2] = false;
	lock[ACTIONBAR_USE] = false;
}

void InputState::setKeybindNames() {
	binding_name[0] = msg->get("Cancel");
	binding_name[1] = msg->get("Accept");
	binding_name[2] = msg->get("Up");
	binding_name[3] = msg->get("Down");
	binding_name[4] = msg->get("Left");
	binding_name[5] = msg->get("Right");
	binding_name[6] = msg->get("Bar1");
	binding_name[7] = msg->get("Bar2");
	binding_name[8] = msg->get("Bar3");
	binding_name[9] = msg->get("Bar4");
	binding_name[10] = msg->get("Bar5");
	binding_name[11] = msg->get("Bar6");
	binding_name[12] = msg->get("Bar7");
	binding_name[13] = msg->get("Bar8");
	binding_name[14] = msg->get("Bar9");
	binding_name[15] = msg->get("Bar0");
	binding_name[16] = msg->get("Character");
	binding_name[17] = msg->get("Inventory");
	binding_name[18] = msg->get("Powers");
	binding_name[19] = msg->get("Log");
	binding_name[20] = msg->get("Main1");
	binding_name[21] = msg->get("Main2");
	binding_name[22] = msg->get("Ctrl");
	binding_name[23] = msg->get("Shift");
	binding_name[24] = msg->get("Alt");
	binding_name[25] = msg->get("Delete");
	binding_name[26] = msg->get("ActionBar Accept");
	binding_name[27] = msg->get("ActionBar Left");
	binding_name[28] = msg->get("ActionBar Right");
	binding_name[29] = msg->get("ActionBar Use");
	binding_name[30] = msg->get("Developer Menu");

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
	if (MOUSE_SCALED) {
		return Point(x,y);
	}

	Point scaled_mouse;
	int offsetY = static_cast<int>(((SCREEN_H - VIEW_H / VIEW_SCALING) / 2) * VIEW_SCALING);

	scaled_mouse.x = static_cast<int>(static_cast<float>(x) * VIEW_SCALING);
	scaled_mouse.y = static_cast<int>(static_cast<float>(y) * VIEW_SCALING) - offsetY;

	return scaled_mouse;
}
