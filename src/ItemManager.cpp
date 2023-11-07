/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson
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

#include "Avatar.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "ItemManager.h"
#include "MenuInventory.h"
#include "MenuManager.h"
#include "MenuPowers.h"
#include "MenuVendor.h"
#include "MessageEngine.h"
#include "NPC.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "StatBlock.h"
#include "Stats.h"
#include "TooltipData.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

#include <cassert>
#include <climits>
#include <cstring>

bool compareItemStack(const ItemStack &stack1, const ItemStack &stack2) {
	return stack1.item < stack2.item;
}

ItemStack::ItemStack(const Point& _p)
	: item(_p.x)
	, quantity(_p.y)
	, can_buyback(false)
{}

LevelScaledValue::LevelScaledValue()
	: item_level(1)
	, base(0)
	, base_max(0)
	, base_step(1)
	, per_item_level(0)
	, per_item_level_max(0)
	, per_item_level_step(1)
	, per_player_level(0)
	, per_player_level_max(0)
	, per_player_level_step(1)
	, per_player_primary(eset->primary_stats.list.size(), 0)
	, per_player_primary_max(eset->primary_stats.list.size(), 0)
	, per_player_primary_step(eset->primary_stats.list.size(), 1)
{}

float LevelScaledValue::get() const {
	float result = base + (per_item_level * static_cast<float>(item_level-1)) + (per_player_level * static_cast<float>(pc->stats.level-1));
	for (size_t i = 0; i < per_player_primary.size(); ++i) {
		result += per_player_primary[i] * static_cast<float>(pc->stats.get_primary(i)-1);
	}
	return result;
}

float LevelScaledValue::getMax() const {
	float result = base_max + (per_item_level_max * static_cast<float>(item_level-1)) + (per_player_level_max * static_cast<float>(pc->stats.level-1));
	for (size_t i = 0; i < per_player_primary_max.size(); ++i) {
		result += per_player_primary_max[i] * static_cast<float>(pc->stats.get_primary(i)-1);
	}
	return result;
}

float LevelScaledValue::getStep() const {
	float result = base_step + (per_item_level_step * static_cast<float>(item_level-1)) + (per_player_level_step * static_cast<float>(pc->stats.level-1));
	for (size_t i = 0; i < per_player_primary_step.size(); ++i) {
		result += per_player_primary_step[i] * static_cast<float>(pc->stats.get_primary(i)-1);
	}
	return result;
}

void LevelScaledValue::randomize() {
	// TODO add a source param?
	int step_count;

	step_count = static_cast<int>((base_max - base) / base_step);
	base += static_cast<float>(Math::randBetween(0, step_count)) * base_step;
	base_max = base;

	step_count = static_cast<int>((per_item_level_max - per_item_level) / per_item_level_step);
	per_item_level += static_cast<float>(Math::randBetween(0, step_count)) * per_item_level_step;
	per_item_level_max = per_item_level;

	step_count = static_cast<int>((per_player_level_max - per_player_level) / per_player_level_step);
	per_player_level += static_cast<float>(Math::randBetween(0, step_count)) * per_player_level_step;
	per_player_level_max = per_player_level;

	for (size_t i = 0; i < per_player_primary.size(); ++i) {
		step_count = static_cast<int>((per_player_primary_max[i] - per_player_primary[i]) / per_player_primary_step[i]);
		per_player_primary[i] += static_cast<float>(Math::randBetween(0, step_count)) * per_player_primary_step[i];
		per_player_primary_max[i] = per_player_primary[i];
	}
}

void LevelScaledValue::parse(std::string& s) {
	std::string section = Parse::popFirstString(s);

	while (!section.empty()) {
		if (section.find_first_of(':') != std::string::npos) {
			section += ':'; // ensure there's a trailing colon when we pop the value following the scale type

			std::string scale_type = Parse::popFirstString(section, ':');

			if (scale_type == "base") {
				base = Parse::popFirstFloat(section, ':');
				base_max = std::max(base, Parse::popFirstFloat(section, ':'));
				std::string step_str = Parse::popFirstString(section, ':');
				base_step = Parse::toFloat(step_str, 1);
			}
			else if (scale_type == "item_level") {
				per_item_level = Parse::popFirstFloat(section, ':');
				per_item_level_max = std::max(per_item_level, Parse::popFirstFloat(section, ':'));
				std::string step_str = Parse::popFirstString(section, ':');
				per_item_level_step = Parse::toFloat(step_str, 1);
			}
			else if (scale_type == "player_level") {
				per_player_level = Parse::popFirstFloat(section, ':');
				per_player_level_max = std::max(per_player_level, Parse::popFirstFloat(section, ':'));
				std::string step_str = Parse::popFirstString(section, ':');
				per_player_level_step = Parse::toFloat(step_str, 1);
			}
			else {
				// player primary stats
				size_t primary_index = eset->primary_stats.getIndexByID(scale_type);
				if (primary_index < eset->primary_stats.list.size()) {
					per_player_primary[primary_index] = Parse::popFirstFloat(section, ':');
					per_player_primary_max[primary_index] = std::max(per_player_primary[primary_index], Parse::popFirstFloat(section, ':'));
					std::string step_str = Parse::popFirstString(section, ':');
					per_player_primary_step[primary_index] = Parse::toFloat(step_str, 1);
				}
			}
		}
		else {
			// no scale type defined, assume base value
			base = Parse::popFirstFloat(section);
			base_max = base;
		}
		section = Parse::popFirstString(s);
	}
}

Item::Item()
	: name("")
	, has_name(false)
	, book_is_readable(true)
	, quest_item(false)
	, level(0)
	, icon(0)
	, max_quantity(INT_MAX)
	, no_stash(NO_STASH_NULL)
	, loot_drops_max(-1)
	, parent(0)
	, set(0)
	, sfx_id(0)
	, power(0)
	, randomizer_def(NULL)
	, base_abs()
	, flavor("")
	, quality("")
	, type("")
	, book("")
	, requires_class("")
	, sfx("")
	, gfx("")
	, power_desc("")
	, pickup_status("")
	, stepfx("")
	, script("")
	, base_dmg((eset ? eset->damage_types.list.size() : 0))
	, requires_stat((eset ? eset->primary_stats.list.size() : 0))
	, requires_level()
	, price()
	, price_sell()
{
}

ItemManager::ItemManager()
{
	loadAll();
}

ItemManager::~ItemManager() {
	for (size_t i = 0; i < items.size(); ++i) {
		delete items[i];
	}
	for (size_t i = 0; i < item_sets.size(); ++i) {
		delete item_sets[i];
	}
	for (size_t i = 0; i < randomizer_defs.size(); ++i) {
		delete randomizer_defs[i];
	}
}

bool ItemManager::isValid(ItemID item_id) {
	return item_id > 0 && item_id < items.size() && items[item_id];
}

bool ItemManager::isValidSet(ItemSetID set_id) {
	return set_id > 0 && set_id < item_sets.size() && item_sets[set_id];
}

/**
 * Load all items files in all mods
 */
void ItemManager::loadAll() {

	// load each items.txt file. Individual item IDs can be overwritten with mods.
	this->loadItems("items/items.txt");
	this->loadExtendedItems(settings->path_user + "saves/" + eset->misc.save_prefix + "/extended_items.txt");
	this->loadTypes("items/types.txt");
	this->loadSets("items/sets.txt");
	this->loadQualities("items/qualities.txt");

	if (items.empty())
		Utils::logInfo("ItemManager: No items were found.");
}

/**
 * Load a specific items file
 *
 * @param filename The (full) path and name of the file to load
 */
