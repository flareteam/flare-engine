/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2015-2016 Justin Jacobs

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
#include "MenuManager.h"
#include "Platform.h"
#include "SDLInputState.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsDebug.h"
#include "UtilsParsing.h"

#include <math.h>

SDLInputState::SDLInputState(void)
	: InputState()
	, joy(NULL)
	, joy_num(0)
	, joy_axis_num(0)
	, resize_ticks(-1)
	, joystick_init(false)
	, text_input(false)
{
	PlatformSetExitEventFilter();

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
		logInfo("InputState: %d joystick(s) found.", SDL_NumJoysticks());
		joy_num = SDL_NumJoysticks();
	}
	else {
		logInfo("InputState: No joysticks were found.");
		ENABLE_JOYSTICK = false;
		return;
	}

	for(int i = 0; i < SDL_NumJoysticks(); i++) {
		logInfo("InputState: Joystick %d, %s", i, getJoystickName(i).c_str());
	}
}

void SDLInputState::initJoystick() {
	// close our joystick handle if it's open
	if (joy) {
		SDL_JoystickClose(joy);
		joy = NULL;
	}

	joy_num = SDL_NumJoysticks();
	if (ENABLE_JOYSTICK && joy_num > 0) {
		joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
		logInfo("InputState: Using joystick %d.", JOYSTICK_DEVICE);
	}

	if (joy) {
		joy_axis_num = SDL_JoystickNumAxes(joy);
		joy_axis_prev.resize(joy_axis_num*2, 0);
		joy_axis_deltas.resize(joy_axis_num*2, 0);
	}
}

