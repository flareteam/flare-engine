/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Joseph Bleau
Copyright © 2013 Kurt Rinnert
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

#ifndef WIDGET_H
#define WIDGET_H

/**
 * Base interface all widget needs to implement
 */
#include "CommonIncludes.h"
#include "Utils.h"

class Widget {
public:
	enum {
		SCROLL_VERTICAL = 0,
		SCROLL_HORIZONTAL = 1,
		SCROLL_TWO_DIRECTIONS = 2
	};

	Widget();

	virtual ~Widget();
	virtual void render() = 0;
	virtual void activate();
	virtual void deactivate();
	virtual void defocus();
	virtual bool getNext();  // getNext and getPrev should be implemented
	virtual bool getPrev(); // if the widget has items internally that can be iterated
	virtual void setBasePos(int x, int y, int a);
	virtual void setPos(int offset_x, int offset_y);
	bool in_focus;
	bool focusable;
	bool enable_tablist_nav; // when disabled, this widget will be skipped during tablist navigation
	bool tablist_nav_right; // uses the center-right point for tablist nav instead of the center. Primarily for MenuConfig widgets
	uint8_t scroll_type;
	Rect pos; // This is the position of the button within the screen
	Rect local_frame; // Local reference frame is this is a daughter widget
	Point local_offset; // Offset in local frame is this is a daughter widget
	Point pos_base; // the initial x/y position of this widget, often from a config file
	int alignment;
};

class TabList {
private:
	std::vector<Widget*> widgets;
	int current;
	int previous;
	bool locked;
	bool current_is_valid();
	bool previous_is_valid();
	uint8_t scrolltype;
	int MV_LEFT;
	int MV_RIGHT;
	int ACTIVATE;
	TabList *prev_tablist;
	TabList *next_tablist;
public:
	enum {
		WIDGET_SELECT_AUTO = 0,
		WIDGET_SELECT_LEFT = 1,
		WIDGET_SELECT_RIGHT = 2,
		WIDGET_SELECT_UP = 3,
		WIDGET_SELECT_DOWN = 4
	};

	static const bool GET_INNER = true;

	TabList();
	~TabList();

	void lock();
	void unlock();
	void add(Widget* widget);			// Add a widget
	void remove(Widget* widget);		// Remove a widget
	void clear();						// Remove all widgets
	void setCurrent(Widget* widget);
	int getCurrent();
	Widget* getWidgetByIndex(int index);
	unsigned size();
	Widget* getNext(bool inner, uint8_t dir);	// Increment current selected, return widget
	Widget* getPrev(bool inner, uint8_t dir);	// Decrement current selected, return widget
	int getNextRelativeIndex(uint8_t dir);
	void deactivatePrevious();
	void activate();					// Fire off what happens when the user presses 'accept'
	void defocus();						// Call when user clicks outside of a widget, resets current
	void setPrevTabList(TabList *tl);
	void setNextTabList(TabList *tl);
	void setScrollType(uint8_t _scrolltype);
	void setInputs(int _LEFT, int _RIGHT, int _ACTIVATE);

	void logic();

	bool ignore_no_mouse;
};

#endif

