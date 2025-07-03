/*
Copyright © 2011-2012 Clint Bellanger
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

#ifndef SDL_INPUT_STATE_H
#define SDL_INPUT_STATE_H

#include "InputState.h"
#include "Utils.h"

/**
 * class SDLInputState
 *
 * Handles keyboard and mouse states using SDL API
 */
class SDLInputState : public InputState {
public:
	SDLInputState(void);
	~SDLInputState();

	void setBind(int action, int type, int bind, std::string *keybind_msg);
	void removeBind(int action, size_t index);

	void initJoystick();
	void initBindings();
	void handle();
	void hideCursor();
	void showCursor();
	std::string getJoystickName(int index);
	std::string getKeyName(int key, bool get_short_string = !InputState::GET_SHORT_STRING);
	std::string getMouseButtonName(int button, bool get_short_string = !InputState::GET_SHORT_STRING);
	std::string getJoystickButtonName(int button);
	std::string getJoystickAxisName(int axis);
	std::string getBindingString(int key, bool get_short_string = !InputState::GET_SHORT_STRING);
	std::string getBindingStringByIndex(int key, int binding_index, bool get_short_string = !GET_SHORT_STRING);
	std::string getGamepadBindingString(int key, bool get_short_string = !InputState::GET_SHORT_STRING);
	std::string getMovementString();
	std::string getAttackString();
	int getNumJoysticks();
	bool usingMouse();
	bool usingTouchscreen();
	void startTextInput();
	void stopTextInput();
	void setCommonStrings();
	void joystickRumble(uint16_t low_freq, uint16_t high_freq, uint32_t duration);
	void setJoystickLED(Color color);

	void reset();

private:
	int getBindFromString(const std::string& bind, int type);
	std::string getInputBindName(int type, int bind);

	Timer resize_cooldown;
	bool joystick_init;
	bool text_input;

	std::vector<int> gamepad_ids;
	SDL_GameController* gamepad;

	std::string xbox_buttons[SDL_CONTROLLER_BUTTON_MAX];
	std::string xbox_axes[SDL_CONTROLLER_AXIS_MAX*2]; // doubled because we need both positive and negative axis names

	std::vector<InputBind> restricted_bindings;
};

#endif
