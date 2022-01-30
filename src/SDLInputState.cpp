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
#include "InputState.h"
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
	, resize_cooldown()
	, joystick_init(false)
	, text_input(false)
	, gamepad(NULL)
{
	platform.setExitEventFilter();

	restricted_bindings.push_back(InputBind(InputBind::MOUSE, SDL_BUTTON_LEFT));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_LCTRL));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_RCTRL));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_LSHIFT));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_RSHIFT));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_LALT));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_RALT));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_DELETE));
	restricted_bindings.push_back(InputBind(InputBind::KEY, SDL_SCANCODE_BACKSPACE));

	initBindings();

	for (int key=0; key<KEY_COUNT; key++) {
		pressing[key] = false;
		un_press[key] = false;
		lock[key] = false;
	}

	loadKeyBindings();
	setCommonStrings();

	// get the joystick ids for all valid game controllers
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			gamepad_ids.push_back(i);
		}
	}

	initJoystick();
}

void SDLInputState::setBind(int action, int type, int bind, std::string *keybind_msg) {
	if (action < 0 || action >= KEY_COUNT)
		return;

	for (size_t i = 0; i < binding[action].size(); ++i) {
		// exact binding already exists for this action
		if (binding[action][i].type == type && binding[action][i].bind == bind)
			return;
	}

	if (keybind_msg) {
		// prevent binding reserved keys
		for (size_t i = 0; i < restricted_bindings.size(); ++i) {
			if (type == restricted_bindings[i].type && bind == restricted_bindings[i].bind) {
				*keybind_msg = msg->get("Can not bind: %s", getInputBindName(type, bind));
				return;
			}
		}

		// unbind other actions that have this same bind
		for (int i = 0; i < KEY_COUNT_USER; ++i) {
			for (size_t j = binding[i].size(); j > 0; --j) {
				if (type == binding[i][j-1].type && bind == binding[i][j-1].bind) {
					*keybind_msg = msg->get("'%s' is no longer bound to:", getInputBindName(type, bind)) + " '" + binding_name[i] + "'";
					removeBind(i, j-1);
				}
			}
		}
	}

	InputBind input_bind;
	input_bind.type = type;
	input_bind.bind = bind;

	binding[action].push_back(input_bind);
}

void SDLInputState::removeBind(int action, size_t index) {
	if (action < 0 || action >= KEY_COUNT)
		return;

	if (index >= binding[action].size())
		return;

	binding[action].erase(binding[action].begin() + index);
}

void SDLInputState::initJoystick() {
	// close our gamepad handle if it's open
	if (gamepad) {
		SDL_GameControllerClose(gamepad);
		gamepad = NULL;
	}

	// get the joystick ids for all valid game controllers
	gamepad_ids.clear();
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			gamepad_ids.push_back(i);
		}
	}

	bool enable_log_msg = !joystick_init || joysticks_changed;

	if (gamepad_ids.empty()) {
		if (enable_log_msg) {
			Utils::logInfo("InputState: No gamepads were found.");
		}
		settings->enable_joystick = false;
	}
	else {
		if (enable_log_msg) {
			Utils::logInfo("InputState: %d gamepad(s) found.", gamepad_ids.size());
		}
	}

	if (static_cast<size_t>(settings->joystick_device) >= gamepad_ids.size())
		settings->joystick_device = 0;

	for (size_t i = 0; i < gamepad_ids.size(); ++i) {
		if (settings->enable_joystick && i == static_cast<size_t>(settings->joystick_device)) {
			gamepad = SDL_GameControllerOpen(gamepad_ids[i]);
		}
		if (enable_log_msg) {
			if (settings->enable_joystick && i == static_cast<size_t>(settings->joystick_device)) {
				Utils::logInfo("InputState: Gamepad #%d, %s [*]", i, SDL_GameControllerNameForIndex(static_cast<int>(i)));
			}
			else {
				Utils::logInfo("InputState: Gamepad #%d, %s", i, SDL_GameControllerNameForIndex(static_cast<int>(i)));
			}
		}
	}
}

