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
#include "FileParser.h"
#include "FogOfWar.h"
#include "FontEngine.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuMiniMap.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

FogOfWar::FogOfWar()
	: dark_layer_id(0)
	, fog_layer_id(0)
	, tileset_dark("tilesetdefs/tileset_fow_dark.txt")
	, tileset_fog("tilesetdefs/tileset_fow_fog.txt")
	, mask_definition("engine/fow_mask.txt")
	, bounds(0,0,0,0)
	, color_sight(255,255,255)
	, color_visited(128,128,128)
	, color_hidden(0,0,0)
	, update_minimap(true) {

	for (int i=0; i<512; i++) {
		std::stringstream ss;
		ss << i;

		Image* tile_num = render_device->createImage(64,32);
		font->render(ss.str(), 16, 5, FontEngine::JUSTIFY_LEFT, tile_num, 0, Color(255,0,0,255));
		Sprite* tile_spr = tile_num->createSprite();

		tile_numbers.resize(tile_numbers.size()+1, tile_spr);
	}
}

int FogOfWar::load() {
	FileParser infile;

	if (!infile.open(mask_definition, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return 0;

	Utils::logInfo("FOW: Loading mask '%s'", mask_definition.c_str());

	while (infile.next()) {
		if (infile.section == "header")
			loadHeader(infile);
		else if (infile.section == "bits")
			loadDefBit(infile);
		else if (infile.section == "tiles")
			loadDefTile(infile);
		else if (infile.section == "mask")
			loadDefMask(infile);
	}

	infile.close();

	tset_dark.load(tileset_dark);
	tset_fog.load(tileset_fog);

	return 0;
}

void FogOfWar::logic() {
	updateTiles();
	if (update_minimap) {
		calcMiniBoundaries();
		menu->mini->update(&mapr->collider, &bounds);
		update_minimap = false;
	}
}

void FogOfWar::handleIntramapTeleport() {
	calcBoundaries();
	
	for (int x = bounds.x; x <= bounds.w; x++) {
		for (int y = bounds.y; y <= bounds.h; y++) {
			if (x>=0 && y>=0 && x < mapr->w && y < mapr->h) {
				mapr->layers[fog_layer_id][x][y] = TILE_HIDDEN;
			}
		}
	}
}

Color FogOfWar::getTileColorMod(const int_fast16_t x, const int_fast16_t y) {
	if (mapr->layers[dark_layer_id][x][y] == 0 && mapr->layers[fog_layer_id][x][y] > 0)
		return color_visited;
	else if (mapr->layers[dark_layer_id][x][y] > 0)
		return color_hidden;
	else 
		return color_sight;
}

void FogOfWar::calcBoundaries() {
	bounds.x = static_cast<short>(pc->stats.pos.x)-mask_radius;
	bounds.y = static_cast<short>(pc->stats.pos.y)-mask_radius;
	bounds.w = static_cast<short>(pc->stats.pos.x)+mask_radius;
	bounds.h = static_cast<short>(pc->stats.pos.y)+mask_radius;
}

void FogOfWar::calcMiniBoundaries() {
	bounds.x = static_cast<short>(pc->stats.pos.x)-mask_radius;
	bounds.y = static_cast<short>(pc->stats.pos.y)-mask_radius;
	bounds.w = static_cast<short>(pc->stats.pos.x)+mask_radius;
	bounds.h = static_cast<short>(pc->stats.pos.y)+mask_radius;

	if (bounds.x < 0) bounds.x = 0;
	if (bounds.y < 0) bounds.y = 0;
	if (bounds.w > mapr->w) bounds.w = mapr->w;
	if (bounds.h > mapr->h) bounds.h = mapr->h;
}

void FogOfWar::updateTiles() {
	calcBoundaries();
	const unsigned short * mask = &def_mask[0];

	for (int x = bounds.x; x <= bounds.w; x++) {
		for (int y = bounds.y; y <= bounds.h; y++) {
			if (x>=0 && y>=0 && x < mapr->w && y < mapr->h) {
				unsigned short prev_tile = mapr->layers[dark_layer_id][x][y];
				mapr->layers[dark_layer_id][x][y] &= *mask;
				mapr->layers[fog_layer_id][x][y] = *mask;
				if ((prev_tile == TILE_HIDDEN) && prev_tile != mapr->layers[dark_layer_id][x][y]) {
					update_minimap = true;
				}
			}
			mask++;
		}
	}
}

void FogOfWar::loadHeader(FileParser &infile) {
	if (infile.key == "radius") {
		// @ATTR masK_radius|int|Fog of war mask radius
		this->mask_radius = Parse::toInt(infile.val);
	}
	else if (infile.key == "bits_per_tile") {
		// @ATTR bits_per_tile|int|Number of "sub-tiles"
		this->bits_per_tile = static_cast<unsigned short>(std::max(Parse::toInt(infile.val), 1));
	}
	else {
		infile.error("FOW: '%s' is not a valid key.", infile.key.c_str());
		Utils::Exit(1);
	}
}

void FogOfWar::loadDefBit(FileParser &infile) {
	int val = Parse::toInt(infile.val);
	int bit = 0;

	if (val > 0) {
		bit = 1 << (val - 1);
	}

	if (def_bits.size() < static_cast<unsigned long>(bits_per_tile)+1)
		def_bits.insert(std::pair<std::string, int>(infile.key, bit));
	else {
		infile.error("FOW: bits_per_tile is '%u' but found more", bits_per_tile);
		Utils::Exit(1);
	}
}

void FogOfWar::loadDefTile(FileParser &infile) {
	std::string val = Parse::stripCarriageReturn(infile.val);
	std::string bit;
	std::map<std::string, int>::iterator it;
	int tile_bits = 0;
	unsigned long prev_comma = 0;
	unsigned long comma = 0;

	while (prev_comma < val.length()) {
		comma = val.find(",", prev_comma+1);
		if(prev_comma == 0)
			bit = val.substr(0, comma-prev_comma);
		else
			bit = val.substr(prev_comma+1, comma-prev_comma-1);

		bit = Parse::trim(bit);
		it = def_bits.find(bit);

		if (it != def_bits.end())
			tile_bits = tile_bits | it->second;
		else {
			infile.error("FOW: Bit definition '%s' not found.", bit.c_str());
			Utils::Exit(1);
		}

		prev_comma = comma;
	}

	def_tiles.insert(std::pair<std::string, int>(infile.key, tile_bits));
}

void FogOfWar::loadDefMask(FileParser &infile) {
	if (infile.key == "data") {
		def_mask = new short unsigned[(mask_radius*2+1) * (mask_radius*2+1)];
		std::string val;
		std::string tile_def;
		std::map<std::string, int>::iterator it;
		int k = 0;

		for (int j=0; j<mask_radius*2+1; j++) {
			val = infile.getRawLine();
			infile.incrementLineNum();
			if (!val.empty() && val[val.length()-1] != ',') {
				val += ',';
			}

			// verify the width of this row
			int comma_count = 0;
			for (unsigned i=0; i<val.length(); ++i) {
				if (val[i] == ',') comma_count++;
			}
			if (comma_count != mask_radius*2+1) {
				infile.error("FOW: A row of mask data has a width not equal to %d.", mask_radius*2+1);
				Utils::Exit(1);
			}

			for (int i=0; i<mask_radius*2+1; i++) {
				tile_def = Parse::popFirstString(val, ',');
				it = def_tiles.find(tile_def);
				if (it != def_tiles.end()) {
					def_mask[k++] = static_cast<unsigned short>(it->second);
				}
				else
					infile.error("FOW: Tile definition '%s' not found.", tile_def.c_str());
			}
		}
		def_tiles.clear();
		def_bits.clear();
	}
}

FogOfWar::~FogOfWar() {
}
