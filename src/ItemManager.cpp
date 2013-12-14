/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert

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

#include "CommonIncludes.h"
#include "FileParser.h"
#include "ItemManager.h"
#include "Settings.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

#include <cassert>
#include <climits>
#include <cstring>

using namespace std;

/**
 * Resizes vector vec, so it can fit index id.
 */
template <typename Ty_>
static inline void ensureFitsId(vector<Ty_>& vec, int id) {
	// id's are always greater or equal 1;
	if (id < 1) return;

	typedef typename vector<Ty_>::size_type VecSz;

	if (vec.size() <= VecSz(id+1))
		vec.resize(id+1);
}

/**
 * Trims vector allocated memory to its size.
 *
 * Emulates C++2011 vector::shrink_to_fit().
 * It is sometimes also called "swap trick".
 */
template <typename Ty_>
static inline void shrinkVecToFit(std::vector<Ty_>& vec) {
	if (vec.capacity() != vec.size())
		std::vector<Ty_>(vec).swap(vec);
}

ItemManager::ItemManager()
	: color_normal(font->getColor("item_normal"))
	, color_low(font->getColor("item_low"))
	, color_high(font->getColor("item_high"))
	, color_epic(font->getColor("item_epic"))
	, color_bonus(font->getColor("item_bonus"))
	, color_penalty(font->getColor("item_penalty"))
	, color_requirements_not_met(font->getColor("requirements_not_met"))
	, color_flavor(font->getColor("item_flavor")) {
	// NB: 20 is arbitrary picked number, but it looks like good start.
	items.reserve(20);
	item_sets.reserve(5);

	loadAll();
}

/**
 * Load all items files in all mods
 */
void ItemManager::loadAll() {

	// load each items.txt file. Individual item IDs can be overwritten with mods.
	this->loadItems();
	this->loadTypes();
	this->loadSets();

	/*
	 * Shrinks the items vector to the absolute needed size.
	 *
	 * While loading the items, the item vector grows dynamically. To have
	 * no much time overhead for reallocating the vector, a new reallocation
	 * is twice as large as the needed item id, which means in the worst case
	 * the item vector was reallocated for loading the last element, so the
	 * vector is twice as large as needed. This memory is definitly not used,
	 * so we can free it.
	 */
	shrinkVecToFit(items);
	shrinkVecToFit(item_sets);

	if (items.empty()) fprintf(stderr, "No items were found.\n");
	if (item_sets.empty()) printf("No item sets were found.\n");
}

/**
 * Load a specific items file
 *
 * @param filename The full path and name of the file to load
 */
