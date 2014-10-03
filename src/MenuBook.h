/*
Copyright © 2014 Igor Paliychuk

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
#include "FileParser.h"
#include "Menu.h"
#include "Utils.h"
#include "Widget.h"
#include "WidgetLabel.h"

class StatBlock;
class WidgetButton;

class MenuBook : public Menu {
public:

	MenuBook();
	~MenuBook();

	void loadGraphics();
	void logic();
	void render();

	std::string book_name;
	bool book_loaded;

	TabList tablist;

private:
	WidgetButton *closeButton;
	std::vector<Sprite*> image;
	std::vector<Point> image_dest;

	std::vector<Sprite*> text;
	std::vector<std::string> textData;
	std::vector<Color> textColor;
	std::vector<int> justify;
	std::vector<std::string> textFont;
	std::vector<Rect> size;

	void loadBook();
	void alignElements();
	void loadImage(FileParser &infile);
	void loadText(FileParser &infile);
	void clearBook();
};

#endif //MENU_BOOK_H
