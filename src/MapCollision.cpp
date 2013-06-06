/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller

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

#include "AStarNode.h"
#include "MapCollision.h"
#include "Settings.h"
#include <cfloat>
#include <math.h>

using namespace std;

MapCollision::MapCollision()
	: map_size(Point()) {
	memset(colmap, 0, sizeof(colmap));
}

void MapCollision::setmap(const unsigned short _colmap[][256], unsigned short w, unsigned short h) {
	for (int i=0; i<w; i++)
		for (int j=0; j<h; j++)
			colmap[i][j] = _colmap[i][j];

	map_size.x = w;
	map_size.y = h;
}

/**
 * Process movement for cardinal (90 degree) and ordinal (45 degree) directions
 * If we encounter an obstacle at 90 degrees, stop.
 * If we encounter an obstacle at 45 or 135 degrees, slide.
 */
bool MapCollision::move(float &x, float &y, float _step_x, float _step_y, MOVEMENTTYPE movement_type, bool is_hero) {
	// when trying to slide against a bottom or right wall, step_x or step_y can become 0
	// this causes diag to become false, making this function return false
	// we try to catch such a scenario and return true early
	bool force_slide = (_step_x != 0 && _step_y > 0) || (_step_x > 0 && _step_y != 0);

	while (_step_x != 0 || _step_y != 0) {

		float step_x = 0;
		if (_step_x > 0) {
			// find next interesting value, which is either the whole step, or the transition to the next tile
			step_x = min((float)ceil(x) - x, _step_x);
			// if we are standing on the edge of a tile (ceil(x) - x == 0), we need to look one tile ahead
			if (step_x == 0) step_x	= min(1.f, _step_x);
		} else if (_step_x < 0) {
			step_x = max((float)floor(x) - x, _step_x);
			if (step_x == 0) step_x = max(-1.f, _step_x);
		}


		float step_y = 0;
		if (_step_y > 0) {
			step_y = min((float)ceil(y) - y, _step_y);
			if (step_y == 0) step_y	= min(1.f, _step_y);
		} else if (_step_y < 0) {
			step_y = max((float)floor(y) - y, _step_y);
			if (step_y == 0) step_y	= max(-1.f, _step_y);
		}

		_step_x -= step_x;
		_step_y -= step_y;

		bool diag = (step_x != 0) && (step_y != 0);

		if (is_valid_position(x + step_x, y + step_y, movement_type, is_hero)) {
			x += step_x;
			y += step_y;
		}
		else if ((diag || force_slide) && is_valid_position(x + step_x, y, movement_type, is_hero)) { // slide along wall
			x += step_x;
			if (!diag && force_slide) return true;
		}
		else if ((diag || force_slide)  && is_valid_position(x, y + step_y, movement_type, is_hero)) { // slide along wall
			y += step_y;
			if (!diag && force_slide) return true;
		}
		else {
			// is there a singular obstacle or corner we can step around?
			// only works if we are moving straight

			if (diag) return false;

			int way_around = is_one_step_around(x, y, round(step_x), round(step_y));

			if (!way_around)
				return false;

			if (round(step_x)) {
				y+= way_around;
			}
			else {
				x+= way_around;
			}
		}
	}
	return true;
}

/**
 * Determines whether the grid position is outside the map boundary
 */
bool MapCollision::is_outside_map(float tile_x, float tile_y) const {
	return (tile_x < 0 || tile_y < 0 || tile_x >= map_size.x || tile_y >= map_size.y);
}

/**
 * A map space is empty if it contains no blocking type
 * A position outside the map boundary is not empty
 */
bool MapCollision::is_empty(float x, float y) const {
	// map bounds check
	if (is_outside_map(x, y)) return false;

	// collision type check
	const int tile_x = floor(x);
	const int tile_y = floor(y);
	return (colmap[tile_x][tile_y] == BLOCKS_NONE);
}

