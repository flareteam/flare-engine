/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

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

#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "MenuConfirm.h"
#include "SharedResources.h"
#include "WidgetButton.h"
#include "WidgetHorizontalList.h"

#include <string>

MenuConfirm::MenuConfirm()
	: Menu()
	, button_close(new WidgetButton("images/menus/buttons/button_x.png"))
	, action_list(new WidgetHorizontalList())
	, clicked_confirm(false)
	, clicked_cancel(false)
{
	// Load config settings
	FileParser infile;
	if(infile.open("menus/confirm.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
		}
		infile.close();
	}

	action_list->has_action = true;

	tablist.ignore_no_mouse = true;
	tablist.add(action_list);
	// tablist.add(button_close);

	setBackground("images/menus/confirm_bg.png");
	align();
}

void MenuConfirm::align() {
	Menu::align();

	label.setJustify(FontEngine::JUSTIFY_CENTER);
	label.setText(title);
	label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	action_list->pos.x = window_area.x + window_area.w/2 - action_list->pos.w/2;
	action_list->pos.y = window_area.y + window_area.h/2;
	action_list->refresh();
	label.setPos(window_area.x + window_area.w/2, window_area.y + window_area.h - (action_list->pos.h * 2));

	button_close->pos.x = window_area.x + window_area.w;
	button_close->pos.y = window_area.y;
}

void MenuConfirm::logic() {
	if (visible && action_list->enabled) {
		tablist.logic();

		if (!inpt->usingMouse() && tablist.getCurrent() == -1) {
			tablist.getNext(!TabList::GET_INNER, TabList::WIDGET_SELECT_AUTO);
		}
		else if (inpt->usingMouse()) {
			tablist.defocus();
		}

		clicked_confirm = false;

		if (action_list->checkClick() && action_list->checkAction()) {
			clicked_confirm = true;
		}
		else if (button_close->checkClick() || (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL])) {
			if (inpt->pressing[Input::CANCEL])
				inpt->lock[Input::CANCEL] = true;

			visible = false;
			clicked_confirm = false;
			clicked_cancel = true;
		}
	}
}

void MenuConfirm::render() {
	// background
	Menu::render();

	label.render();

	action_list->render();
	button_close->render();
}

void MenuConfirm::setTitle(const std::string& s) {
	title = s;
	align();
}

void MenuConfirm::show() {
	visible = true;
	clicked_confirm = false;
	clicked_cancel = false;
	action_list->select(0);
	action_list->enabled = true;
	align();
}

MenuConfirm::~MenuConfirm() {
	delete action_list;
	delete button_close;
}

