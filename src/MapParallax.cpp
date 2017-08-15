/*
Copyright © 2017 Justin Jacobs

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


#include "FileParser.h"
#include "MapParallax.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

#include <math.h>
#include <cassert>

MapParallax::MapParallax() {
}

MapParallax::~MapParallax() {
	clear();
}

void MapParallax::clear() {
	for (size_t i = 0; i < sprites.size(); ++i) {
		delete sprites[i];
	}

	sprites.clear();
	speeds.clear();
}

void MapParallax::load(const std::string& filename) {
	clear();

	// @CLASS MapParallax|Description of maps/backgrounds/
	FileParser infile;
	if (infile.open(filename)) {
		while (infile.next()) {
			if (infile.new_section && infile.section == "layer") {
				sprites.resize(sprites.size()+1, NULL);
				speeds.resize(speeds.size()+1);
				fixed_speeds.resize(fixed_speeds.size()+1);
				fixed_offsets.resize(fixed_offsets.size()+1);
			}

			if (sprites.empty() || speeds.empty())
				continue;

			if (infile.key == "image") {
				// @ATTR layer.image|filename|Image file to use as a scrolling background.
				Image *graphics = render_device->loadImage(infile.val);
				if (graphics) {
					sprites.back() = graphics->createSprite();
					graphics->unref();
				}
			}
			else if (infile.key == "speed") {
				// @ATTR layer.speed|float|Speed at which the background will move relative to the camera.
				speeds.back() = toFloat(infile.val);
			}
			else if (infile.key == "fixed_speed") {
				// @ATTR layer.fixed_speed|float, float : X speed, Y speed|Speed at which the background will move independent of the camera movement.
				fixed_speeds.back().x = toFloat(popFirstString(infile.val));
				fixed_speeds.back().y = toFloat(popFirstString(infile.val));
			}
		}

		infile.close();
	}

	assert(sprites.size() == speeds.size());
	assert(sprites.size() == fixed_speeds.size());
	assert(sprites.size() == fixed_offsets.size());
}

void MapParallax::setMapCenter(int x, int y) {
	map_center.x = static_cast<float>(x) + 0.5f;
	map_center.y = static_cast<float>(y) + 0.5f;
}

void MapParallax::render(const FPoint& cam) {
	for (size_t i = 0; i < sprites.size(); ++i) {
		int width = sprites[i]->getGraphicsWidth();
		int height = sprites[i]->getGraphicsHeight();

		fixed_offsets[i].x += fixed_speeds[i].x;
		fixed_offsets[i].y += fixed_speeds[i].y;

		if (fixed_offsets[i].x > static_cast<float>(width))
			fixed_offsets[i].x -= static_cast<float>(width);
		if (fixed_offsets[i].x < static_cast<float>(width*(-1)))
			fixed_offsets[i].x += static_cast<float>(width);

		if (fixed_offsets[i].y > static_cast<float>(height))
			fixed_offsets[i].y -= static_cast<float>(height);
		if (fixed_offsets[i].y < static_cast<float>(height*(-1)))
			fixed_offsets[i].y += static_cast<float>(height);

		FPoint dp;
		dp.x = map_center.x - cam.x;
		dp.y = map_center.y - cam.y;

		Point center_tile = map_to_screen(map_center.x + (dp.x * speeds[i]) + fixed_offsets[i].x, map_center.y + (dp.y * speeds[i]) + fixed_offsets[i].y, cam.x, cam.y);
		center_tile.x -= width/2;
		center_tile.y -= height/2;

		Point draw_pos;
		draw_pos.x = center_tile.x - static_cast<int>(ceil(static_cast<float>(VIEW_W_HALF + center_tile.x) / static_cast<float>(width))) * width;
		draw_pos.y = center_tile.y - static_cast<int>(ceil(static_cast<float>(VIEW_H_HALF + center_tile.y) / static_cast<float>(height))) * height;
		Point start_pos = draw_pos;

		while (draw_pos.x < VIEW_W) {
			draw_pos.y = start_pos.y;
			while (draw_pos.y < VIEW_H) {
				sprites[i]->setDest(draw_pos.x, draw_pos.y);
				render_device->render(sprites[i]);

				draw_pos.y += height;
			}
			draw_pos.x += width;
		}
	}
}

