/*
Copyright © 2015 Igor Paliychuk

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

#include "EngineSettings.h"
#include "EventManager.h"
#include "LootManager.h"
#include "MapSaver.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsFileSystem.h"

MapSaver::MapSaver(Map *_map) : map(_map) {
	EVENT_COMPONENT_NAME[EventComponent::TOOLTIP] = "tooltip";
	EVENT_COMPONENT_NAME[EventComponent::POWER] = "power";
	EVENT_COMPONENT_NAME[EventComponent::POWER_PATH] = "power_path";
	EVENT_COMPONENT_NAME[EventComponent::POWER_DAMAGE] = "power_damage";
	EVENT_COMPONENT_NAME[EventComponent::POWER_STATS] = "power_stats";
	EVENT_COMPONENT_NAME[EventComponent::POWER_LEVEL] = "power_level";
	EVENT_COMPONENT_NAME[EventComponent::INTERMAP] = "intermap";
	EVENT_COMPONENT_NAME[EventComponent::INTERMAP_ID] = "intermap_id";
	EVENT_COMPONENT_NAME[EventComponent::INTRAMAP] = "intramap";
	EVENT_COMPONENT_NAME[EventComponent::MAPMOD] = "mapmod";
	EVENT_COMPONENT_NAME[EventComponent::MAPMOD_TOGGLE] = "mapmod_toggle";
	EVENT_COMPONENT_NAME[EventComponent::SOUNDFX] = "soundfx";
	EVENT_COMPONENT_NAME[EventComponent::LOOT] = "loot"; // HALF-IMPLEMENTED
	EVENT_COMPONENT_NAME[EventComponent::LOOT_COUNT] = "loot_count";
	EVENT_COMPONENT_NAME[EventComponent::MSG] = "msg";
	EVENT_COMPONENT_NAME[EventComponent::SHAKYCAM] = "shakycam";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_STATUS] = "requires_status";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_NOT_STATUS] = "requires_not_status";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_LEVEL] = "requires_level";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_NOT_LEVEL] = "requires_not_level";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_CURRENCY] = "requires_currency";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_NOT_CURRENCY] = "requires_not_currency";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_ITEM] = "requires_item";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_NOT_ITEM] = "requires_not_item";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_CLASS] = "requires_class";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_NOT_CLASS] = "requires_not_class";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_TILE] = "requires_tile";
	EVENT_COMPONENT_NAME[EventComponent::REQUIRES_NOT_TILE] = "requires_not_tile";
	EVENT_COMPONENT_NAME[EventComponent::SET_STATUS] = "set_status";
	EVENT_COMPONENT_NAME[EventComponent::UNSET_STATUS] = "unset_status";
	EVENT_COMPONENT_NAME[EventComponent::REMOVE_CURRENCY] = "remove_currency";
	EVENT_COMPONENT_NAME[EventComponent::REMOVE_ITEM] = "remove_item";
	EVENT_COMPONENT_NAME[EventComponent::REWARD_XP] = "reward_xp";
	EVENT_COMPONENT_NAME[EventComponent::REWARD_CURRENCY] = "reward_currency";
	EVENT_COMPONENT_NAME[EventComponent::REWARD_ITEM] = "reward_item";
	EVENT_COMPONENT_NAME[EventComponent::REWARD_LOOT] = "reward_loot";
	EVENT_COMPONENT_NAME[EventComponent::REWARD_LOOT_COUNT] = "reward_loot_count";
	EVENT_COMPONENT_NAME[EventComponent::RESTORE] = "restore";
	EVENT_COMPONENT_NAME[EventComponent::SPAWN] = "spawn";
	EVENT_COMPONENT_NAME[EventComponent::SPAWN_LEVEL] = "spawn_level";
	EVENT_COMPONENT_NAME[EventComponent::STASH] = "stash";
	EVENT_COMPONENT_NAME[EventComponent::NPC] = "npc";
	EVENT_COMPONENT_NAME[EventComponent::MUSIC] = "music";
	EVENT_COMPONENT_NAME[EventComponent::CUTSCENE] = "cutscene";
	EVENT_COMPONENT_NAME[EventComponent::REPEAT] = "repeat";
	EVENT_COMPONENT_NAME[EventComponent::SAVE_GAME] = "save_game";
	EVENT_COMPONENT_NAME[EventComponent::BOOK] = "book";
	EVENT_COMPONENT_NAME[EventComponent::SCRIPT] = "script";
	EVENT_COMPONENT_NAME[EventComponent::CHANCE_EXEC] = "chance_exec";
	EVENT_COMPONENT_NAME[EventComponent::RESPEC] = "respec";
	EVENT_COMPONENT_NAME[EventComponent::SHOW_ON_MINIMAP] = "show_on_minimap";
	EVENT_COMPONENT_NAME[EventComponent::PARALLAX_LAYERS] = "parallax_layers";
	EVENT_COMPONENT_NAME[EventComponent::RANDOM_STATUS] = "random_status";

	dest_file = map->getFilename();
}


MapSaver::~MapSaver() {
}

/*
 * tileset_definitions is a string, containing tileset description
 * for editing map in Tiled Editor. This data is not present in map,
 * loaded by game, so when saving map from game tileset_definitions
 * should be empty string
 */
