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

#include "Avatar.h"
#include "CommonIncludes.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FogOfWar.h"
#include "FontEngine.h"
#include "InputState.h"
#include "MapCollision.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuMiniMap.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"

#include <cmath>

MenuMiniMap::MenuMiniMap()
	: color_wall(128,128,128)
	, color_obst(64,64,64)
	, color_hero(255,255,255)
	, color_enemy(255,0,0)
	, color_ally(255,255,0)
	, color_npc(0,255,0)
	, color_teleport(0,191,255)
	, map_surface(NULL)
	, map_surface_2x(NULL)
	, map_surface_entities(NULL)
	, map_surface_entities_2x(NULL)
	, label(new WidgetLabel())
	, compass(NULL)
	, button_config(NULL)
	, current_zoom(1)
	, lock_zoom_change(false)
	, clicked_config(false)

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
			// @ATTR color_wall|color, int : Color, Alpha|Color used for walls.
			else if (infile.key == "color_wall") {
				color_wall = Parse::toRGBA(infile.val);
			}
			// @ATTR color_obst|color, int : Color, Alpha|Color used for small obstacles and pits.
			else if (infile.key == "color_obst") {
				color_obst = Parse::toRGBA(infile.val);
			}
			// @ATTR color_hero|color, int : Color, Alpha|Color used for the player character.
			else if (infile.key == "color_hero") {
				color_hero = Parse::toRGBA(infile.val);
			}
			// @ATTR color_enemy|color, int : Color, Alpha|Color used for enemies engaged in combat.
			else if (infile.key == "color_enemy") {
				color_enemy = Parse::toRGBA(infile.val);
			}
			// @ATTR color_ally|color, int : Color, Alpha|Color used for allies.
			else if (infile.key == "color_ally") {
				color_ally = Parse::toRGBA(infile.val);
			}
			// @ATTR color_npc|color, int : Color, Alpha|Color used for NPCs.
			else if (infile.key == "color_npc") {
				color_npc = Parse::toRGBA(infile.val);
			}
			// @ATTR color_teleport|color, int : Color, Alpha|Color used for intermap teleports.
			else if (infile.key == "color_teleport") {
				color_teleport = Parse::toRGBA(infile.val);
			}
			// @ATTR button_config|point|Position of the 'Configuration' button. The button will be hidden if not defined.
			else if (infile.key == "button_config") {
				if (!button_config) {
					button_config = new WidgetButton("images/menus/buttons/button_config.png");
				}
				Point p = Parse::toPoint(infile.val);
				button_config->setBasePos(p.x, p.y, Utils::ALIGN_TOPLEFT);
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

	if (button_config)
		button_config->tooltip = msg->get("Configuration");

	align();
}

