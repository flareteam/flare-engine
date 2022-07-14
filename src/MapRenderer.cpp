/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert
Copyright © 2012-2016 Justin Jacobs

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
#include "Camera.h"
#include "CampaignManager.h"
#include "CombatText.h"
#include "CommonIncludes.h"
#include "CursorManager.h"
#include "EnemyGroupManager.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EngineSettings.h"
#include "EventManager.h"
#include "FogOfWar.h"
#include "FontEngine.h"
#include "Hazard.h"
#include "HazardManager.h"
#include "InputState.h"
#include "MapRenderer.h"
#include "MenuDevConsole.h"
#include "MenuManager.h"
#include "NPC.h"
#include "NPCManager.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "StatBlock.h"
#include "TooltipManager.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "WidgetTooltip.h"

#include <stdint.h>
#include <limits>
#include <math.h>

MapRenderer::MapRenderer()
	: Map()
	, tip(new WidgetTooltip())
	, tip_pos()
	, show_tooltip(false)
	, entity_hidden_normal(NULL)
	, entity_hidden_enemy(NULL)
	, cam()
	, map_change(false)
	, teleportation(false)
	, teleport_destination()
	, respawn_point()
	, cutscene(false)
	, cutscene_file("")
	, stash(false)
	, stash_pos()
	, enemies_cleared(false)
	, save_game(false)
	, npc_id(-1)
	, show_book("")
	, index_objectlayer(0)
	, is_spawn_map(false)
{
	// Load entity markers
	Image *gfx = render_device->loadImage("images/menus/entity_hidden.png", RenderDevice::ERROR_NORMAL);
	if (gfx) {
		const int gfx_w = gfx->getWidth();
		const int gfx_h = gfx->getHeight();

		entity_hidden_normal = gfx->createSprite();
		entity_hidden_normal->setClip(0, 0, gfx_w, gfx_h/2);

		entity_hidden_enemy = gfx->createSprite();
		entity_hidden_enemy->setClip(0, gfx_h/2, gfx_w, gfx_h/2);

		gfx->unref();
	}
}

void MapRenderer::clearQueues() {
	Map::clearQueues();
	loot.clear();
}

bool MapRenderer::enemyGroupPlaceEnemy(float x, float y, const Map_Group &g) {
	if (collider.isEmpty(x, y)) {
		Enemy_Level enemy_lev = enemyg->getRandomEnemy(g.category, g.levelmin, g.levelmax);
		if (!enemy_lev.type.empty()) {
			Map_Enemy group_member = Map_Enemy(enemy_lev.type, FPoint(x, y));

			group_member.direction = (g.direction == -1 ? rand()%8 : g.direction);
			group_member.wander_radius = g.wander_radius;
			group_member.requirements = g.requirements;
			group_member.invincible_requirements = g.invincible_requirements;

			if (g.area.x == 1 && g.area.y == 1) {
				// this is a single enemy
				group_member.waypoints = g.waypoints;
			}

			group_member.spawn_level = g.spawn_level;

			enemies.push(group_member);
		}
		return true;
	}
	return false;
}

