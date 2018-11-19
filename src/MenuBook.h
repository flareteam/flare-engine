/*
Copyright © 2014 Igor Paliychuk
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

#ifndef MENU_BOOK_H
#define MENU_BOOK_H

#include "CommonIncludes.h"
#include "EventManager.h"
#include "Menu.h"
#include "Utils.h"

class StatBlock;
class WidgetButton;

class MenuBook : public Menu {
public:

	MenuBook();
	~MenuBook();
	void align();

	void loadGraphics();
	void logic();
	void render();
	void closeWindow();

	std::string book_name;
	std::string last_book_name;
	bool book_loaded;

private:
	class BookImage {
	public:
		Sprite* image;
		int icon;
		Point dest;
		std::vector<StatusID> requires_status;
		std::vector<StatusID> requires_not_status;

		BookImage()
			: image(NULL)
			, icon(-1)
		{}
	};

	class BookText {
	public:
		Sprite* sprite;
		std::string text;
		std::string text_raw;
		std::string font;
		Color color;
		Rect size;
		int justify;
		bool shadow;
		std::vector<StatusID> requires_status;
		std::vector<StatusID> requires_not_status;

		BookText()
			: sprite(NULL)
			, justify(0)
			, shadow(false)
		{}
	};

	class BookButton {
	public:
		WidgetButton* button;
		Point dest;
		std::string image;
		std::string label;
		Event event;

		BookButton()
			: button(NULL)
		{}
	};

	WidgetButton *closeButton;
	std::vector<BookImage> images;
	std::vector<BookText> text;
	std::vector<BookButton> buttons;
	Event* event_open;
	Event* event_close;

	void loadBook();
	void alignElements();
	void loadImage(FileParser &infile, BookImage& bimage);
	void loadText(FileParser &infile, BookText& btext);
	void loadButton(FileParser &infile, BookButton& bbutton);
	void loadBookEvent(FileParser &infile, Event& ev);
	void clearBook();
	void refreshText();
};

#endif //MENU_BOOK_H
