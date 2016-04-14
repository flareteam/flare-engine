/*
Copyright © 2013 Ryan Dansie
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

#include "AStarContainer.h"
#include <cstring>
#include <cfloat>

AStarContainer::AStarContainer(unsigned int _map_width, unsigned int _map_height, unsigned int _node_limit)
	: size(0)
	, node_limit(_node_limit)
	, map_width(_map_width)
	, map_height(_map_height)
{
	nodes.resize(node_limit, NULL);

	//initialise the map array. A -1 value will mean there is no node at that position
	map_pos.resize(map_width);
	for (unsigned i=0; i<map_width; ++i) {
		map_pos[i].resize(map_height, -1);
	}
}

AStarContainer::~AStarContainer() {
	for(unsigned int i=0; i<size; i++)
		delete nodes[i];
	nodes.clear();
}

int AStarContainer::getSize() {
	return size;
}

void AStarContainer::add(AStarNode* node) {
	if (size >= node_limit) return;

	//add the new node at the end and update its index
	nodes[size] = node;
	map_pos[node->getX()][node->getY()] = static_cast<short>(size);

	//reorder the heap based on f ordering, staring with thenewly added node and working up the tree from there
	int m = size;

	AStarNode* temp = NULL;
	while(m != 0) {
		//if the current nodes f value is shorter than its parent, they need to be swapped
		if(nodes[m]->getFinalCost() <= nodes[m/2]->getFinalCost()) {
			temp = nodes[m/2];
			nodes[m/2] = nodes[m];
			map_pos[nodes[m/2]->getX()][nodes[m/2]->getY()] = static_cast<short>(m/2);
			nodes[m] = temp;
			map_pos[nodes[m]->getX()][nodes[m]->getY()] = static_cast<short>(m);
			m=m/2;
		}
		else
			break;
	}
	size++;
}

AStarNode* AStarContainer::get_shortest_f() {
	return nodes[0];
}

void AStarContainer::remove(AStarNode* node) {

	unsigned int heap_indexv = map_pos[node->getX()][node->getY()] + 1;

	//swap the last node in the list with the node being deleted
	nodes[heap_indexv-1] = nodes[size-1];
	map_pos[nodes[heap_indexv-1]->getX()][nodes[heap_indexv-1]->getY()] = static_cast<short>(heap_indexv-1);

	size--;

	if(size == 0) {
		map_pos[node->getX()][node->getY()] = -1;
		return;
	}

	// reorder the heap to maintain the f ordering, starting at the node which replaced the deleted node, and working down the tree

	while(true) {
		//start at the node which dropped down the tree on the previous iteration
		unsigned int heap_indexu = heap_indexv;
		if(2*heap_indexu+1 <= size) { //if both children exist
			//Select the lowest of the two children.
			if(nodes[heap_indexu-1]->getFinalCost() >= nodes[2*heap_indexu-1]->getFinalCost()) heap_indexv = 2*heap_indexu;
			if(nodes[heap_indexv-1]->getFinalCost() >= nodes[2*heap_indexu]->getFinalCost()) heap_indexv = 2*heap_indexu+1;
		}
		else if (2*heap_indexu <= size) { //if only child #1 exists
			//Check if the F cost is greater than the child
			if(nodes[heap_indexu-1]->getFinalCost() >= nodes[2*heap_indexu-1]->getFinalCost()) heap_indexv = 2*heap_indexu;
		}

		if(heap_indexu != heap_indexv) { //If parent's F > one or both of its children, swap them
			AStarNode* temp = nodes[heap_indexu-1];
			nodes[heap_indexu-1] = nodes[heap_indexv-1];
			map_pos[nodes[heap_indexu-1]->getX()][nodes[heap_indexu-1]->getY()] = static_cast<short>(heap_indexu-1);
			nodes[heap_indexv-1] = temp;
			map_pos[nodes[heap_indexv-1]->getX()][nodes[heap_indexv-1]->getY()] = static_cast<short>(heap_indexv-1);
		}
		else {
			break;//if item <= both children, exit loop
		}
	}//Repeat forever

	//remove the node from the map pos index
	map_pos[node->getX()][node->getY()] = -1;
}

bool AStarContainer::exists(const Point& pos) {
	return map_pos[pos.x][pos.y] != -1;
}

AStarNode* AStarContainer::get(int x, int y) {
	return nodes[map_pos[x][y]];
}

bool AStarContainer::isEmpty() {
	return size == 0;
}

void AStarContainer::updateParent(const Point& pos, const Point& parent_pos, float score) {
	get(pos.x, pos.y)->setParent(parent_pos);
	get(pos.x, pos.y)->setActualCost(score);

	//reorder the heap based on the new f value of this node. starting at the updated node and working up the tree
	int m = map_pos[pos.x][pos.y];
	AStarNode* temp = NULL;
	while(m != 0) {
		//if the current node has a lower f value than its parent in the heap, swap them
		if(nodes[m]->getFinalCost() <= nodes[m/2]->getFinalCost()) {
			temp = nodes[m/2];
			nodes[m/2] = nodes[m];
			map_pos[nodes[m/2]->getX()][nodes[m/2]->getY()] = static_cast<short>(m/2);
			nodes[m] = temp;
			map_pos[nodes[m]->getX()][nodes[m]->getY()] = static_cast<short>(m);
			m=m/2;
		}
		else
			break;
	}
}

AStarCloseContainer::AStarCloseContainer(unsigned int _map_width, unsigned int _map_height, unsigned int _node_limit)
	: size(0)
	, node_limit(_node_limit)
	, map_width(_map_width)
	, map_height(_map_height)
{
	nodes.resize(node_limit, NULL);

	//initialise the map array. A -1 value will mean there is no node at that position
	map_pos.resize(map_width);
	for (unsigned i=0; i<map_width; ++i) {
		map_pos[i].resize(map_height, -1);
	}
}

AStarCloseContainer::~AStarCloseContainer() {
	for(unsigned int i=0; i<size; i++)
		delete nodes[i];
	nodes.clear();
}

int AStarCloseContainer::getSize() {
	return size;
}

void AStarCloseContainer::add(AStarNode* node) {
	if (size >= node_limit) return;

	nodes[size] = node;
	map_pos[node->getX()][node->getY()] = static_cast<short>(size);
	size++;
}

bool AStarCloseContainer::exists(const Point& pos) {
	return map_pos[pos.x][pos.y] != -1;
}

AStarNode* AStarCloseContainer::get(int x, int y) {
	return nodes[map_pos[x][y]];
}

AStarNode* AStarCloseContainer::get_shortest_h() {
	AStarNode *current = NULL;
	float lowest_score = FLT_MAX;
	for(unsigned int i = 0; i < size; i++) {
		if(nodes[i]->getH() < lowest_score) {
			lowest_score = nodes[i]->getH();
			current = nodes[i];
		}
	}
	return current;
}
