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
#include "Utils.h"
#include "UtilsParsing.h"

short unsigned FogOfWar::TILE_HIDDEN = 0;

FogOfWar::FogOfWar()
	: dark_layer_id(0)
	, fog_layer_id(0)
	, mask_definition("engine/fow_mask.txt")
	, mask_radius(0)
	, bits_per_tile(0)
	, def_mask(NULL)
	, bounds(0,0,0,0)
	, color_sight(255,255,255)
	, color_fog(128,128,128)
	, color_dark(0,0,0)
	, update_minimap(true)
	, loaded(false)
	, prev_hero_pos(-1, -1) {
}

int FogOfWar::load() {
	FileParser infile;
	// @CLASS FogOfWar|Description of engine/fow_mask.txt
	if (!infile.open(mask_definition, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return 0;

	bool invalid_config = false;

	if (!loaded) {
		Utils::logInfo("FogOfWar: Loading mask '%s'", mask_definition.c_str());

		while (infile.next()) {
			if (infile.section == "header") {
				loadHeader(infile);
			}
			else if (infile.section == "bits") {
				if (infile.new_section) {
					def_bits.clear();
					def_tiles.clear();
					if (def_mask) {
						delete def_mask;
						def_mask = NULL;
					}
				}

				loadDefBit(infile);
			}
			else if (infile.section == "tiles") {
				if (infile.new_section) {
					def_tiles.clear();
					if (def_mask) {
						delete def_mask;
						def_mask = NULL;
					}
					if (def_bits.empty()) {
						infile.error("FogOfWar: Unable to load tile section because no bits are defined.");
					}
				}

				loadDefTile(infile);
			}
			else if (infile.section == "mask") {
				if (infile.new_section) {
					if (def_mask) {
						delete def_mask;
						def_mask = NULL;
					}
					if (def_tiles.empty()) {
						infile.error("FogOfWar: Unable to load mask section because no tiles are defined.");
					}
				}

				loadDefMask(infile);
			}
		}

		infile.close();

		if (bits_per_tile > 0 && def_bits.empty()) {
			Utils::logError("FogOfWar: No bits defined, but bits_per_tile > 0");
			invalid_config = true;
		}
		else if (static_cast<size_t>(bits_per_tile + 1) != def_bits.size()) {
			Utils::logError("FogOfWar: Found %u bits, but bits_per_tile is %d. Setting bits_per_tile to %u.", def_bits.size()-1, bits_per_tile, def_bits.size()-1);
			bits_per_tile = static_cast<int>(def_bits.size()) - 1;
		}

		if (!def_mask) {
			Utils::logError("FogOfWar: No mask defined.");
			invalid_config = true;
		}

		if (invalid_config) {
			bits_per_tile = 0;
			if (def_mask) {
				delete def_mask;
				def_mask = NULL;
			}
		}
		else {
			for (unsigned short i=0; i<bits_per_tile; i++) {
				TILE_HIDDEN |= static_cast<unsigned short>(1<<i);
			}
		}

		def_tiles.clear();
		def_bits.clear();
	}

	if (!def_mask) {
		mapr->fogofwar = FogOfWar::TYPE_NONE;
		invalid_config = true;
	}

	if (mapr->fogofwar == FogOfWar::TYPE_OVERLAY) {
		if (this->tileset_dark.empty()) {
			if (!loaded)
				Utils::logError("FogOfWar: tileset_dark is not set");

			mapr->fogofwar = FogOfWar::TYPE_TINT;
		}
		if (this->tileset_fog.empty()) {
			if (!loaded)
				Utils::logError("FogOfWar: tileset_fog is not set");

			mapr->fogofwar = FogOfWar::TYPE_TINT;
		}
		if (!invalid_config && !loaded && mapr->fogofwar == FogOfWar::TYPE_OVERLAY) {
			tset_dark.load(tileset_dark);
			tset_fog.load(tileset_fog);
		}
	}

	loaded = true;

	return 0;
}

void FogOfWar::logic() {
	if (prev_hero_pos.x == pc->stats.pos.x && prev_hero_pos.y == pc->stats.pos.y)
		return;

	prev_hero_pos = pc->stats.pos;

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
		return color_fog;
	else if (mapr->layers[dark_layer_id][x][y] > 0)
		return color_dark;
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
	if (!def_mask)
		return;

	calcBoundaries();
	const unsigned short * mask = &def_mask[0];

	for (int x = bounds.x; x <= bounds.w; x++) {
		for (int y = bounds.y; y <= bounds.h; y++) {
			if (x>=0 && y>=0 && x < mapr->w && y < mapr->h) {
				unsigned short prev_dark_tile = mapr->layers[dark_layer_id][x][y];

				mapr->layers[dark_layer_id][x][y] &= *mask;
				mapr->layers[fog_layer_id][x][y] = *mask;

				if (prev_dark_tile != mapr->layers[dark_layer_id][x][y]) {
					update_minimap = true;
				}
			}
			mask++;
		}
	}
}

void FogOfWar::loadHeader(FileParser &infile) {
	if (infile.key == "radius") {
		// @ATTR header.radius|int|Fog of war mask radius, also how far the player can see.
		this->mask_radius = Parse::toInt(infile.val);
	}
	else if (infile.key == "bits_per_tile") {
		// @ATTR header.bits_per_tile|int|How may bits(subdivisions) a tile is made of. In powers of two. Example: if it is set to 4 then the tile will be subdivided in 4, let's say North, South, East, West.
		this->bits_per_tile = static_cast<unsigned short>(std::max(Parse::toInt(infile.val), 1));
	}
	else if (infile.key == "color_dark") {
		// @ATTR header.color_dark|color|Tint color for dark tiles. Used by fog of war type 2-tint.
		this->color_dark = Parse::toRGB(infile.val);
	}
	else if (infile.key == "color_fog") {
		// @ATTR header.color_fog|color|Tint color for fog tiles. Used by fog of war type 2-tint.
		this->color_fog = Parse::toRGB(infile.val);
	}
	else if (infile.key == "tileset_dark") {
		// @ATTR header.tileset_dark|filename|Filename of a tileset definition to use for unvisited areas. Used by fog of war type 3-overlay.
		this->tileset_dark = infile.val;
	}
	else if (infile.key == "tileset_fog") {
		// @ATTR header.tileset_fog|filename|Filename of a tileset definition to use for foggy areas. Used by fog of war type 3-overlay.
		this->tileset_fog = infile.val;
	}
	else {
		infile.error("FogOfWar: '%s' is not a valid key.", infile.key.c_str());
	}
}

void FogOfWar::loadDefBit(FileParser &infile) {
	// @ATTR bits.bit|string, int: Name, Value|A fog of war bit definition can have any name. Better to keep it simple and short. There must be a bit definition that has the value 0. Example: If we have 4 bits per tile then we define: bit=BIT_0,0, bit=BIT_N,1, bit=BIT_W,2, bit=BIT_S,3, bit=BIT_E,4.
	if (infile.key == "bit") {
		int bit = 0;
		std::string bit_name = Parse::popFirstString(infile.val);
		int val = Parse::popFirstInt(infile.val);

		if (val > 0) {
			bit = 1 << (val - 1);
		}

		if (def_bits.size() < static_cast<unsigned long>(bits_per_tile)+1)
			def_bits.insert(std::pair<std::string, int>(bit_name, bit));
		else {
			infile.error("FogOfWar: bits_per_tile is '%u' but found more", bits_per_tile);
		}
	}
	else {
		infile.error("FogOfWar: '%s' is not a valid key.", infile.key.c_str());
	}
}

void FogOfWar::loadDefTile(FileParser &infile) {
	// @ATTR tiles.tile|string, repeatable(predefined_string): Name, Bit definitions|A fog of war tile definition can have any name. Better to keep it simple and short. There must be a tile definition that contains no bits and a tile definition that contains all bits. Example: A tile containing North and West bits will be tile=NW,BIT_N,BIT_W.
	if (infile.key == "tile") {
		if (def_bits.empty())
			return;

		std::string tile_name = Parse::popFirstString(infile.val);
		std::string val = Parse::stripCarriageReturn(infile.val);

		std::string bit;
		int tile_bits = 0;

		unsigned long comma;
		unsigned long prev_comma = 0;

		std::map<std::string, int>::iterator it;
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
				infile.error("FogOfWar: Bit definition '%s' not found.", bit.c_str());
			}

			prev_comma = comma;
		}

		def_tiles.insert(std::pair<std::string, int>(tile_name, tile_bits));
	}
}

void FogOfWar::loadDefMask(FileParser &infile) {
	// @ATTR mask.data|raw|The mask definition is a matrix (2\*radius+1 by 2\*radius+1) that contains fog of war tile definitions. All the margins of the matrix must be the tile definition that contains all bits.
	if (infile.key == "data") {
		if (def_tiles.empty())
			return;

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
				// mask data is broken! Clear def_mask and abort.
				infile.error("FogOfWar: Mask data row %d/%d has a width not equal to %d.", j+1, mask_radius*2+1, mask_radius*2+1);
				delete def_mask;
				def_mask = NULL;
				break;
			}

			for (int i=0; i<mask_radius*2+1; i++) {
				tile_def = Parse::popFirstString(val, ',');
				it = def_tiles.find(tile_def);
				if (it != def_tiles.end()) {
					def_mask[k++] = static_cast<unsigned short>(it->second);
				}
				else
					infile.error("FogOfWar: Tile definition '%s' not found.", tile_def.c_str());
			}
		}
	}
}

FogOfWar::~FogOfWar() {
}