void ItemManager::loadItems(const std::string& filename) {
	FileParser infile;

	// @CLASS ItemManager: Items|Description of Items in items/items.txt.
	if (!infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return;

	// used to clear vectors when overriding items
	bool clear_req_stat = false;
	bool clear_bonus = false;
	bool clear_loot_anim = false;
	bool clear_replace_power = false;

	ItemID id = 0;
	Item* item;
	bool id_line;
	while (infile.next()) {
		if (infile.key == "id") {
			// @ATTR id|item_id|An uniq id of the item used as reference from other classes.
			id_line = true;
			id = Parse::toItemID(infile.val);
			if (id < items.size() && items[id]) {
				clear_req_stat = true;
				clear_bonus = true;
				clear_loot_anim = true;
				clear_replace_power = true;
			}
			else {
				items.resize(std::max(id+1, items.size()), NULL);
				items[id] = new Item();
			}
			item = items[id];

			// set the max quantity if it has not been done yet
			if (item->max_quantity == INT_MAX)
				item->max_quantity = 1;
		}
		else id_line = false;

		if (id < 1) {
			if (id_line) infile.error("ItemManager: Item index out of bounds 1-%d, skipping item.", INT_MAX);
			continue;
		}
		if (id_line) continue;

		if (infile.key == "name") {
			// @ATTR name|string|Item name displayed on long and short tooltips.
			item->name = msg->get(infile.val);
			item->has_name = true;
		}
		else if (infile.key == "flavor")
			// @ATTR flavor|string|A description of the item.
			item->flavor = msg->get(infile.val);
		else if (infile.key == "level")
			// @ATTR level|int|The item's level.
			item->level = Parse::toInt(infile.val);
		else if (infile.key == "icon") {
			// @ATTR icon|icon_id|An id for the icon to display for this item.
			item->icon = Parse::toInt(infile.val);
		}
		else if (infile.key == "book") {
			// @ATTR book|filename|A book file to open when this item is activated.
			item->book = infile.val;
		}
		else if (infile.key == "book_is_readable") {
			// @ATTR book_is_readable|bool|If true, "read" is displayed in the tooltip instead of "use". Defaults to true.
			item->book_is_readable = Parse::toBool(infile.val);
		}
		else if (infile.key == "quality") {
			// @ATTR quality|predefined_string|Item quality matching an id in items/qualities.txt
			item->quality = infile.val;
		}
		else if (infile.key == "item_type") {
			// @ATTR item_type|predefined_string|Equipment slot matching an id in items/types.txt
			item->type = infile.val;
		}
		else if (infile.key == "equip_flags") {
			// @ATTR equip_flags|list(predefined_string)|A comma separated list of flags to set when this item is equipped. See engine/equip_flags.txt.
			item->equip_flags.clear();
			std::string flag = Parse::popFirstString(infile.val);

			while (flag != "") {
				item->equip_flags.push_back(flag);
				flag = Parse::popFirstString(infile.val);
			}
		}
		else if (infile.key == "dmg") {
			// @ATTR dmg|predefined_string, float, float : Damage type, Min, Max|Defines the item's base damage type and range. Max may be ommitted and will default to Min.
			std::string dmg_type_str = Parse::popFirstString(infile.val);

			size_t dmg_type = eset->damage_types.list.size();
			for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
				if (dmg_type_str == eset->damage_types.list[i].id) {
					dmg_type = i;
					break;
				}
			}

			if (dmg_type == eset->damage_types.list.size()) {
				infile.error("ItemManager: '%s' is not a known damage type id.", dmg_type_str.c_str());
			}
			else {
				item->base_dmg[dmg_type].min = Parse::popFirstFloat(infile.val);
				if (infile.val.length() > 0)
					item->base_dmg[dmg_type].max = Parse::popFirstFloat(infile.val);
				else
					item->base_dmg[dmg_type].max = item->base_dmg[dmg_type].min;
			}
		}
		else if (infile.key == "abs") {
			// @ATTR abs|float, float : Min, Max|Defines the item absorb value, if only min is specified the absorb value is fixed.
			item->base_abs.min = Parse::popFirstFloat(infile.val);
			if (infile.val.length() > 0)
				item->base_abs.max = Parse::popFirstFloat(infile.val);
			else
				item->base_abs.max = item->base_abs.min;
		}
		else if (infile.key == "requires_level") {
			// @ATTR requires_level|list(level_scaled_value)|The hero's level must match or exceed this value in order to equip this item.
			item->requires_level.parse(infile.val);
		}
		else if (infile.key == "requires_stat") {
			// @ATTR requires_stat|repeatable(predefined_string, list(level_scaled_value)) : Primary stat name, Value|Make item require specific stat level ex. requires_stat=physical,6 will require hero to have level 6 in physical stats
			if (clear_req_stat) {
				Utils::logInfo("ItemManager: Item %zu, clearing requires_stat list.", id);
				item->requires_stat.clear();
				clear_req_stat = false;
			}

			std::string s = Parse::popFirstString(infile.val);
			size_t req_stat_index = eset->primary_stats.getIndexByID(s);
			if (req_stat_index != eset->primary_stats.list.size())
				item->requires_stat[req_stat_index].parse(infile.val);
			else
				infile.error("ItemManager: '%s' is not a valid primary stat.", s.c_str());
		}
		else if (infile.key == "requires_class") {
			// @ATTR requires_class|predefined_string|The hero's base class (engine/classes.txt) must match for this item to be equipped.
			item->requires_class = infile.val;
		}
		else if (infile.key == "bonus") {
			// @ATTR bonus|repeatable(stat_id, list(level_scaled_value)) : Stat ID, Value|Adds a bonus to the item by stat ID, example: bonus=hp,50
			if (clear_bonus) {
				Utils::logInfo("ItemManager: Item %zu, clearing bonus list.", id);
				item->bonus.clear();
				clear_bonus = false;
			}
			BonusData bdata;
			parseBonus(bdata, infile);
			item->bonus.push_back(bdata);
		}
		else if (infile.key == "bonus_power_level") {
			// @ATTR bonus_power_level|repeatable(power_id, list(level_scaled_value)) : Base power, Bonus levels|Grants bonus levels to a given base power.
			BonusData bdata;
			bdata.type = BonusData::POWER_LEVEL;
			bdata.power_id = Parse::toPowerID(Parse::popFirstString(infile.val));
			bdata.value.parse(infile.val);
			item->bonus.push_back(bdata);
		}
		else if (infile.key == "soundfx") {
			// @ATTR soundfx|filename|Sound effect filename to play for the specific item.
			item->sfx = infile.val;
			item->sfx_id = snd->load(item->sfx, "ItemManager");
		}
		else if (infile.key == "gfx")
			// @ATTR gfx|filename|Filename of an animation set to display when the item is equipped.
			item->gfx = infile.val;
		else if (infile.key == "loot_animation") {
			// @ATTR loot_animation|repeatable(filename, int, int) : Loot image, Min quantity, Max quantity|Specifies the loot animation file for the item. The max quantity, or both quantity values, may be omitted.
			if (clear_loot_anim) {
				Utils::logInfo("ItemManager: Item %zu, clearing loot_animation list.", id);
				item->loot_animation.clear();
				clear_loot_anim = false;
			}
			LootAnimation la;
			la.name = Parse::popFirstString(infile.val);
			la.low = Parse::popFirstInt(infile.val);
			la.high = Parse::popFirstInt(infile.val);
			item->loot_animation.push_back(la);
		}
		else if (infile.key == "power") {
			// @ATTR power|power_id|Adds a specific power to the item which makes it usable as a power and can be placed in action bar.
			if (Parse::toInt(infile.val) > 0)
				item->power = Parse::toInt(infile.val);
			else
				infile.error("ItemManager: Power index out of bounds 1-%d, skipping power.", INT_MAX);
		}
		else if (infile.key == "replace_power") {
			// @ATTR replace_power|repeatable(power_id, power_id) : Old power, New power|Replaces the old power id with the new power id in the action bar when equipped.
			if (clear_replace_power) {
				Utils::logInfo("ItemManager: Item %zu, clearing replace_power list.", id);
				item->replace_power.clear();
				clear_replace_power = false;
			}
			std::pair<PowerID, PowerID> power_ids;
			power_ids.first = Parse::toPowerID(Parse::popFirstString(infile.val));
			power_ids.second = Parse::toPowerID(Parse::popFirstString(infile.val));
			item->replace_power.push_back(power_ids);
		}
		else if (infile.key == "power_desc")
			// @ATTR power_desc|string|A string describing the additional power.
			item->power_desc = msg->get(infile.val);
		else if (infile.key == "price")
			// @ATTR price|list(level_scaled_value)|The amount of currency the item costs, if set to 0 the item cannot be sold.
			item->price.parse(infile.val);
		else if (infile.key == "price_per_level") {
			// @ATTR price_per_level|int|(Deprecated in v1.14.17; Use 'price=player_level:{value}' instead). Additional price for each player level above 1
			item->price.per_player_level = static_cast<float>(Parse::toInt(infile.val));
			infile.error("ItemManager: 'price_per_level' is deprecated. Use 'price=player_level:%d' instead.", static_cast<int>(item->price.per_player_level));
		}
		else if (infile.key == "price_sell")
			// @ATTR price_sell|list(level_scaled_value)|The amount of currency the item is sold for, if set to 0 the sell prices is prices*vendor_ratio.
			item->price_sell.parse(infile.val);
		else if (infile.key == "max_quantity")
			// @ATTR max_quantity|int|Max item count per stack.
			item->max_quantity = Parse::toInt(infile.val);
		else if (infile.key == "pickup_status")
			// @ATTR pickup_status|string|Set a campaign status when item is picked up, this is used for quest items.
			item->pickup_status = infile.val;
		else if (infile.key == "stepfx")
			// @ATTR stepfx|predefined_string|Sound effect when walking, this applies only to armors.
			item->stepfx = infile.val;
		else if (infile.key == "disable_slots") {
			// @ATTR disable_slots|list(predefined_string)|A comma separated list of equip slot types to disable when this item is equipped.
			item->disable_slots.clear();
			std::string slot_type = Parse::popFirstString(infile.val);

			while (slot_type != "") {
				item->disable_slots.push_back(slot_type);
				slot_type = Parse::popFirstString(infile.val);
			}
		}
		else if (infile.key == "quest_item") {
			// @ATTR quest_item|bool|If true, this item is a quest item and can not be dropped or sold. The item also can't be stashed, unless the no_stash property is set to something other than "all".
			item->quest_item = Parse::toBool(infile.val);

			// for legacy reasons, quest items can't be stashed by default
			if (item->no_stash == Item::NO_STASH_NULL)
				item->no_stash = Item::NO_STASH_ALL;
		}
		else if (infile.key == "no_stash") {
			// @ATTR no_stash|["ignore", "private", "shared", "all"]|If not set to 'ignore', this item will not be able to be put in the corresponding stash.
			std::string temp = Parse::popFirstString(infile.val);
			if (temp == "ignore")
				item->no_stash = Item::NO_STASH_IGNORE;
			else if (temp == "private")
				item->no_stash = Item::NO_STASH_PRIVATE;
			else if (temp == "shared")
				item->no_stash = Item::NO_STASH_SHARED;
			else if (temp == "all")
				item->no_stash = Item::NO_STASH_ALL;
			else
				infile.error("ItemManager: '%s' is not a valid value for 'no_stash'. Use 'ignore', 'private', 'shared', or 'all'.", temp.c_str());
		}
		else if (infile.key == "script") {
			// @ATTR script|filename|Loads and executes a script file when the item is activated from the player's inventory.
			item->script = Parse::popFirstString(infile.val);
		}
		else if (infile.key == "loot_drops_max") {
			// @ATTR loot_drops_max|int|The number of instances of this item that can drop during a single loot event.
			item->loot_drops_max = Parse::toInt(infile.val);
		}
		else if (infile.key == "randomizer_def") {
			// @ATTR randomizer_def|filename|Randomizer definition file for this item. See "ItemManager: Randomizer Definition".
			item->randomizer_def = loadRandomizerDef(infile.val);
		}
		else {
			infile.error("ItemManager: '%s' is not a valid key.", infile.key.c_str());
		}

	}
	infile.close();

	eset->misc.currency_id = verifyID(eset->misc.currency_id, NULL, !VERIFY_ALLOW_ZERO, VERIFY_ALLOCATE);

	size_t count_allocated = 0;
	for (size_t i = 0; i < items.size(); ++i) {
		item = items[i];

		if (!item)
			continue;
		else
			count_allocated++;

		// normal items can be stored in either stash
		if (item->no_stash == Item::NO_STASH_NULL) {
			item->no_stash = Item::NO_STASH_IGNORE;
		}

		item->updateLevelScaling();
	}
	Utils::logInfo("ItemManager: Item IDs = %zu reserved / %zu allocated / %zu empty / %zu bytes used", items.size()-1, count_allocated, items.size()-1-count_allocated, (sizeof(Item*) * items.size()) + (sizeof(Item) * count_allocated));

	if (eset->loot.extended_items_offset < items.size()) {
		eset->loot.extended_items_offset = items.size();
	}
	Utils::logInfo("ItemManager: Extended item offset set to %zu.", eset->loot.extended_items_offset);
}

