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

#include "FileParser.h"
#include "UtilsParsing.h"
#include "WidgetSettings.h"

WidgetSettings widget_settings;

WidgetSettings::WidgetSettings() {
}

WidgetSettings::~WidgetSettings() {
}

void WidgetSettings::load() {
	// set defaults
	selection_rect_color = Color(255, 248, 220, 255);
	tab_padding = Point(8, 0);

	FileParser infile;
	// @CLASS WidgetSettings|Description of engine/widget_settings.txt
	if (infile.open("engine/widget_settings.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		while (infile.next()) {
			if (infile.section == "misc") {
				if (infile.key == "selection_rect_color") {
					// @ATTR misc.selection_rect_color|color, int : Color, Alpha|Color of the selection rectangle when navigating widgets without a mouse.
					selection_rect_color = toRGBA(infile.val);
				}
			}
			else if (infile.section == "tab") {
				if (infile.key == "padding") {
					// @ATTR tab.padding|int, int : Left/right padding, Top padding|The pixel padding around tabs. Controls how the left and right edges are drawn.
					tab_padding = toPoint(infile.val);
				}
			}
		}
	}
}
