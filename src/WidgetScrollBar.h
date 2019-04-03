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

class WidgetScrollBar : public Widget {
private:

	std::string fileName; // the path to the ScrollBar's atlas

	Sprite *scrollbars;

	int value;
	int bar_height;
	int maximum;

public:
	static const std::string DEFAULT_FILE;

	explicit WidgetScrollBar(const std::string& _fileName);
	~WidgetScrollBar();

	void loadArt();
	int checkClick();
	int checkClickAt(int x, int y);
	void set();
	int getValue();
	Rect getBounds();
	void render();
	void refresh(int x, int y, int h, int val, int max);

	Rect pos_up;
	Rect pos_down;
	Rect pos_knob;
	bool pressed_up;
	bool pressed_down;
	bool pressed_knob;
};

#endif