void SDLInputState::initBindings() {
	// clear all bindings first
	for (int key=0; key<KEY_COUNT; key++) {
		binding[key].clear();
	}

	if (platform.needs_alt_escape_key) {
		// web browsers reserve Escape for exiting fullscreen, so we provide an alternate binding
		// backslash is used due to its general proximity to Enter on both ANSI and ISO layouts
		setBind(Input::CANCEL, InputBind::KEY, SDL_SCANCODE_BACKSLASH, NULL);
	}
	else {
		setBind(Input::CANCEL, InputBind::KEY, SDL_SCANCODE_ESCAPE, NULL);
	}

	setBind(Input::ACCEPT, InputBind::KEY, SDL_SCANCODE_RETURN, NULL);
	setBind(Input::ACCEPT, InputBind::KEY, SDL_SCANCODE_SPACE, NULL);

	if (platform.is_mobile_device) {
		setBind(Input::CANCEL, InputBind::KEY, SDL_SCANCODE_AC_BACK, NULL);
		setBind(Input::ACCEPT, InputBind::KEY, SDL_SCANCODE_MENU, NULL);
	}

	setBind(Input::UP, InputBind::KEY, SDL_SCANCODE_W, NULL);
	setBind(Input::UP, InputBind::KEY, SDL_SCANCODE_UP, NULL);

	setBind(Input::DOWN, InputBind::KEY, SDL_SCANCODE_S, NULL);
	setBind(Input::DOWN, InputBind::KEY, SDL_SCANCODE_DOWN, NULL);

	setBind(Input::LEFT, InputBind::KEY, SDL_SCANCODE_A, NULL);
	setBind(Input::LEFT, InputBind::KEY, SDL_SCANCODE_LEFT, NULL);

	setBind(Input::RIGHT, InputBind::KEY, SDL_SCANCODE_D, NULL);
	setBind(Input::RIGHT, InputBind::KEY, SDL_SCANCODE_RIGHT, NULL);

	setBind(Input::BAR_1, InputBind::KEY, SDL_SCANCODE_Q, NULL);
	setBind(Input::BAR_2, InputBind::KEY, SDL_SCANCODE_E, NULL);
	setBind(Input::BAR_3, InputBind::KEY, SDL_SCANCODE_R, NULL);
	setBind(Input::BAR_4, InputBind::KEY, SDL_SCANCODE_F, NULL);
	setBind(Input::BAR_5, InputBind::KEY, SDL_SCANCODE_1, NULL);
	setBind(Input::BAR_6, InputBind::KEY, SDL_SCANCODE_2, NULL);
	setBind(Input::BAR_7, InputBind::KEY, SDL_SCANCODE_3, NULL);
	setBind(Input::BAR_8, InputBind::KEY, SDL_SCANCODE_4, NULL);
	setBind(Input::BAR_9, InputBind::KEY, SDL_SCANCODE_5, NULL);
	setBind(Input::BAR_0, InputBind::KEY, SDL_SCANCODE_6, NULL);

	setBind(Input::CHARACTER, InputBind::KEY, SDL_SCANCODE_C, NULL);
	setBind(Input::INVENTORY, InputBind::KEY, SDL_SCANCODE_I, NULL);
	setBind(Input::POWERS, InputBind::KEY, SDL_SCANCODE_P, NULL);
	setBind(Input::LOG, InputBind::KEY, SDL_SCANCODE_L, NULL);

	setBind(Input::MAIN1, InputBind::MOUSE, SDL_BUTTON_LEFT, NULL);
	setBind(Input::MAIN2, InputBind::MOUSE, SDL_BUTTON_RIGHT, NULL);

	setBind(Input::MENU_PAGE_NEXT, InputBind::KEY, SDL_SCANCODE_PAGEDOWN, NULL);
	setBind(Input::MENU_PAGE_PREV, InputBind::KEY, SDL_SCANCODE_PAGEUP, NULL);
	setBind(Input::MENU_ACTIVATE, InputBind::KEY, SDL_SCANCODE_V, NULL);

	setBind(Input::DEVELOPER_MENU, InputBind::KEY, SDL_SCANCODE_F5, NULL);

	setBind(Input::EQUIPMENT_SWAP, InputBind::KEY, SDL_SCANCODE_TAB, NULL);
	setBind(Input::MINIMAP_MODE, InputBind::KEY, SDL_SCANCODE_M, NULL);
	setBind(Input::LOOT_TOOLTIP_MODE, InputBind::KEY, SDL_SCANCODE_SLASH, NULL);
	setBind(Input::ACTIONBAR, InputBind::KEY, SDL_SCANCODE_B, NULL);

	// Gamepad bindings
	setBind(Input::CANCEL, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_B, NULL);
	setBind(Input::ACCEPT, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_A, NULL);

	setBind(Input::RIGHT, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_LEFTX*2), NULL);
	setBind(Input::DOWN, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_LEFTY*2), NULL);
	setBind(Input::LEFT, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_LEFTX*2) + 1, NULL);
	setBind(Input::UP, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_LEFTY*2) + 1, NULL);

	setBind(Input::LEFT, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_DPAD_LEFT, NULL);
	setBind(Input::RIGHT, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, NULL);
	setBind(Input::UP, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_DPAD_UP, NULL);
	setBind(Input::DOWN, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_DPAD_DOWN, NULL);

	setBind(Input::BAR_1, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_LEFTSTICK, NULL);
	setBind(Input::BAR_2, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_RIGHTSTICK, NULL);

	setBind(Input::INVENTORY, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_Y, NULL);

	// TODO implement input contexts so that MAIN1 doesn't interfere with MENU_PAGE_NEXT
	// setBind(Input::MAIN1, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, NULL);
	// setBind(Input::MAIN2, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_TRIGGERRIGHT*2), NULL);

	setBind(Input::MAIN1, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_TRIGGERRIGHT*2), NULL);
	setBind(Input::MAIN2, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_TRIGGERLEFT*2), NULL);

	setBind(Input::ACTIONBAR, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_BACK, NULL);

	setBind(Input::MENU_PAGE_NEXT, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, NULL);
	setBind(Input::MENU_PAGE_PREV, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, NULL);
	setBind(Input::MENU_ACTIVATE, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_X, NULL);

	setBind(Input::PAUSE, InputBind::GAMEPAD, SDL_CONTROLLER_BUTTON_START, NULL);

	setBind(Input::AIM_RIGHT, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_RIGHTX*2), NULL);
	setBind(Input::AIM_DOWN, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_RIGHTY*2), NULL);
	setBind(Input::AIM_LEFT, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_RIGHTX*2) + 1, NULL);
	setBind(Input::AIM_UP, InputBind::GAMEPAD_AXIS, (SDL_CONTROLLER_AXIS_RIGHTY*2) + 1, NULL);

	// Not user-modifiable
	setBind(Input::CTRL, InputBind::KEY, SDL_SCANCODE_LCTRL, NULL);
	setBind(Input::CTRL, InputBind::KEY, SDL_SCANCODE_RCTRL, NULL);
	setBind(Input::SHIFT, InputBind::KEY, SDL_SCANCODE_LSHIFT, NULL);
	setBind(Input::SHIFT, InputBind::KEY, SDL_SCANCODE_RSHIFT, NULL);
	setBind(Input::ALT, InputBind::KEY, SDL_SCANCODE_LALT, NULL);
	setBind(Input::ALT, InputBind::KEY, SDL_SCANCODE_RALT, NULL);
	setBind(Input::DEL, InputBind::KEY, SDL_SCANCODE_DELETE, NULL);
	setBind(Input::DEL, InputBind::KEY, SDL_SCANCODE_BACKSPACE, NULL);
	setBind(Input::TEXTEDIT_UP, InputBind::KEY, SDL_SCANCODE_UP, NULL);
	setBind(Input::TEXTEDIT_DOWN, InputBind::KEY, SDL_SCANCODE_DOWN, NULL);
}

