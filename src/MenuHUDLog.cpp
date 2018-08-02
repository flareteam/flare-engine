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
 * class MenuHUDLog
 */

#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "Menu.h"
#include "MenuHUDLog.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "Settings.h"
#include "Utils.h"
#include "UtilsParsing.h"

MenuHUDLog::MenuHUDLog()
	: overlay_bg(NULL)
	, enable_overlay(true)
	, click_to_dismiss(false)
	, start_at_bottom(true)
	, overlay_at_bottom(true)
	, hide_overlay(false)
{

	// Load config settings
	FileParser infile;
	if(infile.open("menus/hudlog.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "enable_overlay")
				// @ATTR enable_overlay|bool|If true, shows an overlay of the last message on top of other menus.
				enable_overlay = Parse::toBool(infile.val);
			else if (infile.key == "start_at_bottom")
				// @ATTR start_at_bottom|bool|If true, messages start at the bottom and get pushed up. If false, messages start at the top and get pushed down.
				start_at_bottom = Parse::toBool(infile.val);
			else if (infile.key == "overlay_at_bottom")
				// @ATTR overlay_at_bottom|bool|If true, the overlay message will be at the bottom of the HUD log area. If false, it will be at the top.
				overlay_at_bottom = Parse::toBool(infile.val);
			else
				infile.error("MenuHUDLog: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	align();

	font->setFont("font_regular");
	paragraph_spacing = font->getLineHeight()/2;
}

/**
 * Calculate how long a given message should remain on the HUD
 * Formula: minimum time plus x frames per character
 */
int MenuHUDLog::calcDuration(const std::string& s) {
	// 5 seconds plus an extra second per 10 letters
	return settings->max_frames_per_sec * 5 + static_cast<int>(s.length()) * (settings->max_frames_per_sec/10);
}

/**
 * Perform one frame of logic
 * Age messages
 */
void MenuHUDLog::logic() {
	for (unsigned i=0; i<msg_age.size(); i++) {
		if (msg_age[i] > 0)
			msg_age[i]--;
		else
			remove(i);
	}

	// click to dismiss messages when rendered on top of other menus
	if (overlay_bg && click_to_dismiss) {
		if (inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
			Rect overlay_area;
			overlay_area.x = overlay_bg->getDest().x;
			overlay_area.y = overlay_bg->getDest().y;
			overlay_area.w = overlay_bg->getGraphicsWidth();
			overlay_area.h = overlay_bg->getGraphicsHeight();

			if (Utils::isWithinRect(overlay_area, inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;
				hide_overlay = true;
			}
		}
	}
}


/**
 * New messages appear on the screen for a brief time
 */
void MenuHUDLog::render() {
	if (msg_buffer.empty()) {
		return;
	}

	click_to_dismiss = false;
	hide_overlay = true;

	Rect dest;
	dest.x = window_area.x + paragraph_spacing;

	if (start_at_bottom) {
		dest.y = window_area.y+window_area.h;

		// go through new messages
		for (size_t i = msg_age.size(); i > 0; i--) {
			if (msg_age[i-1] > 0 && dest.y > window_area.y && msg_buffer[i-1]) {
				dest.y -= msg_buffer[i-1]->getGraphicsHeight() + paragraph_spacing;
				msg_buffer[i-1]->setDestFromRect(dest);
				render_device->render(msg_buffer[i-1]);
			}
			else return; // no more new messages
		}
	}
	else {
		dest.y = window_area.y + paragraph_spacing;

		// go through new messages
		for (size_t i = msg_age.size(); i > 0; i--) {
			int msg_height = msg_buffer[i-1]->getGraphicsHeight() + paragraph_spacing;
			if (msg_age[i-1] > 0 && dest.y + msg_height < window_area.y + window_area.h && msg_buffer[i-1]) {
				// dest.y -= msg_buffer[i-1]->getGraphicsHeight() + paragraph_spacing;
				msg_buffer[i-1]->setDestFromRect(dest);
				render_device->render(msg_buffer[i-1]);
				dest.y += msg_height;
			}
			else return; // no more new messages
		}
	}

}


/**
 * Displays the last message with a shaded background
 * It is meant to be displayed on top of other menus in place of the normal render output
 */
void MenuHUDLog::renderOverlay() {
	if (msg_buffer.empty() || hide_overlay || !enable_overlay) {
		return;
	}

	click_to_dismiss = true;

	int msg_height = msg_buffer.back()->getGraphicsHeight() + paragraph_spacing*2;
	bool resize_bg = !overlay_bg || overlay_bg->getGraphicsHeight() != msg_height;

	if (resize_bg) {
		if (overlay_bg) {
			delete overlay_bg;
			overlay_bg = NULL;
		}

		Image *temp = render_device->createImage(window_area.w, msg_height);

		if (temp) {
			// fill with translucent black
			Color bg_color;
			bg_color.a = 200;
			temp->fillWithColor(bg_color);

			overlay_bg = temp->createSprite();
			temp->unref();
		}
	}

	int start_y;
	if (overlay_at_bottom)
		start_y = window_area.y + window_area.h - msg_height;
	else
		start_y = window_area.y;

	if (overlay_bg) {
		overlay_bg->setDest(window_area.x, start_y);
		render_device->render(overlay_bg);
	}

	Rect dest;
	dest.x = window_area.x + paragraph_spacing;
	dest.y = start_y + paragraph_spacing;

	msg_buffer.back()->setDestFromRect(dest);
	render_device->render(msg_buffer.back());
}


/**
 * Add a new message to the log
 */
void MenuHUDLog::add(const std::string& s, int type) {
	hide_overlay = false;

	// Make sure we don't spam the same message repeatedly
	if (log_msg.empty() || log_msg.back() != s || type == MSG_UNIQUE) {
		// add new message
		log_msg.push_back(Utils::substituteVarsInString(s, pc));
		msg_age.push_back(calcDuration(log_msg.back()));

		// render the log entry and store it in a buffer
		font->setFont("font_regular");
		Point size = font->calc_size(log_msg.back(), window_area.w - (paragraph_spacing*2));
		Image *graphics = render_device->createImage(size.x, size.y);
		font->renderShadowed(log_msg.back(), 0, 0, FontEngine::JUSTIFY_LEFT, graphics, window_area.w - (paragraph_spacing*2), font->getColor(FontEngine::COLOR_MENU_NORMAL));
		msg_buffer.push_back(graphics->createSprite());
		graphics->unref();
	}
	else if (!msg_age.empty()) {
		msg_age.back() = calcDuration(log_msg.back());
	}

	// force HUD messages to vanish in order
	if (msg_age.size() > 1) {
		const size_t last = msg_age.size()-1;
		if (msg_age[last] < msg_age[last-1])
			msg_age[last] = msg_age[last-1];
	}

}

/**
 * Remove the given message from the list
 */
void MenuHUDLog::remove(int msg_index) {
	if (msg_buffer.at(msg_index))
		delete msg_buffer.at(msg_index);
	msg_buffer.erase(msg_buffer.begin()+msg_index);
	msg_age.erase(msg_age.begin()+msg_index);
	log_msg.erase(log_msg.begin()+msg_index);
}

void MenuHUDLog::clear() {
	for (unsigned i=0; i<msg_buffer.size(); i++) {
		if (msg_buffer[i])
			delete msg_buffer[i];
	}
	msg_buffer.clear();
	msg_age.clear();
	log_msg.clear();
}

MenuHUDLog::~MenuHUDLog() {
	delete overlay_bg;

	for (unsigned i=0; i<msg_buffer.size(); i++) {
		if (msg_buffer[i])
			delete msg_buffer[i];
	}
}