bool MapSaver::saveMap(const std::string& tileset_definitions) {
	std::ofstream outfile;

	outfile.open(Filesystem::convertSlashes(dest_file).c_str(), std::ios::out);

	if (outfile.is_open()) {

		outfile << "## flare-engine generated map file ##" << "\n";

		writeHeader(outfile);
		writeTilesets(outfile, tileset_definitions);
		writeLayers(outfile);

		writeEvents(outfile);
		writeNPCs(outfile);
		writeEnemies(outfile);

		if (outfile.bad()) {
			Utils::logError("MapSaver: Unable to save the map. No write access or disk is full!");
			return false;
		}
		outfile.close();
		outfile.clear();

		return true;
	}
	else {
		Utils::logError("MapSaver: Could not open %s for writing", dest_file.c_str());
	}
	return false;
}

bool MapSaver::saveMap(const std::string& file, const std::string& tileset_definitions) {
	dest_file = file;

	return saveMap(tileset_definitions);
}


void MapSaver::writeHeader(std::ofstream& map_file) {
	map_file << "[header]" << std::endl;
	map_file << "width=" << map->w << std::endl;
	map_file << "height=" << map->h << std::endl;
	map_file << "tilewidth=" << "64" << std::endl;
	map_file << "tileheight=" << "32" << std::endl;
	map_file << "orientation=" << "isometric" << std::endl;
	map_file << "music=" << map->music_filename << std::endl;
	map_file << "tileset=" << map->getTileset() << std::endl;
	map_file << "title=" << map->title << std::endl;
	map_file << "hero_pos=" << static_cast<int>(map->hero_pos.x) << "," << static_cast<int>(map->hero_pos.y) << std::endl;

	if (!map->procgen_chunks.empty() && !map->procgen_chunks[0].empty()) {
		map_file << "procgen_chunks=" << map->procgen_chunks[0].size() << "," << map->procgen_chunks.size();
		for (size_t i = 0; i < map->procgen_chunks.size(); ++i) {
			for (size_t j = 0; j < map->procgen_chunks[i].size(); ++j) {
				Chunk* chunk = &map->procgen_chunks[i][j];
				map_file << "," << chunk->type << "," << chunk->door_level << "," << chunk->variant;
				for (size_t k = 0; k < chunk->links.size(); ++k) {
					map_file << ",";

					if (chunk->links[k])
						map_file << 1;
					else
						map_file << 0;
				}
			}
		}
		map_file << std::endl;
	}

	map_file << std::endl;
}

void MapSaver::writeTilesets(std::ofstream& map_file, const std::string& tileset_definitions) {
	map_file << "[tilesets]" << std::endl;

	map_file << tileset_definitions << std::endl;

	map_file << std::endl;
}


void MapSaver::writeLayers(std::ofstream& map_file) {
	for (unsigned short i = 0; i < map->layernames.size(); i++) {
		map_file << "[layer]" << std::endl;

		map_file << "type=" << map->layernames[i] << std::endl;
		map_file << "data=" << std::endl;

		std::string layer = "";
		for (int line = 0; line < map->h; line++) {
			std::stringstream map_row;
			for (int tile = 0; tile < map->w; tile++) {
				map_row << map->layers[i][tile][line] << ",";
			}
			layer += map_row.str();
			layer += '\n';
		}
		layer.erase(layer.end()-2, layer.end());
		layer += '\n';

		map_file << layer << std::endl;
	}
}


