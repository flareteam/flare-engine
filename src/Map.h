/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2013 Stefan Beller
Copyright © 2013 Henrik Andersson
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

#ifndef MAP_H
#define MAP_H

#include "CommonIncludes.h"
#include "EventManager.h"
#include "MapCollision.h"
#include "Utils.h"

class Event;
class FileParser;
class StatBlock;

class Map_Group {
public:
	std::string type;
	std::string category;
	Point pos;
	Point area;
	int levelmin;
	int levelmax;
	int numbermin;
	int numbermax;
	float chance;
	int direction;
	std::queue<FPoint> waypoints;
	int wander_radius;
	std::vector<StatusID> requires_status;
	std::vector<StatusID> requires_not_status;
	std::vector<StatusID> invincible_requires_status;
	std::vector<StatusID> invincible_requires_not_status;

	Map_Group()
		: type("")
		, category("")
		, pos()
		, area(1,1)
		, levelmin(0)
		, levelmax(0)
		, numbermin(1)
		, numbermax(1)
		, chance(1.0f)
		, direction(-1)
		, waypoints(std::queue<FPoint>())
		, wander_radius(4)
		, requires_status()
		, requires_not_status()
		, invincible_requires_status()
		, invincible_requires_not_status() {
	}
};

class Map_NPC {
public:
	std::string type;
	std::string id;
	FPoint pos;
	std::vector<StatusID> requires_status;
	std::vector<StatusID> requires_not_status;

	Map_NPC()
		: type("")
		, id("")
		, pos()
		, requires_status()
		, requires_not_status() {
	}
};

class Map_Enemy {
public:
	std::string type;
	FPoint pos;
	int direction;
	std::queue<FPoint> waypoints;
	int wander_radius;
	bool hero_ally;
	bool enemy_ally;
	int summon_power_index;
	StatBlock* summoner;
	std::vector<StatusID> requires_status;
	std::vector<StatusID> requires_not_status;
	std::vector<StatusID> invincible_requires_status;
	std::vector<StatusID> invincible_requires_not_status;

	Map_Enemy(const std::string& _type="", FPoint _pos=FPoint())
		: type(_type)
		, pos(_pos)
		, direction(rand() % 8)
		, waypoints(std::queue<FPoint>())
		, wander_radius(4)
		, hero_ally(false)
		, enemy_ally(false)
		, summon_power_index(0)
		, summoner(NULL)
		, requires_status()
		, requires_not_status()
		, invincible_requires_status()
		, invincible_requires_not_status() {
	}
};

class Map {
protected:
	void loadHeader(FileParser &infile);
	void loadLayer(FileParser &infile);
	void loadEnemyGroup(FileParser &infile, Map_Group *group);
	void loadNPC(FileParser &infile);

	void clearLayers();
	void clearQueues();

	std::vector<StatBlock> statblocks;

	std::string filename;
	std::string tileset;

	int collision_layer;
public:
	Map();
	~Map();
	std::string getFilename() { return filename; }
	std::string getTileset() { return tileset; }
	void setTileset(const std::string& tset) { tileset = tset; }
	void removeLayer(unsigned index);

	int load(const std::string& filename);

	std::string music_filename;

	std::vector<Map_Layer> layers; // visible layers in maprenderer
	std::vector<std::string> layernames;

	void clearEvents();

	int addEventStatBlock(Event &evnt);

	// enemy load handling
	std::queue<Map_Enemy> enemies;
	std::queue<Map_Group> enemy_groups;

	// npc load handling
	std::queue<Map_NPC> npcs;

	// map events
	std::vector<Event> events;
	std::vector<Event> delayed_events;

	// intemap_random queue
	std::string intermap_random_filename;
	std::queue<EventComponent> intermap_random_queue;

	// vars
	std::string title;
	unsigned short w;
	unsigned short h;
	bool hero_pos_enabled;
	FPoint hero_pos;
	std::string parallax_filename;
	Color background_color;

};

#endif // MAP_H
