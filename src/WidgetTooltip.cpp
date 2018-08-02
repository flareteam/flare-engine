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

#include "EngineSettings.h"
#include "FontEngine.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "WidgetTooltip.h"

WidgetTooltip::WidgetTooltip() {
	background = render_device->loadImage("images/menus/tooltips.png", RenderDevice::ERROR_NONE);
	sprite_buf = NULL;
}

WidgetTooltip::~WidgetTooltip() {
	if (background)
		background->unref();

	delete sprite_buf;
}

/**
 * Knowing the total size of the text and the position of origin,
 * calculate the starting position of the background and text
 */
Point WidgetTooltip::calcPosition(uint8_t style, const Point& pos, const Point& size) {

	Point tip_pos;

	// TopLabel style is fixed and centered over the origin
	if (style == TooltipData::STYLE_TOPLABEL) {
		tip_pos.x = pos.x - size.x/2;
		tip_pos.y = pos.y - eset->tooltips.offset;
	}
	// Float style changes position based on the screen quadrant of the origin
	// (usually used for tooltips which are long and we don't want them to overflow
	//  off the end of the screen)
	else if (style == TooltipData::STYLE_FLOAT) {
		// upper left
		if (pos.x < settings->view_w_half && pos.y < settings->view_h_half) {
			tip_pos.x = pos.x + eset->tooltips.offset;
			tip_pos.y = pos.y + eset->tooltips.offset;
		}
		// upper right
		else if (pos.x >= settings->view_w_half && pos.y < settings->view_h_half) {
			tip_pos.x = pos.x - eset->tooltips.offset - size.x;
			tip_pos.y = pos.y + eset->tooltips.offset;
		}
		// lower left
		else if (pos.x < settings->view_w_half && pos.y >= settings->view_h_half) {
			tip_pos.x = pos.x + eset->tooltips.offset;
			tip_pos.y = pos.y - eset->tooltips.offset - size.y;
		}
		// lower right
		else if (pos.x >= settings->view_w_half && pos.y >= settings->view_h_half) {
			tip_pos.x = pos.x - eset->tooltips.offset - size.x;
			tip_pos.y = pos.y - eset->tooltips.offset - size.y;
		}

		// very large tooltips might still be off screen at this point
		// so we try to constrain them to the screen bounds
		// we give priority to being able to read the top-left of the tooltip over the bottom-right
		if (tip_pos.x + size.x > settings->view_w) tip_pos.x = settings->view_w - size.x;
		if (tip_pos.y + size.y > settings->view_h) tip_pos.y = settings->view_h - size.y;
		if (tip_pos.x < 0) tip_pos.x = 0;
		if (tip_pos.y < 0) tip_pos.y = 0;
	}

	return tip_pos;
}

/**
 * Creates the cached text buffer if needed and sets the position & bounds of the tooltip
 */
void WidgetTooltip::prerender(TooltipData&tip, const Point& pos, uint8_t style) {
	if (sprite_buf == NULL || !tip.compare(data_buf)) {
		if (!createBuffer(tip)) return;
	}

	Point size;
	size.x = sprite_buf->getGraphicsWidth();
	size.y = sprite_buf->getGraphicsHeight();

	Point tip_pos = calcPosition(style, pos, size);

	sprite_buf->setDestFromPoint(tip_pos);

	bounds.x = tip_pos.x;
	bounds.y = tip_pos.y;
	bounds.w = size.x;
	bounds.h = size.y;
}

/**
 * Tooltip position depends on the screen quadrant of the source.
 * Draw the buffered tooltip if it exists, else render the tooltip and buffer it
 */
void WidgetTooltip::render(TooltipData &tip, const Point& pos, uint8_t style) {
	if (tip.isEmpty())
		return;

	prerender(tip, pos, style);
	render_device->render(sprite_buf);
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
	Point size = font->calc_size(fulltext, eset->tooltips.width);

	// WARNING: dynamic memory allocation. Be careful of memory leaks.
	if (sprite_buf) {
		delete sprite_buf;
		sprite_buf = NULL;
	}

	Image *graphics;
	graphics = render_device->createImage(size.x + (eset->tooltips.margin*2), size.y + (eset->tooltips.margin*2));

	if (!graphics) {
		Utils::logError("WidgetTooltip: Could not create tooltip buffer.");
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
		src.w = graphics->getWidth() - eset->tooltips.background_border;
		src.h = graphics->getHeight() - eset->tooltips.background_border;
		dest.x = 0;
		dest.y = 0;
		render_device->renderToImage(background, src, graphics, dest);

		// right
		src.x = background->getWidth() - eset->tooltips.background_border;
		src.y = 0;
		src.w = eset->tooltips.background_border;
		src.h = graphics->getHeight() - eset->tooltips.background_border;
		dest.x = graphics->getWidth() - eset->tooltips.background_border;
		dest.y = 0;
		render_device->renderToImage(background, src, graphics, dest);

		// bottom
		src.x = 0;
		src.y = background->getHeight() - eset->tooltips.background_border;
		src.w = graphics->getWidth() - eset->tooltips.background_border;
		src.h = eset->tooltips.background_border;
		dest.x = 0;
		dest.y = graphics->getHeight() - eset->tooltips.background_border;
		render_device->renderToImage(background, src, graphics, dest);

		// bottom right
		src.x = background->getWidth() - eset->tooltips.background_border;
		src.y = background->getHeight() - eset->tooltips.background_border;
		src.w = eset->tooltips.background_border;
		src.h = eset->tooltips.background_border;
		dest.x = graphics->getWidth() - eset->tooltips.background_border;
		dest.y = graphics->getHeight() - eset->tooltips.background_border;
		render_device->renderToImage(background, src, graphics, dest);
	}

	int cursor_y = eset->tooltips.margin;

	for (unsigned int i=0; i<tip.lines.size(); i++) {
		if (background)
			font->renderShadowed(tip.lines[i], eset->tooltips.margin, cursor_y, FontEngine::JUSTIFY_LEFT, graphics, size.x, tip.colors[i]);
		else
			font->render(tip.lines[i], eset->tooltips.margin, cursor_y, FontEngine::JUSTIFY_LEFT, graphics, size.x, tip.colors[i]);

		cursor_y = font->cursor_y;
	}

	sprite_buf = graphics->createSprite();
	graphics->unref();

	data_buf = tip;
	return true;
}

