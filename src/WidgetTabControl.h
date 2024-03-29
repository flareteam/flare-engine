/*
Copyright © 2011-2012 Clint Bellanger
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

#ifndef MENU_TAB_CONTROL_H
#define MENU_TAB_CONTROL_H

#include "CommonIncludes.h"

class WidgetButton;
class WidgetLabel;

class WidgetTabControl : public Widget {
private:

	void loadGraphics();
	void renderTab(unsigned number);

	unsigned getNextEnabledTab(unsigned tab);
	unsigned getPrevEnabledTab(unsigned tab);

	Sprite *active_tab_surface;
	Sprite *inactive_tab_surface;

	std::vector<std::string> titles; // Titles of the tabs.
	std::vector<Rect> tabs; // Rectangles for each tab title on the tab header.
	std::vector<WidgetLabel> active_labels;
	std::vector<WidgetLabel> inactive_labels;
	std::vector<bool> enabled;
	std::vector<TabList*> tablists;

	unsigned active_tab;    // Index of the currently active tab.
	Rect tabs_area;    // Area the tab titles are displayed.
	bool lock_main1;
	bool dragging;

	WidgetButton *button_prev;
	WidgetButton *button_next;

	bool show_buttons;

public:

	WidgetTabControl();
	~WidgetTabControl();

	void setupTab(unsigned index, const std::string& title, TabList* tl);
	void setMainArea(int x, int y, int w);

	int getActiveTab();
	void setActiveTab(unsigned tab);

	int getTabHeight();

	void setEnabled(unsigned index, bool val);

	void logic();
	void logic(int x, int y);
	void render();

	bool getNext();
	bool getPrev();
};

#endif
