/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2013 Stefan Beller
Copyright © 2013 Henrik Andersson

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

#ifndef MAP_H
#define MAP_H

#include <vector>
#include <queue>

#include "FileParser.h"
#include "Utils.h"
#include "StatBlock.h"

typedef unsigned short maprow[256];

class Map_Group {
public:
	std::string category;
	Point pos;
	Point area;
	int levelmin;
	int levelmax;
	int numbermin;
	int numbermax;
	float chance;
	Map_Group()
		: category("")
		, pos()
		, area()
		, levelmin(0)
		, levelmax(0)
		, numbermin(0)
		, numbermax(0)
		, chance(1.0f)
	{}
};

class Map_NPC {
public:
	std::string id;
	Point pos;
	Map_NPC()
	: id("")
	, pos()
	{}
};

class Map_Event {
public:
	std::string type;
	std::vector<Event_Component> components;
	SDL_Rect location;
	SDL_Rect hotspot;
	int cooldown; // events that run multiple times pause this long in frames
	int cooldown_ticks;
	StatBlock *stats;
	bool keep_after_trigger; // if this event has been triggered once, should this event be kept? If so, this event can be triggered multiple times.

	Map_Event()
	 : type("")
	 , components(std::vector<Event_Component>())
	 , cooldown(0)
	 , cooldown_ticks(0)
	 , stats(NULL)
	 , keep_after_trigger(true)
	{
		location.x = location.y = location.w = location.h = 0;
		hotspot.x = hotspot.y = hotspot.w = hotspot.h = 0;
	}

	// returns a pointer to the event component within the components list
	// no need to free the pointer by caller
	// NULL will be returned if no such event is found
	Event_Component *getComponent(const std::string &_type)
	{
		std::vector<Event_Component>::iterator it;
		for (it = components.begin(); it != components.end(); ++it)
			if (it->type == _type)
				return &(*it);
		return NULL;
	}

	void deleteAllComponents(const std::string &_type) {
		std::vector<Event_Component>::iterator it;
		for (it = components.begin(); it != components.end(); ++it)
			if (it->type == _type)
				it = components.erase(it);
	}

	~Map_Event()
	{
		delete stats; // may be NULL, but delete can deal with null pointers.
	}
};

class Map_Enemy {
public:
	std::string type;
	FPoint pos;
	int direction;
	std::queue<FPoint> waypoints;
	bool wander;
	SDL_Rect wander_area;
	bool hero_ally;
	int summon_power_index;
	StatBlock* summoner;

	Map_Enemy(std::string _type="", Point _pos=Point())
	 : type(_type)
	 , pos(_pos)
	 , direction(rand() % 8)
	 , waypoints(std::queue<FPoint>())
	 , wander(false)
	 , hero_ally(false)
	 , summon_power_index(0)
	 , summoner(NULL)
	{
		wander_area.x = 0;
		wander_area.y = 0;
		wander_area.w = 0;
		wander_area.h = 0;
	}
};

class Map
{
protected:
	std::vector<maprow*> layers; // visible layers in maprenderer
	std::vector<std::string> layernames;

	void loadHeader(FileParser &infile);
	void loadLayer(FileParser &infile, maprow **cur_layer);
	void loadEnemy(FileParser &infile);
	void loadEnemyGroup(FileParser &infile, Map_Group *group);
	void loadNPC(FileParser &infile);
	void loadEvent(FileParser &infile);
	void loadEventComponent(FileParser &infile);

	void clearLayers();
	void clearQueues();

	// map events
	std::vector<Map_Event> events;
	std::queue<Map_Group> enemy_groups;

	std::string filename;
	std::string tileset;
	std::string music_filename;

	int load(std::string filename);
public:
	Map();

	void clearEvents();

	// enemy load handling
	std::queue<Map_Enemy> enemies;

	// npc load handling
	std::queue<Map_NPC> npcs;

	// vars
	std::string title;
	short w;
	short h;
	FPoint spawn;
	int spawn_dir;

};

#endif // MAP_H
