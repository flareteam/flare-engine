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


    ///private:

        unsigned int size;

        unsigned int map_width;
        //AStarNode array -- binary heap ordered
        AStarNode** nodes;

        //2d array linking map pos to the index needed for the nodes array
        int* map_pos;
};

#endif // ASTARCONTAINER_H
