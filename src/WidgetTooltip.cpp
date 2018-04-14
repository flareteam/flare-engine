/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
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

#include "FontEngine.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "WidgetTooltip.h"

int TOOLTIP_CONTEXT = TOOLTIP_NONE;

WidgetTooltip::WidgetTooltip() {
	background = render_device->loadImage("images/menus/tooltips.png", "", false);
}

WidgetTooltip::~WidgetTooltip() {
	if (background)
		background->unref();
}

/**
 * Knowing the total size of the text and the position of origin,
 * calculate the starting position of the background and text
 */
Point WidgetTooltip::calcPosition(STYLE style, const Point& pos, const Point& size) {

	Point tip_pos;

	// TopLabel style is fixed and centered over the origin
	if (style == STYLE_TOPLABEL) {
		tip_pos.x = pos.x - size.x/2;
		tip_pos.y = pos.y - TOOLTIP_OFFSET;
	}
	// Float style changes position based on the screen quadrant of the origin
	// (usually used for tooltips which are long and we don't want them to overflow
	//  off the end of the screen)
	else if (style == STYLE_FLOAT) {
		// upper left
		if (pos.x < VIEW_W_HALF && pos.y < VIEW_H_HALF) {
			tip_pos.x = pos.x + TOOLTIP_OFFSET;
			tip_pos.y = pos.y + TOOLTIP_OFFSET;
		}
		// upper right
		else if (pos.x >= VIEW_W_HALF && pos.y < VIEW_H_HALF) {
			tip_pos.x = pos.x - TOOLTIP_OFFSET - size.x;
			tip_pos.y = pos.y + TOOLTIP_OFFSET;
		}
		// lower left
		else if (pos.x < VIEW_W_HALF && pos.y >= VIEW_H_HALF) {
			tip_pos.x = pos.x + TOOLTIP_OFFSET;
			tip_pos.y = pos.y - TOOLTIP_OFFSET - size.y;
		}
		// lower right
		else if (pos.x >= VIEW_W_HALF && pos.y >= VIEW_H_HALF) {
			tip_pos.x = pos.x - TOOLTIP_OFFSET - size.x;
			tip_pos.y = pos.y - TOOLTIP_OFFSET - size.y;
		}

		// very large tooltips might still be off screen at this point
		// so we try to constrain them to the screen bounds
		// we give priority to being able to read the top-left of the tooltip over the bottom-right
		if (tip_pos.x + size.x > VIEW_W) tip_pos.x = VIEW_W - size.x;
		if (tip_pos.y + size.y > VIEW_H) tip_pos.y = VIEW_H - size.y;
		if (tip_pos.x < 0) tip_pos.x = 0;
		if (tip_pos.y < 0) tip_pos.y = 0;
	}

	return tip_pos;
}

/**
 * Creates the cached text buffer if needed and sets the position & bounds of the tooltip
 */
void WidgetTooltip::prerender(TooltipData&tip, const Point& pos, STYLE style) {
	if (tip.tip_buffer == NULL) {
		if (!createBuffer(tip)) return;
	}

	Point size;
	size.x = tip.tip_buffer->getGraphicsWidth();
	size.y = tip.tip_buffer->getGraphicsHeight();

	Point tip_pos = calcPosition(style, pos, size);

	tip.tip_buffer->setDestX(tip_pos.x);
	tip.tip_buffer->setDestY(tip_pos.y);

	bounds.x = tip_pos.x;
	bounds.y = tip_pos.y;
	bounds.w = size.x;
	bounds.h = size.y;
}

/**
 * Tooltip position depends on the screen quadrant of the source.
 * Draw the buffered tooltip if it exists, else render the tooltip and buffer it
 */
void WidgetTooltip::render(TooltipData &tip, const Point& pos, STYLE style) {
	prerender(tip, pos, style);
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
	Point size = font->calc_size(fulltext, TOOLTIP_WIDTH);

	// WARNING: dynamic memory allocation. Be careful of memory leaks.
	if (tip.tip_buffer) {
		delete tip.tip_buffer;
		tip.tip_buffer = NULL;
	}

	Image *graphics;
	graphics = render_device->createImage(size.x + (TOOLTIP_MARGIN*2), size.y + (TOOLTIP_MARGIN*2));

	if (!graphics) {
		logError("WidgetTooltip: Could not create tooltip buffer.");
		return false;
	}

	// style the tooltip background
	if (!background) {
		graphics->fillWithColor(Color(0,0,0,255));
	}
	else {
		Rect src;
		Rect dest;

		// top left
		src.x = 0;
		src.y = 0;
		src.w = graphics->getWidth()-TOOLTIP_BACKGROUND_BORDER;
		src.h = graphics->getHeight()-TOOLTIP_BACKGROUND_BORDER;
		dest.x = 0;
		dest.y = 0;
		render_device->renderToImage(background, src, graphics, dest);

		// right
		src.x = background->getWidth()-TOOLTIP_BACKGROUND_BORDER;
		src.y = 0;
		src.w = TOOLTIP_BACKGROUND_BORDER;
		src.h = graphics->getHeight()-TOOLTIP_BACKGROUND_BORDER;
		dest.x = graphics->getWidth()-TOOLTIP_BACKGROUND_BORDER;
		dest.y = 0;
		render_device->renderToImage(background, src, graphics, dest);

		// bottom
		src.x = 0;
		src.y = background->getHeight()-TOOLTIP_BACKGROUND_BORDER;
		src.w = graphics->getWidth()-TOOLTIP_BACKGROUND_BORDER;
		src.h = TOOLTIP_BACKGROUND_BORDER;
		dest.x = 0;
		dest.y = graphics->getHeight()-TOOLTIP_BACKGROUND_BORDER;
		render_device->renderToImage(background, src, graphics, dest);

		// bottom right
		src.x = background->getWidth()-TOOLTIP_BACKGROUND_BORDER;
		src.y = background->getHeight()-TOOLTIP_BACKGROUND_BORDER;
		src.w = TOOLTIP_BACKGROUND_BORDER;
		src.h = TOOLTIP_BACKGROUND_BORDER;
		dest.x = graphics->getWidth()-TOOLTIP_BACKGROUND_BORDER;
		dest.y = graphics->getHeight()-TOOLTIP_BACKGROUND_BORDER;
		render_device->renderToImage(background, src, graphics, dest);
	}

	int cursor_y = TOOLTIP_MARGIN;

	for (unsigned int i=0; i<tip.lines.size(); i++) {
		if (background)
			font->renderShadowed(tip.lines[i], TOOLTIP_MARGIN, cursor_y, JUSTIFY_LEFT, graphics, size.x, tip.colors[i]);
		else
			font->render(tip.lines[i], TOOLTIP_MARGIN, cursor_y, JUSTIFY_LEFT, graphics, size.x, tip.colors[i]);

		cursor_y = font->cursor_y;
	}

	tip.tip_buffer = graphics->createSprite();
	graphics->unref();

	return true;
}

