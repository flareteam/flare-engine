/*
Copyright © 2011-2012 kitano
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

#include "EngineSettings.h"
#include "FontEngine.h"
#include "InputState.h"
#include "Platform.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "WidgetInput.h"

WidgetInput::WidgetInput(const std::string& filename)
	: background(NULL)
	, enabled(true)
	, pressed(false)
	, hover(false)
	, cursor_pos(0)
	, edit_mode(false)
	, max_length(0)
	, only_numbers(false)
	, accept_to_defocus(true)
{

	loadGraphics(filename);

	render_to_alpha = false;
	color_normal = font->getColor("widget_normal");
}

void WidgetInput::setPos(int offset_x, int offset_y) {
	setPosition(pos_base.x+offset_x, pos_base.y+offset_y);
}

void WidgetInput::loadGraphics(const std::string& filename) {
	// load input background image
	Image *graphics;
	graphics = render_device->loadImage(filename, RenderDevice::ERROR_EXIT);
	if (graphics) {
		background = graphics->createSprite();
		pos.w = background->getGraphicsWidth();
		pos.h = background->getGraphicsHeight()/2;
		graphics->unref();
	}
}

void WidgetInput::trimText() {
	std::string text_with_cursor = text;
	text_with_cursor.insert(cursor_pos, "|");

	int padding = font->getFontHeight();
	trimmed_text = font->trimTextToWidth(text, pos.w-padding, false, text.length());

	size_t trim_pos = (cursor_pos > 0 ? cursor_pos - 1 : cursor_pos);
	trimmed_text_cursor = font->trimTextToWidth(text_with_cursor, pos.w-padding, false, trim_pos);
}

void WidgetInput::activate() {
	if (!edit_mode)
		edit_mode = true;
}

void WidgetInput::logic() {
	if (logic(inpt->mouse.x,inpt->mouse.y))
		return;
}

bool WidgetInput::logic(int x, int y) {
	Point mouse(x, y);

	// Change the hover state
	hover = isWithinRect(pos, mouse);

	if (checkClick()) {
		edit_mode = true;
	}

	// if clicking elsewhere unfocus the text box
	if (inpt->pressing[Input::MAIN1]) {
		if (!isWithinRect(pos, inpt->mouse)) {
			edit_mode = false;
		}
	}

	if (edit_mode) {
		inpt->slow_repeat[Input::DEL] = true;
		inpt->slow_repeat[Input::LEFT] = true;
		inpt->slow_repeat[Input::RIGHT] = true;
		inpt->startTextInput();

		if (inpt->inkeys != "") {
			// handle text input
			// only_numbers will restrict our input to 0-9 characters
			if (!only_numbers || (inpt->inkeys[0] >= 48 && inpt->inkeys[0] <= 57)) {
				text.insert(cursor_pos, inpt->inkeys);
				cursor_pos += inpt->inkeys.length();
				trimText();
			}

			// HACK: this prevents normal keys from triggering common menu shortcuts
			for (size_t i = 0; i < inpt->KEY_COUNT; ++i) {
				if (inpt->pressing[i]) {
					inpt->lock[i] = true;
					inpt->repeat_ticks[i] = 1;
				}
			}
		}

		// handle backspaces
		if (inpt->pressing[Input::DEL] && inpt->repeat_ticks[Input::DEL] == 0) {
			if (!text.empty() && cursor_pos > 0) {
				// remove utf-8 character
				// size_t old_cursor_pos = cursor_pos;
				size_t n = cursor_pos-1;
				while (n > 0 && ((text[n] & 0xc0) == 0x80)) {
					n--;
				}
				text = text.substr(0, n) + text.substr(cursor_pos, text.length());
				cursor_pos -= (cursor_pos) - n;
				trimText();
			}
		}

		// cursor movement
		if (!text.empty() && cursor_pos > 0 && inpt->pressing[Input::LEFT] && inpt->repeat_ticks[Input::LEFT] == 0) {
			cursor_pos--;
			trimText();
		}
		else if (!text.empty() && cursor_pos < text.length() && inpt->pressing[Input::RIGHT] && inpt->repeat_ticks[Input::RIGHT] == 0) {
			inpt->lock[Input::RIGHT] = true;
			cursor_pos++;
			trimText();
		}

		// defocus with Enter or Escape
		if (accept_to_defocus && inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
			inpt->lock[Input::ACCEPT] = true;
			edit_mode = false;
		}
		else if (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) {
			inpt->lock[Input::CANCEL] = true;
			edit_mode = false;
		}
	}
	else {
		inpt->slow_repeat[Input::DEL] = false;
		inpt->slow_repeat[Input::LEFT] = false;
		inpt->slow_repeat[Input::RIGHT] = false;
		inpt->stopTextInput();
	}

	return true;
}

void WidgetInput::render() {
	Rect src;
	src.x = 0;
	src.y = (edit_mode ? pos.h : 0);
	src.w = pos.w;
	src.h = pos.h;

	if (background) {
		background->local_frame = local_frame;
		background->setOffset(local_offset);
		background->setClip(src);
		background->setDest(pos);
		render_device->render(background);
	}

	font->setFont("font_regular");

	if (!edit_mode) {
		font->render(trimmed_text, font_pos.x, font_pos.y, FontEngine::JUSTIFY_LEFT, NULL, 0, color_normal);
	}
	else {
		font->renderShadowed(trimmed_text_cursor, font_pos.x, font_pos.y, FontEngine::JUSTIFY_LEFT, NULL, 0, color_normal);
	}

	if (in_focus && !edit_mode) {
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

	// handle on-screen keyboard
	if (PLATFORM.is_mobile_device && edit_mode) {
		osk_buf.clear();
		osk_buf.addText(trimmed_text_cursor);
		osk_tip.render(osk_buf, Point(settings->view_w_half + pos.w/2, 0), TooltipData::STYLE_FLOAT);
	}
}

void WidgetInput::setPosition(int x, int y) {
	pos.x = x + local_frame.x - local_offset.x;
	pos.y = y + local_frame.y - local_offset.y;

	font->setFont("font_regular");
	font_pos.x = pos.x  + (font->getFontHeight()/2);
	font_pos.y = pos.y + (pos.h/2) - (font->getFontHeight()/2);
}

bool WidgetInput::checkClick() {

	// disabled buttons can't be clicked;
	if (!enabled) return false;

	// main button already in use, new click not allowed
	if (inpt->lock[Input::MAIN1]) return false;

	// main click released, so the button state goes back to unpressed
	if (pressed && !inpt->lock[Input::MAIN1]) {
		pressed = false;

		if (isWithinRect(pos, inpt->mouse)) {

			// activate upon release
			return true;
		}
	}

	pressed = false;

	// detect new click
	if (inpt->pressing[Input::MAIN1]) {
		if (isWithinRect(pos, inpt->mouse)) {

			inpt->lock[Input::MAIN1] = true;
			pressed = true;

		}
	}
	return false;
}

WidgetInput::~WidgetInput() {
	if (background) delete background;
}

