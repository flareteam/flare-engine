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

	void initJoystick();
	void defaultQwertyKeyBindings();
	void setFixedKeyBindings();
	void handle();
	void hideCursor();
	void showCursor();
	std::string getJoystickName(int index);
	std::string getKeyName(int key);
	std::string getMouseButtonName(int button);
	std::string getJoystickButtonName(int button);
	std::string getBindingString(int key, int bindings_list = InputState::BINDING_DEFAULT);
	std::string getMovementString();
	std::string getAttackString();
	std::string getContinueString();
	int getNumJoysticks();
	bool usingMouse();
	void startTextInput();
	void stopTextInput();
	void setKeybind(int key, int binding_button, int bindings_list, std::string& keybind_msg);

private:
	int getKeyFromName(const std::string& key_name);
	void validateFixedKeyBinding(int action, int key, int bindings_list);

	SDL_Joystick* joy;
	int joy_num;
	int joy_axis_num;
	Timer resize_cooldown;
	bool joystick_init;
	bool text_input;

	std::vector<int> joy_axis_prev;
	std::vector<int> joy_axis_deltas;
};

#endif
