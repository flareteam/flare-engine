/*
Copyright © 2014 Justin Jacobs

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
 * class MenuPromptList
 */

#pragma once
#ifndef MENU_PROMPTLIST_H
#define MENU_PROMPTLIST_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetListBox.h"

class MenuPromptList : public Menu {
protected:
	void alignElements();
	void loadGraphics();

	WidgetButton *button_confirm;
	WidgetButton *button_close;
	WidgetLabel label;
	WidgetListBox *listbox;
	TabList	tablist;

	std::string title_text;
	int index_offset;

	Point close_pos;
	LabelInfo title;
	Point confirm_pos;
	Point listbox_pos;
	int listbox_rows;

public:
	MenuPromptList(const std::string& title_text, int _index_offset = 0);
	~MenuPromptList();

	void addOption(const std::string& option);
	int getSelectedIndex();

	void logic();
	virtual void render();

	bool confirm_clicked;
};

#endif
