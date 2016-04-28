/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

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
 * class WidgetCheckBox
 */

#include "CommonIncludes.h"
#include "SharedResources.h"
#include "WidgetCheckBox.h"
#include "Widget.h"

WidgetCheckBox::WidgetCheckBox (const std::string &fname)
	: enabled(true)
	, cb(NULL)
	, checked(false)
	, pressed(false) {
	focusable = true;

	Image *graphics;
	graphics = render_device->loadImage(fname, "Couldn't load image", true);
	if (graphics) {
		cb = graphics->createSprite();
		pos.w = cb->getGraphicsWidth();
		pos.h = cb->getGraphicsHeight() / 2;
		cb->setClip(0, 0, pos.w, pos.h);
		graphics->unref();
	}
	render_to_alpha = false;
}

void WidgetCheckBox::activate() {
	pressed = true;
}

WidgetCheckBox::~WidgetCheckBox () {
	if (cb) delete cb;
}

void WidgetCheckBox::Check () {
	checked = true;
	if (cb) cb->setClip(0,pos.h,pos.w,pos.h);
}

void WidgetCheckBox::unCheck () {
	checked = false;
	if (cb)	cb->setClip(0,0,pos.w,pos.h);
}

void WidgetCheckBox::toggleCheck () {
	checked = !checked;
	if (cb)	cb->setClip(0,(checked ? pos.h : 0), pos.w, pos.h);
}

bool WidgetCheckBox::checkClick() {
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}

bool WidgetCheckBox::checkClick (int x, int y) {
	if (!enabled) return false;

	Point mouse(x,y);

	// main button already in use, new click not allowed
	if (inpt->lock[MAIN1]) return false;
	if (inpt->lock[ACCEPT]) return false;

	if (pressed && !inpt->lock[MAIN1] && !inpt->lock[ACCEPT]) { // this is a button release
		pressed = false;
		toggleCheck();
		return true;
	}

	if (inpt->pressing[MAIN1]) {
		if (isWithin(pos, mouse)) {
			pressed = true;
			inpt->lock[MAIN1] = true;
		}
	}
	return false;
}


bool WidgetCheckBox::isChecked () const {
	return checked;
}

void WidgetCheckBox::render() {
	if (cb) {
		cb->local_frame = local_frame;
		cb->setOffset(local_offset);
		cb->setDest(pos);
		render_device->render(cb);
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

