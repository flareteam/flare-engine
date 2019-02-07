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
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

#include <math.h>

MapParallax::MapParallax()
	: current_layer(0)
	, loaded(false)
	, current_filename("")
{
}

MapParallax::~MapParallax() {
	clear();
}

void MapParallax::clear() {
	for (size_t i = 0; i < layers.size(); ++i) {
		delete layers[i].sprite;
	}

	layers.clear();

	loaded = false;
}

void MapParallax::load(const std::string& filename) {
	current_filename = filename;

	if (loaded)
		clear();

	if (!settings->parallax_layers)
		return;

	// @CLASS MapParallax|Description of maps/parallax/
	FileParser infile;
	if (infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section && infile.section == "layer") {
				layers.resize(layers.size()+1);
			}

			if (layers.empty())
				continue;

			if (infile.key == "image") {
				// @ATTR layer.image|filename|Image file to use as a scrolling background.
				Image *graphics = render_device->loadImage(infile.val, RenderDevice::ERROR_NORMAL);
				if (graphics) {
					layers.back().sprite = graphics->createSprite();
					graphics->unref();
				}
			}
			else if (infile.key == "speed") {
				// @ATTR layer.speed|float|Speed at which the background will move relative to the camera.
				layers.back().speed = Parse::toFloat(infile.val);
			}
			else if (infile.key == "fixed_speed") {
				// @ATTR layer.fixed_speed|float, float : X speed, Y speed|Speed at which the background will move independent of the camera movement.
				layers.back().fixed_speed.x = Parse::toFloat(Parse::popFirstString(infile.val));
				layers.back().fixed_speed.y = Parse::toFloat(Parse::popFirstString(infile.val));
			}
			else if (infile.key == "map_layer") {
				// @ATTR layer.map_layer|string|The tile map layer that this parallax layer will be rendered on top of.
				layers.back().map_layer = infile.val;
			}
		}

		infile.close();
	}

	loaded = true;
}

void MapParallax::setMapCenter(int x, int y) {
	map_center.x = static_cast<float>(x) + 0.5f;
	map_center.y = static_cast<float>(y) + 0.5f;
}

void MapParallax::render(const FPoint& cam, const std::string& map_layer) {
	if (!settings->parallax_layers) {
		if (loaded)
			clear();

		return;
	}
	else if (!loaded) {
		load(current_filename);
	}

	if (map_layer.empty())
		current_layer = 0;

	for (size_t i = current_layer; i < layers.size(); ++i) {
		if (layers[i].map_layer != map_layer)
			continue;

		int width = layers[i].sprite->getGraphicsWidth();
		int height = layers[i].sprite->getGraphicsHeight();

		layers[i].fixed_offset.x += layers[i].fixed_speed.x;
		layers[i].fixed_offset.y += layers[i].fixed_speed.y;

		if (layers[i].fixed_offset.x > static_cast<float>(width))
			layers[i].fixed_offset.x -= static_cast<float>(width);
		if (layers[i].fixed_offset.x < static_cast<float>(-width))
			layers[i].fixed_offset.x += static_cast<float>(width);

		if (layers[i].fixed_offset.y > static_cast<float>(height))
			layers[i].fixed_offset.y -= static_cast<float>(height);
		if (layers[i].fixed_offset.y < static_cast<float>(-height))
			layers[i].fixed_offset.y += static_cast<float>(height);

		FPoint dp;
		dp.x = map_center.x - cam.x;
		dp.y = map_center.y - cam.y;

		Point center_tile = Utils::mapToScreen(map_center.x + (dp.x * layers[i].speed) + layers[i].fixed_offset.x, map_center.y + (dp.y * layers[i].speed) + layers[i].fixed_offset.y, cam.x, cam.y);
		center_tile.x -= width/2;
		center_tile.y -= height/2;

		Point draw_pos;
		draw_pos.x = center_tile.x - static_cast<int>(ceilf(static_cast<float>(settings->view_w_half + center_tile.x) / static_cast<float>(width))) * width;
		draw_pos.y = center_tile.y - static_cast<int>(ceilf(static_cast<float>(settings->view_h_half + center_tile.y) / static_cast<float>(height))) * height;
		Point start_pos = draw_pos;

		while (draw_pos.x < settings->view_w) {
			draw_pos.y = start_pos.y;
			while (draw_pos.y < settings->view_h) {
				layers[i].sprite->setDest(draw_pos.x, draw_pos.y);
				render_device->render(layers[i].sprite);

				draw_pos.y += height;
			}
			draw_pos.x += width;
		}

		current_layer++;
	}
}