void MapRenderer::pushEnemyGroup(Map_Group &g) {
	// activate at all?
	if (!Math::percentChanceF(g.chance)) {
		return;
	}

	// The algorithm tries to place the enemies at random locations.
	// However if a location is not possible (unwalkable or there is already an entity),
	// then try again.
	// This could result in an infinite loop if there were more enemies than
	// actual places, so have an upper bound of tries.

	// random number of enemies
	int enemies_to_spawn = Math::randBetween(g.numbermin, g.numbermax);

	// pick an upper bound, which is definitely larger than threetimes the enemy number to spawn.
	int allowed_misses = 5 * g.numbermax;

	while (enemies_to_spawn > 0 && allowed_misses > 0) {

		float x = (g.area.x == 0) ? (static_cast<float>(g.pos.x) + 0.5f) : (static_cast<float>(g.pos.x + (rand() % g.area.x))) + 0.5f;
		float y = (g.area.y == 0) ? (static_cast<float>(g.pos.y) + 0.5f) : (static_cast<float>(g.pos.y + (rand() % g.area.y))) + 0.5f;

		if (enemyGroupPlaceEnemy(x, y, g))
			enemies_to_spawn--;
		else
			allowed_misses--;
	}
	if (enemies_to_spawn > 0) {
		// now that the fast method of spawning enemies doesn't work, but we
		// still have enemies to place, do not place them randomly, but at the
		// first free spot
		for (int x = g.pos.x; x < g.pos.x + g.area.x && enemies_to_spawn > 0; x++) {
			for (int y = g.pos.y; y < g.pos.y + g.area.y && enemies_to_spawn > 0; y++) {
				float xpos = static_cast<float>(x) + 0.5f;
				float ypos = static_cast<float>(y) + 0.5f;
				if (enemyGroupPlaceEnemy(xpos, ypos, g))
					enemies_to_spawn--;
			}
		}

	}
	if (enemies_to_spawn > 0) {
		Utils::logError("MapRenderer: Could not spawn all enemies in group at %s (x=%d,y=%d,w=%d,h=%d), %d missing (min=%d max=%d)",
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

int MapRenderer::load(const std::string& fname) {
	// unload sounds
	snd->reset();
	while (!sids.empty()) {
		snd->unload(sids.back());
		sids.pop_back();
	}

	// clear enemy spawn queue
	while (!powers->map_enemies.empty()) {
		powers->map_enemies.pop();
	}

	// clear combat text
	comb->clear();

	show_tooltip = false;
	is_spawn_map = (fname == "maps/spawn.txt");

	Map::load(fname);

	loadMusic();

	for (unsigned i = 0; i < layers.size(); ++i) {
		if (layernames[i] == "collision") {
			short width = static_cast<short>(layers[i].size());
			if (width == 0) {
				Utils::logError("MapRenderer: Map width is 0. Can't set collision layer.");
				break;
			}
			short height = static_cast<short>(layers[i][0].size());
			collider.setMap(layers[i], width, height);
			removeLayer(i);
		}
	}
	for (unsigned i = 0; i < layers.size(); ++i)
		if (layernames[i] == "object")
			index_objectlayer = i;
	if (fogofwar) {
		for (unsigned short i = 0; i < layers.size(); ++i) {
			if (layernames[i] == "fow_dark")
				fow->dark_layer_id = i;
			if (layernames[i] == "fow_fog")
				fow->fog_layer_id = i;
		}
	}

	while (!enemy_groups.empty()) {
		pushEnemyGroup(enemy_groups.front());
		enemy_groups.pop();
	}

	tset.load(this->tileset);

	std::vector<unsigned> corrupted;
	for (unsigned i = 0; i < layers.size(); ++i) {
		for (unsigned x = 0; x < layers[i].size(); ++x) {
			for (unsigned y = 0; y < layers[i][x].size(); ++y) {
				const unsigned tile_id = layers[i][x][y];
				TileSet* tile_set = &tset;

				if (fogofwar == FogOfWar::TYPE_OVERLAY) {
					if (i == fow->dark_layer_id) tile_set = &fow->tset_dark;
					if (i == fow->fog_layer_id) tile_set = &fow->tset_fog;
			    }
			    if (fogofwar)
					if (i == fow->dark_layer_id || i == fow->fog_layer_id)
						continue;

				if (tile_id > 0 && (tile_id >= tile_set->tiles.size() || tile_set->tiles[tile_id].tile == NULL)) {
					if (std::find(corrupted.begin(), corrupted.end(), tile_id) == corrupted.end()) {
						corrupted.push_back(tile_id);
					}
					layers[i][x][y] = 0;
				}
			}
		}
	}

	if (!corrupted.empty()) {
		Utils::logError("MapRenderer: Tileset or Map corrupted. A tile has a larger id than the tileset allows or is undefined.");
		while (!corrupted.empty()) {
			Utils::logError("MapRenderer: Removing offending tile id %d.", corrupted.back());
			corrupted.pop_back();
		}
	}

	setMapParallax(parallax_filename);

	render_device->setBackgroundColor(background_color);

	return 0;
}

void MapRenderer::loadMusic() {
	if (!settings->audio) return;

	if (settings->music_volume > 0) {
		// load and play music
		snd->loadMusic(music_filename);
	}
	else {
		snd->stopMusic();
	}
}

void MapRenderer::logic(bool paused) {
	if (fogofwar) {
		fow->logic();
	}

	// handle tile set logic e.g. animations
	tset.logic();
	if (fogofwar == FogOfWar::TYPE_OVERLAY) {
		fow->tset_dark.logic();
		fow->tset_fog.logic();
	}

	// TODO there's a bit too much "logic" here for a class that's supposed to be dedicated to rendering
	// some of these timers should be moved out at some point
	if (paused)
		return;

	// handle statblock logic for map powers
	for (unsigned i=0; i<statblocks.size(); ++i) {
		for (size_t j=0; j<statblocks[i].powers_ai.size(); ++j) {
			statblocks[i].powers_ai[j].cooldown.tick();
		}
	}

	// handle event cooldowns
	std::vector<Event>::iterator it;
	for (it = events.begin(); it < events.end(); ++it) {
		if (!it->delay.isEnd())
			it->delay.tick();
		else
			it->cooldown.tick();
	}

	// handle delayed events
	for (it = delayed_events.end(); it != delayed_events.begin(); ) {
		--it;

		it->delay.tick();

		if (it->delay.isEnd()) {
			EventManager::executeDelayedEvent(*it);
			it = delayed_events.erase(it);
		}
	}

	cam.logic();
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
		const unsigned tilex = static_cast<unsigned>(floorf(it->map_pos.x));
		const unsigned tiley = static_cast<unsigned>(floorf(it->map_pos.y));
		const int commax = static_cast<int>((it->map_pos.x - static_cast<float>(tilex)) * (1<<10));
		const int commay = static_cast<int>((it->map_pos.y - static_cast<float>(tiley)) * (1<<10));
		it->prio += (static_cast<uint64_t>(tilex + tiley) << 37) + (static_cast<uint64_t>(tilex) << 20) + (static_cast<uint64_t>(commax + commay) << 8);
	}
}

void calculatePriosOrtho(std::vector<Renderable> &r) {
	for (std::vector<Renderable>::iterator it = r.begin(); it != r.end(); ++it) {
		const unsigned tilex = static_cast<unsigned>(floorf(it->map_pos.x));
		const unsigned tiley = static_cast<unsigned>(floorf(it->map_pos.y));
		const int commay = static_cast<int>(it->map_pos.y * (1<<10));
		it->prio += (static_cast<uint64_t>(tiley) << 37) + (static_cast<uint64_t>(tilex) << 20) + (static_cast<uint64_t>(commay) << 8);
	}
}

void MapRenderer::render(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {

	map_parallax.render(cam.shake, "");

	if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL) {
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

	drawHiddenEntityMarkers();
}

void MapRenderer::drawRenderable(std::vector<Renderable>::iterator r_cursor) {
	if (r_cursor->image != NULL) {
		Rect dest;
		Point p = Utils::mapToScreen(r_cursor->map_pos.x, r_cursor->map_pos.y, cam.shake.x, cam.shake.y);
		dest.x = p.x - r_cursor->offset.x;
		dest.y = p.y - r_cursor->offset.y;
		render_device->render(*r_cursor, dest);
	}
}

void MapRenderer::renderIsoLayer(const Map_Layer& layerdata, const TileSet& tile_set) {
	int_fast16_t i; // first index of the map array
	int_fast16_t j; // second index of the map array
	Point dest;
	const Point upperleft(Utils::screenToMap(0, 0, cam.shake.x, cam.shake.y));
	const int_fast16_t max_tiles_width =   static_cast<int_fast16_t>((settings->view_w / eset->tileset.tile_w) + 2*tset.max_size_x);
	const int_fast16_t max_tiles_height = static_cast<int_fast16_t>((2 * settings->view_h / eset->tileset.tile_h) + 2*(tset.max_size_y+1));

	j = static_cast<int_fast16_t>(upperleft.y - tset.max_size_y/2 + tset.max_size_x);
	i = static_cast<int_fast16_t>(upperleft.x - tset.max_size_y/2 - tset.max_size_x);

	for (uint_fast16_t y = max_tiles_height ; y; --y) {
		int_fast16_t tiles_width = 0;

		// make sure the isometric corners are not rendered:
		// corner north west, upper left  (i < 0)
		if (i < -1) {
			j = static_cast<int_fast16_t>(j + i + 1);
			tiles_width = static_cast<int_fast16_t>(tiles_width - (i + 1));
			i = -1;
		}
		// corner north east, upper right (j > mapheight)
		const int_fast16_t d = static_cast<int_fast16_t>(j - h);
		if (d >= 0) {
			j = static_cast<int_fast16_t>(j - d);
			tiles_width = static_cast<int_fast16_t>(tiles_width + d);
			i = static_cast<int_fast16_t>(i + d);
		}

		// lower right (south east) corner is covered by (j+i-w+1)
		// lower left (south west) corner is caught by having 0 in there, so j>0
		const int_fast16_t j_end = std::max(static_cast<int_fast16_t>(j+i-w+1),	std::max(static_cast<int_fast16_t>(j - max_tiles_width), static_cast<int_fast16_t>(0)));

		Point p = Utils::mapToScreen(float(i), float(j), cam.shake.x, cam.shake.y);
		p = centerTile(p);

		// draw one horizontal line
		while (j > j_end) {
			--j;
			++i;
			++tiles_width;
			p.x += eset->tileset.tile_w;

			if (const uint_fast16_t current_tile = layerdata[i][j]) {
				const Tile_Def &tile = tile_set.tiles[current_tile];
				dest.x = p.x - tile.offset.x;
				dest.y = p.y - tile.offset.y;

				//skip rendering tiles that are underneath fow hidden tiles
				if (fogofwar == FogOfWar::TYPE_OVERLAY) {
					if (&layerdata != &layers[fow->dark_layer_id]) {
						if (layers[fow->dark_layer_id][i][j] == FogOfWar::TILE_HIDDEN) {

							//check tile's corners
							Point t_l(Utils::screenToMap(dest.x, dest.y, cam.shake.x, cam.shake.y));
							Point t_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y, cam.shake.x, cam.shake.y));
							Point b_l(Utils::screenToMap(dest.x, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));
							Point b_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));

							//limit to map bounds
							if (t_l.x < 0) t_l.x = 0;
							if (t_l.x >= w) t_l.x = w-1;
							if (t_l.y < 0) t_l.y = 0;
							if (t_l.y >= h) t_l.y = h-1;

							if (t_r.x < 0) t_r.x = 0;
							if (t_r.x >= w) t_r.x = w-1;
							if (t_r.y < 0) t_r.y = 0;
							if (t_r.y >= h) t_r.y = h-1;

							if (b_l.x < 0) b_l.x = 0;
							if (b_l.x >= w) b_l.x = w-1;
							if (b_l.y < 0) b_l.y = 0;
							if (b_l.y >= h) b_l.y = h-1;

							if (b_r.x < 0) b_r.x = 0;
							if (b_r.x >= w) b_r.x = w-1;
							if (b_r.y < 0) b_r.y = 0;
							if (b_r.y >= h) b_r.y = h-1;

							if (layers[fow->dark_layer_id][t_l.x][t_l.y] == FogOfWar::TILE_HIDDEN) {
								if (layers[fow->dark_layer_id][t_r.x][t_r.y] == FogOfWar::TILE_HIDDEN) {
									if (layers[fow->dark_layer_id][b_l.x][b_l.y] == FogOfWar::TILE_HIDDEN) {
										if (layers[fow->dark_layer_id][b_r.x][b_r.y] == FogOfWar::TILE_HIDDEN) {
											continue;
										}
									}
								}
							}
						}
					}
				}

				// no need to set w and h in dest, as it is ignored
				// by SDL_BlitSurface
				tile.tile->setDestFromPoint(dest);
				if (fogofwar == FogOfWar::TYPE_TINT) {
					tile.tile->color_mod = fow->getTileColorMod(i, j);
				}
				render_device->render(tile.tile);
			}
		}
		j = static_cast<int_fast16_t>(j + tiles_width);
		i = static_cast<int_fast16_t>(i - tiles_width);
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
	Point dest;

	const Point upperleft(Utils::screenToMap(0, 0, cam.shake.x, cam.shake.y));
	const int_fast16_t max_tiles_width = static_cast<int_fast16_t>((settings->view_w / eset->tileset.tile_w) + 2 * tset.max_size_x);
	const int_fast16_t max_tiles_height = static_cast<int_fast16_t>(((settings->view_h / eset->tileset.tile_h) + 2 * tset.max_size_y)*2);

	std::vector<Renderable>::iterator r_cursor = r.begin();
	std::vector<Renderable>::iterator r_end = r.end();

	// object layer
	int_fast16_t j = static_cast<int_fast16_t>(upperleft.y - tset.max_size_y + tset.max_size_x);
	int_fast16_t i = static_cast<int_fast16_t>(upperleft.x - tset.max_size_y - tset.max_size_x);

	while (r_cursor != r_end && (static_cast<int>(r_cursor->map_pos.x) + static_cast<int>(r_cursor->map_pos.y) < i + j || static_cast<int>(r_cursor->map_pos.x) < i)) // implicit floor
		++r_cursor;

	if (index_objectlayer >= layers.size())
		return;

	std::queue<std::vector<Renderable>::iterator> render_behind_SW;
	std::queue<std::vector<Renderable>::iterator> render_behind_NE;
	std::queue<std::vector<Renderable>::iterator> render_behind_none;

	Map_Layer drawn_tiles(w, std::vector<unsigned short>(h, 0));

	for (uint_fast16_t y = max_tiles_height ; y; --y) {
		int_fast16_t tiles_width = 0;

		// make sure the isometric corners are not rendered:
		if (i < -1) {
			j = static_cast<int_fast16_t>(j + i + 1);
			tiles_width = static_cast<int_fast16_t>(tiles_width - (i + 1));
			i = -1;
		}
		const int_fast16_t d = static_cast<int_fast16_t>(j - h);
		if (d >= 0) {
			j = static_cast<int_fast16_t>(j - d);
			tiles_width = static_cast<int_fast16_t>(tiles_width + d);
			i = static_cast<int_fast16_t>(i + d);
		}
		const int_fast16_t j_end = std::max(static_cast<int_fast16_t>(j+i-w+1), std::max(static_cast<int_fast16_t>(j - max_tiles_width), static_cast<int_fast16_t>(0)));

		// draw one horizontal line
		Point p = Utils::mapToScreen(float(i), float(j), cam.shake.x, cam.shake.y);
		p = centerTile(p);
		const Map_Layer &current_layer = layers[index_objectlayer];
		bool is_last_NE_tile = false;
		while (j > j_end) {
			--j;
			++i;
			++tiles_width;
			p.x += eset->tileset.tile_w;

			bool draw_tile = true;

			std::vector<Renderable>::iterator r_pre_cursor = r_cursor;
			while (r_pre_cursor != r_end) {
				int r_cursor_x = static_cast<int>(r_pre_cursor->map_pos.x);
				int r_cursor_y = static_cast<int>(r_pre_cursor->map_pos.y);

				if ((r_cursor_x-1 == i && r_cursor_y+1 == j) || (r_cursor_x+1 == i && r_cursor_y-1 == j)) {
					draw_tile = false;
					break;
				}
				else if (r_cursor_x+1 > i || r_cursor_y+1 > j) {
					break;
				}
				++r_pre_cursor;
			}

			if (draw_tile && !drawn_tiles[i][j]) {
				if (const uint_fast16_t current_tile = current_layer[i][j]) {
					const Tile_Def &tile = tset.tiles[current_tile];
					dest.x = p.x - tile.offset.x;
					dest.y = p.y - tile.offset.y;
					tile.tile->setDestFromPoint(dest);

					//skip rendering tiles that are underneath fow hidden tiles
					if (fogofwar == FogOfWar::TYPE_OVERLAY) {
						if (&current_layer != &layers[fow->dark_layer_id]) {
							if (layers[fow->dark_layer_id][i][j] == FogOfWar::TILE_HIDDEN) {

								//check tile's corners
								Point t_l(Utils::screenToMap(dest.x, dest.y, cam.shake.x, cam.shake.y));
								Point t_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y, cam.shake.x, cam.shake.y));
								Point b_l(Utils::screenToMap(dest.x, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));
								Point b_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));

								//limit to map bounds
								if (t_l.x < 0) t_l.x = 0;
								if (t_l.x >= w) t_l.x = w-1;
								if (t_l.y < 0) t_l.y = 0;
								if (t_l.y >= h) t_l.y = h-1;

								if (t_r.x < 0) t_r.x = 0;
								if (t_r.x >= w) t_r.x = w-1;
								if (t_r.y < 0) t_r.y = 0;
								if (t_r.y >= h) t_r.y = h-1;

								if (b_l.x < 0) b_l.x = 0;
								if (b_l.x >= w) b_l.x = w-1;
								if (b_l.y < 0) b_l.y = 0;
								if (b_l.y >= h) b_l.y = h-1;

								if (b_r.x < 0) b_r.x = 0;
								if (b_r.x >= w) b_r.x = w-1;
								if (b_r.y < 0) b_r.y = 0;
								if (b_r.y >= h) b_r.y = h-1;

								if (layers[fow->dark_layer_id][t_l.x][t_l.y] == FogOfWar::TILE_HIDDEN) {
									if (layers[fow->dark_layer_id][t_r.x][t_r.y] == FogOfWar::TILE_HIDDEN) {
										if (layers[fow->dark_layer_id][b_l.x][b_l.y] == FogOfWar::TILE_HIDDEN) {
											if (layers[fow->dark_layer_id][b_r.x][b_r.y] == FogOfWar::TILE_HIDDEN) {
												continue;
											}
										}
									}
								}
							}
						}
					}

					checkHiddenEntities(i, j, current_layer, r);
					if (fogofwar == FogOfWar::TYPE_TINT) {
						tile.tile->color_mod = fow->getTileColorMod(i, j);
					}
					render_device->render(tile.tile);
					drawn_tiles[i][j] = 1;
				}
			}

			if (r_cursor == r_end)
				continue;

