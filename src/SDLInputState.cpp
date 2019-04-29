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
#include "CursorManager.h"
#include "MenuManager.h"
#include "MessageEngine.h"
#include "Platform.h"
#include "RenderDevice.h"
#include "SDLInputState.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "UtilsDebug.h"
#include "UtilsParsing.h"

#include <math.h>

SDLInputState::SDLInputState(void)
	: InputState()
	, joy(NULL)
	, joy_num(0)
	, joy_axis_num(0)
	, resize_cooldown()
	, joystick_init(false)
	, text_input(false)
{
	platform.setExitEventFilter();

	defaultQwertyKeyBindings();
	defaultJoystickBindings();

	for (int key=0; key<KEY_COUNT; key++) {
		pressing[key] = false;
		un_press[key] = false;
		lock[key] = false;
	}

	loadKeyBindings();
	setKeybindNames();

	// print some information to the console about connected joysticks
	if(SDL_NumJoysticks() > 0) {
		Utils::logInfo("InputState: %d joystick(s) found.", SDL_NumJoysticks());
		joy_num = SDL_NumJoysticks();
	}
	else {
		Utils::logInfo("InputState: No joysticks were found.");
		settings->enable_joystick = false;
		return;
	}

	for(int i = 0; i < SDL_NumJoysticks(); i++) {
		Utils::logInfo("InputState: Joystick %d, %s", i, getJoystickName(i).c_str());
	}
}

void SDLInputState::initJoystick() {
	// close our joystick handle if it's open
	if (joy) {
		SDL_JoystickClose(joy);
		joy = NULL;
	}

	joy_num = SDL_NumJoysticks();
	if (settings->enable_joystick && joy_num > 0) {
		joy = SDL_JoystickOpen(settings->joystick_device);
		Utils::logInfo("InputState: Using joystick %d.", settings->joystick_device);
	}

	if (joy) {
		joy_axis_num = SDL_JoystickNumAxes(joy);
		joy_axis_prev.resize(joy_axis_num*2, 0);
		joy_axis_deltas.resize(joy_axis_num*2, 0);
	}
}

