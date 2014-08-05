/*
Copyright © 2011-2012 kitano
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
 * class WidgetInput
 *
 * A simple text box with a label above it.
 * It has two images - one for focused and one for unfocused.
 */

#pragma once
#ifndef WIDGETINPUT_H
#define WIDGETINPUT_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "Widget.h"

class WidgetInput : public Widget {

protected:

	void loadGraphics(const std::string& filename);

	Sprite *background;

	bool enabled;
	bool pressed;
	bool hover;

	std::string text; // the text that has been typed into the box
	unsigned int max_characters;
	int cursor_frame;

	Point font_pos;

	Color color_normal;

public:
	WidgetInput(const std::string& filename);
	~WidgetInput();

	void logic();
	bool logic(int x, int y);
	void render();
	bool checkClick();
	std::string getText() {
		return text;
	}
	void setText(std::string _text) {
		text = _text;
	}
	void setPosition(int x, int y);

	bool inFocus;
};

#endif
