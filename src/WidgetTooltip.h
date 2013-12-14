/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert

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
 * class WidgetTooltip
 */

#pragma once
#ifndef WIDGET_TOOLTIP_H
#define WIDGET_TOOLTIP_H

#include "CommonIncludes.h"
#include "SharedResources.h"
#include "TooltipData.h"
#include "Utils.h"

extern int TOOLTIP_CONTEXT;
const int TOOLTIP_NONE = 0;
const int TOOLTIP_MAP = 1;
const int TOOLTIP_MENU = 2;

class WidgetTooltip {
private:
	int offset; // distance between cursor and tooltip
	int width; // max width of tooltips (wrap text)
	int margin; // outer margin between tooltip text and the edge of the tooltip background
public:
	WidgetTooltip();
	Point calcPosition(STYLE style, Point pos, Point size);
	void render(TooltipData &tip, Point pos, STYLE style);
	void createBuffer(TooltipData &tip);
};

#endif
