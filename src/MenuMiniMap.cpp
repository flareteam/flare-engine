/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
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
 * class MenuMiniMap
 */

#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "MapCollision.h"
#include "Menu.h"
#include "MenuMiniMap.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

#include <cmath>

MenuMiniMap::MenuMiniMap()
	: color_wall(128,128,128,255)
	, color_obst(64,64,64,255)
	, color_hero(255,255,255,255)
	, map_surface(NULL)
	, map_surface_2x(NULL)
	, label(new WidgetLabel())
	, compass(NULL)
	, current_zoom(1)
	, lock_zoom_change(false)
{
	std::string bg_filename;

	// Load config settings
	FileParser infile;
	// @CLASS MenuMiniMap|Description of menus/minimap.txt
	if (infile.open("menus/minimap.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR map_pos|rectangle|Position and dimensions of the map.
			if(infile.key == "map_pos") {
				pos = Parse::toRect(infile.val);
			}
			// @ATTR text_pos|label|Position of the text label with the map name.
			else if(infile.key == "text_pos") {
				label->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR background|filename|Optional background image.
			else if (infile.key == "background") {
				bg_filename = infile.val;
			}
			else {
				infile.error("MenuMiniMap: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	label->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	if (!bg_filename.empty())
		setBackground(bg_filename);

	// load compass image
	Image *gfx = NULL;
	if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
		gfx = render_device->loadImage("images/menus/compass_iso.png", RenderDevice::ERROR_NORMAL);
	}
	else if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL) {
		gfx = render_device->loadImage("images/menus/compass_ortho.png", RenderDevice::ERROR_NORMAL);
	}
	if (gfx) {
		compass = gfx->createSprite();
		gfx->unref();
	}

	align();
}

void MenuMiniMap::align() {
	Menu::align();
	label->setPos(window_area.x, window_area.y);

	map_area.x = window_area.x + pos.x;
	map_area.y = window_area.y + pos.y;
	map_area.w = pos.w;
	map_area.h = pos.h;

	// compass
	Point compass_pos(window_area.x + pos.x + pos.w - compass->getGraphicsWidth(), pos.y + window_area.y);
	compass->setDestFromPoint(compass_pos);
}

void MenuMiniMap::setMapTitle(const std::string& map_title) {
	label->setText(map_title);
}

void MenuMiniMap::createMapSurface(Sprite **target_surface, int w, int h) {
	if (!target_surface)
		return;

	if (*target_surface) {
		delete *target_surface;
		*target_surface = NULL;
	}

	Image *graphics;
	graphics = render_device->createImage(w, h);
	if (graphics) {
		*target_surface = graphics->createSprite();
		(*target_surface)->getGraphics()->fillWithColor(Color(0,0,0,0));
		graphics->unref();
	}
}

void MenuMiniMap::logic() {
	if (settings->minimap_mode != Settings::MINIMAP_HIDDEN && inpt->usingMouse()) {
		bool is_within_maparea = Utils::isWithinRect(map_area, inpt->mouse);

		if (!lock_zoom_change)
			lock_zoom_change = inpt->pressing[Input::MAIN1] && !is_within_maparea;
		else if (!inpt->pressing[Input::MAIN1])
			lock_zoom_change = false;

		if (is_within_maparea && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1] && !lock_zoom_change) {
			inpt->lock[Input::MAIN1] = true;
			if (settings->minimap_mode == Settings::MINIMAP_NORMAL)
				settings->minimap_mode = Settings::MINIMAP_2X;
			else if (settings->minimap_mode == Settings::MINIMAP_2X)
				settings->minimap_mode = Settings::MINIMAP_NORMAL;
		}
	}
}

void MenuMiniMap::render() {
}

void MenuMiniMap::render(const FPoint& hero_pos) {
	if (!settings->show_hud || settings->minimap_mode == Settings::MINIMAP_HIDDEN)
		return;

	Menu::render();

	label->render();

	if (settings->minimap_mode == Settings::MINIMAP_NORMAL)
		current_zoom = 1;
	else if (settings->minimap_mode == Settings::MINIMAP_2X)
		current_zoom = 2;

	renderMapSurface(hero_pos);

	if (compass) {
		render_device->render(compass);
	}
}

void MenuMiniMap::prerender(MapCollision *collider, int map_w, int map_h) {
	map_size.x = map_w;
	map_size.y = map_h;

	if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
		prerenderIso(collider, &map_surface, 1);
		prerenderIso(collider, &map_surface_2x, 2);
	}
	else {
		// eset->tileset.TILESET_ORTHOGONAL
		prerenderOrtho(collider, &map_surface, 1);
		prerenderOrtho(collider, &map_surface_2x, 2);
	}
}

void MenuMiniMap::renderMapSurface(const FPoint& hero_pos) {

	Point hero_offset;
	if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
		Point h_pos = Point(hero_pos);
		hero_offset.x = h_pos.x - h_pos.y + std::max(map_size.x, map_size.y);
		hero_offset.y = h_pos.x + h_pos.y;
	}
	else {
		// eset->tileset.TILESET_ORTHOGONAL
		hero_offset = Point(hero_pos);
	}

	Rect clip;
	clip.x = (current_zoom * hero_offset.x) - pos.w/2;
	clip.y = (current_zoom * hero_offset.y) - pos.h/2;
	clip.w = pos.w;
	clip.h = pos.h;

	Sprite* target_surface = NULL;
	if (settings->minimap_mode == Settings::MINIMAP_NORMAL && map_surface) {
		target_surface = map_surface;
	}
	else if (settings->minimap_mode == Settings::MINIMAP_2X && map_surface_2x) {
		target_surface = map_surface_2x;
	}

	if (target_surface) {
		// ensure the clip doesn't exceed the surface dimensions
		// TODO should this be in RenderDevice?
		const int target_w = target_surface->getGraphicsWidth();
		const int target_h = target_surface->getGraphicsHeight();
		if (clip.x + clip.w > target_w)
			clip.w = target_w - clip.x;
		if (clip.y + clip.h > target_h)
			clip.h = target_h - clip.y;

		target_surface->setClipFromRect(clip);
		target_surface->setDestFromRect(map_area);
		render_device->render(target_surface);
	}

	// draw the player cursor
	Point center(window_area.x + pos.x + pos.w/2, window_area.y + pos.y + pos.h/2);
	render_device->drawLine(center.x - current_zoom, center.y, center.x + current_zoom, center.y, color_hero);
	render_device->drawLine(center.x, center.y - current_zoom, center.x, center.y + current_zoom, color_hero);
}

