/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert
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
 * class ItemManager
 */

#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "CommonIncludes.h"
#include "Utils.h"

class FileParser;
class StatBlock;
class TooltipData;

class LootAnimation {
public:
	std::string name;
	int low;
	int high;
	LootAnimation()
		: name("")
		, low(0)
		, high(0) {
	}
};

class BonusData {
public:
	int stat_index; // Stats.h
	int damage_index_min; // engine/damage_types.txt
	int damage_index_max; // engine/damage_types.txt
	int resist_index; // engine/elements.txt
	int base_index;
	bool is_speed;
	bool is_attack_speed;
	int value;
	int power_id; // for bonus_power_level
	BonusData()
		: stat_index(-1)
		, damage_index_min(-1)
		, damage_index_max(-1)
		, resist_index(-1)
		, base_index(-1)
		, is_speed(false)
		, is_attack_speed(false)
		, value(0)
		, power_id(0) {
	}
};

class SetBonusData : public BonusData {
public:
	int requirement;
	SetBonusData()
		: BonusData()
		, requirement(0) {
	}
};

class ItemQuality {
public:
	std::string id;
	std::string name;
	Color color;
	int overlay_icon;
	ItemQuality()
		: id("")
		, name("")
		, color(255,255,255)
		, overlay_icon(-1) {
	}
};

class Item {
private:
	std::string name;     // item name displayed on long and short tool tips
	friend class ItemManager;

public:
	enum {
		NO_STASH_IGNORE = 0,
		NO_STASH_PRIVATE = 1,
		NO_STASH_SHARED = 2,
		NO_STASH_ALL = 3
	};

	bool has_name;        // flag that is set when the item name is parsed
	std::string flavor;   // optional flavor text describing the item
	int level;            // rough estimate of quality, used in the loot algorithm
	int set;              // item can be attached to item set
	std::string quality;  // should match an id from items/qualities.txt
	std::string type;     // equipment slot or base item type
	std::vector<std::string> equip_flags;   // common values include: melee, ranged, mental, shield
	int icon;             // icon index on small pixel sheet
	std::string book;     // book file location
	bool book_is_readable; // whether to display "use" or "read" in the tooltip
	std::vector<int> dmg_min; // minimum damage amount
	std::vector<int> dmg_max; // maximum damage amount
	int abs_min;          // minimum absorb amount
	int abs_max;          // maximum absorb amount
	int requires_level;   // Player level must match or exceed this value to use item
	std::vector<size_t> req_stat;         // physical, mental, offense, defense
	std::vector<int> req_val;          // 1-5 (used with req_stat)
	std::string requires_class;
	std::vector<BonusData> bonus;   // stat to increase/decrease e.g. hp, accuracy, speed
	std::string sfx;           // the item sound when it hits the floor or inventory, etc
	SoundID sfx_id;
	std::string gfx;           // the sprite layer shown when this item is equipped
	std::vector<LootAnimation> loot_animation;// the flying loot animation for this item
	int power;            // this item can be dragged to the action bar and used as a power
	std::vector<Point> replace_power;        // alter powers when this item is equipped. Power id 'x' is replaced with id 'y'
	std::string power_desc;    // shows up in green text on the tooltip
	int price;            // if price = 0 the item cannot be sold
	int price_per_level;  // additional price for each character level above 1
	int price_sell;       // if price_sell = 0, the sell price is price*vendor_ratio
	int max_quantity;     // max count per stack
	std::string pickup_status; // when this item is picked up, set a campaign state (usually for quest items)
	std::string stepfx;        // sound effect played when walking (armors only)
	std::vector<std::string> disable_slots; // if this item is equipped, it will disable slots that match the types in the list
	bool quest_item;
	int no_stash;

	int getPrice();
	int getSellPrice(bool is_new_buyback);

	Item();
	~Item() {
	}
};

class ItemSet {
public:
	std::string name;            // item set name displayed on long and short tool tips
	std::vector<int> items;      // items, included into set
	std::vector<SetBonusData> bonus;// vector with stats to increase/decrease
	Color color;

	ItemSet()
		: name("") {
		color.r = 255;
		color.g = 255;
		color.b = 255;
	}

	~ItemSet() {
	}
};

class ItemStack {
public:
	ItemStack()
		: item(0)
		, quantity(0)
		, can_buyback(false) {
	}
	~ItemStack() {}
	int item;
	int quantity;
	bool can_buyback;
	bool operator > (const ItemStack &param) const;
	bool empty();
	void clear();
};

class ItemType {
public:
	ItemType()
		: id("")
		, name("") {
	}
	~ItemType() {}

	std::string id;
	std::string name;
};

class ItemManager {
protected:
	void loadItems(const std::string& filename);
	void loadTypes(const std::string& filename);
	void loadSets(const std::string& filename);
	void loadQualities(const std::string& filename);
private:
	void loadAll();
	void parseBonus(BonusData& bdata, FileParser& infile);
	void getBonusString(std::stringstream& ss, BonusData* bdata);

public:
	enum {
		VENDOR_BUY = 0,
		VENDOR_SELL = 1,
		PLAYER_INV = 2
	};

	static const bool DEFAULT_SELL_PRICE = true;

	ItemManager();
	~ItemManager();
	void playSound(int item, const Point& pos = Point(0,0));
	TooltipData getTooltip(ItemStack stack, StatBlock *stats, int context);
	TooltipData getShortTooltip(ItemStack item);
	std::string getItemName(unsigned id);
	std::string getItemType(const std::string& _type);
	Color getItemColor(unsigned id);
	int getItemIconOverlay(size_t id);
	void addUnknownItem(unsigned id);
	bool requirementsMet(const StatBlock *stats, int item);

	std::vector<Item> items;
	std::vector<ItemType> item_types;
	std::vector<ItemSet> item_sets;
	std::vector<ItemQuality> item_qualities;
};

bool compareItemStack(const ItemStack &stack1, const ItemStack &stack2);

#endif