void SDLInputState::defaultQwertyKeyBindings () {
	if (platform.is_mobile_device) {
		binding[Input::CANCEL] = SDLK_AC_BACK;
		binding_alt[Input::ACCEPT] = SDLK_MENU;
	}
	else {
		binding[Input::CANCEL] = SDLK_ESCAPE;
		binding_alt[Input::ACCEPT] = SDLK_SPACE;
	}
	binding[Input::ACCEPT] = SDLK_RETURN;
	binding[Input::UP] = SDLK_w;
	binding[Input::DOWN] = SDLK_s;
	binding[Input::LEFT] = SDLK_a;
	binding[Input::RIGHT] = SDLK_d;

	binding_alt[Input::CANCEL] = SDLK_ESCAPE;
	binding_alt[Input::UP] = SDLK_UP;
	binding_alt[Input::DOWN] = SDLK_DOWN;
	binding_alt[Input::LEFT] = SDLK_LEFT;
	binding_alt[Input::RIGHT] = SDLK_RIGHT;

	binding[Input::BAR_1] = binding_alt[Input::BAR_1] = SDLK_q;
	binding[Input::BAR_2] = binding_alt[Input::BAR_2] = SDLK_e;
	binding[Input::BAR_3] = binding_alt[Input::BAR_3] = SDLK_r;
	binding[Input::BAR_4] = binding_alt[Input::BAR_4] = SDLK_f;
	binding[Input::BAR_5] = binding_alt[Input::BAR_5] = SDLK_1;
	binding[Input::BAR_6] = binding_alt[Input::BAR_6] = SDLK_2;
	binding[Input::BAR_7] = binding_alt[Input::BAR_7] = SDLK_3;
	binding[Input::BAR_8] = binding_alt[Input::BAR_8] = SDLK_4;
	binding[Input::BAR_9] = binding_alt[Input::BAR_9] = SDLK_5;
	binding[Input::BAR_0] = binding_alt[Input::BAR_0] = SDLK_6;

	binding[Input::CHARACTER] = binding_alt[Input::CHARACTER] = SDLK_c;
	binding[Input::INVENTORY] = binding_alt[Input::INVENTORY] = SDLK_i;
	binding[Input::POWERS] = binding_alt[Input::POWERS] = SDLK_p;
	binding[Input::LOG] = binding_alt[Input::LOG] = SDLK_l;

	binding[Input::MAIN1] = binding_alt[Input::MAIN1] = (SDL_BUTTON_LEFT+MOUSE_BIND_OFFSET) * (-1);
	binding[Input::MAIN2] = binding_alt[Input::MAIN2] = (SDL_BUTTON_RIGHT+MOUSE_BIND_OFFSET) * (-1);

	binding[Input::CTRL] = SDLK_LCTRL;
	binding_alt[Input::CTRL] = SDLK_RCTRL;
	binding[Input::SHIFT] = SDLK_LSHIFT;
	binding_alt[Input::SHIFT] = SDLK_RSHIFT;
	binding[Input::DEL] = SDLK_DELETE;
	binding_alt[Input::DEL] = SDLK_BACKSPACE;
	binding[Input::ALT] = SDLK_LALT;
	binding_alt[Input::ALT] = SDLK_RALT;

	binding[Input::ACTIONBAR] = binding_alt[Input::ACTIONBAR] = SDLK_b;
	binding[Input::ACTIONBAR_BACK] = binding_alt[Input::ACTIONBAR_BACK] = SDLK_z;
	binding[Input::ACTIONBAR_FORWARD] = binding_alt[Input::ACTIONBAR_FORWARD] = SDLK_x;
	binding[Input::ACTIONBAR_USE] = binding_alt[Input::ACTIONBAR_USE] = SDLK_n;

	binding[Input::DEVELOPER_MENU] = binding_alt[Input::DEVELOPER_MENU] = SDLK_F5;

	// Convert SDL_Keycode to SDL_Scancode, skip mouse binding
	for (int key=0; key<KEY_COUNT; key++) {
		if (SDL_GetScancodeFromKey(binding[key]) > 0) binding[key] = SDL_GetScancodeFromKey(binding[key]);
		if (SDL_GetScancodeFromKey(binding_alt[key]) > 0) binding_alt[key] = SDL_GetScancodeFromKey(binding_alt[key]);
	}
}

void SDLInputState::setFixedKeyBindings() {
	validateFixedKeyBinding(Input::MAIN1, (SDL_BUTTON_LEFT+MOUSE_BIND_OFFSET) * (-1), InputState::BINDING_DEFAULT);

	validateFixedKeyBinding(Input::CTRL, SDLK_LCTRL, InputState::BINDING_DEFAULT);
	validateFixedKeyBinding(Input::CTRL, SDLK_RCTRL, InputState::BINDING_ALT);
	binding_joy[Input::CTRL] = -1;

	validateFixedKeyBinding(Input::SHIFT, SDLK_LSHIFT, InputState::BINDING_DEFAULT);
	validateFixedKeyBinding(Input::SHIFT, SDLK_RSHIFT, InputState::BINDING_ALT);
	binding_joy[Input::SHIFT] = -1;

	validateFixedKeyBinding(Input::DEL, SDLK_DELETE, InputState::BINDING_DEFAULT);
	validateFixedKeyBinding(Input::DEL, SDLK_BACKSPACE, InputState::BINDING_ALT);
	binding_joy[Input::DEL] = -1;

	validateFixedKeyBinding(Input::ALT, SDLK_LALT, InputState::BINDING_DEFAULT);
	validateFixedKeyBinding(Input::ALT, SDLK_RALT, InputState::BINDING_ALT);
	binding_joy[Input::ALT] = -1;
}