void ItemManager::loadItems() {
	FileParser infile;

	// @CLASS Item|Description about the class and it usage, items/items.txt...
	if (!infile.open("items/items.txt", true, false))
		return;

	int id = 0;
	bool id_line = false;
	while (infile.next()) {
		if (infile.key == "id") {
			// @ATTR id|integer|An uniq id of the item used as reference from other classes.
			id_line = true;
			id = toInt(infile.val);
			ensureFitsId(items, id+1);
		}
		else id_line = false;

		if (id < 1) {
			if (id_line) fprintf(stderr, "Item index out of bounds 1-%d, skipping\n", INT_MAX);
			continue;
		}
		if (id_line) continue;

		assert(items.size() > std::size_t(id));

		if (infile.key == "name")
			// @ATTR name|string|Item name displayed on long and short tooltips.
			items[id].name = msg->get(infile.val);
		else if (infile.key == "flavor")
			// @ATTR flavor|string|
			items[id].flavor = msg->get(infile.val);
		else if (infile.key == "level")
			// @ATTR level|integer|
			items[id].level = toInt(infile.val);
		else if (infile.key == "icon") {
			// @ATTR icon|integer|
			items[id].icon = toInt(infile.nextValue());
		}
		else if (infile.key == "quality") {
			// @ATTR quality|[low:high:epic]|Item quality, corresponds to item color.
			if (infile.val == "low")
				items[id].quality = ITEM_QUALITY_LOW;
			else if (infile.val == "high")
				items[id].quality = ITEM_QUALITY_HIGH;
			else if (infile.val == "epic")
				items[id].quality = ITEM_QUALITY_EPIC;
		}
		else if (infile.key == "item_type") {
			// @ATTR item_type|string|Equipment slot [artifact, head, chest, hands, legs, feets, main, off, ring] or base item type [gem, consumable]
			items[id].type = infile.val;
		}
		else if (infile.key == "equip_flags") {
			infile.val = infile.val + ',';
			std::string flag = eatFirstString(infile.val,',');

			while (flag != "") {
				items[id].equip_flags.push_back(flag);
				flag = eatFirstString(infile.val,',');
			}
		}
		else if (infile.key == "dmg_melee") {
			// @ATTR dmg_melee|[min (integer), max (integer)]|Defines the item melee damage, if only min is specified the melee damage is fixed.
			items[id].dmg_melee_min = toInt(infile.nextValue());
			if (infile.val.length() > 0)
				items[id].dmg_melee_max = toInt(infile.nextValue());
			else
				items[id].dmg_melee_max = items[id].dmg_melee_min;
		}
		else if (infile.key == "dmg_ranged") {
			// @ATTR dmg_ranged|[min (integer), max (integer)]|Defines the item ranged damage, if only min is specified the ranged damage is fixed.
			items[id].dmg_ranged_min = toInt(infile.nextValue());
			if (infile.val.length() > 0)
				items[id].dmg_ranged_max = toInt(infile.nextValue());
			else
				items[id].dmg_ranged_max = items[id].dmg_ranged_min;
		}
		else if (infile.key == "dmg_ment") {
			// @ATTR dmg_ment|[min (integer), max (integer)]|Defines the item mental damage, if only min is specified the ranged damage is fixed.
			items[id].dmg_ment_min = toInt(infile.nextValue());
			if (infile.val.length() > 0)
				items[id].dmg_ment_max = toInt(infile.nextValue());
			else
				items[id].dmg_ment_max = items[id].dmg_ment_min;
		}
		else if (infile.key == "abs") {
			// @ATTR abs|[min (integer), max (integer)]|Defines the item absorb value, if only min is specified the absorb value is fixed.
			items[id].abs_min = toInt(infile.nextValue());
			if (infile.val.length() > 0)
				items[id].abs_max = toInt(infile.nextValue());
			else
				items[id].abs_max = items[id].abs_min;
		}
		else if (infile.key == "req") {
			// @ATTR req|[ [p:m:o:d], amount (integer) ]|Make item require specific stat level ex. req=p,6 will require hero to have level 6 in physical stats
			string s = infile.nextValue();
			if (s == "p")
				items[id].req_stat = REQUIRES_PHYS;
			else if (s == "m")
				items[id].req_stat = REQUIRES_MENT;
			else if (s == "o")
				items[id].req_stat = REQUIRES_OFF;
			else if (s == "d")
				items[id].req_stat = REQUIRES_DEF;
			items[id].req_val = toInt(infile.nextValue());
		}
		else if (infile.key == "bonus") {
			// @ATTR bonus|[power_tag (string), amount (integer)]|Adds a bonus to the item power_tag being a uniq tag of a power definition, e.: bonus=HP regen, 50
			items[id].bonus_stat.push_back(infile.nextValue());
			items[id].bonus_val.push_back(toInt(infile.nextValue()));
		}
		else if (infile.key == "soundfx") {
			// @ATTR soundfx|string|Sound effect for the specific item.
			items[id].sfx = snd->load(infile.val, "ItemManager");
		}
		else if (infile.key == "gfx")
			// @ATTR gfx|string|Graphics for the specific item.
			items[id].gfx = infile.val;
		else if (infile.key == "loot_animation") {
			// @ATTR loot_animation|string|Specifies the loot animation for the item.
			infile.val = infile.val + ',';
			LootAnimation la;
			la.name = eatFirstString(infile.val, ',');
			la.low = eatFirstInt(infile.val, ',');
			la.high = eatFirstInt(infile.val, ',');
			items[id].loot_animation.push_back(la);
		}
		else if (infile.key == "power") {
			// @ATTR power|power_id|Adds a specific power to the item which makes it usable as a power and can be placed in action bar.
			if (toInt(infile.val) > 0) {
				items[id].power = toInt(infile.val);
			}
			else fprintf(stderr, "Power index inside item %d definition out of bounds 1-%d, skipping item\n", id, INT_MAX);
		}
		else if (infile.key == "power_mod")
			// @ATTR power_mod|integer|Power modifier of item.
			items[id].power_mod = toInt(infile.val);
		else if (infile.key == "power_desc")
			// @ATTR power_desc|string|A string describing the additional power.
			items[id].power_desc = msg->get(infile.val);
		else if (infile.key == "price")
			// @ATTR price|integer|The amount of currency the item costs, if set to 0 the item cannot be sold.
			items[id].price = toInt(infile.val);
		else if (infile.key == "price_sell")
			// @ATTR sell_price|integer|The amount of currency the item is sold for, if set to 0 the sell prices is prices*vendor_ratio.
			items[id].price_sell = toInt(infile.val);
		else if (infile.key == "max_quantity")
			// @ATTR max_quantity|integer|Max item count per stack.
			items[id].max_quantity = toInt(infile.val);
		else if (infile.key == "rand_loot")
			// @ATTR rand_loot|integer|Max amount appearing in loot stack.
			items[id].rand_loot = toInt(infile.val);
		else if (infile.key == "rand_vendor")
			// @ATTR rand_vendor|integer|Max amount appearing in vendor stack.
			items[id].rand_vendor = toInt(infile.val);
		else if (infile.key == "pickup_status")
			// @ATTR pickup_status|string|Set a campaign status when item is picked up, this is used for quest items.
			items[id].pickup_status = infile.val;
		else if (infile.key == "stepfx")
			// @ATTR stepfx|string|Sound effect when walking, this applies only to armors.
			items[id].stepfx = infile.val;
		else if (infile.key == "class") {
			// @ATTR class|[classname (string), ...]|A comma separated list of classes the item belongs too.
			string classname = infile.nextValue();
			while (classname != "") {
				unsigned pos; // find the position where this classname is stored:
				for (pos = 0; pos < item_class_names.size(); pos++) {
					if (item_class_names[pos] == classname)
						break;
				}
				// if it was not found, add it to the end.
				// pos is already the correct index.
				if (pos == item_class_names.size()) {
					item_class_names.push_back(classname);
					item_class_items.push_back(vector<unsigned int>());
				}
				// add item id to the item list of that class:
				item_class_items[pos].push_back(id);
				classname = infile.nextValue();
			}
		}

	}
	infile.close();
}

