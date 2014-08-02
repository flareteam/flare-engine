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

#include "FileParser.h"
#include "MenuPromptList.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"

MenuPromptList::MenuPromptList(const std::string& _title_text, int _index_offset)
	: Menu()
	, title_text(_title_text)
	, index_offset(_index_offset)
	, listbox_rows(6)
	, confirm_clicked(false) {

	// Load config settings
	FileParser infile;
	// @CLASS MenuPromptList|Description of menus/prompt_list.txt
	if(infile.open("menus/promptlist.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|x (integer), y (integer)|Position of the close button.
			if(infile.key == "close") close_pos = toPoint(infile.val);
			// @ATTR label_title|label|Position of the menu title.
			else if(infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR confirm|x (integer), y (integer)|Position of the "OK" button
			else if (infile.key == "confirm") confirm_pos = toPoint(infile.val);
			// @ATTR listbox|x (integer), y (integer)|Position of the listbox itself.
			else if (infile.key == "listbox") listbox_pos = toPoint(infile.val);
			// @ATTR listbox_rows|integer|The height (in rows) of the listbox. The default is 6.
			else if (infile.key == "listbox_rows") listbox_rows = toInt(infile.val);
		}
		infile.close();
	}

	listbox = new WidgetListBox(listbox_rows, "images/menus/buttons/listbox_char.png");

	button_confirm = new WidgetButton("images/menus/buttons/button_default.png");
	button_confirm->label = msg->get("OK");

	button_close = new WidgetButton("images/menus/buttons/button_x.png");

	setBackground("images/menus/prompt_list_bg.png");

	tablist.add(listbox);
	tablist.add(button_confirm);
	tablist.add(button_close);

	align();
	alignElements();
}

void MenuPromptList::alignElements() {
	listbox->pos.x = window_area.x + listbox_pos.x;
	listbox->pos.y = window_area.y + listbox_pos.y;

	button_confirm->pos.x = window_area.x + confirm_pos.x;
	button_confirm->pos.y = window_area.y + confirm_pos.y;
	button_confirm->refresh();

	button_close->pos.x = window_area.x + close_pos.x;
	button_close->pos.y = window_area.y + close_pos.y;

	label.set(window_area.x + title.x, window_area.y + title.y, title.justify, title.valign, msg->get(title_text), font->getColor("menu_normal"), title.font_style);
}

void MenuPromptList::addOption(const std::string& option) {
	listbox->append(option, "");
}

int MenuPromptList::getSelectedIndex() {
	return listbox->getSelected();
}

void MenuPromptList::logic() {
	if (visible) {
		if (NO_MOUSE) {
			tablist.logic();
		}

		listbox->checkClick();

		if (button_confirm->checkClick()) {
			confirm_clicked = true;
		}
		if (button_close->checkClick()) {
			visible = false;
		}
	}
}

void MenuPromptList::render() {
	if (visible) {
		// background
		Menu::render();

		label.render();

		listbox->render();
		button_confirm->render();
		button_close->render();
	}
}

MenuPromptList::~MenuPromptList() {
	delete listbox;
	delete button_confirm;
	delete button_close;
}

