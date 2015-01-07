/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert

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
#include "CommonIncludes.h"
#include "EnemyGroupManager.h"
#include "FileParser.h"
#include "MapRenderer.h"
#include "PowerManager.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetTooltip.h"

#include <stdint.h>
#include <limits>
#include <math.h>

MapRenderer::MapRenderer()
	: Map()
	, music(NULL)
	, tip(new WidgetTooltip())
	, tip_pos()
	, show_tooltip(false)
	, shakycam()
	, npc_tooltip_margin(0)
	, cam()
	, map_change(false)
	, teleportation(false)
	, teleport_destination()
	, respawn_point()
	, cutscene(false)
	, cutscene_file("")
	, log_msg("")
	, shaky_cam_ticks(0)
	, stash(false)
	, stash_pos()
	, enemies_cleared(false)
	, save_game(false)
	, npc_id(-1)
	, index_objectlayer(0)
{
	FileParser infile;
	// load tooltip_margin from engine config file
	// @CLASS Map|Description of engine/tooltips.txt
	if (infile.open("engine/tooltips.txt")) {
		while (infile.next()) {
			if (infile.key == "npc_tooltip_margin") {
				// @ATTR npc_tooltip_margin|integer|Vertical offset for NPC labels.
				npc_tooltip_margin = toInt(infile.val);
			}
		}
		infile.close();
	}
}

void MapRenderer::clearQueues() {
	Map::clearQueues();
	loot.clear();
}

bool MapRenderer::enemyGroupPlaceEnemy(float x, float y, Map_Group &g) {
	if (collider.is_empty(x, y)) {
		Enemy_Level enemy_lev = enemyg->getRandomEnemy(g.category, g.levelmin, g.levelmax);
		if (!enemy_lev.type.empty()) {
			Map_Enemy group_member = Map_Enemy(enemy_lev.type, FPoint(x, y));

			group_member.direction = g.direction;
			group_member.wander_radius = g.wander_radius;
			group_member.requires_status = g.requires_status;
			group_member.requires_not_status = g.requires_not_status;

			if (g.area.x == 1 && g.area.y == 1) {
				// this is a single enemy
				group_member.waypoints = g.waypoints;
			}

			enemies.push(group_member);
		}
		return true;
	}
	return false;
}

void MapRenderer::pushEnemyGroup(Map_Group &g) {
	// activate at all?
	float activate_chance = (rand() % 100) / 100.0f;
	if (activate_chance > g.chance) {
		return;
	}

	// The algorithm tries to place the enemies at random locations.
	// However if a location is not possible (unwalkable or there is already an entity),
	// then try again.
	// This could result in an infinite loop if there were more enemies than
	// actual places, so have an upper bound of tries.

	// random number of enemies
	int enemies_to_spawn = randBetween(g.numbermin, g.numbermax);

	// pick an upper bound, which is definitely larger than threetimes the enemy number to spawn.
	int allowed_misses = 5 * g.numbermax;

	while (enemies_to_spawn && allowed_misses) {

		float x = (g.pos.x + (rand() % g.area.x)) + 0.5f;
		float y = (g.pos.y + (rand() % g.area.y)) + 0.5f;

		if (enemyGroupPlaceEnemy(x, y, g))
			enemies_to_spawn--;
		else
			allowed_misses--;
	}
	if (enemies_to_spawn) {
		// now that the fast method of spawning enemies doesn't work, but we
		// still have enemies to place, do not place them randomly, but at the
		// first free spot
		for (int x = g.pos.x; x < g.pos.x + g.area.x && enemies_to_spawn; x++) {
			for (int y = g.pos.y; y < g.pos.y + g.area.y && enemies_to_spawn; y++) {
				float xpos = x + 0.5f;
				float ypos = y + 0.5f;
				if (enemyGroupPlaceEnemy(xpos, ypos, g))
					enemies_to_spawn--;
			}
		}

	}
	if (enemies_to_spawn) {
		logError("MapRenderer: Could not spawn all enemies in group at %s (x=%d,y=%d,w=%d,h=%d), %d missing (min=%d max=%d)\n",
				filename.c_str(), g.pos.x, g.pos.y, g.area.x, g.area.y, enemies_to_spawn, g.numbermin, g.numbermax);
	}
}