/**
 * Load a specific item types file
 *
 * @param filename The (full) path and name of the file to load
 */
void ItemManager::loadTypes(const std::string& filename) {
	FileParser infile;

	// @CLASS ItemManager: Types|Definition of a item types, items/types.txt...
	if (infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "type") {
					// check if the previous type and remove it if there is no identifier
					if (!item_types.empty() && item_types.back().id == "") {
						item_types.pop_back();
					}
					item_types.resize(item_types.size()+1);
				}
			}

			if (item_types.empty() || infile.section != "type")
				continue;

			// @ATTR type.id|string|Item type identifier.
			if (infile.key == "id")
				item_types.back().id = infile.val;
			// @ATTR type.name|string|Item type name.
			else if (infile.key == "name")
				item_types.back().name = infile.val;
			else
				infile.error("ItemManager: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();

		// check if the last type and remove it if there is no identifier
		if (!item_types.empty() && item_types.back().id == "") {
			item_types.pop_back();
		}
	}
}

/**
 * Load a specific item qualities file
 *
 * @param filename The (full) path and name of the file to load
 */
void ItemManager::loadQualities(const std::string& filename) {
	FileParser infile;

	// @CLASS ItemManager: Qualities|Definition of a item qualities, items/types.txt...
	if (infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "quality") {
					// check if the previous quality and remove it if there is no identifier
					if (!item_qualities.empty() && item_qualities.back().id == "") {
						item_qualities.pop_back();
					}
					item_qualities.resize(item_qualities.size()+1);
				}
			}

			if (item_qualities.empty() || infile.section != "quality")
				continue;

			// @ATTR quality.id|string|Item quality identifier.
			if (infile.key == "id")
				item_qualities.back().id = infile.val;
			// @ATTR quality.name|string|Item quality name.
			else if (infile.key == "name")
				item_qualities.back().name = infile.val;
			// @ATTR quality.color|color|Item quality color.
			else if (infile.key == "color")
				item_qualities.back().color = Parse::toRGB(infile.val);
			// @ATTR quality.overlay_icon|icon_id|The icon to be used as an overlay.
			else if (infile.key == "overlay_icon")
				item_qualities.back().overlay_icon = Parse::toInt(infile.val);
			else
				infile.error("ItemManager: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();

		// check if the last quality and remove it if there is no identifier
		if (!item_qualities.empty() && item_qualities.back().id == "") {
			item_qualities.pop_back();
		}
	}
}