/**
 * A map space is a wall if it contains a wall blocking type (normal or hidden)
 * A position outside the map boundary is a wall
 */
bool MapCollision::is_wall(int x, int y) const {

	// bounds check
	if (is_outside_map(x, y)) return true;

	// collision type check
	return (colmap[x][y] == BLOCKS_ALL || colmap[x][y] == BLOCKS_ALL_HIDDEN);
}

/**
 * Is this a valid tile for an entity with this movement type?
 */
bool MapCollision::is_valid_tile(int tile_x, int tile_y, MOVEMENTTYPE movement_type, bool is_hero) const {

	// outside the map isn't valid
	if (is_outside_map(tile_x,tile_y)) return false;

	if(is_hero && colmap[tile_x][tile_y] == BLOCKS_ENEMIES && !ENABLE_ALLY_COLLISION)
		return true;

	// occupied by an entity isn't valid
	if (colmap[tile_x][tile_y] == BLOCKS_ENTITIES) return false;

	// intangible creatures can be everywhere
	if (movement_type == MOVEMENT_INTANGIBLE) return true;

	// flying creatures can't be in walls
	if (movement_type == MOVEMENT_FLYING) {
		return (!(colmap[tile_x][tile_y] == BLOCKS_ALL || colmap[tile_x][tile_y] == BLOCKS_ALL_HIDDEN));
	}

	// normal creatures can only be in empty spaces
	return (colmap[tile_x][tile_y] == BLOCKS_NONE);
}

/**
 * Is this a valid position for an entity with this movement type?
 */
bool MapCollision::is_valid_position(float x, float y, MOVEMENTTYPE movement_type, bool is_hero) const {
	return is_valid_tile(floor(x), floor(y), movement_type, is_hero);
}

bool inline MapCollision::is_sidestepable(int tile_x, int tile_y, int offx, int offy) {
	return !is_outside_map(tile_x + offx, tile_y + offy) && !colmap[tile_x + offx][tile_y + offy];
}

/**
 * If we have encountered a collision (i.e., is_empty(x, y) already said no), then see if we've
 * hit an object/wall where there is a path around it by one step.  This is to avoid getting
 * "caught" on the corners of a jagged wall.
 *
 * @return if no side-step path exists, the return value is zero.  Otherwise,
 *         it is the coodinate modifier value for the opposite coordinate
 *         (i.e., if xdir was zero and ydir was non-zero, the return value
 *         should be applied to xdir)
 */
int MapCollision::is_one_step_around(float x, float y, int xdir, int ydir) {
	int tile_x = x;
	int tile_y = y;

	if (xdir) {
		if (is_sidestepable(tile_x, tile_y, xdir, -1) && (tile_x - x) > 0)
			return 1;
		if (is_sidestepable(tile_x, tile_y, xdir,  1) && (tile_x - x) < 0)
			return 2;
	}
	if (ydir) {
		if (is_sidestepable(tile_x, tile_y, -1, ydir) && (tile_y - y) > 0)
			return 1;
		if (is_sidestepable(tile_x, tile_y,  1, ydir) && (tile_y - y) < 0)
			return 2;
	}

	return 0;
}


/**
 * Does not have the "slide" submovement that move() features
 * Line can be arbitrary angles.
 */
bool MapCollision::line_check(int x1, int y1, int x2, int y2, int check_type, MOVEMENTTYPE movement_type) {
	float x = (float)x1;
	float y = (float)y1;
	float dx = (float)abs(x2 - x1);
	float dy = (float)abs(y2 - y1);
	float step_x;
	float step_y;
	int steps = (int)max(dx, dy);


	if (dx > dy) {
		step_x = 1;
		step_y = dy / dx;
	}
	else {
		step_y = 1;
		step_x = dx / dy;
	}
	// fix signs
	if (x1 > x2) step_x = -step_x;
	if (y1 > y2) step_y = -step_y;


	if (check_type == CHECK_SIGHT) {
		for (int i=0; i<steps; i++) {
			x += step_x;
			y += step_y;
			if (is_wall(round(x), round(y)))
				return false;
		}
	}
	else if (check_type == CHECK_MOVEMENT) {
		for (int i=0; i<steps; i++) {
			x += step_x;
			y += step_y;
			if (!is_valid_position(round(x), round(y), movement_type, false))
				return false;
		}
	}

	return true;
}

