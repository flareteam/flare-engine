/*
Copyright © 2016 Justin Jacobs

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

#ifndef ICON_MANAGER_H
#define ICON_MANAGER_H

#include "Utils.h"

class Sprite;

class IconSet {
public:
	IconSet();

	Sprite *gfx;
	int id_begin;
	int id_end;
	int columns;
};

class IconManager {
public:
	IconManager();
	~IconManager();

	void setIcon(int icon_id, Point dest_pos);
	void render();
	void renderToImage(Image *img);

	Point text_offset;

private:
	bool loadIconSet(IconSet& icon_set, const std::string& filename, int first_id);

	std::vector<IconSet> icon_sets;
	IconSet *current_set;
	Rect current_src;
	Rect current_dest;
};

#endif
