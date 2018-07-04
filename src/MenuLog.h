/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MenuLog
 */

#ifndef MENU_LOG_H
#define MENU_LOG_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "WidgetLabel.h"
#include "WidgetLog.h"

class WidgetButton;
class WidgetTabControl;

class MenuLog : public Menu {
public:
	enum {
		TYPE_QUESTS = 0,
		TYPE_MESSAGES = 1
	};
	static const int TYPE_COUNT = 2;

	MenuLog();
	~MenuLog();
	void align();

	void logic();
	void render();
	void add(const std::string& s, int log_type, int msg_type);
	void setNextColor(const Color& color, int log_type);
	void setNextStyle(int style, int log_type);
	void remove(int msg_index, int log_type);
	void clear(int log_type);
	void clearAll();
	void addSeparator(int log_type);
	void setNextTabList(TabList *tl);

	std::vector<TabList> tablist_log;

	TabList* getCurrentTabList();
	void defocusTabLists();

private:
	WidgetLabel label_log;
	WidgetButton *closeButton;
	WidgetTabControl *tabControl;

	void loadGraphics();

	WidgetLog *log[TYPE_COUNT];
	std::string tab_labels[TYPE_COUNT];
	Rect tab_rect[TYPE_COUNT];

	Rect tab_area;
	Color tab_bg;
};

#endif
