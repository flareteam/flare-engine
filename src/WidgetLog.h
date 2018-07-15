/*
Copyright © 2014-2016 Justin Jacobs

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
	void setFont(int style);

	WidgetScrollBox *scroll_box;
	int line_height;
	int paragraph_spacing;
	int padding;
	unsigned max_messages;

	std::vector<std::string> messages;
	std::vector<Color> colors;
	std::vector<int> styles;
	std::vector<bool> separators;

	bool updated;

	Color next_color;
	int next_style;

public:
	enum {
		FONT_REGULAR = 0,
		FONT_BOLD = 1
	};

	enum {
		MSG_NORMAL = 0,
		MSG_UNIQUE = 1
	};

	static const unsigned MAX_MESSAGES = 50;

	WidgetLog (int width, int height);
	~WidgetLog ();
	void setBasePos(int x, int y, int a);
	void setPos(int offset_x, int offset_y);

	void logic();
	void render();

	Widget* getWidget() {
		return reinterpret_cast<Widget*>(scroll_box);    // for adding to tablist
	}

	void add(const std::string &s, int type);
	void setNextColor(const Color& color);
	void setNextStyle(int style);
	void remove(unsigned msg_index);
	void clear();
	void setMaxMessages(unsigned count);
	void addSeparator();
	bool isEmpty();
};

#endif

