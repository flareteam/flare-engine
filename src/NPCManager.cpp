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
#include "FileParser.h"
#include "ItemManager.h"
#include "NPCManager.h"
#include "NPC.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "WidgetTooltip.h"
#include "SharedGameResources.h"
#include "UtilsParsing.h"

#include <limits>

using namespace std;

NPCManager::NPCManager(StatBlock *_stats)
	: tip(new WidgetTooltip())
	, stats(_stats)
	, tip_buf()
	, tooltip_margin(0) {
	FileParser infile;
	// load tooltip_margin from engine config file
	// @CLASS NPCManager|Description of engine/tooltips.txt
	if (infile.open("engine/tooltips.txt")) {
		while (infile.next()) {
			if (infile.key == "npc_tooltip_margin") {
				// @ATTR npc_tooltip_margin|integer|Vertical offset for NPC labels.
				tooltip_margin = toInt(infile.val);
			}
		}
		infile.close();
	}
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
	}

}

void NPCManager::logic() {
	for (unsigned i=0; i<npcs.size(); i++) {
		npcs[i]->logic();
	}
}

int NPCManager::getID(std::string npcName) {
	for (unsigned i=0; i<npcs.size(); i++) {
		if (npcs[i]->filename == npcName) return i;
	}
	return -1;
}

int NPCManager::checkNPCClick(Point mouse, FPoint cam) {
	Point p;
	Rect r;
	for (unsigned i=0; i<npcs.size(); i++) {

		p = map_to_screen(npcs[i]->pos.x, npcs[i]->pos.y, cam.x, cam.y);

		Renderable ren = npcs[i]->activeAnimation->getCurrentFrame(npcs[i]->direction);
		r.w = ren.src.w;
		r.h = ren.src.h;
		r.x = p.x - ren.offset.x;
		r.y = p.y - ren.offset.y;

		if (isWithin(r, mouse)) {
			return i;
		}
	}
	return -1;
}

int NPCManager::getNearestNPC(FPoint pos) {
	int nearest = -1;
	float best_distance = std::numeric_limits<float>::max();

	for (unsigned i=0; i<npcs.size(); i++) {
		float distance = calcDist(pos, npcs[i]->pos);
		if (distance < best_distance) {
			best_distance = distance;
			nearest = i;
		}
	}

	if (best_distance > INTERACT_RANGE) nearest = -1;

	return nearest;
}

/**
 * On mouseover, display NPC's name
 */
void NPCManager::renderTooltips(FPoint cam, Point mouse, int nearest) {
	Point p;
	Rect r;
	int id = -1;

	for (unsigned i=0; i<npcs.size(); i++) {
		if ((NO_MOUSE || TOUCHSCREEN) && nearest != -1 && (unsigned)nearest != i) continue;

		p = map_to_screen(npcs[i]->pos.x, npcs[i]->pos.y, cam.x, cam.y);

		Renderable ren = npcs[i]->activeAnimation->getCurrentFrame(npcs[i]->direction);
		r.w = ren.src.w;
		r.h = ren.src.h;
		r.x = p.x - ren.offset.x;
		r.y = p.y - ren.offset.y;

		if ((NO_MOUSE || TOUCHSCREEN) && nearest != -1 && (unsigned)nearest == i) {
			id = nearest;
			break;
		}
		else if (!NO_MOUSE && isWithin(r, mouse)) {
			id = i;
			break;
		}
	}
	if (id != -1 && TOOLTIP_CONTEXT != TOOLTIP_MENU) {

		// adjust dest.y so that the tooltip floats above the item
		p.y -= tooltip_margin;

		// use current tip or make a new one?
		if (!tip_buf.compareFirstLine(npcs[id]->name)) {
			tip_buf.clear();
			tip_buf.addText(npcs[id]->name);
		}

		tip->render(tip_buf, p, STYLE_TOPLABEL);
		TOOLTIP_CONTEXT = TOOLTIP_MAP;
	}
	else if (TOOLTIP_CONTEXT != TOOLTIP_MENU) {
		TOOLTIP_CONTEXT = TOOLTIP_NONE;
	}
}

NPCManager::~NPCManager() {
	for (unsigned i=0; i<npcs.size(); i++) {
		delete npcs[i];
	}

	tip_buf.clear();
	delete tip;
}
