/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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

/**
 * class FogOfWar
 *
 * Contains logic and rendering routines for fog of war.
 */

#include "Avatar.h"
#include "FogOfWar.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuMiniMap.h"
#include "SharedGameResources.h"

FogOfWar::FogOfWar()
	: layer_id(0)
	, color_sight(255,255,255)
	, color_visited(128,128,128)
	, color_hidden(0,0,0)
	, tileset("tilesetdefs/tileset_fogofwar.txt")
	, update_minimap(true) {
}

int FogOfWar::load() {
	tset.load(tileset);

	return 0;
}
void FogOfWar::logic() {
	calcBoundaries();
	updateTiles(0);
	if (update_minimap) {
		menu->mini->prerender(&mapr->collider, mapr->w, mapr->h);
		update_minimap = false;
	}
}

void FogOfWar::handleIntramapTeleport() {
	calcBoundaries();
	updateTiles(2);
	if (update_minimap) {
		menu->mini->prerender(&mapr->collider, mapr->w, mapr->h);
		update_minimap = false;
	}
}

Color FogOfWar::getTileColorMod(const int_fast16_t x, const int_fast16_t y) {
	if (mapr->layers[layer_id][x][y] == 2)
		return color_visited;
	else if (mapr->layers[layer_id][x][y] == 1)
		return color_hidden;
	else
		return color_sight;
}

void FogOfWar::calcBoundaries() {
	start_x = static_cast<short>(pc->stats.pos.x-pc->sight-2);
	start_y = static_cast<short>(pc->stats.pos.y-pc->sight-2);
	end_x = static_cast<short>(pc->stats.pos.x+pc->sight+2);
	end_y = static_cast<short>(pc->stats.pos.y+pc->sight+2);

	if (start_x < 0) start_x = 0;
	if (start_y < 0) start_y = 0;
	if (end_x > mapr->w) end_x = mapr->w;
	if (end_y > mapr->h) end_y = mapr->h;
}

void FogOfWar::updateTiles(unsigned short sight_tile) {
	for (unsigned short lx = start_x; lx < end_x; lx++) {
		for (unsigned short ly = start_y; ly < end_y; ly++) {
			Point lPoint(lx, ly);
			float delta = Utils::calcDist(FPoint(lPoint), FPoint(pc->stats.pos));
			unsigned short prev_tile = mapr->layers[layer_id][lx][ly];
			if (delta < pc->sight) {
				mapr->layers[layer_id][lx][ly] = sight_tile;
			}
			else if (mapr->layers[layer_id][lx][ly] == 0) {
				mapr->layers[layer_id][lx][ly] = 2;
			}
			if (prev_tile == 1 && prev_tile != mapr->layers[layer_id][lx][ly]) {
				update_minimap = true;
			}
		}
	}
}

FogOfWar::~FogOfWar() {
}