do_last_NE_tile:
			// some renderable entities go in this layer

			// calculate south/south-west tile bounds
			Rect tile_SW_bounds, tile_S_bounds;
			Point tile_SW_center, tile_S_center;
			getTileBounds(static_cast<int_fast16_t>(i-2), static_cast<int_fast16_t>(j+2), current_layer, tile_SW_bounds, tile_SW_center);
			getTileBounds(static_cast<int_fast16_t>(i-1), static_cast<int_fast16_t>(j+2), current_layer, tile_S_bounds, tile_S_center);

			// calculate east/north-east tile bounds
			Rect tile_NE_bounds, tile_E_bounds;
			Point tile_NE_center, tile_E_center;
			getTileBounds(i, j, current_layer, tile_NE_bounds, tile_NE_center);
			getTileBounds(i, static_cast<int_fast16_t>(j+1), current_layer, tile_E_bounds, tile_E_center);

			bool draw_SW_tile = false;
			bool draw_NE_tile = false;

			while (r_cursor != r_end) {
				// implicit floor by int cast
				int r_cursor_x = static_cast<int>(r_cursor->map_pos.x);
				int r_cursor_y = static_cast<int>(r_cursor->map_pos.y);

				if (r_cursor_x+1 == i && r_cursor_y-1 == j) {
					draw_SW_tile = true;
					draw_NE_tile = !is_last_NE_tile;

					// r_cursor left/right side
					Point r_cursor_left = Utils::mapToScreen(r_cursor->map_pos.x, r_cursor->map_pos.y, cam.shake.x, cam.shake.y);
					r_cursor_left.y -= r_cursor->offset.y;
					Point r_cursor_right = r_cursor_left;
					r_cursor_left.x -= r_cursor->offset.x;
					r_cursor_right.x += r_cursor->src.w - r_cursor->offset.x;

					bool is_behind_SW = false;
					bool is_behind_NE = false;

					// check left of r_cursor
					if (Utils::isWithinRect(tile_S_bounds, r_cursor_right) && Utils::isWithinRect(tile_SW_bounds, r_cursor_left)) {
						is_behind_SW = true;
					}

					// check right of r_cursor
					if (draw_NE_tile && Utils::isWithinRect(tile_E_bounds, r_cursor_left) && Utils::isWithinRect(tile_NE_bounds, r_cursor_right)) {
						is_behind_NE = true;
					}

					if (is_behind_SW)
						render_behind_SW.push(r_cursor);
					else if (is_behind_NE)
						render_behind_NE.push(r_cursor);
					else
						render_behind_none.push(r_cursor);

					++r_cursor;
				}
				else {
					break;
				}
			}

			while (!render_behind_SW.empty()) {
				drawRenderable(render_behind_SW.front());
				render_behind_SW.pop();
			}

			// draw the south-west tile
			if (draw_SW_tile && i-2 >= 0 && j+2 < h && !drawn_tiles[i-2][j+2]) {
				if (const uint_fast16_t current_tile = current_layer[i-2][j+2]) {
					const Tile_Def &tile = tset.tiles[current_tile];
					dest.x = tile_SW_center.x - tile.offset.x;
					dest.y = tile_SW_center.y - tile.offset.y;
					tile.tile->setDestFromPoint(dest);
					checkHiddenEntities(i, j, current_layer, r);
					if (fogofwar == FogOfWar::TYPE_TINT) {
						tile.tile->color_mod = fow->getTileColorMod(i, j);
					}
					render_device->render(tile.tile);
					drawn_tiles[i-2][j+2] = 1;
				}
			}

			while (!render_behind_NE.empty()) {
				drawRenderable(render_behind_NE.front());
				render_behind_NE.pop();
			}

			// draw the north-east tile
			if (draw_NE_tile && !draw_tile && !drawn_tiles[i][j]) {
				if (const uint_fast16_t current_tile = current_layer[i][j]) {
					const Tile_Def &tile = tset.tiles[current_tile];
					dest.x = tile_NE_center.x - tile.offset.x;
					dest.y = tile_NE_center.y - tile.offset.y;
					tile.tile->setDestFromPoint(dest);
					checkHiddenEntities(i, j, current_layer, r);
					if (fogofwar == FogOfWar::TYPE_TINT) {
						tile.tile->color_mod = fow->getTileColorMod(i, j);
					}
					render_device->render(tile.tile);
					drawn_tiles[i][j] = 1;
				}
			}

			while (!render_behind_none.empty()) {
				drawRenderable(render_behind_none.front());
				render_behind_none.pop();
			}

			// Okay, this is a bit of a HACK
			// In order to properly render the first row and last column of the map, we need to advance to an imaginary tile
			// Care must be taken in the code after the "do_last_NE_tile" goto label to avoid accessing map data with these coordinates
			if (is_last_NE_tile) {
				++j;
				--i;
				is_last_NE_tile = false;
			}
			else if (i == w-1 || j == 0) {
				--j;
				++i;
				is_last_NE_tile = true;
				goto do_last_NE_tile;
			}
		}
		j = static_cast<int_fast16_t>(j + tiles_width);
		i = static_cast<int_fast16_t>(i - tiles_width);
		if (y % 2)
			i++;
		else
			j++;

		while (r_cursor != r_end && (static_cast<int>(r_cursor->map_pos.x) + static_cast<int>(r_cursor->map_pos.y) < i + j || static_cast<int>(r_cursor->map_pos.x) <= i)) // implicit floor by int cast
			++r_cursor;
	}
}

