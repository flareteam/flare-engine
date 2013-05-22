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
	: layers()
	, events()
	, w(0)
	, h(0)
	, spawn()
	, spawn_dir(0)
{
}



void Map::clearLayers() {

	for (unsigned i = 0; i < layers.size(); ++i)
		delete[] layers[i];
	layers.clear();
	layernames.clear();
}

void Map::clearQueues() {
	enemies = std::queue<Map_Enemy>();
	npcs = std::queue<Map_NPC>();
}

void Map::clearEvents() {
	events.clear();
}

int Map::load(std::string filename) {
	FileParser infile;
	maprow *cur_layer = NULL;

	clearEvents();
	clearLayers();
	clearQueues();

	if (!infile.open("maps/" + filename))
		return 0;

	while (infile.next()) {
		if (infile.new_section) {

			// for sections that are stored in collections, add a new object here
			if (infile.section == "enemy")
				enemies.push(Map_Enemy());
			else if (infile.section == "enemygroup")
				enemy_groups.push(Map_Group());
			else if (infile.section == "npc")
				npcs.push(Map_NPC());
			else if (infile.section == "event")
				events.push_back(Map_Event());

		}
		if (infile.section == "header")
			loadHeader(infile);
		else if (infile.section == "layer")
			loadLayer(infile, &cur_layer);
		else if (infile.section == "enemy")
			loadEnemy(infile);
		else if (infile.section == "enemygroup")
			loadEnemyGroup(infile, &enemy_groups.back());
		else if (infile.section == "npc")
			loadNPC(infile);
		else if (infile.section == "event")
			loadEvent(infile);
	}

	infile.close();

	return 0;
}

void Map::loadHeader(FileParser &infile) {
	if (infile.key == "title") {
		this->title = msg->get(infile.val);
	}
	else if (infile.key == "width") {
		this->w = toInt(infile.val);
	}
	else if (infile.key == "height") {
		this->h = toInt(infile.val);
	}
	else if (infile.key == "tileset") {
		this->tileset = infile.val;
	}
	else if (infile.key == "music") {
		music_filename = infile.val;
	}
	else if (infile.key == "location") {
		spawn.x = toInt(infile.nextValue()) + 0.5;
		spawn.y = toInt(infile.nextValue()) + 0.5;
		spawn_dir = toInt(infile.nextValue());
	}
}

void Map::loadLayer(FileParser &infile, maprow **current_layer) {
	if (infile.key == "type") {
		*current_layer = new maprow[w];
		layers.push_back(*current_layer);
		layernames.push_back(infile.val);
	}
	else if (infile.key == "format") {
		if (infile.val != "dec") {
			fprintf(stderr, "ERROR: maploading: The format of a layer must be \"dec\"!\n");
			SDL_Quit();
			exit(1);
		}
	}
	else if (infile.key == "data") {
		// layer map data handled as a special case
		// The next h lines must contain layer data.  TODO: err
		for (int j=0; j<h; j++) {
			std::string val = infile.getRawLine() + ',';
			for (int i=0; i<w; i++)
				(*current_layer)[i][j] = eatFirstInt(val, ',');
		}
	}
}

void Map::loadEnemy(FileParser &infile) {
	if (infile.key == "type") {
		enemies.back().type = infile.val;
	}
	else if (infile.key == "location") {
		enemies.back().pos.x = toInt(infile.nextValue()) + 0.5;
		enemies.back().pos.y = toInt(infile.nextValue()) + 0.5;
	}
	else if (infile.key == "direction") {
		enemies.back().direction = toInt(infile.val);
	}
	else if (infile.key == "waypoints") {
		std::string none = "";
		std::string a = infile.nextValue();
		std::string b = infile.nextValue();

		while (a != none) {
			Point p;
			p.x = toInt(a) + 0.5;
			p.y = toInt(b) + 0.5;
			enemies.back().waypoints.push(p);
			a = infile.nextValue();
			b = infile.nextValue();
		}
	}
	else if (infile.key == "wander_area") {
		enemies.back().wander = true;
		enemies.back().wander_area.x = toInt(infile.nextValue()) + 0.5;
		enemies.back().wander_area.y = toInt(infile.nextValue()) + 0.5;
		enemies.back().wander_area.w = toInt(infile.nextValue()) + 0.5;
		enemies.back().wander_area.h = toInt(infile.nextValue()) + 0.5;
	}
}