/**
 * No guarantee that maps will use all layers
 * Clear all tile layers (e.g. when loading a map)
 */
void MapRenderer::clearLayers() {
	Map::clearLayers();
	index_objectlayer = 0;
}

int MapRenderer::load(std::string fname) {
	/* unload sounds */
	snd->reset();
	while (!sids.empty()) {
		snd->unload(sids.back());
		sids.pop_back();
	}

	show_tooltip = false;

	Map::load(fname);

	loadMusic();

	for (unsigned i = 0; i < layers.size(); ++i) {
		if (layernames[i] == "collision") {
			collider.setmap(layers[i], w, h);
			layernames.erase(layernames.begin() + i);
			delete[] layers[i];
			layers.erase(layers.begin() + i);
		}
	}
	for (unsigned i = 0; i < layers.size(); ++i)
		if (layernames[i] == "object")
			index_objectlayer = i;

	while (!enemy_groups.empty()) {
		pushEnemyGroup(enemy_groups.front());
		enemy_groups.pop();
	}

	tset.load(this->tileset);

	std::vector<unsigned> corrupted;
	for (unsigned i = 0; i < layers.size(); ++i) {
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const unsigned tile_id = layers[i][x][y];
				if (tile_id > 0 && (tile_id >= tset.tiles.size() || tset.tiles[tile_id].tile == NULL)) {
					if (std::find(corrupted.begin(), corrupted.end(), tile_id) == corrupted.end()) {
						corrupted.push_back(tile_id);
					}
					layers[i][x][y] = 0;
				}
			}
		}
	}

	if (!corrupted.empty()) {
		logError("MapRenderer: Tileset or Map corrupted. A tile has a larger id than the tileset allows or is undefined.\n");
		while (!corrupted.empty()) {
			logError("MapRenderer: Removing offending tile id %d.\n", corrupted.back());
			corrupted.pop_back();
		}
	}

	// some events automatically trigger when the map loads
	// e.g. change map state based on campaign status
	executeOnLoadEvents();

	return 0;
}

void MapRenderer::loadMusic() {

	// keep playing if already the correct track
	if (played_music_filename == music_filename)
		return;

	played_music_filename = music_filename;

	if (music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = NULL;
	}
	if (AUDIO && MUSIC_VOLUME) {
		music = Mix_LoadMUS(mods->locate(played_music_filename).c_str());
		if(!music)
			logError("MapRenderer: Mix_LoadMUS: %s\n", Mix_GetError());
	}

	if (music) {
		Mix_VolumeMusic(MUSIC_VOLUME);
		Mix_PlayMusic(music, -1);
	}
}

void MapRenderer::logic() {

	// handle camera shaking timer
	if (shaky_cam_ticks > 0) shaky_cam_ticks--;

	// handle tile set logic e.g. animations
	tset.logic();

	// handle event cooldowns
	std::vector<Event>::iterator it;
	for (it = events.begin(); it < events.end(); ++it) {
		if ((*it).cooldown_ticks > 0) (*it).cooldown_ticks--;
	}

}

bool priocompare(const Renderable &r1, const Renderable &r2) {
	return r1.prio < r2.prio;
}

/**
 * Sort in the same order as the tiles are drawn
 * Depends upon the map implementation
 */
void calculatePriosIso(std::vector<Renderable> &r) {
	for (std::vector<Renderable>::iterator it = r.begin(); it != r.end(); ++it) {
		const unsigned tilex = (const unsigned)floor(it->map_pos.x);
		const unsigned tiley = (const unsigned)floor(it->map_pos.y);
		const int commax = (const int)((float)(it->map_pos.x - tilex) * (2<<16));
		const int commay = (const int)((float)(it->map_pos.y - tiley) * (2<<16));
		it->prio += (((uint64_t)(tilex + tiley)) << 54) + (((uint64_t)tilex) << 42) + ((commax + commay) << 16);
	}
}