void SDLInputState::defaultQwertyKeyBindings () {
	if (platform_options.is_mobile_device) {
		binding[CANCEL] = SDLK_AC_BACK;
		binding_alt[ACCEPT] = SDLK_MENU;
	}
	else {
		binding[CANCEL] = SDLK_ESCAPE;
		binding_alt[ACCEPT] = SDLK_SPACE;
	}
	binding[ACCEPT] = SDLK_RETURN;
	binding[UP] = SDLK_w;
	binding[DOWN] = SDLK_s;
	binding[LEFT] = SDLK_a;
	binding[RIGHT] = SDLK_d;

	binding_alt[CANCEL] = SDLK_ESCAPE;
	binding_alt[UP] = SDLK_UP;
	binding_alt[DOWN] = SDLK_DOWN;
	binding_alt[LEFT] = SDLK_LEFT;
	binding_alt[RIGHT] = SDLK_RIGHT;

	binding[BAR_1] = binding_alt[BAR_1] = SDLK_q;
	binding[BAR_2] = binding_alt[BAR_2] = SDLK_e;
	binding[BAR_3] = binding_alt[BAR_3] = SDLK_r;
	binding[BAR_4] = binding_alt[BAR_4] = SDLK_f;
	binding[BAR_5] = binding_alt[BAR_5] = SDLK_1;
	binding[BAR_6] = binding_alt[BAR_6] = SDLK_2;
	binding[BAR_7] = binding_alt[BAR_7] = SDLK_3;
	binding[BAR_8] = binding_alt[BAR_8] = SDLK_4;
	binding[BAR_9] = binding_alt[BAR_9] = SDLK_5;
	binding[BAR_0] = binding_alt[BAR_0] = SDLK_6;

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
			case SDL_MOUSEMOTION:
				last_is_joystick = false;
				if (!platform_options.is_mobile_device) {
					mouse = scaleMouse(event.motion.x, event.motion.y);
					curs->show_cursor = true;
				}
				break;
			case SDL_MOUSEWHEEL:
				last_is_joystick = false;
				if (!platform_options.is_mobile_device) {
					if (event.wheel.y > 0) {
						scroll_up = true;
					} else if (event.wheel.y < 0) {
						scroll_down = true;
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				last_is_joystick = false;
				if (!platform_options.is_mobile_device) {
					mouse = scaleMouse(event.button.x, event.button.y);
					bind_button = (event.button.button + MOUSE_BIND_OFFSET) * (-1);
					for (int key=0; key<key_count; key++) {
						if (bind_button == binding[key] || bind_button == binding_alt[key]) {
							pressing[key] = true;
							un_press[key] = false;
						}
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				last_is_joystick = false;
				if (!platform_options.is_mobile_device) {
					mouse = scaleMouse(event.button.x, event.button.y);
					bind_button = (event.button.button + MOUSE_BIND_OFFSET) * (-1);
					for (int key=0; key<key_count; key++) {
						if (bind_button == binding[key] || bind_button == binding_alt[key]) {
							un_press[key] = true;
						}
					}
					last_button = bind_button;
				}
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					resize_ticks = MAX_FRAMES_PER_SEC/4;
				}
				else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					if (platform_options.is_mobile_device) {
						// on mobile, we the user could kill the app, so save the game beforehand
						logInfo("InputState: Minimizing app, saving...");
						save_load->saveGame();
						logInfo("InputState: Game saved");
					}
					window_minimized = true;
					snd->pauseAll();
					if (menu)
						menu->showExitMenu();
				}
				else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
					window_restored = true;
					snd->resumeAll();
				}
				break;

			// Mobile touch events
			// NOTE Should these be limited to mobile only?
			case SDL_FINGERMOTION:
				last_is_joystick = false;
				if (platform_options.is_mobile_device) {
					mouse.x = static_cast<int>((event.tfinger.x + event.tfinger.dx) * VIEW_W);
					mouse.y = static_cast<int>((event.tfinger.y + event.tfinger.dy) * VIEW_H);

					if (event.tfinger.dy > 0) {
						scroll_up = true;
					} else if (event.tfinger.dy < 0) {
						scroll_down = true;
					}
				}
				break;
			case SDL_FINGERDOWN:
				last_is_joystick = false;
				if (platform_options.is_mobile_device) {
					touch_locked = true;
					mouse.x = static_cast<int>(event.tfinger.x * VIEW_W);
					mouse.y = static_cast<int>(event.tfinger.y * VIEW_H);
					pressing[MAIN1] = true;
					un_press[MAIN1] = false;
				}
				break;
			case SDL_FINGERUP:
				last_is_joystick = false;
				if (platform_options.is_mobile_device) {
					touch_locked = false;
					un_press[MAIN1] = true;
					last_button = binding[MAIN1];
				}
				break;

			case SDL_KEYDOWN:
				if (!NO_MOUSE)
					last_is_joystick = false;

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
				if (!NO_MOUSE)
					last_is_joystick = false;

				for (int key=0; key<key_count; key++) {
					if (event.key.keysym.sym == binding[key] || event.key.keysym.sym == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_key = event.key.keysym.sym;

				if (event.key.keysym.sym == SDLK_UP) pressing_up = false;
				if (event.key.keysym.sym == SDLK_DOWN) pressing_down = false;
				break;
			case SDL_JOYHATMOTION:
				if (joy && SDL_JoystickInstanceID(joy) == event.jhat.which && ENABLE_JOYSTICK) {
					last_is_joystick = true;
					curs->show_cursor = false;
					hideCursor();
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
				if (joy && SDL_JoystickInstanceID(joy) == event.jbutton.which && ENABLE_JOYSTICK) {
					for (int key=0; key<key_count; key++) {
						if (event.jbutton.button == binding_joy[key]) {
							last_is_joystick = true;
							curs->show_cursor = false;
							hideCursor();
							pressing[key] = true;
							un_press[key] = false;
						}
					}
				}
				break;
			case SDL_JOYBUTTONUP:
				if (joy && SDL_JoystickInstanceID(joy) == event.jbutton.which && ENABLE_JOYSTICK) {
					for (int key=0; key<key_count; key++) {
						if (event.jbutton.button == binding_joy[key]) {
							last_is_joystick = true;
							un_press[key] = true;
						}
						last_joybutton = event.jbutton.button;
					}
				}
				break;
			case SDL_JOYAXISMOTION:
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
							last_is_joystick = true;
							curs->show_cursor = false;
							hideCursor();
							last_joyaxis = (i+JOY_AXIS_OFFSET) * (-1);
						}
					}
				}
				break;
			case SDL_JOYDEVICEADDED:
				if (!joystick_init) {
					joystick_init = true;
				}
				else {
					logInfo("InputState: Joystick added.");
					joysticks_changed = true;
					initJoystick();
				}
				break;
			case SDL_JOYDEVICEREMOVED:
				logInfo("InputState: Joystick removed.");
				joysticks_changed = true;
				ENABLE_JOYSTICK = false;
				initJoystick();
				break;
			case SDL_QUIT:
				done = 1;
				break;
			default:
				break;
		}
	}

	if (resize_ticks > 0) {
		resize_ticks--;
	}
	if (resize_ticks == 0) {
		resize_ticks = -1;
		window_resized = true;
		render_device->windowResize();
	}

	// this flag is used to guard against removing and then adding an already connected controller on startup
	// once this function runs once, it is assumed startup is finished
	if (!joystick_init)
		joystick_init = true;

	// handle slow key repeating
	for (int i = 0; i < key_count; i++) {
		if (slow_repeat[i]) {
			if (!pressing[i]) {
				// key not pressed, reset delay
				max_repeat_ticks[i] = MAX_FRAMES_PER_SEC;
			}
			else if (pressing[i] && !lock[i]) {
				// lock key and set delay
				lock[i] = true;
				repeat_ticks[i] = 0;
				max_repeat_ticks[i] = std::max(MAX_FRAMES_PER_SEC/10, max_repeat_ticks[i] - (MAX_FRAMES_PER_SEC/2));
			}
			else if (pressing[i] && lock[i]) {
				// delay unlocking key
				repeat_ticks[i]++;

				if (repeat_ticks[i] >= max_repeat_ticks[i]) {
					lock[i] = false;
				}
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

bool SDLInputState::usingMouse() {
	return !NO_MOUSE && !last_is_joystick;
}

void SDLInputState::startTextInput() {
	if (!text_input) {
		SDL_StartTextInput();
		text_input = true;
	}
}

void SDLInputState::stopTextInput() {
	if (text_input) {
		SDL_StopTextInput();
		text_input = false;
	}
}

SDLInputState::~SDLInputState() {
	if (joy)
		SDL_JoystickClose(joy);
}
