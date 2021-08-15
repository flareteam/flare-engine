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

#define	FOW_BIT_NW				(1 << 0)
#define	FOW_BIT_N				(1 << 1)
#define	FOW_BIT_NE				(1 << 2)
#define	FOW_BIT_W				(1 << 3)
#define	FOW_BIT_C				(1 << 4)
#define	FOW_BIT_E				(1 << 5)
#define	FOW_BIT_SW				(1 << 6)
#define	FOW_BIT_S				(1 << 7)
#define	FOW_BIT_SE				(1 << 8)

// these defines are all 7 chars to make it easier to read the circle masks
#define	fow_non					0
#define	fow_all					(FOW_BIT_NW | FOW_BIT_N | FOW_BIT_NE | FOW_BIT_W | FOW_BIT_C | FOW_BIT_E | FOW_BIT_SW | FOW_BIT_S | FOW_BIT_SE)
#define	NUM_FOW_ENTRIES			fow_all

// straights
#define	fow_EEE					(FOW_BIT_SE | FOW_BIT_E | FOW_BIT_NE)
#define	fow_NNN					(FOW_BIT_NE | FOW_BIT_N | FOW_BIT_NW)
#define	fow_WWW					(FOW_BIT_NW | FOW_BIT_W | FOW_BIT_SW)
#define	fow_SSS					(FOW_BIT_SW | FOW_BIT_S | FOW_BIT_SE)

// corners
#define	fow_CNE					(FOW_BIT_E | FOW_BIT_NE | FOW_BIT_N | FOW_BIT_NW | FOW_BIT_C | FOW_BIT_SE)
#define	fow_CNW					(FOW_BIT_N | FOW_BIT_NW | FOW_BIT_W | FOW_BIT_SW | FOW_BIT_C | FOW_BIT_NE)
#define	fow_CSW					(FOW_BIT_W | FOW_BIT_SW | FOW_BIT_S | FOW_BIT_NW | FOW_BIT_C | FOW_BIT_SE)
#define	fow_CSE					(FOW_BIT_S | FOW_BIT_SE | FOW_BIT_E | FOW_BIT_NE | FOW_BIT_C | FOW_BIT_SW)

// joins
#define	fow_JNE					(FOW_BIT_E | FOW_BIT_NE | FOW_BIT_N)
#define	fow_JNW					(FOW_BIT_N | FOW_BIT_NW | FOW_BIT_W)
#define	fow_JSW					(FOW_BIT_W | FOW_BIT_SW | FOW_BIT_S)
#define	fow_JSE					(FOW_BIT_S | FOW_BIT_SE | FOW_BIT_E)

#define	FOW_RADIUS_MIN			3
#define	FOW_RADIUS_MAX			9
#define	NUM_FOW_RADII			((FOW_RADIUS_MAX - FOW_RADIUS_MIN) + 1)

#define	FOW_MAX_RADIUS_LENGTH	((FOW_RADIUS_MAX * 2) + 1)

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

		TILE_SIGHT = 0,
		TILE_HIDDEN = 1,
		TILE_VISITED = 2,
	};

	unsigned short layer_id;
	std::string tileset;
	TileSet tset;

	void logic();
	void applyMask();
	int load();
	void handleIntramapTeleport();
	Color getTileColorMod(const int_fast16_t x, const int_fast16_t y);

	FogOfWar();
	~FogOfWar();

private:
	Rect bounds;

	Color color_sight;
	Color color_visited;
	Color color_hidden;

	bool update_minimap;

	void calcBoundaries();
	void updateTiles(unsigned short sight_tile);
	static const unsigned short CIRCLE_MASK[NUM_FOW_RADII][FOW_MAX_RADIUS_LENGTH * FOW_MAX_RADIUS_LENGTH];
};

#endif
