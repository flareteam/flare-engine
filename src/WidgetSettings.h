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
 * class WidgetSettings
 */

#ifndef WIDGET_SETTINGS_H
#define WIDGET_SETTINGS_H

#include "Utils.h"

class WidgetSettings {
public:
	WidgetSettings();
	~WidgetSettings();
	void load();

	Color selection_rect_color;
	Point tab_padding;
};

extern WidgetSettings widget_settings;

#endif