void MapSaver::writeEnemies(std::ofstream& map_file) {
	for (size_t i = 0; i < map->enemy_groups.size(); ++i) {
		Map_Group &group = map->enemy_groups[i];

		map_file << "[enemy]" << std::endl;

		if (group.type == "") {
			map_file << "type=enemy" << std::endl;
		}
		else {
			map_file << "type=" << group.type << std::endl;
		}

		map_file << "location=" << group.pos.x << "," << group.pos.y << "," << group.area.x << "," << group.area.y << std::endl;

		map_file << "category=" << group.category << std::endl;

		if (group.levelmin != 0 || group.levelmax != 0) {
			map_file << "level=" << group.levelmin << "," << group.levelmax << std::endl;
		}

		if (group.numbermin != 1 || group.numbermax != 1) {
			map_file << "number=" << group.numbermin << "," << group.numbermax << std::endl;
		}

		if (group.chance != 100.f) {
			map_file << "chance=" << group.chance << std::endl;
		}

		if (group.direction != Map_Group::RANDOM_DIRECTION) {
			map_file << "direction=" << group.direction << std::endl;
		}

		if (!group.waypoints.empty() && group.wander_radius == 0) {
			map_file << "waypoints=";
			for (size_t j = 0; j < group.waypoints.size(); ++j) {
				map_file << static_cast<int>(group.waypoints[j].x) << "," << static_cast<int>(group.waypoints[j].y) << ";";
			}
			map_file << std::endl;
		}

		if ((group.wander_radius != Map_Group::DEFAULT_WANDER_RADIUS && group.waypoints.empty())) {
			map_file << "wander_radius=" << group.wander_radius << std::endl;
		}

		for (size_t j = 0; j < group.requirements.size(); ++j) {
			EventComponent& ec = group.requirements[j];

			if (ec.type == EventComponent::REQUIRES_STATUS) {
				map_file << "requires_status=" << ec.s << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_STATUS) {
				map_file << "requires_not_status=" << ec.s << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_LEVEL) {
				map_file << "requires_level=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_LEVEL) {
				map_file << "requires_not_level=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_CURRENCY) {
				map_file << "requires_currency=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_CURRENCY) {
				map_file << "requires_not_currency=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_ITEM) {
				map_file << "requires_item=" << ec.id << ":" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_ITEM) {
				map_file << "requires_not_item=" << ec.id << ":" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_CLASS) {
				map_file << "requires_class=" << ec.s << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_CLASS) {
				map_file << "requires_not_class=" << ec.s << std::endl;
			}
		}

		for (size_t j = 0; j < group.invincible_requirements.size(); ++j) {
			EventComponent& ec = group.invincible_requirements[j];

			if (ec.type == EventComponent::REQUIRES_STATUS) {
				map_file << "requires_status=" << ec.s << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_STATUS) {
				map_file << "requires_not_status=" << ec.s << std::endl;
			}
			// invincible_requirements only accepts status
		}

		if (group.spawn_level.ratio > 0) {
			map_file << "spawn_level=";

			if (group.spawn_level.mode == SpawnLevel::MODE_DEFAULT) {
				map_file << "default";
			}
			else if (group.spawn_level.mode == SpawnLevel::MODE_FIXED) {
				map_file << "fixed," << static_cast<int>(group.spawn_level.ratio);
			}
			else if (group.spawn_level.mode == SpawnLevel::MODE_LEVEL) {
				map_file << "source_level," << group.spawn_level.ratio;
			}
			else if (group.spawn_level.mode == SpawnLevel::MODE_STAT && group.spawn_level.stat < eset->primary_stats.list.size()) {
				map_file << "source_stat," << group.spawn_level.ratio << "," << eset->primary_stats.list[group.spawn_level.stat].id;
			}

			map_file << std::endl;
		}

		map_file << std::endl;
	}
}