void MapRenderer::renderIso(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	size_t index = 0;

	while (index < index_objectlayer) {
		renderIsoLayer(layers[index], tset);
		map_parallax.render(cam.shake, layernames[index]);
		index++;
	}

	renderIsoBackObjects(r_dead);
	renderIsoFrontObjects(r);
	map_parallax.render(cam.shake, layernames[index]);

	index++;
	while (index < layers.size()) {
		if (fogofwar == FogOfWar::TYPE_OVERLAY) {
			if (layernames[index] == "fow_dark") {
				renderIsoLayer(layers[index],fow->tset_dark);
			}
			else if (layernames[index] == "fow_fog") {
				renderIsoLayer(layers[index],fow->tset_fog);
			}
			else {
				renderIsoLayer(layers[index], tset);
			}
		}
		else if (layernames[index] != "fow_dark" && layernames[index] != "fow_fog") {
			renderIsoLayer(layers[index], tset);
		}
		map_parallax.render(cam.shake, layernames[index]);
		index++;
	}

	checkTooltip();

	drawDevHUD();
	drawDevCursor();
}

void MapRenderer::renderOrthoLayer(const Map_Layer& layerdata, const TileSet& tile_set) {

	Point dest;
	const Point upperleft(Utils::screenToMap(0, 0, cam.shake.x, cam.shake.y));

	short int startj = static_cast<short int>(std::max(0, upperleft.y));
	short int starti = static_cast<short int>(std::max(0, upperleft.x));
	const short max_tiles_width =  std::min(w, static_cast<short unsigned int>(starti + (settings->view_w / eset->tileset.tile_w) + 2 * tset.max_size_x));
	const short max_tiles_height = std::min(h, static_cast<short unsigned int>(startj + (settings->view_h / eset->tileset.tile_h) + 2 * tset.max_size_y));

	short int i;
	short int j;

	for (j = startj; j < max_tiles_height; j++) {
		Point p = Utils::mapToScreen(starti, j, cam.shake.x, cam.shake.y);
		p = centerTile(p);
		for (i = starti; i < max_tiles_width; i++) {

			if (const unsigned short current_tile = layerdata[i][j]) {
				const Tile_Def &tile = tile_set.tiles[current_tile];
				dest.x = p.x - tile.offset.x;
				dest.y = p.y - tile.offset.y;

				bool skip_tile_render = false;

				//skip rendering tiles that are underneath fow hidden tiles
				if (fogofwar == FogOfWar::TYPE_OVERLAY) {
					if (&layerdata != &layers[fow->dark_layer_id]) {
						if (layers[fow->dark_layer_id][i][j] == FogOfWar::TILE_HIDDEN) {

							//check tile's corners
							Point t_l(Utils::screenToMap(dest.x, dest.y, cam.shake.x, cam.shake.y));
							Point t_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y, cam.shake.x, cam.shake.y));
							Point b_l(Utils::screenToMap(dest.x, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));
							Point b_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));

							//limit to map bounds
							if (t_l.x < 0) t_l.x = 0;
							if (t_l.x >= w) t_l.x = w-1;
							if (t_l.y < 0) t_l.y = 0;
							if (t_l.y >= h) t_l.y = h-1;

							if (t_r.x < 0) t_r.x = 0;
							if (t_r.x >= w) t_r.x = w-1;
							if (t_r.y < 0) t_r.y = 0;
							if (t_r.y >= h) t_r.y = h-1;

							if (b_l.x < 0) b_l.x = 0;
							if (b_l.x >= w) b_l.x = w-1;
							if (b_l.y < 0) b_l.y = 0;
							if (b_l.y >= h) b_l.y = h-1;

							if (b_r.x < 0) b_r.x = 0;
							if (b_r.x >= w) b_r.x = w-1;
							if (b_r.y < 0) b_r.y = 0;
							if (b_r.y >= h) b_r.y = h-1;

							if (layers[fow->dark_layer_id][t_l.x][t_l.y] == FogOfWar::TILE_HIDDEN) {
								if (layers[fow->dark_layer_id][t_r.x][t_r.y] == FogOfWar::TILE_HIDDEN) {
									if (layers[fow->dark_layer_id][b_l.x][b_l.y] == FogOfWar::TILE_HIDDEN) {
										if (layers[fow->dark_layer_id][b_r.x][b_r.y] == FogOfWar::TILE_HIDDEN) {
											skip_tile_render = true;
										}
									}
								}
							}
						}
					}
				}

				tile.tile->setDestFromPoint(dest);
				if (!skip_tile_render) {
					if (fogofwar == FogOfWar::TYPE_TINT) {
						tile.tile->color_mod = fow->getTileColorMod(i, j);
					}
					render_device->render(tile.tile);
				}
			}
			p.x += eset->tileset.tile_w;
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
	Point dest;
	std::vector<Renderable>::iterator r_cursor = r.begin();
	std::vector<Renderable>::iterator r_end = r.end();

	const Point upperleft(Utils::screenToMap(0, 0, cam.shake.x, cam.shake.y));

	short int startj = static_cast<short int>(std::max(0, upperleft.y));
	short int starti = static_cast<short int>(std::max(0, upperleft.x));
	const short max_tiles_width  = std::min(w, static_cast<short unsigned int>(starti + (settings->view_w / eset->tileset.tile_w) + 2 * tset.max_size_x));
	const short max_tiles_height = std::min(h, static_cast<short unsigned int>(startj + (settings->view_h / eset->tileset.tile_h) + 2 * tset.max_size_y));

	while (r_cursor != r_end && static_cast<int>(r_cursor->map_pos.y) < startj)
		++r_cursor;

	if (index_objectlayer >= layers.size())
		return;

	for (j = startj; j < max_tiles_height; j++) {
		Point p = Utils::mapToScreen(starti, j, cam.shake.x, cam.shake.y);
		p = centerTile(p);
		for (i = starti; i<max_tiles_width; i++) {

			if (const unsigned short current_tile = layers[index_objectlayer][i][j]) {
				const Tile_Def &tile = tset.tiles[current_tile];
				dest.x = p.x - tile.offset.x;
				dest.y = p.y - tile.offset.y;
				tile.tile->setDestFromPoint(dest);

				bool skip_tile_render = false;

				//skip rendering tiles that are underneath fow hidden tiles
				if (fogofwar == FogOfWar::TYPE_OVERLAY) {
					if (&layers[index_objectlayer] != &layers[fow->dark_layer_id]) {
						if (layers[fow->dark_layer_id][i][j] == FogOfWar::TILE_HIDDEN) {

							//check tile's corners
							Point t_l(Utils::screenToMap(dest.x, dest.y, cam.shake.x, cam.shake.y));
							Point t_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y, cam.shake.x, cam.shake.y));
							Point b_l(Utils::screenToMap(dest.x, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));
							Point b_r(Utils::screenToMap(dest.x + tile.tile->getClip().w, dest.y + tile.tile->getClip().h, cam.shake.x, cam.shake.y));

							//limit to map bounds
							if (t_l.x < 0) t_l.x = 0;
							if (t_l.x >= w) t_l.x = w-1;
							if (t_l.y < 0) t_l.y = 0;
							if (t_l.y >= h) t_l.y = h-1;

							if (t_r.x < 0) t_r.x = 0;
							if (t_r.x >= w) t_r.x = w-1;
							if (t_r.y < 0) t_r.y = 0;
							if (t_r.y >= h) t_r.y = h-1;

							if (b_l.x < 0) b_l.x = 0;
							if (b_l.x >= w) b_l.x = w-1;
							if (b_l.y < 0) b_l.y = 0;
							if (b_l.y >= h) b_l.y = h-1;

							if (b_r.x < 0) b_r.x = 0;
							if (b_r.x >= w) b_r.x = w-1;
							if (b_r.y < 0) b_r.y = 0;
							if (b_r.y >= h) b_r.y = h-1;

							if (layers[fow->dark_layer_id][t_l.x][t_l.y] == FogOfWar::TILE_HIDDEN) {
								if (layers[fow->dark_layer_id][t_r.x][t_r.y] == FogOfWar::TILE_HIDDEN) {
									if (layers[fow->dark_layer_id][b_l.x][b_l.y] == FogOfWar::TILE_HIDDEN) {
										if (layers[fow->dark_layer_id][b_r.x][b_r.y] == FogOfWar::TILE_HIDDEN) {
											skip_tile_render = true;
										}
									}
								}
							}
						}
					}
				}

				checkHiddenEntities(i, j, layers[index_objectlayer], r);
				if (!skip_tile_render) {
					if (fogofwar == FogOfWar::TYPE_TINT) {
						tile.tile->color_mod = fow->getTileColorMod(i, j);
					}
					render_device->render(tile.tile);
				}
			}
			p.x += eset->tileset.tile_w;

			while (r_cursor != r_end && static_cast<int>(r_cursor->map_pos.y) == j && static_cast<int>(r_cursor->map_pos.x) < i) // implicit floor
				++r_cursor;

			// some renderable entities go in this layer
			while (r_cursor != r_end && static_cast<int>(r_cursor->map_pos.y) == j && static_cast<int>(r_cursor->map_pos.x) == i) // implicit floor
				drawRenderable(r_cursor++);
		}
		while (r_cursor != r_end && static_cast<int>(r_cursor->map_pos.y) <= j) // implicit floor
			++r_cursor;
	}
}