std::string ItemManager::getItemName(ItemID id) {
	if (!isValid(id) || !items[id]->has_name)
		items[id]->name = msg->get("Unknown Item");

	return items[id]->name;
}

std::string ItemManager::getItemType(const std::string& _type) {
	for (unsigned i=0; i<item_types.size(); ++i) {
		if (item_types[i].id == _type)
			return item_types[i].name;
	}
	// If all else fails, return the original string
	return _type;
}

Color ItemManager::getItemColor(ItemID id) {
	if (isValid(id)) {
		if (items[id]->set > 0) {
			return item_sets[items[id]->set]->color;
		}
		else {
			for (unsigned i=0; i<item_qualities.size(); ++i) {
				if (item_qualities[i].id == items[id]->quality) {
					return item_qualities[i].color;
				}
			}
		}
	}

	return font->getColor(FontEngine::COLOR_WIDGET_NORMAL);
}

int ItemManager::getItemIconOverlay(size_t id) {
	for (size_t i=0; i < item_qualities.size(); ++i) {
		if (item_qualities[i].id == items[id]->quality) {
			return item_qualities[i].overlay_icon;
		}
	}

	// no overlay icon
	return -1;
}

/**
 * Load a specific item sets file
 *
 * @param filename The (full) path and name of the file to load
 */
void ItemManager::loadSets(const std::string& filename) {
	FileParser infile;

	// @CLASS ItemManager: Sets|Definition of a item sets, items/sets.txt...
	if (!infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return;

	bool clear_bonus = true;

	ItemSet* item_set = NULL;
	ItemSetID id = 0;
	bool id_line;
	while (infile.next()) {
		if (infile.key == "id") {
			// @ATTR id|int|A uniq id for the item set.
			id_line = true;
			id = Parse::toSizeT(infile.val);

			if (id < item_sets.size() && item_sets[id]) {
				clear_bonus = true;
			}
			else {
				item_sets.resize(std::max(id+1, item_sets.size()), NULL);
				item_sets[id] = new ItemSet();
			}
			item_set = item_sets[id];
		}
		else id_line = false;

		if (id < 1) {
			if (id_line) infile.error("ItemManager: Item set index out of bounds 1-%d, skipping set.", INT_MAX);
			continue;
		}
		if (id_line) continue;

		if (infile.key == "name") {
			// @ATTR name|string|Name of the item set.
			item_set->name = msg->get(infile.val);
		}
		else if (infile.key == "items") {
			// @ATTR items|list(item_id)|List of item id's that is part of the set.
			item_set->items.clear();
			std::string item_str = Parse::popFirstString(infile.val);
			while (!item_str.empty()) {
				ItemID item_id = verifyID(Parse::toItemID(item_str), &infile, !VERIFY_ALLOW_ZERO, !VERIFY_ALLOCATE);
				if (item_id > 0) {
					items[item_id]->set = id;
					item_set->items.push_back(item_id);
				}
				item_str = Parse::popFirstString(infile.val);
			}
		}
		else if (infile.key == "color") {
			// @ATTR color|color|A specific of color for the set.
			item_set->color = Parse::toRGB(infile.val);
		}
		else if (infile.key == "bonus") {
			// @ATTR bonus|repeatable(int, stat_id, list(level_scaled_value)) : Required set item count, Stat ID, Value|Bonus to append to items in the set.
			if (clear_bonus) {
				item_set->bonus.clear();
				clear_bonus = false;
			}
			SetBonusData bonus;
			bonus.requirement = Parse::popFirstInt(infile.val);
			parseBonus(bonus, infile);
			item_set->bonus.push_back(bonus);
		}
		else if (infile.key == "bonus_power_level") {
			// @ATTR bonus_power_level|repeatable(int, power_id, list(level_scaled_value)) : Required set item count, Base power, Bonus levels|Grants bonus levels to a given base power.
			SetBonusData bonus;
			bonus.type = BonusData::POWER_LEVEL;
			bonus.requirement = Parse::popFirstInt(infile.val);
			bonus.power_id = Parse::toPowerID(Parse::popFirstString(infile.val));
			bonus.value.parse(infile.val);
			item_set->bonus.push_back(bonus);
		}
		else {
			infile.error("ItemManager: '%s' is not a valid key.", infile.key.c_str());
		}
	}
	infile.close();

	size_t count_allocated = 0;
	for (size_t i = 0; i < item_sets.size(); ++i) {
		item_set = item_sets[i];

		if (!item_set)
			continue;
		else
			count_allocated++;
	}
	Utils::logInfo("ItemManager: Item Set IDs = %zu reserved / %zu allocated / %zu empty / %zu bytes used", item_sets.size()-1, count_allocated, item_sets.size()-1-count_allocated, (sizeof(ItemSet*) * item_sets.size()) + (sizeof(ItemSet) * count_allocated));
}

void ItemManager::parseBonus(BonusData& bdata, FileParser& infile) {
	std::string bonus_str = Parse::popFirstString(infile.val);
	std::string bonus_value_str = "";

	// Check if this bonus is a multiplier
	for (size_t i = 0; i < infile.val.size(); ++i) {
		if (infile.val[i] == '%') {
			bdata.is_multiplier = true;
		}
		else {
			bonus_value_str += infile.val[i];
		}
	}

	bdata.value.parse(bonus_value_str);

	if (bdata.is_multiplier) {
		bdata.value.base /= 100;
		bdata.value.base_max /= 100;
		bdata.value.base_step /= 100;
		bdata.value.per_item_level /= 100;
		bdata.value.per_item_level_max /= 100;
		bdata.value.per_item_level_step /= 100;
		bdata.value.per_player_level /= 100;
		bdata.value.per_player_level_max /= 100;
		bdata.value.per_player_level_step /= 100;
		for (size_t i = 0; i < bdata.value.per_player_primary.size(); ++i) {
			bdata.value.per_player_primary[i] /= 100;
			bdata.value.per_player_primary_max[i] /= 100;
			bdata.value.per_player_primary_step[i] /= 100;
		}
	}

	if (bonus_str == "speed") {
		bdata.type = BonusData::SPEED;
		return;
	}
	else if (bonus_str == "attack_speed") {
		bdata.type = BonusData::ATTACK_SPEED;
		return;
	}

	// TODO deprecated
	if (bonus_str == "hp_percent") {
		infile.error("ItemManager: 'hp_percent' is deprecated. Converting to 'hp'.");
		bdata.type = BonusData::STAT;
		bdata.index = Stats::HP_MAX;
		bdata.is_multiplier = true;
		bdata.value.base = bdata.value.base_max = (bdata.value.base + 100) / 100; // assuming only base value here
		return;
	}
	else if (bonus_str == "mp_percent") {
		infile.error("ItemManager: 'mp_percent' is deprecated. Converting to 'mp'.");
		bdata.type = BonusData::STAT;
		bdata.index = Stats::MP_MAX;
		bdata.is_multiplier = true;
		bdata.value.base = bdata.value.base_max = (bdata.value.base + 100) / 100; // assuming only base value here
		return;
	}

	for (size_t i = 0; i < Stats::COUNT; ++i) {
		if (bonus_str == Stats::KEY[i]) {
			bdata.type = BonusData::STAT;
			bdata.index = i;
			return;
		}
	}

	for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
		if (bonus_str == eset->damage_types.list[i].min) {
			bdata.type = BonusData::DAMAGE_MIN;
			bdata.index = i;
			return;
		}
		else if (bonus_str == eset->damage_types.list[i].max) {
			bdata.type = BonusData::DAMAGE_MAX;
			bdata.index = i;
			return;
		}
	}

	for (size_t i = 0; i < eset->elements.list.size(); ++i) {
		if (bonus_str == eset->elements.list[i].resist_id) {
			bdata.type = BonusData::RESIST_ELEMENT;
			bdata.index = i;
			return;
		}
	}

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (bonus_str == eset->primary_stats.list[i].id) {
			bdata.type = BonusData::PRIMARY_STAT;
			bdata.index = i;
			if (bdata.is_multiplier) {
				// primary stat bonus can't be multipliers
				bdata.is_multiplier = false;
				infile.error("ItemManager: Primary stat bonus can't be percentage.");
			}
			return;
		}
	}

	for (size_t i = 0; i < eset->resource_stats.list.size(); ++i) {
		for (size_t j = 0; j < EngineSettings::ResourceStats::STAT_COUNT; ++j) {
			if (bonus_str == eset->resource_stats.list[i].ids[j]) {
				bdata.type = BonusData::RESOURCE_STAT;
				bdata.index = i;
				bdata.sub_index = j;
				return;
			}
		}
	}

	infile.error("ItemManager: Unknown bonus type '%s'.", bonus_str.c_str());
}

