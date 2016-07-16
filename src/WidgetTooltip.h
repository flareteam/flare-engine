/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
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
 * class WidgetTooltip
 */

#ifndef WIDGET_TOOLTIP_H
#define WIDGET_TOOLTIP_H

#include "CommonIncludes.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "TooltipData.h"
#include "Utils.h"

extern int TOOLTIP_CONTEXT;
const int TOOLTIP_NONE = 0;
const int TOOLTIP_MAP = 1;
const int TOOLTIP_MENU = 2;

class WidgetTooltip {
public:
	WidgetTooltip();
	~WidgetTooltip();
	Point calcPosition(STYLE style, const Point& pos, const Point& size);
	void render(TooltipData &tip, const Point& pos, STYLE style);
	bool createBuffer(TooltipData &tip);

	Rect bounds;

private:
	Image *background;
};

#endif