void ItemManager::loadTypes() {
	FileParser infile;
	string type,description;
	type = description = "";

	// @CLASS Item Type|Definition of a item types, items/types.txt...
	if (infile.open("items/types.txt", true, false)) {
		while (infile.next()) {
			// @ATTR name|string|Item type name.
			if (infile.key == "name") type = infile.val;
			// @ATTR description|string|Item type description.
			else if (infile.key == "description") description = infile.val;

			if (type != "" && description != "") {
				item_types[type] = description;
				type = description = "";
			}
		}
		infile.close();
	}
}

string ItemManager::getItemType(std::string _type) {
	map<string,string>::iterator it,end;
	for (it=item_types.begin(), end=item_types.end(); it!=end; ++it) {
		if (_type.compare(it->first) == 0) return it->second;
	}
	// If all else fails, return the original string
	return _type;
}

void ItemManager::loadSets() {
	FileParser infile;

	// @CLASS Item Set|Definition of a item sets, items/sets.txt...
	if (!infile.open("items/sets.txt", true, false))
		return;

	int id = 0;
	bool id_line;
	while (infile.next()) {
		if (infile.key == "id") {
			// @ATTR id|integer|A uniq id for the item set.
			id_line = true;
			id = toInt(infile.val);
			ensureFitsId(item_sets, id+1);
		}
		else id_line = false;

		if (id < 1) {
			if (id_line) fprintf(stderr, "Item set index out of bounds 1-%d, skipping\n", INT_MAX);
			continue;
		}
		if (id_line) continue;

		assert(item_sets.size() > std::size_t(id));

		if (infile.key == "name") {
			// @ATTR name|string|Name of the item set.
			item_sets[id].name = msg->get(infile.val);
		}
		else if (infile.key == "items") {
			// @ATTR name|[item_id,...]|List of item id's that is part of the set.
			string item_id = infile.nextValue();
			while (item_id != "") {
				int temp_id = toInt(item_id);
				if (temp_id > 0 && temp_id < static_cast<int>(items.size())) {
					items[temp_id].set = id;
					item_sets[id].items.push_back(temp_id);
				}
				else {
					const int maxsize = static_cast<int>(items.size()-1);
					const char* cname = item_sets[id].name.c_str();
					fprintf(stderr, "Item index inside item set %s definition out of bounds 1-%d, skipping item\n", cname, maxsize);
				}
				item_id = infile.nextValue();
			}
		}
		else if (infile.key == "color") {
			// @ATTR color|color|A specific of color for the set.
			item_sets[id].color.r = toInt(infile.nextValue());
			item_sets[id].color.g = toInt(infile.nextValue());
			item_sets[id].color.b = toInt(infile.nextValue());
		}
		else if (infile.key == "bonus") {
			// @ATTR bonus|[requirements (integer), bonus stat (string), bonus (integer)]|Bonus to append to items in the set.
			Set_bonus bonus;
			bonus.requirement = toInt(infile.nextValue());
			bonus.bonus_stat = infile.nextValue();
			bonus.bonus_val = toInt(infile.nextValue());
			item_sets[id].bonus.push_back(bonus);
		}
	}
	infile.close();
}