void ItemManager::getBonusString(std::stringstream& ss, BonusData* bdata) {
	float scaled_bdata_value = bdata->value.get();

	// power level bonuses can only be whole integers
	if (bdata->power_id > 0) {
		scaled_bdata_value = static_cast<float>(static_cast<int>(scaled_bdata_value));
	}

	if (bdata->type == BonusData::SPEED) {
		ss << msg->getv("%s%% Speed", Utils::floatToString(scaled_bdata_value, eset->number_format.item_tooltips).c_str());
		return;
	}
	else if (bdata->type == BonusData::ATTACK_SPEED) {
		ss << msg->getv("%s%% Attack Speed", Utils::floatToString(scaled_bdata_value, eset->number_format.item_tooltips).c_str());
		return;
	}

	if (bdata->is_multiplier)
		ss << Utils::floatToString(scaled_bdata_value, eset->number_format.item_tooltips + 2) << "×";
	else if (scaled_bdata_value > 0)
		ss << "+" << Utils::floatToString(scaled_bdata_value, eset->number_format.item_tooltips);
	else
		ss << Utils::floatToString(scaled_bdata_value, eset->number_format.item_tooltips);

	if (bdata->type == BonusData::STAT) {
		if (!bdata->is_multiplier && Stats::PERCENT[bdata->index])
			ss << "%";

		ss << " " << Stats::NAME[bdata->index];
	}
	else if (bdata->type == BonusData::DAMAGE_MIN) {
		ss << " " << eset->damage_types.list[bdata->index].name_min;
	}
	else if (bdata->type == BonusData::DAMAGE_MAX) {
		ss << " " << eset->damage_types.list[bdata->index].name_max;
	}
	else if (bdata->type == BonusData::RESIST_ELEMENT) {
		if (!bdata->is_multiplier)
			ss << "%";

		ss << " " << msg->getv("Resistance (%s)", eset->elements.list[bdata->index].name.c_str());
	}
	else if (bdata->type == BonusData::PRIMARY_STAT) {
		ss << " " << eset->primary_stats.list[bdata->index].name;
	}
	else if (powers && powers->isValid(bdata->power_id)) {
		ss << " " << powers->powers[bdata->power_id]->name;
		if (menu && menu->pow) {
			std::string req_str = menu->pow->getItemBonusPowerReqString(bdata->power_id);
			if (!req_str.empty())
				ss << " (" << msg->getv("Requires %s", req_str.c_str()) << ")";
		}
	}
	else if (bdata->type == BonusData::RESOURCE_STAT) {
		ss << " " << eset->resource_stats.list[bdata->index].text[bdata->sub_index];
	}
}

void ItemManager::playSound(ItemID item, const Point& pos) {
	if (!isValid(item))
		return;

	std::stringstream channel_name;
	channel_name << "item_" << items[item]->sfx_id;
	snd->play(items[item]->sfx_id, channel_name.str(), FPoint(pos), false);
}

TooltipData ItemManager::getShortTooltip(ItemStack stack) {
	std::stringstream ss;
	TooltipData tip;

	if (stack.empty() || !isValid(stack.item)) return tip;

	// name
	if (stack.quantity > 1) {
		ss << getItemName(stack.item) << " (" << stack.quantity << ")";
	}
	else {
		ss << getItemName(stack.item);
	}
	tip.addColoredText(ss.str(), getItemColor(stack.item));

	return tip;
}

/**
 * Create detailed tooltip showing all relevant item info
 */
