/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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

/**
 * class TileSet
 *
 * TileSet storage and file loading
 */

#include "TileSet.h"
#include "SharedResources.h"
#include "FileParser.h"
#include "UtilsParsing.h"
#include "Settings.h"

#include <cstdio>

using namespace std;

TileSet::TileSet() {
	reset();
}

void TileSet::reset() {

	sprites.clearGraphics();

	alpha_background = true;
	trans_r = 255;
	trans_g = 0;
	trans_b = 255;

	tiles.clear();
	anim.clear();

	max_size_x = 0;
	max_size_y = 0;
}

void TileSet::loadGraphics(const std::string& filename) {
	if (!sprites.graphicsIsNull()) {
		sprites.clearGraphics();
		tiles.clear();
	}

	if (!TEXTURE_QUALITY)
		sprites.setGraphics(render_device->loadGraphicSurface("images/tilesets/noalpha/" + filename, "Couldn't load image", false, true));

	if (sprites.graphicsIsNull())
		sprites.setGraphics(render_device->loadGraphicSurface("images/tilesets/" + filename));
}

void TileSet::load(const std::string& filename) {
	if (current_map == filename) return;

	reset();

	FileParser infile;
	string img;

	if (infile.open(filename)) {
		while (infile.next()) {
			if (infile.key == "tile") {

				infile.val = infile.val + ',';
				unsigned index = eatFirstInt(infile.val, ',');

				if (index >= tiles.size())
					tiles.resize(index + 1);

				tiles[index].tile.setClipX(eatFirstInt(infile.val, ','));
				tiles[index].tile.setClipY(eatFirstInt(infile.val, ','));
				tiles[index].tile.setClipW(eatFirstInt(infile.val, ','));
				tiles[index].tile.setClipH(eatFirstInt(infile.val, ','));
				tiles[index].offset.x = eatFirstInt(infile.val, ',');
				tiles[index].offset.y = eatFirstInt(infile.val, ',');
				max_size_x = std::max(max_size_x, (tiles[index].tile.getClip().w / TILE_W) + 1);
				max_size_y = std::max(max_size_y, (tiles[index].tile.getClip().h / TILE_H) + 1);
			}
			else if (infile.key == "img") {
				img = infile.val;
			}
			else if (infile.key == "transparency") {
				alpha_background = false;

				infile.val = infile.val + ',';
				trans_r = (Uint8)eatFirstInt(infile.val, ',');
				trans_g = (Uint8)eatFirstInt(infile.val, ',');
				trans_b = (Uint8)eatFirstInt(infile.val, ',');

			}
			else if (infile.key == "animation") {
				int frame = 0;
				unsigned TILE_ID = toInt(infile.nextValue());

				if (TILE_ID >= anim.size())
					anim.resize(TILE_ID + 1);

				string repeat_val = infile.nextValue();
				while (repeat_val != "") {
					anim[TILE_ID].frames++;
					anim[TILE_ID].pos.resize(frame + 1);
					anim[TILE_ID].frame_duration.resize(frame + 1);
					anim[TILE_ID].pos[frame].x = toInt(repeat_val);
					anim[TILE_ID].pos[frame].y = toInt(infile.nextValue());
					anim[TILE_ID].frame_duration[frame] = toInt(infile.nextValue());

					frame++;
					repeat_val = infile.nextValue();
				}
			}
		}
		infile.close();
		loadGraphics(img);
	}

	current_map = filename;
}

void TileSet::logic() {
	for (unsigned i = 0; i < anim.size() ; i++) {
		Tile_Anim &an = anim[i];
		if (!an.frames)
			continue;
		if (an.duration >= an.frame_duration[an.current_frame]) {
			tiles[i].tile.setClipX(an.pos[an.current_frame].x);
			tiles[i].tile.setClipY(an.pos[an.current_frame].y);
			an.duration = 0;
			an.current_frame = (an.current_frame + 1) % an.frames;
		}
		an.duration++;
	}
}

TileSet::~TileSet() {
	sprites.clearGraphics();
}
