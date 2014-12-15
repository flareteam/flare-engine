/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson

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
#include "FileParser.h"
#include "MapCollision.h"
#include "Menu.h"
#include "MenuMiniMap.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

#include <cmath>

MenuMiniMap::MenuMiniMap()
	: color_wall(0)
	, color_obst(0)
	, color_hero(0)
	, map_surface(NULL) {

	createMapSurface();
	if (map_surface) {
		color_wall = map_surface->getGraphics()->MapRGB(128,128,128);
		color_obst = map_surface->getGraphics()->MapRGB(64,64,64);
		color_hero = map_surface->getGraphics()->MapRGB(255,255,255);
	}

	// Load config settings
	FileParser infile;
	// @CLASS MenuMiniMap|Description of menus/minimap.txt
	if (infile.open("menus/minimap.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR map_pos|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the map.
			if(infile.key == "map_pos") {
				pos = toRect(infile.val);
			}
			// @ATTR text_pos|label|Position of the text label with the map name.
			else if(infile.key == "text_pos") {
				text_pos = eatLabelInfo(infile.val);
			}
			else {
				infile.error("MenuMiniMap: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// label for map name
	label = new WidgetLabel();

	align();
}

void MenuMiniMap::getMapTitle(std::string map_title) {
	label->set(window_area.x+text_pos.x, window_area.y+text_pos.y, text_pos.justify, text_pos.valign, map_title, font->getColor("menu_normal"), text_pos.font_style);
}

void MenuMiniMap::createMapSurface() {
	if (map_surface) {
		delete map_surface;
		map_surface = NULL;
	}

	Image *graphics;
	graphics = render_device->createImage(512, 512);
	if (graphics) {
		map_surface = graphics->createSprite();
		graphics->unref();
	}
}

void MenuMiniMap::render() {
}

void MenuMiniMap::render(FPoint hero_pos) {
	if (!text_pos.hidden) label->render();

	if (map_surface) {
		if (TILESET_ORIENTATION == TILESET_ISOMETRIC)
			renderIso(hero_pos);
		else // TILESET_ORTHOGONAL
			renderOrtho(hero_pos);
	}
}

void MenuMiniMap::prerender(MapCollision *collider, int map_w, int map_h) {
	if (!map_surface) return;

	map_size.x = map_w;
	map_size.y = map_h;
	map_surface->getGraphics()->fillWithColor(map_surface->getGraphics()->MapRGBA(0,0,0,0));

	if (TILESET_ORIENTATION == TILESET_ISOMETRIC)
		prerenderIso(collider);
	else // TILESET_ORTHOGONAL
		prerenderOrtho(collider);
}

/**
 * Render a top-down version of the map (90 deg angle)
 */
void MenuMiniMap::renderOrtho(FPoint hero_pos) {

	const int herox = int(hero_pos.x);
	const int heroy = int(hero_pos.y);

	Rect clip;
	clip.x = herox - pos.w/2;
	clip.y = heroy - pos.h/2;
	clip.w = pos.w;
	clip.h = pos.h;

	Rect map_area;
	map_area.x = window_area.x + pos.x;
	map_area.y = window_area.y + pos.y;
	map_area.w = pos.w;
	map_area.h = pos.h;

	if (map_surface) {
		map_surface->setClip(clip);
		map_surface->setDest(map_area);
		render_device->render(map_surface);
	}

	render_device->drawPixel(window_area.x + pos.x + pos.w/2, window_area.y + pos.y + pos.h/2, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2 + 1, window_area.y + pos.y + pos.h/2, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2 - 1, window_area.y + pos.y + pos.h/2, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2, window_area.y + pos.y + pos.h/2 + 1, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2, window_area.y + pos.y + pos.h/2 - 1, color_hero);

}

/**
 * Render an "isometric" version of the map (45 deg angle)
 */
void MenuMiniMap::renderIso(FPoint hero_pos) {

	const int herox = int(hero_pos.x);
	const int heroy = int(hero_pos.y);
	const int heroy_screen = herox + heroy;
	const int herox_screen = herox - heroy + std::max(map_size.x, map_size.y);

	Rect clip;
	clip.x = herox_screen - pos.w/2;
	clip.y = heroy_screen - pos.h/2;
	clip.w = pos.w;
	clip.h = pos.h;

	Rect map_area;
	map_area.x = window_area.x + pos.x;
	map_area.y = window_area.y + pos.y;
	map_area.w = pos.w;
	map_area.h = pos.h;

	if (map_surface) {
		map_surface->setClip(clip);
		map_surface->setDest(map_area);
		render_device->render(map_surface);
	}

	render_device->drawPixel(window_area.x + pos.x + pos.w/2 + 1, window_area.y + pos.y + pos.h/2, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2 - 1, window_area.y + pos.y + pos.h/2, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2, window_area.y + pos.y + pos.h/2 + 1, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2, window_area.y + pos.y + pos.h/2 - 1, color_hero);
	render_device->drawPixel(window_area.x + pos.x + pos.w/2, window_area.y + pos.y + pos.h/2, color_hero);
}

void MenuMiniMap::prerenderOrtho(MapCollision *collider) {
	for (int i=0; i<std::min(map_surface->getGraphicsWidth(), map_size.x); i++) {
		for (int j=0; j<std::min(map_surface->getGraphicsHeight(), map_size.y); j++) {
			if (collider->colmap[i][j] == 1 || collider->colmap[i][j] == 5) {
				map_surface->getGraphics()->drawPixel(i, j, color_wall);
			}
			else if (collider->colmap[i][j] == 2 || collider->colmap[i][j] == 6) {
				map_surface->getGraphics()->drawPixel(i, j, color_obst);
			}
		}
	}
}

void MenuMiniMap::prerenderIso(MapCollision *collider) {
	// a 2x1 pixel area correlates to a tile, so we can traverse tiles using pixel counting
	Uint32 draw_color;
	int tile_type;

	Point tile_cursor;
	tile_cursor.x = -std::max(map_size.x, map_size.y)/2;
	tile_cursor.y = std::max(map_size.x, map_size.y)/2;

	bool odd_row = false;

	// for each pixel row
	for (int j=0; j<map_surface->getGraphicsHeight(); j++) {

		// for each 2-px wide column
		for (int i=0; i<map_surface->getGraphicsWidth(); i+=2) {

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
						map_surface->getGraphics()->drawPixel(i, j, draw_color);
						map_surface->getGraphics()->drawPixel(i+1, j, draw_color);
					}
					else {
						map_surface->getGraphics()->drawPixel(i-1, j, draw_color);
						map_surface->getGraphics()->drawPixel(i, j, draw_color);
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
			tile_cursor.x -= map_surface->getGraphicsWidth()/2;
			tile_cursor.y += (map_surface->getGraphicsWidth()/2 +1);
		}
		else {
			odd_row = true;
			tile_cursor.x -= (map_surface->getGraphicsWidth()/2 -1);
			tile_cursor.y += map_surface->getGraphicsWidth()/2;
		}
	}
}

MenuMiniMap::~MenuMiniMap() {
	if (map_surface)
		delete map_surface;

	delete label;
}
