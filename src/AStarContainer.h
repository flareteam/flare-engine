/*
Copyright © 2013 Ryan Dansie

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

#ifndef ASTARCONTAINER_H
#define ASTARCONTAINER_H

#include "AStarNode.h"

/* Designed to be used for the Open nodes.
*  Unsuitable for Closed nodes but a close node conatiner is declared below
*
*  All code in the class assumes that the nodes and points provided are within the bounds of the map limits
*/
typedef short astar_mapcol[256];
class AStarContainer {
public:
	AStarContainer(unsigned int _map_width, unsigned int node_limit);
	AStarContainer(const AStarContainer&); // copy constructor not yet implemented

	~AStarContainer();
	//assumes that the node is not already in the collection
	void add(AStarNode* node);
	//assumes that there is at least 1 node in the collection
	AStarNode* get_shortest_f();
	//assumes that the node exists in the collection
	void remove(AStarNode* node);
	bool exists(Point pos);
	//assumes that the node exists in the collection
	AStarNode* get(int x, int y);
	bool isEmpty();
	void updateParent(Point pos, Point parent_pos, float score);

private:
	unsigned int size;

	/* This is an array of AStarNode pointers. This is the main data for this collection.
	*  The size of the array is based on the node limit.
	*
	*  The nodes in this array are ordered based on their f value and the node with the lowest f value is always at position 0.
	*  The ordering is not linear, so after positon 0, we cannot assume that position 1 has the second shortest f value.
	*
	*  The ordering is based on a binary heap structure.
	*  Essentially, each node can have up to 2 child nodes and each child node must have a lower f value than its parent.
	*  This rule must be maintained whenever nodes are added or removed or their f value changes
	*
	*  The tree is represented by a 1 dimentional array where position 0 has children at position 1 and 2
	*  Node 1 would have children at position 3 and 4 and node 2 would have children at position 5 and 6 and so on
	*
	*  A more detailed explanation of the structure can be found at the below web address.
	*  Please note that there is additional code found in this class which is not mentioned in the article. This is to provide an index based on map position.
	*  Also note that the code within the article is based on arrays with starting position 1, whereas we use 0 based arrays.
	*  http://www.policyalmanac.org/games/binaryHeaps.htm
	*/
	AStarNode** nodes;

	/* This is an array of astar_mapcol (short[256]) which acts as an index for the main node array.
	*  Its essentially a 2d array: short[map_width][256],
	*  so elements can be accessed using cartesian coordinates e.g. map_pos[x][y]
	*  To access an AStarNode based on map position use: nodes[map_pos[x][y]]
	*
	*  The map height is always set to 256 even if the actual map is smaller.
	*  This helps with initialisation and potentially allows the compiler to optimise array access.
	*
	*  The data in this array is initialised as -1, which indicates hat there is no corresponding node for that position
	*  This must be maintained when nodes are added, removed and re-ordered in the node array
	*/
	astar_mapcol* map_pos;
};

/* This class is used to store the closed list of a* nodes
*  The nodes within this class have no ordering but stil have a map position index
*/
class AStarCloseContainer {
public:
	AStarCloseContainer(unsigned int _map_width, unsigned int node_limit);
	AStarCloseContainer(const AStarCloseContainer&); // copy constructor not yet implemented
	~AStarCloseContainer();

	int getSize();
	void add(AStarNode* node);
	bool exists(Point pos);
	AStarNode* get(int x, int y);
	AStarNode* get_shortest_h();

private:
	unsigned int size;
	AStarNode** nodes;
	astar_mapcol* map_pos;

};

#endif // ASTARCONTAINER_H
