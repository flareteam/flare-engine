/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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

/*
 * MapCollision.h
 * RPGEngine
 *
 * Handle collisions between objects and the map
 */

#ifdef __EMSCRIPTEN__
// some of our movement code asserts fail on Emscripten builds
// since the game can still run with occasionally bugged movement, we can disable those asserts
#define NDEBUG
#endif

#include "AStarContainer.h"
#include "AStarNode.h"
#include "EngineSettings.h"
#include "MapCollision.h"
#include "SharedResources.h"

#include <cfloat>
#include <math.h>
#include <cassert>
#include <cstring>

// this value is used to determine the greatest possible position within a tile before transitioning to the next tile
// so if an entity has a position of (1-MIN_TILE_GAP, 0) and moves to the east, they will move to (1,0)
const float MapCollision::MIN_TILE_GAP = 0.001f;

MapCollision::MapCollision()
	: map_size(Point())
{
	colmap.resize(1);
	colmap[0].resize(1);
}

void MapCollision::setMap(const Map_Layer& _colmap, unsigned short w, unsigned short h) {
	colmap.resize(w);
	for (unsigned i=0; i<w; ++i) {
		colmap[i].resize(h);
	}
	for (unsigned i=0; i<w; i++)
		for (unsigned j=0; j<h; j++)
			colmap[i][j] = _colmap[i][j];

	map_size.x = w;
	map_size.y = h;
}

int sgn(float f) {
	if (f > 0)		return 1;
	else if (f < 0)	return -1;
	else			return 0;
}

bool MapCollision::smallStep(float &x, float &y, float step_x, float step_y, int movement_type, int collide_type) {
	if (isValidPosition(x + step_x, y + step_y, movement_type, collide_type)) {
		x += step_x;
		y += step_y;
		assert(isValidPosition(x,y,movement_type, collide_type));
		return true;
	}
	else {
		return false;
	}
}

bool MapCollision::smallStepForcedSlideAlongGrid(float &x, float &y, float step_x, float step_y, int movement_type, int collide_type) {
	if (isValidPosition(x + step_x, y, movement_type, collide_type)) { // slide along wall
		if (step_x == 0) return true;
		x += step_x;
		assert(isValidPosition(x,y,movement_type, collide_type));
	}
	else if (isValidPosition(x, y + step_y, movement_type, collide_type)) {
		if (step_y == 0) return true;
		y += step_y;
		assert(isValidPosition(x,y,movement_type, collide_type));
	}
	else {
		return false;
	}
	return true;
}

bool MapCollision::smallStepForcedSlide(float &x, float &y, float step_x, float step_y, int movement_type, int collide_type) {
	// is there a singular obstacle or corner we can step around?
	// only works if we are moving straight
	const float epsilon = 0.01f;
	if (step_x != 0) {
		assert(step_y == 0);
		float dy = y - floorf(y);

		if (isValidTile(int(x), int(y) + 1, movement_type, collide_type)
				&& isValidTile(int(x) + sgn(step_x), int(y) + 1, movement_type, collide_type)
				&& dy > 0.5) {
			y += std::min(1 - dy + epsilon, float(fabs(step_x)));
		}
		else if (isValidTile(int(x), int(y) - 1, movement_type, collide_type)
				 && isValidTile(int(x) + sgn(step_x), int(y) - 1, movement_type, collide_type)
				 && dy < 0.5) {
			y -= std::min(dy + epsilon, float(fabs(step_x)));
		}
		else {
			return false;
		}
		assert(isValidPosition(x,y,movement_type, collide_type));
	}
	else if (step_y != 0) {
		assert(step_x == 0);
		float dx = x - floorf(x);

		if (isValidTile(int(x) + 1, int(y), movement_type, collide_type)
				&& isValidTile(int(x) + 1, int(y) + sgn(step_y), movement_type, collide_type)
				&& dx > 0.5) {
			x += std::min(1 - dx + epsilon, float(fabs(step_y)));
		}
		else if (isValidTile(int(x) - 1, int(y), movement_type, collide_type)
				 && isValidTile(int(x) - 1, int(y) + sgn(step_y), movement_type, collide_type)
				 && dx < 0.5) {
			x -= std::min(dx + epsilon, float(fabs(step_y)));
		}
		else {
			return false;
		}
	}
	else {
		assert(false);
	}
	return true;
}

/**
 * Process movement for cardinal (90 degree) and ordinal (45 degree) directions
 * If we encounter an obstacle at 90 degrees, stop.
 * If we encounter an obstacle at 45 or 135 degrees, slide.
 */
