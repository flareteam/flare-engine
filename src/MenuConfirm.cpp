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
#include "Settings.h"
#include "SharedResources.h"
#include "WidgetButton.h"

#include <string>

MenuConfirm::MenuConfirm(const std::string& _buttonMsg, const std::string& _boxMsg)
	: Menu()
	, buttonConfirm(NULL)
	, buttonClose(NULL)
	, hasConfirmButton(false)
	, confirmClicked(false)
	, cancelClicked(false)
	, isWithinButtons(false) {

	// Load config settings
	FileParser infile;
	if(infile.open("menus/confirm.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
		}
		infile.close();
	}

	if (_buttonMsg != "") hasConfirmButton = true;
	// Text to display in confirmation box
	boxMsg = _boxMsg;

	if (hasConfirmButton) {
		buttonConfirm = new WidgetButton();
		buttonConfirm->label = _buttonMsg;
		tablist.add(buttonConfirm);
	}

	buttonClose = new WidgetButton("images/menus/buttons/button_x.png");
	tablist.add(buttonClose);

	setBackground("images/menus/confirm_bg.png");
	align();
}

void MenuConfirm::align() {
	Menu::align();

	if (hasConfirmButton) {
		buttonConfirm->pos.x = window_area.x + window_area.w/2 - buttonConfirm->pos.w/2;
		buttonConfirm->pos.y = window_area.y + window_area.h/2;
		buttonConfirm->refresh();
		label.set(window_area.x + window_area.w/2, window_area.y + window_area.h - (buttonConfirm->pos.h * 2), FontEngine::JUSTIFY_CENTER, VALIGN_TOP, boxMsg, font->getColor("menu_normal"));
	}
	else {
		label.set(window_area.x + window_area.w/2, window_area.y + (window_area.h / 4), FontEngine::JUSTIFY_CENTER, VALIGN_TOP, boxMsg, font->getColor("menu_normal"));
	}

	buttonClose->pos.x = window_area.x + window_area.w;
	buttonClose->pos.y = window_area.y;
}

void MenuConfirm::logic() {
	if (visible) {
		tablist.logic(true);
		confirmClicked = false;

		if (hasConfirmButton && buttonConfirm->checkClick()) {
			confirmClicked = true;
		}
		if (buttonClose->checkClick()) {
			visible = false;
			confirmClicked = false;
			cancelClicked = true;
		}

		// check if the mouse cursor is hovering over the close button
		// this is for the confirm dialog that shows when changing keybinds
		isWithinButtons = isWithinRect(buttonClose->pos, inpt->mouse) || (hasConfirmButton && isWithinRect(buttonConfirm->pos, inpt->mouse));
	}
}

void MenuConfirm::render() {
	// background
	Menu::render();

	label.render();

	if (hasConfirmButton) buttonConfirm->render();
	buttonClose->render();
}

MenuConfirm::~MenuConfirm() {
	if (hasConfirmButton) delete buttonConfirm;
	delete buttonClose;
}