void MenuMiniMap::align() {
	Menu::align();
	label->setPos(window_area.x, window_area.y);

	if (button_config)
		button_config->setPos(window_area.x, window_area.y);

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
	if (!settings->show_hud || settings->minimap_mode == Settings::MINIMAP_HIDDEN)
		return;

	if (inpt->usingMouse()) {
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

	if (button_config) {
		button_config->enabled = !pc->stats.corpse;
		if (button_config->checkClick()) {
			clicked_config = true;
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

	if (button_config)
		button_config->render();
}

void MenuMiniMap::prerender(MapCollision *collider, int map_w, int map_h) {
	map_size.x = map_w;
	map_size.y = map_h;

	clearEntities();

	if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
		prerenderIso(collider, &map_surface, &map_surface_entities, 1);
		prerenderIso(collider, &map_surface_2x, &map_surface_entities_2x, 2);
	}
	else {
		// eset->tileset.TILESET_ORTHOGONAL
		prerenderOrtho(collider, &map_surface, &map_surface_entities, 1);
		prerenderOrtho(collider, &map_surface_2x, &map_surface_entities_2x, 2);
	}
}
void MenuMiniMap::update(MapCollision *collider, Rect *bounds) {
	if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
		updateIso(collider, &map_surface, 1, bounds);
		updateIso(collider, &map_surface_2x, 2, bounds);
	}
	else {
		// eset->tileset.TILESET_ORTHOGONAL
		updateOrtho(collider, &map_surface, 1, bounds);
		updateOrtho(collider, &map_surface_2x, 2, bounds);
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
	Sprite* target_surface_entities = NULL;
	if (settings->minimap_mode == Settings::MINIMAP_NORMAL && map_surface) {
		target_surface = map_surface;
		target_surface_entities = map_surface_entities;
	}
	else if (settings->minimap_mode == Settings::MINIMAP_2X && map_surface_2x) {
		target_surface = map_surface_2x;
		target_surface_entities = map_surface_entities_2x;
	}

	if (target_surface) {
		target_surface->setClipFromRect(clip);
		target_surface->setDestFromRect(map_area);
		render_device->render(target_surface);
	}

	if (target_surface_entities) {
		if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
			renderEntitiesIso(target_surface_entities, current_zoom);
		}
		else {
			// eset->tileset.TILESET_ORTHOGONAL
			renderEntitiesOrtho(target_surface_entities, current_zoom);
		}

		target_surface_entities->setClipFromRect(clip);
		target_surface_entities->setDestFromRect(map_area);
		render_device->render(target_surface_entities);
	}
}

void MenuMiniMap::prerenderOrtho(MapCollision *collider, Sprite** tile_surface, Sprite** entity_surface, int zoom) {
	int surface_size = std::max(map_size.x + zoom, map_size.y + zoom) * zoom;
	createMapSurface(tile_surface, surface_size, surface_size);
	createMapSurface(entity_surface, surface_size, surface_size);

	if (!(*tile_surface))
		return;

	Image* target_img = (*tile_surface)->getGraphics();
	const int target_w = (*tile_surface)->getGraphicsWidth();
	const int target_h = (*tile_surface)->getGraphicsHeight();

	Color draw_color;

	target_img->beginPixelBatch();

	for (int i=0; i<std::min(target_w, map_size.x); i++) {
		for (int j=0; j<std::min(target_h, map_size.y); j++) {
			bool draw_tile = true;
			int tile_type = collider->colmap[i][j];

			if (tile_type == 1 || tile_type == 5) draw_color = color_wall;
			else if (tile_type == 2 || tile_type == 6) draw_color = color_obst;
			else draw_tile = false;

			if (eset->misc.fogofwar) {
				tile_type = mapr->layers[fow->layer_id][i][j];
				if (tile_type == FogOfWar::TILE_HIDDEN) draw_tile = false;
			}

			if (draw_tile && draw_color.a != 0) {
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
void MenuMiniMap::updateOrtho(MapCollision *collider, Sprite** tile_surface, int zoom, Rect *bounds) {

	if (!(*tile_surface))
		return;

	Image* target_img = (*tile_surface)->getGraphics();

	Color draw_color;

	target_img->beginPixelBatch();

	for (int i=bounds->x; i<=bounds->w; i++) {
		for (int j=bounds->y; j<=bounds->h; j++) {
			bool draw_tile = true;
			int tile_type = collider->colmap[i][j];

			if (tile_type == 1 || tile_type == 5) draw_color = color_wall;
			else if (tile_type == 2 || tile_type == 6) draw_color = color_obst;
			else draw_tile = false;

			tile_type = mapr->layers[fow->layer_id][i][j];
			if (tile_type != 0) draw_tile = false;

			if (draw_tile && draw_color.a != 0) {
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

void MenuMiniMap::prerenderIso(MapCollision *collider, Sprite** tile_surface, Sprite** entity_surface, int zoom) {
	int surface_size = std::max(map_size.x + zoom, map_size.y + zoom) * 2 * zoom;
	createMapSurface(tile_surface, surface_size, surface_size);
	createMapSurface(entity_surface, surface_size, surface_size);

	if (!(*tile_surface))
		return;

	// a 2x1 pixel area correlates to a tile, so we can traverse tiles using pixel counting
	Color draw_color;
	int tile_type;

	Point tile_cursor;
	tile_cursor.x = -std::max(map_size.x, map_size.y)/2;
	tile_cursor.y = std::max(map_size.x, map_size.y)/2;

	bool odd_row = false;

	Image* target_img = (*tile_surface)->getGraphics();
	const int target_w = (*tile_surface)->getGraphicsWidth();
	const int target_h = (*tile_surface)->getGraphicsHeight();

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
				
				if (eset->misc.fogofwar) {
					tile_type = mapr->layers[fow->layer_id][tile_cursor.x][tile_cursor.y];
					if (tile_type == FogOfWar::TILE_HIDDEN) draw_tile = false;
				}
				
				if (draw_tile && draw_color.a != 0) {
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

void MenuMiniMap::updateIso(MapCollision *collider, Sprite** tile_surface, int zoom, Rect *bounds) {
		
	if (!(*tile_surface))
		return;

	// a 2x1 pixel area correlates to a tile, so we can traverse tiles using pixel counting
	Color draw_color;
	int tile_type;

	Point tile_cursor;
	tile_cursor.x = -std::max(map_size.x, map_size.y)/2;
	tile_cursor.y = std::max(map_size.x, map_size.y)/2;

	bool odd_row = false;

	Image* target_img = (*tile_surface)->getGraphics();
	const int target_w = (*tile_surface)->getGraphicsWidth();
	const int target_h = (*tile_surface)->getGraphicsHeight();

	target_img->beginPixelBatch();

	// for each pixel row
	for (int j=0; j<target_h; j++) {

		// for each 2-px wide column
		for (int i=0; i<target_w; i+=2) {

			// if this tile is the max map size
			if (tile_cursor.x >= bounds->x && tile_cursor.y >= bounds->y && tile_cursor.x < bounds->w && tile_cursor.y < bounds->h) {

				tile_type = collider->colmap[tile_cursor.x][tile_cursor.y];
				bool draw_tile = true;

				// walls and low obstacles show as different colors
				if (tile_type == 1 || tile_type == 5) draw_color = color_wall;
				else if (tile_type == 2 || tile_type == 6) draw_color = color_obst;
				else draw_tile = false;
				
				// fog of war
				tile_type = mapr->layers[fow->layer_id][tile_cursor.x][tile_cursor.y];
				if (tile_type != 0) draw_tile = false;

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

void MenuMiniMap::renderEntitiesOrtho(Sprite* entity_surface, int zoom) {
	if (!entity_surface)
		return;

	Color draw_color;
	int tile_type;

	Image* target_img = entity_surface->getGraphics();
	const int target_w = entity_surface->getGraphicsWidth();
	const int target_h = entity_surface->getGraphicsHeight();

	target_img->fillWithColor(Color(0,0,0,0));

	clearEntities();
	fillEntities();

	target_img->beginPixelBatch();

	for (int i=0; i<std::min(target_w, map_size.x); i++) {
		for (int j=0; j<std::min(target_h, map_size.y); j++) {
			bool draw_tile = true;
			tile_type = entities[i][j];

			if (tile_type == TILE_HERO) draw_color = color_hero;
			else if (tile_type == TILE_ENEMY) draw_color = color_enemy;
			else if (tile_type == TILE_NPC) draw_color = color_npc;
			else if (tile_type == TILE_TELEPORT) draw_color = color_teleport;
			else if (tile_type == TILE_ALLY) draw_color = color_ally;
			else draw_tile = false;

			if (draw_tile && draw_color.a != 0) {
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

void MenuMiniMap::renderEntitiesIso(Sprite* entity_surface, int zoom) {
	if (!entity_surface)
		return;

	// a 2x1 pixel area correlates to a tile, so we can traverse tiles using pixel counting
	Color draw_color;
	int tile_type;

	Point tile_cursor;
	tile_cursor.x = -std::max(map_size.x, map_size.y)/2;
	tile_cursor.y = std::max(map_size.x, map_size.y)/2;

	bool odd_row = false;

	Image* target_img = entity_surface->getGraphics();
	const int target_w = entity_surface->getGraphicsWidth();
	const int target_h = entity_surface->getGraphicsHeight();

	target_img->fillWithColor(Color(0,0,0,0));

	clearEntities();
	fillEntities();

	target_img->beginPixelBatch();

	// for each pixel row
	for (int j=0; j<target_h; j++) {

		// for each 2-px wide column
		for (int i=0; i<target_w; i+=2) {

			// if this tile is the max map size
			if (tile_cursor.x >= 0 && tile_cursor.y >= 0 && tile_cursor.x < map_size.x && tile_cursor.y < map_size.y) {

				tile_type = entities[tile_cursor.x][tile_cursor.y];
				bool draw_tile = true;

				if (tile_type == TILE_HERO) draw_color = color_hero;
				else if (tile_type == TILE_ENEMY) draw_color = color_enemy;
				else if (tile_type == TILE_NPC) draw_color = color_npc;
				else if (tile_type == TILE_TELEPORT) draw_color = color_teleport;
				else if (tile_type == TILE_ALLY) draw_color = color_ally;
				else draw_tile = false;

				if (draw_tile && draw_color.a != 0) {
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

void MenuMiniMap::clearEntities() {
	entities.resize(map_size.x);
	for (size_t i=0; i<entities.size(); ++i) {
		entities[i].resize(map_size.y);
		for (size_t j=0; j<entities[i].size(); ++j) {
			entities[i][j] = 0;
		}
	}
}

void MenuMiniMap::fillEntities() {
	Point hero = Point(pc->stats.pos);
	if (hero.x >= 0 && hero.y >= 0 && hero.x < map_size.x && hero.y < map_size.y) {
		entities[hero.x][hero.y] = TILE_HERO;
	}

	for (size_t i=0; i<mapr->events.size(); ++i) {
		EventComponent* ec_minimap = mapr->events[i].getComponent(EventComponent::SHOW_ON_MINIMAP);
		if (ec_minimap && !ec_minimap->x)
			continue;

		if (mapr->events[i].getComponent(EventComponent::NPC_HOTSPOT) && EventManager::isActive(mapr->events[i])) {
			if (eset->misc.fogofwar) {
				float delta = Utils::calcDist(pc->stats.pos, mapr->events[i].center);
				if (delta > pc->sight) {
					continue;
				}
			}
			entities[mapr->events[i].location.x][mapr->events[i].location.y] = TILE_NPC;
		}
		else if ((mapr->events[i].activate_type == Event::ACTIVATE_ON_TRIGGER || mapr->events[i].activate_type == Event::ACTIVATE_ON_INTERACT) && mapr->events[i].getComponent(EventComponent::INTERMAP) && EventManager::isActive(mapr->events[i])) {
			// TODO use location when hotspot is inappropriate?
			Point event_pos(mapr->events[i].location.x, mapr->events[i].location.y);
			for (int j=event_pos.x; j<event_pos.x + mapr->events[i].location.w; ++j) {
				for (int k=event_pos.y; k<event_pos.y + mapr->events[i].location.h; ++k) {
					if (eset->misc.fogofwar)
						if (mapr->layers[fow->layer_id][event_pos.x][event_pos.y] == FogOfWar::TILE_HIDDEN) continue;

					entities[j][k] = TILE_TELEPORT;
				}
			}
		}
	}

	for (size_t i=0; i<entitym->entities.size(); ++i) {
		Entity *e = entitym->entities[i];
		if (e->stats.hp > 0) {
			if (eset->misc.fogofwar) {
				float delta = Utils::calcDist(pc->stats.pos, e->stats.pos);
				if (delta > pc->sight) {
					continue;
				}
			}
			if (e->stats.hero_ally) {
				entities[static_cast<int>(e->stats.pos.x)][static_cast<int>(e->stats.pos.y)] = TILE_ALLY;
			}
			else if (e->stats.in_combat) {
				entities[static_cast<int>(e->stats.pos.x)][static_cast<int>(e->stats.pos.y)] = TILE_ENEMY;
			}
		}
	}
}

MenuMiniMap::~MenuMiniMap() {
	delete map_surface;
	delete map_surface_2x;
	delete map_surface_entities;
	delete map_surface_entities_2x;

	delete label;
	delete compass;
	delete button_config;
}