bool MapCollision::move(float &x, float &y, float _step_x, float _step_y, int movement_type, int collide_type) {
	// when trying to slide against a bottom or right wall, step_x or step_y can become 0
	// this causes diag to become false, making this function return false
	// we try to catch such a scenario and return true early
	bool force_slide = (_step_x != 0 && _step_y != 0);

	while (_step_x != 0 || _step_y != 0) {

		float step_x = 0;
		if (_step_x > 0) {
			// find next interesting value, which is either the whole step, or the transition to the next tile
			step_x = std::min(ceilf(x) - x, _step_x);
			// if we are standing on the edge of a tile (ceilf(x) - x == 0), we need to look one tile ahead
			if (step_x <= MIN_TILE_GAP) step_x = std::min(1.f, _step_x);
		}
		else if (_step_x < 0) {
			step_x = std::max(floorf(x) - x, _step_x);
			if (step_x == 0) step_x = std::max(-1.f, _step_x);
		}

		float step_y = 0;
		if (_step_y > 0) {
			step_y = std::min(ceilf(y) - y, _step_y);
			if (step_y <= MIN_TILE_GAP) step_y = std::min(1.f, _step_y);
		}
		else if (_step_y < 0) {
			step_y = std::max(floorf(y) - y, _step_y);
			if (step_y == 0) step_y	= std::max(-1.f, _step_y);
		}

		_step_x -= step_x;
		_step_y -= step_y;

		if (!smallStep(x, y, step_x, step_y, movement_type, collide_type)) {
			if (force_slide) {
				if (!smallStepForcedSlideAlongGrid(x, y, step_x, step_y, movement_type, collide_type))
					return false;
			}
			else {
				if (!smallStepForcedSlide(x, y, step_x, step_y, movement_type, collide_type))
					return false;
			}
		}
	}
	return true;
}

/**
 * Determines whether the grid position is outside the map boundary
 */
bool MapCollision::isTileOutsideMap(const int& tile_x, const int& tile_y) const {
	return (tile_x < 0 || tile_y < 0 || tile_x >= map_size.x || tile_y >= map_size.y);
}

bool MapCollision::isOutsideMap(const float& tile_x, const float& tile_y) const {
	return isTileOutsideMap(static_cast<int>(tile_x), static_cast<int>(tile_y));
}

/**
 * A map space is empty if it contains no blocking type
 * A position outside the map boundary is not empty
 */
bool MapCollision::isEmpty(const float& x, const float& y) const {
	// map bounds check
	const int tile_x = static_cast<int>(x);
	const int tile_y = static_cast<int>(y);
	if (isTileOutsideMap(tile_x, tile_y)) return false;

	// collision type check
	return (colmap[tile_x][tile_y] == BLOCKS_NONE || colmap[tile_x][tile_y] == MAP_ONLY || colmap[tile_x][tile_y] == MAP_ONLY_ALT);
}

/**
 * A map space is a wall if it contains a wall blocking type (normal or hidden)
 * A position outside the map boundary is a wall
 */
bool MapCollision::isWall(const float& x, const float& y) const {
	// bounds check
	const int tile_x = static_cast<int>(x);
	const int tile_y = static_cast<int>(y);
	if (isTileOutsideMap(tile_x, tile_y)) return true;

	// collision type check
	return (colmap[tile_x][tile_y] == BLOCKS_ALL || colmap[tile_x][tile_y] == BLOCKS_ALL_HIDDEN);
}

/**
 * Is this a valid tile for an entity with this movement type?
 */
bool MapCollision::isValidTile(const int& tile_x, const int& tile_y, int movement_type, int collide_type) const {
	// outside the map isn't valid
	if (isTileOutsideMap(tile_x,tile_y)) return false;

	if (collide_type == COLLIDE_NORMAL) {
		if (colmap[tile_x][tile_y] == BLOCKS_ENEMIES)
			return false;
		if (colmap[tile_x][tile_y] == BLOCKS_ENTITIES)
			return false;
	}
	else if (collide_type == COLLIDE_HERO) {
		if (colmap[tile_x][tile_y] == BLOCKS_ENEMIES && !eset->misc.enable_ally_collision)
			return true;
	}

	// intangible creatures can be everywhere
	if (movement_type == MOVE_INTANGIBLE)
		return true;

	// flying creatures can't be in walls
	if (movement_type == MOVE_FLYING) {
		return (!(colmap[tile_x][tile_y] == BLOCKS_ALL || colmap[tile_x][tile_y] == BLOCKS_ALL_HIDDEN));
	}

	if (colmap[tile_x][tile_y] == MAP_ONLY || colmap[tile_x][tile_y] == MAP_ONLY_ALT)
		return true;

	// normal creatures can only be in empty spaces
	return (colmap[tile_x][tile_y] == BLOCKS_NONE);
}

