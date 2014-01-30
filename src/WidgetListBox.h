/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Justin Jacobs
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

#pragma once
#ifndef WIDGET_ListBox_H
#define WIDGET_ListBox_H

#include "CommonIncludes.h"
#include "Widget.h"
#include "TooltipData.h"

class WidgetLabel;
class WidgetScrollBar;
class WidgetTooltip;

class WidgetListBox : public Widget {
private:

	std::string fileName; // the path to the ListBoxs background image

	Sprite *listboxs;
	Mix_Chunk *click;

	int list_amount;
	int list_height;
	int cursor;
	bool has_scroll_bar;
	int non_empty_slots;
	bool any_selected;
	std::string *values;
	std::string *tooltips;
	WidgetLabel *vlabels;
	Rect *rows;
	WidgetTooltip *tip;
	WidgetScrollBar *scrollbar;
	Color color_normal;
	Color color_disabled;

public:
	WidgetListBox(int amount, int height, const std::string& _fileName);
	~WidgetListBox();

	bool checkClick();
	bool checkClick(int x, int y);
	TooltipData checkTooltip(Point mouse);
	void append(std::string value, std::string tooltip);
	void set(int index, std::string value, std::string tooltip);
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
	void refresh();

	bool getNext();
	bool getPrev();

	Rect pos_scroll;
	bool pressed;
	bool *selected;
	bool multi_select;
	bool can_deselect;
	bool can_select;
	int scrollbar_offset;
};

#endif
