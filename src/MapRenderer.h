/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson
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
 * class MapRenderer
 *
 * Map data structure and rendering
 * This class is capable of rendering isometric and orthogonal maps.
 */

#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include "Camera.h"
#include "CommonIncludes.h"
#include "Map.h"
#include "MapCollision.h"
#include "MapParallax.h"
#include "TileSet.h"
#include "TooltipData.h"
#include "Utils.h"

class FileParser;
class Sprite;
class WidgetTooltip;

class MapRenderer : public Map {
private:

	WidgetTooltip *tip;
	TooltipData tip_buf;
	Point tip_pos;
	bool show_tooltip;

	bool enemyGroupPlaceEnemy(float x, float y, const Map_Group &g);
	void pushEnemyGroup(Map_Group &g);

	void clearQueues();

	void drawRenderable(std::vector<Renderable>::iterator r_cursor);

	void renderIsoLayer(const Map_Layer& layerdata, const TileSet& tile_set);

	// renders only objects
	void renderIsoBackObjects(std::vector<Renderable> &r);

	// renders interleaved objects and layer
	void renderIsoFrontObjects(std::vector<Renderable> &r);
	void renderIso(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	void renderOrthoLayer(const Map_Layer& layerdata);
	void renderOrthoBackObjects(std::vector<Renderable> &r);
	void renderOrthoFrontObjects(std::vector<Renderable> &r);
	void renderOrtho(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	void clearLayers();

	void createTooltip(EventComponent *ec);

	void getTileBounds(const int_fast16_t x, const int_fast16_t y, const Map_Layer& layerdata, Rect& bounds, Point& center);

	void drawDevCursor();
	void drawDevHUD();

	void drawHiddenEntityMarkers();

	void checkHiddenEntities(const int_fast16_t x, const int_fast16_t y, const Map_Layer& layerdata, std::vector<Renderable> &r);

	TileSet tset;

	MapParallax map_parallax;

	Sprite* entity_hidden_normal;
	Sprite* entity_hidden_enemy;

	std::vector<std::vector<Renderable>::iterator> hidden_entities;

public:
	// functions
	MapRenderer();
	~MapRenderer();

	MapRenderer(const MapRenderer &copy); // not implemented

	int load(const std::string& filename);
	void logic(bool paused);
	void render(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	void checkEvents(const FPoint& loc);
	void checkHotspots();
	void checkNearestEvent();
	void checkTooltip();

	// some events are automatically triggered when the map is loaded
	void executeOnLoadEvents();

	// some events are triggered on exiting the map
	void executeOnMapExitEvents();

	// some events can trigger powers
	void activatePower(PowerID power_index, unsigned statblock_index, const FPoint &target);

	bool isValidTile(const unsigned &tile);
	Point centerTile(const Point& p);

	void setMapParallax(const std::string& mp_filename);

	// cam is where on the map the camera is pointing
	Camera cam;

	// indicates that the map was changed by an event, so the GameStatePlay
	// will tell the mini map to update.
	bool map_change;

	MapCollision collider;

	// event-created loot or items
	std::vector<EventComponent> loot;
	Point loot_count;

	// teleport handling
	bool teleportation;
	FPoint teleport_destination;
	std::string teleport_mapname;
	std::string respawn_map;
	FPoint respawn_point;

	// cutscene handling
	bool cutscene;
	std::string cutscene_file;

	// stash handling
	bool stash;
	FPoint stash_pos;

	// enemy clear
	bool enemies_cleared;

	// event talker
	std::string event_npc;

	// trigger for save game events
	bool save_game;

	// map soundids
	std::vector<SoundID> sids;

	// npc handling
	int npc_id;

	// book from map event
	std::string show_book;

	void loadMusic();

	/**
	 * The index of the layer, which mixes with the objects on screen. Layers
	 * before that are painted below objects; Layers after are painted on top.
	 */
	unsigned index_objectlayer;

	// flag used to prevent rendering when in maps/spawn.txt
	bool is_spawn_map;
};


#endif
