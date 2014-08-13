/*
Copyright © 2014 Justin Jacobs

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
 * class WidgetLog
 */

#include "CommonIncludes.h"
#include "SharedResources.h"
#include "Widget.h"
#include "WidgetLog.h"
#include "WidgetScrollBox.h"

using namespace std;

WidgetLog::WidgetLog (int width, int height)
	: scroll_box(new WidgetScrollBox(width, height))
	, padding(4)
	, max_messages(50)
{
	font->setFont("font_regular");
	line_height = font->getLineHeight();
	paragraph_spacing = line_height/2;
	color_normal = font->getColor("menu_normal");
}

WidgetLog::~WidgetLog () {
	delete scroll_box;
}

void WidgetLog::logic() {
	scroll_box->logic();
}

void WidgetLog::render() {
	scroll_box->render();
}

void WidgetLog::setPosition(int x, int y) {
	scroll_box->pos.x = x;
	scroll_box->pos.y = y;
}

bool WidgetLog::inFocus() {
	return scroll_box->in_focus;
}

void WidgetLog::refresh() {
	int y,y2;
	y = y2 = padding;

	int content_width = scroll_box->pos.w-(padding*2);
	font->setFont("font_regular");

	// Resize the scrollbox content area first
	for (unsigned int i=0; i<messages.size(); i++) {
		Point size = font->calc_size(messages[i], content_width);
		y += size.y+paragraph_spacing;
	}
	y+=padding;
	scroll_box->resize(y);

	// Render messages into the scrollbox area
	for (unsigned int i = messages.size(); i > 0; i--) {
		Point size = font->calc_size(messages[i-1], content_width);
		font->renderShadowed(messages[i-1], padding, y2, JUSTIFY_LEFT, scroll_box->contents->getGraphics(), content_width, colors[i-1]);
		y2 += size.y+paragraph_spacing;
	}
}

void WidgetLog::add(const std::string &s, bool prevent_spam, Color* color) {
	// First, make sure we're not repeating the last log message, to avoid spam
	if (messages.empty() || messages.back() != s || !prevent_spam) {
		// If we have too many messages, remove the oldest ones
		while (messages.size() >= max_messages) {
			messages.erase(messages.begin());
			colors.erase(colors.begin());
		}

		// Add the new message.
		messages.push_back(s);
		if (color == NULL) {
			colors.push_back(color_normal);
		}
		else {
			colors.push_back(*color);
		}
		refresh();
	}
}

void WidgetLog::remove(unsigned msg_index) {
	if (msg_index < messages.size()) {
		messages.erase(messages.begin()+msg_index);
		colors.erase(colors.begin()+msg_index);
		refresh();
	}
}

void WidgetLog::clear() {
	messages.clear();
	colors.clear();
	refresh();
}