void calculatePriosOrtho(std::vector<Renderable> &r) {
	for (std::vector<Renderable>::iterator it = r.begin(); it != r.end(); ++it) {
		const unsigned tilex = (const unsigned)floor(it->map_pos.x);
		const unsigned tiley = (const unsigned)floor(it->map_pos.y);
		const int commay = (const int)(1024 * it->map_pos.y);
		it->prio += (((uint64_t)tiley) << 48) + (((uint64_t)tilex) << 32) + (commay << 16);
	}
}

void MapRenderer::render(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {

	if (shaky_cam_ticks == 0) {
		shakycam.x = cam.x;
		shakycam.y = cam.y;
	}
	else {
		shakycam.x = cam.x + (rand() % 16 - 8) * 0.0078125f;
		shakycam.y = cam.y + (rand() % 16 - 8) * 0.0078125f;
	}

	if (TILESET_ORIENTATION == TILESET_ORTHOGONAL) {
		calculatePriosOrtho(r);
		calculatePriosOrtho(r_dead);
		std::sort(r.begin(), r.end(), priocompare);
		std::sort(r_dead.begin(), r_dead.end(), priocompare);
		renderOrtho(r, r_dead);
	}
	else {
		calculatePriosIso(r);
		calculatePriosIso(r_dead);
		std::sort(r.begin(), r.end(), priocompare);
		std::sort(r_dead.begin(), r_dead.end(), priocompare);
		renderIso(r, r_dead);
	}
}

void MapRenderer::drawRenderable(std::vector<Renderable>::iterator r_cursor) {
	if (r_cursor->image != NULL) {
		Rect dest;
		Point p = map_to_screen(r_cursor->map_pos.x, r_cursor->map_pos.y, shakycam.x, shakycam.y);
		dest.x = p.x - r_cursor->offset.x;
		dest.y = p.y - r_cursor->offset.y;
		render_device->render(*r_cursor, dest);
	}
}

void MapRenderer::renderIsoLayer(const unsigned short layerdata[256][256]) {
	int_fast16_t i; // first index of the map array
	int_fast16_t j; // second index of the map array
	Rect dest;
	const Point upperleft = floor(screen_to_map(0, 0, shakycam.x, shakycam.y));
	const int_fast16_t max_tiles_width =   (VIEW_W / TILE_W) + 2*tset.max_size_x;
	const int_fast16_t max_tiles_height = (2 * VIEW_H / TILE_H) + 2*tset.max_size_y;

	j = upperleft.y - tset.max_size_y/2 + tset.max_size_x;
	i = upperleft.x - tset.max_size_y/2 - tset.max_size_x;

	for (uint_fast16_t y = max_tiles_height ; y; --y) {
		int_fast16_t tiles_width = 0;

		// make sure the isometric corners are not rendered:
		// corner north west, upper left  (i < 0)
		if (i < -1) {
			j += i + 1;
			tiles_width -= i + 1;
			i = -1;
		}
		// corner north east, upper right (j > mapheight)
		const int_fast16_t d = j - h;
		if (d >= 0) {
			j -= d;
			tiles_width += d;
			i += d;
		}

		// lower right (south east) corner is covered by (j+i-w+1)
		// lower left (south west) corner is caught by having 0 in there, so j>0
		const int_fast16_t j_end = std::max(static_cast<int_fast16_t>(j+i-w+1),	std::max(static_cast<int_fast16_t>(j - max_tiles_width), static_cast<int_fast16_t>(0)));

		Point p = map_to_screen(float(i), float(j), shakycam.x, shakycam.y);
		p = center_tile(p);

		// draw one horizontal line
		while (j > j_end) {
			--j;
			++i;
			++tiles_width;
			p.x += TILE_W;

			if (const uint_fast16_t current_tile = layerdata[i][j]) {
				dest.x = p.x - tset.tiles[current_tile].offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y;
				// no need to set w and h in dest, as it is ignored
				// by SDL_BlitSurface
				tset.tiles[current_tile].tile->setDest(dest);
				render_device->render(tset.tiles[current_tile].tile);
			}
		}
		j += tiles_width;
		i -= tiles_width;
		// Go one line deeper, the starting position goes zig-zag
		if (y % 2)
			i++;
		else
			j++;
	}
}

void MapRenderer::renderIsoBackObjects(std::vector<Renderable> &r) {
	std::vector<Renderable>::iterator it;
	for (it = r.begin(); it != r.end(); ++it)
		drawRenderable(it);
}

void MapRenderer::renderIsoFrontObjects(std::vector<Renderable> &r) {
	Rect dest;

	const Point upperleft = floor(screen_to_map(0, 0, shakycam.x, shakycam.y));
	const int_fast16_t max_tiles_width =   (VIEW_W / TILE_W) + 2 * tset.max_size_x;
	const int_fast16_t max_tiles_height = ((VIEW_H / TILE_H) + 2 * tset.max_size_y)*2;

	std::vector<Renderable>::iterator r_cursor = r.begin();
	std::vector<Renderable>::iterator r_end = r.end();

	// object layer
	int_fast16_t j = upperleft.y - tset.max_size_y + tset.max_size_x;
	int_fast16_t i = upperleft.x - tset.max_size_y - tset.max_size_x;

	while (r_cursor != r_end && ((int)(r_cursor->map_pos.x) + (int)(r_cursor->map_pos.y) < i + j || (int)(r_cursor->map_pos.x) < i)) // implicit floor
		++r_cursor;

	if (index_objectlayer >= layers.size())
		return;

	maprow *objectlayer = layers[index_objectlayer];
	for (uint_fast16_t y = max_tiles_height ; y; --y) {
		int_fast16_t tiles_width = 0;

		// make sure the isometric corners are not rendered:
		if (i < -1) {
			j += i + 1;
			tiles_width -= i + 1;
			i = -1;
		}
		const int_fast16_t d = j - h;
		if (d >= 0) {
			j -= d;
			tiles_width += d;
			i += d;
		}
		const int_fast16_t j_end = std::max(static_cast<int_fast16_t>(j+i-w+1), std::max(static_cast<int_fast16_t>(j - max_tiles_width), static_cast<int_fast16_t>(0)));

		// draw one horizontal line
		Point p = map_to_screen(float(i), float(j), shakycam.x, shakycam.y);
		p = center_tile(p);
		while (j > j_end) {
			--j;
			++i;
			++tiles_width;
			p.x += TILE_W;

			if (const uint_fast16_t current_tile = objectlayer[i][j]) {
				dest.x = p.x - tset.tiles[current_tile].offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y;
				tset.tiles[current_tile].tile->setDest(dest);
				render_device->render(tset.tiles[current_tile].tile);
			}

			// some renderable entities go in this layer
			while (r_cursor != r_end && ((int)r_cursor->map_pos.x == i && (int)r_cursor->map_pos.y == j)) { // implicit floor by int cast
				drawRenderable(r_cursor);
				++r_cursor;
			}
		}
		j += tiles_width;
		i -= tiles_width;
		if (y % 2)
			i++;
		else
			j++;

		while (r_cursor != r_end && ((int)r_cursor->map_pos.x + (int)r_cursor->map_pos.y < i + j || (int)r_cursor->map_pos.x <= i)) // implicit floor by int cast
			++r_cursor;
	}
}

void MapRenderer::renderIso(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	size_t index = 0;
	while (index < index_objectlayer)
		renderIsoLayer(layers[index++]);

	renderIsoBackObjects(r_dead);
	renderIsoFrontObjects(r);

	index++;
	while (index < layers.size())
		renderIsoLayer(layers[index++]);

	checkTooltip();
}

void MapRenderer::renderOrthoLayer(const unsigned short layerdata[256][256]) {

	const Point upperleft = floor(screen_to_map(0, 0, shakycam.x, shakycam.y));

	short int startj = std::max(0, upperleft.y);
	short int starti = std::max(0, upperleft.x);
	const short max_tiles_width =  std::min(w, static_cast<short int>(starti + (VIEW_W / TILE_W) + 2 * tset.max_size_x));
	const short max_tiles_height = std::min(h, static_cast<short int>(startj + (VIEW_H / TILE_H) + 2 * tset.max_size_y));

	short int i;
	short int j;

	for (j = startj; j < max_tiles_height; j++) {
		Point p = map_to_screen(starti, j, shakycam.x, shakycam.y);
		p = center_tile(p);
		for (i = starti; i < max_tiles_width; i++) {

			if (const unsigned short current_tile = layerdata[i][j]) {
				Rect dest;
				dest.x = p.x - tset.tiles[current_tile].offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y;
				tset.tiles[current_tile].tile->setDest(dest);
				render_device->render(tset.tiles[current_tile].tile);
			}
			p.x += TILE_W;
		}
	}
}

void MapRenderer::renderOrthoBackObjects(std::vector<Renderable> &r) {
	// some renderables are drawn above the background and below the objects
	std::vector<Renderable>::iterator it;
	for (it = r.begin(); it != r.end(); ++it)
		drawRenderable(it);
}

void MapRenderer::renderOrthoFrontObjects(std::vector<Renderable> &r) {

	short int i;
	short int j;
	Rect dest;
	std::vector<Renderable>::iterator r_cursor = r.begin();
	std::vector<Renderable>::iterator r_end = r.end();

	const Point upperleft = floor(screen_to_map(0, 0, shakycam.x, shakycam.y));

	short int startj = std::max(0, upperleft.y);
	short int starti = std::max(0, upperleft.x);
	const short max_tiles_width  = std::min(w, static_cast<short int>(starti + (VIEW_W / TILE_W) + 2 * tset.max_size_x));
	const short max_tiles_height = std::min(h, static_cast<short int>(startj + (VIEW_H / TILE_H) + 2 * tset.max_size_y));

	while (r_cursor != r_end && (int)(r_cursor->map_pos.y) < startj)
		++r_cursor;

	if (index_objectlayer >= layers.size())
		return;

	maprow *objectlayer = layers[index_objectlayer];
	for (j = startj; j < max_tiles_height; j++) {
		Point p = map_to_screen(starti, j, shakycam.x, shakycam.y);
		p = center_tile(p);
		for (i = starti; i<max_tiles_width; i++) {

			if (const unsigned short current_tile = objectlayer[i][j]) {
				dest.x = p.x - tset.tiles[current_tile].offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y;
				tset.tiles[current_tile].tile->setDest(dest);
				render_device->render(tset.tiles[current_tile].tile);
			}
			p.x += TILE_W;

			while (r_cursor != r_end && (int)(r_cursor->map_pos.y) == j && (int)(r_cursor->map_pos.x) < i) // implicit floor
				++r_cursor;

			// some renderable entities go in this layer
			while (r_cursor != r_end && (int)(r_cursor->map_pos.y) == j && (int)(r_cursor->map_pos.x) == i) // implicit floor
				drawRenderable(r_cursor++);
		}
		while (r_cursor != r_end && (int)(r_cursor->map_pos.y) <= j) // implicit floor
			++r_cursor;
	}
}

void MapRenderer::renderOrtho(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	unsigned index = 0;
	while (index < index_objectlayer)
		renderOrthoLayer(layers[index++]);

	renderOrthoBackObjects(r_dead);
	renderOrthoFrontObjects(r);
	index++;

	while (index < layers.size())
		renderOrthoLayer(layers[index++]);

	checkTooltip();
}

void MapRenderer::executeOnLoadEvents() {
	std::vector<Event>::iterator it;

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		if ((*it).type == "on_load") {
			if (EventManager::executeEvent(*it))
				it = events.erase(it);
		}
	}
}

