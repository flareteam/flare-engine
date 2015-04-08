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

#include <math.h>

SDLInputState::SDLInputState(void)
	: InputState()
{
#ifndef __ANDROID__
	SDL_StartTextInput();
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

void SDLInputState::defaultQwertyKeyBindings () {
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

void SDLInputState::handle(bool dump_event) {
	InputState::handle(dump_event);

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

#ifndef __ANDROID__
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
				for (int key=0; key<key_count; key++) {
					if (event.button.button == binding[key] || event.button.button == binding_alt[key]) {
						pressing[key] = true;
						un_press[key] = false;
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				mouse.x = event.button.x;
				mouse.y = event.button.y;
				for (int key=0; key<key_count; key++) {
					if (event.button.button == binding[key] || event.button.button == binding_alt[key]) {
						un_press[key] = true;
					}
				}
				last_button = event.button.button;
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					window_resized = true;
					render_device->windowResize();
				}
				break;
#else
			// detect restoring hidden Android app to bypass frameskip
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					window_resized = true;
					render_device->windowResize();
				}
				else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
					window_minimized = true;
				else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
					window_restored = true;
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

SDLInputState::~SDLInputState() {
}
