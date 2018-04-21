/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

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
 * class TileSet
 *
 * TileSet storage and file loading
 */

#include "FileParser.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "TileSet.h"
#include "UtilsParsing.h"

TileSet::TileSet()
	: sprites(NULL) {
	reset();
}

void TileSet::reset() {

	if (sprites) {
		delete sprites;
		sprites = NULL;
	}

	for (unsigned i = 0; i < tiles.size(); i++) {
		if (tiles[i].tile) delete tiles[i].tile;
	}

	tiles.clear();
	anim.clear();

	max_size_x = 0;
	max_size_y = 0;
}

void TileSet::loadGraphics(const std::string& filename) {
	if (sprites) {
		delete sprites;
		sprites = NULL;
	}

	for (unsigned i = 0; i < tiles.size(); i++) {
		if (tiles[i].tile) delete tiles[i].tile;
	}
	tiles.clear();

	Image *graphics = render_device->loadImage(filename);
	if (graphics) {
		sprites = graphics->createSprite();
		graphics->unref();
	}
}

void TileSet::load(const std::string& filename) {
	if (current_filename == filename) return;

	reset();

	FileParser infile;

	// @CLASS TileSet|Description of tilesets in tilesets/
	if (infile.open(filename)) {
		while (infile.next()) {
			if (infile.key == "img") {
				// @ATTR img|filename|Filename of a tile sheet image.
				loadGraphics(infile.val);
			}
			else if (infile.key == "tile") {
				// @ATTR tile|int, int, int, int, int, int, int : Index, X, Y, Width, Height, X offset, Y offset|A single tile definition.

				// Verify that we have graphics for tiles
				if (!sprites) {
					infile.error("Tileset: No 'img' defined. Aborting.");
					logErrorDialog("Tileset: No 'img' defined. Aborting.");
					mods->resetModConfig();
					Exit(1);
				}

				unsigned index = popFirstInt(infile.val);

				if (index >= tiles.size())
					tiles.resize(index + 1);

				tiles[index].tile = sprites->getGraphics()->createSprite();

				tiles[index].tile->setClipX(popFirstInt(infile.val));
				tiles[index].tile->setClipY(popFirstInt(infile.val));
				tiles[index].tile->setClipW(popFirstInt(infile.val));
				tiles[index].tile->setClipH(popFirstInt(infile.val));
				tiles[index].offset.x = popFirstInt(infile.val);
				tiles[index].offset.y = popFirstInt(infile.val);
				max_size_x = std::max(max_size_x, (tiles[index].tile->getClip().w / TILE_W) + 1);
				max_size_y = std::max(max_size_y, (tiles[index].tile->getClip().h / TILE_H) + 1);
			}
			else if (infile.key == "animation") {
				// @ATTR animation|list(int, int, int, duration) : Tile index, X, Y, duration|An animation for a tile. Durations are in 'ms' or 's'.
				int frame = 0;
				unsigned TILE_ID = popFirstInt(infile.val);

				if (TILE_ID >= anim.size())
					anim.resize(TILE_ID + 1);

				std::string repeat_val = popFirstString(infile.val);
				while (repeat_val != "") {
					anim[TILE_ID].frames++;
					anim[TILE_ID].pos.resize(frame + 1);
					anim[TILE_ID].frame_duration.resize(frame + 1);
					anim[TILE_ID].pos[frame].x = toInt(repeat_val);
					anim[TILE_ID].pos[frame].y = popFirstInt(infile.val);
					anim[TILE_ID].frame_duration[frame] = static_cast<unsigned short>(parse_duration(popFirstString(infile.val)));

					frame++;
					repeat_val = popFirstString(infile.val);
				}
			}
			else {
				infile.error("TileSet: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	current_filename = filename;
}

void TileSet::logic() {
	for (unsigned i = 0; i < anim.size() ; i++) {
		Tile_Anim &an = anim[i];
		if (!an.frames)
			continue;
		if (an.duration >= an.frame_duration[an.current_frame]) {
			tiles[i].tile->setClipX(an.pos[an.current_frame].x);
			tiles[i].tile->setClipY(an.pos[an.current_frame].y);
			an.duration = 0;
			an.current_frame = static_cast<unsigned short>((an.current_frame + 1) % an.frames);
		}
		an.duration++;
	}
}

TileSet::~TileSet() {
	if (sprites) delete sprites;
	for (unsigned i = 0; i < tiles.size(); i++) {
		if (tiles[i].tile) delete tiles[i].tile;
	}
}