void MapRenderer::executeOnMapExitEvents() {
	std::vector<Event>::iterator it;

	// We're leaving the map, so the events of this map are removed anyway in
	// the next frame (Reminder: We're about to load a new map ;),
	// so we will ignore the events keep_after_trigger value and do not delete
	// any event in this loop
	for (it = events.begin(); it != events.end(); ++it) {

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		if ((*it).type == "on_mapexit")
			EventManager::executeEvent(*it); // ignore repeat value
	}
}

void MapRenderer::checkEvents(FPoint loc) {
	Point maploc;
	maploc.x = int(loc.x);
	maploc.y = int(loc.y);
	std::vector<Event>::iterator it;

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		if ((*it).type == "on_clear") {
			if (enemies_cleared && EventManager::executeEvent(*it))
				it = events.erase(it);
			continue;
		}

		bool inside = maploc.x >= (*it).location.x &&
					  maploc.y >= (*it).location.y &&
					  maploc.x <= (*it).location.x + (*it).location.w-1 &&
					  maploc.y <= (*it).location.y + (*it).location.h-1;

		if ((*it).type == "on_leave") {
			if (inside) {
				if (!(*it).getComponent("wasInsideEventArea")) {
					(*it).components.push_back(Event_Component());
					(*it).components.back().type = std::string("wasInsideEventArea");
				}
			}
			else {
				if ((*it).getComponent("wasInsideEventArea")) {
					(*it).deleteAllComponents("wasInsideEventArea");
					if (EventManager::executeEvent(*it))
						it = events.erase(it);
				}
			}
		}
		else {
			if (inside)
				if (EventManager::executeEvent(*it))
					it = events.erase(it);
		}
	}
}

