/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson

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

#include "CommonIncludes.h"
#include "GameStatePlay.h"
#include "Map.h"
#include "MapCollision.h"
#include "Settings.h"
#include "TileSet.h"
#include "Utils.h"
#include "StatBlock.h"
#include "TooltipData.h"

class FileParser;
class WidgetTooltip;

class MapRenderer : public Map {
private:

	Mix_Music *music;

	WidgetTooltip *tip;
	TooltipData tip_buf;
	Point tip_pos;
	bool show_tooltip;

	bool enemyGroupPlaceEnemy(float x, float y, Map_Group &g);
	void pushEnemyGroup(Map_Group &g);

	std::string played_music_filename;

	void clearQueues();

	// some events are automatically triggered when the map is loaded
	void executeOnLoadEvents();

	void drawRenderable(std::vector<Renderable>::iterator r_cursor);

	void renderIsoLayer(const Map_Layer& layerdata);

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

	void createTooltip(Event_Component *ec);

	FPoint shakycam;
	TileSet tset;

	int npc_tooltip_margin;

public:
	// functions
	MapRenderer();
	~MapRenderer();

	MapRenderer(const MapRenderer &copy); // not implemented

	int load(std::string filename);
	void logic();
	void render(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	void checkEvents(FPoint loc);
	void checkHotspots();
	void checkNearestEvent();
	void checkTooltip();

	// some events are triggered on exiting the map
	void executeOnMapExitEvents();

	// some events can trigger powers
	void activatePower(int power_index, unsigned statblock_index, FPoint &target);

	bool isValidTile(const unsigned &tile);

	// cam(x,y) is where on the map the camera is pointing
	FPoint cam;

	// indicates that the map was changed by an event, so the GameStatePlay
	// will tell the mini map to update.
	bool map_change;

	MapCollision collider;

	// event-created loot or items
	std::vector<Event_Component> loot;

	// teleport handling
	bool teleportation;
	FPoint teleport_destination;
	std::string teleport_mapname;
	std::string respawn_map;
	FPoint respawn_point;

	// cutscene handling
	bool cutscene;
	std::string cutscene_file;

	// message handling
	std::string log_msg;

	// shaky cam
	int shaky_cam_ticks;

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
	std::vector<SoundManager::SoundID> sids;

	// npc handling
	int npc_id;

	void loadMusic();

	/**
	 * The index of the layer, which mixes with the objects on screen. Layers
	 * before that are painted below objects; Layers after are painted on top.
	 */
	unsigned index_objectlayer;

};


#endif