/**
 * Is this a valid position for an entity with this movement type?
 */
bool MapCollision::isValidPosition(const float& x, const float& y, int movement_type, int collide_type) const {
	if (x < 0 || y < 0) return false;

	return isValidTile(int(x), int(y), movement_type, collide_type);
}

/**
 * Does not have the "slide" submovement that move() features
 * Line can be arbitrary angles.
 */
bool MapCollision::lineCheck(const float& x1, const float& y1, const float& x2, const float& y2, int check_type, int movement_type) {
	float x = x1;
	float y = y1;
	float dx = static_cast<float>(fabs(x2 - x1));
	float dy = static_cast<float>(fabs(y2 - y1));
	float step_x;
	float step_y;
	int steps = static_cast<int>(std::max(dx, dy));


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
			if (isWall(x, y))
				return false;
		}
	}
	else if (check_type == CHECK_MOVEMENT) {
		for (int i=0; i<steps; i++) {
			x += step_x;
			y += step_y;
			if (!isValidPosition(x, y, movement_type, COLLIDE_NORMAL))
				return false;
		}
	}

	return true;
}

bool MapCollision::lineOfSight(const float& x1, const float& y1, const float& x2, const float& y2) {
	return lineCheck(x1, y1, x2, y2, CHECK_SIGHT, MOVE_NORMAL);
}

bool MapCollision::lineOfMovement(const float& x1, const float& y1, const float& x2, const float& y2, int movement_type) {
	if (isOutsideMap(x2, y2)) return false;

	// intangible entities can always move
	if (movement_type == MOVE_INTANGIBLE) return true;

	// if the target is blocking, clear it temporarily
	int tile_x = int(x2);
	int tile_y = int(y2);
	bool target_blocks = false;
	int target_blocks_type = colmap[tile_x][tile_y];
	if (colmap[tile_x][tile_y] == BLOCKS_ENTITIES || colmap[tile_x][tile_y] == BLOCKS_ENEMIES) {
		target_blocks = true;
		unblock(x2,y2);
	}

	bool has_movement = lineCheck(x1, y1, x2, y2, CHECK_MOVEMENT, movement_type);

	if (target_blocks) block(x2,y2, target_blocks_type == BLOCKS_ENEMIES);
	return has_movement;

}

/**
 * Checks whether the entity in pos 1 is facing the point at pos 2
 * based on a 180 degree field of vision
 */
