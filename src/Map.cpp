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


#include "Map.h"

#include "FileParser.h"
#include "UtilsParsing.h"
#include "Settings.h"

Map::Map()
	: filename("")
	, collision_layer(-1)
	, layers()
	, events()
	, w(1)
	, h(1)
	, spawn()
	, spawn_dir(0) {
}

Map::~Map() {
	clearLayers();
}

void Map::clearLayers() {
	layers.clear();
	layernames.clear();
}

void Map::clearQueues() {
	enemies = std::queue<Map_Enemy>();
	npcs = std::queue<Map_NPC>();
}

void Map::clearEvents() {
	events.clear();
	statblocks.clear();
}

void Map::removeLayer(unsigned index) {
	layernames.erase(layernames.begin() + index);
	layers.erase(layers.begin() + index);
}

int Map::load(std::string fname) {
	FileParser infile;

	clearEvents();
	clearLayers();
	clearQueues();

	// @CLASS Map|Description of maps/
	if (!infile.open(fname))
		return 0;

	this->filename = fname;

	while (infile.next()) {
		if (infile.new_section) {

			// for sections that are stored in collections, add a new object here
			if (infile.section == "enemy")
				enemy_groups.push(Map_Group());
			else if (infile.section == "npc")
				npcs.push(Map_NPC());
			else if (infile.section == "event")
				events.push_back(Event());

		}
		if (infile.section == "header")
			loadHeader(infile);
		else if (infile.section == "layer")
			loadLayer(infile);
		else if (infile.section == "enemy")
			loadEnemyGroup(infile, &enemy_groups.back());
		else if (infile.section == "npc")
			loadNPC(infile);
		else if (infile.section == "event")
			EventManager::loadEvent(infile, &events.back());
	}

	infile.close();

	// create StatBlocks for events that need powers
	for (unsigned i=0; i<events.size(); ++i) {
		Event_Component *ec_power = events[i].getComponent("power");
		if (ec_power) {
			statblocks.push_back(StatBlock());
			StatBlock *statb = &statblocks.back();

			if (!statb) {
				logError("Map: Could not create StatBlock for Event.");
				continue;
			}

			// store the index of this StatBlock so that we can find it when the event is activated
			ec_power->y = statblocks.size()-1;

			statb->current[STAT_ACCURACY] = 1000; // always hit the target

			Event_Component *ec_path = events[i].getComponent("power_path");
			if (ec_path) {
				// source is power path start
				statb->pos.x = ec_path->x + 0.5f;
				statb->pos.y = ec_path->y + 0.5f;
			}
			else {
				// source is event location
				statb->pos.x = events[i].location.x + 0.5f;
				statb->pos.y = events[i].location.y + 0.5f;
			}

			Event_Component *ec_damage = events[i].getComponent("power_damage");
			if (ec_damage) {
				statb->current[STAT_DMG_MELEE_MIN] = statb->current[STAT_DMG_RANGED_MIN] = statb->current[STAT_DMG_MENT_MIN] = ec_damage->a;
				statb->current[STAT_DMG_MELEE_MAX] = statb->current[STAT_DMG_RANGED_MAX] = statb->current[STAT_DMG_MENT_MAX] = ec_damage->b;
			}
		}
	}

	// ensure that our map contains a collison layer
	if (std::find(layernames.begin(), layernames.end(), "collision") == layernames.end()) {
		layernames.push_back("collision");
		layers.resize(layers.size()+1);
		layers.back().resize(w);
		for (unsigned i=0; i<w; ++i) {
			layers.back()[i].resize(h, 0);
		}
		collision_layer = layers.size()-1;
	}

	return 0;
}

