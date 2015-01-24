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

#include "MapSaver.h"
#include "Settings.h"

MapSaver::MapSaver(Map *_map) : map(_map)
{
    dest_file = map->getFilename();
}


MapSaver::~MapSaver()
{
}

bool MapSaver::saveMap()
{
    std::ofstream outfile;

    outfile.open(dest_file.c_str(), std::ios::out);

    if (outfile.is_open()) {

        outfile << "## flare-engine generated map file ##" << "\n";

        writeHeader(outfile);
        writeTilesets(outfile);
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
    return false;
}

bool MapSaver::saveMap(std::string file)
{
    dest_file = file;

    return saveMap();
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

    map_file << std::endl;
}

void MapSaver::writeTilesets(std::ofstream& map_file)
{
    map_file << "[tilesets]" << std::endl;

    std::string tileset = map->getTileset();

    if (tileset == "tilesetdefs/tileset_cave.txt")
    {
        map_file << "tileset=../../../tiled/cave/tiled_collision.png,64,32,0,0" << std::endl;
        map_file << "tileset=../../../tiled/cave/tiled_cave.png,64,128,0,0" << std::endl;
        map_file << "tileset=../../../tiled/cave/set_rules.png,64,32,0,0" << std::endl;
    }
    else if (tileset == "tilesetdefs/tileset_dungeon.txt")
    {
        map_file << "tileset=../../../tiled/dungeon/tiled_collision.png,64,32,0,0" << std::endl;
        map_file << "tileset=../../../tiled/dungeon/tiled_dungeon.png,64,128,0,0" << std::endl;
        map_file << "tileset=../../../tiled/dungeon/set_rules.png,64,32,0,0" << std::endl;
        map_file << "tileset=../../../tiled/dungeon/tiled_dungeon_2x2.png,128,64,0,16" << std::endl;
        map_file << "tileset=../../../tiled/dungeon/door_left.png,64,128,-16,-8" << std::endl;
        map_file << "tileset=../../../tiled/dungeon/door_right.png,64,128,16,-8" << std::endl;
        map_file << "tileset=../../../tiled/dungeon/stairs.png,256,256,0,48" << std::endl;
    }
    else if (tileset == "tilesetdefs/tileset_grassland.txt")
    {
        map_file << "tileset=../../../tiled/grassland/tiled_collision.png,64,32,0,0" << std::endl;
        map_file << "tileset=../../../tiled/grassland/grassland.png,64,128,0,0" << std::endl;
        map_file << "tileset=../../../tiled/grassland/grassland_water.png,64,64,0,32" << std::endl;
        map_file << "tileset=../../../tiled/grassland/grassland_structures.png,64,256,0,0" << std::endl;
        map_file << "tileset=../../../tiled/grassland/grassland_trees.png,128,256,-32,0" << std::endl;
        map_file << "tileset=../../../tiled/grassland/set_rules.png,64,32,0,0" << std::endl;
        map_file << "tileset=../../../tiled/grassland/tiled_grassland_2x2.png,128,64,0,16" << std::endl;
    }

    map_file << std::endl;
}


void MapSaver::writeLayers(std::ofstream& map_file)
{
    for (short i = 0; i < map->layernames.size(); i++)
    {
        map_file << "[layer]" << std::endl;

        map_file << "type=" << map->layernames[i] << std::endl;
        map_file << "data=" << std::endl;

        std::string layer = "";
        for (int line = 0; line < map->h; line++)
        {
            std::string map_row = "";
            for (int tile = 0; tile < map->w; tile++)
            {
                maprow* row = map->layers[i];
                map_row += std::to_string(row[tile][line]);
                map_row += ",";

            }
            layer += map_row;
            layer += '\n';
        }
        layer.pop_back();
        layer.pop_back();
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
        map_file << "type=" << group.front().category << std::endl;

        if (group.front().levelmin != 0 || group.front().levelmax != 0)
        {
            map_file << "level=" << group.front().levelmin << "," << group.front().levelmax << std::endl;
        }

        map_file << "location=" << group.front().pos.x << "," << group.front().pos.y << "," << group.front().area.x << "," << group.front().area.y << std::endl;

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

        if (!group.front().waypoints.empty())
        {
            // UNIMPLEMENTED
            map_file << "waypoints=" << std::endl;
        }

        if (group.front().wander_radius != 4)
        {
            map_file << "wander_radius=" << group.front().wander_radius << std::endl;
        }


        for (int i = 0; i < group.front().requires_status.size(); i++)
        {
            map_file << "requires_status=" << group.front().requires_status[i] << std::endl;
        }

        for (int i = 0; i < group.front().requires_status.size(); i++)
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
        map_file << "type=" << npcs.front().id << std::endl;
        map_file << "location=" << npcs.front().pos.x << "," << npcs.front().pos.y << ",1,1" << std::endl;

        for (int j = 0; j < npcs.front().requires_status.size(); j++)
        {
            map_file << "requires_status=" << npcs.front().requires_status[j] << std::endl;
        }
        for (int j = 0; j < npcs.front().requires_not_status.size(); j++)
        {
            map_file << "requires_not_status=" << npcs.front().requires_not_status[j] << std::endl;
        }

        map_file << std::endl;

        npcs.pop();
    }
}

void MapSaver::writeEvents(std::ofstream& map_file)
{
    for (int i = 0; i < map->events.size(); i++)
    {
        map_file << "[event]" << std::endl;
        map_file << "type=" << map->events[i].type << std::endl;

        Rect location = map->events[i].location;
        map_file << "location=" << location.x << "," << location.y << "," << location.w << "," << location.h  << std::endl;

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
            map_file << "cooldown=" << map->events[i].cooldown << std::endl;
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
    for (int i = 0; i < components.size(); i++)
    {
        Event_Component e = components[i];

        map_file << e.type << "=";

        if (e.type == "tooltip") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "power_path") {
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
        else if (e.type == "power_damage") {
            map_file << e.a << "," << e.b << std::endl;
        }
        else if (e.type == "intermap") {
            map_file << e.s << "," << e.x << "," << e.y << std::endl;
        }
        else if (e.type == "intramap") {
            map_file << e.x << "," << e.y << std::endl;
        }
        else if (e.type == "mapmod") {
            map_file << e.s << "," << e.x << "," << e.y << "," << e.z << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "soundfx") {
            map_file << e.s;
            if (e.x != -1 && e.y != -1)
            {
                map_file << "," << e.x << "," << e.y;
            }
            map_file << std::endl;
        }
        else if (e.type == "loot") {
            map_file << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "msg") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "shakycam") {
            // UNIMPLEMENTED
            // Should support ms too
            map_file << e.x/MAX_FRAMES_PER_SEC << "s" << std::endl;
        }
        else if (e.type == "requires_status") {
            map_file << e.s << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "requires_not_status") {
            map_file << e.s << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "requires_level") {
            map_file << e.x << std::endl;
        }
        else if (e.type == "requires_not_level") {
            map_file << e.x << std::endl;
        }
        else if (e.type == "requires_currency") {
            map_file << e.x << std::endl;
        }
        else if (e.type == "requires_item") {
            map_file << e.x << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "requires_class") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "set_status") {
            map_file << e.s << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "unset_status") {
            map_file << e.s << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "remove_currency") {
            map_file << e.x << std::endl;
        }
        else if (e.type == "remove_item") {
            map_file << e.x << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "reward_xp") {
            map_file << e.x << std::endl;
        }
        else if (e.type == "reward_currency") {
            map_file << e.x << std::endl;
        }
        else if (e.type == "reward_item") {
            map_file << e.x << ",";
            map_file << e.y << std::endl;
        }
        else if (e.type == "restore") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "power") {
            map_file << e.x << std::endl;
        }
        else if (e.type == "spawn") {
            map_file << e.s << "," << e.x << "," << e.y << std::endl;
            // UNIMPLEMENTED
        }
        else if (e.type == "stash") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "npc") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "music") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "cutscene") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "repeat") {
            map_file << e.s << std::endl;
        }
        else if (e.type == "save_game") {
            map_file << e.s << std::endl;
        }
    }
}
