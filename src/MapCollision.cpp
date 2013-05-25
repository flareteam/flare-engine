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
#include "AStarContainer.h"
#include <cfloat>

#include <time.h>

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
bool MapCollision::move(int &x, int &y, int step_x, int step_y, int dist, MOVEMENTTYPE movement_type, bool is_hero) {

	bool diag = step_x && step_y;

	for (int i = dist; i--;) {
		if (is_valid_position(x + step_x, y + step_y, movement_type, is_hero)) {
			x+= step_x;
			y+= step_y;
		}
		else if (diag && is_valid_position(x + step_x, y, movement_type, is_hero)) { // slide along wall
			x+= step_x;
		}
		else if (diag && is_valid_position(x, y + step_y, movement_type, is_hero)) { // slide along wall
			y+= step_y;
		}
		else { // is there a singular obstacle or corner we can step around?
			// only works if we are moving straight
			if (diag) return false;

			int way_around = is_one_step_around(x, y, step_x, step_y);

			if (!way_around) {
				return false;
			}

			if (step_x) {
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
bool MapCollision::is_outside_map(int tile_x, int tile_y) const {
	return (tile_x < 0 || tile_y < 0 || tile_x >= map_size.x || tile_y >= map_size.y);
}

/**
 * A map space is empty if it contains no blocking type
 * A position outside the map boundary is not empty
 */
bool MapCollision::is_empty(int x, int y) const {
	int tile_x = x >> TILE_SHIFT; // fast div
	int tile_y = y >> TILE_SHIFT; // fast div

	// bounds check
	if (is_outside_map(tile_x, tile_y)) return false;

	// collision type check
	return (colmap[tile_x][tile_y] == BLOCKS_NONE);
}

/**
 * A map space is a wall if it contains a wall blocking type (normal or hidden)
 * A position outside the map boundary is a wall
 */
bool MapCollision::is_wall(int x, int y) const {
	int tile_x = x >> TILE_SHIFT; // fast div
	int tile_y = y >> TILE_SHIFT; // fast div

	// bounds check
	if (is_outside_map(tile_x, tile_y)) return true;

	// collision type check
	return (colmap[tile_x][tile_y] == BLOCKS_ALL || colmap[tile_x][tile_y] == BLOCKS_ALL_HIDDEN);
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
bool MapCollision::is_valid_position(int x, int y, MOVEMENTTYPE movement_type, bool is_hero) const {

	const int tile_x = x >> TILE_SHIFT; // fast div
	const int tile_y = y >> TILE_SHIFT; // fast div

	return is_valid_tile(tile_x, tile_y, movement_type, is_hero);
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
int MapCollision::is_one_step_around(int x, int y, int xdir, int ydir) {
	int tile_x = x >> TILE_SHIFT; // fast div
	int tile_y = y >> TILE_SHIFT; // fast div
	int ret = 0;

	if (xdir) {
		if (is_sidestepable(tile_x, tile_y, xdir, -1)) {
			ret = 1;
		}
		if (is_sidestepable(tile_x, tile_y, xdir,  1)) {
			ret |= 2;
		}
		if (ret == 3) { // If we can go either way, choose the route that shortest

			// translation: ret = y % UNITS_PER_TILE > UNITS_PER_TILE / 2 ? 1 : -1;
			// realistically, if we were using compile time constants, the compiler
			// would generate pretty much those instructions.
			ret = (y & (UNITS_PER_TILE - 1)) < UNITS_PER_TILE >> 1 ? 1 : -1;
		}
	}
	else {
		if (is_sidestepable(tile_x, tile_y, -1, ydir)) {
			ret = 1;
		}
		if (is_sidestepable(tile_x, tile_y,  1, ydir)) {
			ret |= 2;
		}
		if (ret == 3) {
			ret = (x & (UNITS_PER_TILE - 1)) < UNITS_PER_TILE >> 1 ? 1 : -1;
		}
	}

	return !ret ? 0 : (ret == 1 ? -1 : 1);
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
	int tile_x = x2 >> TILE_SHIFT;
	int tile_y = y2 >> TILE_SHIFT;
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
bool MapCollision::compute_path(Point start_pos, Point end_pos, vector<Point> &path, MOVEMENTTYPE movement_type, unsigned int limit) {

clock_t timeBegin = clock();
clock_t timePreConstructor = 0;
clock_t timePostConstructor = 0;
clock_t timePreMainLoop = 0;
clock_t timePostMainLoop = 0;
clock_t timeEnd = 0;

clock_t preFirstHalf = 0;
clock_t postFirstHalf = 0;
clock_t postSecondHalf = 0;
int firstHalfTotal = 0;
int secondHalfTotal = 0;

clock_t preRemove = 0;
clock_t postRemove = 0;
int removeTotal = 0;

int firstQuarterTotal = 0;
int secondQuarterTotal = 0;

clock_t postGetShortest = 0;
int shortestTotal = 0;

clock_t preAddClose = 0;
int addCloseTotal = 0;

clock_t preValidTile = 0;
clock_t postValidTile = 0;
clock_t postSearchClose = 0;

int validTileTotal = 0;
int searchCloseTotal = 0;

clock_t preIfExists = 0;
clock_t postIfExists = 0;
int ifExistsTotal = 0;

clock_t preIfNotExists = 0;
clock_t postIfNotExists = 0;
int ifNotExistsTotal = 0;

	if (limit == 0)
		limit = 256;

    //original:600
    //with new collection:
    limit = 1800;

	// path must be empty
	if (!path.empty())
		path.clear();


timePreConstructor = clock();
	// convert start & end to MapCollision precision
	Point start = map_to_collision(start_pos);
timePostConstructor = clock();

	Point end = map_to_collision(end_pos);

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
	node->setEstimatedCost((float)calcDist(start,end));
	node->setParent(current);

	AStarContainer open(map_size.x, map_size.y, limit);
	AStarCloseContainer close(map_size.x, map_size.y, limit);
	///list<AStarNode> close;

    open.add(node);
	///open.push_back(node);

timePreMainLoop = clock();
	while (!open.isEmpty() && close.getSize() < limit) {

preFirstHalf = clock();
		/**float lowest_score = FLT_MAX;
		// find lowest score available inside open, make it current node and move it to close
		list<AStarNode>::iterator lowest_it;
		for (list<AStarNode>::iterator it=open.begin(); it != open.end(); ++it) {
			if (it->getFinalCost() < lowest_score) {
				lowest_score = it->getFinalCost();
				lowest_it = it;
			}
		}*/

		AStarNode* lowest_it = open.get_shortest_f();
postGetShortest = clock();
shortestTotal += postGetShortest - preFirstHalf;

		node = lowest_it;
		current.x = node->getX();
		current.y = node->getY();
preAddClose = clock();
        close.add(node);
		///close.push_back(*node);
		///open.erase(lowest_it);
preRemove = clock();
firstQuarterTotal += preRemove - preFirstHalf;
addCloseTotal += preRemove - preAddClose;
if(open.get_shortest_f()->x < 0 || open.get_shortest_f()->x > 256 || open.get_shortest_f()->y < 0 || open.get_shortest_f()->y > 256)
{
    int x;
    postRemove = clock();
    x = 1;
}
int presize = open.size;
AStarNode* node1 = open.nodes[0];
int heap_indexv = open.map_pos[node1->getX() + node1->getY() * map_size.x] + 1;
AStarNode* node2 = open.nodes[1];
AStarNode* node3 = open.nodes[2];
AStarNode* node4 = open.nodes[3];
AStarNode* node5 = open.nodes[4];

		open.remove(lowest_it);
if(!open.isEmpty())
    if(open.get_shortest_f()->x < 0 || open.get_shortest_f()->x > 256 || open.get_shortest_f()->y < 0 || open.get_shortest_f()->y > 256)
{
    int x;
    x = 0;
    x = 1;
    postRemove = clock();
}

if(!open.isEmpty())
if(node == open.get_shortest_f())
{
    int i = open.size;
    postRemove = clock();
}

if(open.size > 0)
if(open.map_pos[open.nodes[0]->getX() + open.nodes[0]->getY() * map_size.x] != 0){
int presize2 = open.size;
AStarNode* node12 = open.nodes[0];
int heap_indexv2 = open.map_pos[node12->getX() + node12->getY() * map_size.x] + 1;
AStarNode* node22 = open.nodes[1];
AStarNode* node32 = open.nodes[2];
AStarNode* node42 = open.nodes[3];
AStarNode* node52 = open.nodes[4];
    postRemove = clock();
}

postRemove = clock();
removeTotal += postRemove - preRemove;

		if ( current.x == end.x && current.y == end.y){
            ///delete node;
			break; //path found !
		}

		list<Point> neighbours = node->getNeighbours(256,256); //256 is map max size
///(map_size.x-1, map_size.y-1);///

postFirstHalf = clock();
firstHalfTotal += postFirstHalf - preFirstHalf;
secondQuarterTotal += postFirstHalf - postRemove;

		// for every neighbour of current node
		for (list<Point>::iterator it=neighbours.begin(); it != neighbours.end(); ++it)	{
			Point neighbour = *it;

if(neighbour.x > map_size.x)
    postRemove = clock();
if(neighbour.y > map_size.y)
    postRemove = clock();


preValidTile = clock();
			// if neighbour is not free of any collision, skip it
			if (!is_valid_tile(neighbour.x,neighbour.y,movement_type, false))
				continue;
postValidTile = clock();
validTileTotal += postValidTile - preValidTile;
			// if nabour is already in close, skip it
			if(close.exists(neighbour))
                continue;
			///if(find(close.begin(), close.end(), neighbour)!=close.end())
				///continue;
postSearchClose = clock();
searchCloseTotal += postSearchClose - postValidTile;

			///list<AStarNode>::iterator i = find(open.begin(), open.end(), neighbour);

			// if neighbour isn't inside open, add it as a new Node
			///if (i==open.end()) {
			if(!open.exists(neighbour)){
preIfExists = clock();
				AStarNode* newNode = new AStarNode(neighbour.x,neighbour.y);
				newNode->setActualCost(node->getActualCost()+(float)calcDist(current,neighbour));
				newNode->setParent(current);
				newNode->setEstimatedCost((float)calcDist(neighbour,end));
				///open.push_back(newNode);
				open.add(newNode);
/*if(!open.isEmpty())
if(open.get_shortest_f()->x < 0 || open.get_shortest_f()->x > 256 || open.get_shortest_f()->y < 0 || open.get_shortest_f()->y > 256)
{
    int x;
    postRemove = clock();
    x = 1;
}

if(open.size > 1)
if(open.nodes[0] == open.nodes[1]){
    int dfskv = open.size;
    postRemove = clock();
}

if(open.map_pos[open.nodes[0]->getX() + open.nodes[0]->getY() * map_size.x] != 0){
    postRemove = clock();
}
*/
postIfExists = clock();
ifExistsTotal += postIfExists - preIfExists;
			}
			// else, update it's cost if better
			else{
preIfNotExists = clock();
                AStarNode* i = open.get(neighbour.x, neighbour.y);

if(i->x < 0 || i->x > 256 || i->y < 0 || i->y > 256)
{
    int x;
    postRemove = clock();
    x = 1;
}

                if (node->getActualCost()+(float)calcDist(current,neighbour) < i->getActualCost()) {
                    Point pos(i->getX(), i->getY());
                    Point parent_pos(node->getX(), node->getY());
                    open.updateParent(pos, parent_pos, node->getActualCost()+(float)calcDist(current,neighbour));
                    ///i->setActualCost(node.getActualCost()+(float)calcDist(current,neighbour));
                    ///i->setParent(current);
if(open.get_shortest_f()->x < 0 || open.get_shortest_f()->x > 256 || open.get_shortest_f()->y < 0 || open.get_shortest_f()->y > 256)
{
    int x;
    postRemove = clock();
    x = 1;
}

if(open.size > 1)
if(open.nodes[0] == open.nodes[1]){
    int dfskv = open.size;
    postRemove = clock();
}

if(open.map_pos[open.nodes[0]->getX() + open.nodes[0]->getY() * map_size.x] != 0){
    postRemove = clock();
}

                }
postIfNotExists = clock();
ifNotExistsTotal += postIfNotExists - preIfNotExists;
			}
		}
if(!open.isEmpty())
if(open.get_shortest_f()->x < 0 || open.get_shortest_f()->x > 256 || open.get_shortest_f()->y < 0 || open.get_shortest_f()->y > 256)
{
    int x;
    postRemove = clock();
    x = 1;
}
		///delete node;
if(!open.isEmpty())
if(open.get_shortest_f()->x < 0 || open.get_shortest_f()->x > 256 || open.get_shortest_f()->y < 0 || open.get_shortest_f()->y > 256)
{
    int x;
    postRemove = clock();
    x = 1;
}
postSecondHalf = clock();
secondHalfTotal += postSecondHalf - postFirstHalf;
	}
timePostMainLoop = clock();

	if (current.x != end.x || current.y != end.y) {

		// reblock target if needed
		if (target_blocks) block(end_pos.x, end_pos.y, target_blocks_type == BLOCKS_ENEMIES);

		///float lowest_score = FLT_MAX;
		// find the closed node which is closest to the target and create a path
		///list<AStarNode>::iterator lowest_it;
		///for (list<AStarNode>::iterator it=close.begin(); it != close.end(); ++it) {
			///if (it->getH() < lowest_score) {
				///lowest_score = it->getH();
				///lowest_it = it;
			///}
		///}

		node = close.get_shortest_h();

		///node = &(*lowest_it);
		current.x = node->getX();
		current.y = node->getY();

		//couldnt find the target so map a path to the closest node found
		while (current.x != start.x || current.y != start.y) {
			path.push_back(collision_to_map(current));
			current = close.get(current.x, current.y)->getParent();/// find(close.begin(), close.end(), current)->getParent();
		}

		///return false;
	}
	else {
		// store path from end to start
		path.push_back(collision_to_map(end));
		while (current.x != start.x || current.y != start.y) {
			path.push_back(collision_to_map(current));
			current = close.get(current.x, current.y)->getParent();///current = find(close.begin(), close.end(), current)->getParent();
		}
	}

	// reblock target if needed
	if (target_blocks) block(end_pos.x, end_pos.y, target_blocks_type == BLOCKS_ENEMIES);

timeEnd = clock();

fprintf(stderr, "--------------------------------\n");
fprintf(stderr, "total time: %d\n", timeEnd - timeBegin);
fprintf(stderr, "time to constructor: %d\n", timePreConstructor - timeBegin);
fprintf(stderr, "time in constructor: %d\n", timePostConstructor - timePreConstructor);
fprintf(stderr, "time to main loop: %d\n", timePreMainLoop - timePostConstructor);
fprintf(stderr, "time in main loop: %d\n", timePostMainLoop - timePreMainLoop);
fprintf(stderr, "time to end: %d\n", timeEnd - timePostMainLoop);
fprintf(stderr, "first half: %d\n", firstHalfTotal);
fprintf(stderr, "second half: %d\n", secondHalfTotal);
fprintf(stderr, "remove: %d\n", removeTotal);
fprintf(stderr, "first quarter: %d\n", firstQuarterTotal);
fprintf(stderr, "second quarter: %d\n", secondQuarterTotal);
fprintf(stderr, "get shortest: %d\n", shortestTotal);
fprintf(stderr, "add close: %d\n", addCloseTotal);
fprintf(stderr, "is valid tile: %d\n", validTileTotal);
fprintf(stderr, "search close: %d\n", searchCloseTotal);
fprintf(stderr, "if exists: %d\n", ifExistsTotal);
fprintf(stderr, "if not exists: %d\n", ifNotExistsTotal);



	return !path.empty();
}

void MapCollision::block(const int x, const int y, bool is_ally) {

	const int tile_x = x >> TILE_SHIFT; // fast div
	const int tile_y = y >> TILE_SHIFT; // fast div

	if (colmap[tile_x][tile_y] == BLOCKS_NONE) {
		if(is_ally)
			colmap[tile_x][tile_y] = BLOCKS_ENEMIES;
		else
			colmap[tile_x][tile_y] = BLOCKS_ENTITIES;
	}

}

void MapCollision::unblock(int x, int y) {

	const int tile_x = x >> TILE_SHIFT; // fast div
	const int tile_y = y >> TILE_SHIFT; // fast div

	if (colmap[tile_x][tile_y] == BLOCKS_ENTITIES || colmap[tile_x][tile_y] == BLOCKS_ENEMIES) {
		colmap[tile_x][tile_y] = BLOCKS_NONE;
	}

}

MapCollision::~MapCollision() {
}

