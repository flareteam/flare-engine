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
	: tip(new WidgetTooltip())
	, style(TooltipData::STYLE_FLOAT)
	, context(CONTEXT_NONE)
{}

TooltipManager::~TooltipManager() {
	delete tip;
}

void TooltipManager::clear() {
	tip_data.clear();
}

bool TooltipManager::isEmpty() {
	return tip_data.isEmpty();
}

void TooltipManager::push(const TooltipData& _tip_data, const Point& _pos, uint8_t _style) {
	if (_tip_data.isEmpty())
		return;

	tip_data = _tip_data;
	pos = _pos;
	style = _style;
}

void TooltipManager::render() {
	if (!isEmpty()) {
		context = CONTEXT_MENU;
	}
	else if (context != CONTEXT_MAP) {
		context = CONTEXT_NONE;
	}

	tip->render(tip_data, pos, style);
}
