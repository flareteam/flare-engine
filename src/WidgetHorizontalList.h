/*
Copyright © 2018 Justin Jacobs

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
 * class WidgetHorizontalList
 */

#ifndef WIDGET_HORIZONTALLIST_H
#define WIDGET_HORIZONTALLIST_H

#include "CommonIncludes.h"
#include "Widget.h"
#include "WidgetLabel.h"

class WidgetButton;

class WidgetHorizontalList : public Widget {
private:
	static const std::string DEFAULT_FILE_LEFT;
	static const std::string DEFAULT_FILE_RIGHT;

	class HListItem {
	public:
		HListItem()
			: button(NULL)
		{}
		~HListItem() {}

		std::string value;
		std::string tooltip;
		WidgetButton *button;
	};

	void checkTooltip(const Point& mouse);
	void getVisibleButtonRange(size_t& start, size_t& end);
	bool multipleActionsVisible();

	WidgetLabel label;
	WidgetButton *button_left;
	WidgetButton *button_right;

	unsigned cursor;
	bool changed_without_mouse;
	bool action_triggered;
	bool activated;
	std::vector<HListItem> list_items;
	Rect tooltip_area;
	Point action_button_size;

public:
	explicit WidgetHorizontalList();
	~WidgetHorizontalList();

	void setPos(int offset_x, int offset_y);

	bool checkClick();
	bool checkClickAt(int x, int y);
	bool checkAction();
	void activate();
	void render();
	void refresh();

	void append(const std::string& value, const std::string& tooltip);
	void clear();
	std::string getValue();
	void setValue(unsigned index, const std::string& value);
	unsigned getSelected();
	unsigned getSize();
	bool isEmpty();

	void select(unsigned index);
	void scrollLeft();
	void scrollRight();

	bool getPrev();
	bool getNext();

	bool enabled;
	bool has_action;
	size_t max_visible_actions;
};

#endif
