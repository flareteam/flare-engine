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
#include "EngineSettings.h"
#include "FogOfWar.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuMiniMap.h"
#include "SharedGameResources.h"
#include "SharedResources.h"

const unsigned short FogOfWar::CIRCLE_MASK[NUM_FOW_RADII][FOW_MAX_RADIUS_LENGTH * FOW_MAX_RADIUS_LENGTH] = {
	// radius 3
	{
		fow_all,fow_all,fow_CNW,fow_NNN,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_SSS,fow_CSE,fow_all,fow_all,
	},
	// radius 4
	{
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
	},
	// radius 5
	{
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
	},
	// radius 6
	{
		fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,
	},
	// radius 7
	{
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,
	},
	// radius 8
	{
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
	},
	// radius 9
	{
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
	}
};

FogOfWar::FogOfWar()
	: layer_id(0)
	, tileset("tilesetdefs/tileset_fogofwar.txt")
	, bounds(0,0,0,0)
	, color_sight(255,255,255)
	, color_visited(128,128,128)
	, color_hidden(0,0,0)
	, update_minimap(true) {
}

int FogOfWar::load() {
	tset.load(tileset);
	for (int x = 0; x < mapr->w; x++) {
		for (int y = 0; y < mapr->h; y++) {
			std::cout << mapr->layers[layer_id][x][y] << ", ";
		}
		std::cout << std::endl;
	}
	return 0;
}

void FogOfWar::logic() {
	calcBoundaries();
	updateTiles(TILE_SIGHT);
	if (update_minimap) {
		menu->mini->update(&mapr->collider, &bounds);
		update_minimap = false;
	}
}

void FogOfWar::handleIntramapTeleport() {
	calcBoundaries();
	updateTiles(TILE_VISITED);
	if (update_minimap) {
		menu->mini->update(&mapr->collider, &bounds);
		update_minimap = false;
	}
}

Color FogOfWar::getTileColorMod(const int_fast16_t x, const int_fast16_t y) {
	if (mapr->layers[layer_id][x][y] == TILE_VISITED)
		return color_visited;
	else if (mapr->layers[layer_id][x][y] == TILE_HIDDEN)
		return color_hidden;
	else
		return color_sight;
}

void FogOfWar::calcBoundaries() {
	bounds.x = static_cast<short>(pc->stats.pos.x-pc->sight);
	bounds.y = static_cast<short>(pc->stats.pos.y-pc->sight);
	bounds.w = static_cast<short>(pc->stats.pos.x+pc->sight);
	bounds.h = static_cast<short>(pc->stats.pos.y+pc->sight);

	//if (bounds.x < 0) bounds.x = 0;
	//if (bounds.y < 0) bounds.y = 0;
	//if (bounds.w > mapr->w) bounds.w = mapr->w;
	//if (bounds.h > mapr->h) bounds.h = mapr->h;
}
/*
void FogOfWar::updateTiles(unsigned short sight_tile) {
	for (int x = bounds.x; x < bounds.w; x++) {
		for (int y = bounds.y; y < bounds.h; y++) {
			Point tile(x, y);
			float delta = Utils::calcDist(FPoint(tile), FPoint(pc->stats.pos));
			unsigned short prev_tile = mapr->layers[layer_id][x][y];
			if (delta < pc->sight) {
				mapr->layers[layer_id][x][y] = sight_tile;
			}
			else if (mapr->layers[layer_id][x][y] == TILE_SIGHT) {
				mapr->layers[layer_id][x][y] = TILE_VISITED;
			}
			if ((prev_tile == TILE_HIDDEN) && prev_tile != mapr->layers[layer_id][x][y]) {
				update_minimap = true; 
			}
		}
	}
}
*/
void FogOfWar::updateTiles(unsigned short sight_tile) {
	applyMask();
}

void FogOfWar::applyMask() {
	int radius = 9;
	const unsigned short * mask = &CIRCLE_MASK[radius - FOW_RADIUS_MIN][0];
	  
  	for (int x = bounds.x; x < bounds.w+1; x++) {
		for (int y = bounds.y; y < bounds.h+1; y++) {
			if (x>0 && y>0 && x < mapr->w && y < mapr->h) {
				mapr->layers[layer_id][x][y] &= *mask;
			}
			mask++;
		}
	}
}


FogOfWar::~FogOfWar() {
}