void SDLInputState::validateFixedKeyBinding(int action, int key, int bindings_list) {
	if (bindings_list != InputState::BINDING_DEFAULT && bindings_list != InputState::BINDING_ALT)
		return;

	int scan_key = (key < 0 ? key : SDL_GetScancodeFromKey(key));

	for (int i = 0; i < KEY_COUNT; ++i) {
		if (i == action) {
			if (bindings_list == InputState::BINDING_DEFAULT)
				binding[action] = scan_key;
			else if (bindings_list == InputState::BINDING_ALT)
				binding_alt[action] = scan_key;

			continue;
		}

		if (binding[i] == scan_key)
			binding[i] = -1;

		if (binding_alt[i] == scan_key)
			binding_alt[i] = -1;
	}
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
				if (!platform.is_mobile_device) {
					mouse = scaleMouse(event.motion.x, event.motion.y);
					curs->show_cursor = true;
				}
				break;
			case SDL_MOUSEWHEEL:
				last_is_joystick = false;
				if (!platform.is_mobile_device) {
					if (event.wheel.y > 0) {
						scroll_up = true;
					} else if (event.wheel.y < 0) {
						scroll_down = true;
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				last_is_joystick = false;
				if (!platform.is_mobile_device) {
					mouse = scaleMouse(event.button.x, event.button.y);
					bind_button = (event.button.button + MOUSE_BIND_OFFSET) * (-1);
					for (int key=0; key<KEY_COUNT; key++) {
						if (bind_button == binding[key] || bind_button == binding_alt[key]) {
							pressing[key] = true;
							un_press[key] = false;
						}
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				last_is_joystick = false;
				if (!platform.is_mobile_device) {
					mouse = scaleMouse(event.button.x, event.button.y);
					bind_button = (event.button.button + MOUSE_BIND_OFFSET) * (-1);
					for (int key=0; key<KEY_COUNT; key++) {
						if (bind_button == binding[key] || bind_button == binding_alt[key]) {
							un_press[key] = true;
						}
					}
					last_button = bind_button;
				}
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					resize_cooldown.setDuration(settings->max_frames_per_sec / 4);
				}
				else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					if (platform.is_mobile_device) {
						// on mobile, we the user could kill the app, so save the game beforehand
						Utils::logInfo("InputState: Minimizing app, saving...");
						save_load->saveGame();
						Utils::logInfo("InputState: Game saved");
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
				if (platform.is_mobile_device) {
					mouse.x = static_cast<int>((event.tfinger.x + event.tfinger.dx) * settings->view_w);
					mouse.y = static_cast<int>((event.tfinger.y + event.tfinger.dy) * settings->view_h);

					if (event.tfinger.dy > 0) {
						scroll_up = true;
					} else if (event.tfinger.dy < 0) {
						scroll_down = true;
					}

					for (size_t i = 0; i < touch_fingers.size(); ++i) {
						if (touch_fingers[i].id == event.tfinger.fingerId) {
							touch_fingers[i].pos.x = mouse.x;
							touch_fingers[i].pos.y = mouse.y;
						}
					}
				}
				break;
			case SDL_FINGERDOWN:
				last_is_joystick = false;
				if (platform.is_mobile_device) {
					touch_locked = true;
					mouse.x = static_cast<int>(event.tfinger.x * settings->view_w);
					mouse.y = static_cast<int>(event.tfinger.y * settings->view_h);
					pressing[Input::MAIN1] = true;
					un_press[Input::MAIN1] = false;

					FingerData fd;
					fd.id = static_cast<long int>(event.tfinger.fingerId);
					fd.pos.x = mouse.x;
					fd.pos.y = mouse.y;
					touch_fingers.push_back(fd);
				}
				break;
			case SDL_FINGERUP:
				last_is_joystick = false;
				if (platform.is_mobile_device) {
					for (size_t i = 0; i < touch_fingers.size(); ++i) {
						if (touch_fingers[i].id == event.tfinger.fingerId) {
							touch_fingers.erase(touch_fingers.begin() + i);
							break;
						}
					}
					if (touch_fingers.empty()) {
						touch_locked = false;
						un_press[Input::MAIN1] = true;
						last_button = binding[Input::MAIN1];
					}
					else {
						mouse.x = touch_fingers.back().pos.x;
						mouse.y = touch_fingers.back().pos.y;
					}
				}
				break;

			case SDL_KEYDOWN:
				if (!settings->no_mouse)
					last_is_joystick = false;

				for (int key=0; key<KEY_COUNT; key++) {
					if (event.key.keysym.scancode == binding[key] || event.key.keysym.scancode == binding_alt[key]) {
						pressing[key] = true;
						un_press[key] = false;
					}
				}

				if (event.key.keysym.sym == SDLK_UP) pressing_up = true;
				if (event.key.keysym.sym == SDLK_DOWN) pressing_down = true;
				break;
			case SDL_KEYUP:
				if (!settings->no_mouse)
					last_is_joystick = false;

				for (int key=0; key<KEY_COUNT; key++) {
					if (event.key.keysym.scancode == binding[key] || event.key.keysym.scancode == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_key = event.key.keysym.scancode;

				if (event.key.keysym.sym == SDLK_UP) pressing_up = false;
				if (event.key.keysym.sym == SDLK_DOWN) pressing_down = false;
				break;
			case SDL_JOYHATMOTION:
				if (joy && SDL_JoystickInstanceID(joy) == event.jhat.which && settings->enable_joystick) {
					last_is_joystick = true;
					curs->show_cursor = false;
					hideCursor();
					joy_hat_event = true;
					switch (event.jhat.value) {
						case SDL_HAT_CENTERED:
							un_press[Input::UP] = true;
							un_press[Input::DOWN] = true;
							un_press[Input::LEFT] = true;
							un_press[Input::RIGHT] = true;
							break;
						case SDL_HAT_UP:
							pressing[Input::UP] = true;
							un_press[Input::UP] = false;
							pressing[Input::DOWN] = false;
							lock[Input::DOWN] = false;
							pressing[Input::LEFT] = false;
							lock[Input::LEFT] = false;
							pressing[Input::RIGHT] = false;
							lock[Input::RIGHT] = false;
							break;
						case SDL_HAT_DOWN:
							pressing[Input::UP] = false;
							lock[Input::UP] = false;
							pressing[Input::DOWN] = true;
							un_press[Input::DOWN] = false;
							pressing[Input::LEFT] = false;
							lock[Input::LEFT] = false;
							pressing[Input::RIGHT] = false;
							lock[Input::RIGHT] = false;
							break;
						case SDL_HAT_LEFT:
							pressing[Input::UP] = false;
							lock[Input::UP] = false;
							pressing[Input::DOWN] = false;
							lock[Input::DOWN] = false;
							pressing[Input::LEFT] = true;
							un_press[Input::LEFT] = false;
							pressing[Input::RIGHT] = false;
							lock[Input::RIGHT] = false;
							break;
						case SDL_HAT_RIGHT:
							pressing[Input::UP] = false;
							lock[Input::UP] = false;
							pressing[Input::DOWN] = false;
							lock[Input::DOWN] = false;
							pressing[Input::LEFT] = false;
							lock[Input::LEFT] = false;
							pressing[Input::RIGHT] = true;
							un_press[Input::RIGHT] = false;
							break;
						case SDL_HAT_LEFTUP:
							pressing[Input::UP] = true;
							un_press[Input::UP] = false;
							pressing[Input::DOWN] = false;
							lock[Input::DOWN] = false;
							pressing[Input::LEFT] = true;
							un_press[Input::LEFT] = false;
							pressing[Input::RIGHT] = false;
							lock[Input::RIGHT] = false;
							break;
						case SDL_HAT_LEFTDOWN:
							pressing[Input::UP] = false;
							lock[Input::UP] = false;
							pressing[Input::DOWN] = true;
							un_press[Input::DOWN] = false;
							pressing[Input::LEFT] = true;
							un_press[Input::LEFT] = false;
							pressing[Input::RIGHT] = false;
							lock[Input::RIGHT] = false;
							break;
						case SDL_HAT_RIGHTUP:
							pressing[Input::UP] = true;
							un_press[Input::UP] = false;
							pressing[Input::DOWN] = false;
							lock[Input::DOWN] = false;
							pressing[Input::LEFT] = false;
							lock[Input::LEFT] = false;
							pressing[Input::RIGHT] = true;
							un_press[Input::RIGHT] = false;
							break;
						case SDL_HAT_RIGHTDOWN:
							pressing[Input::UP] = false;
							lock[Input::UP] = false;
							pressing[Input::DOWN] = true;
							un_press[Input::DOWN] = false;
							pressing[Input::LEFT] = false;
							lock[Input::LEFT] = false;
							pressing[Input::RIGHT] = true;
							un_press[Input::RIGHT] = false;
							break;
					}
				}
				break;
			case SDL_JOYBUTTONDOWN:
				if (joy && SDL_JoystickInstanceID(joy) == event.jbutton.which && settings->enable_joystick) {
					for (int key=0; key<KEY_COUNT; key++) {
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
				if (joy && SDL_JoystickInstanceID(joy) == event.jbutton.which && settings->enable_joystick) {
					for (int key=0; key<KEY_COUNT; key++) {
						if (event.jbutton.button == binding_joy[key]) {
							last_is_joystick = true;
							un_press[key] = true;
						}
						last_joybutton = event.jbutton.button;
					}
				}
				break;
			case SDL_JOYAXISMOTION:
				if(settings->enable_joystick && joy_axis_num > 0 && !joy_hat_event) {
					std::vector<bool> joy_axis_pressed;
					joy_axis_pressed.resize(joy_axis_num*2, false);
					last_joyaxis = -1;

					for (int i=0; i<joy_axis_num*2; i++) {
						int axis = SDL_JoystickGetAxis(joy, i/2);

						joy_axis_deltas[i] = (axis - joy_axis_prev[i])/2;
						joy_axis_prev[i] = axis;

						if (i % 2 == 0) {
							if (axis < -(settings->joy_deadzone))
								joy_axis_pressed[i] = true;
							else if (axis <= settings->joy_deadzone)
								joy_axis_pressed[i] = false;
						}
						else {
							if (axis > settings->joy_deadzone)
								joy_axis_pressed[i] = true;
							else if (axis >= -(settings->joy_deadzone))
								joy_axis_pressed[i] = false;
						}
					}
					for (int i=0; i<joy_axis_num*2; i++) {
						int bind_axis = (i+JOY_AXIS_OFFSET) * (-1);
						for (int key=0; key<KEY_COUNT; key++) {
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
					Utils::logInfo("InputState: Joystick added.");
					joysticks_changed = true;
					initJoystick();
				}
				break;
			case SDL_JOYDEVICEREMOVED:
				Utils::logInfo("InputState: Joystick removed.");
				joysticks_changed = true;
				settings->enable_joystick = false;
				initJoystick();
				break;
			case SDL_QUIT:
				done = 1;
				break;
			default:
				break;
		}
	}

	if (resize_cooldown.getDuration() > 0) {
		resize_cooldown.tick();

		if (resize_cooldown.isEnd()) {
			resize_cooldown.setDuration(0);
			window_resized = true;
			render_device->windowResize();
		}
	}

	// this flag is used to guard against removing and then adding an already connected controller on startup
	// once this function runs once, it is assumed startup is finished
	if (!joystick_init)
		joystick_init = true;

	// handle slow key repeating
	for (int i = 0; i < KEY_COUNT; i++) {
		if (slow_repeat[i]) {
			if (!pressing[i]) {
				// key not pressed, reset delay
				repeat_cooldown[i].setDuration(settings->max_frames_per_sec);
			}
			else if (pressing[i] && !lock[i]) {
				// lock key and set delay
				lock[i] = true;
				int prev_duration = static_cast<int>(repeat_cooldown[i].getDuration());
				repeat_cooldown[i].setDuration(std::max(settings->max_frames_per_sec / 10, prev_duration - (settings->max_frames_per_sec / 2)));
			}
			else if (pressing[i] && lock[i]) {
				// delay unlocking key
				repeat_cooldown[i].tick();

				if (repeat_cooldown[i].isEnd()) {
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
	key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(key));
	// first, we try to provide a translation of the key
	switch (static_cast<SDL_Keycode>(key)) {
		case SDLK_BACKSPACE:    return msg->get("Backspace");
		case SDLK_CAPSLOCK:     return msg->get("CapsLock");
		case SDLK_DELETE:       return msg->get("Delete");
		case SDLK_DOWN:         return msg->get("Down");
		case SDLK_END:          return msg->get("End");
		case SDLK_ESCAPE:       return msg->get("Escape");
		case SDLK_HOME:         return msg->get("Home");
		case SDLK_INSERT:       return msg->get("Insert");
		case SDLK_LALT:         return msg->get("Left Alt");
		case SDLK_LCTRL:        return msg->get("Left Ctrl");
		case SDLK_LEFT:         return msg->get("Left");
		case SDLK_LSHIFT:       return msg->get("Left Shift");
		case SDLK_NUMLOCKCLEAR: return msg->get("NumLock");
		case SDLK_PAGEDOWN:     return msg->get("PageDown");
		case SDLK_PAGEUP:       return msg->get("PageUp");
		case SDLK_PAUSE:        return msg->get("Pause");
		case SDLK_PRINTSCREEN:  return msg->get("PrintScreen");
		case SDLK_RALT:         return msg->get("Right Alt");
		case SDLK_RCTRL:        return msg->get("Right Ctrl");
		case SDLK_RETURN:       return msg->get("Return");
		case SDLK_RIGHT:        return msg->get("Right");
		case SDLK_RSHIFT:       return msg->get("Right Shift");
		case SDLK_SCROLLLOCK:   return msg->get("ScrollLock");
		case SDLK_SPACE:        return msg->get("Space");
		case SDLK_TAB:          return msg->get("Tab");
		case SDLK_UP:           return msg->get("Up");
	}

	// no translation for this key, so just get the name straight from SDL
	return std::string(SDL_GetKeyName(static_cast<SDL_Keycode>(key)));
}

std::string SDLInputState::getMouseButtonName(int button) {
	int real_button = (button + MOUSE_BIND_OFFSET) * (-1);

	if (real_button > 0 && real_button <= MOUSE_BUTTON_NAME_COUNT)
		return mouse_button[real_button - 1];
	else
		return msg->get("Mouse %d", real_button);
}

std::string SDLInputState::getJoystickButtonName(int button) {
	if (button < -1) {
		int axis = (button + JOY_AXIS_OFFSET) * (-1);

		if (axis % 2 == 0)
			return msg->get("Axis %d -", axis/2);
		else
			return msg->get("Axis %d +", axis/2);
	}
	else
		return msg->get("Button %d", button);
}

std::string SDLInputState::getBindingString(int key, int bindings_list) {
	std::string none = msg->get("(none)");

	if (bindings_list == InputState::BINDING_DEFAULT) {
		if (binding[key] == 0 || binding[key] == -1)
			return none;
		else if (binding[key] < -1)
			return getMouseButtonName(binding[key]);
		else
			return getKeyName(binding[key]);
	}
	else if (bindings_list == InputState::BINDING_ALT) {
		if (binding_alt[key] == 0 || binding_alt[key] == -1)
			return none;
		else if (binding_alt[key] < -1)
			return getMouseButtonName(binding_alt[key]);
		else
			return getKeyName(binding_alt[key]);
	}
	else if (bindings_list == InputState::BINDING_JOYSTICK) {
		if (binding_joy[key] == -1)
			return none;
		else
			return getJoystickButtonName(binding_joy[key]);
	}
	else {
		return none;
	}
}

std::string SDLInputState::getMovementString() {
	std::stringstream ss;
	ss << "[";

	if (settings->enable_joystick) {
		ss << getBindingString(Input::LEFT, InputState::BINDING_JOYSTICK) <<  "/";
		ss << getBindingString(Input::RIGHT, InputState::BINDING_JOYSTICK) << "/";
		ss << getBindingString(Input::UP, InputState::BINDING_JOYSTICK) << "/";
		ss << getBindingString(Input::DOWN, InputState::BINDING_JOYSTICK);
	}
	else if (settings->touchscreen) {
		ss << msg->get("Touch control D-Pad");
	}
	else if (settings->mouse_move) {
		ss << (settings->mouse_move_swap ? getBindingString(Input::MAIN2) : getBindingString(Input::MAIN1));
	}
	else {
		ss << getBindingString(Input::LEFT) <<  "/";
		ss << getBindingString(Input::RIGHT) << "/";
		ss << getBindingString(Input::UP) << "/";
		ss << getBindingString(Input::DOWN);
	}

	ss << "]";
	return ss.str();
}

std::string SDLInputState::getAttackString() {
	std::stringstream ss;
	ss << "[";

	if (settings->enable_joystick) {
		ss << getBindingString(Input::ACTIONBAR_USE, InputState::BINDING_JOYSTICK);
	}
	else if (settings->touchscreen) {
		ss << msg->get("Touch control buttons");
	}
	else {
		ss << getBindingString(Input::MAIN1);
	}

	ss << "]";
	return ss.str();
}

std::string SDLInputState::getContinueString() {
	std::stringstream ss;
	ss << "[";

	if (settings->touchscreen) {
		ss << msg->get("Tap");
	}
	else {
		int binding_type = (settings->enable_joystick ? InputState::BINDING_JOYSTICK : InputState::BINDING_DEFAULT);
		ss << getBindingString(Input::ACCEPT, binding_type);
	}

	ss << "]";
	return ss.str();
}

int SDLInputState::getNumJoysticks() {
	return joy_num;
}

bool SDLInputState::usingMouse() {
	return !settings->no_mouse && !last_is_joystick;
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

void SDLInputState::setKeybind(int key, int binding_button, int bindings_list, std::string& keybind_msg) {
	keybind_msg = "";

	// unbind duplicate bindings for this key
	if (key != -1) {

		// prevent unmapping "fixed" keybinds
		if (bindings_list != InputState::BINDING_JOYSTICK) {
			if ((key == ((SDL_BUTTON_LEFT+MOUSE_BIND_OFFSET) * (-1)) && binding_button != Input::MAIN1) ||
				key == SDL_SCANCODE_LCTRL ||
				key == SDL_SCANCODE_RCTRL ||
				key == SDL_SCANCODE_LSHIFT ||
				key == SDL_SCANCODE_RSHIFT ||
				key == SDL_SCANCODE_LALT ||
				key == SDL_SCANCODE_RALT ||
				key == SDL_SCANCODE_DELETE ||
				key == SDL_SCANCODE_BACKSPACE) {

				if (key < -1)
					keybind_msg = msg->get("Can not bind: %s", getMouseButtonName(key).c_str());
				else
					keybind_msg = msg->get("Can not bind: %s", getKeyName(key).c_str());

				return;
			}
		}

		for (int i = 0; i < KEY_COUNT; ++i) {
			// the same key can be bound to both default & alt binding lists for the same action
			if (bindings_list != InputState::BINDING_JOYSTICK && i == binding_button)
				continue;

			if ((bindings_list == InputState::BINDING_DEFAULT && binding[i] == key && i != binding_button) || (bindings_list == InputState::BINDING_ALT && binding[i] == key)) {
				keybind_msg = msg->get("'%s' is no longer bound to:", getBindingString(i, InputState::BINDING_DEFAULT)) + " '" + binding_name[i] + "'";
				binding[i] = -1;
			}
			if ((bindings_list == InputState::BINDING_DEFAULT && binding_alt[i] == key) || (bindings_list == InputState::BINDING_ALT && binding_alt[i] == key && i != binding_button)) {
				keybind_msg = msg->get("'%s' is no longer bound to:", getBindingString(i, InputState::BINDING_ALT)) + " '" + binding_name[i] + "'";
				binding_alt[i] = -1;
			}
			if (bindings_list == InputState::BINDING_JOYSTICK && binding_joy[i] == key && i != binding_button) {
				keybind_msg = msg->get("'%s' is no longer bound to:", getBindingString(i, InputState::BINDING_JOYSTICK)) + " '" + binding_name[i] + "'";
				binding_joy[i] = -1;
			}
		}
	}

	if (bindings_list == InputState::BINDING_DEFAULT)
		binding[binding_button] = key;
	else if (bindings_list == InputState::BINDING_ALT)
		binding_alt[binding_button] = key;
	else if (bindings_list == InputState::BINDING_JOYSTICK)
		binding_joy[binding_button] = key;
}

int SDLInputState::getKeyFromName(const std::string& key_name) {
	return SDL_GetKeyFromName(key_name.c_str());
}

SDLInputState::~SDLInputState() {
	if (joy)
		SDL_JoystickClose(joy);
}
