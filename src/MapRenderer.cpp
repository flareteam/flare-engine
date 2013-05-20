/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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

using namespace std;

const int CLICK_RANGE = 3 * UNITS_PER_TILE; //for activating events

MapRenderer::MapRenderer(CampaignManager *_camp)
	: Map()
	, music(NULL)
	, tip(new WidgetTooltip())
	, tip_pos()
	, show_tooltip(false)
	, shakycam(Point())
	, backgroundsurface(NULL)
	, backgroundsurfaceoffset()
	, repaint_background(false)
	, camp(_camp)
	, powers(NULL)
	, cam()
	, hero_tile()
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
	, enemies_cleared(false) {
}

void MapRenderer::clearQueues() {
	Map::clearQueues();
	loot.clear();
}

void MapRenderer::push_enemy_group(Map_Group g) {
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
	int allowed_misses = 3 * g.numbermax;

	while (enemies_to_spawn && allowed_misses) {

		int x = (g.pos.x + (rand() % g.area.x)) * UNITS_PER_TILE + UNITS_PER_TILE / 2;
		int y = (g.pos.y + (rand() % g.area.y)) * UNITS_PER_TILE + UNITS_PER_TILE / 2;
		bool success = false;

		if (collider.is_empty(x, y)) {
			Enemy_Level enemy_lev = enemyg->getRandomEnemy(g.category, g.levelmin, g.levelmax);
			if (enemy_lev.type != "") {
				Map_Enemy group_member = Map_Enemy(enemy_lev.type, Point(x, y));
				collider.block(x, y, false);
				enemies.push(group_member);

				success = true;
			}
		}
		if (success)
			enemies_to_spawn--;
		else
			allowed_misses--;
	}
	if (enemies_to_spawn)
		fprintf(stderr, "Could not spawn all enemies in group at (x=%d,y=%d,w=%d,h=%d), %d missing\n",
		g.pos.x, g.pos.y, g.area.x, g.area.y, enemies_to_spawn);
}

/**
 * No guarantee that maps will use all layers
 * Clear all tile layers (e.g. when loading a map)
 */
void MapRenderer::clearLayers() {
	Map::clearLayers();

	SDL_FreeSurface(backgroundsurface);
	backgroundsurface = 0;
	index_objectlayer = 0;
}