TooltipData ItemManager::getTooltip(ItemStack stack, StatBlock *stats, int context, bool input_hint) {
	TooltipData tip;

	if (stack.empty() || !isValid(stack.item))
		return tip;

	Item* item = items[stack.item];

	Color color = getItemColor(stack.item);

	// name
	std::stringstream ss;
	if (stack.quantity == 1)
		ss << getItemName(stack.item);
	else
		ss << getItemName(stack.item) << " (" << stack.quantity << ")";
	tip.addColoredText(ss.str(), color);

	// quest item
	if (item->quest_item) {
		tip.addColoredText(msg->get("Quest Item"), font->getColor(FontEngine::COLOR_ITEM_BONUS));
	}

	// only show the name of the currency item
	if (stack.item == eset->misc.currency_id) {
		if (input_hint)
			getTooltipInputHint(tip, stack, context);
		return tip;
	}

	// flavor text
	if (!item->flavor.empty()) {
		tip.addColoredText(Utils::substituteVarsInString(item->flavor, pc), font->getColor(FontEngine::COLOR_ITEM_FLAVOR));
	}

	// level
	if (item->level != 0) {
		tip.addText(msg->getv("Level %d", item->level));
	}

	// type
	if (!item->type.empty()) {
		tip.addText(msg->get(getItemType(item->type)));
	}

	// item quality text for colorblind users
	if (settings->colorblind && !item->quality.empty()) {
		color = font->getColor(FontEngine::COLOR_WIDGET_NORMAL);
		for (size_t i=0; i<item_qualities.size(); ++i) {
			if (item_qualities[i].id == item->quality) {
				tip.addColoredText(msg->getv("Quality: %s", msg->get(item_qualities[i].name).c_str()), color);
				break;
			}
		}
	}

	// damage
	for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
		if (item->base_dmg[i].max > 0) {
			std::stringstream dmg_str;
			dmg_str << eset->damage_types.list[i].name;
			dmg_str << ": " << Utils::createMinMaxString(item->base_dmg[i].min, item->base_dmg[i].max, eset->number_format.item_tooltips);
			tip.addText(dmg_str.str());
		}
	}

	// absorb
	if (item->base_abs.max > 0) {
		std::stringstream abs_str;
		abs_str << msg->get("Absorb");
		abs_str << ": " << Utils::createMinMaxString(item->base_abs.min, item->base_abs.max, eset->number_format.item_tooltips);
		tip.addText(abs_str.str());
	}

	// bonuses
	unsigned bonus_counter = 0;
	while (bonus_counter < item->bonus.size()) {
		ss.str("");

		BonusData* bdata = &item->bonus[bonus_counter];

		float scaled_bdata_value = bdata->value.get();

		if (bdata->type == BonusData::SPEED || bdata->type == BonusData::ATTACK_SPEED) {
			if (scaled_bdata_value >= 100)
				color = font->getColor(FontEngine::COLOR_ITEM_BONUS);
			else
				color = font->getColor(FontEngine::COLOR_ITEM_PENALTY);
		}
		else if (bdata->is_multiplier) {
			if (scaled_bdata_value >= 1)
				color = font->getColor(FontEngine::COLOR_ITEM_BONUS);
			else
				color = font->getColor(FontEngine::COLOR_ITEM_PENALTY);
		}
		else {
			if (scaled_bdata_value > 0)
				color = font->getColor(FontEngine::COLOR_ITEM_BONUS);
			else
				color = font->getColor(FontEngine::COLOR_ITEM_PENALTY);
		}

		getBonusString(ss, bdata);
		tip.addColoredText(ss.str(), color);
		bonus_counter++;
	}

	// power
	if (!item->power_desc.empty()) {
		tip.addColoredText(item->power_desc, font->getColor(FontEngine::COLOR_ITEM_BONUS));
	}

	// level requirement
	int scaled_requires_level = static_cast<int>(item->requires_level.get());
	if (scaled_requires_level > 0) {
		if (stats->level < scaled_requires_level)
			color = font->getColor(FontEngine::COLOR_REQUIREMENTS_NOT_MET);
		else
			color = font->getColor(FontEngine::COLOR_WIDGET_NORMAL);

		tip.addColoredText(msg->getv("Requires Level %d", scaled_requires_level), color);
	}

	// base stat requirement
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		int scaled_requires_primary = static_cast<int>(item->requires_stat[i].get());
		if (scaled_requires_primary > 0) {
			if (stats->get_primary(i) < scaled_requires_primary)
				color = font->getColor(FontEngine::COLOR_REQUIREMENTS_NOT_MET);
			else
				color = font->getColor(FontEngine::COLOR_WIDGET_NORMAL);

			tip.addColoredText(msg->getv("Requires %s %d", eset->primary_stats.list[i].name.c_str(), scaled_requires_primary), color);
		}
	}

	// requires class
	if (!item->requires_class.empty()) {
		if (item->requires_class != stats->character_class)
			color = font->getColor(FontEngine::COLOR_REQUIREMENTS_NOT_MET);
		else
			color = font->getColor(FontEngine::COLOR_WIDGET_NORMAL);

		tip.addColoredText(msg->getv("Requires Class: %s", msg->get(item->requires_class).c_str()), color);
	}

	// buy or sell price
	if (item->getPrice(USE_VENDOR_RATIO) > 0 && stack.item != eset->misc.currency_id) {
		Color currency_color = getItemColor(eset->misc.currency_id);

		int price_per_unit;
		if (context == VENDOR_BUY) {
			price_per_unit = item->getPrice(USE_VENDOR_RATIO);
			if (stats->currency < price_per_unit)
				color = font->getColor(FontEngine::COLOR_REQUIREMENTS_NOT_MET);
			else
				color = currency_color;

			if (item->max_quantity <= 1)
				tip.addColoredText(msg->getv("Buy Price: %d %s", price_per_unit, eset->loot.currency.c_str()), color);
			else
				tip.addColoredText(msg->getv("Buy Price: %d %s each", price_per_unit, eset->loot.currency.c_str()), color);
		}
		else if (context == VENDOR_SELL) {
			price_per_unit = item->getSellPrice(stack.can_buyback);
			if (stats->currency < price_per_unit)
				color = font->getColor(FontEngine::COLOR_REQUIREMENTS_NOT_MET);
			else
				color = currency_color;

			if (item->max_quantity <= 1)
				tip.addColoredText(msg->getv("Buy Price: %d %s", price_per_unit, eset->loot.currency.c_str()), color);
			else
				tip.addColoredText(msg->getv("Buy Price: %d %s each", price_per_unit, eset->loot.currency.c_str()), color);
		}
		else if (context == PLAYER_INV) {
			price_per_unit = item->getSellPrice(DEFAULT_SELL_PRICE);
			if (price_per_unit == 0)
				price_per_unit = 1;

			if (item->max_quantity <= 1)
				tip.addColoredText(msg->getv("Sell Price: %d %s", price_per_unit, eset->loot.currency.c_str()), currency_color);
			else
				tip.addColoredText(msg->getv("Sell Price: %d %s each", price_per_unit, eset->loot.currency.c_str()), currency_color);
		}
	}

	if (item->set > 0) {
		int set_count = menu->inv->getEquippedSetCount(item->set);

		// item set bonuses
		ItemSet* item_set = item_sets[item->set];
		bonus_counter = 0;

		tip.addColoredText("\n" + msg->get("Set:") + ' ' + msg->get(item_set->name), item_set->color);

		while (bonus_counter < item_set->bonus.size()) {
			ss.str("");

			SetBonusData* bdata = &(item_set->bonus[bonus_counter]);

			ss << msg->getv("%d items:", bdata->requirement) << ' ';

			getBonusString(ss, bdata);
			if (bdata->requirement <= set_count)
				tip.addColoredText(ss.str(), item_set->color);
			else
				tip.addColoredText(ss.str(), font->getColor(FontEngine::COLOR_WIDGET_DISABLED));
			bonus_counter++;
		}
	}

	if (input_hint)
		getTooltipInputHint(tip, stack, context);

	return tip;
}

void ItemManager::getTooltipInputHint(TooltipData& tip, ItemStack stack, int context) {
	bool show_activate_msg = false;
	std::string activate_bind_str;

	bool show_more_msg = false;
	std::string more_bind_str;

	if (inpt->mode == InputState::MODE_TOUCHSCREEN) {
		tip.addColoredText('\n' + msg->get("Tap icon again for more options"), font->getColor(FontEngine::COLOR_ITEM_BONUS));
	}
	else if (inpt->mode == InputState::MODE_JOYSTICK) {
		if (context == PLAYER_INV && menu->inv->canActivateItem(stack.item)) {
			show_activate_msg = true;
			activate_bind_str = inpt->getGamepadBindingString(Input::MENU_ACTIVATE);
		}
		show_more_msg = true;
		more_bind_str = inpt->getGamepadBindingString(Input::ACCEPT);
	}
	else if (!inpt->usingMouse()) {
		if (context == PLAYER_INV && menu->inv->canActivateItem(stack.item)) {
			show_activate_msg = true;
			activate_bind_str = inpt->getBindingString(Input::MENU_ACTIVATE);
		}
		show_more_msg = true;
		more_bind_str = inpt->getBindingString(Input::ACCEPT);
	}
	else {
		if (context == PLAYER_INV && menu->inv->canActivateItem(stack.item)) {
			show_activate_msg = true;
			activate_bind_str = inpt->getBindingString(Input::MAIN2);
		}
	}

	if (show_activate_msg || show_more_msg) {
		tip.addText("");
	}

	// input hint for consumables/books
	if (show_activate_msg) {
		if (items[stack.item] && !items[stack.item]->book.empty() && items[stack.item]->book_is_readable) {
			tip.addColoredText(msg->getv("Press [%s] to read", activate_bind_str.c_str()), font->getColor(FontEngine::COLOR_ITEM_BONUS));
		}
		else if (menu->inv->canActivateItem(stack.item)) {
			tip.addColoredText(msg->getv("Press [%s] to use", activate_bind_str.c_str()), font->getColor(FontEngine::COLOR_ITEM_BONUS));
		}
	}
	if (show_more_msg) {
		tip.addColoredText(msg->getv("Press [%s] for more options", more_bind_str.c_str()), font->getColor(FontEngine::COLOR_ITEM_BONUS));
	}
}

