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

#include "TooltipManager.h"
#include "WidgetTooltip.h"

TooltipManager::TooltipManager()
	: context(CONTEXT_NONE)
{
	for (size_t i = 0; i < TOOLTIP_COUNT; ++i) {
		tip[i] = new WidgetTooltip();
		if (i > 0) {
			tip[i]->parent = tip[i-1];
		}

		tip_data[i] = TooltipData();
		pos[i] = Point();
		style[i] = 0;
	}
}

TooltipManager::~TooltipManager() {
	for (size_t i = 0; i < TOOLTIP_COUNT; ++i) {
		delete tip[i];
	}
}

void TooltipManager::clear() {
	for (size_t i = 0; i < TOOLTIP_COUNT; ++i) {
		tip_data[i].clear();
	}
}

bool TooltipManager::isEmpty() {
	for (size_t i = 0; i < TOOLTIP_COUNT; ++i) {
		if (!tip_data[i].isEmpty())
			return false;
	}
	return true;
}

void TooltipManager::push(const TooltipData& _tip_data, const Point& _pos, uint8_t _style, size_t tip_index) {
	if (_tip_data.isEmpty() || tip_index >= TOOLTIP_COUNT)
		return;

	tip_data[tip_index] = _tip_data;
	pos[tip_index] = _pos;
	style[tip_index] = _style;
}

void TooltipManager::render() {
	if (!isEmpty()) {
		context = CONTEXT_MENU;
	}
	else if (context != CONTEXT_MAP) {
		context = CONTEXT_NONE;
	}

	for (size_t i = 0; i < TOOLTIP_COUNT; ++i) {
		tip[i]->render(tip_data[i], pos[i], style[i]);
	}
}
