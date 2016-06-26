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

#include "FileParser.h"
#include "MenuBook.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "SharedGameResources.h"

#include <climits>

MenuBook::MenuBook()
	: book_name("")
	, last_book_name("")
	, book_loaded(false) {

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	tablist = TabList();
	tablist.add(closeButton);
}

void MenuBook::loadBook() {
	if (last_book_name != book_name) {
		last_book_name = "";
		book_loaded = false;
		clearBook();
	}

	if (book_loaded) return;

	// Read data from config file
	FileParser infile;

	// @CLASS MenuBook|Description of books in books/
	if (infile.open(book_name)) {
		last_book_name = book_name;

		while (infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			infile.val = infile.val + ',';

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				closeButton->setBasePos(x, y);
			}
			// @ATTR background|filename|Filename for the background image.
			else if (infile.key == "background") {
				setBackground(popFirstString(infile.val));
			}
			else if (infile.section == "") {
				infile.error("MenuBook: '%s' is not a valid key.", infile.key.c_str());
			}

			if (infile.new_section) {

				// for sections that are stored in collections, add a new object here
				if (infile.section == "text") {
					text.push_back(NULL);
					textData.push_back("");
					textColor.push_back(Color());
					justify.push_back(0);
					textFont.push_back("");
					size.push_back(Rect());
				}
				else if (infile.section == "image") {
					image.push_back(NULL);
					image_dest.push_back(Point());
				}

			}
			if (infile.section == "text")
				loadText(infile);
			else if (infile.section == "image")
				loadImage(infile);
		}

		infile.close();
	}

	// setup image dest
	for (unsigned i=0; i < image.size(); i++) {
	       image[i]->setDest(image_dest[i]);
	}

	// render text to surface
	for (unsigned i=0; i<text.size(); i++) {
		font->setFont(textFont[i]);
		Point pSize = font->calc_size(textData[i], size[i].w);
		Image *graphics = render_device->createImage(size[i].w, pSize.y);

		if (justify[i] == JUSTIFY_CENTER)
			font->render(textData[i], size[i].w/2, 0, justify[i], graphics, size[i].w, textColor[i]);
		else if (justify[i] == JUSTIFY_RIGHT)
			font->render(textData[i], size[i].w, 0, justify[i], graphics, size[i].w, textColor[i]);
		else
			font->render(textData[i], 0, 0, justify[i], graphics, size[i].w, textColor[i]);
		text[i] = graphics->createSprite();
		graphics->unref();
	}

	align();

	book_loaded = true;
}

void MenuBook::loadImage(FileParser &infile) {
	// @ATTR image.image_pos|point|Position of the image.
	if (infile.key == "image_pos") {
		image_dest.back() = toPoint(infile.val);
	}
	// @ATTR image.image|filename|Filename of the image.
	else if (infile.key == "image") {
		Image *graphics;
		graphics = render_device->loadImage(popFirstString(infile.val));
		if (graphics) {
		  image.back() = graphics->createSprite();
		  graphics->unref();
		}
	}
	else {
		infile.error("MenuBook: '%s' is not a valid key.", infile.key.c_str());
	}
}

void MenuBook::loadText(FileParser &infile) {
	// @ATTR text.text_pos|int, int, int, ["left", "center", "right"] : X, Y, Width, Text justify|Position of the text.
	if (infile.key == "text_pos") {
		size.back().x = popFirstInt(infile.val);
		size.back().y = popFirstInt(infile.val);
		size.back().w = popFirstInt(infile.val);
		std::string _justify = popFirstString(infile.val);

		if (_justify == "left") justify.back() = JUSTIFY_LEFT;
		else if (_justify == "center") justify.back() = JUSTIFY_CENTER;
		else if (_justify == "right") justify.back() = JUSTIFY_RIGHT;
	}
	// @ATTR text.text_font|color, string : Font color, Font style|Font color and style.
	else if (infile.key == "text_font") {
		Color color;
		color.r = static_cast<Uint8>(popFirstInt(infile.val));
		color.g = static_cast<Uint8>(popFirstInt(infile.val));
		color.b = static_cast<Uint8>(popFirstInt(infile.val));
		textColor.back() = color;
		textFont.back() = popFirstString(infile.val);
	}
	// @ATTR text.text|string|The text to be displayed.
	else if (infile.key == "text") {
		// we use substr here to remove the comma from the end
		textData.back() = msg->get(infile.val.substr(0, infile.val.length() - 1));
	}
	else {
		infile.error("MenuBook: '%s' is not a valid key.", infile.key.c_str());
	}
}

void MenuBook::align() {
	Menu::align();

	closeButton->setPos(window_area.x, window_area.y);

	for (unsigned i=0; i<text.size(); i++) {
		text[i]->setDestX(size[i].x + window_area.x);
		text[i]->setDestY(size[i].y + window_area.y);
	}
	for (unsigned i=0; i<image.size(); i++) {
		image[i]->setDestX(image_dest[i].x + window_area.x);
		image[i]->setDestY(image_dest[i].y + window_area.y);
	}
}

void MenuBook::clearBook() {
	for (unsigned i=0; i<text.size(); i++) {
		delete text[i];
	}
	text.clear();

	textData.clear();
	textColor.clear();
	textFont.clear();
	size.clear();
	image_dest.clear();

	for (unsigned i=0; i<image.size(); i++) {
		delete image[i];
	}
	image.clear();
}

void MenuBook::logic() {
	if (book_name == "") return;
	else {
		loadBook();
		visible = true;
	}

	tablist.logic();

	if (closeButton->checkClick() || (inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT])) {
		if (inpt->pressing[ACCEPT]) inpt->lock[ACCEPT] = true;

		clearBook();

		snd->play(sfx_close);

		visible = false;
		book_name = "";
		last_book_name = "";
		book_loaded = false;
	}
}

void MenuBook::render() {
	if (!visible) return;

	Menu::render();

	closeButton->render();
	for (unsigned i=0; i<text.size(); i++) {
		render_device->render(text[i]);
	}
	for (unsigned i=0; i<image.size(); i++) {
		render_device->render(image[i]);
	}
}

MenuBook::~MenuBook() {
	delete closeButton;
	clearBook();

}
