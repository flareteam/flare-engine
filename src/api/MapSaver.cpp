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

#ifdef FLARE_MAP_SAVER

#include "MapSaver.h"
#include "Settings.h"

MapSaver::MapSaver(Map *_map) : map(_map)
{
	EVENT_COMPONENT_NAME[EC::TOOLTIP] = "tooltip";
	EVENT_COMPONENT_NAME[EC::POWER_PATH] = "power_path";
	EVENT_COMPONENT_NAME[EC::POWER_DAMAGE] = "power_damage";
	EVENT_COMPONENT_NAME[EC::INTERMAP] = "intermap";
	EVENT_COMPONENT_NAME[EC::INTRAMAP] = "intramap";
	EVENT_COMPONENT_NAME[EC::MAPMOD] = "mapmod";
	EVENT_COMPONENT_NAME[EC::SOUNDFX] = "soundfx";
	EVENT_COMPONENT_NAME[EC::LOOT] = "loot"; // HALF-IMPLEMENTED
	EVENT_COMPONENT_NAME[EC::LOOT_COUNT] = "loot_count"; // UNIMPLEMENTED
	EVENT_COMPONENT_NAME[EC::MSG] = "msg";
	EVENT_COMPONENT_NAME[EC::SHAKYCAM] = "shakycam";
	EVENT_COMPONENT_NAME[EC::REQUIRES_STATUS] = "requires_status";
	EVENT_COMPONENT_NAME[EC::REQUIRES_NOT_STATUS] = "requires_not_status";
	EVENT_COMPONENT_NAME[EC::REQUIRES_LEVEL] = "requires_level";
	EVENT_COMPONENT_NAME[EC::REQUIRES_NOT_LEVEL] = "requires_not_level";
	EVENT_COMPONENT_NAME[EC::REQUIRES_CURRENCY] = "requires_currency";
	EVENT_COMPONENT_NAME[EC::REQUIRES_NOT_CURRENCY] = "requires_not_currency";
	EVENT_COMPONENT_NAME[EC::REQUIRES_ITEM] = "requires_item";
	EVENT_COMPONENT_NAME[EC::REQUIRES_NOT_ITEM] = "requires_not_item";
	EVENT_COMPONENT_NAME[EC::REQUIRES_CLASS] = "requires_class";
	EVENT_COMPONENT_NAME[EC::REQUIRES_NOT_CLASS] = "requires_not_class";
	EVENT_COMPONENT_NAME[EC::SET_STATUS] = "set_status";
	EVENT_COMPONENT_NAME[EC::UNSET_STATUS] = "unset_status";
	EVENT_COMPONENT_NAME[EC::REMOVE_CURRENCY] = "remove_currency";
	EVENT_COMPONENT_NAME[EC::REMOVE_ITEM] = "remove_item";
	EVENT_COMPONENT_NAME[EC::REWARD_XP] = "reward_xp";
	EVENT_COMPONENT_NAME[EC::REWARD_CURRENCY] = "reward_currency";
	EVENT_COMPONENT_NAME[EC::REWARD_ITEM] = "reward_item";
	EVENT_COMPONENT_NAME[EC::RESTORE] = "restore";
	EVENT_COMPONENT_NAME[EC::POWER] = "power";
	EVENT_COMPONENT_NAME[EC::SPAWN] = "spawn";
	EVENT_COMPONENT_NAME[EC::STASH] = "stash";
	EVENT_COMPONENT_NAME[EC::NPC] = "npc";
	EVENT_COMPONENT_NAME[EC::MUSIC] = "music";
	EVENT_COMPONENT_NAME[EC::CUTSCENE] = "cutscene";
	EVENT_COMPONENT_NAME[EC::REPEAT] = "repeat";
	EVENT_COMPONENT_NAME[EC::SAVE_GAME] = "save_game";
	EVENT_COMPONENT_NAME[EC::BOOK] = "book";
	EVENT_COMPONENT_NAME[EC::SCRIPT] = "script";
	EVENT_COMPONENT_NAME[EC::CHANCE_EXEC] = "chance_exec";
	EVENT_COMPONENT_NAME[EC::RESPEC] = "respec";

	dest_file = map->getFilename();
}


MapSaver::~MapSaver()
{
}

/*
 * tileset_definitions is a string, containing tileset description
 * for editing map in Tiled Editor. This data is not present in map,
 * loaded by game, so when saving map from game tileset_definitions
 * should be empty string
 */
