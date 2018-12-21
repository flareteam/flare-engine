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
 * class WidgetButton
 */

#include "FontEngine.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "TooltipManager.h"
#include "WidgetButton.h"

const std::string WidgetButton::DEFAULT_FILE = "images/menus/buttons/button_default.png";
const std::string WidgetButton::NO_FILE = "_NO_FILE_";

WidgetButton::WidgetButton(const std::string& _fileName)
	: Widget()
	, fileName(_fileName)
	, buttons()
	, wlabel()
	, activated(false)
	, label("")
	, tooltip("")
	, enabled(true)
	, pressed(false)
	, hover(false)
{
	focusable = true;
	loadArt();

	text_color_normal = font->getColor(FontEngine::COLOR_WIDGET_NORMAL);
	text_color_pressed = text_color_normal;
	text_color_hover = text_color_normal;
	text_color_disabled = font->getColor(FontEngine::COLOR_WIDGET_DISABLED);
}

void WidgetButton::activate() {
	pressed = true;
	activated = true;
}

void WidgetButton::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);
	refresh();
}

void WidgetButton::setLabel(const std::string& s) {
	label = s;
	refresh();

	if (!buttons) {
		pos.w = wlabel.getBounds()->w;
		pos.h = wlabel.getBounds()->h;
		refresh();
		pos = *wlabel.getBounds();
	}
}

void WidgetButton::setTextColor(int state, Color c) {
	if (state == BUTTON_NORMAL)
		text_color_normal = c;
	else if (state == BUTTON_PRESSED)
		text_color_pressed = c;
	else if (state == BUTTON_HOVER)
		text_color_hover = c;
	else if (state == BUTTON_DISABLED)
		text_color_disabled = c;
}

void WidgetButton::loadArt() {
	if (fileName == NO_FILE)
		return;

	// load button images
	Image *graphics = NULL;
	if (fileName != DEFAULT_FILE) {
		graphics = render_device->loadImage(fileName, RenderDevice::ERROR_NORMAL);
	}
	if (!graphics) {
		graphics = render_device->loadImage(DEFAULT_FILE, RenderDevice::ERROR_EXIT);
	}
	if (graphics) {
		buttons = graphics->createSprite();
		pos.w = buttons->getGraphicsWidth();
		pos.h = buttons->getGraphicsHeight()/4; // height of one button
		buttons->setClip(0, 0, pos.w, pos.h);
		graphics->unref();
	};
}

bool WidgetButton::checkClick() {
	return checkClickAt(inpt->mouse.x,inpt->mouse.y);
}

/**
 * Sets and releases the "pressed" visual state of the button
 * If press and release, activate (return true)
 */
bool WidgetButton::checkClickAt(int x, int y) {
	enable_tablist_nav = enabled;

	Point mouse(x,y);

	checkTooltip(mouse);

	// Change the hover state
	hover = Utils::isWithinRect(pos, mouse) && inpt->usingMouse();

	// disabled buttons can't be clicked;
	if (!enabled) return false;

	// main button already in use, new click not allowed
	if (inpt->lock[Input::MAIN1]) return false;
	if (!inpt->usingMouse() && inpt->lock[Input::ACCEPT]) return false;

	// main click released, so the button state goes back to unpressed
	if (pressed && !inpt->lock[Input::MAIN1] && (!inpt->lock[Input::ACCEPT] || inpt->usingMouse()) && (Utils::isWithinRect(pos, mouse) || activated)) {
		activated = false;
		pressed = false;
		return true;
	}

	pressed = false;

	// detect new click
	if (inpt->pressing[Input::MAIN1]) {
		if (Utils::isWithinRect(pos, mouse)) {

			inpt->lock[Input::MAIN1] = true;
			pressed = true;

		}
	}
	return false;

}

void WidgetButton::render() {
	// the "button" surface contains button variations.
	// choose which variation to display.
	int y;
	if (!enabled) {
		y = BUTTON_DISABLED * pos.h;
		wlabel.setColor(text_color_disabled);
	}
	else if (pressed) {
		y = BUTTON_PRESSED * pos.h;
		wlabel.setColor(text_color_pressed);
	}
	else if (hover || in_focus) {
		y = BUTTON_HOVER * pos.h;
		wlabel.setColor(text_color_hover);
	}
	else {
		y = BUTTON_NORMAL * pos.h;
		wlabel.setColor(text_color_normal);
	}

	if (buttons) {
		buttons->local_frame = local_frame;
		buttons->setOffset(local_offset);

		Rect clip = buttons->getClip();
		clip.y = y;

		buttons->setClipFromRect(clip);
		buttons->setDestFromRect(pos);

		render_device->render(buttons);
	}

	// render label
	wlabel.local_frame = local_frame;
	wlabel.local_offset = local_offset;
	wlabel.render();
}

/**
 * Create the text buffer
 */
void WidgetButton::refresh() {
	if (label != "") {

		if (buttons) {
			wlabel.setPos(pos.x + (pos.w/2), pos.y + (pos.h/2));
			wlabel.setJustify(FontEngine::JUSTIFY_CENTER);
			wlabel.setVAlign(LabelInfo::VALIGN_CENTER);
		}
		else {
			wlabel.setPos(pos.x, pos.y);
			wlabel.setJustify(FontEngine::JUSTIFY_LEFT);
			wlabel.setVAlign(LabelInfo::VALIGN_TOP);
		}
		wlabel.setText(label);

		if (enabled)
			wlabel.setColor(font->getColor(FontEngine::COLOR_WIDGET_NORMAL));
		else
			wlabel.setColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED));
	}
}

void WidgetButton::checkTooltip(const Point& mouse) {
	if (inpt->usingMouse() && Utils::isWithinRect(pos, mouse) && tooltip != "") {
		TooltipData tip_data;
		tip_data.addText(tooltip);
		Point new_mouse(mouse.x + local_frame.x - local_offset.x, mouse.y + local_frame.y - local_offset.y);
		tooltipm->push(tip_data, new_mouse, TooltipData::STYLE_FLOAT);
	}
}

WidgetButton::~WidgetButton() {
	delete buttons;
}

