/*
Copyright © 2011-2012 Clint Bellanger
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
 * class WidgetLabel
 *
 * A simple text display for menus
 */

#include "FontEngine.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "WidgetLabel.h"

LabelInfo::LabelInfo()
	: x(0)
	, y(0)
	, justify(FontEngine::JUSTIFY_LEFT)
	, valign(VALIGN_TOP)
	, hidden(false)
	, font_style("font_regular") {
}

const std::string WidgetLabel::DEFAULT_FONT = "font_regular";

WidgetLabel::WidgetLabel()
	: justify(FontEngine::JUSTIFY_LEFT)
	, valign(LabelInfo::VALIGN_TOP)
	, max_width(0)
	, update_flag(UPDATE_NONE)
	, hidden(false)
	, window_resize_flag(false)
	, label(NULL)
	, text("")
	, font_style(DEFAULT_FONT)
	, color(font->getColor(FontEngine::COLOR_WIDGET_NORMAL))
{
	bounds.x = bounds.y = 0;
	bounds.w = bounds.h = 0;
	enable_tablist_nav = false;
}

void WidgetLabel::setMaxWidth(int width) {
	if (width != max_width) {
		max_width = width;
		setUpdateFlag(UPDATE_RECACHE);
	}
}

void WidgetLabel::setHidden(bool _hidden) {
	hidden = _hidden;
}

void WidgetLabel::setPos(int offset_x, int offset_y) {
	Rect old_pos = pos;
	Widget::setPos(offset_x, offset_y);

	if (old_pos.x != pos.x || old_pos.y != pos.y) {
		setUpdateFlag(UPDATE_POS);
	}
}

void WidgetLabel::setJustify(int _justify) {
	if (justify != _justify) {
		justify = _justify;
		setUpdateFlag(UPDATE_RECACHE);
	}
}

void WidgetLabel::setText(const std::string& _text) {
	if (text != _text) {
		text = _text;
		setUpdateFlag(UPDATE_RECACHE);
	}
}

void WidgetLabel::setVAlign(int _valign) {
	if (valign != _valign) {
		valign = _valign;
		setUpdateFlag(UPDATE_RECACHE);
	}
}

void WidgetLabel::setColor(const Color& _color) {
	if (color.r != _color.r || color.g != _color.g || color.b != _color.b) {
		color = _color;
		setUpdateFlag(UPDATE_RECACHE);
	}
}

void WidgetLabel::setFont(const std::string& _font) {
	if (font_style != _font) {
		font_style = _font;
		setUpdateFlag(UPDATE_RECACHE);
	}
}

void WidgetLabel::setFromLabelInfo(const LabelInfo& label_info) {
	if (pos_base.x != label_info.x || pos_base.y != label_info.y)
		setUpdateFlag(UPDATE_POS);

	setBasePos(label_info.x, label_info.y, alignment);

	setJustify(label_info.justify);
	setVAlign(label_info.valign);
	setFont(label_info.font_style);
	setHidden(label_info.hidden);
}

std::string WidgetLabel::getText() {
	return text;
}

/**
 * Gets the label's dimensions, re-caching beforehand if necessary
 */
Rect* WidgetLabel::getBounds() {
	update();
	return &bounds;
}

bool WidgetLabel::isHidden() {
	return hidden;
}

/**
 * Apply horizontal justify and vertical alignment to label position
 */
void WidgetLabel::applyOffsets() {
	// apply JUSTIFY
	if (justify == FontEngine::JUSTIFY_LEFT)
		bounds.x = pos.x;
	else if (justify == FontEngine::JUSTIFY_RIGHT)
		bounds.x = pos.x - bounds.w;
	else if (justify == FontEngine::JUSTIFY_CENTER)
		bounds.x = pos.x - bounds.w/2;

	// apply VALIGN
	if (valign == LabelInfo::VALIGN_TOP) {
		bounds.y = pos.y;
	}
	else if (valign == LabelInfo::VALIGN_BOTTOM) {
		bounds.y = pos.y - bounds.h;
	}
	else if (valign == LabelInfo::VALIGN_CENTER) {
		bounds.y = pos.y - bounds.h/2;
	}

	if (label) {
		label->setDestFromRect(bounds);
	}
}

/**
 * We buffer the rendered text instead of calculating it each frame
 * This function refreshes the buffer.
 */
void WidgetLabel::recacheTextSprite() {
	Image *image;

	if (label) {
		delete label;
		label = NULL;
	}

	if (text.empty())
		return;

	std::string temp_text = text;

	font->setFont(font_style);
	bounds.w = font->calc_width(temp_text);
	bounds.h = font->getFontHeight();

	if (max_width > 0 && bounds.w > max_width) {
		temp_text = font->trimTextToWidth(text, max_width, FontEngine::USE_ELLIPSIS, 0);
		bounds.w = font->calc_width(temp_text);
	}

	image = render_device->createImage(bounds.w, bounds.h);
	if (!image) return;

	font->renderShadowed(temp_text, 0, 0, FontEngine::JUSTIFY_LEFT, image, 0, color);
	label = image->createSprite();
	image->unref();
}

/**
 * Sets a flag so that update() knows what's changed
 */
void WidgetLabel::setUpdateFlag(int _update_flag) {
	if (_update_flag > update_flag || _update_flag == UPDATE_NONE)
		update_flag = _update_flag;
}

/**
 * Runs recacheTextSprite() and applyOffsets() based on what's changed
 */
void WidgetLabel::update() {
	// we only need to check if the window was resized once per frame
	// yet, this update function may be called multiple times (ex. getBounds())
	// so we set a flag to prevent unnecessary re-caching
	if (inpt->window_resized && !window_resize_flag) {
		setUpdateFlag(UPDATE_RECACHE);
		window_resize_flag = true;
	}

	if (update_flag == UPDATE_RECACHE)
		recacheTextSprite();

	if (update_flag >= UPDATE_POS)
		applyOffsets();

	setUpdateFlag(UPDATE_NONE);
}

/**
 * Draw the buffered string surface to the screen
 */
void WidgetLabel::render() {
	if (hidden)
		return;

	update();

	if (label) {
		label->local_frame = local_frame;
		label->setOffset(local_offset);
		render_device->render(label);
	}

	// reset flag
	window_resize_flag = false;
}

WidgetLabel::~WidgetLabel() {
	if (label) delete label;
}
