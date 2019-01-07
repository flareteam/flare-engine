/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2016 Justin Jacobs
Copyright © 2013 Kurt Rinnert

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
 * class WidgetScrollBox
 */

#ifndef WIDGET_SCROLLBOX_H
#define WIDGET_SCROLLBOX_H

#include "SharedResources.h"
#include "Widget.h"
#include "WidgetScrollBar.h"

class Widget;

class WidgetScrollBox : public Widget {
public:
	WidgetScrollBox (int width, int height);
	~WidgetScrollBox ();
	void setPos(int offset_x, int offset_y);

	void addChildWidget(Widget* child);
	void clearChildWidgets();
	Point input_assist(const Point& mouse);
	void logic();
	void logic(int x, int y);
	void resize(int w, int h);
	void refresh();
	void render();

	Sprite *contents;
	bool update;
	Color bg;

	TabList tablist;
	bool getNext();
	bool getPrev();
	void activate();
	int getCursor() {
		return cursor;
	}
	void scrollToTop();

private:
	void scroll(int amount);
	void scrollTo(int amount);
	void scrollDown();
	void scrollUp();
	std::vector<Widget*> children;
	int currentChild;

	int cursor;
	WidgetScrollBar * scrollbar;
};

#endif

