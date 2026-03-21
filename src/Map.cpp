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


#include "Avatar.h"
#include "CampaignManager.h"
#include "EffectManager.h"
#include "EngineSettings.h"
#include "EventManager.h"
#include "FileParser.h"
#include "FogOfWar.h"
#include "Map.h"
#include "MapSaver.h"
#include "MapRenderer.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "PowerManager.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "StatBlock.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <vector>

void SpawnLevel::parse(FileParser &infile) {
	std::string next = Parse::popFirstString(infile.val);

	if (next == "default") mode = MODE_DEFAULT;
	else if (next == "fixed") mode = MODE_FIXED;
	else if (next == "source_stat") mode = MODE_STAT;
	else if (next == "source_level") mode = MODE_LEVEL;
	else if (next == "hero_stat") {
		mode = MODE_STAT;
		ratio_source = RATIO_SOURCE_HERO;
	}
	else if (next == "hero_level") {
		mode = MODE_LEVEL;
		ratio_source = RATIO_SOURCE_HERO;
	}
	else if (next == "stat") {
		mode = MODE_STAT;
		is_legacy = true;
		infile.error("SpawnLevel: 'stat' mode is deprecated. Use 'source_stat' instead.");
	}
	else if (next == "level") {
		mode = MODE_LEVEL;
		is_legacy = true;
		infile.error("SpawnLevel: 'level' mode is deprecated. Use 'source_level' instead.");
	}
	else infile.error("SpawnLevel: Unknown spawn level mode '%s'", next.c_str());

	if (mode != MODE_DEFAULT) {
		ratio = Parse::popFirstFloat(infile.val);

		if (mode != MODE_FIXED) {
			next = Parse::popFirstString(infile.val);
			float nextf = Parse::toFloat(next);

			if (is_legacy) {
				// legacy format detected!
				if (nextf != 0) {
					ratio = ratio / nextf;
				}

				if (mode == MODE_STAT) {
					next = Parse::popFirstString(infile.val);
				}

			}

			if (mode == MODE_STAT) {
				size_t prim_stat_index = eset->primary_stats.getIndexByID(next);

				if (prim_stat_index != eset->primary_stats.list.size()) {
					stat = prim_stat_index;
				}
				else {
					infile.error("SpawnLevel: '%s' is not a valid primary stat.", next.c_str());
				}
			}
		}
	}
}

void SpawnLevel::parseString(const std::string& s) {
	std::string parse_str = s;

	std::string next = Parse::popFirstString(parse_str);

	if (next == "default") mode = MODE_DEFAULT;
	else if (next == "fixed") mode = MODE_FIXED;
	else if (next == "source_stat") mode = MODE_STAT;
	else if (next == "source_level") mode = MODE_LEVEL;
	else if (next == "hero_stat") {
		mode = MODE_STAT;
		ratio_source = RATIO_SOURCE_HERO;
	}
	else if (next == "hero_level") {
		mode = MODE_LEVEL;
		ratio_source = RATIO_SOURCE_HERO;
	}
	else Utils::logError("SpawnLevel: Unknown spawn level mode '%s'", next.c_str());

	if (mode != MODE_DEFAULT) {
		ratio = Parse::popFirstFloat(parse_str);

		if (mode != MODE_FIXED) {
			next = Parse::popFirstString(parse_str);

			if (mode == MODE_STAT) {
				size_t prim_stat_index = eset->primary_stats.getIndexByID(next);

				if (prim_stat_index != eset->primary_stats.list.size()) {
					stat = prim_stat_index;
				}
				else {
					Utils::logError("SpawnLevel: '%s' is not a valid primary stat.", next.c_str());
				}
			}
		}
	}
}

void SpawnLevel::applyToStatBlock(StatBlock *src_stats, StatBlock *ratio_stats) {
	if (!src_stats)
		return;

	if (pc && ratio_source == RATIO_SOURCE_HERO) {
		ratio_stats = &pc->stats;
	}

	if (mode == MODE_FIXED) {
		src_stats->level = static_cast<int>(ratio);
	}
	else if (ratio_stats && ratio != 0) {
		if (mode == MODE_LEVEL) {
			src_stats->level = static_cast<int>(static_cast<float>(ratio_stats->level) * ratio);
		}
		else if (mode == MODE_STAT) {
			int stat_val = 0;
			for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
				if (stat == i) {
					stat_val = ratio_stats->get_primary(i);
					break;
				}
			}

			src_stats->level = static_cast<int>(static_cast<float>(stat_val) * ratio);
		}
	}
}

Map::Map()
	: filename("")
	, procgen_doors_max(0)
	, procgen_door_spacing_min(0)
	, procgen_branches_per_door_level_max(0)
	, layers()
	, events()
	, w(1)
	, h(1)
	, hero_pos_enabled(false)
	, hero_pos()
	, background_color(0,0,0,0)
	, fogofwar(eset->misc.fogofwar)
	, save_fogofwar(eset->misc.save_fogofwar)
	, force_spawn_pos(false)
	, procgen_type(Chunk::TYPE_EMPTY)
	, procgen_unique_status_id(0)
	, procgen_reset_status()
	, procgen_reset_status_id(0)
{
}

Map::~Map() {
	clearLayers();
}

void Map::clearLayers() {
	layers.clear();
	layernames.clear();
	layernames_hashed.clear();
}