int MapRenderer::load(std::string filename) {
	/* unload sounds */
	snd->reset();
	while (!sids.empty()) {
		snd->unload(sids.back());
		sids.pop_back();
	}

	show_tooltip = false;

	Map::load(filename);

	loadMusic();

	for (unsigned i = 0; i < layers.size(); ++i) {
		if (layernames[i] == "collision") {
			collider.setmap(layers[i], w, h);
			layernames.erase(layernames.begin() + i);
			layers.erase(layers.begin() + i);
		}
	}
	for (unsigned i = 0; i < layers.size(); ++i)
		if (layernames[i] == "object")
			index_objectlayer = i;

	while (!enemy_groups.empty()) {
		push_enemy_group(enemy_groups.front());
		enemy_groups.pop();
	}

	tset.load(this->tileset);

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
		music = Mix_LoadMUS(mods->locate("music/" + played_music_filename).c_str());
		if(!music)
			cout << "Mix_LoadMUS: "<< Mix_GetError()<<endl;
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
	vector<Map_Event>::iterator it;
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
void calculatePriosIso(vector<Renderable> &r) {
	for (vector<Renderable>::iterator it = r.begin(); it != r.end(); ++it) {
		const unsigned tilex = it->map_pos.x >> TILE_SHIFT;
		const unsigned tiley = it->map_pos.y >> TILE_SHIFT;
		it->prio += (((uint64_t)(tilex + tiley)) << 48) + (((uint64_t)tilex) << 32) + ((it->map_pos.x + it->map_pos.y) << 16);
	}
}

void calculatePriosOrtho(vector<Renderable> &r) {
	for (vector<Renderable>::iterator it = r.begin(); it != r.end(); ++it) {
		const unsigned tilex = it->map_pos.x >> TILE_SHIFT;
		const unsigned tiley = it->map_pos.y >> TILE_SHIFT;
		it->prio += (((uint64_t)tiley) << 48) + (((uint64_t)tilex) << 32) + (it->map_pos.y << 16);
	}
}

void MapRenderer::render(vector<Renderable> &r, vector<Renderable> &r_dead) {

	if (shaky_cam_ticks == 0) {
		shakycam.x = cam.x;
		shakycam.y = cam.y;
	}
	else {
		shakycam.x = cam.x + (rand() % 16 - 8) /UNITS_PER_PIXEL_X;
		shakycam.y = cam.y + (rand() % 16 - 8) /UNITS_PER_PIXEL_Y;
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

void MapRenderer::createBackgroundSurface() {
	SDL_FreeSurface(backgroundsurface);
	backgroundsurface = createSurface(
							VIEW_W + 2 * movedistance_to_rerender * TILE_W * tset.max_size_x,
							VIEW_H + 2 * movedistance_to_rerender * TILE_H * tset.max_size_y);
	// background has no alpha:
	SDL_SetColorKey(backgroundsurface, 0, 0);
}

void MapRenderer::drawRenderable(vector<Renderable>::iterator r_cursor) {
	if (r_cursor->sprite) {
		SDL_Rect dest;
		Point p = map_to_screen(r_cursor->map_pos.x, r_cursor->map_pos.y, shakycam.x, shakycam.y);
		dest.x = p.x - r_cursor->offset.x;
		dest.y = p.y - r_cursor->offset.y;
		SDL_BlitSurface(r_cursor->sprite, &r_cursor->src, screen, &dest);
	}
}

void MapRenderer::renderIsoLayer(SDL_Surface *wheretorender, Point offset, const unsigned short layerdata[256][256]) {
	int_fast16_t i; // first index of the map array
	int_fast16_t j; // second index of the map array
	SDL_Rect dest;
	const Point upperright = screen_to_map(0, 0, shakycam.x, shakycam.y);
	const int_fast16_t max_tiles_width =   (VIEW_W / TILE_W) + 2 * tset.max_size_x;
	const int_fast16_t max_tiles_height = ((2 * VIEW_H / TILE_H) + 2 * tset.max_size_y) * 2;

	j = upperright.y / UNITS_PER_TILE - tset.max_size_y + tset.max_size_x;
	i = upperright.x / UNITS_PER_TILE - tset.max_size_y - tset.max_size_x;

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

		Point p = map_to_screen(i * UNITS_PER_TILE, j * UNITS_PER_TILE, shakycam.x, shakycam.y);
		p = center_tile(p);

		// draw one horizontal line
		while (j > j_end) {
			--j;
			++i;
			++tiles_width;
			p.x += TILE_W;

			if (const uint_fast16_t current_tile = layerdata[i][j]) {
				dest.x = p.x - tset.tiles[current_tile].offset.x + offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y + offset.y;
				// no need to set w and h in dest, as it is ignored
				// by SDL_BlitSurface
				SDL_BlitSurface(tset.sprites, &(tset.tiles[current_tile].src), wheretorender, &dest);
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

void MapRenderer::renderIsoBackObjects(vector<Renderable> &r) {
	vector<Renderable>::iterator it;
	for (it = r.begin(); it != r.end(); ++it)
		drawRenderable(it);
}

void MapRenderer::renderIsoFrontObjects(vector<Renderable> &r) {
	int_fast16_t i;
	int_fast16_t j;
	SDL_Rect dest;
	const Point upperright = screen_to_map(0, 0, shakycam.x, shakycam.y);
	const int_fast16_t max_tiles_width =   (VIEW_W / TILE_W) + 2 * tset.max_size_x;
	const int_fast16_t max_tiles_height = ((VIEW_H / TILE_H) + 2 * tset.max_size_y)*2;

	vector<Renderable>::iterator r_cursor = r.begin();
	vector<Renderable>::iterator r_end = r.end();

	// object layer
	j = upperright.y / UNITS_PER_TILE - tset.max_size_y + tset.max_size_x;
	i = upperright.x / UNITS_PER_TILE - tset.max_size_y - tset.max_size_x;

	while (r_cursor != r_end && ((r_cursor->map_pos.x>>TILE_SHIFT) + (r_cursor->map_pos.y>>TILE_SHIFT) < i + j || (r_cursor->map_pos.x>>TILE_SHIFT) < i))
		++r_cursor;

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
		Point p = map_to_screen(i * UNITS_PER_TILE, j * UNITS_PER_TILE, shakycam.x, shakycam.y);
		p = center_tile(p);
		while (j > j_end) {
			--j;
			++i;
			++tiles_width;
			p.x += TILE_W;

			if (const uint_fast16_t current_tile = objectlayer[i][j]) {

				dest.x = p.x - tset.tiles[current_tile].offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y;
				SDL_BlitSurface(tset.sprites, &(tset.tiles[current_tile].src), screen, &dest);
			}

			// some renderable entities go in this layer
			while (r_cursor != r_end && (r_cursor->map_pos.x>>TILE_SHIFT) == i && (r_cursor->map_pos.y>>TILE_SHIFT) == j) {
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

		while (r_cursor != r_end && ((r_cursor->map_pos.x>>TILE_SHIFT) + (r_cursor->map_pos.y>>TILE_SHIFT) < i + j || (r_cursor->map_pos.x>>TILE_SHIFT) <= i))
			++r_cursor;
	}
}

void MapRenderer::renderIso(vector<Renderable> &r, vector<Renderable> &r_dead) {
	const Point nulloffset(0, 0);
	if (ANIMATED_TILES) {
		for (unsigned i = 0; i < index_objectlayer; ++i)
			renderIsoLayer(screen, nulloffset, layers[i]);
	}
	else {
		if (abs(shakycam.x - backgroundsurfaceoffset.x) > movedistance_to_rerender * TILE_W
				|| abs(shakycam.y - backgroundsurfaceoffset.y) > movedistance_to_rerender * TILE_H
				|| repaint_background) {

			if (!backgroundsurface)
				createBackgroundSurface();

			repaint_background = false;

			backgroundsurfaceoffset = shakycam;

			SDL_FillRect(backgroundsurface, 0, 0);
			Point off(VIEW_W_HALF, VIEW_H_HALF);
			for (unsigned i = 0; i < index_objectlayer; ++i)
					renderIsoLayer(screen, off, layers[i]);
		}
		Point p = map_to_screen(shakycam.x, shakycam.y , backgroundsurfaceoffset.x, backgroundsurfaceoffset.y);
		SDL_Rect src;
		src.x = p.x;
		src.y = p.y;
		src.w = 2 * VIEW_W;
		src.h = 2 * VIEW_H;
		SDL_BlitSurface(backgroundsurface, &src, screen , 0);
	}

	renderIsoBackObjects(r_dead);
	renderIsoFrontObjects(r);
	for (unsigned i = index_objectlayer + 1; i < layers.size(); ++i)
		renderIsoLayer(screen, nulloffset, layers[i]);

	checkTooltip();
}

void MapRenderer::renderOrthoLayer(const unsigned short layerdata[256][256]) {

	const Point upperright = screen_to_map(0, 0, shakycam.x, shakycam.y);

	short int startj = max(0, upperright.y / UNITS_PER_TILE);
	short int starti = max(0, upperright.x / UNITS_PER_TILE);
	const short max_tiles_width =  min(w, static_cast<short int>(starti + (VIEW_W / TILE_W) + 2 * tset.max_size_x));
	const short max_tiles_height = min(h, static_cast<short int>(startj + (VIEW_H / TILE_H) + 2 * tset.max_size_y));

	short int i;
	short int j;

	for (j = startj; j < max_tiles_height; j++) {
		Point p = map_to_screen(starti * UNITS_PER_TILE, j * UNITS_PER_TILE, shakycam.x, shakycam.y);
		p = center_tile(p);
		for (i = starti; i < max_tiles_width; i++) {

			if (const unsigned short current_tile = layerdata[i][j]) {
				SDL_Rect dest;
				dest.x = p.x - tset.tiles[current_tile].offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y;
				SDL_BlitSurface(tset.sprites, &(tset.tiles[current_tile].src), screen, &dest);
			}
			p.x += TILE_W;
		}
	}
}

void MapRenderer::renderOrthoBackObjects(std::vector<Renderable> &r) {
	// some renderables are drawn above the background and below the objects
	vector<Renderable>::iterator it;
	for (it = r.begin(); it != r.end(); ++it)
		drawRenderable(it);
}

void MapRenderer::renderOrthoFrontObjects(std::vector<Renderable> &r) {

	short int i;
	short int j;
	SDL_Rect dest;
	vector<Renderable>::iterator r_cursor = r.begin();
	vector<Renderable>::iterator r_end = r.end();

	const Point upperright = screen_to_map(0, 0, shakycam.x, shakycam.y);

	short int startj = max(0, upperright.y / UNITS_PER_TILE);
	short int starti = max(0, upperright.x / UNITS_PER_TILE);
	const short max_tiles_width =  min(w, static_cast<short int>(starti + (VIEW_W / TILE_W) + 2 * tset.max_size_x));
	const short max_tiles_height = min(h, static_cast<short int>(startj + (VIEW_H / TILE_H) + 2 * tset.max_size_y));

	while (r_cursor != r_end && (r_cursor->map_pos.y>>TILE_SHIFT) < startj)
		++r_cursor;

	maprow *objectlayer = layers[index_objectlayer];
	for (j = startj; j<max_tiles_height; j++) {
		Point p = map_to_screen(starti * UNITS_PER_TILE, j * UNITS_PER_TILE, shakycam.x, shakycam.y);
		p = center_tile(p);
		for (i = starti; i<max_tiles_width; i++) {

			if (const unsigned short current_tile = objectlayer[i][j]) {
				dest.x = p.x - tset.tiles[current_tile].offset.x;
				dest.y = p.y - tset.tiles[current_tile].offset.y;
				SDL_BlitSurface(tset.sprites, &(tset.tiles[current_tile].src), screen, &dest);
			}
			p.x += TILE_W;

			while (r_cursor != r_end && (r_cursor->map_pos.y>>TILE_SHIFT) == j && (r_cursor->map_pos.x>>TILE_SHIFT) < i)
				++r_cursor;

			// some renderable entities go in this layer
			while (r_cursor != r_end && (r_cursor->map_pos.y>>TILE_SHIFT) == j && (r_cursor->map_pos.x>>TILE_SHIFT) == i)
				drawRenderable(r_cursor++);
		}
		while (r_cursor != r_end && (r_cursor->map_pos.y>>TILE_SHIFT) <= j)
			++r_cursor;
	}
}

void MapRenderer::renderOrtho(vector<Renderable> &r, vector<Renderable> &r_dead) {

	unsigned index = 0;
	while (index < index_objectlayer)
		renderOrthoLayer(layers[index++]);

	renderOrthoBackObjects(r_dead);
	renderOrthoFrontObjects(r);
	index++;

	while (index < layers.size())
		renderOrthoLayer(layers[index++]);

	//render event tooltips
	checkTooltip();
}

void MapRenderer::executeOnLoadEvents() {
	vector<Map_Event>::iterator it;

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!isActive(*it)) continue;

		if ((*it).type == "on_load") {
			if (executeEvent(*it))
				it = events.erase(it);
		}
	}
}

void MapRenderer::executeOnMapExitEvents() {
	vector<Map_Event>::iterator it;

	// We're leaving the map, so the events of this map are removed anyway in
	// the next frame (Reminder: We're about to load a new map ;),
	// so we will ignore the events keep_after_trigger value and do not delete
	// any event in this loop
	for (it = events.begin(); it != events.end(); ++it) {

		// skip inactive events
		if (!isActive(*it)) continue;

		if ((*it).type == "on_mapexit")
			executeEvent(*it); // ignore repeat value
	}
}

void MapRenderer::checkEvents(Point loc) {
	Point maploc;
	maploc.x = loc.x >> TILE_SHIFT;
	maploc.y = loc.y >> TILE_SHIFT;
	vector<Map_Event>::iterator it;

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!isActive(*it)) continue;

		if ((*it).type == "on_clear") {
			if (enemies_cleared && executeEvent(*it))
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
					(*it).components.back().type = string("wasInsideEventArea");
				}
			}
			else {
				if ((*it).getComponent("wasInsideEventArea")) {
					(*it).deleteAllComponents("wasInsideEventArea");
					if (executeEvent(*it))
						it = events.erase(it);
				}
			}
		}
		else {
			if (inside)
				if (executeEvent(*it))
					it = events.erase(it);
		}
	}
}

/**
 * Some events have a hotspot (rectangle screen area) where the user can click
 * to trigger the event.
 *
 * The hero must be within range (CLICK_RANGE) to activate an event.
 *
 * This function checks valid mouse clicks against all clickable events, and
 * executes
 */
void MapRenderer::checkHotspots() {
	show_tooltip = false;

	vector<Map_Event>::iterator it;

	// work backwards through events because events can be erased in the loop.
	// this prevents the iterator from becoming invalid.
	for (it = events.end(); it != events.begin(); ) {
		--it;

		for (int x=it->hotspot.x; x < it->hotspot.x + it->hotspot.w; ++x) {
			for (int y=it->hotspot.y; y < it->hotspot.y + it->hotspot.h; ++y) {
				bool matched = false;
				for (unsigned index = 0; index <= index_objectlayer; ++index) {
					maprow *current_layer = layers[index];
					Point p = map_to_screen(x * UNITS_PER_TILE,
											y * UNITS_PER_TILE,
											shakycam.x,
											shakycam.y);
					p = center_tile(p);

					if (const short current_tile = current_layer[x][y]) {
						// first check if mouse pointer is in rectangle of that tile:
						SDL_Rect dest;
						dest.x = p.x - tset.tiles[current_tile].offset.x;
						dest.y = p.y - tset.tiles[current_tile].offset.y;
						dest.w = tset.tiles[current_tile].src.w;
						dest.h = tset.tiles[current_tile].src.h;

						if (isWithin(dest, inpt->mouse)) {
							// Now that the mouse is within the rectangle of the tile, we can check for
							// pixel precision. We need to have checked the rectangle first, because
							// otherwise the pixel precise check might hit a neighbouring tile in the
							// tileset. We need to calculate the point relative to the
							Point p1;
							p1.x = inpt->mouse.x - dest.x + tset.tiles[current_tile].src.x;
							p1.y = inpt->mouse.y - dest.y + tset.tiles[current_tile].src.y;
							matched |= checkPixel(p1, tset.sprites);
							tip_pos.x = dest.x + dest.w/2;
							tip_pos.y = dest.y;
						}
					}
				}

				if (matched) {
					// skip inactive events
					if (!isActive(*it)) continue;

					// skip events without hotspots
					if ((*it).hotspot.h == 0) continue;

					// skip events on cooldown
					if ((*it).cooldown_ticks != 0) continue;

					// new tooltip?
					Event_Component *ec = (*it).getComponent("tooltip");
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

					if ((abs(cam.x - (*it).location.x * UNITS_PER_TILE) < CLICK_RANGE)
							&& (abs(cam.y - (*it).location.y * UNITS_PER_TILE) < CLICK_RANGE)) {

						// only check events if the player is clicking
						// and allowed to click
						if (!inpt->pressing[MAIN1]) return;
						else if (inpt->lock[MAIN1]) return;

						inpt->lock[MAIN1] = true;
						if (executeEvent(*it))
							it = events.erase(it);
					}
					return;
				}
				else show_tooltip = false;
			}
		}
	}
}

void MapRenderer::checkNearestEvent(Point loc) {
	if (inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
		if (inpt->pressing[ACCEPT]) inpt->lock[ACCEPT] = true;

		vector<Map_Event>::iterator it;
		vector<Map_Event>::iterator nearest = events.end();
		int best_distance = std::numeric_limits<int>::max();

		// loop in reverse because we may erase elements
		for (it = events.end(); it != events.begin(); ) {
			--it;

			// skip inactive events
			if (!isActive(*it)) continue;

			// skip events without hotspots
			if ((*it).hotspot.h == 0) continue;

			// skip events on cooldown
			if ((*it).cooldown_ticks != 0) continue;

			Point ev_loc;
			ev_loc.x = (*it).location.x * UNITS_PER_TILE;
			ev_loc.y = (*it).location.y * UNITS_PER_TILE;
			int distance = (int)calcDist(loc,ev_loc);
			if (distance < CLICK_RANGE && distance < best_distance) {
				best_distance = distance;
				nearest = it;
			}
		}
		if (nearest != events.end()) {
			if(executeEvent(*nearest))
				events.erase(nearest);
		}
	}
}

bool MapRenderer::isActive(const Map_Event &e) {
	for (unsigned i=0; i < e.components.size(); i++) {
		if (e.components[i].type == "requires_not") {
			if (camp->checkStatus(e.components[i].s)) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_status") {
			if (!camp->checkStatus(e.components[i].s)) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_item") {
			if (!camp->checkItem(e.components[i].x)) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_level") {
			if (camp->hero->level < e.components[i].x) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_not_level") {
			if (camp->hero->level >= e.components[i].x) {
				return false;
			}
		}
	}
	return true;
}

void MapRenderer::checkTooltip() {
	if (show_tooltip)
		tip->render(tip_buf, tip_pos, STYLE_TOPLABEL);
}

/**
 * A particular event has been triggered.
 * Process all of this events components.
 *
 * @param The triggered event
 * @return Returns true if the event shall not be run again.
 */
bool MapRenderer::executeEvent(Map_Event &ev) {
	if(&ev == NULL) return false;

	// skip executing events that are on cooldown
	if (ev.cooldown_ticks > 0) return false;

	// set cooldown
	ev.cooldown_ticks = ev.cooldown;

	const Event_Component *ec;

	for (unsigned i = 0; i < ev.components.size(); ++i) {
		ec = &ev.components[i];

		if (ec->type == "set_status") {
			camp->setStatus(ec->s);
		}
		else if (ec->type == "unset_status") {
			camp->unsetStatus(ec->s);
		}
		else if (ec->type == "intermap") {

			if (fileExists(mods->locate("maps/" + ec->s))) {
				teleportation = true;
				teleport_mapname = ec->s;
				teleport_destination.x = ec->x * UNITS_PER_TILE + UNITS_PER_TILE/2;
				teleport_destination.y = ec->y * UNITS_PER_TILE + UNITS_PER_TILE/2;
			}
			else {
				ev.keep_after_trigger = false;
				log_msg = msg->get("Unknown destination");
			}
		}
		else if (ec->type == "intramap") {
			teleportation = true;
			teleport_mapname = "";
			teleport_destination.x = ec->x * UNITS_PER_TILE + UNITS_PER_TILE/2;
			teleport_destination.y = ec->y * UNITS_PER_TILE + UNITS_PER_TILE/2;
		}
		else if (ec->type == "mapmod") {
			if (ec->s == "collision") {
				collider.colmap[ec->x][ec->y] = ec->z;
			}
			else {
				int index = distance(layernames.begin(), find(layernames.begin(), layernames.end(), ec->s));
				layers[index][ec->x][ec->y] = ec->z;

				if (ec->a < (int)(index_objectlayer))
					repaint_background = true;
			}
			map_change = true;
		}
		else if (ec->type == "soundfx") {
			Point pos(0,0);
			bool loop = false;

			if (ec->x != -1 && ec->y != -1) {
				if (ec->x != 0 && ec->y != 0) {
					pos.x = ec->x * UNITS_PER_TILE + UNITS_PER_TILE/2;
					pos.y = ec->y * UNITS_PER_TILE + UNITS_PER_TILE/2;
				}
			}
			else if (ev.location.x != 0 && ev.location.y != 0) {
				pos.x = ev.location.x * UNITS_PER_TILE + UNITS_PER_TILE/2;
				pos.y = ev.location.y * UNITS_PER_TILE + UNITS_PER_TILE/2;
			}

			if (ev.type == "on_load")
				loop = true;

			SoundManager::SoundID sid = snd->load(ec->s, "MapRenderer background soundfx");

			snd->play(sid, GLOBAL_VIRTUAL_CHANNEL, pos, loop);
			sids.push_back(sid);
		}
		else if (ec->type == "loot") {
			loot.push_back(*ec);
		}
		else if (ec->type == "msg") {
			log_msg = ec->s;
		}
		else if (ec->type == "shakycam") {
			shaky_cam_ticks = ec->x;
		}
		else if (ec->type == "remove_item") {
			camp->removeItem(ec->x);
		}
		else if (ec->type == "reward_xp") {
			camp->rewardXP(ec->x, true);
		}
		else if (ec->type == "spawn") {
			Point spawn_pos;
			spawn_pos.x = ec->x;
			spawn_pos.y = ec->y;
			powers->spawn(ec->s, spawn_pos);
		}
		else if (ec->type == "power") {

			int power_index = ec->x;

			Event_Component *ec_path = ev.getComponent("power_path");
			if (ev.stats == NULL) {
				ev.stats = new StatBlock();

				ev.stats->current[STAT_ACCURACY] = 1000; //always hits its target

				// if a power path was specified, place the source position there
				if (ec_path) {
					ev.stats->pos.x = ec_path->x * UNITS_PER_TILE + UNITS_PER_TILE/2;
					ev.stats->pos.y = ec_path->y * UNITS_PER_TILE + UNITS_PER_TILE/2;
				}
				// otherwise the source position is the event position
				else {
					ev.stats->pos.x = ev.location.x * UNITS_PER_TILE + UNITS_PER_TILE/2;
					ev.stats->pos.y = ev.location.y * UNITS_PER_TILE + UNITS_PER_TILE/2;
				}

				Event_Component *ec_damage = ev.getComponent("power_damage");
				if (ec_damage) {
					ev.stats->current[STAT_DMG_MELEE_MIN] = ev.stats->current[STAT_DMG_RANGED_MIN] = ev.stats->current[STAT_DMG_MENT_MIN] = ec_damage->a;
					ev.stats->current[STAT_DMG_MELEE_MAX] = ev.stats->current[STAT_DMG_RANGED_MAX] = ev.stats->current[STAT_DMG_MENT_MAX] = ec_damage->b;
				}
			}

			Point target;

			if (ec_path) {
				// targets hero option
				if (ec_path->s == "hero") {
					target.x = cam.x;
					target.y = cam.y;
				}
				// targets fixed path option
				else {
					target.x = ec_path->a * UNITS_PER_TILE + UNITS_PER_TILE/2;
					target.y = ec_path->b * UNITS_PER_TILE + UNITS_PER_TILE/2;
				}
			}
			// no path specified, targets self location
			else {
				target.x = ev.stats->pos.x;
				target.y = ev.stats->pos.y;
			}

			powers->activate(power_index, ev.stats, target);
		}
		else if (ec->type == "stash") {
			stash = true;
			stash_pos.x = ev.location.x * UNITS_PER_TILE + UNITS_PER_TILE/2;
			stash_pos.y = ev.location.y * UNITS_PER_TILE + UNITS_PER_TILE/2;
		}
		else if (ec->type == "npc") {
			event_npc = ec->s;
		}
		else if (ec->type == "music") {
			music_filename = ec->s;
			loadMusic();
		}
		else if (ec->type == "cutscene") {
			cutscene = true;
			cutscene_file = ec->s;
		}
		else if (ec->type == "repeat") {
			ev.keep_after_trigger = toBool(ec->s);
		}
	}
	return !ev.keep_after_trigger;
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

