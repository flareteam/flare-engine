/*
Copyright © 2012-2016 Justin Jacobs
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
#include "EngineSettings.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "TooltipManager.h"
#include "UtilsDebug.h"
#include "Widget.h"
#include "WidgetSlider.h"

#include <assert.h>

const std::string WidgetSlider::DEFAULT_FILE = "images/menus/buttons/slider_default.png";

WidgetSlider::WidgetSlider (const std::string& fname)
	: enabled(true)
	, sl(NULL)
	, pressed(false)
	, changed_without_mouse(false)
	, minimum(0)
	, maximum(0)
	, value(0) {

	Image *graphics = NULL;
	if (fname != DEFAULT_FILE) {
		graphics = render_device->loadImage(fname, RenderDevice::ERROR_NORMAL);
	}
	if (!graphics) {
		graphics = render_device->loadImage(DEFAULT_FILE, RenderDevice::ERROR_EXIT);
	}
	if (graphics) {
		sl = graphics->createSprite();
		pos.w = sl->getGraphicsWidth();
		pos.h = sl->getGraphicsHeight() / 2;
		pos_knob.w = sl->getGraphicsWidth() / 8;
		pos_knob.h = sl->getGraphicsHeight() / 2;
		graphics->unref();
	}

	scroll_type = SCROLL_HORIZONTAL;
}

WidgetSlider::~WidgetSlider () {
	if (sl) delete sl;
}

void WidgetSlider::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);
	set(minimum, maximum, value);
}

bool WidgetSlider::checkClick() {
	return checkClickAt(inpt->mouse.x,inpt->mouse.y);
}


bool WidgetSlider::checkClickAt(int x, int y) {
	enable_tablist_nav = enabled;

	if (!enabled) return false;
	Point mouse(x, y);
	//
	//	We are just grabbing the knob
	//
	if (!pressed && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
		if (Utils::isWithinRect(pos_knob, mouse)) {
			pressed = true;
			inpt->lock[Input::MAIN1] = true;
			return true;
		}
		return false;
	}

	// getNext() or getPrev() was used to change the slider, so treat it as a "click"
	if (changed_without_mouse) {
		changed_without_mouse = false;
		return true;
	}

	// buttons already in use, new click not allowed
	if (inpt->lock[Input::UP]) return false;
	if (inpt->lock[Input::DOWN]) return false;

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

		if (!Utils::isWithinRect(tmp_pos, mouse)) {
			pressed = false;
			return false;
		}
		if (!inpt->lock[Input::MAIN1]) {
			pressed = false;
		}

		// set the value of the slider
		int tmp = std::max(0, std::min(mouse.x - pos.x, pos.w));

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
		sl->setClipFromRect(base);
		sl->setDestFromRect(pos);
		render_device->render(sl);
		sl->setClipFromRect(knob);
		sl->setDestFromRect(pos_knob);
		render_device->render(sl);
	}

	if (in_focus) {
		Point topLeft;
		Point bottomRight;

		topLeft.x = pos.x + local_frame.x - local_offset.x;
		topLeft.y = pos.y + local_frame.y - local_offset.y;
		bottomRight.x = topLeft.x + pos.w;
		bottomRight.y = topLeft.y + pos.h;

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
			render_device->drawRectangle(topLeft, bottomRight, eset->widgets.selection_rect_color);
		}
	}

	if (pressed || in_focus) {
		std::stringstream ss;
		ss << value;

		TooltipData tip_data;
		tip_data.addText(ss.str());
		Point new_mouse;
		new_mouse.x = pos_knob.x + (pos_knob.w * 2) + local_frame.x - local_offset.x;
		new_mouse.y = pos_knob.y + (pos_knob.h / 2) + local_frame.y - local_offset.y;
		tooltipm->push(tip_data, new_mouse, TooltipData::STYLE_TOPLABEL);
	}
}

bool WidgetSlider::getPrev() {
	if (!enabled) return false;

	value -= (maximum - minimum)/10;
	if (value < minimum)
		value = minimum;

	pos_knob.x = pos.x + ((value-minimum)* pos.w)/(maximum-minimum) - (pos_knob.w/2);
	pos_knob.y = pos.y;

	changed_without_mouse = true;

	return true;
}

bool WidgetSlider::getNext() {
	if (!enabled) return false;

	value += (maximum - minimum)/10;
	if (value > maximum)
		value = maximum;

	pos_knob.x = pos.x + ((value-minimum)* pos.w)/(maximum-minimum) - (pos_knob.w/2);
	pos_knob.y = pos.y;

	changed_without_mouse = true;

	return true;
}


