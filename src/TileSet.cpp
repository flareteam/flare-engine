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

#include "EngineSettings.h"
#include "FileParser.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "TileSet.h"
#include "UtilsParsing.h"

TileSet::TileSet() {
	reset();
}

void TileSet::reset() {
	for (size_t i = 0; i < sprites.size(); ++i) {
		if (sprites[i])
			delete sprites[i];
	}
	sprites.clear();

	for (size_t i = 0; i < tiles.size(); ++i) {
		if (tiles[i].tile)
			delete tiles[i].tile;
	}

	tiles.clear();
	anim.clear();

	max_size_x = 0;
	max_size_y = 0;
}

void TileSet::loadGraphics(const std::string& filename, Sprite** sprite) {
	if (*sprite) {
		delete *sprite;
		*sprite = NULL;
	}

	if (filename.empty())
		return;

	Image *graphics = render_device->loadImage(filename, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		*sprite = graphics->createSprite();
		graphics->unref();
	}
}

void TileSet::load(const std::string& filename) {
	if (current_filename == filename) return;

	reset();

	std::vector<std::string> image_filenames;
	std::vector<size_t> tile_images;
	std::vector<Rect> tile_clips;
	std::vector<Point> tile_offsets;

	FileParser infile;

	// @CLASS TileSet|Description of tilesets in tilesets/
	if (infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if ((infile.new_section && infile.section == "tileset") || (sprites.empty() && infile.section.empty())) {
				image_filenames.resize(image_filenames.size() + 1);
				sprites.resize(sprites.size() + 1, NULL);
			}

			if (infile.key == "img") {
				// @ATTR tileset.img|filename|Filename of a tile sheet image.
				image_filenames.back() = infile.val;
			}
			else if (infile.key == "tile") {
				// @ATTR tileset.tile|int, int, int, int, int, int, int : Index, X, Y, Width, Height, X offset, Y offset|A single tile definition.

				size_t index = Parse::popFirstInt(infile.val);

				if (index >= tiles.size()) {
					if (index==511) std::cout << "LOADED BIG INDEX TILE\n";
					tiles.resize(index + 1);
					std::cout << "NEW SIZE: " << tiles.size() << "\n";
					tile_images.resize(index + 1);
					tile_clips.resize(index + 1);
					tile_offsets.resize(index + 1);
				}

				Rect clip;
				clip.x = Parse::popFirstInt(infile.val);
				clip.y = Parse::popFirstInt(infile.val);
				clip.w = Parse::popFirstInt(infile.val);
				clip.h = Parse::popFirstInt(infile.val);

				Point offset;
				offset.x = Parse::popFirstInt(infile.val);
				offset.y = Parse::popFirstInt(infile.val);

				tile_images[index] = image_filenames.size() - 1;
				tile_clips[index] = clip;
				tile_offsets[index] = offset;
			}
			else if (infile.key == "animation") {
				// @ATTR tileset.animation|list(int, int, int, duration) : Tile index, X, Y, duration|An animation for a tile. Durations are in 'ms' or 's'.

				unsigned short frame = 0;
				size_t index = Parse::popFirstInt(infile.val);

				if (index >= anim.size())
					anim.resize(index + 1);

				std::string repeat_val = Parse::popFirstString(infile.val);
				while (repeat_val != "") {
					anim[index].frames++;
					anim[index].pos.resize(frame + 1);
					anim[index].frame_duration.resize(frame + 1);
					anim[index].pos[frame].x = Parse::toInt(repeat_val);
					anim[index].pos[frame].y = Parse::popFirstInt(infile.val);
					anim[index].frame_duration[frame] = static_cast<unsigned short>(Parse::toDuration(Parse::popFirstString(infile.val)));

					frame++;
					repeat_val = Parse::popFirstString(infile.val);
				}
			}
			else {
				infile.error("TileSet: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// load tileset images
	for (size_t i = 0; i < image_filenames.size(); ++i) {
		loadGraphics(image_filenames[i], &sprites[i]);
	}

	// set up individual tile sprites
	for (size_t i = 0; i < tiles.size(); ++i) {
		if (!sprites[tile_images[i]])
			continue;

		tiles[i].tile = sprites[tile_images[i]]->getGraphics()->createSprite();
		tiles[i].tile->setClipFromRect(tile_clips[i]);
		tiles[i].offset = tile_offsets[i];

		max_size_x = std::max(max_size_x, (tiles[i].tile->getClip().w / eset->tileset.tile_w) + 1);
		max_size_y = std::max(max_size_y, (tiles[i].tile->getClip().h / eset->tileset.tile_h) + 1);
	}

	current_filename = filename;
}

void TileSet::logic() {
	for (size_t i = 0; i < anim.size(); ++i) {
		Tile_Anim &an = anim[i];
		if (!an.frames)
			continue;
		if (tiles[i].tile && an.duration >= an.frame_duration[an.current_frame]) {
			Rect clip = tiles[i].tile->getClip();
			clip.x = an.pos[an.current_frame].x;
			clip.y = an.pos[an.current_frame].y;
			tiles[i].tile->setClipFromRect(clip);
			an.duration = 0;
			an.current_frame = static_cast<unsigned short>((an.current_frame + 1) % an.frames);
		}
		an.duration++;
	}
}

TileSet::~TileSet() {
	for (size_t i = 0; i < sprites.size(); ++i) {
		if (sprites[i])
			delete sprites[i];
	}

	for (size_t i = 0; i < tiles.size(); ++i) {
		if (tiles[i].tile)
			delete tiles[i].tile;
	}
}
