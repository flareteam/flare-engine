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

//designed to be used for the Open nodes. Unsuitable for Closed nodes
class AStarContainer
{
    public:
        AStarContainer(unsigned int map_width, unsigned int map_height, unsigned int node_limit);
        ~AStarContainer();
        //assumes that the node is not already in the collection
        void add(AStarNode* node);
        AStarNode* get_shortest_f();
        //assumes that the node exists in the collection
        void remove(AStarNode* node);
        bool exists(Point pos);
        //assumes that the node exists in the collection
        AStarNode* get(int x, int y);
        bool isEmpty();
        //sorts the heap based on updated pos
        void updateParent(Point pos, Point parent_pos, float score);


    private:

        unsigned int size;

        unsigned int map_width;
        //AStarNode array -- binary heap ordered
        AStarNode** nodes;

        //2d array linking map pos to the index needed for the nodes array
        int* map_pos;
};

class AStarCloseContainer
{
public:
    AStarCloseContainer(unsigned int map_width, unsigned int map_height, unsigned int node_limit);
    ~AStarCloseContainer();

    int getSize();
    void add(AStarNode* node);
    bool exists(Point pos);
    AStarNode* get(int x, int y);
    AStarNode* get_shortest_h();

private:

    unsigned int size;
    unsigned int map_width;
    AStarNode** nodes;
    //2d array linking map pos to the index needed for the nodes array
    int* map_pos;

};

#endif // ASTARCONTAINER_H
