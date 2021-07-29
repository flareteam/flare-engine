/*
Copyright © 2011-2012 Clint Bellanger
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

#ifndef MENU_MINI_MAP_H
#define MENU_MINI_MAP_H

#include "CommonIncludes.h"
#include "Utils.h"

class MapCollision;
class Sprite;
class WidgetButton;
class WidgetLabel;

class MenuMiniMap : public Menu {
private:
	enum {
		TILE_HERO = 1,
		TILE_ENEMY = 2,
		TILE_NPC = 3,
		TILE_TELEPORT = 4,
		TILE_ALLY = 5
	};

	Color color_wall;
	Color color_obst;
	Color color_hero;
	Color color_enemy;
	Color color_ally;
	Color color_npc;
	Color color_teleport;

	Sprite *map_surface;
	Sprite *map_surface_2x;
	Sprite *map_surface_entities;
	Sprite *map_surface_entities_2x;
	Point map_size;

	Rect pos;
	WidgetLabel *label;
	Sprite *compass;
	Rect map_area;
	WidgetButton* button_config;

	int current_zoom;
	bool lock_zoom_change;

	std::vector< std::vector<unsigned short> > entities;

	void createMapSurface(Sprite** target_surface, int w, int h);
	void renderMapSurface(const FPoint& hero_pos);
	void prerenderOrtho(MapCollision *collider, Sprite** tile_surface, Sprite** entity_surface, int zoom);
	void prerenderIso(MapCollision *collider, Sprite** tile_surface, Sprite** entity_surface, int zoom);
	void updateIso(MapCollision *collider, Sprite** tile_surface, int zoom, Rect *bounds);
	void updateOrtho(MapCollision *collider, Sprite** tile_surface, int zoom, Rect *bounds);
	void renderEntitiesOrtho(Sprite* entity_surface, int zoom);
	void renderEntitiesIso(Sprite* entity_surface, int zoom);
	void clearEntities();
	void fillEntities();


public:
	MenuMiniMap();
	~MenuMiniMap();
	void align();
	void logic();

	void render();
	void render(const FPoint& hero_pos);
	void prerender(MapCollision *collider, int map_w, int map_h);
	void setMapTitle(const std::string& map_title);
	void update(MapCollision *collider, Rect *bounds);

	bool clicked_config;
};


#endif