void Map::loadEnemyGroup(FileParser &infile, Map_Group *group) {
	if (infile.key == "type") {
		group->category = infile.val;
	}
	else if (infile.key == "level") {
		group->levelmin = toInt(infile.nextValue());
		group->levelmax = toInt(infile.nextValue());
	}
	else if (infile.key == "location") {
		group->pos.x = toInt(infile.nextValue());
		group->pos.y = toInt(infile.nextValue());
		group->area.x = toInt(infile.nextValue());
		group->area.y = toInt(infile.nextValue());
	}
	else if (infile.key == "number") {
		group->numbermin = toInt(infile.nextValue());
		group->numbermax = toInt(infile.nextValue());
	}
	else if (infile.key == "chance") {
		float n = toInt(infile.nextValue()) / 100.0f;
		group->chance = std::min(1.0f, std::max(0.0f, n));
	}
}

void Map::loadNPC(FileParser &infile) {
	if (infile.key == "type") {
		npcs.back().id = infile.val;
	}
	else if (infile.key == "location") {
		npcs.back().pos.x = toInt(infile.nextValue()) + 0.5;
		npcs.back().pos.y = toInt(infile.nextValue()) + 0.5;
	}
}

void Map::loadEvent(FileParser &infile) {
	if (infile.key == "type") {
		std::string type = infile.val;
		events.back().type = type;

		if      (type == "on_trigger");
		else if (type == "on_mapexit"); // no need to set keep_after_trigger to false correctly, it's ignored anyway
		else if (type == "on_leave");
		else if (type == "on_load") {
			events.back().keep_after_trigger = false;
		}
		else if (type == "on_clear") {
			events.back().keep_after_trigger = false;
		}
		else {
			fprintf(stderr, "Map: Loading event in file %s\nEvent type %s unknown, change to \"on_trigger\" to suppress this warning.\n", infile.getFileName().c_str(), type.c_str());
		}
	}
	else if (infile.key == "location") {
		events.back().location.x = toInt(infile.nextValue());
		events.back().location.y = toInt(infile.nextValue());
		events.back().location.w = toInt(infile.nextValue());
		events.back().location.h = toInt(infile.nextValue());
	}
	else if (infile.key == "hotspot") {
		if (infile.val == "location") {
			events.back().hotspot.x = events.back().location.x;
			events.back().hotspot.y = events.back().location.y;
			events.back().hotspot.w = events.back().location.w;
			events.back().hotspot.h = events.back().location.h;
		}
		else {
			events.back().hotspot.x = toInt(infile.nextValue());
			events.back().hotspot.y = toInt(infile.nextValue());
			events.back().hotspot.w = toInt(infile.nextValue());
			events.back().hotspot.h = toInt(infile.nextValue());
		}
	}
	else if (infile.key == "cooldown") {
		events.back().cooldown = parse_duration(infile.val);
	}
	else {
		loadEventComponent(infile);
	}
}