/**
 * Check requirements on an item
 */
bool ItemManager::requirementsMet(const StatBlock *stats, ItemID item_id) {
	if (!stats || !isValid(item_id))
		return false;

	Item* item = items[item_id];

	// level
	int scaled_requires_level = static_cast<int>(item->requires_level.get());
	if (scaled_requires_level > 0 && stats->level < scaled_requires_level) {
		return false;
	}

	// base stats
	for (size_t i = 0; i < eset->primary_stats.list.size(); i++) {
		if (stats->get_primary(i) < static_cast<int>(item->requires_stat[i].get()))
			return false;
	}

	// class
	if (!item->requires_class.empty() && item->requires_class != stats->character_class) {
		return false;
	}

	// otherwise there is no requirement, so it is usable.
	return true;
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

/**
 * Check if an item stack is empty and provide some error checking
 */
bool ItemStack::empty() {
	if (item != 0 && quantity > 0) {
		return false;
	}
	else if (item == 0 && quantity != 0) {
		Utils::logError("ItemStack: Item id is zero, but quantity is %d.", quantity);
		clear();
	}
	else if (item != 0 && quantity == 0) {
		Utils::logError("ItemStack: Item id is %d, but quantity is zero.", item);
		clear();
	}
	return true;
}

void ItemStack::clear() {
	item = 0;
	quantity = 0;
	can_buyback = false;
}

int Item::getPrice(bool use_vendor_ratio) {
	int new_price = static_cast<int>(price.get());
	if (new_price == 0)
		return new_price;

	new_price = static_cast<int>(static_cast<float>(new_price) * eset->loot.vendor_ratio_buy);

	NPC* vendor_npc = ((menu && menu->vendor && menu->vendor->visible) ? menu->vendor->npc : NULL);

	if (use_vendor_ratio) {
		// get vendor ratio from NPC (or fall back to global value)
		float vendor_ratio_buy = (vendor_npc && vendor_npc->vendor_ratio_buy > 0) ? vendor_npc->vendor_ratio_buy : eset->loot.vendor_ratio_buy;

		new_price = static_cast<int>(static_cast<float>(new_price) * vendor_ratio_buy);
	}

	return std::max(new_price, 1);
}

int Item::getSellPrice(bool is_new_buyback) {
	int new_price = 0;
	NPC* vendor_npc = ((menu && menu->vendor && menu->vendor->visible) ? menu->vendor->npc : NULL);

	// get vendor ratio from NPC (or fall back to global value)
	float vendor_ratio_sell = (vendor_npc && vendor_npc->vendor_ratio_sell > 0) ? vendor_npc->vendor_ratio_sell : eset->loot.vendor_ratio_sell;
	float vendor_ratio_sell_old = (vendor_npc && vendor_npc->vendor_ratio_sell_old > 0) ? vendor_npc->vendor_ratio_sell_old : eset->loot.vendor_ratio_sell_old;

	if (is_new_buyback || vendor_ratio_sell_old == 0) {
		// default sell price
		int scaled_price_sell = static_cast<int>(price_sell.get());
		if (scaled_price_sell != 0)
			new_price = scaled_price_sell;
		else
			new_price = static_cast<int>(static_cast<float>(getPrice(!ItemManager::USE_VENDOR_RATIO)) * vendor_ratio_sell);
	}
	else {
		// sell price adjusted because the player can no longer buyback the item at the original sell price
		new_price = static_cast<int>(static_cast<float>(getPrice(!ItemManager::USE_VENDOR_RATIO)) * vendor_ratio_sell_old);
	}

	return std::max(new_price, 1);
}

ItemID ItemManager::verifyID(ItemID item_id, FileParser* infile, bool allow_zero, bool allocate) {
	if ((!allow_zero && item_id == 0) || item_id >= items.size() || (item_id > 0 && !items[item_id])) {
		if (infile != NULL)
			infile->error("ItemManager: %zu is not a valid item id.", item_id);
		else
			Utils::logError("ItemManager: %zu is not a valid item id.", item_id);

		if (item_id > 0 && allocate) {
			if (item_id >= items.size()) {
				items.resize(item_id+1, NULL);
			}
			items[item_id] = new Item();
			Utils::logInfo("ItemManager: Allocated unknown item, %zu.", item_id);
			return item_id;
		}

		return 0;
	}
	return item_id;
}

ItemRandomizerDef* ItemManager::loadRandomizerDef(const std::string& filename) {
	for (size_t i = 0; i < randomizer_defs.size(); ++i) {
		if (randomizer_defs[i]->filename == filename)
			return randomizer_defs[i];
	}

	FileParser infile;

	// @CLASS ItemManager: Randomizer Definition|Description of item randomizer configuration in items/random/...
	if (!infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return NULL;

	randomizer_defs.resize(randomizer_defs.size()+1, new ItemRandomizerDef());
	ItemRandomizerDef* ird = randomizer_defs.back();
	ird->filename = filename;

	ItemRandomizerDef::Option* option = NULL;

	while (infile.next()) {
		if (infile.section == "option") {
			if (infile.new_section) {
				ird->option.resize(ird->option.size()+1);
				option = &(ird->option.back());
			}
			if (option) {
				if (infile.key == "chance") {
					// @ATTR option.chance|float|Percent chance that this option will be picked when generating an item.
					option->chance = Parse::toFloat(infile.val);
				}
				else if (infile.key == "quality") {
					// @ATTR option.quality|predefined_string|The quality ID that will be assigned to the item.
					option->quality = infile.val;
				}
				else if (infile.key == "bonus_count") {
					// @ATTR option.bonus_count|int, int : Minimum count, Maximum count|The range used to determine the number of bonuses for the item.
					option->bonus_min = Parse::popFirstInt(infile.val);
					option->bonus_max = Parse::popFirstInt(infile.val);
					option->bonus_max = std::max(option->bonus_min, option->bonus_max);
				}
				else if (infile.key == "level_src") {
					// @ATTR option.level_src|["base", "hero"]|Determines how option.level_range will be applied. If "base", the source item's level will be used. If "hero", the player's current level will be used.
					if (infile.val == "base")
						option->level_src = ItemRandomizerDef::Option::LEVEL_SRC_BASE;
					else if (infile.val == "hero")
						option->level_src = ItemRandomizerDef::Option::LEVEL_SRC_HERO;
				}
				else if (infile.key == "level_range") {
					// @ATTR option.level_range|int, int : Minimum level, Maximum level|The range used to determine the item level. The resulting item level is relative, depending on the value of option.level_src.
					option->level_range_min = Parse::popFirstInt(infile.val);
					option->level_range_max = Parse::popFirstInt(infile.val);
				}
			}
		}
		else if (infile.section == "bonuses") {
			if (infile.key == "bonus") {
				// @ATTR bonuses.bonus|repeatable(stat_id, list(level_scaled_value)) : Stat ID, Value|Adds a bonus to the item by stat ID. The level_scaled_value here is extended for randomization to also support the maximum value and the step. These additional values are delimited by colons. Example: bonus=hp,base:50:100:1
				BonusData bdata;
				parseBonus(bdata, infile);
				ird->bonus.push_back(bdata);
			}
			else if (infile.key == "bonus_power_level") {
				// @ATTR bonuses.bonus_power_level|repeatable(power_id, list(level_scaled_value)) : Base power, Bonus levels|Grants bonus levels to a given base power. The level_scaled_value here is extended for randomization to also support the maximum value and the step. These additional values are delimited by colons. Example: bonus=1,base:1:5:1
				BonusData bdata;
				bdata.type = BonusData::POWER_LEVEL;
				bdata.power_id = Parse::toPowerID(Parse::popFirstString(infile.val));
				bdata.value.parse(infile.val);
				ird->bonus.push_back(bdata);
			}
		}
	}
	infile.close();

	return ird;
}

ItemID ItemManager::allocateExtendedItem(ItemID item_id, ItemID parent_id) {
	size_t start_id = eset->loot.extended_items_offset;
	if (item_id != 0)
		start_id = std::max(start_id, item_id);

	if (items.size() <= start_id) {
		items.resize(start_id + 1, NULL);
	}

	ItemID extended_item = 0;
	if (item_id == 0) {
		for (size_t i = start_id; i < items.size(); ++i) {
			if (!items[i]) {
				extended_item = i;
				break;
			}
		}
		if (extended_item == 0) {
			// didn't find an empty slot, resize items
			extended_item = items.size();
			items.resize(extended_item + 1);
		}
	}
	else {
		extended_item = item_id;
	}

	items[extended_item] = new Item();
	*(items[extended_item]) = *(items[parent_id]); // TODO explicit copy constructor for Item?
	items[extended_item]->parent = parent_id;

	return extended_item;
}

ItemID ItemManager::getExtendedItem(ItemID item_id) {
	if (item_id >= items.size() || items[item_id] == NULL) {
		return item_id;
	}

	if (items[item_id]->randomizer_def) {
		ItemID extended_item = allocateExtendedItem(0, item_id);

		ItemRandomizerDef* ird = items[item_id]->randomizer_def;

		// pick an option
		std::vector<size_t> option_ids;
		float option_chance = Math::randBetweenF(0, 100);
		float option_threshold = 100;
		size_t bonus_count = 0;

		for (size_t i = 0; i < ird->option.size(); ++i) {
			if (ird->option[i].chance >= option_chance) {
				if (option_chance <= option_threshold) {
					if (option_chance != option_threshold) {
						option_ids.clear();
					}

					option_threshold = option_chance;
				}
			}

			if (option_chance <= option_threshold) {
				option_ids.push_back(i);
			}
		}
		if (!option_ids.empty()) {
			size_t option_roll = static_cast<size_t>(rand()) % option_ids.size();
			ItemRandomizerDef::Option* option = &(ird->option[option_roll]);
			bonus_count = static_cast<size_t>(Math::randBetween(option->bonus_min, option->bonus_max));
			if (!option->quality.empty())
				items[extended_item]->quality = option->quality;

			// set the item level
			if (option->level_src == ItemRandomizerDef::Option::LEVEL_SRC_BASE) {
				int min = items[item_id]->level + option->level_range_min;
				int max = items[item_id]->level + option->level_range_max;

				items[extended_item]->level = Math::randBetween(min, max);
			}
			else if (option->level_src == ItemRandomizerDef::Option::LEVEL_SRC_HERO) {
				int min = pc->stats.level + option->level_range_min;
				int max = pc->stats.level + option->level_range_max;

				items[extended_item]->level = Math::randBetween(min, max);
			}

		}

		// we set the item level when adding bonuses,
		// so we can run updateLevelScaling() before that to avoid iterating over the bonuses twice
		items[extended_item]->updateLevelScaling();

		bonus_count = std::min(bonus_count, ird->bonus.size());

		std::vector<size_t> bonus_ids(ird->bonus.size(), 0);
		for (size_t i = 0; i < bonus_ids.size(); ++i) {
			bonus_ids[i] = i;
		}

		for (size_t i = 0; i < bonus_count; ++i) {
			size_t roll = static_cast<size_t>(Math::randBetween(0, static_cast<int>(bonus_ids.size()-1)));
			size_t bonus_id = bonus_ids[roll];

			BonusData bdata = ird->bonus[bonus_id];
			bdata.is_extended = true;
			bdata.value.item_level = items[extended_item]->level;
			bdata.value.randomize();

			items[extended_item]->bonus.push_back(bdata);

			bonus_ids.erase(bonus_ids.begin()+roll);
		}

		items[extended_item]->randomizer_def = NULL;

		return extended_item;
	}
	else {
		return item_id;
	}
}

void ItemManager::loadExtendedItems(const std::string& filename) {
	FileParser infile;

	if (!infile.open(filename, !FileParser::MOD_FILE, FileParser::ERROR_NONE))
		return;

	ItemID id = 0;
	Item* item;
	bool id_line;
	while (infile.next()) {
		if (infile.key == "id") {
			id_line = true;

			ItemID item_id = Parse::toItemID(Parse::popFirstString(infile.val));

			if (item_id < items.size() && items[item_id]) {
				infile.error("ItemManager: Existing item with ID %zu found when loading extended items. Skipping.", item_id);
				continue;
			}

			ItemID parent_id = verifyID(Parse::toItemID(Parse::popFirstString(infile.val)), &infile, !VERIFY_ALLOW_ZERO, !VERIFY_ALLOCATE);

			if (parent_id == 0)
				continue;

			id = allocateExtendedItem(item_id, parent_id);
			item = items[item_id];
		}
		else {
			id_line = false;
		}
		if (id < 1) {
			if (id_line) infile.error("ItemManager: Item index out of bounds 1-%d, skipping set.", INT_MAX);
			continue;
		}
		if (id_line) continue;

		if (infile.key == "level") {
			item->level = Parse::toInt(infile.val);
		}
		else if (infile.key == "quality") {
			item->quality = infile.val;
		}
		else if (infile.key == "bonus") {
			BonusData bdata;
			bdata.is_extended = true;
			parseBonus(bdata, infile);
			item->bonus.push_back(bdata);
		}
		else if (infile.key == "bonus_power_level") {
			BonusData bdata;
			bdata.is_extended = true;
			bdata.type = BonusData::POWER_LEVEL;
			bdata.power_id = Parse::toPowerID(Parse::popFirstString(infile.val));
			bdata.value.parse(infile.val);
			item->bonus.push_back(bdata);
		}
	}
	infile.close();

	size_t count_allocated = 0;

	for (size_t i = eset->loot.extended_items_offset; i < items.size(); ++i) {
		item = items[i];

		if (!item)
			continue;
		else
			count_allocated++;

		item->updateLevelScaling();
	}
	size_t extended_item_count = items.size() - eset->loot.extended_items_offset;
	Utils::logInfo("ItemManager: Extended Item IDs = %zu reserved / %zu allocated / %zu empty / %zu bytes used", extended_item_count, count_allocated, extended_item_count-count_allocated, (sizeof(Item*) * extended_item_count) + (sizeof(Item) * count_allocated));
}

void Item::updateLevelScaling() {
	// set item_level for level-scaled values
	requires_level.item_level = level;
	price.item_level = level;
	price_sell.item_level = level;

	for (size_t i = 0; i < requires_stat.size(); ++i) {
		requires_stat[i].item_level = level;
	}

	for (size_t i = 0; i < bonus.size(); ++i) {
		bonus[i].value.item_level = level;
	}
}

// Bonus documentation

// @CLASS ItemManager|Description of "bonus" attribute in items/items.txt
// @TYPE speed|Movement speed. A value of 100 is 100% speed (aka normal speed).
// @TYPE attack_speed|Attack animation speed. A value of 100 is 100% speed (aka normal speed).
// @TYPE ${STAT}|Increases ${STAT}, where ${STAT} is any valid stat_id.
// @TYPE ${PRIMARYSTAT}|Increases ${PRIMARYSTAT}, where ${PRIMARYSTAT} is any of the primary stats defined in engine/primary_stats.txt. Example: physical