/**
 * Renders icons at small size or large size
 * Also display the stack size
 */
void ItemManager::renderIcon(ItemStack stack, int x, int y, int size) {
	if (icons.graphicsIsNull()) return;

	SDL_Rect src, dest;
	dest.x = x;
	dest.y = y;
	src.w = src.h = dest.w = dest.h = size;

	if (stack.item > 0) {
		int columns = icons.getGraphicsWidth() / ICON_SIZE;
		src.x = (items[stack.item].icon % columns) * size;
		src.y = (items[stack.item].icon / columns) * size;
		icons.setClip(src);
		icons.setDest(dest);
		render_device->render(icons);
	}

	if (stack.quantity > 1 || items[stack.item].max_quantity > 1) {
		WidgetLabel label;
		label.set(dest.x + 2, dest.y + 2, JUSTIFY_LEFT, VALIGN_TOP, abbreviateKilo(stack.quantity), color_normal);
		label.render();
	}
}

void ItemManager::playSound(int item, Point pos) {
	snd->play(items[item].sfx, GLOBAL_VIRTUAL_CHANNEL, pos, false);
}

TooltipData ItemManager::getShortTooltip(ItemStack stack) {
	stringstream ss;
	TooltipData tip;
	SDL_Color color = color_normal;

	if (stack.item == 0) return tip;

	// color quality
	if (items[stack.item].set > 0) {
		color = item_sets[items[stack.item].set].color;
	}
	else if (items[stack.item].quality == ITEM_QUALITY_LOW) {
		color = color_low;
	}
	else if (items[stack.item].quality == ITEM_QUALITY_HIGH) {
		color = color_high;
	}
	else if (items[stack.item].quality == ITEM_QUALITY_EPIC) {
		color = color_epic;
	}

	// name
	if (stack.quantity > 1) {
		ss << stack.quantity << " " << items[stack.item].name;
	}
	else {
		ss << items[stack.item].name;
	}
	tip.addText(ss.str(), color);

	return tip;
}

/**
 * Create detailed tooltip showing all relevant item info
 */
