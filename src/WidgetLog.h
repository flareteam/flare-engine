/*
Copyright © 2014 Justin Jacobs

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
 * class WidgetLog
 */

#ifndef WIDGET_LOG_H
#define WIDGET_LOG_H

#include "CommonIncludes.h"

class Widget;
class WidgetScrollBox;

class WidgetLog : public Widget {
private:
	void refresh();

	WidgetScrollBox *scroll_box;
	int line_height;
	int paragraph_spacing;
	int padding;
	unsigned max_messages;
	Color color_normal;

	std::vector<std::string> messages;
	std::vector<Color> colors;

public:
	WidgetLog (int width, int height);
	~WidgetLog ();
	void setBasePos(int x, int y, ALIGNMENT a = ALIGN_TOPLEFT);
	void setPos(int offset_x = 0, int offset_y = 0);

	void logic();
	void render();

	void setPosition(int x, int y);
	bool inFocus();
	Widget* getWidget() {
		return (Widget*)scroll_box;    // for adding to tablist
	}

	void add(const std::string &s, bool prevent_spam = true, Color* color = NULL);
	void remove(unsigned msg_index);
	void clear();
};

#endif

