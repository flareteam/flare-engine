/*
Copyright © 2014 Igor Paliychuk
Copyright © 2014 Henrik Andersson
Copyright © 2014-2016 Justin Jacobs

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
 * class MenuBook
 */

#include "CampaignManager.h"
#include "EngineSettings.h"
#include "EventManager.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "IconManager.h"
#include "InputState.h"
#include "MenuBook.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"

#include <climits>

MenuBook::MenuBook()
	: book_name("")
	, last_book_name("")
	, book_loaded(false)
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, event_open(NULL)
	, event_close(NULL)
{
	tablist = TabList();
}

void MenuBook::loadBook() {
	if (last_book_name != book_name) {
		last_book_name.clear();
		book_loaded = false;
		clearBook();
	}

	if (book_loaded) return;

	tablist.add(closeButton);

	// Read data from config file
	FileParser infile;

	// @CLASS MenuBook|Description of books in books/
	if (infile.open(book_name, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		last_book_name = book_name;

		while (infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			infile.val = infile.val + ',';

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				closeButton->setBasePos(x, y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR background|filename|Filename for the background image.
			else if (infile.key == "background") {
				setBackground(Parse::popFirstString(infile.val));
			}
			else if (infile.section == "") {
				infile.error("MenuBook: '%s' is not a valid key.", infile.key.c_str());
			}

			if (infile.new_section) {
				// for sections that are stored in collections, add a new object here
				if (infile.section == "text") {
					text.resize(text.size() + 1);
				}
				else if (infile.section == "image") {
					images.resize(images.size() + 1);
				}
				else if (infile.section == "button") {
					buttons.resize(buttons.size() + 1);
				}
				else if (infile.section == "event_open" && !event_open) {
					event_open = new Event();
				}
				else if (infile.section == "event_close" && !event_close) {
					event_close = new Event();
				}

			}
			if (infile.section == "text" && !text.empty())
				loadText(infile, text.back());
			else if (infile.section == "image" && !images.empty())
				loadImage(infile, images.back());
			else if (infile.section == "button" && !buttons.empty())
				loadButton(infile, buttons.back());
			else if (infile.section == "event_open")
				loadBookEvent(infile, *event_open);
			else if (infile.section == "event_close")
				loadBookEvent(infile, *event_close);
		}

		infile.close();
	}

	refreshText();

	for (size_t i = images.size(); i > 0;) {
		i--;

		if (images[i].image)
			images[i].image->setDestFromPoint(images[i].dest);
		else if (images[i].icon == -1)
			images.erase(images.begin() + i);
	}

	for (size_t i = 0; i < buttons.size(); ++i) {
		if (buttons[i].image.empty())
			buttons[i].button = new WidgetButton(WidgetButton::DEFAULT_FILE);
		else
			buttons[i].button = new WidgetButton(buttons[i].image);

		buttons[i].button->setBasePos(buttons[i].dest.x, buttons[i].dest.y, Utils::ALIGN_TOPLEFT);
		buttons[i].button->setLabel(msg->get(buttons[i].label));
		buttons[i].button->refresh();

		tablist.add(buttons[i].button);
	}

	align();

	snd->play(sfx_open, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);

	if (event_open && EventManager::isActive(*event_open)) {
		EventManager::executeEvent(*event_open);
	}

	book_loaded = true;
}

void MenuBook::loadImage(FileParser &infile, BookImage& bimage) {
	// @ATTR image.image_pos|point|Position of the image.
	if (infile.key == "image_pos") {
		bimage.dest = Parse::toPoint(infile.val);
	}
	// @ATTR image.image|filename|Filename of the image.
	else if (infile.key == "image") {
		if (bimage.image) {
			delete bimage.image;
			bimage.image = NULL;
		}

		Image *graphics;
		graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
		if (graphics) {
		  bimage.image = graphics->createSprite();
		  graphics->unref();
		}
	}
	// @ATTR image_icon|icon_id|Use an icon as the image instead of a file.
	else if (infile.key == "image_icon") {
		if (bimage.image) {
			delete bimage.image;
			bimage.image = NULL;
		}

		bimage.icon = Parse::toInt(infile.val);
	}
	// @ATTR image.requires_status|list(string)|Image requires these campaign statuses in order to be visible.
	else if (infile.key == "requires_status") {
		std::string temp = Parse::popFirstString(infile.val);
		while (!temp.empty()) {
			bimage.requires_status.push_back(camp->registerStatus(temp));
			temp = Parse::popFirstString(infile.val);
		}
	}
	// @ATTR image.requires_not_status|list(string)|Image must not have any of these campaign statuses in order to be visible.
	else if (infile.key == "requires_not_status") {
		std::string temp = Parse::popFirstString(infile.val);
		while (!temp.empty()) {
			bimage.requires_not_status.push_back(camp->registerStatus(temp));
			temp = Parse::popFirstString(infile.val);
		}
	}
	else {
		infile.error("MenuBook: '%s' is not a valid key.", infile.key.c_str());
	}
}

void MenuBook::loadText(FileParser &infile, BookText& btext) {
	// @ATTR text.text_pos|int, int, int, ["left", "center", "right"] : X, Y, Width, Text justify|Position of the text.
	if (infile.key == "text_pos") {
		btext.size.x = Parse::popFirstInt(infile.val);
		btext.size.y = Parse::popFirstInt(infile.val);
		btext.size.w = Parse::popFirstInt(infile.val);
		std::string _justify = Parse::popFirstString(infile.val);

		if (_justify == "left") btext.justify = FontEngine::JUSTIFY_LEFT;
		else if (_justify == "center") btext.justify = FontEngine::JUSTIFY_CENTER;
		else if (_justify == "right") btext.justify = FontEngine::JUSTIFY_RIGHT;
	}
	// @ATTR text.text_font|color, string : Font color, Font style|Font color and style.
	else if (infile.key == "text_font") {
		btext.color.r = static_cast<Uint8>(Parse::popFirstInt(infile.val));
		btext.color.g = static_cast<Uint8>(Parse::popFirstInt(infile.val));
		btext.color.b = static_cast<Uint8>(Parse::popFirstInt(infile.val));
		btext.font= Parse::popFirstString(infile.val);
	}
	// @ATTR text.shadow|bool|If true, the text will have a black shadow like the text labels in various menus.
	else if (infile.key == "text_shadow") {
		btext.shadow = Parse::toBool(Parse::popFirstString(infile.val));
	}
	// @ATTR text.text|string|The text to be displayed.
	else if (infile.key == "text") {
		// we use substr here to remove the trailing comma that was added in loadBook()
		btext.text_raw = infile.val.substr(0, infile.val.length() - 1);
	}
	// @ATTR text.requires_status|list(string)|Text requires these campaign statuses in order to be visible.
	else if (infile.key == "requires_status") {
		std::string temp = Parse::popFirstString(infile.val);
		while (!temp.empty()) {
			btext.requires_status.push_back(camp->registerStatus(temp));
			temp = Parse::popFirstString(infile.val);
		}
	}
	// @ATTR text.requires_not_status|list(string)|Text must not have any of these campaign statuses in order to be visible.
	else if (infile.key == "requires_not_status") {
		std::string temp = Parse::popFirstString(infile.val);
		while (!temp.empty()) {
			btext.requires_not_status.push_back(camp->registerStatus(temp));
			temp = Parse::popFirstString(infile.val);
		}
	}
	else {
		infile.error("MenuBook: '%s' is not a valid key.", infile.key.c_str());
	}
}

void MenuBook::loadButton(FileParser &infile, BookButton& bbutton) {
	// @ATTR button.button_pos|point|Position of the button.
	if (infile.key == "button_pos") {
		bbutton.dest.x = Parse::popFirstInt(infile.val);
		bbutton.dest.y = Parse::popFirstInt(infile.val);
	}
	// @ATTR button.button_image|filename|Image file to use for this button. Default is the normal menu button.
	else if (infile.key == "button_image") {
		bbutton.image = Parse::popFirstString(infile.val);
	}
	// @ATTR button.text|string|Optional text label for the button.
	else if (infile.key == "text") {
		bbutton.label = Parse::popFirstString(infile.val);
	}
	else {
		// @ATTR book.${EVENT_COMPONENT}|Event components to execute when the button is clicked. See the definitions in EventManager for possible attributes.
		loadBookEvent(infile, bbutton.event);
	}
}

void MenuBook::loadBookEvent(FileParser &infile, Event& ev) {
	// we use substr here to remove the trailing comma that was added in loadBook()
	std::string trimmed = infile.val.substr(0, infile.val.length() - 1);

	// @ATTR event_open.${EVENT_COMPONENT}|Event components to execute when the book is opened. See the definitions in EventManager for possible attributes.
	// @ATTR event_close.${EVENT_COMPONENT}|Event components to execute when the book is closed. See the definitions in EventManager for possible attributes.
	if (!EventManager::loadEventComponentString(infile.key, trimmed, &ev, NULL)) {
		infile.error("MenuBook: '%s' is not a valid key.", infile.key.c_str());
	}
}

void MenuBook::align() {
	Menu::align();

	closeButton->setPos(window_area.x, window_area.y);

	for (unsigned i=0; i<text.size(); i++) {
		text[i].sprite->setDest(text[i].size.x + window_area.x, text[i].size.y + window_area.y);
	}
	for (size_t i = 0; i < images.size(); ++i) {
		if (images[i].image)
			images[i].image->setDest(images[i].dest.x + window_area.x, images[i].dest.y + window_area.y);
	}
	for (size_t i = 0; i < buttons.size(); ++i) {
		buttons[i].button->setPos(window_area.x, window_area.y);
	}
}

void MenuBook::clearBook() {
	for (size_t i = 0; i < text.size(); ++i) {
		delete text[i].sprite;
	}
	text.clear();

	for (size_t i = 0; i < images.size(); ++i) {
		delete images[i].image;
	}
	images.clear();

	for (size_t i = 0; i < buttons.size(); ++i) {
		delete buttons[i].button;
	}
	buttons.clear();

	tablist.clear();

	delete event_open;
	event_open = NULL;

	delete event_close;
	event_close = NULL;
}

void MenuBook::closeWindow() {
	if (event_close && EventManager::isActive(*event_close)) {
		EventManager::executeEvent(*event_close);
	}

	clearBook();

	visible = false;
	book_name.clear();
	last_book_name.clear();
	book_loaded = false;
}

void MenuBook::refreshText() {
	for (size_t i = text.size(); i > 0;) {
		i--;

		std::string text_new = Utils::substituteVarsInString(msg->get(text[i].text_raw), pc);
		if (text[i].text == text_new)
			continue;

		text[i].text = text_new;

		// render text to surface
		font->setFont(text[i].font);
		Point pSize = font->calc_size(text[i].text, text[i].size.w);
		Image *graphics = NULL;
		if (text[i].shadow) {
			graphics = render_device->createImage(text[i].size.w + 1, pSize.y + 1);
		}
		else {
			graphics = render_device->createImage(text[i].size.w, pSize.y);
		}

		if (graphics) {
			int x_offset = 0;
			if (text[i].justify == FontEngine::JUSTIFY_CENTER)
				x_offset = text[i].size.w / 2;
			else if (text[i].justify == FontEngine::JUSTIFY_RIGHT)
				x_offset = text[i].size.w;

			if (text[i].shadow) {
				font->render(text[i].text, x_offset + 1, 1, text[i].justify, graphics, text[i].size.w, font->getColor(FontEngine::COLOR_BLACK));
			}
			font->render(text[i].text, x_offset, 0, text[i].justify, graphics, text[i].size.w, text[i].color);
			text[i].sprite = graphics->createSprite();
			graphics->unref();
		}

		if (!graphics || !text[i].sprite)
			text.erase(text.begin() + i);
	}

}

void MenuBook::logic() {
	if (book_name.empty()) return;
	else {
		loadBook();
		visible = true;
	}

	if (!visible)
		return;

	tablist.logic();

	if (closeButton->checkClick()) {
		closeWindow();
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}
	else if (inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT] && tablist.getCurrent() == -1) {
		inpt->lock[Input::ACCEPT] = true;
		closeWindow();
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}
	else if (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) {
		inpt->lock[Input::CANCEL] = true;
		closeWindow();
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}

	refreshText();

	for (size_t i = 0; i < buttons.size(); ++i) {
		if (buttons[i].event.components.empty() || !EventManager::isActive(buttons[i].event)) {
			buttons[i].button->enabled = false;
			buttons[i].button->refresh();
		}
		else {
			buttons[i].button->enabled = true;
			buttons[i].button->refresh();
		}

		if (buttons[i].button->checkClick()) {
			if (EventManager::executeEvent(buttons[i].event)) {
				buttons[i].event = Event();
			}
		}
	}
}

void MenuBook::render() {
	if (!visible) return;

	Menu::render();

	closeButton->render();
	for (unsigned i=0; i<text.size(); i++) {
		bool skip = false;

		for (size_t j = 0; j < text[i].requires_status.size(); ++j) {
			if (!camp->checkStatus(text[i].requires_status[j]))
				skip = true;
		}

		for (size_t j = 0; j < text[i].requires_not_status.size(); ++j) {
			if (camp->checkStatus(text[i].requires_not_status[j]))
				skip = true;
		}

		if (skip)
			continue;

		render_device->render(text[i].sprite);
	}
	for (size_t i = 0; i < images.size(); ++i) {
		bool skip = false;

		for (size_t j = 0; j < images[i].requires_status.size(); ++j) {
			if (!camp->checkStatus(images[i].requires_status[j]))
				skip = true;
		}

		for (size_t j = 0; j < images[i].requires_not_status.size(); ++j) {
			if (camp->checkStatus(images[i].requires_not_status[j]))
				skip = true;
		}

		if (skip)
			continue;

		if (images[i].image) {
			render_device->render(images[i].image);
		}
		else if (images[i].icon != -1) {
			icons->setIcon(images[i].icon, Point(images[i].dest.x + window_area.x, images[i].dest.y + window_area.y));
			icons->render();
		}
	}
	for (size_t i = 0; i < buttons.size(); ++i) {
		buttons[i].button->render();
	}
}

MenuBook::~MenuBook() {
	delete closeButton;
	closeWindow();
}
