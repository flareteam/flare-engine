/*
Copyright © 2011-2012 Clint Bellanger

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
	void handle();
	void hideCursor();
	void showCursor();
	std::string getJoystickName(int index);
	std::string getKeyName(int key);
	std::string getBindingString(int key, int bindings_list = INPUT_BINDING_DEFAULT);
	std::string getMovementString();
	std::string getAttackString();
	std::string getContinueString();
	int getNumJoysticks();

private:
	SDL_Joystick* joy;
	int joy_num;
	int joy_axis_num;
	int joy_axis_ticks;
	int temp_joyaxis;

	std::vector<int> joy_axis_prev;
	std::vector<int> joy_axis_deltas;
};

#endif
