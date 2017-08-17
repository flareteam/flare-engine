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

#include "WidgetLabel.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

LabelInfo::LabelInfo() : x(0), y(0), justify(JUSTIFY_LEFT), valign(VALIGN_TOP), hidden(false), font_style("font_regular") {
}

/**
 * This is used in menus (e.g. MenuInventory) when parsing their config files
 */
LabelInfo eatLabelInfo(std::string val) {
	LabelInfo info;
	std::string justify,valign,style;

	std::string tmp = popFirstString(val);
	if (tmp == "hidden") {
		info.hidden = true;
	}
	else {
		info.hidden = false;
		info.x = toInt(tmp);
		info.y = popFirstInt(val);
		justify = popFirstString(val);
		valign = popFirstString(val);
		style = popFirstString(val);

		if (justify == "left") info.justify = JUSTIFY_LEFT;
		else if (justify == "center") info.justify = JUSTIFY_CENTER;
		else if (justify == "right") info.justify = JUSTIFY_RIGHT;

		if (valign == "top") info.valign = VALIGN_TOP;
		else if (valign == "center") info.valign = VALIGN_CENTER;
		else if (valign == "bottom") info.valign = VALIGN_BOTTOM;

		if (style != "") info.font_style = style;
	}

	return info;
}

WidgetLabel::WidgetLabel()
	: justify(JUSTIFY_LEFT)
	, valign(VALIGN_TOP)
	, max_width(0)
	, label(NULL)
	, text("")
	, font_style("font_regular")
	, color(font->getColor("widget_normal"))
{
	bounds.x = bounds.y = 0;
	bounds.w = bounds.h = 0;

	render_to_alpha = false;
}

/**
 * Draw the buffered string surface to the screen
 */
void WidgetLabel::render() {
	if (label) {
		label->local_frame = local_frame;
		label->setOffset(local_offset);
		render_device->render(label);
	}
}

void WidgetLabel::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);
	applyOffsets();
}

void WidgetLabel::setMaxWidth(int width) {
	if (width != max_width) {
		max_width = width;
		recacheTextSprite();
	}
}

void WidgetLabel::set(int _x, int _y, int _justify, int _valign, const std::string& _text, const Color& _color) {
	set(_x, _y, _justify, _valign, _text, _color, "font_regular");
}

/**
 * A shortcut function to set all attributes simultaneously.
 */
void WidgetLabel::set(int _x, int _y, int _justify, int _valign, const std::string& _text, const Color& _color, const std::string& _font) {

	bool changed = false;
	bool changed_pos = false;

	if (justify != _justify) {
		justify = _justify;
		changed = true;
	}
	if (valign != _valign) {
		valign = _valign;
		changed = true;
	}
	if (text != _text) {
		text = _text;
		changed = true;
	}
	if (color.r != _color.r || color.g != _color.g || color.b != _color.b) {
		color = _color;
		changed = true;
	}
	if (pos.x != _x) {
		pos.x = _x;
		changed_pos = true;
	}
	if (pos.y != _y) {
		pos.y = _y;
		changed_pos = true;
	}
	if (font_style != _font) {
		font_style = _font;
		changed = true;
	}

	if (changed) {
		recacheTextSprite();
	}
	else if (changed_pos) {
		applyOffsets();
	}
}

/**
 * Set initial X position of label.
 */
void WidgetLabel::setX(int _x) {
	if (pos.x != _x) {
		pos.x = _x;
		applyOffsets();
	}
}

/**
 * Set initial Y position of label.
 */
void WidgetLabel::setY(int _y) {
	if (pos.y != _y) {
		pos.y = _y;
		applyOffsets();
	}
}

/**
 * Get X position of label.
 */
int WidgetLabel::getX() {
	return pos.x;
}

/**
 * Get Y position of label.
 */
int WidgetLabel::getY() {
	return pos.y;
}

/**
 * Set justify value.
 */
void WidgetLabel::setJustify(int _justify) {
	if (justify != _justify) {
		justify = _justify;
		recacheTextSprite();
	}
}

/**
 * Apply horizontal justify and vertical alignment to label position
 */
void WidgetLabel::applyOffsets() {
	// apply JUSTIFY
	if (justify == JUSTIFY_LEFT)
		bounds.x = pos.x;
	else if (justify == JUSTIFY_RIGHT)
		bounds.x = pos.x - bounds.w;
	else if (justify == JUSTIFY_CENTER)
		bounds.x = pos.x - bounds.w/2;

	// apply VALIGN
	if (valign == VALIGN_TOP) {
		bounds.y = pos.y;
	}
	else if (valign == VALIGN_BOTTOM) {
		bounds.y = pos.y - bounds.h;;
	}
	else if (valign == VALIGN_CENTER) {
		bounds.y = pos.y - bounds.h/2;
	}

	if (label) {
		label->setDestX(bounds.x);
		label->setDestY(bounds.y);
	}
}

/**
 * Update the label text only
 */
void WidgetLabel::set(const std::string& _text) {
	if (text != _text) {
		this->text = _text;
		recacheTextSprite();
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
		temp_text = font->trimTextToWidth(text, max_width, true, 0);
		bounds.w = font->calc_width(temp_text);
	}

	image = render_device->createImage(bounds.w, bounds.h);
	if (!image) return;

	font->renderShadowed(temp_text, 0, 0, JUSTIFY_LEFT, image, 0, color);
	label = image->createSprite();
	image->unref();

	applyOffsets();
}

WidgetLabel::~WidgetLabel() {
	if (label) delete label;
}