void MapRenderer::renderOrtho(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	unsigned index = 0;
	while (index < index_objectlayer) {
		renderOrthoLayer(layers[index], tset);
		map_parallax.render(cam.shake, layernames[index]);
		index++;
	}

	renderOrthoBackObjects(r_dead);
	renderOrthoFrontObjects(r);
	map_parallax.render(cam.shake, layernames[index]);

	index++;
	while (index < layers.size()) {
		if (fogofwar == FogOfWar::TYPE_OVERLAY) {
			if (layernames[index] == "fow_dark") {
				renderOrthoLayer(layers[index],fow->tset_dark);
			}
			else if (layernames[index] == "fow_fog") {
				renderOrthoLayer(layers[index],fow->tset_fog);
			}
			else {
				renderOrthoLayer(layers[index], tset);
			}
		}
		else if (layernames[index] != "fow_dark" && layernames[index] != "fow_fog") {
			renderOrthoLayer(layers[index], tset);
		}
		map_parallax.render(cam.shake, layernames[index]);
		index++;
	}

	checkTooltip();

	drawDevHUD();
	drawDevCursor();
}

void MapRenderer::executeOnLoadEvents() {
	// if set from the command-line, execute a given script if this is our first map load
	if (!settings->load_script.empty() && filename != "maps/spawn.txt") {
		Event evnt;
		EventComponent ec;

		ec.type = EventComponent::SCRIPT;
		ec.s = settings->load_script;
		settings->load_script.clear();

		evnt.components.push_back(ec);
		EventManager::executeEvent(evnt);

		return;
	}

	std::vector<Event>::iterator it;

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		if ((*it).activate_type == Event::ACTIVATE_ON_LOAD) {
			if (EventManager::executeEvent(*it))
				it = events.erase(it);
		}
	}

	// Also check static events, as they should execute alongside on_load events
	// Yet, this should be done *after* the on_load events to not break old behavior.
	// That's why we don't just check static events in the above loop
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		if ((*it).activate_type == Event::ACTIVATE_STATIC) {
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

		if ((*it).activate_type == Event::ACTIVATE_ON_MAPEXIT)
			EventManager::executeEvent(*it); // ignore repeat value
	}
}

