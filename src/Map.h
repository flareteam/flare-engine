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

class SpawnLevel {
public:
	enum {
		MODE_DEFAULT = 0,
		MODE_FIXED = 1,
		MODE_STAT = 2,
		MODE_LEVEL = 3
	};

	enum {
		RATIO_SOURCE_DEFAULT = 0,
		RATIO_SOURCE_HERO = 1,
	};

	uint8_t mode;
	uint8_t ratio_source;
	float ratio;
	size_t stat;
	bool is_legacy;

	SpawnLevel()
		: mode(MODE_DEFAULT)
		, ratio_source(0)
		, ratio(0)
		, stat(0)
		, is_legacy(false)
	{}
	~SpawnLevel() {}

	void parse(FileParser& infile);
	void parseString(const std::string& s);
	void applyToStatBlock(StatBlock *src_stats, StatBlock *ratio_stats);
};

class Map_Group {
public:
	static const int DEFAULT_WANDER_RADIUS = 4;
	static const int RANDOM_DIRECTION = -1;

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
	std::vector<FPoint> waypoints;
	int wander_radius;
	std::vector<EventComponent> requirements;
	std::vector<EventComponent> invincible_requirements;
	SpawnLevel spawn_level;

	Map_Group()
		: type("")
		, category("")
		, pos()
		, area(1,1)
		, levelmin(0)
		, levelmax(0)
		, numbermin(1)
		, numbermax(1)
		, chance(100.0f)
		, direction(RANDOM_DIRECTION)
		, waypoints()
		, wander_radius(DEFAULT_WANDER_RADIUS)
		, requirements()
		, invincible_requirements()
		, spawn_level() {
	}
};

class Map_NPC {
public:
	static const int DEFAULT_WANDER_RADIUS = 0;
	static const int RANDOM_DIRECTION = -1;

	std::string type;
	std::string id;
	FPoint pos;
	std::vector<EventComponent> requirements;
	int direction;
	std::vector<FPoint> waypoints;
	int wander_radius;

	Map_NPC()
		: type("")
		, id("")
		, pos()
		, requirements()
		, direction(RANDOM_DIRECTION)
		, waypoints()
		, wander_radius(DEFAULT_WANDER_RADIUS) {
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
	PowerID summon_power_index;
	StatBlock* summoner;
	std::vector<EventComponent> requirements;
	std::vector<EventComponent> invincible_requirements;
	SpawnLevel spawn_level;

	Map_Enemy(const std::string& _type="", FPoint _pos=FPoint())
		: type(_type)
		, pos(_pos)
		, direction(rand() % 8)
		, waypoints(std::queue<FPoint>())
		, wander_radius(Map_Group::DEFAULT_WANDER_RADIUS)
		, hero_ally(false)
		, enemy_ally(false)
		, summon_power_index(0)
		, summoner(NULL)
		, requirements()
		, invincible_requirements()
		, spawn_level() {
	}
};

class Chunk {
public:
	enum {
		TYPE_EMPTY = 0,
		TYPE_LINKS,
		TYPE_NORMAL,
		TYPE_START,
		TYPE_END,
		TYPE_KEY,
		TYPE_DOOR_NORTH_SOUTH,
		TYPE_DOOR_WEST_EAST,
		TYPE_BRANCH,
		TYPE_COUNT
	};

	enum {
		LINK_NORTH = 0,
		LINK_SOUTH,
		LINK_WEST,
		LINK_EAST,
		LINK_COUNT,
	};

	int type;
	int door_level;
	size_t variant;
	std::vector<Chunk*> links;

	Chunk()
		: type(TYPE_EMPTY)
		, door_level(0)
		, variant(0)
		, links(4, NULL)
	{}

	bool isStraight();
};

class Map {
protected:
	static const bool EXIT_ON_FAIL = true;

	enum {
		PATH_MAIN = 0,
		PATH_BRANCH
	};

	void loadHeader(FileParser &infile);
	bool loadLayer(FileParser &infile, bool exit_on_fail = EXIT_ON_FAIL);
	void loadEnemyGroup(FileParser &infile, Map_Group *group);
	void loadNPC(FileParser &infile);

	void clearLayers();
	void clearEntities();

	std::vector<StatBlock> statblocks;

	std::string filename;
	std::string tileset;

	Chunk* procGenWalkSingle(int path_type, size_t cur_x, size_t cur_y, int walk_x, int walk_y);
	int procGenCreatePath(int path_type, int desired_length, int* _door_count, size_t start_x, size_t start_y);
	void procGenFillArea(const std::string& config_filename, const Rect& area);

	void copyTileLayer(Map* src, size_t layer_index, size_t src_x, size_t src_y, size_t src_w, size_t src_h, size_t x_offset, size_t y_offset);
	void copyMapObjects(Map* src, Chunk* chunk, size_t src_x, size_t src_y, size_t src_w, size_t src_h, size_t x_offset, size_t y_offset);

	std::vector<Point> procgen_branch_roots;

	int procgen_doors_max;
	int procgen_door_spacing_min;
	int procgen_branches_per_door_level_max;

public:
	static const bool LOAD_PROCGEN_CACHE = true;

	Map();
	~Map();
	std::string getFilename() { return filename; }
	std::string getTileset() { return tileset; }
	void setTileset(const std::string& tset) { tileset = tset; }
	void removeLayer(unsigned index);

	int load(const std::string& filename, bool load_procgen_cache = false);

	std::string music_filename;

	std::vector<Map_Layer> layers; // visible layers in maprenderer
	std::vector<std::string> layernames;
	std::vector<unsigned long> layernames_hashed;

	void clearEvents();

	int addEventStatBlock(Event &evnt);

	std::string getProcgenFilename();
	std::string getFOWFilename();

	// enemy load handling
	std::queue<Map_Enemy> enemies;
	std::vector<Map_Group> enemy_groups;

	// npc load handling
	std::vector<Map_NPC> map_npcs;

	// map events
	std::vector<Event> events;
	std::vector<Event> delayed_events;

	// intemap_random queue
	std::string intermap_random_filename;
	std::queue<EventComponent> intermap_random_queue;

	// procgen chunks (i.e. rooms)
	std::vector< std::vector<Chunk> > procgen_chunks;

	// vars
	std::string title;
	unsigned short w;
	unsigned short h;
	bool hero_pos_enabled;
	FPoint hero_pos;
	std::string parallax_filename;
	Color background_color;
	unsigned short fogofwar;
	bool save_fogofwar;
	bool force_spawn_pos;
	size_t procgen_type;
	StatusID procgen_unique_status_id;
	std::string procgen_reset_status;
	StatusID procgen_reset_status_id;

};

#endif // MAP_H
