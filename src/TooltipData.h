/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2014 Henrik Andersson
Copyright © 2014-2016 Justin Jacobs

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

#ifndef TOOLTIPDATA_H
#define TOOLTIPDATA_H

#include "CommonIncludes.h"
#include "Utils.h"

/**
 * TooltipData contains the text and line colors for one tool tip.
 * Useful for keeping the data separate from the widget itself, so the data
 * can be passed around easily.
 *
 * Contains a image buffer to keep a render of the tooltip, rather than needing
 * to render it each frame. This buffer is not copied during copy/assign to
 * avoid multiple deconstructors on the same dynamically allocated memory. Thus
 * the new copy will recreate its own buffer next time it is displayed.
 */
class TooltipData {
public:
	enum {
		STYLE_FLOAT = 0,
		STYLE_TOPLABEL = 1
	};

	std::vector<std::string> lines;
	std::vector<Color> colors;

	TooltipData();
	~TooltipData();

	// add text with support for new lines
	void addColoredText(const std::string &text, const Color& color);
	void addText(const std::string &text);

	void clear();
	bool isEmpty() const;

	// compare the first line
	bool compareFirstLine(const std::string &text);

	// compare all lines
	bool compare(const TooltipData& tip);
};

#endif // TOOLTIPDATA_H
