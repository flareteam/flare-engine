/*
Copyright © 2014-2016 Justin Jacobs

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
#include "FontEngine.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "Widget.h"
#include "WidgetLog.h"
#include "WidgetScrollBox.h"

WidgetLog::WidgetLog (int width, int height)
	: scroll_box(new WidgetScrollBox(width, height))
	, padding(4)
	, max_messages(MAX_MESSAGES)
	, updated(false)
	, next_color(font->getColor(FontEngine::COLOR_MENU_NORMAL))
	, next_style(FONT_REGULAR)
{
	setFont(FONT_REGULAR);
}

WidgetLog::~WidgetLog () {
	delete scroll_box;
	clear();
}

void WidgetLog::setBasePos(int x, int y, int a) {
	Widget::setBasePos(x, y, a);
	scroll_box->setBasePos(x, y, a);
}

void WidgetLog::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);
	scroll_box->setPos(offset_x, offset_y);
}

void WidgetLog::setFont(int style) {
	if (style == FONT_BOLD) {
		font->setFont("font_bold");
	}
	else {
		font->setFont("font_regular");
	}
	line_height = font->getLineHeight();
	paragraph_spacing = line_height/2;
}

void WidgetLog::logic() {
	scroll_box->logic();
}

void WidgetLog::render() {
	if (updated) {
		refresh();
		updated = false;
	}
	scroll_box->render();
}

void WidgetLog::refresh() {
	int y,y2;
	y = y2 = padding;

	int content_width = scroll_box->pos.w-(padding*2);

	// Resize the scrollbox content area first
	for (size_t i=0; i<messages.size(); i++) {
		setFont(styles[i]);
		Point size = font->calc_size(messages[i], content_width);
		y += size.y+paragraph_spacing;

		if (separators[i])
			y += paragraph_spacing+1;
	}
	y+=(padding*2);
	scroll_box->resize(scroll_box->pos.w, y);

	// Render messages into the scrollbox area
	for (size_t i = messages.size(); i > 0; i--) {
		setFont(styles[i-1]);
		Point size = font->calc_size(messages[i-1], content_width);
		Image* render_target = scroll_box->contents->getGraphics();

		if (!separators.empty() && separators[i-1]) {
			render_target->drawLine(padding, y2, padding + content_width - 1, y2, font->getColor(FontEngine::COLOR_WIDGET_DISABLED));
			y2 += paragraph_spacing;
		}
		font->renderShadowed(messages[i-1], padding, y2, FontEngine::JUSTIFY_LEFT, render_target, content_width, colors[i-1]);
		y2 += size.y+paragraph_spacing;

	}
}

void WidgetLog::add(const std::string &s, int type) {
	// First, make sure we're not repeating the last log message, to avoid spam
	if (messages.empty() || messages.back() != s || type == MSG_UNIQUE) {
		// If we have too many messages, remove the oldest ones
		while (messages.size() >= max_messages) {
			this->remove(0);
		}

		// Add the new message.
		messages.push_back(s);
		colors.push_back(next_color);
		styles.push_back(next_style);
		separators.resize(messages.size(), false);
		updated = true;

		next_color = font->getColor(FontEngine::COLOR_MENU_NORMAL);
		next_style = FONT_REGULAR;
	}
}

void WidgetLog::setNextColor(const Color& color) {
	next_color = color;
}

void WidgetLog::setNextStyle(int style) {
	next_style = style;
}

void WidgetLog::remove(unsigned msg_index) {
	if (msg_index < messages.size()) {
		messages.erase(messages.begin()+msg_index);
		colors.erase(colors.begin()+msg_index);
		styles.erase(styles.begin()+msg_index);
		separators.erase(separators.begin()+msg_index);
		updated = true;
	}
}

void WidgetLog::clear() {
	messages.clear();
	colors.clear();
	styles.clear();
	separators.clear();
	updated = true;

	next_color = font->getColor(FontEngine::COLOR_MENU_NORMAL);
	next_style = FONT_REGULAR;
}

void WidgetLog::setMaxMessages(unsigned count) {
	if (count > MAX_MESSAGES)
		max_messages = count;
	else
		max_messages = MAX_MESSAGES;
}

void WidgetLog::addSeparator() {
	if (messages.empty()) return;

	separators.back() = true;
	updated = true;
}

bool WidgetLog::isEmpty() {
	return messages.empty();
}
