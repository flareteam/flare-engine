/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Joseph Bleau

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


#pragma once
#ifndef WIDGET_H
#define WIDGET_H

/**
 * Base interface all widget needs to implement
 */
#include <SDL.h>
#include <vector>

enum ScrollType {VERTICAL, HORIZONTAL, TWO_DIRECTIONS};

class Widget {
public:
	Widget();

	virtual ~Widget();
	virtual void render(SDL_Surface *target = NULL) = 0;
	virtual void activate();
	virtual bool getNext();  // getNext and getPrev should be implemented
	virtual bool getPrev(); // if the widget has items internally that can be iterated
	bool render_to_alpha;
	bool in_focus;
	bool focusable;
	SDL_Rect pos; // This is the position of the button within the screen
private:
};


class TabList {
private:
	std::vector<Widget*> widgets;
	int current;
	ScrollType scrolltype;
public:
	TabList();
	TabList(ScrollType scrolltype);
	~TabList();

	void add(Widget* widget);			// Add a widget
	void remove(Widget* widget);		// Remove a widget
	void clear();						// Remove all widgets
	Widget* getNext(bool inner = true);	// Increment current selected, return widget
	Widget* getPrev(bool inner = true);	// Decrement current selected, return widget
	void activate();					// Fire off what happens when the user presses 'accept'
	void defocus();						// Call when user clicks outside of a widget, resets current

	void logic();
};

#endif

