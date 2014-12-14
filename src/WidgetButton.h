/*
Copyright © 2011-2012 Clint Bellanger
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
 * class WidgetButton
 */

#ifndef WIDGET_BUTTON_H
#define WIDGET_BUTTON_H

#include "CommonIncludes.h"
#include "Widget.h"
#include "WidgetLabel.h"
#include "TooltipData.h"

class WidgetTooltip;

const int BUTTON_GFX_NORMAL = 0;
const int BUTTON_GFX_PRESSED = 1;
const int BUTTON_GFX_HOVER = 2;
const int BUTTON_GFX_DISABLED = 3;

class WidgetButton : public Widget {
private:

	std::string fileName; // the path to the buttons background image

	Sprite *buttons;

	WidgetLabel wlabel;

	Color color_normal;
	Color color_disabled;

	TooltipData tip_buf;
	TooltipData tip_new;
	WidgetTooltip *tip;
	TooltipData checkTooltip(Point mouse);

public:
	WidgetButton(const std::string& _fileName = "images/menus/buttons/button_default.png");
	~WidgetButton();

	void activate();

	void loadArt();
	bool checkClick();
	bool checkClick(int x, int y);
	void render();
	void refresh();

	std::string label;
	std::string tooltip;
	bool enabled;
	bool pressed;
	bool hover;
};

#endif
