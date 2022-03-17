/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2015 Justin Jacobs
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson

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
 * class WidgetScrollBar
 */

#ifndef WIDGET_ScrollBar_H
#define WIDGET_ScrollBar_H

#include "CommonIncludes.h"
#include "Widget.h"

class Sprite;

class WidgetScrollBar : public Widget {
private:

	enum {
		GFX_PREV = 0,
		GFX_PREV_PRESS = 1,
		GFX_NEXT = 2,
		GFX_NEXT_PRESS = 3,
		GFX_KNOB = 4,
		GFX_TOTAL,
	};

	void setKnobPos();

	std::string fileName; // the path to the ScrollBar's atlas

	Sprite *scrollbars;

	int value;
	int bar_height;
	int maximum;
	bool lock_main1;
	bool dragging;

	Sprite *bg;

	Rect up_to_knob;
	Rect knob_to_down;

	Rect pos_up;
	Rect pos_down;
	Rect pos_knob;
	bool pressed_up;
	bool pressed_down;
	bool pressed_knob;

public:
	enum {
		CLICK_NONE = 0,
		CLICK_UP = 1,
		CLICK_DOWN = 2,
		CLICK_KNOB = 3,
	};

	static const std::string DEFAULT_FILE;

	explicit WidgetScrollBar(const std::string& _fileName);
	~WidgetScrollBar();

	int checkClick();
	int checkClickAt(int x, int y);
	int getValue();
	Rect getBounds();
	void render();
	void refresh(int x, int y, int h, int val, int max);
};

#endif
