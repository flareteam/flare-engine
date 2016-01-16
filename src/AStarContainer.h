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

#include <vector>

#include "AStarNode.h"

typedef std::vector< std::vector<short> > AStar_Grid;

/* Designed to be used for the Open nodes.
*  Unsuitable for Closed nodes but a close node conatiner is declared below
*
*  All code in the class assumes that the nodes and points provided are within the bounds of the map limits
*/
class AStarContainer {
public:
	AStarContainer(unsigned int _map_width, unsigned int _map_height, unsigned int _node_limit);
	AStarContainer(const AStarContainer&); // copy constructor not yet implemented

	~AStarContainer();
	int getSize();
	//assumes that the node is not already in the collection
	void add(AStarNode* node);
	//assumes that there is at least 1 node in the collection
	AStarNode* get_shortest_f();
	//assumes that the node exists in the collection
	void remove(AStarNode* node);
	bool exists(const Point& pos);
	//assumes that the node exists in the collection
	AStarNode* get(int x, int y);
	bool isEmpty();
	void updateParent(const Point& pos, const Point& parent_pos, float score);

private:
	unsigned int size;
	unsigned int node_limit;
	unsigned int map_width;
	unsigned int map_height;

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
	std::vector<AStarNode*> nodes;

	/* This is a 2d array of shorts ([map_width][map_height]) which acts as an index for the main node array.
	*  Elements can be accessed using cartesian coordinates e.g. map_pos[x][y]
	*  To access an AStarNode based on map position use: nodes[map_pos[x][y]]
	*
	*  The data in this array is initialised as -1, which indicates hat there is no corresponding node for that position
	*  This must be maintained when nodes are added, removed and re-ordered in the node array
	*/
	AStar_Grid map_pos;
};

/* This class is used to store the closed list of a* nodes
*  The nodes within this class have no ordering but stil have a map position index
*/
class AStarCloseContainer {
public:
	AStarCloseContainer(unsigned int _map_width, unsigned int _map_height, unsigned int _node_limit);
	AStarCloseContainer(const AStarCloseContainer&); // copy constructor not yet implemented
	~AStarCloseContainer();

	int getSize();
	void add(AStarNode* node);
	bool exists(const Point& pos);
	AStarNode* get(int x, int y);
	AStarNode* get_shortest_h();

private:
	unsigned int size;
	unsigned int node_limit;
	unsigned int map_width;
	unsigned int map_height;
	std::vector<AStarNode*> nodes;
	AStar_Grid map_pos;

};

#endif // ASTARCONTAINER_H