void Map::loadHeader(FileParser &infile) {
	if (infile.key == "title") {
		// @ATTR title|string|Title of map
		this->title = msg->get(infile.val);
	}
	else if (infile.key == "width") {
		// @ATTR width|integer|Width of map
		this->w = std::max(toInt(infile.val), 1);
	}
	else if (infile.key == "height") {
		// @ATTR height|integer|Height of map
		this->h = std::max(toInt(infile.val), 1);
	}
	else if (infile.key == "tileset") {
		// @ATTR tileset|string|Filename of a tileset definition to use for map
		this->tileset = infile.val;
	}
	else if (infile.key == "music") {
		// @ATTR music|string|Filename of background music to use for map
		music_filename = infile.val;
	}
	else if (infile.key == "location") {
		// @ATTR location|[x(integer), y(integer), direction(integer))|Spawn point location in map
		spawn.x = toInt(infile.nextValue()) + 0.5f;
		spawn.y = toInt(infile.nextValue()) + 0.5f;
		spawn_dir = toInt(infile.nextValue());
	}
	else if (infile.key == "tilewidth") {
		// @ATTR tilewidth|integer|Inherited from Tiled map file. Unused by engine.
	}
	else if (infile.key == "tileheight") {
		// @ATTR tileheight|integer|Inherited from Tiled map file. Unused by engine.
	}
	else if (infile.key == "orientation") {
		// this is only used by Tiled when importing Flare maps
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

void Map::loadLayer(FileParser &infile) {
	if (infile.key == "type") {
		// @ATTR layer.type|string|Map layer type.
		layers.resize(layers.size()+1);
		layers.back().resize(w);
		for (unsigned i=0; i<w; ++i) {
			layers.back()[i].resize(h);
		}
		layernames.push_back(infile.val);
		if (infile.val == "collision")
			collision_layer = layernames.size()-1;
	}
	else if (infile.key == "format") {
		// @ATTR layer.format|string|Format for map layer, must be 'dec'
		if (infile.val != "dec") {
			infile.error("Map: The format of a layer must be \"dec\"!");
			Exit(1);
		}
	}
	else if (infile.key == "data") {
		// @ATTR layer.data|raw|Raw map layer data
		// layer map data handled as a special case
		// The next h lines must contain layer data.
		for (int j=0; j<h; j++) {
			std::string val = infile.getRawLine();
			infile.incrementLineNum();
			if (!val.empty() && val[val.length()-1] != ',') {
				val += ',';
			}

			// verify the width of this row
			int comma_count = 0;
			for (unsigned i=0; i<val.length(); ++i) {
				if (val[i] == ',') comma_count++;
			}
			if (comma_count != w) {
				infile.error("Map: A row of layer data has a width not equal to %d.", w);
				Exit(1);
			}

			for (int i=0; i<w; i++)
				layers.back()[i][j] = popFirstInt(val, ',');
		}
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

void Map::loadEnemyGroup(FileParser &infile, Map_Group *group) {
	if (infile.key == "type") {
		// @ATTR enemygroup.type|string|The category of enemies that will spawn in this group.
		group->category = infile.val;
	}
	else if (infile.key == "level") {
		// @ATTR enemygroup.level|[min(integer), max(integer)]|Defines the level range of enemies in group. If only one number is given, it's the exact level.
		group->levelmin = std::max(0, toInt(infile.nextValue()));
		group->levelmax = std::max(0, toInt(infile.nextValue(), group->levelmin));
	}
	else if (infile.key == "location") {
		// @ATTR enemygroup.location|[x(integer), y(integer), x2(integer), y2(integer)]|Location area for enemygroup
		group->pos.x = toInt(infile.nextValue());
		group->pos.y = toInt(infile.nextValue());
		group->area.x = toInt(infile.nextValue());
		group->area.y = toInt(infile.nextValue());
	}
	else if (infile.key == "number") {
		// @ATTR enemygroup.number|[min(integer), max(integer]|Defines the range of enemies in group. If only one number is given, it's the exact amount.
		group->numbermin = std::max(0, toInt(infile.nextValue()));
		group->numbermax = std::max(0, toInt(infile.nextValue(), group->numbermin));
	}
	else if (infile.key == "chance") {
		// @ATTR enemygroup.chance|integer|Percentage of chance
		float n = std::max(0, toInt(infile.nextValue())) / 100.0f;
		group->chance = std::min(1.0f, std::max(0.0f, n));
	}
	else if (infile.key == "direction") {
		// @ATTR enemygroup.direction|direction|Direction that enemies will initially face.
		group->direction = parse_direction(infile.val);
	}
	else if (infile.key == "waypoints") {
		// @ATTR enemygroup.waypoints|[x(integer), y(integer)]|Enemy waypoints; single enemy only; negates wander_radius
		std::string none = "";
		std::string a = infile.nextValue();
		std::string b = infile.nextValue();

		while (a != none) {
			FPoint p;
			p.x = toInt(a) + 0.5f;
			p.y = toInt(b) + 0.5f;
			group->waypoints.push(p);
			a = infile.nextValue();
			b = infile.nextValue();
		}

		// disable wander radius, since we can't have waypoints and wandering at the same time
		group->wander_radius = 0;
	}
	else if (infile.key == "wander_radius") {
		// @ATTR enemygroup.wander_radius|integer|The radius (in tiles) that an enemy will wander around randomly; negates waypoints
		group->wander_radius = std::max(0, toInt(infile.nextValue()));

		// clear waypoints, since wandering will use the waypoint queue
		while (!group->waypoints.empty()) {
			group->waypoints.pop();
		}
	}
	else if (infile.key == "requires_status") {
		// @ATTR enemygroup.requires_status|string|Status required for loading enemies
		group->requires_status.push_back(infile.nextValue());
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR enemygroup.requires_not_status|string|Status required to be missing for loading enemies
		group->requires_not_status.push_back(infile.nextValue());
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

void Map::loadNPC(FileParser &infile) {
	std::string s;
	if (infile.key == "type") {
		// @ATTR npc.type|string|Filename of an NPC definition.
		npcs.back().id = infile.val;
	}
	else if (infile.key == "requires_status") {
		// @ATTR npc.requires_status|string|Status required for NPC load. There can be multiple states, separated by comma
		while ( (s = infile.nextValue()) != "")
			npcs.back().requires_status.push_back(s);
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR npc.requires_not_status|string|Status required to be missing for NPC load. There can be multiple states, separated by comma
		while ( (s = infile.nextValue()) != "")
			npcs.back().requires_not_status.push_back(s);
	}
	else if (infile.key == "location") {
		// @ATTR npc.location|[x(integer), y(integer)]|Location of NPC
		npcs.back().pos.x = toInt(infile.nextValue()) + 0.5f;
		npcs.back().pos.y = toInt(infile.nextValue()) + 0.5f;

		// make sure this NPC has a collision tile
		// otherwise, it becomes possible for the player to stand "inside" the npc, which will trigger their event infinitely
		if (collision_layer != -1) {
			unsigned tile_x = npcs.back().pos.x;
			unsigned tile_y = npcs.back().pos.y;
			if (tile_x < (unsigned)w && tile_y < (unsigned)h) {
				short unsigned int& tile = layers[collision_layer][tile_x][tile_y];
				if (tile == BLOCKS_NONE) {
					logError("Map: NPC at (%d, %d) does not have a collision tile. Creating one now.", tile_x, tile_y);
					tile = BLOCKS_MOVEMENT_HIDDEN;
				}
			}
		}
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