void MapSaver::writeNPCs(std::ofstream& map_file) {
	for (size_t i = 0; i < map->map_npcs.size(); ++i) {
		Map_NPC &npc = map->map_npcs[i];

		map_file << "[npc]" << std::endl;

		if (npc.type == "") {
			map_file << "type=npc" << std::endl;
		}
		else {
			map_file << "type=" << npc.type << std::endl;
		}

		map_file << "location=" << static_cast<int>(npc.pos.x) << "," << static_cast<int>(npc.pos.y) << ",1,1" << std::endl;
		map_file << "filename=" << npc.id << std::endl;

		if (npc.direction != Map_NPC::RANDOM_DIRECTION) {
			map_file << "direction=" << npc.direction << std::endl;
		}

		if (!npc.waypoints.empty() && npc.wander_radius == 0) {
			map_file << "waypoints=";
			for (size_t j = 0; j < npc.waypoints.size(); ++j) {
				map_file << static_cast<int>(npc.waypoints[j].x) << "," << static_cast<int>(npc.waypoints[j].y) << ";";
			}
			map_file << std::endl;
		}

		if ((npc.wander_radius != Map_NPC::DEFAULT_WANDER_RADIUS && npc.waypoints.empty())) {
			map_file << "wander_radius=" << npc.wander_radius << std::endl;
		}


		for (size_t j = 0; j < npc.requirements.size(); ++j) {
			EventComponent& ec = npc.requirements[j];

			if (ec.type == EventComponent::REQUIRES_STATUS) {
				map_file << "requires_status=" << ec.s << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_STATUS) {
				map_file << "requires_not_status=" << ec.s << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_LEVEL) {
				map_file << "requires_level=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_LEVEL) {
				map_file << "requires_not_level=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_CURRENCY) {
				map_file << "requires_currency=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_CURRENCY) {
				map_file << "requires_not_currency=" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_ITEM) {
				map_file << "requires_item=" << ec.id << ":" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_ITEM) {
				map_file << "requires_not_item=" << ec.id << ":" << ec.data[0].Int << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_CLASS) {
				map_file << "requires_class=" << ec.s << std::endl;
			}
			else if (ec.type == EventComponent::REQUIRES_NOT_CLASS) {
				map_file << "requires_not_class=" << ec.s << std::endl;
			}
		}

		map_file << std::endl;
	}
}

void MapSaver::writeEvents(std::ofstream& map_file) {
	for (size_t i = 0; i < map->events.size(); i++) {
		Event &event = map->events[i];

		map_file << "[event]" << std::endl;

		if (event.type == "") {
			map_file << "type=event" << std::endl;
		}
		else {
			map_file << "type=" << event.type << std::endl;
		}

		Rect location = event.location;
		map_file << "location=" << location.x << "," << location.y << "," << location.w << "," << location.h  << std::endl;

		if (event.activate_type == Event::ACTIVATE_ON_TRIGGER) {
			map_file << "activate=on_trigger" << std::endl;
		}
		else if (event.activate_type == Event::ACTIVATE_ON_MAPEXIT) {
			map_file << "activate=on_mapexit" << std::endl;
		}
		else if (event.activate_type == Event::ACTIVATE_ON_LEAVE) {
			map_file << "activate=on_leave" << std::endl;
		}
		else if (event.activate_type == Event::ACTIVATE_ON_LOAD) {
			map_file << "activate=on_load" << std::endl;
		}
		else if (event.activate_type == Event::ACTIVATE_ON_CLEAR) {
			map_file << "activate=on_clear" << std::endl;
		}
		else if (event.activate_type == Event::ACTIVATE_STATIC) {
			map_file << "activate=static" << std::endl;
		}

		Rect hotspot = event.hotspot;
		if (hotspot.x == location.x && hotspot.y == location.y && hotspot.w == location.w && hotspot.h == location.h) {
			map_file << "hotspot=" << "location" << std::endl;
		}
		else if (hotspot.x != 0 && hotspot.y != 0 && hotspot.w != 0 && hotspot.h != 0) {
			map_file << "hotspot=" << hotspot.x << "," << hotspot.y << "," << hotspot.w << "," << hotspot.h << std::endl;
		}

		if (event.cooldown.getDuration() > 0) {
			std::string suffix = "ms";
			int value = static_cast<int>(1000.f * static_cast<float>(event.cooldown.getDuration()) / settings->max_frames_per_sec);
			if (value % 1000 == 0)
			{
				value = event.cooldown.getDuration() / settings->max_frames_per_sec;
				suffix = "s";
			}
			map_file << "cooldown=" << value << suffix << std::endl;
		}

		Rect reachable_from = event.reachable_from;
		if (reachable_from.x != 0 && reachable_from.y != 0 && reachable_from.w != 0 && reachable_from.h != 0) {
			map_file << "reachable_from=" << reachable_from.x << "," << reachable_from.y << "," << reachable_from.w << "," << reachable_from.h << std::endl;
		}
		writeEventComponents(map_file, static_cast<int>(i));

		map_file << std::endl;
	}
}