/**
 * Some events have a hotspot (rectangle screen area) where the user can click
 * to trigger the event.
 *
 * The hero must be within range (INTERACT_RANGE) to activate an event.
 *
 * This function checks valid mouse clicks against all clickable events, and
 * executes
 */
void MapRenderer::checkHotspots() {
	if (NO_MOUSE) return;

	show_tooltip = false;

	std::vector<Event>::iterator it;

	// work backwards through events because events can be erased in the loop.
	// this prevents the iterator from becoming invalid.
	for (it = events.end(); it != events.begin(); ) {
		--it;

		for (int x=it->hotspot.x; x < it->hotspot.x + it->hotspot.w; ++x) {
			for (int y=it->hotspot.y; y < it->hotspot.y + it->hotspot.h; ++y) {
				bool matched = false;
				bool is_npc = false;

				Event_Component* npc = (*it).getComponent("npc_hotspot");
				if (npc) {
					is_npc = true;

					Point p = map_to_screen(float(npc->x), float(npc->y), shakycam.x, shakycam.y);
					p = center_tile(p);

					Rect dest;
					dest.x = p.x - npc->z;
					dest.y = p.y - npc->a;
					dest.w = npc->b;
					dest.h = npc->c;

					if (isWithin(dest, inpt->mouse)) {
						matched = true;
						tip_pos.x = dest.x + dest.w/2;
						tip_pos.y = p.y - npc_tooltip_margin;
					}
				}
				else {
					for (unsigned index = 0; index <= index_objectlayer; ++index) {
						maprow *current_layer = layers[index];
						Point p = map_to_screen(float(x), float(y), shakycam.x, shakycam.y);
						p = center_tile(p);

						if (const short current_tile = current_layer[x][y]) {
							// first check if mouse pointer is in rectangle of that tile:
							Rect dest;
							dest.x = p.x - tset.tiles[current_tile].offset.x;
							dest.y = p.y - tset.tiles[current_tile].offset.y;
							dest.w = tset.tiles[current_tile].tile->getClip().w;
							dest.h = tset.tiles[current_tile].tile->getClip().h;

							if (isWithin(dest, inpt->mouse)) {
								matched = true;
								tip_pos = map_to_screen(it->center.x, it->center.y, shakycam.x, shakycam.y);
								tip_pos.y -= TILE_H;
							}
						}
					}
				}

				if (matched) {
					// skip inactive events
					if (!EventManager::isActive(*it)) continue;

					// skip events without hotspots
					if ((*it).hotspot.h == 0) continue;

					// skip events on cooldown
					if ((*it).cooldown_ticks != 0) continue;

					// new tooltip?
					createTooltip((*it).getComponent("tooltip"));

					if ((((*it).reachable_from.w == 0 && (*it).reachable_from.h == 0) || isWithin((*it).reachable_from, floor(cam)))
							&& calcDist(cam, (*it).center) < INTERACT_RANGE) {

						// only check events if the player is clicking
						// and allowed to click
						if (is_npc) {
							curs->setCursor(CURSOR_TALK);
						}
						else {
							curs->setCursor(CURSOR_INTERACT);
						}
						if (!inpt->pressing[MAIN1]) return;
						else if (inpt->lock[MAIN1]) return;

						inpt->lock[MAIN1] = true;
						if (EventManager::executeEvent(*it))
							it = events.erase(it);
					}
					return;
				}
				else show_tooltip = false;
			}
		}
	}
}

