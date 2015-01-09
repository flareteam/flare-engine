/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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
 * class WidgetTooltip
 */

#include "FileParser.h"
#include "WidgetTooltip.h"
#include "Settings.h"
#include "Utils.h"
#include "UtilsParsing.h"

int TOOLTIP_CONTEXT = TOOLTIP_NONE;

WidgetTooltip::WidgetTooltip()
	: offset(0)
	, width(0)
	, margin(0) {

	FileParser infile;
	// load tooltip settings from engine config file
	// @CLASS WidgetTooltip|Description of engine/tooltips.txt
	if (infile.open("engine/tooltips.txt")) {
		while (infile.next()) {
			// @ATTR tooltip_offset|integer|Offset in pixels from the origin point (usually mouse cursor).
			if (infile.key == "tooltip_offset")
				offset = toInt(infile.val);
			// @ATTR tooltip_width|integer|Maximum width of tooltip in pixels.
			else if (infile.key == "tooltip_width")
				width = toInt(infile.val);
			// @ATTR tooltip_margin|integer|Padding between the text and the tooltip borders.
			else if (infile.key == "tooltip_margin")
				margin = toInt(infile.val);
		}
		infile.close();
	}
}

/**
 * Knowing the total size of the text and the position of origin,
 * calculate the starting position of the background and text
 */
Point WidgetTooltip::calcPosition(STYLE style, Point pos, Point size) {

	Point tip_pos;

	// TopLabel style is fixed and centered over the origin
	if (style == STYLE_TOPLABEL) {
		tip_pos.x = pos.x - size.x/2;
		tip_pos.y = pos.y - offset;
	}
	// Float style changes position based on the screen quadrant of the origin
	// (usually used for tooltips which are long and we don't want them to overflow
	//  off the end of the screen)
	else if (style == STYLE_FLOAT) {
		// upper left
		if (pos.x < VIEW_W_HALF && pos.y < VIEW_H_HALF) {
			tip_pos.x = pos.x + offset;
			tip_pos.y = pos.y + offset;
		}
		// upper right
		else if (pos.x >= VIEW_W_HALF && pos.y < VIEW_H_HALF) {
			tip_pos.x = pos.x - offset - size.x;
			tip_pos.y = pos.y + offset;
		}
		// lower left
		else if (pos.x < VIEW_W_HALF && pos.y >= VIEW_H_HALF) {
			tip_pos.x = pos.x + offset;
			tip_pos.y = pos.y - offset - size.y;
		}
		// lower right
		else if (pos.x >= VIEW_W_HALF && pos.y >= VIEW_H_HALF) {
			tip_pos.x = pos.x - offset - size.x;
			tip_pos.y = pos.y - offset - size.y;
		}
	}

	return tip_pos;
}

/**
 * Tooltip position depends on the screen quadrant of the source.
 * Draw the buffered tooltip if it exists, else render the tooltip and buffer it
 */
void WidgetTooltip::render(TooltipData &tip, Point pos, STYLE style) {
	if (tip.tip_buffer == NULL) {
		if (!createBuffer(tip)) return;
	}

	Point size;
	size.x = tip.tip_buffer->getGraphicsWidth();
	size.y = tip.tip_buffer->getGraphicsHeight();

	Point tip_pos = calcPosition(style, pos, size);

	tip.tip_buffer->setDestX(tip_pos.x);
	tip.tip_buffer->setDestY(tip_pos.y);
	render_device->render(tip.tip_buffer);
}

/**
 * Rendering a wordy tooltip (TTF to raster) can be expensive.
 * Instead of doing this each frame, do it once and cache the result.
 */
bool WidgetTooltip::createBuffer(TooltipData &tip) {

	if (tip.lines.empty()) {
		tip.lines.resize(1);
		tip.colors.resize(1);
	}

	// concat multi-line tooltip, used in determining total display size
	std::string fulltext;
	fulltext = tip.lines[0];
	for (unsigned int i=1; i<tip.lines.size(); i++) {
		fulltext = fulltext + "\n" + tip.lines[i];
	}

	font->setFont("font_regular");

	// calculate the full size to display a multi-line tooltip
	Point size = font->calc_size(fulltext, width);

	// WARNING: dynamic memory allocation. Be careful of memory leaks.
	if (tip.tip_buffer) {
		delete tip.tip_buffer;
		tip.tip_buffer = NULL;
	}

	Image *graphics;
	graphics = render_device->createImage(size.x + margin+margin, size.y + margin+margin);

	if (!graphics) {
		logError("WidgetTooltip: Could not create tooltip buffer.");
		return false;
	}

	// style the tooltip background
	// currently this is plain black
	graphics->fillWithColor(graphics->MapRGB(0,0,0));

	int cursor_y = margin;

	for (unsigned int i=0; i<tip.lines.size(); i++) {
		font->render(tip.lines[i], margin, cursor_y, JUSTIFY_LEFT, graphics, size.x, tip.colors[i]);
		cursor_y = font->cursor_y;
	}

	tip.tip_buffer = graphics->createSprite();
	graphics->unref();

	return true;
}

