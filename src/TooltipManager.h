/*
Copyright © 2018 Justin Jacobs

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
 * class TooltipManager
 */

#ifndef TOOLTIP_MANAGER_H
#define TOOLTIP_MANAGER_H

#include "TooltipData.h"
#include "Utils.h"

class WidgetTooltip;

class TooltipManager {
private:
	WidgetTooltip *tip;
	TooltipData tip_data;
	Point pos;
	uint8_t style;

public:
	enum {
		CONTEXT_NONE = 0,
		CONTEXT_MENU = 1,
		CONTEXT_MAP = 2
	};

	TooltipManager();
	~TooltipManager();
	TooltipManager(const TooltipManager& copy); // copy constructor not implemented

	void clear();
	bool isEmpty();
	void push(const TooltipData& _tip_data, const Point& _pos, uint8_t _style);
	void render();

	uint8_t context;
};

#endif