void MapRenderer::checkNearestEvent() {
	if (NO_MOUSE) show_tooltip = false;

	std::vector<Event>::iterator it;
	std::vector<Event>::iterator nearest = events.end();
	float best_distance = std::numeric_limits<float>::max();

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		// skip events without hotspots
		if ((*it).hotspot.h == 0) continue;

		// skip events on cooldown
		if ((*it).cooldown_ticks != 0) continue;

		float distance = calcDist(cam, (*it).center);
		if ((((*it).reachable_from.w == 0 && (*it).reachable_from.h == 0) || isWithin((*it).reachable_from, floor(cam)))
				&& distance < INTERACT_RANGE && distance < best_distance) {
			best_distance = distance;
			nearest = it;
		}

	}

	if (nearest != events.end()) {
		if (NO_MOUSE || TOUCHSCREEN) {
			// new tooltip?
			createTooltip((*nearest).getComponent("tooltip"));
			tip_pos = map_to_screen((*nearest).center.x, (*nearest).center.y, shakycam.x, shakycam.y);
			if ((*nearest).getComponent("npc_hotspot")) {
				tip_pos.y -= npc_tooltip_margin;
			}
			else {
				tip_pos.y -= TILE_H;
			}
		}

		if (inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
			if (inpt->pressing[ACCEPT]) inpt->lock[ACCEPT] = true;

			if(EventManager::executeEvent(*nearest))
				events.erase(nearest);
		}
	}
}

