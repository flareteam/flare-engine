/*
Copyright © 2011-2012 Clint Bellanger

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

#pragma once
#ifndef MENU_LOG_H
#define MENU_LOG_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "WidgetLabel.h"

class WidgetButton;
class WidgetScrollBox;
class WidgetTabControl;

const unsigned int MAX_LOG_MESSAGES = 32;

const unsigned LOG_TYPE_COUNT = 2;
const int LOG_TYPE_QUESTS = 0;
const int LOG_TYPE_MESSAGES = 1;

class MenuLog : public Menu {
private:

	WidgetButton *closeButton;
	WidgetTabControl *tabControl;

	void loadGraphics();

	std::vector<std::string> log_msg[LOG_TYPE_COUNT];
	WidgetScrollBox *msg_buffer[LOG_TYPE_COUNT];
	int log_count[LOG_TYPE_COUNT];
	std::string tab_labels[LOG_TYPE_COUNT];
	SDL_Rect tab_rect[LOG_TYPE_COUNT];
	int paragraph_spacing;

	LabelInfo title;
	Point close_pos;
	int tab_content_indent;
	SDL_Rect tab_area;
	SDL_Color tab_bg;
	SDL_Color color_normal;

public:
	MenuLog();
	~MenuLog();

	void update();
	void logic();
	void render();
	void refresh(int log_type);
	void add(const std::string& s, int log_type);
	void remove(int msg_index, int log_type);
	void clear(int log_type);
	void clear();

	TabList tablist;
};

#endif
