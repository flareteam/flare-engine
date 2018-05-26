/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2016 Justin Jacobs

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

#ifndef MAP_COLLISION_H
#define MAP_COLLISION_H

#include "CommonIncludes.h"
#include "Utils.h"

typedef std::vector< std::vector<unsigned short> > Map_Layer;

class MapCollision {
private:
	// this value is used to determine the greatest possible position within a tile before transitioning to the next tile
	// so if an entity has a position of (1-MIN_TILE_GAP, 0) and moves to the east, they will move to (1,0)
	const float MIN_TILE_GAP = 0.001f;

	// collision check types
	const int CHECK_MOVEMENT = 1;
	const int CHECK_SIGHT = 2;

	bool line_check(const float& x1, const float& y1, const float& x2, const float& y2, int check_type, int movement_type);

	bool small_step_forced_slide_along_grid(
		float &x, float &y, float step_x, float step_y, int movement_type, int collide_type);
	bool small_step_forced_slide(
		float &x, float &y, float step_x, float step_y, int movement_type, int collide_type);
	bool small_step(
		float &x, float &y, float step_x, float step_y, int movement_type, int collide_type);

	bool is_valid_tile(const int& x, const int& y, int movement_type, int collide_type) const;

public:
	// const flags
	static const bool IGNORE_BLOCKED = true;
	static const bool IS_ALLY = true;

	// entity collision type
	enum {
		COLLIDE_NORMAL = 0,
		COLLIDE_HERO = 1,
		COLLIDE_NO_ENTITY = 2
	};

	// movement options
	enum {
		MOVE_NORMAL = 0,
		MOVE_FLYING = 1, // can move through BLOCKS_MOVEMENT (e.g. water)
		MOVE_INTANGIBLE = 2 // can move through BLOCKS_ALL (e.g. walls)
	};

	// collision tile types
	// The numbers 0..6 are the collision tiles as produced by tiled,
	// only 7 and 8 deal with entities on the map
	enum {
		BLOCKS_NONE = 0,
		BLOCKS_ALL = 1,
		BLOCKS_MOVEMENT = 2,
		BLOCKS_ALL_HIDDEN = 3,
		BLOCKS_MOVEMENT_HIDDEN = 4,
		MAP_ONLY = 5,
		MAP_ONLY_ALT = 6,
		BLOCKS_ENTITIES = 7, // hero or enemies are blocking this tile, so any other entity is blocked
		BLOCKS_ENEMIES = 8  // an ally is standing on that tile, so the hero could pass if ENABLE_ALLY_COLLISION is false
	};

	MapCollision();
	~MapCollision();

	void setmap(const Map_Layer& _colmap, unsigned short w, unsigned short h);
	bool move(float &x, float &y, float step_x, float step_y, int movement_type, int collide_type);

	bool is_outside_map(const int& tile_x, const int& tile_y) const;
	bool is_outside_map(const float& tile_x, const float& tile_y) const;
	bool is_empty(const float& x, const float& y) const;
	bool is_wall(const float& x, const float& y) const;

	bool is_valid_position(const float& x, const float& y, int movement_type, int collide_type) const;

	bool line_of_sight(const float& x1, const float& y1, const float& x2, const float& y2);
	bool line_of_movement(const float& x1, const float& y1, const float& x2, const float& y2, int movement_type);

	bool is_facing(const float& x1, const float& y1, char direction, const float& x2, const float& y2);

	bool compute_path(const FPoint& start, const FPoint& end, std::vector<FPoint> &path, int movement_type, unsigned int limit = 0);

	void block(const float& map_x, const float& map_y, bool is_ally);
	void unblock(const float& map_x, const float& map_y);

	FPoint get_random_neighbor(const Point& target, int range, bool ignore_blocked);

	int getCollideType(bool hero) {
		return hero ? COLLIDE_HERO : COLLIDE_NORMAL;
	}

	Map_Layer colmap;
	Point map_size;
};

#endif
