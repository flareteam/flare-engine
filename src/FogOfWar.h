/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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
 * class FogOfWar
 *
 * Contains logic and rendering routines for fog of war.
 */

#ifndef FOGOFWAR_H
#define FOGOFWAR_H

#include "CommonIncludes.h"
#include "MapCollision.h"
#include "TileSet.h"
#include "Utils.h"

class FogOfWar {
public:
	enum {
		TYPE_NONE = 0,
		TYPE_MINIMAP = 1,
		TYPE_TINT = 2,
		TYPE_OVERLAY = 3,
	};

	static short unsigned TILE_HIDDEN;

	unsigned short dark_layer_id;
	unsigned short fog_layer_id;
	std::string tileset_dark;
	std::string tileset_fog;
	std::string mask_definition;
	TileSet tset_dark;
	TileSet tset_fog;
	int mask_radius;

	void logic();
	void handleIntramapTeleport();
	int load();
	Color getTileColorMod(const int_fast16_t x, const int_fast16_t y);

	FogOfWar();
	~FogOfWar();

private:
	int bits_per_tile;
	std::map<std::string, int> def_bits;
	std::map<std::string, int> def_tiles;
	unsigned short *def_mask;

	void loadHeader(FileParser &infile);
	void loadDefBit(FileParser &infile);
	void loadDefTile(FileParser &infile);
	void loadDefMask(FileParser &infile);

	Rect bounds;

	Color color_sight;
	Color color_visited;
	Color color_hidden;

	bool update_minimap;

	void calcBoundaries();
	void calcMiniBoundaries();
	void updateTiles();
};

#endif
