/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2014 Henrik Andersson

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
 * class MenuLog
 */

#include "FileParser.h"
#include "Menu.h"
#include "MenuLog.h"
#include "ModManager.h"
#include "Settings.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetLog.h"
#include "WidgetTabControl.h"

MenuLog::MenuLog() {
	visible = false;

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Load config settings
	FileParser infile;
	// @CLASS MenuLog|Description of menus/log.txt
	if(infile.open("menus/log.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR label_title|label|Position of the "Log" text.
			if(infile.key == "label_title") {
				title = eatLabelInfo(infile.val);
			}
			// @ATTR close|x (integer), y (integer)|Position of the close button.
			else if(infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR tab_area|x (integer), y (integer), w (integer), h (integer)|The position of the row of tabs, followed by the dimensions of the log text area.
			else if(infile.key == "tab_area") {
				tab_area = toRect(infile.val);
			}
			else {
				infile.error("MenuLog: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// Initialize the tab control.
	tabControl = new WidgetTabControl();

	// Store the amount of displayed log messages on each log, and the maximum.
	for (unsigned i=0; i<LOG_TYPE_COUNT; i++) {
		log[i] = new WidgetLog(tab_area.w,tab_area.h);
		log[i]->setBasePos(tab_area.x, tab_area.y + tabControl->getTabHeight());
		tablist.add(log[i]->getWidget());
	}

	// Define the header.
	tabControl->setTabTitle(LOG_TYPE_MESSAGES, msg->get("Notes"));
	tabControl->setTabTitle(LOG_TYPE_QUESTS, msg->get("Quests"));

	setBackground("images/menus/log.png");

	align();
}

void MenuLog::align() {
	Menu::align();

	tabControl->setMainArea(window_area.x + tab_area.x, window_area.y + tab_area.y, tab_area.w, tab_area.h);
	tabControl->updateHeader();

	closeButton->setPos(window_area.x, window_area.y);

	label_log.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Log"), font->getColor("menu_normal"), title.font_style);

	for (unsigned i=0; i<LOG_TYPE_COUNT; i++) {
		log[i]->setPos(window_area.x, window_area.y);
	}
}

/**
 * Perform one frame of logic.
 */
void MenuLog::logic() {
	if(!visible) return;

	tablist.logic();

	// make shure keyboard navigation leads us to correct tab
	for (unsigned i = 0; i < LOG_TYPE_COUNT; i++) {
		if (log[i]->inFocus()) {
			tabControl->setActiveTab(i);
			break;
		}
	}

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}

	tabControl->logic();
	int active_log = tabControl->getActiveTab();
	log[active_log]->logic();
}

/**
 * Render graphics for this frame when the menu is open
 */
void MenuLog::render() {
	if (!visible) return;

	// Background.
	Menu::render();

	// Close button.
	closeButton->render();

	// Text overlay.
	if (!title.hidden) label_log.render();

	// Tab control.
	tabControl->render();

	// Display latest log messages for the active tab.
	int active_log = tabControl->getActiveTab();
	log[active_log]->render();
}

/**
 * Add a new message to the log.
 */
void MenuLog::add(const std::string& s, int log_type, bool prevent_spam) {
	log[log_type]->add(s, prevent_spam);
}

/**
 * Remove log message with the given identifier.
 */
void MenuLog::remove(int msg_index, int log_type) {
	log[log_type]->remove(msg_index);
}

void MenuLog::clear(int log_type) {
	log[log_type]->clear();
}

void MenuLog::clear() {
	for (unsigned i=0; i<LOG_TYPE_COUNT; i++) {
		log[i]->clear();
	}
}

MenuLog::~MenuLog() {
	for (unsigned i=0; i<LOG_TYPE_COUNT; i++) {
		delete log[i];
	}

	delete closeButton;
	delete tabControl;
}