void MapRenderer::checkEvents(const FPoint& loc) {
	Point maploc;
	maploc.x = int(loc.x);
	maploc.y = int(loc.y);
	std::vector<Event>::iterator it;

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		// static events are run every frame without interaction from the player
		if ((*it).activate_type == Event::ACTIVATE_STATIC) {
			if (EventManager::executeEvent(*it))
				it = events.erase(it);
			continue;
		}

		if ((*it).activate_type == Event::ACTIVATE_ON_CLEAR) {
			if (enemies_cleared && EventManager::executeEvent(*it))
				it = events.erase(it);
			continue;
		}

		bool inside = maploc.x >= (*it).location.x &&
					  maploc.y >= (*it).location.y &&
					  maploc.x <= (*it).location.x + (*it).location.w-1 &&
					  maploc.y <= (*it).location.y + (*it).location.h-1;

		if ((*it).activate_type == Event::ACTIVATE_ON_LEAVE) {
			if (inside) {
				if (!(*it).getComponent(EventComponent::WAS_INSIDE_EVENT_AREA)) {
					(*it).components.push_back(EventComponent());
					(*it).components.back().type = EventComponent::WAS_INSIDE_EVENT_AREA;
				}
			}
			else {
				if ((*it).getComponent(EventComponent::WAS_INSIDE_EVENT_AREA)) {
					(*it).deleteAllComponents(EventComponent::WAS_INSIDE_EVENT_AREA);
					if (EventManager::executeEvent(*it))
						it = events.erase(it);
				}
			}
		}
		else if ((*it).activate_type == Event::ACTIVATE_ON_TRIGGER) {
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
 * The hero must be within range (eset->misc.interact_range) to activate an event.
 *
 * This function checks valid mouse clicks against all clickable events, and
 * executes
 */
void MapRenderer::checkHotspots() {
	if (!inpt->usingMouse()) return;

	show_tooltip = false;

	std::vector<Event>::iterator it;

	// work backwards through events because events can be erased in the loop.
	// this prevents the iterator from becoming invalid.
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		// skip events without hotspots
		if (it->hotspot.h == 0) continue;

		// skip events on cooldown
		if (!it->cooldown.isEnd() || !it->delay.isEnd()) continue;

		EventComponent* npc = (*it).getComponent(EventComponent::NPC_HOTSPOT);

		for (int x=it->hotspot.x; x < it->hotspot.x + it->hotspot.w; ++x) {
			for (int y=it->hotspot.y; y < it->hotspot.y + it->hotspot.h; ++y) {
				bool matched = false;
				bool is_npc = false;

				if (npc) {
					is_npc = true;

					Point p = Utils::mapToScreen(float(npc->data[0].Int), float(npc->data[1].Int), cam.shake.x, cam.shake.y);
					p = centerTile(p);

					Rect dest;
					if (npc->id < npcs->npcs.size()) {
						dest = npcs->npcs[npc->id]->getRenderBounds(mapr->cam.pos);
					}

					if (Utils::isWithinRect(dest, inpt->mouse)) {
						matched = true;
						tip_pos.x = dest.x + dest.w/2;
						tip_pos.y = p.y - eset->tooltips.margin_npc;
					}
				}
				else {
					for (unsigned index = 0; index <= index_objectlayer; ++index) {
						Point p = Utils::mapToScreen(float(x), float(y), cam.shake.x, cam.shake.y);
						p = centerTile(p);

						if (const short current_tile = layers[index][x][y]) {
							// first check if mouse pointer is in rectangle of that tile:
							const Tile_Def &tile = tset.tiles[current_tile];
							Rect dest;
							dest.x = p.x - tile.offset.x;
							dest.y = p.y - tile.offset.y;
							dest.w = tile.tile->getClip().w;
							dest.h = tile.tile->getClip().h;

							if (Utils::isWithinRect(dest, inpt->mouse)) {
								matched = true;
								tip_pos = Utils::mapToScreen(it->center.x, it->center.y, cam.shake.x, cam.shake.y);
								tip_pos.y -= eset->tileset.tile_h;
							}
						}
					}
				}

				if (matched) {
					// new tooltip?
					createTooltip(it->getComponent(EventComponent::TOOLTIP));

					if (((it->reachable_from.w == 0 && it->reachable_from.h == 0) || Utils::isWithinRect(it->reachable_from, Point(cam.pos)))
							&& Utils::calcDist(pc->stats.pos, it->center) < eset->misc.interact_range) {

						// only check events if the player is clicking
						// and allowed to click
						if (is_npc) {
							curs->setCursor(CursorManager::CURSOR_TALK);
						}
						else {
							curs->setCursor(CursorManager::CURSOR_INTERACT);
						}
						if (!inpt->pressing[Input::MAIN1]) return;
						else if (inpt->lock[Input::MAIN1]) return;
						else if (pc->using_main1) return;

						inpt->lock[Input::MAIN1] = true;
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
	if (!inpt->usingMouse()) show_tooltip = false;

	std::vector<Event>::iterator it;
	std::vector<Event>::iterator nearest = events.end();
	float best_distance = std::numeric_limits<float>::max();

	// loop in reverse because we may erase elements
	for (it = events.end(); it != events.begin(); ) {
		--it;

		// skip inactive events
		if (!EventManager::isActive(*it)) continue;

		// skip events without hotspots
		if (it->hotspot.h == 0) continue;

		// skip events on cooldown
		if (!it->cooldown.isEnd() || !it->delay.isEnd()) continue;

		float distance = Utils::calcDist(pc->stats.pos, it->center);
		if (((it->reachable_from.w == 0 && it->reachable_from.h == 0) || Utils::isWithinRect(it->reachable_from, Point(cam.pos)))
				&& distance < eset->misc.interact_range && distance < best_distance) {
			best_distance = distance;
			nearest = it;
		}

	}

	if (nearest != events.end()) {
		if (!inpt->usingMouse() || settings->touchscreen) {
			// new tooltip?
			createTooltip(nearest->getComponent(EventComponent::TOOLTIP));
			tip_pos = Utils::mapToScreen(nearest->center.x, nearest->center.y, cam.shake.x, cam.shake.y);
			if (nearest->getComponent(EventComponent::NPC_HOTSPOT)) {
				tip_pos.y -= eset->tooltips.margin_npc;
			}
			else {
				tip_pos.y -= eset->tileset.tile_h;
			}
		}

		if (inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
			inpt->lock[Input::ACCEPT] = true;

			if(EventManager::executeEvent(*nearest))
				events.erase(nearest);
		}
	}
}

void MapRenderer::checkTooltip() {
	if (show_tooltip && settings->show_hud && !(settings->dev_mode && menu->devconsole->visible))
		tip->render(tip_buf, tip_pos, TooltipData::STYLE_TOPLABEL);
}

void MapRenderer::createTooltip(EventComponent *ec) {
	if (ec && !ec->s.empty() && tooltipm->context != TooltipManager::CONTEXT_MENU) {
		show_tooltip = true;
		if (!tip_buf.compareFirstLine(ec->s)) {
			tip_buf.clear();
			tip_buf.addText(ec->s);
		}
		tooltipm->context = TooltipManager::CONTEXT_MAP;
	}
	else if (tooltipm->context != TooltipManager::CONTEXT_MENU) {
		tooltipm->context = TooltipManager::CONTEXT_NONE;
	}
}

/**
 * Activate a power that is attached to an event
 */
void MapRenderer::activatePower(PowerID power_index, unsigned statblock_index, const FPoint &target) {
	if (powers->powers[power_index].is_empty) {
		Utils::logError("MapRenderer: Power index %d is not valid.", power_index);
		return;
	}

	if (statblock_index < statblocks.size()) {
		// check power cooldown before activating
		if (statblocks[statblock_index].powers_ai[0].cooldown.isEnd()) {
			statblocks[statblock_index].powers_ai[0].cooldown.setDuration(powers->powers[power_index].cooldown);
			powers->activate(power_index, &statblocks[statblock_index], target);
		}
	}
	else {
		Utils::logError("MapRenderer: StatBlock index is out of bounds.");
	}
}

bool MapRenderer::isValidTile(const unsigned &tile) {
	if (tile == 0)
		return true;

	if (tile >= tset.tiles.size())
		return false;

	return tset.tiles[tile].tile != NULL;
}

Point MapRenderer::centerTile(const Point& p) {
	Point r = p;

	if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL) {
		r.x += eset->tileset.tile_w_half;
		r.y += eset->tileset.tile_h_half;
	}
	else //eset->tileset.TILESET_ISOMETRIC
		r.y += eset->tileset.tile_h_half;
	return r;
}

void MapRenderer::getTileBounds(const int_fast16_t x, const int_fast16_t y, const Map_Layer& layerdata, Rect& bounds, Point& center) {
	if (x >= 0 && x < w && y >= 0 && y < h) {
		if (const uint_fast16_t tile_index = layerdata[x][y]) {
			const Tile_Def &tile = tset.tiles[tile_index];
			if (!tile.tile)
				return;
			center = centerTile(Utils::mapToScreen(float(x), float(y), cam.shake.x, cam.shake.y));
			bounds.x = center.x - tile.offset.x;
			bounds.y = center.y - tile.offset.y;
			bounds.w = tile.tile->getClip().w;
			bounds.h = tile.tile->getClip().h;
		}
	}
}

void MapRenderer::drawDevCursor() {
	// Developer mode only: draw colored cursor around tile under mouse pointer
	if (!(settings->dev_mode && menu->devconsole->visible))
		return;

	Color dev_cursor_color = Color(255,255,0,255);
	FPoint target = Utils::screenToMap(inpt->mouse.x,  inpt->mouse.y, cam.shake.x, cam.shake.y);

	if (!collider.isOutsideMap(floorf(target.x), floorf(target.y))) {
		if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL) {
			Point p_topleft = Utils::mapToScreen(floorf(target.x), floorf(target.y), cam.shake.x, cam.shake.y);
			Point p_bottomright(p_topleft.x + eset->tileset.tile_w, p_topleft.y + eset->tileset.tile_h);

			render_device->drawRectangle(p_topleft, p_bottomright, dev_cursor_color);
		}
		else {
			Point p_left = Utils::mapToScreen(floorf(target.x), floorf(target.y+1), cam.shake.x, cam.shake.y);
			Point p_top(p_left.x + eset->tileset.tile_w_half, p_left.y - eset->tileset.tile_h_half);
			Point p_right(p_left.x + eset->tileset.tile_w, p_left.y);
			Point p_bottom(p_left.x + eset->tileset.tile_w_half, p_left.y + eset->tileset.tile_h_half);

			render_device->drawLine(p_left.x, p_left.y, p_top.x, p_top.y, dev_cursor_color);
			render_device->drawLine(p_top.x, p_top.y, p_right.x, p_right.y, dev_cursor_color);
			render_device->drawLine(p_right.x, p_right.y, p_bottom.x, p_bottom.y, dev_cursor_color);
			render_device->drawLine(p_bottom.x, p_bottom.y, p_left.x, p_left.y, dev_cursor_color);
		}

		// draw distance line
		if (menu->devconsole->distance_timer.isEnd()) {
			Point p0 = Utils::mapToScreen(menu->devconsole->target.x, menu->devconsole->target.y, cam.shake.x, cam.shake.y);
			Point p1 = Utils::mapToScreen(pc->stats.pos.x, pc->stats.pos.y, cam.shake.x, cam.shake.y);
			render_device->drawLine(p0.x, p0.y, p1.x, p1.y, dev_cursor_color);
		}
	}
}

void MapRenderer::drawDevHUD() {
	if (!(settings->dev_mode && settings->dev_hud))
		return;

	Color color_hazard(255,0,0,255);
	Color color_entity(0,255,0,255);
	Color color_cam(255,255,0,255);
	int cross_size = eset->tileset.tile_h_half / 4;

	// ellipses are distorted for isometric tilesets
	int distort = eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL ? 1 : 2;

	// camera
	{
		Point p0 = Utils::mapToScreen(cam.pos.x, cam.pos.y, cam.shake.x, cam.shake.y);
		render_device->drawLine(p0.x - cross_size, p0.y, p0.x + cross_size, p0.y, color_cam);
		render_device->drawLine(p0.x, p0.y - cross_size, p0.x, p0.y + cross_size, color_cam);
	}

	// player
	{
		Point p0 = Utils::mapToScreen(pc->stats.pos.x, pc->stats.pos.y, cam.shake.x, cam.shake.y);
		render_device->drawLine(p0.x - cross_size, p0.y, p0.x + cross_size, p0.y, color_entity);
		render_device->drawLine(p0.x, p0.y - cross_size, p0.x, p0.y + cross_size, color_entity);
	}

	// enemies
	for (size_t i = 0; i < entitym->entities.size(); ++i) {
		Point p0 = Utils::mapToScreen(entitym->entities[i]->stats.pos.x, entitym->entities[i]->stats.pos.y, cam.shake.x, cam.shake.y);
		render_device->drawLine(p0.x - cross_size, p0.y, p0.x + cross_size, p0.y, color_entity);
		render_device->drawLine(p0.x, p0.y - cross_size, p0.x, p0.y + cross_size, color_entity);
	}

	// hazards
	for (size_t i = 0; i < hazards->h.size(); ++i) {
		if (hazards->h[i]->delay_frames != 0)
			continue;

		Point p0 = Utils::mapToScreen(hazards->h[i]->pos.x, hazards->h[i]->pos.y, cam.shake.x, cam.shake.y);
		Point p1 = Utils::mapToScreen(hazards->h[i]->pos.x + hazards->h[i]->power->radius, hazards->h[i]->pos.y, cam.shake.x, cam.shake.y);
		int radius = p1.x - p0.x;
		render_device->drawLine(p0.x - cross_size, p0.y, p0.x + cross_size, p0.y, color_hazard);
		render_device->drawLine(p0.x, p0.y - cross_size, p0.x, p0.y + cross_size, color_hazard);

		render_device->drawEllipse(p0.x - radius, p0.y - radius/distort, p0.x + radius, p0.y + radius/distort, color_hazard, 15);
	}
}

void MapRenderer::drawHiddenEntityMarkers() {
	if (!settings->entity_markers)
		return;

	std::vector<std::vector<Renderable>::iterator>::iterator hero_it = hidden_entities.end();
	Point hidden_hero_pos(0, settings->view_h);

	const int marker_w = entity_hidden_normal->getGraphicsWidth();
	const int marker_h = entity_hidden_normal->getGraphicsHeight();

	for (size_t i = 0; i < hidden_entities.size(); ++i) {
		if (!hidden_entities[i]->image)
			continue;

		if (hidden_entities[i]->type == Renderable::TYPE_NORMAL)
			continue;

		Point dest;
		Point p = Utils::mapToScreen(hidden_entities[i]->map_pos.x, hidden_entities[i]->map_pos.y, cam.shake.x, cam.shake.y);
		dest.x = p.x - marker_w / 2;
		dest.y = p.y - hidden_entities[i]->offset.y - marker_h;

		// only take the highest point of the hero sprite
		if (hidden_entities[i]->type == Renderable::TYPE_HERO) {
			if (dest.y <= hidden_hero_pos.y) {
				hero_it = hidden_entities.begin() + i;
				hidden_hero_pos = dest;
			}
			continue;
		}

		if (hidden_entities[i]->type == Renderable::TYPE_ENEMY) {
			entity_hidden_enemy->setDestFromPoint(dest);
			entity_hidden_enemy->alpha_mod = hidden_entities[i]->alpha_mod;
			render_device->render(entity_hidden_enemy);
		}
		else if (hidden_entities[i]->type == Renderable::TYPE_ALLY) {
			entity_hidden_normal->setDestFromPoint(dest);
			entity_hidden_normal->alpha_mod = hidden_entities[i]->alpha_mod;
			render_device->render(entity_hidden_normal);
		}
	}

	if (hero_it != hidden_entities.end()) {
		entity_hidden_normal->setDestFromPoint(hidden_hero_pos);
		render_device->render(entity_hidden_normal);
	}

	hidden_entities.clear();
}

void MapRenderer::checkHiddenEntities(const int_fast16_t x, const int_fast16_t y, const Map_Layer& layerdata, std::vector<Renderable> &r) {
	if (!settings->entity_markers)
		return;

	Rect tile_bounds;
	Point tile_center;
	getTileBounds(x, y, layerdata, tile_bounds, tile_center);

	const int tall_threshold = eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC ? eset->tileset.tile_h * 2 : eset->tileset.tile_h;
	if (tile_bounds.h <= tall_threshold) {
		return;
	}

	bool hero_is_hidden = false;

	std::vector<Renderable>::iterator it = r.begin();
	while (it != r.end()) {
		if (it->type == Renderable::TYPE_NORMAL) {
			++it;
			continue;
		}

		const int it_x = static_cast<int>(it->map_pos.x);
		const int it_y = static_cast<int>(it->map_pos.y);
		if ((eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC && (x < it_x || y < it_y)) ||
		    (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL && y < it_y))
		{
			++it;
			continue;
		}

		bool is_hidden = false;

		if (it->type == Renderable::TYPE_HERO && hero_is_hidden) {
			is_hidden = true;
		}
		else if (it->type != Renderable::TYPE_NORMAL) {
			Point p = Utils::mapToScreen(it->map_pos.x, it->map_pos.y, cam.shake.x, cam.shake.y);
			p.x -= it->offset.x;
			if (Utils::isWithinRect(tile_bounds, p)) {
				is_hidden = true;
			}
			else {
				p.x += it->offset.x * 2;
				if (Utils::isWithinRect(tile_bounds, p)) {
					is_hidden = true;
				}
			}
		}

		if (is_hidden) {
			if (it->type == Renderable::TYPE_HERO)
				hero_is_hidden = true;

			bool already_hidden = false;
			for (size_t i = 0; i < hidden_entities.size(); ++i) {
				if (it == hidden_entities[i]) {
					already_hidden = true;
					break;
				}
			}

			if (!already_hidden) {
				hidden_entities.push_back(it);
			}
		}

		++it;
	}
}

void MapRenderer::setMapParallax(const std::string& mp_filename) {
	map_parallax.load(mp_filename);
	map_parallax.setMapCenter(w/2, h/2);
}

MapRenderer::~MapRenderer() {
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

	delete entity_hidden_normal;
	delete entity_hidden_enemy;
}