void MapSaver::writeEventComponents(std::ofstream &map_file, int eventID) {
	std::vector<EventComponent>& components = map->events[eventID].components;
	for (unsigned i = 0; i < components.size(); i++) {
		EventComponent& e = components[i];

		if (e.type > 0 && e.type < EventComponent::EVENT_COMPONENT_COUNT) {
			if (e.type == EventComponent::PROCGEN_FILENAME || e.type == EventComponent::PROCGEN_LINK) {
				continue;
			}
			else if (e.type == EventComponent::LOOT && e.data[LootManager::LOOT_EC_TYPE].Int == LootManager::LOOT_EC_TYPE_TABLE_ROW) {
				continue;
			}
			else if (!EVENT_COMPONENT_NAME[e.type].empty()) {
				map_file << EVENT_COMPONENT_NAME[e.type] << "=";
			}
		}
		else {
			continue;
		}

		if (e.type == EventComponent::TOOLTIP) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::POWER) {
			map_file << e.id << std::endl;
		}
		else if (e.type == EventComponent::POWER_PATH) {
			map_file << e.data[0].Int << "," << e.data[1].Int << ",";
			if (e.data[4].Bool)
			{
				map_file << "hero" << std::endl;
			}
			else
			{
				map_file << e.data[2].Int << "," << e.data[3].Int << std::endl;
			}
		}
		else if (e.type == EventComponent::POWER_DAMAGE) {
			map_file << e.data[0].Int << "," << e.data[1].Int << std::endl;
		}
		else if (e.type == EventComponent::POWER_STATS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::POWER_LEVEL) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::INTERMAP) {
			map_file << e.s;

			if (e.data[0].Int != -1 || e.data[1].Int != -1 || e.data[3].Int != 0) {
				map_file << "," << e.data[0].Int << "," << e.data[1].Int;
				if (e.data[3].Int != 0) {
					map_file << "," << eventm->getIntermapIDString(e.data[3].Int);
				}
			}

			map_file << std::endl;
		}
		else if (e.type == EventComponent::INTERMAP_ID) {
			map_file << eventm->getIntermapIDString(e.data[0].Int) << std::endl;
		}
		else if (e.type == EventComponent::INTRAMAP) {
			map_file << e.data[0].Int << "," << e.data[1].Int << std::endl;
		}
		else if (e.type == EventComponent::MAPMOD) {
			map_file << e.s << "," << e.data[0].Int << "," << e.data[1].Int << "," << e.data[2].Int << std::endl;
		}
		else if (e.type == EventComponent::MAPMOD_TOGGLE) {
			map_file << e.s << "," << e.data[0].Int << "," << e.data[1].Int << "," << e.data[2].Int << "," << e.data[3].Int << std::endl;
		}
		else if (e.type == EventComponent::SOUNDFX) {
			map_file << e.s;
			if (e.data[0].Int != -1 && e.data[1].Int != -1)
			{
				map_file << "," << e.data[0].Int << "," << e.data[1].Int << "," << e.data[2].Bool;
			}
			map_file << std::endl;
		}
		else if (e.type == EventComponent::LOOT) {
			if (e.data[LootManager::LOOT_EC_TYPE].Int == LootManager::LOOT_EC_TYPE_TABLE) {
				map_file << e.s << std::endl;
			}
			else if (e.data[LootManager::LOOT_EC_TYPE].Int == LootManager::LOOT_EC_TYPE_SINGLE) {
				map_file << e.s << ",";

				if (e.data[LootManager::LOOT_EC_CHANCE].Int == 0)
					map_file << "fixed";
				else
					map_file << e.data[LootManager::LOOT_EC_CHANCE].Int;

				map_file << "," << e.data[LootManager::LOOT_EC_QUANTITY_MIN].Int << "," << e.data[LootManager::LOOT_EC_QUANTITY_MAX].Int << std::endl;
			}
		}
		else if (e.type == EventComponent::LOOT_COUNT) {
			map_file << e.data[0].Int << "," << e.data[1].Int << std::endl;
		}
		else if (e.type == EventComponent::MSG) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::SHAKYCAM) {
			std::string suffix = "ms";
			int value = static_cast<int>(1000.f * e.data[0].Float / settings->max_frames_per_sec);
			if (value % 1000 == 0)
			{
				value = e.data[0].Int / settings->max_frames_per_sec;
				suffix = "s";
			}
			map_file << value << suffix << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_STATUS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_NOT_STATUS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_LEVEL) {
			map_file << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_NOT_LEVEL) {
			map_file << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_CURRENCY) {
			map_file << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_NOT_CURRENCY) {
			map_file << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_ITEM) {
			map_file << e.id << ":" << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_NOT_ITEM) {
			map_file << e.id << ":" << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_CLASS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_NOT_CLASS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_TILE) {
			map_file << e.s << "," << e.data[0].Int << "," << e.data[1].Int << "," << e.data[2].Int << std::endl;
		}
		else if (e.type == EventComponent::REQUIRES_NOT_TILE) {
			map_file << e.s << "," << e.data[0].Int << "," << e.data[1].Int << "," << e.data[2].Int << std::endl;
		}
		else if (e.type == EventComponent::SET_STATUS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::UNSET_STATUS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::REMOVE_CURRENCY) {
			map_file << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REMOVE_ITEM) {
			map_file << e.id << ":" << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REWARD_XP) {
			map_file << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REWARD_CURRENCY) {
			map_file << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::REWARD_ITEM) {
			map_file << e.id << ":" << e.data[0].Int << std::endl;
		}
		else if (e.type == EventComponent::RESTORE) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::SPAWN) {
			map_file << e.s << "," << e.data[0].Int << "," << e.data[1].Int << std::endl;
		}
		else if (e.type == EventComponent::SPAWN_LEVEL) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::STASH) {
			map_file << e.data[0].Bool << std::endl;
		}
		else if (e.type == EventComponent::NPC) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::MUSIC) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::CUTSCENE) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::REPEAT) {
			map_file << e.data[0].Bool << std::endl;
		}
		else if (e.type == EventComponent::SAVE_GAME) {
			map_file << e.data[0].Bool << std::endl;
		}
		else if (e.type == EventComponent::BOOK) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::SCRIPT) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::CHANCE_EXEC) {
			map_file << e.data[0].Float << std::endl;
		}
		else if (e.type == EventComponent::RESPEC) {
			if (e.data[0].Int == 3)
				map_file << "xp";
			else if (e.data[0].Int == 2)
				map_file << "stats";
			else if (e.data[0].Int == 1)
				map_file << "powers";

			map_file << "," << e.data[1].Bool << std::endl;
		}
		else if (e.type == EventComponent::SHOW_ON_MINIMAP) {
			map_file << e.data[0].Bool << std::endl;
		}
		else if (e.type == EventComponent::PARALLAX_LAYERS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EventComponent::RANDOM_STATUS) {
			if (e.data[0].Int == EventComponent::RANDOM_STATUS_MODE_APPEND)
				map_file << "append," << e.s << std::endl;
			else if (e.data[0].Int == EventComponent::RANDOM_STATUS_MODE_CLEAR)
				map_file << "clear" << std::endl;
			else if (e.data[0].Int == EventComponent::RANDOM_STATUS_MODE_ROLL)
				map_file << "roll" << std::endl;
			else if (e.data[0].Int == EventComponent::RANDOM_STATUS_MODE_SET)
				map_file << "set" << std::endl;
			else if (e.data[0].Int == EventComponent::RANDOM_STATUS_MODE_UNSET)
				map_file << "unset" << std::endl;
		}
		else if (e.type == EventComponent::PROCGEN_FILENAME) {
			map_file << e.s << std::endl;
		}
	}
}

