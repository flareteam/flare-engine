/*
Copyright © 2011-2012 kitano
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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
 * class WidgetInput
 *
 * A simple text box with a label above it.
 * It has two images - one for focused and one for unfocused.
 */

#ifndef WIDGETINPUT_H
#define WIDGETINPUT_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "Widget.h"
#include "WidgetTooltip.h"

class WidgetInput : public Widget {

protected:

	void loadGraphics(const std::string& filename);
	void trimText();
	bool checkClick(const Point& mouse);

	Sprite *background;

	bool enabled;
	bool pressed;

	std::string text; // the text that has been typed into the box
	std::string trimmed_text; // a trimmed version of text that is rendered
	std::string trimmed_text_cursor; // same as trimmed_text, but with a '|' inserted to represent the cursor
	size_t cursor_pos;

	Point font_pos;

	// on-screen keyboard tooltip
	TooltipData osk_buf;
	WidgetTooltip osk_tip;

public:
	static const std::string DEFAULT_FILE;

	explicit WidgetInput(const std::string& filename);
	~WidgetInput();
	void setPos(int offset_x, int offset_y);

	void activate();
	void logic();
	bool logicAt(int x, int y);
	void render();
	std::string getText() {
		return text;
	}
	void setText(const std::string& _text) {
		text = _text;
		cursor_pos = text.length();
		trimText();
	}

	bool edit_mode;
	unsigned int max_length;
	bool only_numbers;
	bool accept_to_defocus;
};

#endif
