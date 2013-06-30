/*
Copyright © 2011-2012 Clint Bellanger

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

/*
 * MapCollision.h
 * RPGEngine
 *
 * Handle collisions between objects and the map
 */

#pragma once
#ifndef MAP_COLLISION_H
#define MAP_COLLISION_H

#include "CommonIncludes.h"
#include "Utils.h"

#include <cstdlib>

// collision tile types
// The numbers 0..6 are the collision tiles as produced by tiled,
// only 7 and 8 deal with entities on the map
const int BLOCKS_NONE = 0;
const int BLOCKS_ALL = 1;
const int BLOCKS_MOVEMENT = 2;
const int BLOCKS_ALL_HIDDEN = 3;
const int BLOCKS_MOVEMENT_HIDDEN = 4;
const int MAP_ONLY = 5;
const int MAP_ONLY_ALT = 6;
const int BLOCKS_ENTITIES = 7; // hero or enemies are blocking this tile, so any other entity is blocked
const int BLOCKS_ENEMIES = 8;  // an ally is standing on that tile, so the hero could pass if ENABLE_ALLY_COLLISION is false

// collision check types
const int CHECK_MOVEMENT = 1;
const int CHECK_SIGHT = 2;

// movement options
typedef enum {
	MOVEMENT_NORMAL = 0,
	MOVEMENT_FLYING = 1, // can move through BLOCKS_MOVEMENT (e.g. water)
	MOVEMENT_INTANGIBLE = 2 // can move through BLOCKS_ALL (e.g. walls)
} MOVEMENTTYPE;

class MapCollision {
private:

	bool line_check(int x1, int y1, int x2, int y2, int check_type, MOVEMENTTYPE movement_type);
	bool is_sidestepable(int tile_x, int tile_y, int offx2, int offy2);

public:
	MapCollision();
	~MapCollision();

	void setmap(const unsigned short _colmap[][256], unsigned short w, unsigned short h);
	bool move(int &x, int &y, int step_x, int step_y, int dist, MOVEMENTTYPE movement_type, bool is_hero);

	bool is_outside_map(int tile_x, int tile_y) const;
	bool is_empty(int x, int y) const;
	bool is_wall(int x, int y) const;

	bool is_valid_tile(int x, int y, MOVEMENTTYPE movement_type, bool is_hero) const;
	bool is_valid_position(int x, int y, MOVEMENTTYPE movement_type, bool is_hero) const;

	int is_one_step_around(int x, int y, int xidr, int ydir);

	bool line_of_sight(int x1, int y1, int x2, int y2);
	bool line_of_movement(int x1, int y1, int x2, int y2, MOVEMENTTYPE movement_type);

	bool is_facing(int x1, int y1, char direction, int x2, int y2);

	bool compute_path(Point start, Point end, std::vector<Point> &path, MOVEMENTTYPE movement_type, unsigned int limit = 0);

	void block(int map_x, int map_y, bool is_ally);
	void unblock(int map_x, int map_y);

	unsigned short colmap[256][256];
	Point map_size;
};

#endif