void MenuMiniMap::prerenderOrtho(MapCollision *collider, Sprite** target_surface, int zoom) {
	int surface_size = std::max(map_size.x + zoom, map_size.y + zoom) * zoom;
	createMapSurface(target_surface, surface_size, surface_size);

	if (!(*target_surface))
		return;

	Image* target_img = (*target_surface)->getGraphics();
	const int target_w = (*target_surface)->getGraphicsWidth();
	const int target_h = (*target_surface)->getGraphicsHeight();

	Color draw_color;

	target_img->beginPixelBatch();

	for (int i=0; i<std::min(target_w, map_size.x); i++) {
		for (int j=0; j<std::min(target_h, map_size.y); j++) {
			bool draw_tile = true;
			int tile_type = collider->colmap[i][j];

			if (tile_type == 1 || tile_type == 5) draw_color = color_wall;
			else if (tile_type == 2 || tile_type == 6) draw_color = color_obst;
			else draw_tile = false;

			if (draw_tile) {
				for (int l = 0; l < zoom; l++) {
					for (int k =0; k < zoom; k++) {
						target_img->drawPixel((zoom*i)+k, (zoom*j)+l, draw_color);
					}
				}
			}
		}
	}

	target_img->endPixelBatch();
}

void MenuMiniMap::prerenderIso(MapCollision *collider, Sprite** target_surface, int zoom) {
	int surface_size = std::max(map_size.x + zoom, map_size.y + zoom) * 2 * zoom;
	createMapSurface(target_surface, surface_size, surface_size);

	if (!(*target_surface))
		return;

	// a 2x1 pixel area correlates to a tile, so we can traverse tiles using pixel counting
	Color draw_color;
	int tile_type;

	Point tile_cursor;
	tile_cursor.x = -std::max(map_size.x, map_size.y)/2;
	tile_cursor.y = std::max(map_size.x, map_size.y)/2;

	bool odd_row = false;

	Image* target_img = (*target_surface)->getGraphics();
	const int target_w = (*target_surface)->getGraphicsWidth();
	const int target_h = (*target_surface)->getGraphicsHeight();

	target_img->beginPixelBatch();

	// for each pixel row
	for (int j=0; j<target_h; j++) {

		// for each 2-px wide column
		for (int i=0; i<target_w; i+=2) {

			// if this tile is the max map size
			if (tile_cursor.x >= 0 && tile_cursor.y >= 0 && tile_cursor.x < map_size.x && tile_cursor.y < map_size.y) {

				tile_type = collider->colmap[tile_cursor.x][tile_cursor.y];
				bool draw_tile = true;

				// walls and low obstacles show as different colors
				if (tile_type == 1 || tile_type == 5) draw_color = color_wall;
				else if (tile_type == 2 || tile_type == 6) draw_color = color_obst;
				else draw_tile = false;

				if (draw_tile) {
					if (odd_row) {
						for (int l = 0; l < zoom; l++) {
							for (int k = 0; k < zoom * 2; k++) {
								target_img->drawPixel((zoom*i)+k, (zoom*j)+l, draw_color);
							}
						}
					}
					else {
						for (int l = 0; l < zoom; l++) {
							for (int k = -((zoom * 2) - zoom); k < zoom; k++) {
								target_img->drawPixel((zoom*i)+k, (zoom*j)+l, draw_color);
							}
						}
					}
				}
			}

			// moving screen-right in isometric is +x -y in map coordinates
			tile_cursor.x++;
			tile_cursor.y--;
		}

		// return tile cursor to next row of tiles
		if (odd_row) {
			odd_row = false;
			tile_cursor.x -= target_w/2;
			tile_cursor.y += (target_w/2 +1);
		}
		else {
			odd_row = true;
			tile_cursor.x -= (target_w/2 -1);
			tile_cursor.y += target_w/2;
		}
	}

	target_img->endPixelBatch();
}

MenuMiniMap::~MenuMiniMap() {
	delete map_surface;
	delete map_surface_2x;

	delete label;
	delete compass;
}
