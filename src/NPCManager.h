/*
Copyright © 2011-2012 Clint Bellanger
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

#ifndef NPC_MANAGER_H
#define NPC_MANAGER_H

#include "CommonIncludes.h"
#include "TooltipData.h"

class StatBlock;
class NPC;
class WidgetTooltip;

class NPCManager {
private:
	WidgetTooltip *tip;
	TooltipData tip_buf;

public:
	explicit NPCManager();
	NPCManager(const NPCManager &copy); // not implemented
	~NPCManager();

	std::vector<NPC*> npcs;
	void handleNewMap();
	void logic();
	void addRenders(std::vector<Renderable> &r);
	int getID(const std::string& npcName);
};

#endif
