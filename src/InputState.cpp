/*
Copyright © 2011-2012 Clint Bellanger
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
 * class InputState
 *
 * Handles keyboard and mouse states
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "InputState.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsDebug.h"
#include "UtilsParsing.h"

#include <math.h>

using namespace std;

InputState::InputState(void)
	: done(false)
	, mouse()
	, last_key(0)
	, last_button(0)
	, last_joybutton(0)
	, scroll_up(false)
	, scroll_down(false)
	, lock_scroll(false)
	, touch_locked(false)
	, current_touch() {
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_StartTextInput();
#else
	SDL_EnableUNICODE(true);
#endif

	defaultQwertyKeyBindings();
	defaultJoystickBindings();

	for (int key=0; key<key_count; key++) {
		pressing[key] = false;
		un_press[key] = false;
		lock[key] = false;
	}

	loadKeyBindings();
	setKeybindNames();
}


void InputState::defaultQwertyKeyBindings () {
	binding[CANCEL] = SDLK_ESCAPE;
	binding[ACCEPT] = SDLK_RETURN;
#ifdef __ANDROID__
    binding[CANCEL] = SDLK_AC_BACK;
	binding[ACCEPT] = SDLK_MENU;
#endif
	binding[UP] = SDLK_w;
	binding[DOWN] = SDLK_s;
	binding[LEFT] = SDLK_a;
	binding[RIGHT] = SDLK_d;

	binding_alt[CANCEL] = SDLK_ESCAPE;
	binding_alt[ACCEPT] = SDLK_SPACE;
	binding_alt[UP] = SDLK_UP;
	binding_alt[DOWN] = SDLK_DOWN;
	binding_alt[LEFT] = SDLK_LEFT;
	binding_alt[RIGHT] = SDLK_RIGHT;

	binding[BAR_1] = binding_alt[BAR_1] = SDLK_1;
	binding[BAR_2] = binding_alt[BAR_2] = SDLK_2;
	binding[BAR_3] = binding_alt[BAR_3] = SDLK_3;
	binding[BAR_4] = binding_alt[BAR_4] = SDLK_4;
	binding[BAR_5] = binding_alt[BAR_5] = SDLK_5;
	binding[BAR_6] = binding_alt[BAR_6] = SDLK_6;
	binding[BAR_7] = binding_alt[BAR_7] = SDLK_7;
	binding[BAR_8] = binding_alt[BAR_8] = SDLK_8;
	binding[BAR_9] = binding_alt[BAR_9] = SDLK_9;
	binding[BAR_0] = binding_alt[BAR_0] = SDLK_0;

	binding[CHARACTER] = binding_alt[CHARACTER] = SDLK_c;
	binding[INVENTORY] = binding_alt[INVENTORY] = SDLK_i;
	binding[POWERS] = binding_alt[POWERS] = SDLK_p;
	binding[LOG] = binding_alt[LOG] = SDLK_l;

	binding[MAIN1] = binding_alt[MAIN1] = SDL_BUTTON_LEFT;
	binding[MAIN2] = binding_alt[MAIN2] = SDL_BUTTON_RIGHT;

	binding[CTRL] = SDLK_LCTRL;
	binding_alt[CTRL] = SDLK_RCTRL;
	binding[SHIFT] = SDLK_LSHIFT;
	binding_alt[SHIFT] = SDLK_RSHIFT;
	binding[DEL] = SDLK_DELETE;
	binding_alt[DEL] = SDLK_BACKSPACE;
	binding[ALT] = SDLK_LALT;
	binding_alt[ALT] = SDLK_RALT;

	binding[ACTIONBAR] = binding_alt[ACTIONBAR] = SDLK_b;
	binding[ACTIONBAR_BACK] = binding_alt[ACTIONBAR_BACK] = SDLK_z;
	binding[ACTIONBAR_FORWARD] = binding_alt[ACTIONBAR_FORWARD] = SDLK_x;
	binding[ACTIONBAR_USE] = binding_alt[ACTIONBAR_USE] = SDLK_n;

	binding[DEVELOPER_MENU] = binding_alt[DEVELOPER_MENU] = SDLK_F5;
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
}

/**
 * Key bindings are found in config/keybindings.txt
 */