bool MapSaver::saveMap(std::string tileset_definitions)
{
	std::ofstream outfile;

	outfile.open(dest_file.c_str(), std::ios::out);

	if (outfile.is_open()) {

		outfile << "## flare-engine generated map file ##" << "\n";

		writeHeader(outfile);
		writeTilesets(outfile, tileset_definitions);
		writeLayers(outfile);

		writeEvents(outfile);
		writeNPCs(outfile);
		writeEnemies(outfile);

		if (outfile.bad())
		{
			logError("MapSaver: Unable to save the map. No write access or disk is full!");
			return false;
		}
		outfile.close();
		outfile.clear();

		return true;
	}
	else {
		logError("MapSaver: Could not open %s for writing", dest_file.c_str());
	}
	return false;
}

bool MapSaver::saveMap(std::string file, std::string tileset_definitions)
{
	dest_file = file;

	return saveMap(tileset_definitions);
}


void MapSaver::writeHeader(std::ofstream& map_file)
{
	map_file << "[header]" << std::endl;
	map_file << "width=" << map->w << std::endl;
	map_file << "height=" << map->h << std::endl;
	map_file << "tilewidth=" << "64" << std::endl;
	map_file << "tileheight=" << "32" << std::endl;
	map_file << "orientation=" << "isometric" << std::endl;
	map_file << "music=" << map->music_filename << std::endl;
	map_file << "tileset=" << map->getTileset() << std::endl;
	map_file << "title=" << map->title << std::endl;
	map_file << "hero_pos" << static_cast<int>(map->hero_pos.x) << "," << static_cast<int>(map->hero_pos.y) << std::endl;

	map_file << std::endl;
}

void MapSaver::writeTilesets(std::ofstream& map_file, std::string tileset_definitions)
{
	map_file << "[tilesets]" << std::endl;

	map_file << tileset_definitions << std::endl;

	map_file << std::endl;
}