bool MapCollision::line_of_sight(int x1, int y1, int x2, int y2) {
	return line_check(x1, y1, x2, y2, CHECK_SIGHT, MOVEMENT_NORMAL);
}

bool MapCollision::line_of_movement(int x1, int y1, int x2, int y2, MOVEMENTTYPE movement_type) {

	// intangible entities can always move
	if (movement_type == MOVEMENT_INTANGIBLE) return true;

	// if the target is blocking, clear it temporarily
	int tile_x = x2;
	int tile_y = y2;
	bool target_blocks = false;
	int target_blocks_type = colmap[tile_x][tile_y];
	if (colmap[tile_x][tile_y] == BLOCKS_ENTITIES || colmap[tile_x][tile_y] == BLOCKS_ENEMIES) {
		target_blocks = true;
		unblock(x2,y2);
	}

	bool has_movement = line_check(x1, y1, x2, y2, CHECK_MOVEMENT, movement_type);

	if (target_blocks) block(x2,y2, target_blocks_type == BLOCKS_ENEMIES);
	return has_movement;

}

/**
 * Checks whether the entity in pos 1 is facing the point at pos 2
 * based on a 180 degree field of vision
 */
bool MapCollision::is_facing(int x1, int y1, char direction, int x2, int y2) {

	// 180 degree fov
	switch (direction) {
		case 2: //north west
			return ((x2-x1) < ((-1 * y2)-(-1 * y1))) && (((-1 * x2)-(-1 * x1)) > (y2-y1));
		case 3: //north
			return y2 < y1;
		case 4: //north east
			return (((-1 * x2)-(-1 * x1)) < ((-1 * y2)-(-1 * y1))) && ((x2-x1) > (y2-y1));
		case 5: //east
			return x2 > x1;
		case 6: //south east
			return ((x2-x1) > ((-1 * y2)-(-1 * y1))) && (((-1 * x2)-(-1 * x1)) < (y2-y1));
		case 7: //south
			return y2 > y1;
		case 0: //south west
			return (((-1 * x2)-(-1 * x1)) > ((-1 * y2)-(-1 * y1))) && ((x2-x1) < (y2-y1));
		case 1: //west
			return x2 < x1;
	}
	return false;
}