void SDLInputState::handle() {
	InputState::handle();

	SDL_Event event;

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
				mode = MODE_KEYBOARD_AND_MOUSE;
				mouse = scaleMouse(event.motion.x, event.motion.y);
				curs->show_cursor = true;
				break;
			case SDL_MOUSEWHEEL:
				mode = MODE_KEYBOARD_AND_MOUSE;
				if (event.wheel.y > 0) {
					scroll_up = true;
				} else if (event.wheel.y < 0) {
					scroll_down = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				mode = MODE_KEYBOARD_AND_MOUSE;
				mouse = scaleMouse(event.button.x, event.button.y);
				for (int key=0; key<KEY_COUNT; key++) {
					for (size_t i = 0; i < binding[key].size(); ++i) {
						if (binding[key][i].type == InputBind::MOUSE && binding[key][i].bind == event.button.button) {
							pressing[key] = true;
							un_press[key] = false;
						}
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				mode = MODE_KEYBOARD_AND_MOUSE;
				mouse = scaleMouse(event.button.x, event.button.y);
				for (int key=0; key<KEY_COUNT; key++) {
					for (size_t i = 0; i < binding[key].size(); ++i) {
						if (binding[key][i].type == InputBind::MOUSE && binding[key][i].bind == event.button.button) {
							un_press[key] = true;
						}
					}
				}
				last_button = event.button.button;
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
				if (settings->touchscreen) {
					mode = MODE_TOUCHSCREEN;
					curs->show_cursor = false;
					mouse.x = static_cast<int>((event.tfinger.x + event.tfinger.dx) * settings->view_w);
					mouse.y = static_cast<int>((event.tfinger.y + event.tfinger.dy) * settings->view_h);
					pressing[Input::MAIN1] = true;
					un_press[Input::MAIN1] = false;

					if (event.tfinger.dy > 0.005) {
						scroll_up = true;
					} else if (event.tfinger.dy < -0.005) {
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
				if (settings->touchscreen) {
					mode = MODE_TOUCHSCREEN;
					curs->show_cursor = false;
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
				if (settings->touchscreen) {
					mode = MODE_TOUCHSCREEN;
					// MAIN1 might have been set to un-press from a SDL_MOUSEBUTTONUP event
					un_press[Input::MAIN1] = false;

					curs->show_cursor = false;
					for (size_t i = 0; i < touch_fingers.size(); ++i) {
						if (touch_fingers[i].id == event.tfinger.fingerId) {
							touch_fingers.erase(touch_fingers.begin() + i);
							break;
						}
					}
					if (touch_fingers.empty()) {
						touch_locked = false;
						un_press[Input::MAIN1] = true;
						// TODO need a permanant MAIN1 binding
						for (size_t i = 0; i < binding[Input::MAIN1].size(); ++i) {
							if (binding[Input::MAIN1][i].type == InputBind::MOUSE) {
								last_button = binding[Input::MAIN1][i].bind;
								break;
							}
						}
					}
					else {
						mouse.x = touch_fingers.back().pos.x;
						mouse.y = touch_fingers.back().pos.y;
					}
				}
				break;
			case SDL_KEYDOWN:
				mode = MODE_KEYBOARD_AND_MOUSE;

				for (int key=0; key<KEY_COUNT; key++) {
					for (size_t i = 0; i < binding[key].size(); ++i) {
						if (binding[key][i].type == InputBind::KEY && binding[key][i].bind == event.key.keysym.scancode) {
							pressing[key] = true;
							un_press[key] = false;
						}
					}
				}
				break;
			case SDL_KEYUP:
				mode = MODE_KEYBOARD_AND_MOUSE;

				for (int key=0; key<KEY_COUNT; key++) {
					for (size_t i = 0; i < binding[key].size(); ++i) {
						if (binding[key][i].type == InputBind::KEY && binding[key][i].bind == event.key.keysym.scancode) {
							pressing[key] = true;
							un_press[key] = true;
						}
					}
				}

				last_key = event.key.keysym.scancode;
				break;
			case SDL_CONTROLLERBUTTONDOWN:
				if (settings->enable_joystick && gamepad) {
					mode = MODE_JOYSTICK;

					SDL_JoystickID joy_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));
					if (joy_id == event.jbutton.which) {
						for (int key=0; key<KEY_COUNT; key++) {
							for (size_t i = 0; i < binding[key].size(); ++i) {
								if (binding[key][i].type == InputBind::GAMEPAD && binding[key][i].bind == event.cbutton.button) {
									curs->show_cursor = false;
									hideCursor();
									pressing[key] = true;
									un_press[key] = false;
								}
							}
						}
					}
				}
				break;
			case SDL_CONTROLLERBUTTONUP:
				if (settings->enable_joystick && gamepad) {
					mode = MODE_JOYSTICK;

					SDL_JoystickID joy_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));
					if (joy_id == event.jbutton.which) {
						for (int key=0; key<KEY_COUNT; key++) {
							for (size_t i = 0; i < binding[key].size(); ++i) {
								if (binding[key][i].type == InputBind::GAMEPAD && binding[key][i].bind == event.cbutton.button) {
									un_press[key] = true;
								}
							}
						}
						last_joybutton = event.cbutton.button;
					}
				}
				break;
			case SDL_CONTROLLERAXISMOTION:
				if (settings->enable_joystick && gamepad) {
					last_joyaxis = -1;

					SDL_JoystickID joy_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));
					if (joy_id == event.jbutton.which) {
						for (int key=0; key<KEY_COUNT; key++) {
							for (size_t i = 0; i < binding[key].size(); ++i) {
								int bind_axis = binding[key][i].bind / 2;
								if (binding[key][i].type == InputBind::GAMEPAD_AXIS && bind_axis == event.caxis.axis) {
									bool is_down;
									if (bind_axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || bind_axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
										is_down = (event.caxis.value >= settings->joy_deadzone);
									}
									else if (binding[key][i].bind % 2 == 0) {
										is_down = (event.caxis.value >= settings->joy_deadzone);
									}
									else {
										is_down = (event.caxis.value <= -(settings->joy_deadzone));
									}

									if (is_down) {
										mode = MODE_JOYSTICK;
										curs->show_cursor = false;
										hideCursor();
										pressing[key] = true;
										un_press[key] = false;
									}
									else if (pressing[key] && mode == MODE_JOYSTICK) {
										un_press[key] = true;
										pressing[key] = false;
										lock[key] = false;
									}
								}
							}
						}
						if (event.caxis.value >= settings->joy_deadzone)
							last_joyaxis = event.caxis.axis * 2;
						else if (event.caxis.value <= -(settings->joy_deadzone))
							last_joyaxis = (event.caxis.axis * 2) + 1;
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
				initJoystick();
				break;
			case SDL_QUIT:
				done = 1;
				// Clear inputs when quit is triggered.
				// Doing this prevents unintended actions being triggered when exiting via an OS keyboard shortcut.
				// For example, using Win+Esc as a global "close window" command while having CANCEL bound to Esc would cause a crash if it was used on the New Game screen.
				for (int key=0; key<KEY_COUNT; key++) {
					pressing[key] = false;
					un_press[key] = false;
					lock[key] = false;
				}
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
	return std::string(SDL_GameControllerNameForIndex(index));
}

std::string SDLInputState::getKeyName(int key, bool get_short_string) {
	key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(key));

	// first, we try to provide a translation of the key
	if (get_short_string) {
		switch (static_cast<SDL_Keycode>(key)) {
			case SDLK_BACKSPACE:    return msg->get("BkSp");
			case SDLK_CAPSLOCK:     return msg->get("Caps");
			case SDLK_DELETE:       return msg->get("Del");
			case SDLK_DOWN:         return msg->get("Down");
			case SDLK_END:          return msg->get("End");
			case SDLK_ESCAPE:       return msg->get("Esc");
			case SDLK_HOME:         return msg->get("Home");
			case SDLK_INSERT:       return msg->get("Ins");
			case SDLK_LALT:         return msg->get("LAlt");
			case SDLK_LCTRL:        return msg->get("LCtrl");
			case SDLK_LEFT:         return msg->get("Left");
			case SDLK_LSHIFT:       return msg->get("LShft");
			case SDLK_NUMLOCKCLEAR: return msg->get("Num");
			case SDLK_PAGEDOWN:     return msg->get("PgDn");
			case SDLK_PAGEUP:       return msg->get("PgUp");
			case SDLK_PAUSE:        return msg->get("Pause");
			case SDLK_PRINTSCREEN:  return msg->get("Print");
			case SDLK_RALT:         return msg->get("RAlt");
			case SDLK_RCTRL:        return msg->get("RCtrl");
			case SDLK_RETURN:       return msg->get("Ret");
			case SDLK_RIGHT:        return msg->get("Right");
			case SDLK_RSHIFT:       return msg->get("RShft");
			case SDLK_SCROLLLOCK:   return msg->get("SLock");
			case SDLK_SPACE:        return msg->get("Spc");
			case SDLK_TAB:          return msg->get("Tab");
			case SDLK_UP:           return msg->get("Up");
		}
	}
	else {
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
	}

	// no translation for this key, so just get the name straight from SDL
	return std::string(SDL_GetKeyName(static_cast<SDL_Keycode>(key)));
}

std::string SDLInputState::getMouseButtonName(int button, bool get_short_string) {
	if (get_short_string) {
		return msg->get("M%d", button);
	}
	else {
		if (button > 0 && button <= MOUSE_BUTTON_NAME_COUNT)
			return mouse_button[button - 1];
		else
			return msg->get("Mouse %d", button);
	}
}

std::string SDLInputState::getJoystickButtonName(int button) {
	if (button < SDL_CONTROLLER_BUTTON_MAX) {
		return xbox_buttons[button];
	}
	else {
		return msg->get("(unknown)");
	}
}

std::string SDLInputState::getJoystickAxisName(int axis) {
	if (axis < SDL_CONTROLLER_AXIS_MAX*2) {
		return xbox_axes[axis];
	}
	else {
		return msg->get("(unknown)");
	}
}

std::string SDLInputState::getBindingString(int key, bool get_short_string) {
	return getBindingStringByIndex(key, -1, get_short_string);
}

std::string SDLInputState::getBindingStringByIndex(int key, int binding_index, bool get_short_string) {
	std::string none = "";
	if (!get_short_string)
		none = msg->get("(none)");

	if (binding[key].empty()) {
		return none;
	}

	int bi = 0;
	if (binding_index != -1 && static_cast<size_t>(binding_index) < binding[key].size())
		bi = binding_index;

	if (binding[key][bi].type == InputBind::KEY)
		return getKeyName(binding[key][bi].bind, get_short_string);
	else if (binding[key][bi].type == InputBind::MOUSE)
		return getMouseButtonName(binding[key][bi].bind, get_short_string);
	else if (binding[key][bi].type == InputBind::GAMEPAD)
		return getJoystickButtonName(binding[key][bi].bind);
	else if (binding[key][bi].type == InputBind::GAMEPAD_AXIS)
		return getJoystickAxisName(binding[key][bi].bind);
	else
		return none;
}

std::string SDLInputState::getGamepadBindingString(int key, bool get_short_string) {
	std::string none = "";
	if (!get_short_string)
		none = msg->get("(none)");

	if (binding[key].empty()) {
		return none;
	}

	for (size_t i = 0; i < binding[key].size(); ++i) {
		if (binding[key][i].type == InputBind::GAMEPAD)
			return getJoystickButtonName(binding[key][i].bind);
		else if (binding[key][i].type == InputBind::GAMEPAD_AXIS)
			return getJoystickAxisName(binding[key][i].bind);
	}

	return none;
}

std::string SDLInputState::getMovementString() {
	std::string output = "[";

	if (settings->enable_joystick) {
		output += getGamepadBindingString(Input::LEFT) +  "/";
		output += getGamepadBindingString(Input::RIGHT) + "/";
		output += getGamepadBindingString(Input::UP) + "/";
		output += getGamepadBindingString(Input::DOWN);
	}
	else if (settings->touchscreen) {
		output += msg->get("Touch control D-Pad");
	}
	else if (settings->mouse_move) {
		output += (settings->mouse_move_swap ? getBindingString(Input::MAIN2) : getBindingString(Input::MAIN1));
	}
	else {
		output += getBindingString(Input::LEFT) + "/";
		output += getBindingString(Input::RIGHT) + "/";
		output += getBindingString(Input::UP) + "/";
		output += getBindingString(Input::DOWN);
	}

	output += "]";
	return output;
}

std::string SDLInputState::getAttackString() {
	std::string output = "[";

	if (settings->touchscreen) {
		output += msg->get("Touch control buttons");
	}
	else {
		output += getBindingString(Input::MAIN1);
	}

	output += "]";
	return output;
}

int SDLInputState::getNumJoysticks() {
	return static_cast<int>(gamepad_ids.size());
}

bool SDLInputState::usingMouse() {
	return !settings->no_mouse && mode != MODE_JOYSTICK;
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

int SDLInputState::getBindFromString(const std::string& bind, int type) {
	// -1 is used to clear all bindings
	if (bind == "-1")
		return -1;

	// mouse buttons are always just plain ints
	if (type == InputBind::MOUSE)
		return Parse::toInt(bind);

	// handle human-readable binds
	std::string temp = bind;
	if (Parse::popFirstString(temp, ':') == "SDL") {
		if (type == InputBind::KEY) {
			return SDL_GetScancodeFromName(temp.c_str());
		}
		else if (type == InputBind::GAMEPAD) {
			return SDL_GameControllerGetButtonFromString(temp.c_str());
		}
		else if (type == InputBind::GAMEPAD_AXIS) {
			std::string axis_name = Parse::popFirstString(temp, ':');
			int axis = SDL_GameControllerGetAxisFromString(axis_name.c_str()) * 2;

			// "+" requires no change
			if (temp == "-")
				axis += 1;

			return axis;
		}
	}

	// bind is probably just an int (most likely written by engine)
	return Parse::toInt(bind);
}

void SDLInputState::setCommonStrings() {
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
	binding_name[Input::EQUIPMENT_SWAP] = msg->get("Equipment Swap");
	binding_name[Input::MINIMAP_MODE] = msg->get("Mini-map Mode");
	binding_name[Input::LOOT_TOOLTIP_MODE] = msg->get("Loot Tooltip Mode");
	binding_name[Input::ACTIONBAR] = msg->get("Action Bar");
	binding_name[Input::MENU_PAGE_NEXT] = msg->get("Menu: Next Page");
	binding_name[Input::MENU_PAGE_PREV] = msg->get("Menu: Previous Page");
	binding_name[Input::MENU_ACTIVATE] = msg->get("Menu: Activate");
	binding_name[Input::PAUSE] = msg->get("Pause");
	binding_name[Input::AIM_UP] = msg->get("Aim Up");
	binding_name[Input::AIM_DOWN] = msg->get("Aim Down");
	binding_name[Input::AIM_LEFT] = msg->get("Aim Left");
	binding_name[Input::AIM_RIGHT] = msg->get("Aim Right");
	binding_name[Input::DEVELOPER_MENU] = msg->get("Developer Menu");
	binding_name[Input::CTRL] = msg->get("Ctrl");
	binding_name[Input::SHIFT] = msg->get("Shift");
	binding_name[Input::ALT] = msg->get("Alt");
	binding_name[Input::DEL] = msg->get("Delete");

	mouse_button[0] = msg->get("Left Mouse");
	mouse_button[1] = msg->get("Middle Mouse");
	mouse_button[2] = msg->get("Right Mouse");
	mouse_button[3] = msg->get("Wheel Up");
	mouse_button[4] = msg->get("Wheel Down");
	mouse_button[5] = msg->get("Mouse X1");
	mouse_button[6] = msg->get("Mouse X2");

	xbox_buttons[SDL_CONTROLLER_BUTTON_A] = msg->get("X360: A");
	xbox_buttons[SDL_CONTROLLER_BUTTON_B] = msg->get("X360: B");
	xbox_buttons[SDL_CONTROLLER_BUTTON_X] = msg->get("X360: X");
	xbox_buttons[SDL_CONTROLLER_BUTTON_Y] = msg->get("X360: Y");
	xbox_buttons[SDL_CONTROLLER_BUTTON_BACK] = msg->get("X360: Back");
	xbox_buttons[SDL_CONTROLLER_BUTTON_GUIDE] = msg->get("X360: Guide");
	xbox_buttons[SDL_CONTROLLER_BUTTON_START] = msg->get("X360: Start");
	xbox_buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK] = msg->get("X360: L3");
	xbox_buttons[SDL_CONTROLLER_BUTTON_RIGHTSTICK] = msg->get("X360: R3");
	xbox_buttons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = msg->get("X360: L1");
	xbox_buttons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = msg->get("X360: R1");
	xbox_buttons[SDL_CONTROLLER_BUTTON_DPAD_UP] = msg->get("X360: D-Up");
	xbox_buttons[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = msg->get("X360: D-Down");
	xbox_buttons[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = msg->get("X360: D-Left");
	xbox_buttons[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = msg->get("X360: D-Right");

	xbox_axes[(SDL_CONTROLLER_AXIS_LEFTX*2)] = msg->get("X360: Left X+");
	xbox_axes[(SDL_CONTROLLER_AXIS_LEFTX*2)+1] = msg->get("X360: Left X-");
	xbox_axes[(SDL_CONTROLLER_AXIS_LEFTY*2)] = msg->get("X360: Left Y+");
	xbox_axes[(SDL_CONTROLLER_AXIS_LEFTY*2)+1] = msg->get("X360: Left Y-");
	xbox_axes[(SDL_CONTROLLER_AXIS_RIGHTX*2)] = msg->get("X360: Right X+");
	xbox_axes[(SDL_CONTROLLER_AXIS_RIGHTX*2)+1] = msg->get("X360: Right X-");
	xbox_axes[(SDL_CONTROLLER_AXIS_RIGHTY*2)] = msg->get("X360: Right Y+");
	xbox_axes[(SDL_CONTROLLER_AXIS_RIGHTY*2)+1] = msg->get("X360: Right Y-");
	xbox_axes[(SDL_CONTROLLER_AXIS_TRIGGERLEFT*2)] = msg->get("X360: L2");
	xbox_axes[(SDL_CONTROLLER_AXIS_TRIGGERRIGHT*2)] = msg->get("X360: R2");
}

std::string SDLInputState::getInputBindName(int type, int bind) {
	if (type == InputBind::KEY) {
		return getKeyName(bind);
	}
	else if (type == InputBind::MOUSE) {
		return getMouseButtonName(bind);
	}
	else if (type == InputBind::GAMEPAD) {
		return getJoystickButtonName(bind);
	}
	else if (type == InputBind::GAMEPAD_AXIS) {
		return getJoystickAxisName(bind);
	}
	return "";
}

SDLInputState::~SDLInputState() {
	if (gamepad)
		SDL_GameControllerClose(gamepad);
}
