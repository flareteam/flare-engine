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
#include "EngineSettings.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "TooltipManager.h"
#include "Widget.h"
#include "WidgetCheckBox.h"

const std::string WidgetCheckBox::DEFAULT_FILE = "images/menus/buttons/checkbox_default.png";

WidgetCheckBox::WidgetCheckBox (const std::string &fname)
	: enabled(true)
	, tooltip("")
	, cb(NULL)
	, checked(false)
	, pressed(false)
	, activated(false)
{
	focusable = true;

	Image *graphics = NULL;
	if (fname != DEFAULT_FILE) {
		graphics = render_device->loadImage(fname, RenderDevice::ERROR_NORMAL);
	}
	if (!graphics) {
		graphics = render_device->loadImage(DEFAULT_FILE, RenderDevice::ERROR_EXIT);
	}
	if (graphics) {
		cb = graphics->createSprite();
		pos.w = cb->getGraphicsWidth();
		pos.h = cb->getGraphicsHeight() / 2;
		cb->setClip(0, 0, pos.w, pos.h);
		graphics->unref();
	}
}

void WidgetCheckBox::activate() {
	pressed = true;
	activated = true;
}

WidgetCheckBox::~WidgetCheckBox () {
	delete cb;
}

void WidgetCheckBox::setChecked(const bool status) {
	checked = status;
	if (cb)	{
		cb->setClip(0, (checked ? pos.h : 0), pos.w, pos.h);
	}
}

bool WidgetCheckBox::checkClick() {
	return checkClickAt(inpt->mouse.x,inpt->mouse.y);
}

bool WidgetCheckBox::checkClickAt(int x, int y) {
	enable_tablist_nav = enabled;

	if (!enabled) return false;

	Point mouse(x,y);

	checkTooltip(mouse);

	// main button already in use, new click not allowed
	if (inpt->lock[Input::MAIN1]) return false;
	if (!inpt->usingMouse() && inpt->lock[Input::ACCEPT]) return false;

	if (pressed && !inpt->lock[Input::MAIN1] && (!inpt->lock[Input::ACCEPT] || inpt->usingMouse()) && (Utils::isWithinRect(pos, mouse) || activated)) { // this is a button release
		activated = false;
		pressed = false;
		setChecked(!checked);
		return true;
	}

	pressed = false;

	if (inpt->pressing[Input::MAIN1]) {
		if (Utils::isWithinRect(pos, mouse)) {
			pressed = true;
			inpt->lock[Input::MAIN1] = true;
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
		cb->setDestFromRect(pos);
		render_device->render(cb);
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
}

void WidgetCheckBox::checkTooltip(const Point& mouse) {
	TooltipData tip_data;
	if (inpt->usingMouse() && Utils::isWithinRect(pos, mouse) && tooltip != "") {
		tip_data.addText(tooltip);
	}

	if (!tip_data.isEmpty()) {
		Point new_mouse(mouse.x + local_frame.x - local_offset.x, mouse.y + local_frame.y - local_offset.y);
		tooltipm->push(tip_data, new_mouse, TooltipData::STYLE_FLOAT);
	}
}