bool MapCollision::isFacing(const float& x1, const float& y1, char direction, const float& x2, const float& y2) {

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
bool MapCollision::computePath(const FPoint& start_pos, const FPoint& end_pos, std::vector<FPoint> &path, int movement_type, unsigned int limit) {

	if (isOutsideMap(end_pos.x, end_pos.y)) return false;

	// default limit set to 10% of the total map size
	if (limit == 0)
		limit = (map_size.x * map_size.y) / 10;

	// path must be empty
	if (!path.empty())
		path.clear();

	// convert start & end to MapCollision precision
	Point start(start_pos);
	Point end(end_pos);

	// if the target square has an entity, temporarily clear it to compute the path
	bool target_blocks = false;
	int target_blocks_type = colmap[end.x][end.y];
	if (colmap[end.x][end.y] == BLOCKS_ENTITIES || colmap[end.x][end.y] == BLOCKS_ENEMIES) {
		target_blocks = true;
		unblock(end_pos.x, end_pos.y);
	}

	Point current = start;
	AStarNode* node = new AStarNode(start);
	node->setActualCost(0);
	node->setEstimatedCost(Utils::calcDist(FPoint(start),FPoint(end)));
	node->setParent(current);

	AStarContainer open(map_size.x, map_size.y, limit);
	AStarCloseContainer close(map_size.x, map_size.y, limit);

	open.add(node);

	while (!open.isEmpty() && static_cast<unsigned>(close.getSize()) < limit) {
		node = open.get_shortest_f();

		current.x = node->getX();
		current.y = node->getY();
		close.add(node);
		open.remove(node);

		if ( current.x == end.x && current.y == end.y)
			break; //path found !

		//limit evaluated nodes to the size of the map
		std::list<Point> neighbours = node->getNeighbours(map_size.x, map_size.y);

		// for every neighbour of current node
		for (std::list<Point>::iterator it=neighbours.begin(); it != neighbours.end(); ++it)	{
			Point neighbour = *it;

			// do not exceed the node limit when adding nodes
			if (static_cast<unsigned>(open.getSize()) >= limit) {
				break;
			}

			// if neighbour is not free of any collision, skip it
			if (!isValidTile(neighbour.x,neighbour.y,movement_type, MapCollision::COLLIDE_NORMAL))
				continue;
			// if nabour is already in close, skip it
			if(close.exists(neighbour))
				continue;

			// if neighbour isn't inside open, add it as a new Node
			if(!open.exists(neighbour)) {
				AStarNode* newNode = new AStarNode(neighbour);
				newNode->setActualCost(node->getActualCost() + Utils::calcDist(FPoint(current),FPoint(neighbour)));
				newNode->setParent(current);
				newNode->setEstimatedCost(Utils::calcDist(FPoint(neighbour),FPoint(end)));
				open.add(newNode);
			}
			// else, update it's cost if better
			else {
				AStarNode* i = open.get(neighbour.x, neighbour.y);
				if (node->getActualCost() + Utils::calcDist(FPoint(current),FPoint(neighbour)) < i->getActualCost()) {
					Point pos(i->getX(), i->getY());
					Point parent_pos(node->getX(), node->getY());
					open.updateParent(pos, parent_pos, node->getActualCost() + Utils::calcDist(FPoint(current),FPoint(neighbour)));
				}
			}
		}
	}

	if (!(current.x == end.x && current.y == end.y)) {

		//couldnt find the target so map a path to the closest node found
		node = close.get_shortest_h();
		current.x = node->getX();
		current.y = node->getY();

		while (!(current.x == start.x && current.y == start.y)) {
			path.push_back(collisionToMap(current));
			current = close.get(current.x, current.y)->getParent();
		}
	}
	else {
		// store path from end to start
		path.push_back(collisionToMap(end));
		while (!(current.x == start.x && current.y == start.y)) {
			path.push_back(collisionToMap(current));
			current = close.get(current.x, current.y)->getParent();
		}
	}
	// reblock target if needed
	if (target_blocks) block(end_pos.x, end_pos.y, target_blocks_type == BLOCKS_ENEMIES);

	return !path.empty();
}

void MapCollision::block(const float& map_x, const float& map_y, bool is_ally) {
	const int tile_x = int(map_x);
	const int tile_y = int(map_y);

	if (isTileOutsideMap(tile_x, tile_y))
		return;

	if (colmap[tile_x][tile_y] == BLOCKS_NONE) {
		if(is_ally)
			colmap[tile_x][tile_y] = BLOCKS_ENEMIES;
		else
			colmap[tile_x][tile_y] = BLOCKS_ENTITIES;
	}

}

void MapCollision::unblock(const float& map_x, const float& map_y) {
	const int tile_x = int(map_x);
	const int tile_y = int(map_y);

	if (isTileOutsideMap(tile_x, tile_y))
		return;

	if (colmap[tile_x][tile_y] == BLOCKS_ENTITIES || colmap[tile_x][tile_y] == BLOCKS_ENEMIES) {
		colmap[tile_x][tile_y] = BLOCKS_NONE;
	}

}

/**
 * Given a target, trys to return one of the 8+ adjacent tiles
 * Returns the retargeted position on success, returns the original position on failure
 */
FPoint MapCollision::getRandomNeighbor(const Point& target, int range, bool ignore_blocked) {
	FPoint new_target(target);
	std::vector<FPoint> valid_tiles;

	for (int i=-range; i<=range; i++) {
		for (int j=-range; j<=range; j++) {
			if (i == 0 && j == 0) continue; // skip the middle tile
			new_target.x = static_cast<float>(target.x + i) + 0.5f;
			new_target.y = static_cast<float>(target.y + j) + 0.5f;
			if (isValidPosition(new_target.x, new_target.y, MOVE_NORMAL, COLLIDE_NORMAL) || ignore_blocked)
				valid_tiles.push_back(new_target);
		}
	}

	if (!valid_tiles.empty())
		return valid_tiles[rand() % valid_tiles.size()];
	else
		return FPoint(target);
}

FPoint MapCollision::collisionToMap(const Point& p) {
	FPoint ret;
	ret.x = static_cast<float>(p.x) + 0.5f;
	ret.y = static_cast<float>(p.y) + 0.5f;
	return ret;
}

MapCollision::~MapCollision() {
}

// re-enable asserts in other files
#ifdef __EMSCRIPTEN__
#undef NDEBUG
#endif
