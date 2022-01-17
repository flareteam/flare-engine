/*
Copyright © 2020 Justin Jacobs

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
#include "MenuGameOver.h"
#include "MessageEngine.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"

#include <string>

MenuGameOver::MenuGameOver()
	: Menu()
	, button_continue(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, button_exit(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, continue_clicked(false)
	, exit_clicked(false)
{
	// default layout
	window_area = Rect(0,0,192,100);
	alignment = Utils::ALIGN_CENTER;
	button_continue->setBasePos(window_area.w/2 - button_continue->pos.w/2, window_area.h/2 - button_continue->pos.h/2, Utils::ALIGN_TOPLEFT);
	button_exit->setBasePos(window_area.w/2 - button_exit->pos.w/2, window_area.h/2 + button_exit->pos.h/2, Utils::ALIGN_TOPLEFT);
	label.setJustify(FontEngine::JUSTIFY_CENTER);
	label.setBasePos(window_area.w/2, 8, Utils::ALIGN_TOPLEFT);

	// Load config settings
	FileParser infile;
	// @CLASS MenuGameOver|Description of menus/game_over.txt
	if(infile.open("menus/game_over.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "label_title") {
				// @ATTR label_title|label|Position of the "Game Over" text.
				label.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			else if (infile.key == "button_continue") {
				// @ATTR button_continue|point|Position of the "Continue" button.
				Point pos = Parse::toPoint(infile.val);
				button_continue->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "button_exit") {
				// @ATTR button_exit|point|Position of the "Exit" button.
				Point pos = Parse::toPoint(infile.val);
				button_exit->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
		}
		infile.close();
	}

	label.setText(msg->get("Game Over"));
	label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	button_continue->setLabel(msg->get("Continue"));
	button_exit->setLabel(msg->get("Save & Exit"));

	tablist.add(button_continue);
	tablist.add(button_exit);

	if (!background)
		setBackground("images/menus/game_over.png");

	align();

	visible = false;
}

void MenuGameOver::align() {
	Menu::align();

	button_continue->setPos(window_area.x, window_area.y);
	button_exit->setPos(window_area.x, window_area.y);
	label.setPos(window_area.x, window_area.y);
}

void MenuGameOver::logic() {
	if (!visible)
		return;

	tablist.logic();

	if (button_continue->checkClick()) {
		continue_clicked = true;
	}
	else if (button_exit->checkClick()) {
		exit_clicked = true;
	}
}

void MenuGameOver::close() {
	visible = false;
	continue_clicked = false;
	exit_clicked = false;
	tablist.defocus();
}

void MenuGameOver::disableSave() {
	button_continue->enabled = false;
	button_exit->setLabel(msg->get("Exit"));
}

void MenuGameOver::render() {
	if (!visible)
		return;

	// background
	Menu::render();

	label.render();

	button_continue->render();
	button_exit->render();
}

MenuGameOver::~MenuGameOver() {
	delete button_continue;
	delete button_exit;
}