void InputState::loadKeyBindings() {

	FileParser infile;

	if (!infile.open(PATH_CONF + FILE_KEYBINDINGS, false, "")) {
		if (!infile.open("engine/default_keybindings.txt", true, "")) {
			saveKeyBindings();
			return;
		}
		else saveKeyBindings();
	}

	while (infile.next()) {
		int key1 = popFirstInt(infile.val);
		int key2 = popFirstInt(infile.val);

		// if we're loading an older keybindings file, we need to unbind all joystick bindings
		int key3 = -1;
		std::string temp = infile.val;
		if (popFirstString(temp) != "") {
			key3 = popFirstInt(infile.val);
		}

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
	ofstream outfile;
	outfile.open((PATH_CONF + FILE_KEYBINDINGS).c_str(), ios::out);

	if (outfile.is_open()) {

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

		if (outfile.bad()) logError("InputState: Unable to write keybindings config file. No write access or disk is full!\n");
		outfile.close();
		outfile.clear();
	}

}

void InputState::handle(bool dump_event) {
	SDL_Event event;

#ifndef __ANDROID__
	SDL_GetMouseState(&mouse.x, &mouse.y);
#endif

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

	/* Check for events */
	while (SDL_PollEvent (&event)) {

		if (dump_event) {
			cout << event << endl;
		}

		// grab symbol keys
#if SDL_VERSION_ATLEAST(2,0,0)
		if (event.type == SDL_TEXTINPUT) {
			inkeys += event.text.text;
		}
#else
		if (event.type == SDL_KEYDOWN) {
			int ch = event.key.keysym.unicode;
			// if it is printable char then write its utf-8 representation
			if (ch >= 0x800) {
				inkeys += (char) ((ch >> 12) | 0xe0);
				inkeys += (char) (((ch >> 6) & 0x3f) | 0x80);
				inkeys += (char) ((ch & 0x3f) | 0x80);
			}
			else if (ch >= 0x80) {
				inkeys += (char) ((ch >> 6) | 0xc0);
				inkeys += (char) ((ch & 0x3f) | 0x80);
			}
			else if (ch >= 32 && ch != 127) {
				inkeys += (char)ch;
			}
		}
#endif

		switch (event.type) {

#if SDL_VERSION_ATLEAST(2,0,0)
			case SDL_MOUSEWHEEL:
				if (event.wheel.y > 0) {
					scroll_up = true;
				} else if (event.wheel.y < 0) {
					scroll_down = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				for (int key=0; key<key_count; key++) {
					if (event.button.button == binding[key] || event.button.button == binding_alt[key]) {
						pressing[key] = true;
						un_press[key] = false;
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				for (int key=0; key<key_count; key++) {
					if (event.button.button == binding[key] || event.button.button == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_button = event.button.button;
				break;
			// Android touch events
			case SDL_FINGERMOTION:
				mouse.x = (int)((event.tfinger.x + event.tfinger.dx) * VIEW_W);
				mouse.y = (int)((event.tfinger.y + event.tfinger.dy) * VIEW_H);

				if (event.tfinger.dy > 0) {
					scroll_up = true;
				} else if (event.tfinger.dy < 0) {
					scroll_down = true;
				}
				break;
			case SDL_FINGERDOWN:
				touch_locked = true;
				mouse.x = (int)(event.tfinger.x * VIEW_W);
				mouse.y = (int)(event.tfinger.y * VIEW_H);
				pressing[MAIN1] = true;
				un_press[MAIN1] = false;
				break;
			case SDL_FINGERUP:
				touch_locked = false;
				un_press[MAIN1] = true;
				last_button = binding[MAIN1];
				break;
#else
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_WHEELUP) {
					scroll_up = true;
				}
				else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
					scroll_down = true;
				}
				if (!lock_scroll || (event.button.button != SDL_BUTTON_WHEELUP && event.button.button != SDL_BUTTON_WHEELDOWN)) {
					for (int key=0; key<key_count; key++) {
						if (event.button.button == binding[key] || event.button.button == binding_alt[key]) {
							pressing[key] = true;
							un_press[key] = false;
						}
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				for (int key=0; key<key_count; key++) {
					if ((scroll_up && (binding[key] == SDL_BUTTON_WHEELUP || binding_alt[key] == SDL_BUTTON_WHEELUP)) ||
					    (scroll_down && (binding[key] == SDL_BUTTON_WHEELDOWN || binding_alt[key] == SDL_BUTTON_WHEELDOWN))) {
						un_press[key] = true;
					}
					else if (event.button.button == binding[key] || event.button.button == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_button = event.button.button;
				break;
#endif
			case SDL_KEYDOWN:
				for (int key=0; key<key_count; key++) {
					if (event.key.keysym.sym == binding[key] || event.key.keysym.sym == binding_alt[key]) {
						pressing[key] = true;
						un_press[key] = false;
					}
				}
				break;
			case SDL_KEYUP:
				for (int key=0; key<key_count; key++) {
					if (event.key.keysym.sym == binding[key] || event.key.keysym.sym == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_key = event.key.keysym.sym;
				break;
				/*
				case SDL_JOYAXISMOTION:
					// Reading joystick from SDL_JOYAXISMOTION is slow. Joystick analog input is handled by SDL_JoystickGetAxis() now.
					break;
				*/
			case SDL_JOYHATMOTION:
				if(JOYSTICK_DEVICE == event.jhat.which && ENABLE_JOYSTICK) {
					switch (event.jhat.value) {
						case SDL_HAT_CENTERED:
							un_press[UP] = true;
							un_press[DOWN] = true;
							un_press[LEFT] = true;
							un_press[RIGHT] = true;
							break;
						case SDL_HAT_UP:
							pressing[UP] = true;
							un_press[UP] = false;
							pressing[DOWN] = false;
							lock[DOWN] = false;
							pressing[LEFT] = false;
							lock[LEFT] = false;
							pressing[RIGHT] = false;
							lock[RIGHT] = false;
							break;
						case SDL_HAT_DOWN:
							pressing[UP] = false;
							lock[UP] = false;
							pressing[DOWN] = true;
							un_press[DOWN] = false;
							pressing[LEFT] = false;
							lock[LEFT] = false;
							pressing[RIGHT] = false;
							lock[RIGHT] = false;
							break;
						case SDL_HAT_LEFT:
							pressing[UP] = false;
							lock[UP] = false;
							pressing[DOWN] = false;
							lock[DOWN] = false;
							pressing[LEFT] = true;
							un_press[LEFT] = false;
							pressing[RIGHT] = false;
							lock[RIGHT] = false;
							break;
						case SDL_HAT_RIGHT:
							pressing[UP] = false;
							lock[UP] = false;
							pressing[DOWN] = false;
							lock[DOWN] = false;
							pressing[LEFT] = false;
							lock[LEFT] = false;
							pressing[RIGHT] = true;
							un_press[RIGHT] = false;
							break;
						case SDL_HAT_LEFTUP:
							pressing[UP] = true;
							un_press[UP] = false;
							pressing[DOWN] = false;
							lock[DOWN] = false;
							pressing[LEFT] = true;
							un_press[LEFT] = false;
							pressing[RIGHT] = false;
							lock[RIGHT] = false;
							break;
						case SDL_HAT_LEFTDOWN:
							pressing[UP] = false;
							lock[UP] = false;
							pressing[DOWN] = true;
							un_press[DOWN] = false;
							pressing[LEFT] = true;
							un_press[LEFT] = false;
							pressing[RIGHT] = false;
							lock[RIGHT] = false;
							break;
						case SDL_HAT_RIGHTUP:
							pressing[UP] = true;
							un_press[UP] = false;
							pressing[DOWN] = false;
							lock[DOWN] = false;
							pressing[LEFT] = false;
							lock[LEFT] = false;
							pressing[RIGHT] = true;
							un_press[RIGHT] = false;
							break;
						case SDL_HAT_RIGHTDOWN:
							pressing[UP] = false;
							lock[UP] = false;
							pressing[DOWN] = true;
							un_press[DOWN] = false;
							pressing[LEFT] = false;
							lock[LEFT] = false;
							pressing[RIGHT] = true;
							un_press[RIGHT] = false;
							break;
					}
				}
				break;
			case SDL_JOYBUTTONDOWN:
				if(JOYSTICK_DEVICE == event.jbutton.which && ENABLE_JOYSTICK) {
					for (int key=0; key<key_count; key++) {
						if (event.jbutton.button == binding_joy[key]) {
							pressing[key] = true;
							un_press[key] = false;
						}
					}
				}
				break;
			case SDL_JOYBUTTONUP:
				if(JOYSTICK_DEVICE == event.jbutton.which && ENABLE_JOYSTICK) {
					for (int key=0; key<key_count; key++) {
						if (event.jbutton.button == binding_joy[key]) {
							un_press[key] = true;
						}
						last_joybutton = event.jbutton.button;
					}
				}
				break;
			case SDL_QUIT:
				done = 1;
				break;
			default:
				break;
		}
	}

	// joystick analog input
	if(ENABLE_JOYSTICK) {
		static bool joyReverseAxisX;
		static bool joyReverseAxisY;
		static bool joyHasMovedX;
		static bool joyHasMovedY;
		static int joyLastPosX;
		static int joyLastPosY;

		int joyAxisXval = SDL_JoystickGetAxis(joy, 0);
		int joyAxisYval = SDL_JoystickGetAxis(joy, 1);

		// axis 0
		if(joyAxisXval < -JOY_DEADZONE) {
			if(!joyReverseAxisX) {
				if(joyLastPosX == JOY_POS_RIGHT) {
					joyHasMovedX = 0;
				}
			}
			else {
				if(joyLastPosX == JOY_POS_LEFT) {
					joyHasMovedX = 0;
				}
			}
			if(joyHasMovedX == 0) {
				if(!joyReverseAxisX) {
					pressing[LEFT] = true;
					un_press[LEFT] = false;
					pressing[RIGHT] = false;
					lock[RIGHT] = false;
					joyLastPosX = JOY_POS_LEFT;
				}
				else {
					pressing[RIGHT] = true;
					un_press[RIGHT] = false;
					pressing[LEFT] = false;
					lock[LEFT] = false;
					joyLastPosX = JOY_POS_RIGHT;
				}
				joyHasMovedX = 1;
			}
		}
		if(joyAxisXval > JOY_DEADZONE) {
			if(!joyReverseAxisX) {
				if(joyLastPosX == JOY_POS_LEFT) {
					joyHasMovedX = 0;
				}
			}
			else {
				if(joyLastPosX == JOY_POS_RIGHT) {
					joyHasMovedX = 0;
				}
			}
			if(joyHasMovedX == 0) {
				if(!joyReverseAxisX) {
					pressing[RIGHT] = true;
					un_press[RIGHT] = false;
					pressing[LEFT] = false;
					lock[LEFT] = false;
					joyLastPosX = JOY_POS_RIGHT;
				}
				else {
					pressing[LEFT] = true;
					un_press[LEFT] = false;
					pressing[RIGHT] = false;
					lock[RIGHT] = false;
					joyLastPosX = JOY_POS_LEFT;
				}
				joyHasMovedX = 1;
			}
		}
		if((joyAxisXval >= -JOY_DEADZONE) && (joyAxisXval < JOY_DEADZONE)) {
			un_press[LEFT] = true;
			un_press[RIGHT] = true;
			joyHasMovedX = 0;
			joyLastPosX = JOY_POS_CENTER;
		}

		// axis 1
		if(joyAxisYval < -JOY_DEADZONE) {
			if(!joyReverseAxisY) {
				if(joyLastPosY == JOY_POS_DOWN) {
					joyHasMovedY = 0;
				}
			}
			else {
				if(joyLastPosY == JOY_POS_UP) {
					joyHasMovedY = 0;
				}
			}
			if(joyHasMovedY == 0) {
				if(!joyReverseAxisY) {
					pressing[UP] = true;
					un_press[UP] = false;
					pressing[DOWN] = false;
					lock[DOWN] = false;
					joyLastPosY = JOY_POS_UP;
				}
				else {
					pressing[DOWN] = true;
					un_press[DOWN] = false;
					pressing[UP] = false;
					lock[UP] = false;
					joyLastPosY = JOY_POS_DOWN;
				}
				joyHasMovedY = 1;
			}
		}
		if(joyAxisYval > JOY_DEADZONE) {
			if(!joyReverseAxisY) {
				if(joyLastPosY == JOY_POS_UP) {
					joyHasMovedY = 0;
				}
			}
			else {
				if(joyLastPosY == JOY_POS_DOWN) {
					joyHasMovedY = 0;
				}
			}
			if(joyHasMovedY == 0) {
				if(!joyReverseAxisY) {
					pressing[DOWN] = true;
					un_press[DOWN] = false;
					pressing[UP] = false;
					lock[UP] = false;
					joyLastPosY = JOY_POS_DOWN;
				}
				else {
					pressing[UP] = true;
					un_press[UP] = false;
					pressing[DOWN] = false;
					lock[DOWN] = false;
					joyLastPosY = JOY_POS_UP;
				}
				joyHasMovedY = 1;
			}
		}
		if((joyAxisYval >= -JOY_DEADZONE) && (joyAxisYval < JOY_DEADZONE)) {
			un_press[UP] = true;
			un_press[DOWN] = true;
			joyHasMovedY = 0;
			joyLastPosY = JOY_POS_CENTER;
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

	mouse_button[0] = msg->get("lmb");
	mouse_button[1] = msg->get("mmb");
	mouse_button[2] = msg->get("rmb");
	mouse_button[3] = msg->get("wheel up");
	mouse_button[4] = msg->get("wheel down");
	mouse_button[5] = msg->get("mbx1");
	mouse_button[6] = msg->get("mbx2");
}

void InputState::hideCursor() {
	SDL_ShowCursor(SDL_DISABLE);
}

void InputState::showCursor() {
	SDL_ShowCursor(SDL_ENABLE);
}

std::string InputState::getJoystickName(int index) {
#if SDL_VERSION_ATLEAST(2,0,0)
	return std::string(SDL_JoystickNameForIndex(index));
#else
	return std::string(SDL_JoystickName(index));
#endif
}

std::string InputState::getKeyName(int key) {
#if SDL_VERSION_ATLEAST(2,0,0)
	return std::string(SDL_GetKeyName((SDL_Keycode)key));
#else
	return std::string(SDL_GetKeyName((SDLKey)key));
#endif
}

InputState::~InputState() {
}
