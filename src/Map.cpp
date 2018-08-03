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


#include "CampaignManager.h"
#include "EffectManager.h"
#include "EngineSettings.h"
#include "EventManager.h"
#include "FileParser.h"
#include "Map.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "PowerManager.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"

Map::Map()
	: filename("")
	, collision_layer(-1)
	, layers()
	, events()
	, w(1)
	, h(1)
	, hero_pos_enabled(false)
	, hero_pos()
	, background_color(0,0,0,0) {
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
	delayed_events.clear();
	statblocks.clear();
}

void Map::removeLayer(unsigned index) {
	layernames.erase(layernames.begin() + index);
	layers.erase(layers.begin() + index);
}

int Map::load(const std::string& fname) {
	FileParser infile;

	clearEvents();
	clearLayers();
	clearQueues();

	music_filename = "";

	collision_layer = -1;
	w = 1;
	h = 1;
	hero_pos_enabled = false;
	hero_pos.x = 0;
	hero_pos.y = 0;

	// @CLASS Map|Description of maps/
	if (!infile.open(fname, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return 0;

	Utils::logInfo("Map: Loading map '%s'", fname.c_str());

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
		EventComponent *ec_power = events[i].getComponent(EventComponent::POWER);
		if (ec_power) {
			// store the index of this StatBlock so that we can find it when the event is activated
			ec_power->y = addEventStatBlock(events[i]);
		}
	}

	// ensure that our map contains a collision layer
	if (std::find(layernames.begin(), layernames.end(), "collision") == layernames.end()) {
		layernames.push_back("collision");
		layers.resize(layers.size()+1);
		layers.back().resize(w);
		for (size_t i=0; i<layers.back().size(); ++i) {
			layers.back()[i].resize(h, 0);
		}
		collision_layer = static_cast<int>(layers.size())-1;
	}

	if (!hero_pos_enabled) {
		Utils::logError("Map: Hero spawn position (hero_pos) not defined in map header. Defaulting to (0,0).");
	}

	return 0;
}

void Map::loadHeader(FileParser &infile) {
	if (infile.key == "title") {
		// @ATTR title|string|Title of map
		this->title = msg->get(infile.val);
	}
	else if (infile.key == "width") {
		// @ATTR width|int|Width of map
		this->w = static_cast<unsigned short>(std::max(Parse::toInt(infile.val), 1));
	}
	else if (infile.key == "height") {
		// @ATTR height|int|Height of map
		this->h = static_cast<unsigned short>(std::max(Parse::toInt(infile.val), 1));
	}
	else if (infile.key == "tileset") {
		// @ATTR tileset|filename|Filename of a tileset definition to use for map
		this->tileset = infile.val;
	}
	else if (infile.key == "music") {
		// @ATTR music|filename|Filename of background music to use for map
		music_filename = infile.val;
	}
	else if (infile.key == "hero_pos") {
		// @ATTR hero_pos|point|The player will spawn in this location if no point was previously given.
		hero_pos.x = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
		hero_pos.y = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
		hero_pos_enabled = true;
	}
	else if (infile.key == "parallax_layers") {
		// @ATTR parallax_layers|filename|Filename of a parallax layers definition.
		parallax_filename = infile.val;
	}
	else if (infile.key == "background_color") {
		// @ATTR background_color|color, int : Color, alpha|Background color for the map.
		background_color = Parse::toRGBA(infile.val);
	}
	else if (infile.key == "tilewidth") {
		// @ATTR tilewidth|int|Inherited from Tiled map file. Unused by engine.
	}
	else if (infile.key == "tileheight") {
		// @ATTR tileheight|int|Inherited from Tiled map file. Unused by engine.
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
		for (size_t i=0; i<layers.back().size(); ++i) {
			layers.back()[i].resize(h);
		}
		layernames.push_back(infile.val);
		if (infile.val == "collision")
			collision_layer = static_cast<int>(layernames.size())-1;
	}
	else if (infile.key == "format") {
		// @ATTR layer.format|string|Format for map layer, must be 'dec'
		if (infile.val != "dec") {
			infile.error("Map: The format of a layer must be 'dec'!");
			Utils::logErrorDialog("Map: The format of a layer must be 'dec'!");
			mods->resetModConfig();
			Utils::Exit(1);
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
				mods->resetModConfig();
				Utils::Exit(1);
			}

			for (int i=0; i<w; i++)
				layers.back()[i][j] = static_cast<unsigned short>(Parse::popFirstInt(val));
		}
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

void Map::loadEnemyGroup(FileParser &infile, Map_Group *group) {
	if (infile.key == "type") {
		// @ATTR enemygroup.type|string|(IGNORED BY ENGINE) The "type" field, as used by Tiled and other mapping tools.
		group->type = infile.val;
	}
	else if (infile.key == "category") {
		// @ATTR enemygroup.category|predefined_string|The category of enemies that will spawn in this group.
		group->category = infile.val;
	}
	else if (infile.key == "level") {
		// @ATTR enemygroup.level|int, int : Min, Max|Defines the level range of enemies in group. If only one number is given, it's the exact level.
		group->levelmin = std::max(0, Parse::popFirstInt(infile.val));
		group->levelmax = std::max(std::max(0, Parse::toInt(Parse::popFirstString(infile.val))), group->levelmin);
	}
	else if (infile.key == "location") {
		// @ATTR enemygroup.location|rectangle|Location area for enemygroup
		group->pos.x = Parse::popFirstInt(infile.val);
		group->pos.y = Parse::popFirstInt(infile.val);
		group->area.x = Parse::popFirstInt(infile.val);
		group->area.y = Parse::popFirstInt(infile.val);
	}
	else if (infile.key == "number") {
		// @ATTR enemygroup.number|int, int : Min, Max|Defines the range of enemies in group. If only one number is given, it's the exact amount.
		group->numbermin = std::max(0, Parse::popFirstInt(infile.val));
		group->numbermax = std::max(std::max(0, Parse::toInt(Parse::popFirstString(infile.val))), group->numbermin);
	}
	else if (infile.key == "chance") {
		// @ATTR enemygroup.chance|int|Percentage of chance
		float n = static_cast<float>(std::max(0, Parse::popFirstInt(infile.val))) / 100.0f;
		group->chance = std::min(1.0f, std::max(0.0f, n));
	}
	else if (infile.key == "direction") {
		// @ATTR enemygroup.direction|direction|Direction that enemies will initially face.
		group->direction = Parse::toDirection(infile.val);
	}
	else if (infile.key == "waypoints") {
		// @ATTR enemygroup.waypoints|list(point)|Enemy waypoints; single enemy only; negates wander_radius
		std::string none = "";
		std::string a = Parse::popFirstString(infile.val);
		std::string b = Parse::popFirstString(infile.val);

		while (a != none) {
			FPoint p;
			p.x = static_cast<float>(Parse::toInt(a)) + 0.5f;
			p.y = static_cast<float>(Parse::toInt(b)) + 0.5f;
			group->waypoints.push(p);
			a = Parse::popFirstString(infile.val);
			b = Parse::popFirstString(infile.val);
		}

		// disable wander radius, since we can't have waypoints and wandering at the same time
		group->wander_radius = 0;
	}
	else if (infile.key == "wander_radius") {
		// @ATTR enemygroup.wander_radius|int|The radius (in tiles) that an enemy will wander around randomly; negates waypoints
		group->wander_radius = std::max(0, Parse::popFirstInt(infile.val));

		// clear waypoints, since wandering will use the waypoint queue
		while (!group->waypoints.empty()) {
			group->waypoints.pop();
		}
	}
	else if (infile.key == "requires_status") {
		// @ATTR enemygroup.requires_status|list(string)|Status required for loading enemies
		std::string s;
		while ((s = Parse::popFirstString(infile.val)) != "") {
			group->requires_status.push_back(camp->registerStatus(s));
		}
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR enemygroup.requires_not_status|list(string)|Status required to be missing for loading enemies
		std::string s;
		while ((s = Parse::popFirstString(infile.val)) != "") {
			group->requires_not_status.push_back(camp->registerStatus(s));
		}
	}
	else if (infile.key == "invincible_requires_status") {
		// @ATTR enemygroup.invincible_requires_status|list(string)|Enemies in this group are invincible to hero attacks when these statuses are set.
		std::string s;
		while ((s = Parse::popFirstString(infile.val)) != "") {
			group->invincible_requires_status.push_back(camp->registerStatus(s));
		}
	}
	else if (infile.key == "invincible_requires_not_status") {
		// @ATTR enemygroup.invincible_requires_not_status|list(string)|Enemies in this group are invincible to hero attacks when these statuses are not set.
		std::string s;
		while ((s = Parse::popFirstString(infile.val)) != "") {
			group->invincible_requires_not_status.push_back(camp->registerStatus(s));
		}
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

void Map::loadNPC(FileParser &infile) {
	std::string s;
	if (infile.key == "type") {
		// @ATTR npc.type|string|(IGNORED BY ENGINE) The "type" field, as used by Tiled and other mapping tools.
		npcs.back().type = infile.val;
	}
	else if (infile.key == "filename") {
		// @ATTR npc.filename|string|Filename of an NPC definition.
		npcs.back().id = infile.val;
	}
	else if (infile.key == "requires_status") {
		// @ATTR npc.requires_status|list(string)|Status required for NPC load. There can be multiple states, separated by comma
		while ( (s = Parse::popFirstString(infile.val)) != "")
			npcs.back().requires_status.push_back(camp->registerStatus(s));
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR npc.requires_not_status|list(string)|Status required to be missing for NPC load. There can be multiple states, separated by comma
		while ( (s = Parse::popFirstString(infile.val)) != "")
			npcs.back().requires_not_status.push_back(camp->registerStatus(s));
	}
	else if (infile.key == "location") {
		// @ATTR npc.location|point|Location of NPC
		npcs.back().pos.x = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
		npcs.back().pos.y = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

int Map::addEventStatBlock(Event &evnt) {
	statblocks.push_back(StatBlock());
	StatBlock *statb = &statblocks.back();

	statb->perfect_accuracy = true; // never miss AND never overhit

	EventComponent *ec_path = evnt.getComponent(EventComponent::POWER_PATH);
	if (ec_path) {
		// source is power path start
		statb->pos.x = static_cast<float>(ec_path->x) + 0.5f;
		statb->pos.y = static_cast<float>(ec_path->y) + 0.5f;
	}
	else {
		// source is event location
		statb->pos.x = static_cast<float>(evnt.location.x) + 0.5f;
		statb->pos.y = static_cast<float>(evnt.location.y) + 0.5f;
	}

	EventComponent *ec_damage = evnt.getComponent(EventComponent::POWER_DAMAGE);
	if (ec_damage) {
		for (size_t i = 0; i < eset->damage_types.count; ++i) {
			if (i % 2 == 0) {
				statb->starting[Stats::COUNT + i] = ec_damage->a; // min
			}
			else {
				statb->starting[Stats::COUNT + i] = ec_damage->b; // max
			}
		}
	}

	// this is used to store cooldown ticks for a map power
	// the power id, type, etc are not used
	statb->powers_ai.resize(1);

	// make this StatBlock immune to negative status effects
	// this is mostly to prevent a player with a damage return bonus from damaging this StatBlock
	// create a temporary EffectDef for immunity; will be used for map StatBlocks
	EffectDef immunity_effect;
	immunity_effect.id = "MAP_EVENT_IMMUNITY";
	immunity_effect.type = "immunity";
	statb->effects.addEffect(immunity_effect, 0, 0, Power::SOURCE_TYPE_ENEMY, EffectManager::NO_POWER);

	// ensure the statblock will be alive
	statb->hp = statb->starting[Stats::HP_MAX] = statb->current[Stats::HP_MAX] = 1;

	// ensure that stats are ready to be used by running logic once
	statb->logic();

	return static_cast<int>(statblocks.size())-1;
}
