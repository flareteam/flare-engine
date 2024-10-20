/*
Copyright © 2011-2012 Clint Bellanger
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

#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include "CommonIncludes.h"
#include "Utils.h"

class Version;

namespace Input {
	// Input action enum
	enum {
		CANCEL = 0,
		ACCEPT,
		UP,
		DOWN,
		LEFT,
		RIGHT,
		BAR_1,
		BAR_2,
		BAR_3,
		BAR_4,
		BAR_5,
		BAR_6,
		BAR_7,
		BAR_8,
		BAR_9,
		BAR_0,
		CHARACTER,
		INVENTORY,
		POWERS,
		LOG,
		MAIN1,
		MAIN2,
		EQUIPMENT_SWAP,
		EQUIPMENT_SWAP_PREV,
		MINIMAP_MODE,
		LOOT_TOOLTIP_MODE,
		ACTIONBAR,
		MENU_PAGE_NEXT,
		MENU_PAGE_PREV,
		MENU_ACTIVATE,
		PAUSE,
		AIM_UP,
		AIM_DOWN,
		AIM_LEFT,
		AIM_RIGHT,
		DEVELOPER_MENU,
		DEVELOPER_CMD_1,
		DEVELOPER_CMD_2,
		DEVELOPER_CMD_3,

		// non-modifiable
		CTRL,
		SHIFT,
		ALT,
		DEL,
		TEXTEDIT_UP,
		TEXTEDIT_DOWN,

		KEY_COUNT,
	};
}

class InputBind {
public:
	enum {
		KEY = 0,
		MOUSE = 1,
		GAMEPAD = 2,
		GAMEPAD_AXIS = 3,
	};

	int type;
	int bind;

	InputBind(int _type = -1, int _bind = -1)
		: type(_type)
		, bind(_bind)
	{}
	~InputBind() {}
};

/**
 * class InputState
 *
 * Handles keyboard and mouse states
 */

class InputState {
protected:
	// some mouse buttons are named (e.g. "Left Mouse")
	static const int MOUSE_BUTTON_NAME_COUNT = 7;

public:
	static const bool GET_SHORT_STRING = true;
	static const int KEY_COUNT = Input::KEY_COUNT;
	static const int KEY_COUNT_USER = KEY_COUNT - 6; // exclude CTRL, SHIFT, etc from keybinding menu

	enum {
		MODE_KEYBOARD_AND_MOUSE = 0,
		MODE_JOYSTICK = 1,
		MODE_TOUCHSCREEN = 2,
	};

	std::vector<InputBind> binding[KEY_COUNT];

	std::string binding_name[KEY_COUNT];
	std::string mouse_button[MOUSE_BUTTON_NAME_COUNT];

	InputState(void);
	virtual ~InputState();

	virtual void setBind(int action, int type, int bind, std::string *keybind_msg) = 0;
	virtual void removeBind(int action, size_t index) = 0;

	virtual void initJoystick() = 0;
	void loadKeyBindings();
	void saveKeyBindings();
	void resetScroll();
	void lockActionBar();
	void unlockActionBar();
	virtual void setCommonStrings() = 0;

	virtual void handle();
	virtual void initBindings() = 0;
	virtual void hideCursor() = 0;
	virtual void showCursor() = 0;
	virtual std::string getJoystickName(int index) = 0;
	virtual std::string getBindingString(int key, bool get_short_string = !GET_SHORT_STRING) = 0;
	virtual std::string getBindingStringByIndex(int key, int binding_index, bool get_short_string = !GET_SHORT_STRING) = 0;
	virtual std::string getGamepadBindingString(int key, bool get_short_string = !GET_SHORT_STRING) = 0;
	virtual std::string getMovementString() = 0;
	virtual std::string getAttackString() = 0;
	virtual int getNumJoysticks() = 0;
	virtual bool usingMouse() = 0;
	virtual bool usingTouchscreen() = 0;
	virtual void startTextInput() = 0;
	virtual void stopTextInput() = 0;

	void enableEventLog();

	bool pressing[KEY_COUNT];
	bool lock[KEY_COUNT];

	// handle repeating keys, such as when holding Backspace to delete text in WidgetInput
	bool slow_repeat[KEY_COUNT];
	Timer repeat_cooldown[KEY_COUNT];

	bool done;
	Point mouse;
	std::string inkeys;
	int last_key;
	int last_button;
	int last_joybutton;
	int last_joyaxis;
	unsigned mode;
	bool scroll_up;
	bool scroll_down;
	bool lock_scroll;
	bool touch_locked;
	bool lock_all;
	bool window_minimized;
	bool window_restored;
	bool window_resized;
	bool joysticks_changed;
	bool refresh_hotkeys;

protected:
	Point scaleMouse(unsigned int x, unsigned int y);
	virtual int getBindFromString(const std::string& bind, int type) = 0;

	bool un_press[KEY_COUNT];
	Point current_touch;
	bool dump_event;

	class FingerData {
	public:
		long int id;
		Point pos;
	};
	std::vector<FingerData> touch_fingers;

	Version* file_version;
	Version* file_version_min;

	std::string config_keys[KEY_COUNT_USER];
};

#endif
