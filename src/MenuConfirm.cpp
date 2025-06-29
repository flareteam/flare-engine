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
#include "UtilsParsing.h"
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
	label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	action_list->has_action = true;
	action_list->refresh();

	// Load config settings
	FileParser infile;
	// @CLASS MenuConfirm|Description of menus/confirm.txt
	if(infile.open("menus/confirm.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val)) {
				// default positions based on window_area
				if (infile.key == "pos") {
					int border_size = 6;

					button_close->setBasePos(window_area.w, 0, Utils::ALIGN_TOPLEFT);
					action_list->setBasePos(window_area.w/2 - action_list->pos.w/2, window_area.h - action_list->pos.h - border_size, Utils::ALIGN_TOPLEFT);

					LabelInfo label_info;
					label_info.x = window_area.w/2;
					label_info.y = border_size;
					label_info.justify = FontEngine::JUSTIFY_CENTER;
					label.setFromLabelInfo(label_info);
				}
				continue;
			}

			if (infile.key == "close") {
				// @ATTR close|point|Position of the close button.
				Point pos = Parse::toPoint(infile.val);
				button_close->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "label_title") {
				// @ATTR label_title|label|Position of the title text.
				label.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			else if (infile.key == "action_list") {
				// @ATTR action_list|point|Position of the action selector widget.
				Point pos = Parse::toPoint(infile.val);
				action_list->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
		}
		infile.close();
	}

	tablist.add(action_list);

	if (!background)
		setBackground("images/menus/confirm_bg.png");

	align();
}

void MenuConfirm::align() {
	Menu::align();

	label.setText(title);

	action_list->setPos(window_area.x, window_area.y);
	action_list->refresh();
	label.setPos(window_area.x, window_area.y);

	button_close->setPos(window_area.x, window_area.y);
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
	if (!visible)
		return;

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
	tablist.defocus();
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