TooltipData ItemManager::getTooltip(ItemStack stack, StatBlock *stats, int context) {
	TooltipData tip;
	SDL_Color color = color_normal;
	string quality_desc = "";

	if (stack.item == 0) return tip;

	// color quality
	if (items[stack.item].set > 0) {
		color = item_sets[items[stack.item].set].color;
	}
	else if (items[stack.item].quality == ITEM_QUALITY_LOW) {
		color = color_low;
		quality_desc = msg->get("Low");
	}
	else if (items[stack.item].quality == ITEM_QUALITY_NORMAL) {
		color = color_normal;
		quality_desc = msg->get("Normal");
	}
	else if (items[stack.item].quality == ITEM_QUALITY_HIGH) {
		color = color_high;
		quality_desc = msg->get("High");
	}
	else if (items[stack.item].quality == ITEM_QUALITY_EPIC) {
		color = color_epic;
		quality_desc = msg->get("Epic");
	}

	// name
	stringstream ss;
	if (stack.quantity < 1000)
		ss << items[stack.item].name;
	else
		ss << items[stack.item].name << " (" << stack.quantity << ")";
	tip.addText(ss.str(), color);

	// level
	if (items[stack.item].level != 0) {
		tip.addText(msg->get("Level %d", items[stack.item].level));
	}

	// type
	if (items[stack.item].type != "other") {
		tip.addText(msg->get(getItemType(items[stack.item].type)));
	}

	// damage
	if (items[stack.item].dmg_melee_max > 0) {
		if (items[stack.item].dmg_melee_min < items[stack.item].dmg_melee_max)
			tip.addText(msg->get("Melee damage: %d-%d", items[stack.item].dmg_melee_min, items[stack.item].dmg_melee_max));
		else
			tip.addText(msg->get("Melee damage: %d", items[stack.item].dmg_melee_max));
	}
	if (items[stack.item].dmg_ranged_max > 0) {
		if (items[stack.item].dmg_ranged_min < items[stack.item].dmg_ranged_max)
			tip.addText(msg->get("Ranged damage: %d-%d", items[stack.item].dmg_ranged_min, items[stack.item].dmg_ranged_max));
		else
			tip.addText(msg->get("Ranged damage: %d", items[stack.item].dmg_ranged_max));
	}
	if (items[stack.item].dmg_ment_max > 0) {
		if (items[stack.item].dmg_ment_min < items[stack.item].dmg_ment_max)
			tip.addText(msg->get("Mental damage: %d-%d", items[stack.item].dmg_ment_min, items[stack.item].dmg_ment_max));
		else
			tip.addText(msg->get("Mental damage: %d", items[stack.item].dmg_ment_max));
	}

	// absorb
	if (items[stack.item].abs_max > 0) {
		if (items[stack.item].abs_min < items[stack.item].abs_max)
			tip.addText(msg->get("Absorb: %d-%d", items[stack.item].abs_min, items[stack.item].abs_max));
		else
			tip.addText(msg->get("Absorb: %d", items[stack.item].abs_max));
	}

	// bonuses
	unsigned bonus_counter = 0;
	string modifier;
	while (bonus_counter < items[stack.item].bonus_val.size() && items[stack.item].bonus_stat[bonus_counter] != "") {
		if (items[stack.item].bonus_stat[bonus_counter] == "speed") {
			modifier = msg->get("%d% Speed", items[stack.item].bonus_val[bonus_counter]);
			if (items[stack.item].bonus_val[bonus_counter] >= 100) color = color_bonus;
			else color = color_penalty;
		}
		else {
			if (items[stack.item].bonus_val[bonus_counter] > 0) {
				modifier = msg->get("Increases %s by %d",
									items[stack.item].bonus_val[bonus_counter],
									msg->get(items[stack.item].bonus_stat[bonus_counter]));

				color = color_bonus;
			}
			else {
				modifier = msg->get("Decreases %s by %d",
									items[stack.item].bonus_val[bonus_counter],
									msg->get(items[stack.item].bonus_stat[bonus_counter]));

				color = color_penalty;
			}
		}
		tip.addText(modifier, color);
		bonus_counter++;
	}

	// power
	if (items[stack.item].power_desc != "") {
		tip.addText(items[stack.item].power_desc, color_bonus);
	}

	// requirement
	if (items[stack.item].req_val > 0) {
		if (items[stack.item].req_stat == REQUIRES_PHYS) {
			if (stats->get_physical() < items[stack.item].req_val) color = color_requirements_not_met;
			else color = color_normal;
			tip.addText(msg->get("Requires Physical %d", items[stack.item].req_val), color);
		}
		else if (items[stack.item].req_stat == REQUIRES_MENT) {
			if (stats->get_mental() < items[stack.item].req_val) color = color_requirements_not_met;
			else color = color_normal;
			tip.addText(msg->get("Requires Mental %d", items[stack.item].req_val), color);
		}
		else if (items[stack.item].req_stat == REQUIRES_OFF) {
			if (stats->get_offense() < items[stack.item].req_val) color = color_requirements_not_met;
			else color = color_normal;
			tip.addText(msg->get("Requires Offense %d", items[stack.item].req_val), color);
		}
		else if (items[stack.item].req_stat == REQUIRES_DEF) {
			if (stats->get_defense() < items[stack.item].req_val) color = color_requirements_not_met;
			else color = color_normal;
			tip.addText(msg->get("Requires Defense %d", items[stack.item].req_val), color);
		}
	}

	if (COLORBLIND && quality_desc != "") {
		color = color_normal;
		tip.addText(msg->get("Quality: %s", quality_desc), color);
	}

	// flavor text
	if (items[stack.item].flavor != "") {
		tip.addText(items[stack.item].flavor, color_flavor);
	}

	// buy or sell price
	if (items[stack.item].price > 0 && stack.item != CURRENCY_ID) {

		int price_per_unit;
		if (context == VENDOR_BUY) {
			price_per_unit = items[stack.item].price;
			if (stats->currency < items[stack.item].price) color = color_requirements_not_met;
			else color = color_normal;
			if (items[stack.item].max_quantity <= 1)
				tip.addText(msg->get("Buy Price: %d %s", price_per_unit, CURRENCY), color);
			else
				tip.addText(msg->get("Buy Price: %d %s each", price_per_unit, CURRENCY), color);
		}
		else if (context == VENDOR_SELL) {
			price_per_unit = items[stack.item].getSellPrice();
			if (stats->currency < price_per_unit) color = color_requirements_not_met;
			else color = color_normal;
			if (items[stack.item].max_quantity <= 1)
				tip.addText(msg->get("Buy Price: %d %s", price_per_unit, CURRENCY), color);
			else
				tip.addText(msg->get("Buy Price: %d %s each", price_per_unit, CURRENCY), color);
		}
		else if (context == PLAYER_INV) {
			price_per_unit = items[stack.item].getSellPrice();
			if (price_per_unit == 0) price_per_unit = 1;
			if (items[stack.item].max_quantity <= 1)
				tip.addText(msg->get("Sell Price: %d %s", price_per_unit, CURRENCY));
			else
				tip.addText(msg->get("Sell Price: %d %s each", price_per_unit, CURRENCY));
		}
	}

	if (items[stack.item].set > 0) {
		// item set bonuses
		ItemSet set = item_sets[items[stack.item].set];
		bonus_counter = 0;
		modifier = "";

		tip.addText("\n" + msg->get("Set: ") + msg->get(item_sets[items[stack.item].set].name), set.color);

		while (bonus_counter < set.bonus.size() && set.bonus[bonus_counter].bonus_stat != "") {
			if (set.bonus[bonus_counter].bonus_val > 0) {
				modifier = msg->get("%d items: ", set.bonus[bonus_counter].requirement) + msg->get("Increases %s by %d", set.bonus[bonus_counter].bonus_val, msg->get(set.bonus[bonus_counter].bonus_stat));
			}
			else {
				modifier = msg->get("%d items: ", set.bonus[bonus_counter].requirement) + msg->get("Decreases %s by %d", set.bonus[bonus_counter].bonus_val, msg->get(set.bonus[bonus_counter].bonus_stat));
			}
			tip.addText(modifier, set.color);
			bonus_counter++;
		}
	}

	return tip;
}

ItemManager::~ItemManager() {
}

/**
 * Compare two item stack to be able to sorting them on their item_id in the vendors' stock
 */
bool ItemStack::operator > (const ItemStack &param) const {
	if (item == 0 && param.item > 0) {
		// Make the empty slots the last while sorting
		return true;
	}
	else if (item > 0 && param.item == 0) {
		// Make the empty slots the last while sorting
		return false;
	}
	else {
		return item > param.item;
	}
}

int Item::getSellPrice() {
	int new_price = 0;
	if (price_sell != 0)
		new_price = price_sell;
	else
		new_price = static_cast<int>(price * VENDOR_RATIO);
	if (new_price == 0) new_price = 1;

	return new_price;
}

