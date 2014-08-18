/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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
 * class MenuHUDLog
 */

#include "FileParser.h"
#include "Menu.h"
#include "MenuHUDLog.h"
#include "SharedResources.h"
#include "Settings.h"

using namespace std;

MenuHUDLog::MenuHUDLog() {

	// Load config settings
	FileParser infile;
	if(infile.open("menus/hudlog.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else
				infile.error("MenuHUDLog: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	align();

	font->setFont("font_regular");
	paragraph_spacing = font->getLineHeight()/2;

	color_normal = font->getColor("menu_normal");
}

/**
 * Calculate how long a given message should remain on the HUD
 * Formula: minimum time plus x frames per character
 */
int MenuHUDLog::calcDuration(const string& s) {
	// 5 seconds plus an extra second per 10 letters
	return MAX_FRAMES_PER_SEC * 5 + s.length() * (MAX_FRAMES_PER_SEC/10);
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
}


/**
 * New messages appear on the screen for a brief time
 */
void MenuHUDLog::render() {

	Rect dest;
	dest.x = window_area.x;
	dest.y = window_area.y+window_area.h;

	// go through new messages
	for (int i = msg_age.size() - 1; i >= 0; i--) {
		if (msg_age[i] > 0 && dest.y > 64 && msg_buffer[i]) {
			dest.y -= msg_buffer[i]->getGraphicsHeight() + paragraph_spacing;
			msg_buffer[i]->setDest(dest);
			render_device->render(msg_buffer[i]);
		}
		else return; // no more new messages
	}
}


/**
 * Add a new message to the log
 */
void MenuHUDLog::add(const string& s, bool prevent_spam) {
	// Make sure we don't spam the same message repeatedly
	if (log_msg.empty() || log_msg.back() != s || !prevent_spam) {
		// add new message
		log_msg.push_back(s);
		msg_age.push_back(calcDuration(s));

		// render the log entry and store it in a buffer
		font->setFont("font_regular");
		Point size = font->calc_size(s, window_area.w);
		Image *graphics = render_device->createImage(size.x, size.y);
		font->renderShadowed(s, 0, 0, JUSTIFY_LEFT, graphics, window_area.w, color_normal);
		msg_buffer.push_back(graphics->createSprite());
	}
	else if (!msg_age.empty()) {
		msg_age.back() = calcDuration(s);
	}

	// force HUD messages to vanish in order
	if (msg_age.size() > 1) {
		const int last = msg_age.size()-1;
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
	for (unsigned i=0; i<msg_buffer.size(); i++) {
		if (msg_buffer[i])
			delete msg_buffer[i];
	}
}