void MapSaver::writeLayers(std::ofstream& map_file)
{
	for (unsigned short i = 0; i < map->layernames.size(); i++)
	{
		map_file << "[layer]" << std::endl;

		map_file << "type=" << map->layernames[i] << std::endl;
		map_file << "data=" << std::endl;

		std::string layer = "";
		for (int line = 0; line < map->h; line++)
		{
			std::stringstream map_row;
			for (int tile = 0; tile < map->w; tile++)
			{
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


void MapSaver::writeEnemies(std::ofstream& map_file)
{
	std::queue<Map_Group> group = map->enemy_groups;

	while (!group.empty())
	{
		map_file << "[enemy]" << std::endl;

		if (group.front().type == "")
		{
			map_file << "type=enemy" << std::endl;
		}
		else
		{
			map_file << "type=" << group.front().type << std::endl;
		}

		map_file << "location=" << group.front().pos.x << "," << group.front().pos.y << "," << group.front().area.x << "," << group.front().area.y << std::endl;

		map_file << "category=" << group.front().category << std::endl;

		if (group.front().levelmin != 0 || group.front().levelmax != 0)
		{
			map_file << "level=" << group.front().levelmin << "," << group.front().levelmax << std::endl;
		}

		if (group.front().numbermin != 1 || group.front().numbermax != 1)
		{
			map_file << "number=" << group.front().numbermin << "," << group.front().numbermax << std::endl;
		}

		if (group.front().chance != 1.0f)
		{
			map_file << "chance=" << group.front().chance*100 << std::endl;
		}

		if (group.front().direction != -1)
		{
			map_file << "direction=" << group.front().direction << std::endl;
		}

		if (!group.front().waypoints.empty() && group.front().wander_radius == 0)
		{
			map_file << "waypoints=";
			std::queue<FPoint> points = group.front().waypoints;
			while (!points.empty())
			{
				map_file << points.front().x - 0.5f << "," << points.front().y - 0.5f;
				points.pop();
				if (!points.empty())
				{
					map_file << ";";
				}
			}
			map_file << std::endl;
		}

		if ((group.front().wander_radius != 4 && group.front().waypoints.empty()))
		{
			map_file << "wander_radius=" << group.front().wander_radius << std::endl;
		}

		for (unsigned i = 0; i < group.front().requires_status.size(); i++)
		{
			map_file << "requires_status=" << group.front().requires_status[i] << std::endl;
		}

		for (unsigned i = 0; i < group.front().requires_status.size(); i++)
		{
			map_file << "requires_not_status=" << group.front().requires_not_status[i] << std::endl;
		}

		map_file << std::endl;
		group.pop();
	}
}


void MapSaver::writeNPCs(std::ofstream& map_file)
{
	std::queue<Map_NPC> npcs = map->npcs;

	while (!npcs.empty())
	{
		map_file << "[npc]" << std::endl;

		if (npcs.front().type == "")
		{
			map_file << "type=npc" << std::endl;
		}
		else
		{
			map_file << "type=" << npcs.front().type << std::endl;
		}

		map_file << "location=" << npcs.front().pos.x - 0.5f << "," << npcs.front().pos.y - 0.5f << ",1,1" << std::endl;
		map_file << "filename=" << npcs.front.id << std::endl;

		for (unsigned j = 0; j < npcs.front().requires_status.size(); j++)
		{
			map_file << "requires_status=" << npcs.front().requires_status[j] << std::endl;
		}
		for (unsigned j = 0; j < npcs.front().requires_not_status.size(); j++)
		{
			map_file << "requires_not_status=" << npcs.front().requires_not_status[j] << std::endl;
		}

		map_file << std::endl;

		npcs.pop();
	}
}

void MapSaver::writeEvents(std::ofstream& map_file)
{
	for (unsigned i = 0; i < map->events.size(); i++)
	{
		map_file << "[event]" << std::endl;

		if (map->events[i].type == "")
		{
			map_file << "type=event" << std::endl;
		}
		else
		{
			map_file << "type=" << map->events[i].type << std::endl;
		}

		Rect location = map->events[i].location;
		map_file << "location=" << location.x << "," << location.y << "," << location.w << "," << location.h  << std::endl;

		if (map->events[i].activate_type == EVENT_ON_TRIGGER)
		{
			map_file << "activate=on_trigger" << std::endl;
		}
		else if (map->events[i].activate_type == EVENT_ON_MAPEXIT)
		{
			map_file << "activate=on_mapexit" << std::endl;
		}
		else if (map->events[i].activate_type == EVENT_ON_LEAVE)
		{
			map_file << "activate=on_leave" << std::endl;
		}
		else if (map->events[i].activate_type == EVENT_ON_LOAD)
		{
			map_file << "activate=on_load" << std::endl;
		}
		else if (map->events[i].activate_type == EVENT_ON_CLEAR)
		{
			map_file << "activate=on_clear" << std::endl;
		}

		Rect hotspot = map->events[i].hotspot;
		if (hotspot.x == location.x && hotspot.y == location.y && hotspot.w == location.w && hotspot.h == location.h)
		{
			map_file << "hotspot=" << "location" << std::endl;
		}
		else if (hotspot.x != 0 && hotspot.y != 0 && hotspot.w != 0 && hotspot.h != 0)
		{
			map_file << "hotspot=" << hotspot.x << "," << hotspot.y << "," << hotspot.w << "," << hotspot.h << std::endl;
		}

		if (map->events[i].cooldown != 0)
		{
			std::string suffix = "ms";
			int value = static_cast<int>(1000.f * map->events[i].cooldown / MAX_FRAMES_PER_SEC);
			if (value % 1000 == 0)
			{
				value = map->events[i].cooldown / MAX_FRAMES_PER_SEC;
				suffix = "s";
			}
			map_file << "cooldown=" << value << suffix << std::endl;
		}

		Rect reachable_from = map->events[i].reachable_from;
		if (reachable_from.x != 0 && reachable_from.y != 0 && reachable_from.w != 0 && reachable_from.h != 0)
		{
			map_file << "reachable_from=" << reachable_from.x << "," << reachable_from.y << "," << reachable_from.w << "," << reachable_from.h << std::endl;
		}
		writeEventComponents(map_file, i);

		map_file << std::endl;
	}
}

void MapSaver::writeEventComponents(std::ofstream &map_file, int eventID)
{
	std::vector<Event_Component> components = map->events[eventID].components;
	for (unsigned i = 0; i < components.size(); i++)
	{
		Event_Component e = components[i];

		if (e.type > 0 && e.type < EC_COUNT)
		{
			map_file << EVENT_COMPONENT_NAME[e.type] << "=";
		}
		else
		{
			continue;
		}

		if (e.type == EC::TOOLTIP) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::POWER_PATH) {
			map_file << e.x << "," << e.y << ",";
			if (e.s == "hero")
			{
				map_file << e.s << std::endl;
			}
			else
			{
				map_file << e.a << "," << e.b << std::endl;
			}
		}
		else if (e.type == EC::POWER_DAMAGE) {
			map_file << e.a << "," << e.b << std::endl;
		}
		else if (e.type == EC::INTERMAP) {
			map_file << e.s << "," << e.x << "," << e.y << std::endl;
		}
		else if (e.type == EC::INTRAMAP) {
			map_file << e.x << "," << e.y << std::endl;
		}
		else if (e.type == EC::MAPMOD) {
			map_file << e.s << "," << e.x << "," << e.y << "," << e.z;

			while (i+1 < components.size() && components[i+1].type == EC::MAPMOD)
			{
				i++;
				e = components[i];
				map_file << ";" << e.s << "," << e.x << "," << e.y << "," << e.z;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::SOUNDFX) {
			map_file << e.s;
			if (e.x != -1 && e.y != -1)
			{
				map_file << "," << e.x << "," << e.y;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::LOOT) {

			std::stringstream chance;

			if (e.z == 0)
				chance << "fixed";
			else
				chance << e.z;

			map_file << e.s << "," << chance.str() << "," << e.a << "," << e.b;

			while (i+1 < components.size() && components[i+1].type == EC::LOOT)
			{
				i++;
				e = components[i];

				if (e.z == 0)
					chance << "fixed";
				else
					chance << e.z;

				map_file << ";" << e.s << "," << chance.str() << "," << e.a << "," << e.b;
			}
			map_file << std::endl;
			// UNIMPLEMENTED
			// Loot tables not supported
		}
		else if (e.type == EC::LOOT_COUNT) {
			// UNIMPLEMENTED
			// Loot count not supported
			map_file << std::endl;
		}
		else if (e.type == EC::MSG) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::SHAKYCAM) {
			std::string suffix = "ms";
			int value = static_cast<int>(1000.f * e.x / MAX_FRAMES_PER_SEC);
			if (value % 1000 == 0)
			{
				value = e.x / MAX_FRAMES_PER_SEC;
				suffix = "s";
			}
			map_file << value << suffix << std::endl;
		}
		else if (e.type == EC::REQUIRES_STATUS) {
			map_file << e.s;

			while (i+1 < components.size() && components[i+1].type == EC::REQUIRES_STATUS)
			{
				i++;
				e = components[i];
				map_file << ";" << e.s;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::REQUIRES_NOT_STATUS) {
			map_file << e.s;

			while (i+1 < components.size() && components[i+1].type == EC::REQUIRES_NOT_STATUS)
			{
				i++;
				e = components[i];
				map_file << ";" << e.s;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::REQUIRES_LEVEL) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::REQUIRES_NOT_LEVEL) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::REQUIRES_CURRENCY) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::REQUIRES_NOT_CURRENCY) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::REQUIRES_ITEM) {
			map_file << e.x;

			while (i+1 < components.size() && components[i+1].type == EC::REQUIRES_ITEM)
			{
				i++;
				e = components[i];
				map_file << "," << e.x;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::REQUIRES_NOT_ITEM) {
			map_file << e.x;

			while (i+1 < components.size() && components[i+1].type == EC::REQUIRES_NOT_ITEM)
			{
				i++;
				e = components[i];
				map_file << "," << e.x;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::REQUIRES_CLASS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::REQUIRES_NOT_CLASS) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::SET_STATUS) {
			map_file << e.s;

			while (i+1 < components.size() && components[i+1].type == EC::SET_STATUS)
			{
				i++;
				e = components[i];
				map_file << "," << e.s;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::UNSET_STATUS) {
			map_file << e.s;

			while (i+1 < components.size() && components[i+1].type == EC::UNSET_STATUS)
			{
				i++;
				e = components[i];
				map_file << "," << e.s;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::REMOVE_CURRENCY) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::REMOVE_ITEM) {
			map_file << e.x;

			while (i+1 < components.size() && components[i+1].type == EC::REMOVE_ITEM)
			{
				i++;
				e = components[i];
				map_file << "," << e.x;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::REWARD_XP) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::REWARD_CURRENCY) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::REWARD_ITEM) {
			map_file << e.x << ",";
			map_file << e.y << std::endl;
		}
		else if (e.type == EC::RESTORE) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::POWER) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::SPAWN) {
			map_file << e.s << "," << e.x << "," << e.y;

			while (i+1 < components.size() && components[i+1].type == EC::SPAWN)
			{
				i++;
				e = components[i];
				map_file << ";" << e.s << "," << e.x << "," << e.y;
			}
			map_file << std::endl;
		}
		else if (e.type == EC::STASH) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::NPC) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::MUSIC) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::CUTSCENE) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::REPEAT) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::SAVE_GAME) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::BOOK) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::SCRIPT) {
			map_file << e.s << std::endl;
		}
		else if (e.type == EC::CHANCE_EXEC) {
			map_file << e.x << std::endl;
		}
		else if (e.type == EC::RESPEC) {
			if (e.x == 3)
				map_file << "xp";
			else if (e.x == 2)
				map_file << "stats";
			else if (e.x == 1)
				map_file << "powers";

			map_file << "," << e.y << endl;
	}
}

#endif //FLARE_MAP_SAVER

