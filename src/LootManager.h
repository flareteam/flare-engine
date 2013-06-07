/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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
 * class LootManager
 *
 * Handles floor loot
 */


#pragma once
#ifndef LOOT_MANAGER_H
#define LOOT_MANAGER_H

#include "CommonIncludes.h"
#include "ItemManager.h"
#include "Loot.h"
#include "Settings.h"

class Animation;

class EnemyManager;
class MenuInventory;
class WidgetTooltip;

// this means that normal items are 10x more common than epic items
// these numbers have to be balanced by various factors
const int RARITY_LOW = 7;
const int RARITY_NORMAL = 10;
const int RARITY_HIGH = 3;
const int RARITY_EPIC = 1;

// how close (map units) does the hero have to be to pick up loot?
const int LOOT_RANGE = 3 * UNITS_PER_TILE;

class LootManager {
private:

	WidgetTooltip *tip;
	StatBlock *hero;

	// functions
	void loadGraphics();

	SoundManager::SoundID sfx_loot;

	// loot refers to ItemManager indices
	std::vector<Loot> loot;

	SDL_Rect animation_pos;
	Point animation_offset;

	// enemies which should drop loot, but didnt yet.
	std::vector<const class Enemy*> enemiesDroppingLoot;

public:
	LootManager(StatBlock *_hero);
	LootManager(const LootManager &copy); // not implemented
	~LootManager();

	void handleNewMap();
	void logic();
	void renderTooltips(Point cam);
	void checkEnemiesForLoot();

	// called by enemy, who definitly wants to drop loot.
	void addEnemyLoot(const Enemy *e);
	void checkMapForLoot();
	void determineLootByEnemy(const Enemy *e, Point pos); // pick from enemy-specific loot table
	void addLoot(ItemStack stack, Point pos);
	ItemStack checkPickup(Point mouse, Point cam, Point hero_pos, MenuInventory *inv);
	ItemStack checkAutoPickup(Point hero_pos, MenuInventory *inv);
	ItemStack checkNearestPickup(Point hero_pos, MenuInventory *inv);

	void addRenders(std::vector<Renderable> &ren, std::vector<Renderable> &ren_dead);

	int tooltip_margin; // pixels between loot drop center and label
	bool full_msg;
};

#endif
