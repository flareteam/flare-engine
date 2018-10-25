/*
Copyright © 2011-2012 Clint Bellanger
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
 * class WidgetButton
 */

#ifndef WIDGET_BUTTON_H
#define WIDGET_BUTTON_H

#include "CommonIncludes.h"
#include "Widget.h"
#include "WidgetLabel.h"

class WidgetButton : public Widget {
private:
	std::string fileName; // the path to the buttons background image

	Sprite *buttons;

	WidgetLabel wlabel;

	void checkTooltip(const Point& mouse);

	bool activated;

	std::string label;

	Color text_color_normal;
	Color text_color_pressed;
	Color text_color_hover;
	Color text_color_disabled;

public:
	static const std::string DEFAULT_FILE;
	static const std::string NO_FILE;

	enum {
		BUTTON_NORMAL = 0,
		BUTTON_PRESSED = 1,
		BUTTON_HOVER = 2,
		BUTTON_DISABLED = 3
	};

	explicit WidgetButton(const std::string& _fileName);
	~WidgetButton();

	void activate();
	void setPos(int offset_x, int offset_y);
	void setLabel(const std::string& s);
	void setTextColor(int state, Color c);

	void loadArt();
	bool checkClick();
	bool checkClickAt(int x, int y);
	void render();
	void refresh();

	std::string tooltip;
	bool enabled;
	bool pressed;
	bool hover;
};

#endif
