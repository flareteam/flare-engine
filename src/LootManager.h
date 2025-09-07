/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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
 * class LootManager
 *
 * Handles floor loot
 */

#ifndef LOOT_MANAGER_H
#define LOOT_MANAGER_H

#include "CommonIncludes.h"
#include "FileParser.h"
#include "ItemManager.h"
#include "Loot.h"
#include "Utils.h"

class Animation;
class EnemyManager;
class StatBlock;

class LootManager {
private:
	// functions
	void loadGraphics();
	void checkEnemiesForLoot();
	void checkMapForLoot();
	void loadLootTables();
	void getLootTable(const std::string &filename, std::vector<EventComponent> *ec_list);
	void checkLootComponent(EventComponent* ec, FPoint *pos, std::vector<ItemStack> *itemstack_vec);

	SoundID sfx_loot;
	std::string sfx_loot_channel;

	// loot refers to ItemManager indices
	std::vector<Loot> loot;

	// enemies which should drop loot, but didnt yet.
	std::vector<class StatBlock*> enemiesDroppingLoot;

	// loot tables defined in files under "loot/"
	std::map<std::string, std::vector<EventComponent> > loot_tables;

	// to prevent dropping multiple loot stacks on the same tile,
	// we block tiles that have loot dropped on them
	std::vector<Point> tiles_to_unblock;

	std::vector< std::vector<Animation*>* > animations;

public:
	static const bool DROPPED_BY_HERO = true;

	enum {
		LOOT_EC_POSX = 0,
		LOOT_EC_POSY = 1,
		LOOT_EC_CHANCE = 2,
		LOOT_EC_QUANTITY_MIN = 3,
		LOOT_EC_QUANTITY_MAX = 4,
		LOOT_EC_MAX_DROPS = 5,
		LOOT_EC_REQUIRES_LEVEL_MIN = 6,
		LOOT_EC_REQUIRES_LEVEL_MAX = 7,
		LOOT_EC_QUANTITY_PER_LEVEL_MIN = 8,
		LOOT_EC_QUANTITY_PER_LEVEL_MAX = 9,
		LOOT_EC_TYPE = 10,
	};

	enum {
		LOOT_EC_TYPE_SINGLE = 0,
		LOOT_EC_TYPE_TABLE,
		LOOT_EC_TYPE_TABLE_ROW,
	};

	LootManager();
	LootManager(const LootManager &copy); // not implemented
	~LootManager();

	void handleNewMap();
	void logic();
	void renderTooltips(const FPoint& cam);

	// called by enemy, who definitly wants to drop loot.
	void addEnemyLoot(StatBlock *e);
	void addLoot(ItemStack stack, const FPoint& pos, bool dropped_by_hero);
	void checkLoot(std::vector<EventComponent> &loot_table, FPoint *pos, std::vector<ItemStack> *itemstack_vec);
	ItemStack checkPickup(const Point& mouse, const FPoint& cam, const FPoint& hero_pos);
	ItemStack checkAutoPickup(const FPoint& hero_pos);
	ItemStack checkNearestPickup(const FPoint& hero_pos);

	void addRenders(std::vector<Renderable> &ren, std::vector<Renderable> &ren_dead);

	void parseLoot(std::string &val, EventComponent *e, std::vector<EventComponent> *ec_list);

	void removeFromEnemiesDroppingLoot(const StatBlock* sb);
};

#endif