/**
* Compute a path from (x1,y1) to (x2,y2)
* Store waypoint inside path
* limit is the maximum number of explored node
* @return true if a path is found
*/
bool MapCollision::compute_path(FPoint start_pos, FPoint end_pos, vector<FPoint> &path, MOVEMENTTYPE movement_type, unsigned int limit) {

	if (limit == 0)
		limit = 256;

	// path must be empty
	if (!path.empty())
		path.clear();

	// convert start & end to MapCollision precision
	Point start = map_to_collision(start_pos);
	Point end = map_to_collision(end_pos);

	// if the target square has an entity, temporarily clear it to compute the path
	bool target_blocks = false;
	int target_blocks_type = colmap[end.x][end.y];
	if (colmap[end.x][end.y] == BLOCKS_ENTITIES || colmap[end.x][end.y] == BLOCKS_ENEMIES) {
		target_blocks = true;
		unblock(end_pos.x, end_pos.y);
	}

	Point current = start;
	AStarNode node(start);
	node.setActualCost(0);
	node.setEstimatedCost((float)calcDist(start,end));
	node.setParent(current);

	list<AStarNode> open;
	list<AStarNode> close;

	open.push_back(node);

	while (!open.empty() && close.size() < limit) {
		float lowest_score = FLT_MAX;
		// find lowest score available inside open, make it current node and move it to close
		list<AStarNode>::iterator lowest_it;
		for (list<AStarNode>::iterator it=open.begin(); it != open.end(); ++it) {
			if (it->getFinalCost() < lowest_score) {
				lowest_score = it->getFinalCost();
				lowest_it = it;
			}
		}
		node = *lowest_it;
		current.x = node.getX();
		current.y = node.getY();
		close.push_back(node);
		open.erase(lowest_it);

		if ( current.x == end.x && current.y == end.y)
			break; //path found !

		list<Point> neighbours = node.getNeighbours(256,256); //256 is map max size

		// for every neighbour of current node
		for (list<Point>::iterator it=neighbours.begin(); it != neighbours.end(); ++it)	{
			Point neighbour = *it;

			// if neighbour is not free of any collision, skip it
			if (!is_valid_tile(neighbour.x,neighbour.y,movement_type, false))
				continue;
			// if nabour is already in close, skip it
			if(find(close.begin(), close.end(), neighbour)!=close.end())
				continue;

			list<AStarNode>::iterator i = find(open.begin(), open.end(), neighbour);
			// if neighbour isn't inside open, add it as a new Node
			if (i==open.end()) {
				AStarNode newNode(neighbour.x,neighbour.y);
				newNode.setActualCost(node.getActualCost()+(float)calcDist(current,neighbour));
				newNode.setParent(current);
				newNode.setEstimatedCost((float)calcDist(neighbour,end));
				open.push_back(newNode);
			}
			// else, update it's cost if better
			else if (node.getActualCost()+(float)calcDist(current,neighbour) < i->getActualCost()) {
				i->setActualCost(node.getActualCost()+(float)calcDist(current,neighbour));
				i->setParent(current);
			}
		}
	}

	if (current.x != end.x || current.y != end.y) {

		// reblock target if needed
		if (target_blocks) block(end_pos.x, end_pos.y, target_blocks_type == BLOCKS_ENEMIES);

		float lowest_score = FLT_MAX;
		// find the closed node which is closest to the target and create a path
		list<AStarNode>::iterator lowest_it;
		for (list<AStarNode>::iterator it=close.begin(); it != close.end(); ++it) {
			if (it->getH() < lowest_score) {
				lowest_score = it->getH();
				lowest_it = it;
			}
		}
		node = *lowest_it;
		current.x = node.getX();
		current.y = node.getY();

		//couldnt find the target so map a path to the closest node found
		while (current.x != start.x || current.y != start.y) {
			path.push_back(collision_to_map(current));
			current = find(close.begin(), close.end(), current)->getParent();
		}

		return false;
	}
	else {
		// store path from end to start
		path.push_back(collision_to_map(end));
		while (current.x != start.x || current.y != start.y) {
			path.push_back(collision_to_map(current));
			current = find(close.begin(), close.end(), current)->getParent();
		}
	}

	// reblock target if needed
	if (target_blocks) block(end_pos.x, end_pos.y, target_blocks_type == BLOCKS_ENEMIES);

	return !path.empty();
}

void MapCollision::block(const int x, const int y, bool is_ally) {

	const int tile_x = x;
	const int tile_y = y;

	if (colmap[tile_x][tile_y] == BLOCKS_NONE) {
		if(is_ally)
			colmap[tile_x][tile_y] = BLOCKS_ENEMIES;
		else
			colmap[tile_x][tile_y] = BLOCKS_ENTITIES;
	}

}

void MapCollision::unblock(int x, int y) {

	const int tile_x = x;
	const int tile_y = y;

	if (colmap[tile_x][tile_y] == BLOCKS_ENTITIES || colmap[tile_x][tile_y] == BLOCKS_ENEMIES) {
		colmap[tile_x][tile_y] = BLOCKS_NONE;
	}

}

MapCollision::~MapCollision() {
}