void MapRenderer::checkTooltip() {
	if (show_tooltip)
		tip->render(tip_buf, tip_pos, STYLE_TOPLABEL);
}

void MapRenderer::createTooltip(Event_Component *ec) {
	if (ec && !ec->s.empty() && TOOLTIP_CONTEXT != TOOLTIP_MENU) {
		show_tooltip = true;
		if (!tip_buf.compareFirstLine(ec->s)) {
			tip_buf.clear();
			tip_buf.addText(ec->s);
		}
		TOOLTIP_CONTEXT = TOOLTIP_MAP;
	}
	else if (TOOLTIP_CONTEXT != TOOLTIP_MENU) {
		TOOLTIP_CONTEXT = TOOLTIP_NONE;
	}
}

/**
 * Activate a power that is attached to an event
 */
void MapRenderer::activatePower(int power_index, unsigned statblock_index, FPoint &target) {
	if (statblock_index < statblocks.size()) {
		// check power cooldown before activating
		if (statblocks[statblock_index].power_ticks[0] == 0) {
			statblocks[statblock_index].power_ticks[0] = powers->powers[power_index].cooldown;
			powers->activate(power_index, &statblocks[statblock_index], target);
		}
	}
	else {
		logError("MapRenderer: StatBlock index is out of bounds.\n");
	}
}

bool MapRenderer::isValidTile(const unsigned &tile) {
	if (tile == 0)
		return true;

	if (tile >= tset.tiles.size())
		return false;

	return tset.tiles[tile].tile != NULL;
}

MapRenderer::~MapRenderer() {
	if (music != NULL) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
	}

	tip_buf.clear();
	clearLayers();
	clearEvents();
	clearQueues();
	delete tip;

	/* unload sounds */
	snd->reset();
	while (!sids.empty()) {
		snd->unload(sids.back());
		sids.pop_back();
	}
}

