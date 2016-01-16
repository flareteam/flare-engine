/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller

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
#include "ItemManager.h"
#include "NPC.h"
#include "NPCManager.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "WidgetTooltip.h"

#include <limits>

NPCManager::NPCManager(StatBlock *_stats)
	: tip(new WidgetTooltip())
	, stats(_stats)
	, tip_buf() {
}

void NPCManager::addRenders(std::vector<Renderable> &r) {
	for (unsigned i=0; i<npcs.size(); i++) {
		r.push_back(npcs[i]->getRender());
	}
}

void NPCManager::handleNewMap() {

	Map_NPC mn;
	ItemStack item_roll;

	// remove existing NPCs
	for (unsigned i=0; i<npcs.size(); i++)
		delete(npcs[i]);

	npcs.clear();

	// read the queued NPCs in the map file
	while (!mapr->npcs.empty()) {
		mn = mapr->npcs.front();
		mapr->npcs.pop();

		bool status_reqs_met = true;
		//if the status requirements arent met, dont load the enemy
		for (unsigned i = 0; i < mn.requires_status.size(); ++i)
			if (!camp->checkStatus(mn.requires_status[i]))
				status_reqs_met = false;

		for (unsigned i = 0; i < mn.requires_not_status.size(); ++i)
			if (camp->checkStatus(mn.requires_not_status[i]))
				status_reqs_met = false;
		if(!status_reqs_met)
			continue;

		NPC *npc = new NPC();
		npc->load(mn.id);
		npc->pos.x = mn.pos.x;
		npc->pos.y = mn.pos.y;

		// npc->stock.sort();
		npcs.push_back(npc);

		// create a map event for this npc
		Event ev;
		Event_Component ec;

		// the event hotspot is a 1x1 tile at the npc's feet
		ev.type = EVENT_ON_TRIGGER;
		ev.keep_after_trigger = true;
		Rect location;
		location.x = static_cast<int>(npc->pos.x);
		location.y = static_cast<int>(npc->pos.y);
		location.w = location.h = 1;
		ev.location = ev.hotspot = location;
		ev.center.x = static_cast<float>(ev.hotspot.x) + static_cast<float>(ev.hotspot.w)/2;
		ev.center.y = static_cast<float>(ev.hotspot.y) + static_cast<float>(ev.hotspot.h)/2;

		ec.type = EC_NPC_ID;
		ec.x = static_cast<int>(npcs.size())-1;
		ev.components.push_back(ec);

		ec.type = EC_TOOLTIP;
		ec.s = npc->name;
		ev.components.push_back(ec);

		// The hitbox for hovering/clicking on an npc is based on their first frame of animation
		// This might cause some undesired behavior for npcs that have packed animations and a lot of variation
		// However, it is sufficient for all of our current game data (fantasycore, no-name mod, polymorphable)
		Renderable ren = npc->activeAnimation->getCurrentFrame(npc->direction);
		ec.type = EC_NPC_HOTSPOT;
		ec.x = static_cast<int>(npc->pos.x);
		ec.y = static_cast<int>(npc->pos.y);
		ec.z = ren.offset.x;
		ec.a = ren.offset.y;
		ec.b = ren.src.w;
		ec.c = ren.src.h;
		ev.components.push_back(ec);

		mapr->events.push_back(ev);
	}

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
	return -1;
}

NPCManager::~NPCManager() {
	for (unsigned i=0; i<npcs.size(); i++) {
		delete npcs[i];
	}

	tip_buf.clear();
	delete tip;
}
