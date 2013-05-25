#include "AStarContainer.h"
#include <cstring>
#include <cfloat>

AStarContainer::AStarContainer(unsigned int map_width, unsigned int map_height, unsigned int node_limit)
    : size(0)
    , map_width(0)
{
    this->map_width = map_width;

    nodes = new AStarNode*[node_limit];
    map_pos = new int[map_width * map_height + 1];

    //initialise the map array. A -1 value will mean there is no node at that position
    ///std::memset(map_pos, -1, sizeof(int) * (map_width + map_height));
    std::fill(map_pos, map_pos + map_width * map_height + 1, -1);
}

AStarContainer::~AStarContainer()
{
    for(int i=0; i<size; i++)
        delete nodes[i];
    delete [] nodes;
    delete [] map_pos;
}

void AStarContainer::add(AStarNode* node){
    nodes[size] = node;

    map_pos[node->getX() + node->getY() * map_width] = size;

    //reorder the heap
    int m = size;

    AStarNode* temp = NULL;
    while(m != 0){
        if(nodes[m]->getFinalCost() <= nodes[m/2]->getFinalCost()){
            temp = nodes[m/2];
            nodes[m/2] = nodes[m];
            map_pos[nodes[m/2]->getX() + nodes[m/2]->getY() * map_width] = m/2;
            nodes[m] = temp;
            map_pos[nodes[m]->getX() + nodes[m]->getY() * map_width] = m;
            m=m/2;
        }
        else
            break;
    }
    size++;
}

AStarNode* AStarContainer::get_shortest_f(){
    return nodes[0];
}

void AStarContainer::remove(AStarNode* node){

    int heap_indexv = map_pos[node->getX() + node->getY() * map_width] + 1;

    //swap the last node in the list with the node being deleted
    nodes[heap_indexv-1] = nodes[size-1];
    map_pos[nodes[heap_indexv-1]->getX() + nodes[heap_indexv-1]->getY() * map_width] = heap_indexv-1;

    size--;

    if(size == 0){
        map_pos[node->getX() + node->getY() * map_width] = -1;
        return;
    }

    //reorder the heap
    int heap_indexu = heap_indexv;

    while(true){
        heap_indexu = heap_indexv;
        if(2*heap_indexu+1 <= size){//if both children exist
            //Select the lowest of the two children.
            if(nodes[heap_indexu-1]->getFinalCost() >= nodes[2*heap_indexu-1]->getFinalCost()) heap_indexv = 2*heap_indexu;
            if(nodes[heap_indexv-1]->getFinalCost() >= nodes[2*heap_indexu]->getFinalCost()) heap_indexv = 2*heap_indexu+1;
        }
        else if (2*heap_indexu <= size){//if only child #1 exists
            //Check if the F cost is greater than the child
            if(nodes[heap_indexu-1]->getFinalCost() >= nodes[2*heap_indexu-1]->getFinalCost()) heap_indexv = 2*heap_indexu;
        }

        if(heap_indexu != heap_indexv){//If parent's F > one or both of its children, swap them
            AStarNode* temp = nodes[heap_indexu-1];
            nodes[heap_indexu-1] = nodes[heap_indexv-1];
            map_pos[nodes[heap_indexu-1]->getX() + nodes[heap_indexu-1]->getY() * map_width] = heap_indexu-1;
            nodes[heap_indexv-1] = temp;
            map_pos[nodes[heap_indexv-1]->getX() + nodes[heap_indexv-1]->getY() * map_width] = heap_indexv-1;

        }
        else{
            break;//if item <= both children, exit loop
        }
    }//Repeat forever

    //remove the node from the map array
    map_pos[node->getX() + node->getY() * map_width] = -1;
}

bool AStarContainer::exists(Point pos){
    return map_pos[pos.x + pos.y * map_width] != -1;
}

AStarNode* AStarContainer::get(int x, int y){
    return nodes[map_pos[x + y * map_width]];
}

bool AStarContainer::isEmpty(){
    return size == 0;
}

void AStarContainer::updateParent(Point pos, Point parent_pos, float score){
    get(pos.x, pos.y)->setParent(parent_pos);
    get(pos.x, pos.y)->setActualCost(score);

    //reorder the heap
    int m = map_pos[pos.x + pos.y * map_width];
    AStarNode* temp = NULL;
    while(m != 0){
        if(nodes[m]->getFinalCost() <= nodes[m/2]->getFinalCost()){
            temp = nodes[m/2];
            nodes[m/2] = nodes[m];
            map_pos[nodes[m/2]->getX() + nodes[m/2]->getY() * map_width] = m/2;
            nodes[m] = temp;
            map_pos[nodes[m]->getX() + nodes[m]->getY() * map_width] = m;
            m=m/2;
        }
        else
            break;
    }

    ///map_pos[pos.x + pos.y * map_width] = m;
}

AStarCloseContainer::AStarCloseContainer(unsigned int map_width, unsigned int map_height, unsigned int node_limit)
    : size(0)
    , map_width(0)
{
    this->map_width = map_width;

    nodes = new AStarNode*[node_limit];
    map_pos = new int[map_width * map_height + 1];

    //initialise the map array. A -1 value will mean there is no node at that position
    ///std::memset(map_pos, -1, sizeof(int) * (map_width + map_height));
    std::fill(map_pos, map_pos + map_width * map_height + 1, -1);
}

AStarCloseContainer::~AStarCloseContainer()
{
    for(int i=0; i<size; i++)
        delete nodes[i];
    delete [] nodes;
    delete [] map_pos;
}

int AStarCloseContainer::getSize()
{
    return size;
}

void AStarCloseContainer::add(AStarNode* node)
{
    nodes[size] = node;
    map_pos[node->getX() + node->getY() * map_width] = size;
    size++;
}

bool AStarCloseContainer::exists(Point pos){
    return map_pos[pos.x + pos.y * map_width] != -1;
}

AStarNode* AStarCloseContainer::get(int x, int y){
    return nodes[map_pos[x + y * map_width]];
}

AStarNode* AStarCloseContainer::get_shortest_h()
{
    AStarNode *current;
    float lowest_score = FLT_MAX;
    for(int i = 0; i < size; i++){
        if(nodes[i]->getH() < lowest_score){
            lowest_score = nodes[i]->getH();
            current = nodes[i];
        }
    }
    return current;
}
