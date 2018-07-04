/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2016 Justin Jacobs
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
 * class WidgetListBox
 */

#ifndef WIDGET_ListBox_H
#define WIDGET_ListBox_H

#include "CommonIncludes.h"
#include "Widget.h"
#include "TooltipData.h"

class WidgetLabel;
class WidgetScrollBar;

class ListBoxItem {
public:
	ListBoxItem()
		: selected(false)
	{}
	~ListBoxItem() {}
	bool operator< (const ListBoxItem& other) const {
		return value < other.value;
	}
	std::string value;
	std::string tooltip;
	bool selected;
};

class WidgetListBox : public Widget {
private:

	std::string fileName; // the path to the ListBoxs background image

	Sprite *listboxs;

	int cursor;
	bool has_scroll_bar;
	bool any_selected;
	std::vector<ListBoxItem> items;
	std::vector<WidgetLabel> vlabels;
	std::vector<Rect> rows;
	WidgetScrollBar *scrollbar;

public:
	WidgetListBox(int height, const std::string& _fileName = "images/menus/buttons/listbox_default.png");
	~WidgetListBox();
	void setPos(int offset_x = 0, int offset_y = 0);

	static const bool GOTO_SELECTED = true;

	bool checkClick();
	bool checkClick(int x, int y);
	TooltipData checkTooltip(const Point& mouse);
	void append(const std::string& value, const std::string& tooltip);
	void set(unsigned index, const std::string& value, const std::string& tooltip);
	void remove(int index);
	void clear();
	void shiftUp();
	void shiftDown();
	int getSelected();
	std::string getValue();
	std::string getValue(int index);
	std::string getTooltip(int index);
	int getSize();
	void scrollUp();
	void scrollDown();
	void render();
	void refresh(bool go_to_selected = false);
	void sort();

	bool getNext();
	bool getPrev();
	void defocus();

	void select(int index);
	void deselect(int index);
	bool isSelected(int index);

	void setHeight(int new_size);

	Rect pos_scroll;
	bool pressed;
	bool multi_select;
	bool can_deselect;
	bool can_select;
	int scrollbar_offset;
	bool disable_text_trim;
};

#endif
