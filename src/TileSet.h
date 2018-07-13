/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson

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
 * class TileSet
 *
 * TileSet storage and file loading
 */

#ifndef TILE_SET_H
#define TILE_SET_H

#include "CommonIncludes.h"
#include "Utils.h"

/**
 * Describes a tile by its location \a src in the tileset sprite and
 * by the \a offset to be applied when rendering it on screen.
 * The offset is measured from upper left corner to the logical midpoint
 * of the tile at groundlevel.
 */
class Tile_Def {
public:
	Point offset;
	Sprite *tile;// inside graphics not used
	Tile_Def()
		: tile(NULL) {
	}
};

class TileSet {
private:
	class Tile_Anim {
	public:
		// Number of frames in this animation. if 0 no animation.
		// 1 makes no sense as it would produce astatic animation.
		unsigned short frames;
		unsigned short current_frame; // is in range 0..(frames-1)
		unsigned short duration; // how long the current frame is already displayed in ticks.
		std::vector<Point> pos; // position of each image.
		std::vector<unsigned short> frame_duration; // duration of each image in ticks. 0 will be treated the same as 1.
		Tile_Anim() {
			frames = 0;
			current_frame = 0;
			duration = 0;
		}
	};

	void loadGraphics(const std::string& filename, Sprite** sprite);
	void reset();

	std::string current_filename;

	std::vector<Sprite*> sprites;
	std::vector<Tile_Anim> anim;

public:
	// functions
	TileSet();
	~TileSet();
	void load(const std::string& filename);
	void logic();

	std::vector<Tile_Def> tiles;

	// oversize of the largest tile available, in number of tiles.
	int max_size_x;
	int max_size_y;
};

#endif