void Map::loadEventComponent(FileParser &infile) {
	// new event component
	events.back().components.push_back(Event_Component());
	Event_Component *e = &events.back().components.back();
	e->type = infile.key;

	if (infile.key == "tooltip") {
		e->s = msg->get(infile.val);
	}
	else if (infile.key == "power_path") {
		// x,y are src, if s=="hero" we target the hero,
		// else we'll use values in a,b as coordinates
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());

		std::string dest = infile.nextValue();
		if (dest == "hero") {
			e->s = "hero";
		}
		else {
			e->a = toInt(dest);
			e->b = toInt(infile.nextValue());
		}
	}
	else if (infile.key == "power_damage") {
		e->a = toInt(infile.nextValue());
		e->b = toInt(infile.nextValue());
	}
	else if (infile.key == "intermap") {
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
	}
	else if (infile.key == "intramap") {
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
	}
	else if (infile.key == "mapmod") {
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
		e->z = toInt(infile.nextValue());

		// add repeating mapmods
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->s = repeat_val;
			e->x = toInt(infile.nextValue());
			e->y = toInt(infile.nextValue());
			e->z = toInt(infile.nextValue());

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "soundfx") {
		e->s = infile.nextValue();
		e->x = e->y = -1;

		std::string s = infile.nextValue();
		if (s != "") e->x = toInt(s);

		s = infile.nextValue();
		if (s != "") e->y = toInt(s);

	}
	else if (infile.key == "loot") {
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue()) + 0.5;
		e->y = toInt(infile.nextValue()) + 0.5;

		// drop chance
		std::string chance = infile.nextValue();
		if (chance == "fixed") e->z = 0;
		else e->z = toInt(chance);

		// quantity min/max
		e->a = toInt(infile.nextValue());
		if (e->a < 1) e->a = 1;
		e->b = toInt(infile.nextValue());
		if (e->b < e->a) e->b = e->a;

		// add repeating loot
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->s = repeat_val;
			e->x = toInt(infile.nextValue()) + 0.5;
			e->y = toInt(infile.nextValue()) + 0.5;

			chance = infile.nextValue();
			if (chance == "fixed") e->z = 0;
			else e->z = toInt(chance);

			e->a = toInt(infile.nextValue());
			if (e->a < 1) e->a = 1;
			e->b = toInt(infile.nextValue());
			if (e->b < e->a) e->b = e->a;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "msg") {
		e->s = msg->get(infile.val);
	}
	else if (infile.key == "shakycam") {
		e->x = toInt(infile.val);
	}
	else if (infile.key == "requires_status") {
		e->s = infile.nextValue();

		// add repeating requires_status
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "requires_not") {
		e->s = infile.nextValue();

		// add repeating requires_not
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "requires_level") {
		e->x = toInt(infile.nextValue());
	}
	else if (infile.key == "requires_not_level") {
		e->x = toInt(infile.nextValue());
	}
	else if (infile.key == "requires_item") {
		e->x = toInt(infile.nextValue());

		// add repeating requires_item
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->x = toInt(repeat_val);

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "set_status") {
		e->s = infile.nextValue();

		// add repeating set_status
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "unset_status") {
		e->s = infile.nextValue();

		// add repeating unset_status
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "remove_item") {
		e->x = toInt(infile.nextValue());

		// add repeating remove_item
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;
			e->x = toInt(repeat_val);

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "reward_xp") {
		e->x = toInt(infile.val);
	}
	else if (infile.key == "power") {
		e->x = toInt(infile.val);
	}
	else if (infile.key == "spawn") {

		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue()) + 0.5;
		e->y = toInt(infile.nextValue()) + 0.5;

		// add repeating spawn
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			events.back().components.push_back(Event_Component());
			e = &events.back().components.back();
			e->type = infile.key;

			e->s = repeat_val;
			e->x = toInt(infile.nextValue()) + 0.5;
			e->y = toInt(infile.nextValue()) + 0.5;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "stash") {
		e->s = infile.val;
	}
	else if (infile.key == "npc") {
		e->s = infile.val;
	}
	else if (infile.key == "music") {
		e->s = infile.val;
	}
	else if (infile.key == "cutscene") {
		e->s = infile.val;
	}
	else if (infile.key == "repeat") {
		e->s = infile.val;
	}
	else {
		fprintf(stderr, "Map: Unknown key value: %s in file %s in section %s\n", infile.key.c_str(), infile.getFileName().c_str(), infile.section.c_str());
	}
}
