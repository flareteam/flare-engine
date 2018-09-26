/*
Copyright © 2017 Justin Jacobs

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
 * class Subtitles
 *
 * Handles loading and displaying subtitles for sound files
 */

#ifndef SUBTITLES_H
#define SUBTITLES_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "WidgetLabel.h"

class Subtitles {
public:
	Subtitles();
	~Subtitles();
	void logic(unsigned long id);
	void render();

private:
	void setTextByID(unsigned long id);
	void updateLabelAndBackground();

	std::vector<unsigned long> filename;
	std::vector<std::string> text;

	WidgetLabel label;
	Point label_pos;
	int label_alignment;
	unsigned long current_id;
	std::string current_text;
	bool visible;
	Sprite *background;
	Rect background_rect;
	Color background_color;
	Timer visible_timer;
};

#endif
