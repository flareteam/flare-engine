/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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
 * class NPCManager
 *
 * NPCs which are not combatative enemies are handled by this Manager.
 * Most commonly this involves vendor and conversation townspeople.
 */

#include "Animation.h"
#include "Avatar.h"
#include "CampaignManager.h"
#include "EntityManager.h"
#include "EngineSettings.h"
#include "EventManager.h"
#include "FogOfWar.h"
#include "ItemManager.h"
#include "MapRenderer.h"
#include "NPC.h"
#include "NPCManager.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "WidgetTooltip.h"

#include <limits>

NPCManager::NPCManager()
	: tip(new WidgetTooltip())
	, tip_buf() {
}

void NPCManager::addRenders(std::vector<Renderable> &r) {
	for (unsigned i=0; i<npcs.size(); i++) {
		if (mapr->fogofwar > FogOfWar::TYPE_MINIMAP) {
			float delta = Utils::calcDist(pc->stats.pos, npcs[i]->stats.pos);
			if (delta > fow->mask_radius-1.0) {
				continue;
			}
		}
		npcs[i]->addRenders(r);
	}
}

void NPCManager::handleNewMap() {

	ItemStack item_roll;

	std::map<std::string, NPC *> allies;

	// remove existing NPCs
	for (unsigned i=0; i<npcs.size(); i++) {
		if (npcs[i]->stats.hero_ally && !npcs[i]->stats.corpse && npcs[i]->stats.cur_state != StatBlock::ENTITY_DEAD && npcs[i]->stats.cur_state != StatBlock::ENTITY_CRITDEAD && npcs[i]->stats.speed > 0.0f) {
			allies[npcs[i]->filename] = npcs[i];
		}
		else {
			delete(npcs[i]);
		}
	}

	npcs.clear();

	// read the queued NPCs in the map file
	for (size_t i = 0; i < mapr->map_npcs.size(); ++i) {
		Map_NPC &mn = mapr->map_npcs[i];

		if (!camp->checkRequirementsInVector(mn.requirements))
			continue;

		// ally npc that was moved from another map should not be loaded once again
		if (allies.find(mn.id) != allies.end()) {
			continue;
		}

		NPC *npc;
		Entity *entity = entitym->getEntityPrototype(mn.id);
		if (entity) {
			npc = new NPC(*entity);
			delete entity;
		}
		else {
			npc = new NPC(Entity());
		}

		if (!npc->load(mn.id)) {
			continue;
		}

		npc->stats.pos.x = mn.pos.x;
		npc->stats.pos.y = mn.pos.y;
		npc->stats.hero_ally = false;
		npc->stats.npc = true;
		if (mn.direction != -1)
			npc->stats.direction = static_cast<unsigned char>(mn.direction);

		for (size_t j = 0; j < mn.waypoints.size(); ++j) {
			npc->stats.waypoints.push(mn.waypoints[j]);
		}
		npc->stats.wander = mn.wander_radius > 0;
		npc->stats.setWanderArea(mn.wander_radius);

		// npc->stock.sort();
		npcs.push_back(npc);
		createMapEvent(*npc, npcs.size());
		if (!mapr->collider.isValidPosition(npc->stats.pos.x, npc->stats.pos.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_TYPE_NONE))
			Utils::logInfo("NPC: Collision tile detected at NPC position (%.2f, %.2f).", npc->stats.pos.x, npc->stats.pos.y);
	}

	while (!allies.empty()) {
		NPC *npc = allies.begin()->second;
		allies.erase(allies.begin());

		npc->stats.pos = mapr->collider.getRandomNeighbor(Point(pc->stats.pos), 1, npc->stats.movement_type, MapCollision::COLLIDE_TYPE_ALL_ENTITIES);
		npc->stats.direction = pc->stats.direction;

		npcs.push_back(npc);
		createMapEvent(*npc, npcs.size());

		mapr->collider.block(npc->stats.pos.x, npc->stats.pos.y, !MapCollision::IS_ALLY);

		entitym->entities.push_back(npc);
	}

}

