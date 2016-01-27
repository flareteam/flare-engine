/*
Copyright © 2012 Justin Jacobs
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
 * class WidgetSlider
 */

#include "CommonIncludes.h"
#include "SharedResources.h"
#include "UtilsDebug.h"
#include "Widget.h"
#include "WidgetSlider.h"

#include <assert.h>

WidgetSlider::WidgetSlider (const std::string& fname)
	: enabled(true)
	, sl(NULL)
	, pressed(false)
	, minimum(0)
	, maximum(0)
	, value(0) {

	Image *graphics;
	graphics = render_device->loadImage(fname, "loading slider graphics", true);
	if (graphics) {
		sl = graphics->createSprite();
		pos.w = sl->getGraphicsWidth();
		pos.h = sl->getGraphicsHeight() / 2;
		pos_knob.w = sl->getGraphicsWidth() / 8;
		pos_knob.h = sl->getGraphicsHeight() / 2;
		graphics->unref();
	}

	render_to_alpha = false;

	scroll_type = HORIZONTAL;
}

WidgetSlider::~WidgetSlider () {
	if (sl) delete sl;
}

void WidgetSlider::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);
	set(minimum, maximum, value);
}

bool WidgetSlider::checkClick() {
	if (!enabled) return false;
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}


bool WidgetSlider::checkClick (int x, int y) {
	if (!enabled) return false;
	Point mouse(x, y);
	//
	//	We are just grabbing the knob
	//
	if (!pressed && inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
		if (isWithin(pos_knob, mouse)) {
			pressed = true;
			inpt->lock[MAIN1] = true;
			return true;
		}
		return false;
	}

	// buttons already in use, new click not allowed
	if (inpt->lock[UP]) return false;
	if (inpt->lock[DOWN]) return false;

	if (!pressed && !inpt->lock[UP] && !inpt->lock[DOWN] && !inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
		return true;
	}
	if (pressed) {
		//
		// The knob has been released
		//
		// create a temporary Rect slightly wider than the slider
		Rect tmp_pos;
		tmp_pos.x = pos.x - (pos_knob.w*2);
		tmp_pos.y = pos.y;
		tmp_pos.w = pos.w + (pos_knob.w*4);
		tmp_pos.h = pos.h;

		if (!isWithin(tmp_pos, mouse)) {
			pressed = false;
			return false;
		}
		if (!inpt->lock[MAIN1]) {
			pressed = false;
		}

		// set the value of the slider
		int tmp = std::max(0, std::min(mouse.x - pos.x, static_cast<int>(pos.w)));

		pos_knob.x = pos.x + tmp - (pos_knob.w/2);
		assert(pos.w);
		value = minimum + (tmp*(maximum-minimum))/pos.w;

		return true;
	}
	return false;
}


void WidgetSlider::set (int min, int max, int val) {
	minimum = min;
	maximum = max;
	value = val;

	if (max-min != 0) {
		pos_knob.x = pos.x + ((val-min)* pos.w)/(max-min) - (pos_knob.w/2);
		pos_knob.y = pos.y;
	}
}


int WidgetSlider::getValue () const {
	return value;
}


void WidgetSlider::render () {
	Rect	base;
	base.x = 0;
	base.y = 0;
	base.h = pos.h;
	base.w = pos.w;

	Rect	knob;
	knob.x = 0;
	knob.y = pos.h;
	knob.h = pos_knob.h;
	knob.w = pos_knob.w;

	if (sl) {
		sl->local_frame = local_frame;
		sl->setOffset(local_offset);
		sl->setClip(base);
		sl->setDest(pos);
		render_device->render(sl);
		sl->setClip(knob);
		sl->setDest(pos_knob);
		render_device->render(sl);
	}

	if (in_focus) {
		Point topLeft;
		Point bottomRight;

		topLeft.x = pos.x + local_frame.x - local_offset.x;
		topLeft.y = pos.y + local_frame.y - local_offset.y;
		bottomRight.x = topLeft.x + pos.w;
		bottomRight.y = topLeft.y + pos.h;
		Color color = Color(255,248,220,255);

		// Only draw rectangle if it fits in local frame
		bool draw = true;
		if (local_frame.w &&
				(topLeft.x<local_frame.x || bottomRight.x>(local_frame.x+local_frame.w))) {
			draw = false;
		}
		if (local_frame.h &&
				(topLeft.y<local_frame.y || bottomRight.y>(local_frame.y+local_frame.h))) {
			draw = false;
		}
		if (draw) {
			render_device->drawRectangle(topLeft, bottomRight, color);
		}
	}
}

bool WidgetSlider::getPrev() {
	if (!enabled) return false;

	value -= (maximum - minimum)/10;
	if (value < minimum)
		value = minimum;

	pos_knob.x = pos.x + ((value-minimum)* pos.w)/(maximum-minimum) - (pos_knob.w/2);
	pos_knob.y = pos.y;

	return true;
}

bool WidgetSlider::getNext() {
	if (!enabled) return false;

	value += (maximum - minimum)/10;
	if (value > maximum)
		value = maximum;

	pos_knob.x = pos.x + ((value-minimum)* pos.w)/(maximum-minimum) - (pos_knob.w/2);
	pos_knob.y = pos.y;

	return true;
}


