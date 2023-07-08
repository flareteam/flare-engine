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

class LevelScaledValue {
public:
	int item_level;
	float base;
	float per_item_level;
	float per_player_level;
	std::vector<float> per_player_primary;

	LevelScaledValue();
	~LevelScaledValue() {}
	float get() const;
	void parse(std::string& s);
};

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
	enum {
		UNKNOWN = 0,
		SPEED,
		ATTACK_SPEED,
		STAT,
		DAMAGE_MIN,
		DAMAGE_MAX,
		RESIST_ELEMENT,
		PRIMARY_STAT,
		RESOURCE_STAT,
		POWER_LEVEL,
	};

	unsigned type;
	size_t index;
	size_t sub_index; // used for resource stats
	LevelScaledValue value;
	PowerID power_id; // for bonus_power_level
	bool is_multiplier;

	BonusData()
		: type(UNKNOWN)
		, index(0)
		, sub_index(0)
		, value()
		, power_id(0)
		, is_multiplier(false)
	{
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
		NO_STASH_NULL = 0,
		NO_STASH_IGNORE = 1,
		NO_STASH_PRIVATE = 2,
		NO_STASH_SHARED = 3,
		NO_STASH_ALL = 4
	};

	bool has_name;        // flag that is set when the item name is parsed
	std::string flavor;   // optional flavor text describing the item
	int level;            // rough estimate of quality, used in the loot algorithm
	ItemSetID set;              // item can be attached to item set
	std::string quality;  // should match an id from items/qualities.txt
	std::string type;     // equipment slot or base item type
	std::vector<std::string> equip_flags;   // common values include: melee, ranged, mental, shield
	int icon;             // icon index on small pixel sheet
	std::string book;     // book file location
	bool book_is_readable; // whether to display "use" or "read" in the tooltip
	std::vector<FMinMax> base_dmg; // minimum/maximum damage amount
	FMinMax base_abs;          // minimum/maximum absorb amount
	LevelScaledValue requires_level;   // Player level must match or exceed this value to use item
	std::map<size_t, LevelScaledValue> requires_stat;
	std::string requires_class;
	std::vector<BonusData> bonus;   // stat to increase/decrease e.g. hp, accuracy, speed
	std::string sfx;           // the item sound when it hits the floor or inventory, etc
	SoundID sfx_id;
	std::string gfx;           // the sprite layer shown when this item is equipped
	std::vector<LootAnimation> loot_animation;// the flying loot animation for this item
	PowerID power;            // this item can be dragged to the action bar and used as a power
	std::vector< std::pair<PowerID, PowerID> > replace_power;        // alter powers when this item is equipped. The first PowerID is replaced with the second.
	std::string power_desc;    // shows up in green text on the tooltip
	LevelScaledValue price;            // if price = 0 the item cannot be sold
	LevelScaledValue price_sell;       // if price_sell = 0, the sell price is price*vendor_ratio
	int max_quantity;     // max count per stack
	std::string pickup_status; // when this item is picked up, set a campaign state (usually for quest items)
	std::string stepfx;        // sound effect played when walking (armors only)
	std::vector<std::string> disable_slots; // if this item is equipped, it will disable slots that match the types in the list
	bool quest_item;
	int no_stash;
	std::string script;

	int getPrice(bool use_vendor_ratio);
	int getSellPrice(bool is_new_buyback);

	Item();
	~Item() {
	}
};

class ItemSet {
public:
	std::string name;            // item set name displayed on long and short tool tips
	std::vector<ItemID> items;      // items, included into set
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
	ItemStack(ItemID _item = 0, int _quantity = 0)
		: item(_item)
		, quantity(_quantity)
		, can_buyback(false) {
	}
	~ItemStack() {}
	explicit ItemStack(const Point& _p);
	bool operator > (const ItemStack &param) const;

	bool empty();
	void clear();

	ItemID item;
	int quantity;
	bool can_buyback;
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
	void getTooltipInputHint(TooltipData& tip, ItemStack stack, int context);

public:
	enum {
		VENDOR_BUY = 0,
		VENDOR_SELL = 1,
		PLAYER_INV = 2
	};

	static const bool USE_VENDOR_RATIO = true;
	static const bool DEFAULT_SELL_PRICE = true;
	static const bool TOOLTIP_INPUT_HINT = true;

	ItemManager();
	~ItemManager();
	void playSound(ItemID item, const Point& pos = Point(0,0));
	TooltipData getTooltip(ItemStack stack, StatBlock *stats, int context, bool input_hint);
	TooltipData getShortTooltip(ItemStack item);
	std::string getItemName(ItemID id);
	std::string getItemType(const std::string& _type);
	Color getItemColor(ItemID id);
	int getItemIconOverlay(size_t id);
	bool requirementsMet(const StatBlock *stats, ItemID item);

	std::map<ItemID, Item> items;
	std::vector<ItemType> item_types;
	std::map<ItemSetID, ItemSet> item_sets;
	std::vector<ItemQuality> item_qualities;
};

bool compareItemStack(const ItemStack &stack1, const ItemStack &stack2);

#endif