void NPCManager::createMapEvent(const NPC& npc, size_t _npcs) {
	// create a map event for provided npc
	Event ev;
	EventComponent ec;

	// the event hotspot is a 1x1 tile at the npc's feet
	ev.activate_type = Event::ACTIVATE_ON_INTERACT;
	ev.keep_after_trigger = true;
	Rect location;
	location.x = static_cast<int>(npc.stats.pos.x);
	location.y = static_cast<int>(npc.stats.pos.y);
	location.w = location.h = 1;
	ev.location = ev.hotspot = location;
	ev.center.x = static_cast<float>(ev.hotspot.x) + static_cast<float>(ev.hotspot.w)/2;
	ev.center.y = static_cast<float>(ev.hotspot.y) + static_cast<float>(ev.hotspot.h)/2;

	ec.type = EventComponent::NPC_ID;
	ec.data[0].Int = static_cast<int>(_npcs)-1;
	ev.components.push_back(ec);

	ec.type = EventComponent::TOOLTIP;
	ec.s = npc.name;
	ev.components.push_back(ec);

	ec.type = EventComponent::NPC_HOTSPOT;
	ec.data[0].Int = static_cast<int>(npc.stats.pos.x);
	ec.data[1].Int = static_cast<int>(npc.stats.pos.y);
	ec.id = npcs.size();
	for (size_t i = 0; i < npcs.size(); ++i) {
		if (&npc == npcs[i]) {
			ec.id = i;
			break;
		}
	}
	ev.components.push_back(ec);

	ev.type = npc.filename;

	mapr->events.push_back(ev);
}

void NPCManager::logic() {
	for (unsigned i=0; i<npcs.size(); i++) {
		npcs[i]->logic();
	}
}

int NPCManager::getID(const std::string& npcName) {
	for (unsigned i=0; i<npcs.size(); i++) {
		if (npcs[i]->filename == npcName) return i;
	}

	// could not find NPC, try loading it here
	NPC *npc;
	Entity *entity = entitym->getEntityPrototype(npcName);
	if (entity) {
		npc = new NPC(*entity);
		delete entity;
	}
	else {
		npc = new NPC(Entity());
	}

	if (npc) {
		npc->load(npcName);
		npcs.push_back(npc);
		return static_cast<int>(npcs.size()-1);
	}

	return -1;
}

Entity* NPCManager::npcFocus(const Point& mouse, const FPoint& cam, bool alive_only) {
	Point p;
	Rect r;
	for(unsigned int i = 0; i < npcs.size(); i++) {
		if(alive_only && (npcs[i]->stats.cur_state == StatBlock::ENTITY_DEAD || npcs[i]->stats.cur_state == StatBlock::ENTITY_CRITDEAD)) {
			continue;
		}
		if (!npcs[i]->stats.hero_ally) {
			continue;
		}

		if (Utils::isWithinRect(npcs[i]->getRenderBounds(cam), mouse)) {
			return npcs[i];
		}
	}
	return NULL;
}

Entity* NPCManager::getNearestNPC(const FPoint& pos, bool get_corpse) {
	Entity* nearest = NULL;
	float best_distance = std::numeric_limits<float>::max();

	for (unsigned i=0; i<npcs.size(); i++) {
		if(!get_corpse && (npcs[i]->stats.cur_state == StatBlock::ENTITY_DEAD || npcs[i]->stats.cur_state == StatBlock::ENTITY_CRITDEAD)) {
			continue;
		}
		if (get_corpse && !npcs[i]->stats.corpse) {
			continue;
		}
		if (!npcs[i]->stats.hero_ally) {
			continue;
		}

		float distance = Utils::calcDist(pos, npcs[i]->stats.pos);
		if (distance < best_distance) {
			best_distance = distance;
			nearest = npcs[i];
		}
	}

	if (best_distance > eset->misc.interact_range)
		nearest = NULL;

	return nearest;
}

NPCManager::~NPCManager() {
	for (unsigned i=0; i<npcs.size(); i++) {
		delete npcs[i];
	}

	tip_buf.clear();
	delete tip;
}
