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
 * class SDLInputState
 *
 * Handles keyboard and mouse states
 */

#include "CommonIncludes.h"
#include "SDLInputState.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsDebug.h"
#include "UtilsParsing.h"
#include "SaveLoad.h"
#include "SharedGameResources.h"

#include <math.h>

#if defined(__ANDROID__) || defined (__IPHONEOS__)

int isExitEvent(void* userdata, SDL_Event* event)
{
	if (event->type == SDL_APP_TERMINATING)
	{
		logInfo("Terminating app, saving...");
		save_load->saveGame();
		logInfo("Saved, ready to exit.");
		return 0;
	}
	return 1;
}

#endif

SDLInputState::SDLInputState(void)
	: InputState()
	, joy(NULL)
	, joy_num(0)
	, joy_axis_num(0)
{
#if !defined(__ANDROID__) && !defined (__IPHONEOS__)
	SDL_StartTextInput();
#else
	SDL_SetEventFilter(isExitEvent, NULL);
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

	// print some information to the console about connected joysticks
	if(SDL_NumJoysticks() > 0) {
		logInfo("%d joystick(s) found.", SDL_NumJoysticks());
		joy_num = SDL_NumJoysticks();
	}
	else {
		logInfo("No joysticks were found.");
		ENABLE_JOYSTICK = false;
		return;
	}

	for(int i = 0; i < SDL_NumJoysticks(); i++) {
		logInfo("  Joy %d) %s", i, getJoystickName(i).c_str());
	}
}

void SDLInputState::initJoystick() {
	SDL_Init(SDL_INIT_JOYSTICK);

	// close our joystick handle if it's open
	if (joy) {
		SDL_JoystickClose(joy);
		joy = NULL;
	}

	if ((ENABLE_JOYSTICK) && (SDL_NumJoysticks() > 0)) {
		joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
		logInfo("Using joystick #%d.", JOYSTICK_DEVICE);
	}

	if (joy) {
		joy_axis_num = SDL_JoystickNumAxes(joy);
		joy_axis_prev.resize(joy_axis_num*2, 0);
		joy_axis_deltas.resize(joy_axis_num*2, 0);
	}
}