void Map::clearEntities() {
	enemies = std::queue<Map_Enemy>();
	enemy_groups.clear();
	map_npcs.clear();
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

int Map::load(const std::string& fname, bool load_procgen_cache) {
	procgen_chunks.clear();

	clearEvents();
	clearLayers();
	clearEntities();

	music_filename = "";
	parallax_filename = "";
	background_color = Color(0,0,0,0);
	fogofwar = eset->misc.fogofwar;
	save_fogofwar = eset->misc.save_fogofwar;
	force_spawn_pos = false;

	w = 1;
	h = 1;
	hero_pos_enabled = false;
	hero_pos.x = 0;
	hero_pos.y = 0;

	Utils::logInfo("Map: Loading map '%s'", fname.c_str());

	this->filename = fname;

	std::string procgen_filename = getProcgenFilename();

	// @CLASS Map|Description of maps/
	FileParser infile;
	if (load_procgen_cache) {
		if (!infile.open(procgen_filename, !FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
			// couldn't load cached map, try loading the original
			if (!infile.open(fname, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
				return 0;
		}
	}
	else {
		if (!infile.open(fname, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
			return 0;
	}

	while (infile.next()) {
		if (infile.new_section) {

			// for sections that are stored in collections, add a new object here
			if (infile.section == "enemy")
				enemy_groups.push_back(Map_Group());
			else if (infile.section == "npc")
				map_npcs.push_back(Map_NPC());
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
			eventm->loadEvent(infile, &events.back());
	}

	infile.close();

	// generate any procedural regions
	std::vector<Event> procgen_regions;
	for (size_t i = 0; i < events.size(); ++i) {
		EventComponent *ec_procgen = events[i].getComponent(EventComponent::PROCGEN_FILENAME);
		if (ec_procgen) {
			procgen_regions.push_back(events[i]);
		}
	}

	if (procgen_regions.size() > 1) {
		Utils::logInfo("Map: Warning! Only 1 'procgen_filename' event is supported.");
	}

	// if a previously generated map exists, load it from disk cache
	// otherwise, save the generated map to disk
	if (!procgen_regions.empty()) {
		if (!camp->checkStatus(procgen_reset_status_id) && Filesystem::fileExists(procgen_filename)) {
			return load(fname, Map::LOAD_PROCGEN_CACHE);
		}
		else {
			EventComponent *ec_procgen = procgen_regions[0].getComponent(EventComponent::PROCGEN_FILENAME);
			procGenFillArea(ec_procgen->s, procgen_regions[0].location);

			MapSaver map_saver(this);
			Utils::logInfo("Saving map: %s", fname.c_str());
			map_saver.saveMap(procgen_filename, "");

			// we can't use the spawn position from the player's save file if we're generating a new map
			force_spawn_pos = true;
		}
	}

	// load fog-of-war configuration file
	// TODO does this need to be done on every map load?
	if (fogofwar) {
		fow->load();
	}

	// create StatBlocks for events that need powers
	for (unsigned i=0; i<events.size(); ++i) {
		EventComponent *ec_power = events[i].getComponent(EventComponent::POWER);
		if (ec_power) {
			// store the index of this StatBlock so that we can find it when the event is activated
			ec_power->data[0].Int = addEventStatBlock(events[i]);
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
	}

	if (fogofwar) {
		// load the fog-of-war data from disk cache, unless this map was just procedurally generated
		if (save_fogofwar && procgen_regions.empty()) {
			std::string fow_filename = getFOWFilename();
			if (infile.open(fow_filename, !FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
				while (infile.next()) {
					if (infile.section == "layer") {
						if (!loadLayer(infile, !EXIT_ON_FAIL)) {
							for (size_t i = layers.size(); i > 0; i--) {
								size_t layer_index = i-1;
								if (layernames[layer_index] == "fow_fog") {
									layernames.erase(layernames.begin() + layer_index);
									layers.erase(layers.begin() + layer_index);
								}
								else if (layernames[layer_index] == "fow_dark") {
									layernames.erase(layernames.begin() + layer_index);
									layers.erase(layers.begin() + layer_index);
								}
							}
							break;
						}
					}
				}
				infile.close();
			}
		}

		// ensure that our map contains a fog of war layers
		if (std::find(layernames.begin(), layernames.end(), "fow_fog") == layernames.end()) {
			layernames.push_back("fow_fog");
			layers.resize(layers.size()+1);
			layers.back().resize(w);
			for (size_t i=0; i<layers.back().size(); ++i) {
				layers.back()[i].resize(h, FogOfWar::TILE_HIDDEN);
			}
		}

		if (std::find(layernames.begin(), layernames.end(), "fow_dark") == layernames.end()) {
			layernames.push_back("fow_dark");
			layers.resize(layers.size()+1);
			layers.back().resize(w);
			for (size_t i=0; i<layers.back().size(); ++i) {
				layers.back()[i].resize(h, FogOfWar::TILE_HIDDEN);
			}
		}
	}

	if (!hero_pos_enabled) {
		Utils::logError("Map: Hero spawn position (hero_pos) not defined in map header. Defaulting to (0,0).");
	}

	// hash layer names for more performant checking from events
	layernames_hashed.resize(layers.size());
	for (size_t i = 0; i < layernames_hashed.size(); ++i) {
		layernames_hashed[i] = Utils::hashString(layernames[i]);
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
	else if (infile.key == "fogofwar") {
		// @ATTR fogofwar|int|Set the fog of war type. 0-disabled, 1-minimap, 2-tint, 3-overlay. Overrides engine settings.
		fogofwar = static_cast<unsigned short>(Parse::toInt(infile.val));
	}
	else if (infile.key == "save_fogofwar") {
		// @ATTR save_fogofwar|bool|If true, the fog of war layer keeps track of the progress. Overrides engine settings.
		save_fogofwar = Parse::toBool(infile.val);
	}
	else if (infile.key == "tilewidth") {
		// @ATTR tilewidth|int|Inherited from Tiled map file. Unused by engine.
	}
	else if (infile.key == "tileheight") {
		// @ATTR tileheight|int|Inherited from Tiled map file. Unused by engine.
	}
	else if (infile.key == "procgen_type") {
		// TODO rename to "procgen_chunk_type"?
		// @ATTR procgen_type|["links", "normal", "start", "end", "key", "door_north_south", "door_west_east"]|Defines the type of chunk this map is when used in procedural map generation.
		if (infile.val == "links") {
			procgen_type = Chunk::TYPE_LINKS;
		}
		else if (infile.val == "normal") {
			procgen_type = Chunk::TYPE_NORMAL;
		}
		else if (infile.val == "start") {
			procgen_type = Chunk::TYPE_START;
		}
		else if (infile.val == "end") {
			procgen_type = Chunk::TYPE_END;
		}
		else if (infile.val == "key") {
			procgen_type = Chunk::TYPE_KEY;
		}
		else if (infile.val == "door_north_south") {
			procgen_type = Chunk::TYPE_DOOR_NORTH_SOUTH;
		}
		else if (infile.val == "door_west_east") {
			procgen_type = Chunk::TYPE_DOOR_WEST_EAST;
		}
		else {
			infile.error("Map: '%s' is not a valid procedural generation chunk type.", infile.val.c_str());
		}
	}
	else if (infile.key == "procgen_unique_status") {
		// @ATTR procgen_unique_status|string|Sets this campaign status once this map is placed as a chunk during procedural generation. Once the status is set, the chunk will no longer be eligible to be placed.
		procgen_unique_status_id = camp->registerStatus(infile.val);
	}
	else if (infile.key == "procgen_reset_status") {
		// @ATTR procgen_reset_status|string|If the map contains a procgen region, it will be regenerated if this campaign status is set.
		procgen_reset_status = infile.val;
		procgen_reset_status_id = camp->registerStatus(procgen_reset_status);
	}
	else if (infile.key == "orientation") {
		// this is only used by Tiled when importing Flare maps
	}
	else if (infile.key == "procgen_chunks") {
		// this is written by MapSaver for procedural maps

		// width/height in chunks
		size_t cmap_w = static_cast<size_t>(Parse::popFirstInt(infile.val));
		size_t cmap_h = static_cast<size_t>(Parse::popFirstInt(infile.val));
		procgen_chunks.resize(cmap_h, std::vector<Chunk>(cmap_w));

		size_t cmap_x = 0;
		size_t cmap_y = 0;

		std::string chunk_type = Parse::popFirstString(infile.val);
		while (!chunk_type.empty() && cmap_y < procgen_chunks.size() && cmap_x < procgen_chunks[cmap_y].size()) {
			Chunk* chunk = &procgen_chunks[cmap_y][cmap_x];
			chunk->type = Parse::toInt(chunk_type);
			chunk->door_level = Parse::popFirstInt(infile.val);
			chunk->variant = Parse::popFirstInt(infile.val);

			if (Parse::popFirstInt(infile.val) == 1 && cmap_y > 0) {
				chunk->links[Chunk::LINK_NORTH] = &procgen_chunks[cmap_y-1][cmap_x];
			}
			if (Parse::popFirstInt(infile.val) == 1 && cmap_y+1 < procgen_chunks.size()) {
				chunk->links[Chunk::LINK_SOUTH] = &procgen_chunks[cmap_y+1][cmap_x];
			}
			if (Parse::popFirstInt(infile.val) == 1 && cmap_x > 0) {
				chunk->links[Chunk::LINK_WEST] = &procgen_chunks[cmap_y][cmap_x-1];
			}
			if (Parse::popFirstInt(infile.val) == 1 && cmap_x+1 < procgen_chunks[cmap_y].size()) {
				chunk->links[Chunk::LINK_EAST] = &procgen_chunks[cmap_y][cmap_x+1];
			}

			cmap_x++;
			if (cmap_x >= procgen_chunks[cmap_y].size()) {
				cmap_x = 0;
				cmap_y++;
			}

			chunk_type = Parse::popFirstString(infile.val);
		}
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

bool Map::loadLayer(FileParser &infile, bool exit_on_fail) {
	if (infile.key == "type") {
		// @ATTR layer.type|string|Map layer type.
		layers.resize(layers.size()+1);
		layers.back().resize(w);
		for (size_t i=0; i<layers.back().size(); ++i) {
			layers.back()[i].resize(h);
		}
		layernames.push_back(infile.val);
	}
	else if (infile.key == "format") {
		// @ATTR layer.format|string|Format for map layer, must be 'dec'
		if (infile.val != "dec") {
			infile.error("Map: The format of a layer must be 'dec'!");
			if (exit_on_fail) {
				Utils::logErrorDialog("Map: The format of a layer must be 'dec'!");
				mods->resetModConfig();
				Utils::Exit(1);
			}
			return false;
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
				if (exit_on_fail) {
					mods->resetModConfig();
					Utils::Exit(1);
				}
				return false;
			}

			for (int i=0; i<w; i++)
				layers.back()[i][j] = static_cast<unsigned short>(Parse::popFirstInt(val));
		}
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}

	return true;
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
		// @ATTR enemygroup.chance|float|Initial percentage chance that this enemy group will be able to spawn enemies.
		group->chance = std::min(100.0f, std::max(0.0f, Parse::popFirstFloat(infile.val)));
	}
	else if (infile.key == "direction") {
		// @ATTR enemygroup.direction|direction|Direction that enemies will initially face.
		group->direction = Parse::toDirection(infile.val);
	}
	else if (infile.key == "waypoints") {
		// @ATTR enemygroup.waypoints|list(point)|Enemy waypoints; single enemy only; negates wander_radius
		std::string a = Parse::popFirstString(infile.val);
		std::string b = Parse::popFirstString(infile.val);

		while (!a.empty()) {
			FPoint p;
			p.x = static_cast<float>(Parse::toInt(a)) + 0.5f;
			p.y = static_cast<float>(Parse::toInt(b)) + 0.5f;
			group->waypoints.push_back(p);
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
		group->waypoints.clear();
	}
	else if (infile.key == "requires_status") {
		// @ATTR enemygroup.requires_status|list(string)|Statuses required to be set for enemy group to load
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_STATUS;
			ec.status = camp->registerStatus(s);
			group->requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR enemygroup.requires_not_status|list(string)|Statuses required to be unset for enemy group to load
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_NOT_STATUS;
			ec.status = camp->registerStatus(s);
			group->requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_level") {
		// @ATTR enemygroup.requires_level|int|Player level must be equal or greater to load enemy group
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_LEVEL;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		group->requirements.push_back(ec);
	}
	else if (infile.key == "requires_not_level") {
		// @ATTR enemygroup.requires_not_level|int|Player level must be lesser to load enemy group
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_NOT_LEVEL;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		group->requirements.push_back(ec);
	}
	else if (infile.key == "requires_currency") {
		// @ATTR enemygroup.requires_currency|int|Player currency must be equal or greater to load enemy group
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_CURRENCY;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		group->requirements.push_back(ec);
	}
	else if (infile.key == "requires_not_currency") {
		// @ATTR enemygroup.requires_not_currency|int|Player currency must be lesser to load enemy group
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_NOT_CURRENCY;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		group->requirements.push_back(ec);
	}
	else if (infile.key == "requires_item") {
		// @ATTR enemygroup.requires_item|list(item_id)|Item required to exist in player inventory to load enemy group. Quantity can be specified by appending ":Q" to the item_id, where Q is an integer.
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			ItemStack item_stack = Parse::toItemQuantityPair(s);
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_ITEM;
			ec.id = item_stack.item;
			ec.data[0].Int = item_stack.quantity;
			group->requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_not_item") {
		// @ATTR enemygroup.requires_not_item|list(item_id)|Item required to not exist in player inventory to load enemy group. Quantity can be specified by appending ":Q" to the item_id, where Q is an integer.
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			ItemStack item_stack = Parse::toItemQuantityPair(s);
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_NOT_ITEM;
			ec.id = item_stack.item;
			ec.data[0].Int = item_stack.quantity;
			group->requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_class") {
		// @ATTR enemygroup.requires_class|predefined_string|Player base class required to load enemy group
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_CLASS;
		ec.s = Parse::popFirstString(infile.val);
		group->requirements.push_back(ec);
	}
	else if (infile.key == "requires_not_class") {
		// @ATTR enemygroup.requires_not_class|predefined_string|Player base class not required to load enemy group
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_NOT_CLASS;
		ec.s = Parse::popFirstString(infile.val);
		group->requirements.push_back(ec);
	}
	else if (infile.key == "invincible_requires_status") {
		// @ATTR enemygroup.invincible_requires_status|list(string)|Enemies in this group are invincible to hero attacks when these statuses are set.
		std::string s;
		while ((s = Parse::popFirstString(infile.val)) != "") {
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_STATUS;
			ec.status = camp->registerStatus(s);
			group->invincible_requirements.push_back(ec);
		}
	}
	else if (infile.key == "invincible_requires_not_status") {
		// @ATTR enemygroup.invincible_requires_not_status|list(string)|Enemies in this group are invincible to hero attacks when these statuses are not set.
		std::string s;
		while ((s = Parse::popFirstString(infile.val)) != "") {
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_NOT_STATUS;
			ec.status = camp->registerStatus(s);
			group->invincible_requirements.push_back(ec);
		}
	}
	else if (infile.key == "spawn_level") {
		// @ATTR enemygroup.spawn_level|["default", "fixed", "source_level", "source_stat", "hero_level", "hero_stat"], float, predefined_string : Mode, Multiplier, Primary stat|The level of spawned creatures. The need for the last two parameters depends on the mode being used. The "default" mode will just use the entity's normal level and doesn't require any additional parameters. The "fixed" mode sets the multiplier as the enemy level. The level modes multiply with the target's level. The stat modes multiply by one of the target's primary stats. The stat is defined with the last parameter, which is simply the ID of the primary stat that should be used for scaling. Because the map has no level/stats of its own, the source modes use the hero's level/stats.
		group->spawn_level.parse(infile);
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

void Map::loadNPC(FileParser &infile) {
	if (infile.key == "type") {
		// @ATTR npc.type|string|(IGNORED BY ENGINE) The "type" field, as used by Tiled and other mapping tools.
		map_npcs.back().type = infile.val;
	}
	else if (infile.key == "filename") {
		// @ATTR npc.filename|filename|Filename of an NPC definition.
		map_npcs.back().id = infile.val;
	}
	else if (infile.key == "location") {
		// @ATTR npc.location|point|Location of NPC
		map_npcs.back().pos.x = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
		map_npcs.back().pos.y = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
	}
	else if (infile.key == "requires_status") {
		// @ATTR npc.requires_status|list(string)|Statuses required to be set for NPC load
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_STATUS;
			ec.status = camp->registerStatus(s);
			map_npcs.back().requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR npc.requires_not_status|list(string)|Statuses required to be unset for NPC load
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_NOT_STATUS;
			ec.status = camp->registerStatus(s);
			map_npcs.back().requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_level") {
		// @ATTR npc.requires_level|int|Player level must be equal or greater to load NPC
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_LEVEL;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		map_npcs.back().requirements.push_back(ec);
	}
	else if (infile.key == "requires_not_level") {
		// @ATTR npc.requires_not_level|int|Player level must be lesser to load NPC
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_NOT_LEVEL;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		map_npcs.back().requirements.push_back(ec);
	}
	else if (infile.key == "requires_currency") {
		// @ATTR npc.requires_currency|int|Player currency must be equal or greater to load NPC
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_CURRENCY;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		map_npcs.back().requirements.push_back(ec);
	}
	else if (infile.key == "requires_not_currency") {
		// @ATTR npc.requires_not_currency|int|Player currency must be lesser to load NPC
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_NOT_CURRENCY;
		ec.data[0].Int = Parse::popFirstInt(infile.val);
		map_npcs.back().requirements.push_back(ec);
	}
	else if (infile.key == "requires_item") {
		// @ATTR npc.requires_item|list(item_id)|Item required to exist in player inventory to load NPC. Quantity can be specified by appending ":Q" to the item_id, where Q is an integer.
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			ItemStack item_stack = Parse::toItemQuantityPair(s);
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_ITEM;
			ec.id = item_stack.item;
			ec.data[0].Int = item_stack.quantity;
			map_npcs.back().requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_not_item") {
		// @ATTR npc.requires_not_item|list(item_id)|Item required to not exist in player inventory to load NPC. Quantity can be specified by appending ":Q" to the item_id, where Q is an integer.
		std::string s;
		while ( (s = Parse::popFirstString(infile.val)) != "") {
			ItemStack item_stack = Parse::toItemQuantityPair(s);
			EventComponent ec;
			ec.type = EventComponent::REQUIRES_NOT_ITEM;
			ec.id = item_stack.item;
			ec.data[0].Int = item_stack.quantity;
			map_npcs.back().requirements.push_back(ec);
		}
	}
	else if (infile.key == "requires_class") {
		// @ATTR npc.requires_class|predefined_string|Player base class required to load NPC
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_CLASS;
		ec.s = Parse::popFirstString(infile.val);
		map_npcs.back().requirements.push_back(ec);
	}
	else if (infile.key == "requires_not_class") {
		// @ATTR npc.requires_not_class|predefined_string|Player base class not required to load NPC
		EventComponent ec;
		ec.type = EventComponent::REQUIRES_NOT_CLASS;
		ec.s = Parse::popFirstString(infile.val);
		map_npcs.back().requirements.push_back(ec);
	}
	else if (infile.key == "direction") {
		// @ATTR npc.direction|direction|Direction that NPC will initially face.
		map_npcs.back().direction = Parse::toDirection(infile.val);
	}
	else if (infile.key == "waypoints") {
		// @ATTR npc.waypoints|list(point)|NPC waypoints; negates wander_radius
		std::string a = Parse::popFirstString(infile.val);
		std::string b = Parse::popFirstString(infile.val);

		while (!a.empty()) {
			FPoint p;
			p.x = static_cast<float>(Parse::toInt(a)) + 0.5f;
			p.y = static_cast<float>(Parse::toInt(b)) + 0.5f;
			map_npcs.back().waypoints.push_back(p);
			a = Parse::popFirstString(infile.val);
			b = Parse::popFirstString(infile.val);
		}

		// disable wander radius, since we can't have waypoints and wandering at the same time
		map_npcs.back().wander_radius = 0;
	}
	else if (infile.key == "wander_radius") {
		// @ATTR npc.wander_radius|int|The radius (in tiles) that an NPC will wander around randomly; negates waypoints
		map_npcs.back().wander_radius = std::max(0, Parse::popFirstInt(infile.val));

		// clear waypoints, since wandering will use the waypoint queue
		map_npcs.back().waypoints.clear();
	}
	else {
		infile.error("Map: '%s' is not a valid key.", infile.key.c_str());
	}
}

int Map::addEventStatBlock(Event &evnt) {
	statblocks.push_back(StatBlock());
	StatBlock *statb = &statblocks.back();

	EventComponent *ec_power_stats = evnt.getComponent(EventComponent::POWER_STATS);
	if (ec_power_stats) {
		statb->load(ec_power_stats->s);
	}

	EventComponent *ec_power_level = evnt.getComponent(EventComponent::POWER_LEVEL);
	if (ec_power_level) {
		SpawnLevel sl;
		sl.parseString(ec_power_level->s);
		sl.applyToStatBlock(statb, &pc->stats);
	}

	statb->perfect_accuracy = true; // never miss AND never overhit

	EventComponent *ec_path = evnt.getComponent(EventComponent::POWER_PATH);
	if (ec_path) {
		// source is power path start
		statb->pos.x = static_cast<float>(ec_path->data[0].Int) + 0.5f;
		statb->pos.y = static_cast<float>(ec_path->data[1].Int) + 0.5f;
	}
	else {
		// source is event location
		statb->pos.x = static_cast<float>(evnt.location.x) + 0.5f;
		statb->pos.y = static_cast<float>(evnt.location.y) + 0.5f;
	}

	EventComponent *ec_damage = evnt.getComponent(EventComponent::POWER_DAMAGE);
	if (ec_damage) {
		for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
			if (!eset->damage_types.list[i].is_elemental) {
				statb->starting[Stats::COUNT + eset->damage_types.indexToMin(i)] = ec_damage->data[0].Float; // min
				statb->starting[Stats::COUNT + eset->damage_types.indexToMax(i)] = ec_damage->data[1].Float; // min
			}
		}
	}

	// this is used to store cooldown ticks for a map power
	// the power id, type, etc are not used
	statb->powers_ai.clear();
	statb->powers_ai.resize(1);

	// make this StatBlock immune to negative status effects
	// this is mostly to prevent a player with a damage return bonus from damaging this StatBlock
	// create a temporary EffectDef for immunity; will be used for map StatBlocks
	EffectDef immunity_effect;
	immunity_effect.id = "MAP_EVENT_IMMUNITY";
	immunity_effect.type = Effect::RESIST_ALL;

	EffectParams immunity_params;
	immunity_params.magnitude = 100;
	immunity_params.source_type = Power::SOURCE_TYPE_ENEMY;

	statb->effects.addEffect(statb, immunity_effect, immunity_params);

	// ensure the statblock will be alive
	statb->hp = statb->starting[Stats::HP_MAX] = statb->current[Stats::HP_MAX] = 1;

	// ensure that stats are ready to be used by running logic once
	statb->logic();

	return static_cast<int>(statblocks.size())-1;
}

Chunk* Map::procGenWalkSingle(int path_type, size_t cur_x, size_t cur_y, int walk_x, int walk_y) {
	if (procgen_chunks.empty())
		return NULL;

	size_t MAP_SIZE_X = procgen_chunks[0].size();
	size_t MAP_SIZE_Y = procgen_chunks.size();

	if (cur_x >= MAP_SIZE_X || cur_y >= MAP_SIZE_Y)
		return NULL; // NOTE: should we always return a valid chunk instead?

	Chunk* prev = &procgen_chunks[cur_y][cur_x];

	if ((walk_x < 0 && cur_x == 0) || (walk_y < 0 && cur_y == 0))
		return prev;
	else if ((cur_x + walk_x >= MAP_SIZE_X) || (cur_y + walk_y >= MAP_SIZE_Y))
		return prev;

	Chunk* next = &procgen_chunks[cur_y + walk_y][cur_x + walk_x];

	if (next->type == Chunk::TYPE_START)
		return prev;
	else if (next->type == Chunk::TYPE_END)
		return prev;
	else if (next->type == Chunk::TYPE_DOOR_NORTH_SOUTH || next->type == Chunk::TYPE_DOOR_WEST_EAST)
		return prev;

	// walking the main path doesn't allow overlapping existing path
	// branches, on the other hand, can overlap if their door level is the same
	if (path_type == PATH_MAIN) {
		if (next->type != Chunk::TYPE_EMPTY)
			return prev;
	}

	int link_count = 0;
	for (int i = 0; i < Chunk::LINK_COUNT; ++i) {
		if (next->links[i])
			link_count++;
	}

	if (link_count > 0 && (next->door_level != prev->door_level || rand() % link_count > 0))
		return prev;

	return next;
}

int Map::procGenCreatePath(int path_type, int desired_length, int* _door_count, size_t start_x, size_t start_y) {
	if (procgen_chunks.empty())
		return 0;

	size_t MAP_SIZE_X = procgen_chunks[0].size();
	size_t MAP_SIZE_Y = procgen_chunks.size();

	if (path_type == PATH_MAIN) {
		// zero out the map data
		for (size_t i = 0; i < MAP_SIZE_Y; ++i) {
			for (size_t j = 0; j < MAP_SIZE_X; ++j) {
				Chunk* chunk = &procgen_chunks[i][j];
				chunk->type = Chunk::TYPE_EMPTY;
				chunk->door_level = 0;
				for (size_t k = 0; k < Chunk::LINK_COUNT; ++k) {
					chunk->links[k] = NULL;
				}
			}
		}

		procgen_branch_roots.clear();
	}

	// set start tile
	size_t cur_x = 0;
	size_t cur_y = 0;
	if (path_type == PATH_MAIN) {
		cur_x = rand() % MAP_SIZE_X;
		cur_y = rand() % MAP_SIZE_Y;
	}
	else {
		cur_x = start_x;
		cur_y = start_y;
	}
	Chunk* current = &(procgen_chunks[cur_y][cur_x]);

	if (path_type == PATH_MAIN)
		current->type = Chunk::TYPE_START;

	int steps = desired_length;

	int door_count = 0;
	int door_chance = 0;
	int door_dist = 0;
	int door_min_dist = procgen_doors_max > 0 ? std::max(2, desired_length / procgen_doors_max) : 0;
	if (procgen_door_spacing_min > 0) {
		door_min_dist = std::max(2, procgen_door_spacing_min);
	}
	int path_length = 1;
	int branch_chance = 50;
	int branch_count = 0;
	int max_branches = procgen_branches_per_door_level_max;

	while (steps >= 0) {
		int link = rand() % Chunk::LINK_COUNT;
		Chunk* prev = current;

		int link_rotate = Chunk::LINK_COUNT;
		while (current->links[link] && link_rotate > 0) {
			link++;
			if (link >= Chunk::LINK_COUNT) {
				link = 0;
			}
			link_rotate--;
		}

		// if we're on a door chunk, we can only go in the direction opposite of the initial link
		if (current->type == Chunk::TYPE_DOOR_NORTH_SOUTH && (link == Chunk::LINK_WEST || link == Chunk::LINK_EAST)) {
			if (current->links[Chunk::LINK_NORTH])
				link = Chunk::LINK_SOUTH;
			else if (current->links[Chunk::LINK_SOUTH])
				link = Chunk::LINK_NORTH;
		}
		else if (current->type == Chunk::TYPE_DOOR_WEST_EAST && (link == Chunk::LINK_NORTH || link == Chunk::LINK_SOUTH)) {
			if (current->links[Chunk::LINK_WEST])
				link = Chunk::LINK_EAST;
			else if (current->links[Chunk::LINK_EAST])
				link = Chunk::LINK_WEST;
		}

		switch (link) {
			case Chunk::LINK_NORTH:
				current = procGenWalkSingle(path_type, cur_x, cur_y, 0, -1);
				if (current != prev) {
					prev->links[Chunk::LINK_NORTH] = current;
					current->links[Chunk::LINK_SOUTH] = prev;
					current->type = Chunk::TYPE_NORMAL;
					cur_y--;
					door_dist++;
					path_length++;
				}
				break;
			case Chunk::LINK_SOUTH:
				current = procGenWalkSingle(path_type, cur_x, cur_y, 0, 1);
				if (current != prev) {
					prev->links[Chunk::LINK_SOUTH] = current;
					current->links[Chunk::LINK_NORTH] = prev;
					current->type = Chunk::TYPE_NORMAL;
					cur_y++;
					door_dist++;
					path_length++;
				}
				break;
			case Chunk::LINK_WEST:
				current = procGenWalkSingle(path_type, cur_x, cur_y, 1, 0);
				if (current != prev) {
					prev->links[Chunk::LINK_EAST] = current;
					current->links[Chunk::LINK_WEST] = prev;
					current->type = Chunk::TYPE_NORMAL;
					cur_x++;
					door_dist++;
					path_length++;
				}
				break;
			case Chunk::LINK_EAST:
				current = procGenWalkSingle(path_type, cur_x, cur_y, -1, 0);
				if (current != prev) {
					prev->links[Chunk::LINK_WEST] = current;
					current->links[Chunk::LINK_EAST] = prev;
					current->type = Chunk::TYPE_NORMAL;
					cur_x--;
					door_dist++;
					path_length++;
				}
				break;
		};

		if (current != prev) {
			if (path_type == PATH_MAIN) {
				if (prev->type != Chunk::TYPE_START && prev->isStraight() && door_count < procgen_doors_max && rand() % 100 < door_chance) {
					if (prev->links[Chunk::LINK_NORTH] && prev->links[Chunk::LINK_SOUTH])
						prev->type = Chunk::TYPE_DOOR_NORTH_SOUTH;
					else if (prev->links[Chunk::LINK_WEST] && prev->links[Chunk::LINK_EAST])
						prev->type = Chunk::TYPE_DOOR_WEST_EAST;
					else {
						// shouldn't be able to get here!
						Utils::logError("Map: Trying to place door chunk, but links don't form a straight path.");
					}
					prev->door_level++;
					current->door_level++;
					door_chance = 0;
					door_count++;
					door_dist = 0;
					branch_count = 0;
				}

				if (door_dist > door_min_dist) {
					if (door_count == 0)
						door_chance = 100;
					else
						door_chance += 5;
				}

				if (current->type == Chunk::TYPE_NORMAL && branch_count < max_branches && rand() % 100 < branch_chance) {
					procgen_branch_roots.push_back(Point(static_cast<int>(cur_x), static_cast<int>(cur_y)));
					branch_chance = 50;
					branch_count++;
				}

				branch_chance += 5;
			}

			current->door_level = prev->door_level;
		}

		steps--;
	}

	// last room is the end
	if (path_type == PATH_MAIN)
		current->type = Chunk::TYPE_END;

	if (_door_count)
		*_door_count = door_count;

	return path_length;
}

void Map::procGenFillArea(const std::string& config_filename, const Rect& area) {
	Utils::logInfo("Procgen filename: %s", config_filename.c_str());

	size_t MAP_SIZE_X = 0;
	size_t MAP_SIZE_Y = 0;

	int main_path_length_min = 0;
	int main_path_length_max = 0;
	int main_path_attempts_max = 0;
	int branch_length_min = 0;
	int branch_length_max = 0;

	std::vector<std::string> chunk_filenames;

	// @CLASS Map: Procedural generation rules|Description of maps/procgen_rules
	FileParser infile;
	if (infile.open(config_filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.section == "settings") {
				if (infile.key == "doors_max") {
					// @ATTR settings.doors_max|int|Maximum number of door chunks that can be placed in this region.
					procgen_doors_max = Parse::toInt(infile.val);
				}
				else if (infile.key == "main_path_length_min") {
					// @ATTR settings.main_path_length_min|int|Minimum length of the 'main' path (start chunk to end chunk).
					main_path_length_min = Parse::toInt(infile.val);
				}
				else if (infile.key == "main_path_length_max") {
					// @ATTR settings.main_path_length_max|int|Maximum length of the 'main' path (start chunk to end chunk).
					main_path_length_max = Parse::toInt(infile.val);
				}
				else if (infile.key == "main_path_attempts_max") {
					// @ATTR settings.main_path_attempts_max|int|Maximum number of attempts to generate the main path to match the min/max constraints.
					main_path_attempts_max = Parse::toInt(infile.val);
				}
				else if (infile.key == "branch_length_min") {
					// @ATTR settings.branch_length_min|int|Minimum number of steps taken when creating a branch off the main path.
					branch_length_min = Parse::toInt(infile.val);
				}
				else if (infile.key == "branch_length_max") {
					// @ATTR settings.branch_length_max|int|Maximum number of steps taken when creating a branch off the main path.
					branch_length_max = Parse::toInt(infile.val);
				}
				else if (infile.key == "branches_per_door_level_max") {
					// @ATTR settings.branches_per_door_level_max|int|Maximum number of branches that can be created between each 'door level'. A door level is defined as the set of chunks between a set of 2 door chunks (start and end chunks count as doors here).
					procgen_branches_per_door_level_max = Parse::toInt(infile.val);
				}
				else if (infile.key == "door_spacing_min") {
					// @ATTR settings.door_spacing_min|int|The minimum number of chunk length of each door level.
					procgen_door_spacing_min = Parse::toInt(infile.val);
				}
			}
			else if (infile.section == "chunks") {
				if (infile.key == "filename") {
					// @ATTR chunks.filename|repeatable(filename)|Map chunk file.
					chunk_filenames.push_back(infile.val);
				}
			}
		}
		infile.close();
	}

	// turn fog-of-war off when loading map chunks
	bool temp_fow = eset->misc.fogofwar;
	eset->misc.fogofwar = false;

	std::vector< std::vector<Map*> > chunk_maps;
	chunk_maps.resize(Chunk::TYPE_COUNT);

	for (size_t i = 0; i < chunk_filenames.size(); ++i) {
		Map *chunk_map = new Map();
		if (chunk_map) {
			chunk_map->load(chunk_filenames[i]);

			if (MAP_SIZE_X == 0 && chunk_map->w > 0 && chunk_map->h > 0) {
				MAP_SIZE_X = area.w / chunk_map->w;
				MAP_SIZE_Y = area.h / chunk_map->h;
				Utils::logInfo("Map: Set procedural generation region to %lux%lu chunks. Each chunk is %lux%lu.", MAP_SIZE_X, MAP_SIZE_Y, chunk_map->w, chunk_map->h);
			}

			if (chunk_map->procgen_type != Chunk::TYPE_EMPTY)
				chunk_maps[chunk_map->procgen_type].push_back(chunk_map);
		}
	}

	Rect link_rects[4];

	if (!chunk_maps[Chunk::TYPE_LINKS].empty()) {
		// the link rects are only taken from the first link chunk
		Map* chunk_links = chunk_maps[Chunk::TYPE_LINKS][0];

		for (size_t i = 0; i < chunk_links->events.size(); ++i) {
			EventComponent *ec = chunk_links->events[i].getComponent(EventComponent::PROCGEN_LINK);
			if (ec) {
				if (ec->s == "north") {
					link_rects[Chunk::LINK_NORTH] = chunk_links->events[i].location;
				}
				else if (ec->s == "south") {
					link_rects[Chunk::LINK_SOUTH] = chunk_links->events[i].location;
				}
				else if (ec->s == "west") {
					link_rects[Chunk::LINK_WEST] = chunk_links->events[i].location;
				}
				else if (ec->s == "east") {
					link_rects[Chunk::LINK_EAST] = chunk_links->events[i].location;
				}
			}
		}
	}

	eset->misc.fogofwar = temp_fow;

	//
	// BEGIN CHUNK GENERATION
	//

	procgen_chunks.clear();
	procgen_chunks.resize(MAP_SIZE_Y, std::vector<Chunk>(MAP_SIZE_X));

	std::vector< std::vector<Chunk*> > normal_chunks; // used to find chunks to change to key, treasure, etc.
	procgen_branch_roots.clear();

	normal_chunks.resize(procgen_doors_max);

	int main_path_length = 0;
	main_path_length_min = main_path_length_min == 0 ? static_cast<int>((MAP_SIZE_X * MAP_SIZE_Y) / 2) : std::max(3, main_path_length_min);
	main_path_length_max = main_path_length_max == 0 ? static_cast<int>(MAP_SIZE_X * MAP_SIZE_Y) : std::max(main_path_length_min, main_path_length_max);
	int gen_attempts = 0;
	int door_count = 0;

	int desired_main_path_length = Math::randBetween(main_path_length_min, main_path_length_max);

	while ((main_path_length < main_path_length_min || main_path_length > main_path_length_max || (door_count == 0 && procgen_doors_max > 0)) && gen_attempts < main_path_attempts_max) {
		main_path_length = procGenCreatePath(PATH_MAIN, desired_main_path_length, &door_count, 0, 0);

		gen_attempts++;
	}
	Utils::logInfo("Map: Generated main path with length=%d and branches=%lu. Generator attempts: %d", main_path_length, procgen_branch_roots.size(), gen_attempts);

	for (size_t i = 0; i < procgen_branch_roots.size(); ++i) {
		int branch_length = Math::randBetween(branch_length_min, std::min(branch_length_min, branch_length_max));
		procGenCreatePath(PATH_BRANCH, branch_length, NULL, procgen_branch_roots[i].x, procgen_branch_roots[i].y);
		// map_chunks[branch_roots[i].second][branch_roots[i].first].type = Chunk::TYPE_BRANCH;
	}

	Point start_chunk;

	// gather all normal chunks for placing keys/treasure/etc.
	for (size_t i = 0; i < MAP_SIZE_Y; ++i) {
		for (size_t j = 0; j < MAP_SIZE_X; ++j) {
			Chunk* chunk = &procgen_chunks[i][j];

			if (chunk->type == Chunk::TYPE_NORMAL && static_cast<size_t>(chunk->door_level) < normal_chunks.size()) {
				normal_chunks[chunk->door_level].push_back(chunk);
			}
			else if (chunk->type == Chunk::TYPE_START) {
				start_chunk.x = static_cast<int>(j);
				start_chunk.y = static_cast<int>(i);
			}
		}
	}

	for (size_t i = 0; i < static_cast<size_t>(door_count); ++i) {
		if (i >= normal_chunks.size())
			break;

		if (normal_chunks[i].empty()) {
			Utils::logError("Map: Generator tried to place key, but no chunks on current door level: %d", i);
			continue;
		}

		size_t chunk_index = rand() % normal_chunks[i].size();
		Chunk* chunk = normal_chunks[i][chunk_index];
		chunk->type = Chunk::TYPE_KEY;
		normal_chunks[i].erase(normal_chunks[i].begin() + chunk_index);
	}

	std::vector<size_t> valid_chunks;

	for (size_t chunk_y = 0; chunk_y < procgen_chunks.size(); ++chunk_y) {
		for (size_t chunk_x = 0; chunk_x < procgen_chunks[chunk_y].size(); ++chunk_x) {
			Chunk* chunk = &procgen_chunks[chunk_y][chunk_x];
			if (chunk->type == Chunk::TYPE_EMPTY)
				continue;

			if (chunk_maps[chunk->type].empty())
				continue;

			valid_chunks.clear();
			for (size_t i = 0; i < chunk_maps[chunk->type].size(); ++i) {
				Map* test_map = chunk_maps[chunk->type][i];
				if (test_map->procgen_unique_status_id > 0 && camp->checkStatus(test_map->procgen_unique_status_id))
					continue;

				valid_chunks.push_back(i);
			}

			if (valid_chunks.empty()) {
				Utils::logError("Map: Generator unable to find valid chunk for region: (%lu, %lu)", chunk_x, chunk_y);
				continue;
			}

			size_t valid_chunk_index = rand() % valid_chunks.size();

			// attempt to reduce the occurrences of the same room variants being connected to each other
			if (chunk->links[Chunk::LINK_WEST] && chunk->type == procgen_chunks[chunk_y][chunk_x-1].type && valid_chunks[valid_chunk_index] == procgen_chunks[chunk_y][chunk_x-1].variant) {
				valid_chunk_index++;
				if (valid_chunk_index >= valid_chunks.size())
					valid_chunk_index = 0;
			}
			if (chunk->links[Chunk::LINK_NORTH] && chunk->type == procgen_chunks[chunk_y-1][chunk_x].type && valid_chunks[valid_chunk_index] == procgen_chunks[chunk_y-1][chunk_x].variant) {
				valid_chunk_index++;
				if (valid_chunk_index >= valid_chunks.size())
					valid_chunk_index = 0;
			}

			chunk->variant = valid_chunks[valid_chunk_index];

			Map* chunk_map = chunk_maps[chunk->type][chunk->variant];

			if (chunk_map->procgen_unique_status_id > 0) {
				camp->setStatus(chunk_map->procgen_unique_status_id);
			}

			size_t link_chunk_variant = 0;
			if (chunk_maps[Chunk::TYPE_LINKS].size() > 1)
				link_chunk_variant = rand() % chunk_maps[Chunk::TYPE_LINKS].size();

			Map* chunk_map_links = chunk_maps[Chunk::TYPE_LINKS][link_chunk_variant];

			size_t x_offset = chunk_x * chunk_map->w;
			size_t y_offset = chunk_y * chunk_map->h;

			if (chunk->type == Chunk::TYPE_START) {
				hero_pos.x = chunk_map->hero_pos.x + static_cast<float>(x_offset);
				hero_pos.y = chunk_map->hero_pos.y + static_cast<float>(y_offset);
			}

			for (size_t layer_index = 0; layer_index < chunk_map->layers.size(); ++layer_index) {
				copyTileLayer(chunk_map, layer_index, 0, 0, 0, 0, x_offset, y_offset);

				// copy tile layers for links
				if (chunk->links[Chunk::LINK_NORTH]) {
					Rect *r = &link_rects[Chunk::LINK_NORTH];
					copyTileLayer(chunk_map_links, layer_index, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
				}
				if (chunk->links[Chunk::LINK_SOUTH]) {
					Rect *r = &link_rects[Chunk::LINK_SOUTH];
					copyTileLayer(chunk_map_links, layer_index, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
				}
				if (chunk->links[Chunk::LINK_WEST]) {
					Rect *r = &link_rects[Chunk::LINK_WEST];
					copyTileLayer(chunk_map_links, layer_index, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
				}
				if (chunk->links[Chunk::LINK_EAST]) {
					Rect *r = &link_rects[Chunk::LINK_EAST];
					copyTileLayer(chunk_map_links, layer_index, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
				}
			}

			copyMapObjects(chunk_map, chunk, 0, 0, 0, 0, x_offset, y_offset);

			if (chunk->links[Chunk::LINK_NORTH]) {
				Rect *r = &link_rects[Chunk::LINK_NORTH];
				copyMapObjects(chunk_map_links, chunk, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
			}
			if (chunk->links[Chunk::LINK_SOUTH]) {
				Rect *r = &link_rects[Chunk::LINK_SOUTH];
				copyMapObjects(chunk_map_links, chunk, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
			}
			if (chunk->links[Chunk::LINK_WEST]) {
				Rect *r = &link_rects[Chunk::LINK_WEST];
				copyMapObjects(chunk_map_links, chunk, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
			}
			if (chunk->links[Chunk::LINK_EAST]) {
				Rect *r = &link_rects[Chunk::LINK_EAST];
				copyMapObjects(chunk_map_links, chunk, r->x, r->y, r->x + r->w, r->y + r->h, x_offset, y_offset);
			}
		}
	}

	// cleanup
	for (size_t i = 0; i < chunk_maps.size(); ++i) {
		for (size_t j = 0; j < chunk_maps[i].size(); ++j) {
			delete chunk_maps[i][j];
		}
	}
}

void Map::copyTileLayer(Map* src, size_t layer_index, size_t src_x, size_t src_y, size_t src_w, size_t src_h, size_t x_offset, size_t y_offset) {
	if (layer_index >= layers.size() || layer_index >= src->layers.size())
		return;

	if (src_w == 0)
		src_w = src->layers[layer_index].size();
	if (src_h == 0)
		src_h = src->layers[layer_index].size();

	for (size_t x = src_x; x < src_w; ++x) {
		if (x + x_offset >= w)
			continue;

		for (size_t y = src_y; y < src_h; ++y) {
			if (y + y_offset >= h)
				continue;

			layers[layer_index][x + x_offset][y + y_offset] = src->layers[layer_index][x][y];
		}
	}
}

void Map::copyMapObjects(Map* src, Chunk* chunk, size_t src_x, size_t src_y, size_t src_w, size_t src_h, size_t x_offset, size_t y_offset) {
	if (src_w == 0)
		src_w = src->w;
	if (src_h == 0)
		src_h = src->h;

	// copy and apply x/y offset to events
	for (size_t event_index = 0; event_index < src->events.size(); ++event_index) {
		Event event = src->events[event_index];

		if (static_cast<size_t>(event.location.x) < src_x || static_cast<size_t>(event.location.y) < src_y || static_cast<size_t>(event.location.x) > src_x + src_w || static_cast<size_t>(event.location.y) > src_y + src_h)
			continue;

		event.location.x += static_cast<int>(x_offset);
		event.location.y += static_cast<int>(y_offset);
		event.hotspot.x += static_cast<int>(x_offset);
		event.hotspot.y += static_cast<int>(y_offset);
		event.center.x += static_cast<float>(x_offset);
		event.center.y += static_cast<float>(y_offset);
		event.reachable_from.x += static_cast<int>(x_offset);
		event.reachable_from.y += static_cast<int>(y_offset);

		bool check_required_door_level = false;
		bool event_matched_door_level = false;
		bool event_is_procgen = false;

		for (size_t ec_index = 0; ec_index < event.components.size(); ++ec_index) {
			EventComponent* ec = &event.components[ec_index];

			if (ec->type == EventComponent::PROCGEN_LINK || ec->type == EventComponent::PROCGEN_FILENAME) {
				event_is_procgen = true;
				break;
			}
			else if (ec->type == EventComponent::POWER_PATH) {
				ec->data[0].Int += static_cast<int>(x_offset);
				ec->data[1].Int += static_cast<int>(y_offset);
				if (!ec->data[4].Bool) {
					ec->data[2].Int += static_cast<int>(x_offset);
					ec->data[3].Int += static_cast<int>(y_offset);
				}
			}
			else if (ec->type == EventComponent::INTRAMAP) {
				ec->data[0].Int += static_cast<int>(x_offset);
				ec->data[1].Int += static_cast<int>(y_offset);
			}
			else if (ec->type == EventComponent::MAPMOD) {
				ec->data[0].Int += static_cast<int>(x_offset);
				ec->data[1].Int += static_cast<int>(y_offset);
			}
			else if (ec->type == EventComponent::MAPMOD_TOGGLE) {
				ec->data[0].Int += static_cast<int>(x_offset);
				ec->data[1].Int += static_cast<int>(y_offset);
			}
			else if (ec->type == EventComponent::SOUNDFX) {
				// make sure we handle sounds that aren't positional
				if (!(ec->data[0].Int == -1 && ec->data[1].Int == -1) && !(ec->data[0].Int == 0 && ec->data[1].Int == 0)) {
					ec->data[0].Int += static_cast<int>(x_offset);
					ec->data[1].Int += static_cast<int>(y_offset);
				}
			}
			else if (ec->type == EventComponent::SPAWN) {
				ec->data[0].Int += static_cast<int>(x_offset);
				ec->data[1].Int += static_cast<int>(y_offset);
			}
			else if (ec->type == EventComponent::PROCGEN_DOOR_LEVEL) {
				check_required_door_level = true;
				if (chunk->door_level == ec->data[0].Int)
					event_matched_door_level = true;
			}
			else if (ec->type == EventComponent::REQUIRES_TILE) {
				ec->data[0].Int += static_cast<int>(x_offset);
				ec->data[1].Int += static_cast<int>(y_offset);
			}
			else if (ec->type == EventComponent::REQUIRES_NOT_TILE) {
				ec->data[0].Int += static_cast<int>(x_offset);
				ec->data[1].Int += static_cast<int>(y_offset);
			}
		}

		if (!event_is_procgen && (!check_required_door_level || event_matched_door_level))
			events.push_back(event);
	}

	for (size_t egroup_index = 0; egroup_index < src->enemy_groups.size(); ++egroup_index) {
		Map_Group enemy_group = src->enemy_groups[egroup_index];

		if (static_cast<size_t>(enemy_group.pos.x) < src_x || static_cast<size_t>(enemy_group.pos.y) < src_y || static_cast<size_t>(enemy_group.pos.x) > src_x + src_w || static_cast<size_t>(enemy_group.pos.y) > src_y + src_h)
			continue;

		enemy_group.pos.x += static_cast<int>(x_offset);
		enemy_group.pos.y += static_cast<int>(y_offset);

		for (size_t waypoint_index = 0; waypoint_index < enemy_group.waypoints.size(); ++waypoint_index) {
			enemy_group.waypoints[waypoint_index].x += static_cast<float>(x_offset);
			enemy_group.waypoints[waypoint_index].y += static_cast<float>(y_offset);
		}

		enemy_groups.push_back(enemy_group);
	}

	for (size_t npc_index = 0; npc_index < src->map_npcs.size(); ++npc_index) {
		Map_NPC npc = src->map_npcs[npc_index];

		if (static_cast<size_t>(npc.pos.x) < src_x || static_cast<size_t>(npc.pos.y) < src_y || static_cast<size_t>(npc.pos.x) > src_x + src_w || static_cast<size_t>(npc.pos.y) > src_y + src_h)
			continue;

		npc.pos.x += static_cast<float>(x_offset);
		npc.pos.y += static_cast<float>(y_offset);

		for (size_t waypoint_index = 0; waypoint_index < npc.waypoints.size(); ++waypoint_index) {
			npc.waypoints[waypoint_index].x += static_cast<float>(x_offset);
			npc.waypoints[waypoint_index].y += static_cast<float>(y_offset);
		}

		map_npcs.push_back(npc);
	}
}


std::string Map::getProcgenFilename() {
	std::stringstream ss;
	ss.str("");
	ss << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << save_load->getGameSlot() << "/maps/" << Utils::hashString(filename) << ".txt";
	return ss.str();
}

std::string Map::getFOWFilename() {
	std::stringstream ss;
	ss.str("");
	ss << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << save_load->getGameSlot() << "/fow/" << Utils::hashString(filename) << ".txt";
	return ss.str();
}

bool Chunk::isStraight() {
	return ((links[LINK_NORTH] && links[LINK_SOUTH] && !links[LINK_WEST] && !links[LINK_EAST]) || (!links[LINK_NORTH] && !links[LINK_SOUTH] && links[LINK_WEST] && links[LINK_EAST]));
}