void SDLInputState::defaultQwertyKeyBindings () {
	binding[CANCEL] = SDLK_ESCAPE;
	binding[ACCEPT] = SDLK_RETURN;
#if defined(__ANDROID__) || defined (__IPHONEOS__)
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

	binding[MAIN1] = binding_alt[MAIN1] = (SDL_BUTTON_LEFT+MOUSE_BIND_OFFSET) * (-1);
	binding[MAIN2] = binding_alt[MAIN2] = (SDL_BUTTON_RIGHT+MOUSE_BIND_OFFSET) * (-1);

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

void SDLInputState::handle() {
	InputState::handle();

	SDL_Event event;
	int bind_button = 0;
	bool joy_hat_event = false;

	/* Check for events */
	while (SDL_PollEvent (&event)) {

		if (dump_event) {
			std::cout << event << std::endl;
		}

		// grab symbol keys
		if (event.type == SDL_TEXTINPUT) {
			inkeys += event.text.text;
		}

		switch (event.type) {

#if !defined(__ANDROID__) && !defined (__IPHONEOS__)
			case SDL_MOUSEMOTION:
				mouse.x = event.motion.x;
				mouse.y = event.motion.y;
				break;
			case SDL_MOUSEWHEEL:
				if (event.wheel.y > 0) {
					scroll_up = true;
				} else if (event.wheel.y < 0) {
					scroll_down = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				mouse.x = event.button.x;
				mouse.y = event.button.y;
				bind_button = (event.button.button + MOUSE_BIND_OFFSET) * (-1);
				for (int key=0; key<key_count; key++) {
					if (bind_button == binding[key] || bind_button == binding_alt[key]) {
						pressing[key] = true;
						un_press[key] = false;
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				mouse.x = event.button.x;
				mouse.y = event.button.y;
				bind_button = (event.button.button + MOUSE_BIND_OFFSET) * (-1);
				for (int key=0; key<key_count; key++) {
					if (bind_button == binding[key] || bind_button == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_button = bind_button;
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					window_resized = true;
					render_device->windowResize();
				}
				break;
#else
			// detect restoring hidden Mobile app to bypass frameskip
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					window_resized = true;
					render_device->windowResize();
				}
				else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					logInfo("Minimizing app, saving...");
					save_load->saveGame();
					logInfo("Game saved");
					window_minimized = true;
					snd->pauseAll();
				}
				else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
					window_restored = true;
					snd->resumeAll();
				}
				break;
			// Mobile touch events
			case SDL_FINGERMOTION:
				mouse.x = static_cast<int>((event.tfinger.x + event.tfinger.dx) * VIEW_W);
				mouse.y = static_cast<int>((event.tfinger.y + event.tfinger.dy) * VIEW_H);

				if (event.tfinger.dy > 0) {
					scroll_up = true;
				} else if (event.tfinger.dy < 0) {
					scroll_down = true;
				}
				break;
			case SDL_FINGERDOWN:
				touch_locked = true;
				mouse.x = static_cast<int>(event.tfinger.x * VIEW_W);
				mouse.y = static_cast<int>(event.tfinger.y * VIEW_H);
				pressing[MAIN1] = true;
				un_press[MAIN1] = false;
				break;
			case SDL_FINGERUP:
				touch_locked = false;
				un_press[MAIN1] = true;
				last_button = binding[MAIN1];
				break;
#endif
			case SDL_KEYDOWN:
				for (int key=0; key<key_count; key++) {
					if (event.key.keysym.sym == binding[key] || event.key.keysym.sym == binding_alt[key]) {
						pressing[key] = true;
						un_press[key] = false;
					}
				}

				if (event.key.keysym.sym == SDLK_UP) pressing_up = true;
				if (event.key.keysym.sym == SDLK_DOWN) pressing_down = true;
				break;
			case SDL_KEYUP:
				for (int key=0; key<key_count; key++) {
					if (event.key.keysym.sym == binding[key] || event.key.keysym.sym == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_key = event.key.keysym.sym;

				if (event.key.keysym.sym == SDLK_UP) pressing_up = false;
				if (event.key.keysym.sym == SDLK_DOWN) pressing_down = false;
				break;
				/*
				case SDL_JOYAXISMOTION:
					// Reading joystick from SDL_JOYAXISMOTION is slow. Joystick analog input is handled by SDL_JoystickGetAxis() now.
					break;
				*/
			case SDL_JOYHATMOTION:
				if(JOYSTICK_DEVICE == event.jhat.which && ENABLE_JOYSTICK) {
					joy_hat_event = true;
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
	if(ENABLE_JOYSTICK && joy_axis_num > 0 && !joy_hat_event) {
		std::vector<bool> joy_axis_pressed;
		joy_axis_pressed.resize(joy_axis_num*2, false);
		last_joyaxis = -1;

		for (int i=0; i<joy_axis_num*2; i++) {
			int axis = SDL_JoystickGetAxis(joy, i/2);

			joy_axis_deltas[i] = (axis - joy_axis_prev[i])/2;
			joy_axis_prev[i] = axis;

			if (i % 2 == 0) {
				if (axis < -JOY_DEADZONE)
					joy_axis_pressed[i] = true;
				else if (axis <= JOY_DEADZONE)
					joy_axis_pressed[i] = false;
			}
			else {
				if (axis > JOY_DEADZONE)
					joy_axis_pressed[i] = true;
				else if (axis >= -JOY_DEADZONE)
					joy_axis_pressed[i] = false;
			}
		}
		for (int i=0; i<joy_axis_num*2; i++) {
			int bind_axis = (i+JOY_AXIS_OFFSET) * (-1);
			for (int key=0; key<key_count; key++) {
				if (bind_axis == binding_joy[key]) {
					if (joy_axis_pressed[i]) {
						pressing[key] = true;
						un_press[key] = false;
					}
					else {
						if (pressing[key]) {
							un_press[key] = true;
						}
						pressing[key] = false;
						lock[key] = false;
					}
				}
			}

			if (joy_axis_pressed[i] && joy_axis_deltas[i] != 0) {
				last_joyaxis = (i+JOY_AXIS_OFFSET) * (-1);
			}
		}
	}
}

void SDLInputState::hideCursor() {
	SDL_ShowCursor(SDL_DISABLE);
}

void SDLInputState::showCursor() {
	SDL_ShowCursor(SDL_ENABLE);
}

std::string SDLInputState::getJoystickName(int index) {
	return std::string(SDL_JoystickNameForIndex(index));
}

std::string SDLInputState::getKeyName(int key) {
	return std::string(SDL_GetKeyName((SDL_Keycode)key));
}

std::string SDLInputState::getBindingString(int key, int bindings_list) {
	std::string none = msg->get("(none)");

	if (bindings_list == INPUT_BINDING_DEFAULT) {
		if (inpt->binding[key] == 0 || inpt->binding[key] == -1)
			return none;
		else if (inpt->binding[key] < -1) {
			int real_button = (inpt->binding[key] + MOUSE_BIND_OFFSET) * (-1);

			if (real_button > 0 && real_button <= MOUSE_BUTTON_NAME_COUNT)
				return mouse_button[real_button - 1];
			else
				return msg->get("Mouse %d", real_button);
		}
		else
			return getKeyName(inpt->binding[key]);
	}
	else if (bindings_list == INPUT_BINDING_ALT) {
		if (inpt->binding_alt[key] == 0 || inpt->binding_alt[key] == -1)
			return none;
		else if (inpt->binding_alt[key] < -1) {
			int real_button = (inpt->binding_alt[key] + MOUSE_BIND_OFFSET) * (-1);

			if (real_button > 0 && real_button <= MOUSE_BUTTON_NAME_COUNT)
				return mouse_button[real_button - 1];
			else
				return msg->get("Mouse %d", real_button);
		}
		else
			return getKeyName(inpt->binding_alt[key]);
	}
	else if (bindings_list == INPUT_BINDING_JOYSTICK) {
		if (inpt->binding_joy[key] == -1)
			return none;
		else if (inpt->binding_joy[key] < -1) {
			int axis = (inpt->binding_joy[key] + JOY_AXIS_OFFSET) * (-1);

			if (axis % 2 == 0)
				return msg->get("Axis %d -", axis/2);
			else
				return msg->get("Axis %d +", axis/2);
		}
		else
			return msg->get("Button %d", inpt->binding_joy[key]);
	}
	else {
		return none;
	}
}

std::string SDLInputState::getMovementString() {
	std::stringstream ss;
	ss << "[";

	if (ENABLE_JOYSTICK) {
		// can't rebind joystick axes
		ss << inpt->getBindingString(LEFT, INPUT_BINDING_JOYSTICK) <<  "/";
		ss << inpt->getBindingString(RIGHT, INPUT_BINDING_JOYSTICK) << "/";
		ss << inpt->getBindingString(UP, INPUT_BINDING_JOYSTICK) << "/";
		ss << inpt->getBindingString(DOWN, INPUT_BINDING_JOYSTICK);
	}
	else if (TOUCHSCREEN) {
		ss << msg->get("%s on ground", msg->get("Tap"));
	}
	else if (MOUSE_MOVE) {
		ss << msg->get("%s on ground", inpt->getBindingString(MAIN1));
	}
	else {
		ss << inpt->getBindingString(LEFT) <<  "/";
		ss << inpt->getBindingString(RIGHT) << "/";
		ss << inpt->getBindingString(UP) << "/";
		ss << inpt->getBindingString(DOWN);
	}

	ss << "]";
	return ss.str();
}

std::string SDLInputState::getAttackString() {
	std::stringstream ss;
	ss << "[";

	if (ENABLE_JOYSTICK) {
		ss << inpt->getBindingString(ACTIONBAR_USE, INPUT_BINDING_JOYSTICK);
	}
	else if (TOUCHSCREEN) {
		ss << msg->get("%s on enemy", msg->get("Tap"));
	}
	else if (MOUSE_MOVE) {
		ss << msg->get("%s on enemy", inpt->getBindingString(MAIN1));
	}
	else {
		ss << inpt->getBindingString(MAIN1);
	}

	ss << "]";
	return ss.str();
}

std::string SDLInputState::getContinueString() {
	std::stringstream ss;
	ss << "[";

	if (TOUCHSCREEN) {
		ss << msg->get("Tap");
	}
	else {
		int binding_type = (ENABLE_JOYSTICK ? INPUT_BINDING_JOYSTICK : INPUT_BINDING_DEFAULT);
		ss << getBindingString(ACCEPT, binding_type);
	}

	ss << "]";
	return ss.str();
}

int SDLInputState::getNumJoysticks() {
	return joy_num;
}

SDLInputState::~SDLInputState() {
	if (joy)
		SDL_JoystickClose(joy);
}
