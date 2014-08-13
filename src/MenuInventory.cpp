/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson
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
 * class MenuInventory
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "Menu.h"
#include "MenuInventory.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"

using namespace std;

MenuInventory::MenuInventory(StatBlock *_stats) {
	stats = _stats;
	MAX_EQUIPPED = 4;
	MAX_CARRIED = 64;
	visible = false;

	setBackground("images/menus/inventory.png");

	currency = 0;

	carried_cols = 4; // default to 4 if menus/inventory.txt::carried_cols not set
	carried_rows = 4; // default to 4 if menus/inventory.txt::carried_rows not set

	drag_prev_src = -1;
	changed_equipment = true;
	changed_artifact = true;
	log_msg = "";
	show_book = "";

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Load config settings
	Rect equipment_slot;
	FileParser infile;
	// @CLASS MenuInventory|Description of menus/inventory.txt
	if (infile.open("menus/inventory.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|x (integer), y (integer)|Position of the close button.
			if(infile.key == "close") close_pos = toPoint(infile.val);
			// @ATTR equipment_slot|x (integer), y (integer), size (integer), slot_type (string)|Position and item type of an equipment slot.
			else if(infile.key == "equipment_slot") {
				equipment_slot.x = popFirstInt(infile.val);
				equipment_slot.y = popFirstInt(infile.val);
				equipment_slot.w = equipment_slot.h = popFirstInt(infile.val);
				equipped_area.push_back(equipment_slot);
				slot_type.push_back(popFirstString(infile.val));
			}
			// @ATTR slot_name|string|The displayed name of the last defined equipment slot.
			else if(infile.key == "slot_name") slot_desc.push_back(infile.val);
			// @ATTR carried_area|x (integer), y (integer)|Position of the first normal inventory slot.
			else if(infile.key == "carried_area") {
				carried_area.x = popFirstInt(infile.val);
				carried_area.y = popFirstInt(infile.val);
			}
			// @ATTR carried_cols|integer|The number of columns for the normal inventory.
			else if (infile.key == "carried_cols") carried_cols = max(1, toInt(infile.val));
			// @ATTR carried_rows|integer|The number of rows for the normal inventory.
			else if (infile.key == "carried_rows") carried_rows = max(1, toInt(infile.val));
			// @ATTR label_title|label|Position of the "Inventory" label.
			else if (infile.key == "label_title") title =  eatLabelInfo(infile.val);
			// @ATTR currency|label|Position of the label that displays the total currency being carried.
			else if (infile.key == "currency") currency_lbl =  eatLabelInfo(infile.val);
			// @ATTR help|x (integer), y (integer), w (integer), h (integer)|A mouse-over area that displays some help text for inventory shortcuts.
			else if (infile.key == "help") help_pos = toRect(infile.val);
		}
		infile.close();
	}

	MAX_EQUIPPED = equipped_area.size();
	MAX_CARRIED = carried_cols * carried_rows;

	color_normal = font->getColor("menu_normal");
	color_high = font->getColor("menu_bonus");

	align();
	alignElements();
}

void MenuInventory::alignElements() {
	for (int i=0; i<MAX_EQUIPPED; i++) {
		equipped_area[i].x += window_area.x;
		equipped_area[i].y += window_area.y;
	}

	carried_area.x += window_area.x;
	carried_area.y += window_area.y;
	carried_area.w = carried_cols*ICON_SIZE;
	carried_area.h = carried_rows*ICON_SIZE;

	inventory[EQUIPMENT].init(MAX_EQUIPPED, equipped_area, slot_type);
	inventory[CARRIED].init(MAX_CARRIED, carried_area, ICON_SIZE, carried_cols);

	closeButton->pos.x = window_area.x+close_pos.x;
	closeButton->pos.y = window_area.y+close_pos.y;

	label_inventory.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Inventory"), color_normal, title.font_style);

	for (int i = 0; i < MAX_EQUIPPED; i++) {
		tablist.add(inventory[EQUIPMENT].slots[i]);
	}
	for (int i = 0; i < MAX_CARRIED; i++) {
		tablist.add(inventory[CARRIED].slots[i]);
	}
}

void MenuInventory::logic() {

	// if the player has just died, the penalty is half his current currency.
	if (stats->death_penalty && DEATH_PENALTY) {
		std::string death_message = "";

		// remove a % of currency
		if (DEATH_PENALTY_CURRENCY > 0) {
			if (currency > 0)
				removeCurrency((currency * DEATH_PENALTY_CURRENCY) / 100);
			death_message += msg->get("Lost %d%% of %s. ", DEATH_PENALTY_CURRENCY, CURRENCY);
		}

		// remove a % of either total xp or xp since the last level
		if (DEATH_PENALTY_XP > 0) {
			if (stats->xp > 0)
				stats->xp -= (stats->xp * DEATH_PENALTY_XP) / 100;
			death_message += msg->get("Lost %d%% of total XP. ", DEATH_PENALTY_XP);
		}
		else if (DEATH_PENALTY_XP_CURRENT > 0) {
			if (stats->xp - stats->xp_table[stats->level-1] > 0)
				stats->xp -= ((stats->xp - stats->xp_table[stats->level-1]) * DEATH_PENALTY_XP_CURRENT) / 100;
			death_message += msg->get("Lost %d%% of current level XP. ", DEATH_PENALTY_XP_CURRENT);
		}

		// prevent down-leveling from removing too much xp
		if (stats->xp < stats->xp_table[stats->level-1])
			stats->xp = stats->xp_table[stats->level-1];

		// remove a random carried item
		if (DEATH_PENALTY_ITEM) {
			std::vector<int> removable_items;
			removable_items.clear();
			for (int i=0; i < MAX_EQUIPPED; i++) {
				if (inventory[EQUIPMENT][i].item > 0) {
					if (items->items[inventory[EQUIPMENT][i].item].type != "quest")
						removable_items.push_back(inventory[EQUIPMENT][i].item);
				}
			}
			for (int i=0; i < MAX_CARRIED; i++) {
				if (inventory[CARRIED][i].item > 0) {
					if (items->items[inventory[CARRIED][i].item].type != "quest")
						removable_items.push_back(inventory[CARRIED][i].item);
				}
			}
			if (!removable_items.empty()) {
				int random_item = rand() % removable_items.size();
				remove(removable_items[random_item]);
				death_message += msg->get("Lost %s.",items->items[removable_items[random_item]].name);
			}
		}

		log_msg = death_message;

		stats->death_penalty = false;
	}

	// a copy of currency is kept in stats, to help with various situations
	stats->currency = currency = getCurrency();

	// check close button
	if (visible) {
		if (NO_MOUSE) {
			tablist.logic();
		}
		if (closeButton->checkClick()) {
			visible = false;
			snd->play(sfx_close);
		}
		if (drag_prev_src == -1) {
			clearHighlight();
		}
	}
}

void MenuInventory::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// text overlay
	if (!title.hidden) label_inventory.render();

	if (!currency_lbl.hidden) {
		label_currency.set(window_area.x+currency_lbl.x, window_area.y+currency_lbl.y, currency_lbl.justify, currency_lbl.valign, msg->get("%d %s", currency, CURRENCY), color_normal, currency_lbl.font_style);
		label_currency.render();
	}

	inventory[EQUIPMENT].render();
	inventory[CARRIED].render();
}

int MenuInventory::areaOver(Point position) {
	if (isWithin(carried_area, position)) {
		return CARRIED;
	}
	else {
		for (unsigned int i=0; i<equipped_area.size(); i++) {
			if (isWithin(equipped_area[i], position)) {
				return EQUIPMENT;
			}
		}
	}

	// point is inside the inventory menu, but not over a slot
	if (isWithin(window_area, position)) {
		return INV_WINDOW;
	}

	return -2;
}

/**
 * If mousing-over an item with a tooltip, return that tooltip data.
 *
 * @param mouse The x,y screen coordinates of the mouse cursor
 */
TooltipData MenuInventory::checkTooltip(Point position) {
	int area;
	int slot;
	TooltipData tip;

	area = areaOver(position);
	if (area < 0) {
		if (position.x >= window_area.x + help_pos.x && position.y >= window_area.y+help_pos.y && position.x < window_area.x+help_pos.x+help_pos.w && position.y < window_area.y+help_pos.y+help_pos.h) {
			tip.addText(msg->get("Use SHIFT to move only one item."));
			tip.addText(msg->get("CTRL-click a carried item to sell it."));
		}
		return tip;
	}
	slot = inventory[area].slotOver(position);

	if (slot == -1)
		return tip;

	if (inventory[area][slot].item > 0) {
		tip = inventory[area].checkTooltip(position, stats, PLAYER_INV);
	}
	else if (area == EQUIPMENT && inventory[area][slot].item == 0) {
		tip.addText(msg->get(slot_desc[slot]));
	}

	return tip;
}

/**
 * Click-start dragging in the inventory
 */
ItemStack MenuInventory::click(Point position) {
	ItemStack item;
	item.item = 0;
	item.quantity = 0;

	drag_prev_src = areaOver(position);
	if (drag_prev_src > -1) {
		item = inventory[drag_prev_src].click(position);
		// if dragging equipment, prepare to change stats/sprites
		if (drag_prev_src == EQUIPMENT) {
			if (stats->humanoid) {
				updateEquipment(inventory[EQUIPMENT].drag_prev_slot);
			}
			else {
				itemReturn(item);
				item.item = 0;
				item.quantity = 0;
			}
		}
	}

	return item;
}

/**
 * Return dragged item to previous slot
 */
void MenuInventory::itemReturn(ItemStack stack) {
	if (drag_prev_src == -1)
		return;
	inventory[drag_prev_src].itemReturn(stack);
	// if returning equipment, prepare to change stats/sprites
	if (drag_prev_src == EQUIPMENT) {
		updateEquipment(inventory[EQUIPMENT].drag_prev_slot);
	}
	drag_prev_src = -1;
}

/**
 * Dragging and dropping an item can be used to rearrange the inventory
 * and equip items
 */
void MenuInventory::drop(Point position, ItemStack stack) {
	items->playSound(stack.item);

	int area = areaOver(position);
	if (area < 0) {
		// not dropped into a slot. Just return it to the previous slot.
		itemReturn(stack);
		return;
	}

	int slot = inventory[area].slotOver(position);
	if (slot == -1) {
		// not dropped into a slot. Just return it to the previous slot.
		itemReturn(stack);
		return;
	}

	int drag_prev_slot = -1;
	if (drag_prev_src != -1)
		drag_prev_slot = inventory[drag_prev_src].drag_prev_slot;

	if (area == EQUIPMENT) { // dropped onto equipped item

		// make sure the item is going to the correct slot
		// we match slot_type to stack.item's type to place items in the proper slots
		// also check to see if the hero meets the requirements
		if (drag_prev_src == CARRIED && slot_type[slot] == items->items[stack.item].type && requirementsMet(stack.item) && stats->humanoid && inventory[EQUIPMENT].slots[slot]->enabled) {
			if (inventory[area][slot].item == stack.item) {
				// Merge the stacks
				add(stack, area, slot, false);
			}
			else if (drag_prev_slot != -1 && inventory[drag_prev_src][drag_prev_slot].item == 0) {
				// Swap the two stacks
				itemReturn(inventory[area][slot]);
				inventory[area][slot] = stack;
				updateEquipment(slot);
			}
			else {
				itemReturn(stack);
			}
		}
		else {
			// equippable items only belong to one slot, for the moment
			itemReturn(stack); // cancel
			updateEquipment(slot);
		}
	}
	else if (area == CARRIED) {
		// dropped onto carried item

		if (drag_prev_src == CARRIED) {
			if (slot != drag_prev_slot) {
				if (inventory[area][slot].item == stack.item) {
					// Merge the stacks
					add(stack, area, slot, false);
				}
				else if (inventory[area][slot].item == 0) {
					// Drop the stack
					inventory[area][slot] = stack;
				}
				else if (drag_prev_slot != -1 && inventory[drag_prev_src][drag_prev_slot].item == 0) {
					// Check if the previous slot is free (could still be used if SHIFT was used).
					// Swap the two stacks
					itemReturn( inventory[area][slot]);
					inventory[area][slot] = stack;
				}
				else {
					itemReturn( stack);
				}
			}
			else {
				itemReturn( stack); // cancel
			}
		}
		else {
			// note: equipment slots 0-3 correspond with item types 0-3
			// also check to see if the hero meets the requirements
			if (inventory[area][slot].item == stack.item || drag_prev_src == -1) {
				// Merge the stacks
				add(stack, area, slot, false);
			}
			else if (inventory[area][slot].item == 0) {
				// Drop the stack
				inventory[area][slot] = stack;
			}
			else if(
				inventory[EQUIPMENT][drag_prev_slot].item == 0
				&& inventory[CARRIED][slot].item != stack.item
				&& items->items[inventory[CARRIED][slot].item].type == slot_type[drag_prev_slot]
				&& requirementsMet(inventory[CARRIED][slot].item)
			) { // The whole equipped stack is dropped on an empty carried slot or on a wearable item
				// Swap the two stacks
				itemReturn(inventory[area][slot]);
				inventory[area][slot] = stack;
			}
			else {
				itemReturn(stack); // cancel
			}
		}
	}

	drag_prev_src = -1;
}

/**
 * Right-clicking on a usable item in the inventory causes it to activate.
 * e.g. drink a potion
 * e.g. equip an item
 */
void MenuInventory::activate(Point position) {
	ItemStack stack;
	Point nullpt;
	nullpt.x = nullpt.y = 0;

	// clicked a carried item
	int slot = inventory[CARRIED].slotOver(position);
	if (slot == -1)
		return;

	// can't interact with quest items
	if (items->items[inventory[CARRIED][slot].item].type == "quest") {
		return;
	}
	else if (items->items[inventory[CARRIED][slot].item].type == "book") {
		snd->play(sfx_open);
		show_book = items->items[inventory[CARRIED][slot].item].book;
	}
	// use a consumable item
	else if (items->items[inventory[CARRIED][slot].item].type == "consumable") {

		//don't use untransform item if hero is not transformed
		if (powers->powers[items->items[inventory[CARRIED][slot].item].power].spawn_type == "untransform" && !stats->transformed) return;

		//check for power cooldown
		if (pc->hero_cooldown[items->items[inventory[CARRIED][slot].item].power] > 0) return;
		else pc->hero_cooldown[items->items[inventory[CARRIED][slot].item].power] = powers->powers[items->items[inventory[CARRIED][slot].item].power].cooldown;

		// if this item requires targeting it can't be used this way
		if (!powers->powers[items->items[inventory[CARRIED][slot].item].power].requires_targeting) {

			unsigned used_item_count = powers->used_items.size();
			unsigned used_equipped_item_count = powers->used_equipped_items.size();
			powers->activate(items->items[inventory[CARRIED][slot].item].power, stats, nullpt);
			// Remove any used items from the queue of items to be removed. We will destroy the items here.
			if (used_item_count < powers->used_items.size()) powers->used_items.pop_back();
			if (used_equipped_item_count < powers->used_equipped_items.size()) powers->used_equipped_items.pop_back();
			inventory[CARRIED].substract(slot);
		}
		else {
			// let player know this can only be used from the action bar
			log_msg = msg->get("This item can only be used from the action bar.");
		}

	}
	// equip an item
	else if (stats->humanoid) {
		int equip_slot = -1;
		// find first empty(or just first) slot for item to equip
		for (int i = 0; i < MAX_EQUIPPED; i++) {
			// first check for first empty
			if ((slot_type[i] == items->items[inventory[CARRIED][slot].item].type) &&
					(inventory[EQUIPMENT].storage[i].item == 0)) {
				equip_slot = i;
			}
		}
		if (equip_slot == -1) {
			// if empty not found, use just first
			for (int i = 0; i < MAX_EQUIPPED; i++) {
				if (slot_type[i] == items->items[inventory[CARRIED][slot].item].type) {
					equip_slot = i;
					break;
				}
			}
		}

		if (equip_slot != -1) {
			if (requirementsMet(inventory[CARRIED][slot].item)) {
				stack = click(position);
				if( inventory[EQUIPMENT][equip_slot].item == stack.item) {
					// Merge the stacks
					add(stack, EQUIPMENT, equip_slot, false);
				}
				else if( inventory[EQUIPMENT][equip_slot].item == 0) {
					// Drop the stack
					inventory[EQUIPMENT][equip_slot] = stack;
				}
				else {
					if( inventory[CARRIED][slot].item == 0) { // Don't forget this slot may have been emptied by the click()
						// Swap the two stacks
						itemReturn( inventory[EQUIPMENT][equip_slot]);
					}
					else {
						// Drop the equipped item anywhere
						add( inventory[EQUIPMENT][equip_slot]);
					}
					inventory[EQUIPMENT][equip_slot] = stack;
				}
				updateEquipment( equip_slot);
				items->playSound(inventory[EQUIPMENT][equip_slot].item);
			}
		}
		else fprintf(stderr, "Can't find equip slot, corresponding to type %s\n", items->items[inventory[CARRIED][slot].item].type.c_str());
	}

	drag_prev_src = -1;
}

/**
 * Insert item into first available carried slot, preferably in the optionnal specified slot
 *
 * @param ItemStack Stack of items
 * @param area Area number where it will try to store the item
 * @param slot Slot number where it will try to store the item
 */
void MenuInventory::add(ItemStack stack, int area, int slot, bool play_sound) {
	if (stack.quantity < 1)
		return;

	if (play_sound)
		items->playSound(stack.item);

	if (stack.item != 0) {
		if (area < 0) {
			area = CARRIED;
		}
		if (area == CARRIED) {
			ItemStack leftover = inventory[CARRIED].add(stack, slot);
			if (leftover.quantity > 0) {
				drop_stack.push(leftover);
			}
		}
		else if (area == EQUIPMENT) {
			ItemStack leftover = inventory[EQUIPMENT].add(stack, slot);
			if (leftover.quantity > 0) {
				add(leftover, CARRIED, -1, false);
			}
		}
	}
	drag_prev_src = -1;
}

/**
 * Remove one given item from the player's inventory.
 */
void MenuInventory::remove(int item) {
	if( !inventory[CARRIED].remove(item)) {
		inventory[EQUIPMENT].remove(item);
		applyEquipment(inventory[EQUIPMENT].storage);
	}
}

/**
 * Remove an equipped item from the player's inventory.
 */
void MenuInventory::removeEquipped(int item) {
	inventory[EQUIPMENT].remove(item);
	applyEquipment(inventory[EQUIPMENT].storage);
}

/**
 * Add currency item
 */
void MenuInventory::addCurrency(int count) {
	ItemStack stack;
	stack.item = CURRENCY_ID;
	stack.quantity = count;
	add(stack, CARRIED, -1, false);
}

/**
 * Remove currency item
 */
void MenuInventory::removeCurrency(int count) {
	for (int i=0; i<count; i++) {
		inventory[CARRIED].remove(CURRENCY_ID);
	}
}

/**
 * Count the number of currency items in the inventory
 */
int MenuInventory::getCurrency() {

	return getItemCountCarried(CURRENCY_ID);
}

/**
 * Check if there is enough currency to buy the given stack, and if so remove it from the current total and add the stack.
 * (Handle the drop into the equipment area, but add() don't handle it well in all circonstances. MenuManager::logic() allow only into the carried area.)
 */
bool MenuInventory::buy(ItemStack stack, int tab) {
	int value_each;
	if (tab == VENDOR_BUY) value_each = items->items[stack.item].price;
	else value_each = items->items[stack.item].getSellPrice();

	int count = value_each * stack.quantity;
	if( getCurrency() >= count) {
		removeCurrency(count);
		items->playSound(CURRENCY_ID);
		return true;
	}
	else {
		return false;
	}
}

/**
 * Similar to sell(), but for use with stash
 */
bool MenuInventory::stashAdd(ItemStack stack) {
	// items that have no price cannot be stored
	if (items->items[stack.item].price == 0) return false;

	drag_prev_src = -1;
	return true;
}
/**
 * Sell a specific stack of items
 */
bool MenuInventory::sell(ItemStack stack) {
	// can't sell currency
	if (stack.item == CURRENCY_ID) return false;

	// items that have no price cannot be sold
	if (items->items[stack.item].price == 0) return false;

	int value_each = items->items[stack.item].getSellPrice();
	int value = value_each * stack.quantity;
	addCurrency(value);
	items->playSound(CURRENCY_ID);
	drag_prev_src = -1;
	return true;
}

/**
 * Cannot pick up new items if the inventory is full.
 * Full means no more carrying capacity (equipped capacity is ignored)
 */
bool MenuInventory::full(ItemStack stack) {
	return inventory[CARRIED].full(stack);
}

/**
 * An alternative version of the above full() function
 * This one only checks for a single item
 * It's primarily used when checking LootManager pickups
 */
bool MenuInventory::full(int item) {
	return inventory[CARRIED].full(item);
}

/**
 * Get the number of the specified item carried (not equipped)
 */
int MenuInventory::getItemCountCarried(int item) {
	return inventory[CARRIED].count(item);
}

/**
 * Check to see if the given item is equipped
 */
bool MenuInventory::isItemEquipped(int item) {
	return inventory[EQUIPMENT].contain(item);
}

/**
 * Check requirements on an item
 */
bool MenuInventory::requirementsMet(int item) {
	if (items->items[item].req_stat == REQUIRES_PHYS) {
		return (stats->get_physical() >= items->items[item].req_val);
	}
	else if (items->items[item].req_stat == REQUIRES_MENT) {
		return (stats->get_mental() >= items->items[item].req_val);
	}
	else if (items->items[item].req_stat == REQUIRES_OFF) {
		return (stats->get_offense() >= items->items[item].req_val);
	}
	else if (items->items[item].req_stat == REQUIRES_DEF) {
		return (stats->get_defense() >= items->items[item].req_val);
	}
	// otherwise there is no requirement, so it is usable.
	return true;
}

void MenuInventory::updateEquipment(int slot) {

	if (slot == -1) {
		// This should never happen, but ignore it if it does
		return;
	}
	else if (slot_type[slot] != "artifact") {
		changed_equipment = true;
	}
	else {
		changed_artifact = true;
	}
}

/**
 * Given the equipped items, calculate the hero's stats
 */
void MenuInventory::applyEquipment(ItemStack *equipped) {

	const vector<Item> &pc_items = items->items;
	int item_id;

	// calculate bonuses to basic stats, added by items
	bool checkRequired = true;
	while(checkRequired) {
		checkRequired = false;
		stats->offense_additional = stats->defense_additional = stats->physical_additional = stats->mental_additional = 0;
		for (int i = 0; i < MAX_EQUIPPED; i++) {
			item_id = equipped[i].item;
			const Item &item = pc_items[item_id];
			unsigned bonus_counter = 0;
			while (bonus_counter < item.bonus_stat.size() && item.bonus_stat[bonus_counter] != "") {
				if (item.bonus_stat[bonus_counter] == "offense")
					stats->offense_additional += item.bonus_val[bonus_counter];
				else if (item.bonus_stat[bonus_counter] == "defense")
					stats->defense_additional += item.bonus_val[bonus_counter];
				else if (item.bonus_stat[bonus_counter] == "physical")
					stats->physical_additional += item.bonus_val[bonus_counter];
				else if (item.bonus_stat[bonus_counter] == "mental")
					stats->mental_additional += item.bonus_val[bonus_counter];
				else if (item.bonus_stat[bonus_counter] == "all basic stats") {
					stats->offense_additional += item.bonus_val[bonus_counter];
					stats->defense_additional += item.bonus_val[bonus_counter];
					stats->physical_additional += item.bonus_val[bonus_counter];
					stats->mental_additional += item.bonus_val[bonus_counter];
				}
				bonus_counter++;
			}
		}

		// calculate bonuses. added by item sets
		vector<int> set;
		vector<int> quantity;
		vector<int>::iterator it;

		for (int i=0; i<MAX_EQUIPPED; i++) {
			item_id = equipped[i].item;
			it = find(set.begin(), set.end(), items->items[item_id].set);
			if (items->items[item_id].set > 0 && it != set.end()) {
				quantity[distance(set.begin(), it)] += 1;
			}
			else if (items->items[item_id].set > 0) {
				set.push_back(items->items[item_id].set);
				quantity.push_back(1);
			}
		}
		// calculate bonuses to basic stats, added by item sets
		ItemSet temp_set;
		for (unsigned k=0; k<set.size(); k++) {
			temp_set = items->item_sets[set[k]];
			for (unsigned bonus_counter=0; bonus_counter<temp_set.bonus.size(); bonus_counter++) {
				if (temp_set.bonus[bonus_counter].requirement != quantity[k]) continue;

				if (temp_set.bonus[bonus_counter].bonus_stat == "offense")
					stats->offense_additional += temp_set.bonus[bonus_counter].bonus_val;
				else if (temp_set.bonus[bonus_counter].bonus_stat == "defense")
					stats->defense_additional += temp_set.bonus[bonus_counter].bonus_val;
				else if (temp_set.bonus[bonus_counter].bonus_stat == "physical")
					stats->physical_additional += temp_set.bonus[bonus_counter].bonus_val;
				else if (temp_set.bonus[bonus_counter].bonus_stat == "mental")
					stats->mental_additional += temp_set.bonus[bonus_counter].bonus_val;
				else if (temp_set.bonus[bonus_counter].bonus_stat == "all basic stats") {
					stats->offense_additional += temp_set.bonus[bonus_counter].bonus_val;
					stats->defense_additional += temp_set.bonus[bonus_counter].bonus_val;
					stats->physical_additional += temp_set.bonus[bonus_counter].bonus_val;
					stats->mental_additional += temp_set.bonus[bonus_counter].bonus_val;
				}
			}
		}
		// check that each equipped item fit requirements
		for (int i = 0; i < MAX_EQUIPPED; i++) {
			if (!requirementsMet(equipped[i].item)) {
				add(equipped[i]);
				equipped[i].item = 0;
				equipped[i].quantity = 0;
				checkRequired = true;
			}
		}
	}

	// defaults
	for (unsigned i=0; i<stats->powers_list_items.size(); ++i) {
		int id = stats->powers_list_items[i];
		// stats->hp > 0 is hack to keep on_death revive passives working
		if (powers->powers[id].passive && stats->hp > 0)
			stats->effects.removeEffectPassive(id);
	}
	stats->powers_list_items.clear();

	// reset wielding vars
	stats->equip_flags.clear();

	// remove all effects and bonuses added by items
	stats->effects.clearItemEffects();

	applyItemStats(equipped);
	applyItemSetBonuses(equipped);


	// disable any incompatible slots, unequipping items if neccessary
	for (int i=0; i<MAX_EQUIPPED; ++i) {
		inventory[EQUIPMENT].slots[i]->enabled = true;

		int id = inventory[EQUIPMENT][i].item;
		for (unsigned j=0; j<items->items[id].disable_slots.size(); ++j) {
			for (int k=0; k<MAX_EQUIPPED; ++k) {
				if (slot_type[k] == items->items[id].disable_slots[j]) {
					add(inventory[EQUIPMENT].storage[k]);
					inventory[EQUIPMENT].storage[k].item = 0;
					inventory[EQUIPMENT].storage[k].quantity = 0;
					inventory[EQUIPMENT].slots[k]->enabled = false;
				}
			}
		}
	}
	// update stat display
	stats->refresh_stats = true;
}

void MenuInventory::applyItemStats(ItemStack *equipped) {
	const vector<Item> &pc_items = items->items;

	// reset additional values
	stats->dmg_melee_min_add = stats->dmg_melee_max_add = 0;
	stats->dmg_ment_min_add = stats->dmg_ment_max_add = 0;
	stats->dmg_ranged_min_add = stats->dmg_ranged_max_add = 0;
	stats->absorb_min_add = stats->absorb_max_add = 0;

	// apply stats from all items
	for (int i=0; i<MAX_EQUIPPED; i++) {
		int item_id = equipped[i].item;
		const Item &item = pc_items[item_id];

		// apply base stats
		stats->dmg_melee_min_add += item.dmg_melee_min;
		stats->dmg_melee_max_add += item.dmg_melee_max;
		stats->dmg_ranged_min_add += item.dmg_ranged_min;
		stats->dmg_ranged_max_add += item.dmg_ranged_max;
		stats->dmg_ment_min_add += item.dmg_ment_min;
		stats->dmg_ment_max_add += item.dmg_ment_max;

		// set equip flags
		for (unsigned j=0; j<item.equip_flags.size(); ++j) {
			stats->equip_flags.insert(item.equip_flags[j]);
		}

		// apply absorb bonus
		stats->absorb_min_add += item.abs_min;
		stats->absorb_max_add += item.abs_max;

		// apply various bonuses
		unsigned bonus_counter = 0;
		while (bonus_counter < item.bonus_stat.size() && item.bonus_stat[bonus_counter] != "") {
			std::string id = item.bonus_stat[bonus_counter];

			if (powers->effects.find(id) != powers->effects.end())
				stats->effects.addEffect(id, powers->effects[id].icon, 0, item.bonus_val[bonus_counter], powers->effects[id].type, powers->effects[id].animation, powers->effects[id].additive, true, -1, powers->effects[id].render_above, 0, SOURCE_TYPE_HERO);

			bonus_counter++;
		}

		// add item powers
		if (item.power > 0) {
			stats->powers_list_items.push_back(item.power);
			if (stats->effects.triggered_others)
				powers->activateSinglePassive(stats,item.power);
		}

	}
}

void MenuInventory::applyItemSetBonuses(ItemStack *equipped) {
	// calculate bonuses. added by item sets
	vector<int> set;
	vector<int> quantity;
	vector<int>::iterator it;

	for (int i=0; i<MAX_EQUIPPED; i++) {
		int item_id = equipped[i].item;
		it = find(set.begin(), set.end(), items->items[item_id].set);
		if (items->items[item_id].set > 0 && it != set.end()) {
			quantity[distance(set.begin(), it)] += 1;
		}
		else if (items->items[item_id].set > 0) {
			set.push_back(items->items[item_id].set);
			quantity.push_back(1);
		}
	}
	// apply item set bonuses
	ItemSet temp_set;
	for (unsigned k=0; k<set.size(); k++) {
		temp_set = items->item_sets[set[k]];
		unsigned bonus_counter = 0;
		for (bonus_counter=0; bonus_counter<temp_set.bonus.size(); bonus_counter++) {
			if (temp_set.bonus[bonus_counter].requirement != quantity[k]) continue;

			std::string id = temp_set.bonus[bonus_counter].bonus_stat;

			if (powers->effects.find(id) != powers->effects.end())
				stats->effects.addEffect(id, powers->effects[id].icon, 0, temp_set.bonus[bonus_counter].bonus_val, powers->effects[id].type, powers->effects[id].animation, powers->effects[id].additive, true, -1, powers->effects[id].render_above, 0, SOURCE_TYPE_HERO);
		}
	}
}

int MenuInventory::getEquippedCount() {
	return (int)equipped_area.size();
}

int MenuInventory::getCarriedRows() {
	return carried_rows;
}

void MenuInventory::clearHighlight() {
	inventory[EQUIPMENT].highlightClear();
	inventory[CARRIED].highlightClear();
}

/**
 * Sort equipment storage array, so items order matches slots order
 */
void MenuInventory::fillEquipmentSlots() {
	// create temporary arrays
	int slot_number = MAX_EQUIPPED;
	int *equip_item = new int[slot_number];
	int *equip_quantity = new int[slot_number];;

	// initialize arrays
	for (int i=0; i<slot_number; i++) {
		equip_item[i] = inventory[EQUIPMENT].storage[i].item;
		equip_quantity[i] = inventory[EQUIPMENT].storage[i].quantity;
	}
	// clean up storage[]
	for (int i=0; i<slot_number; i++) {
		inventory[EQUIPMENT].storage[i].item = 0;
		inventory[EQUIPMENT].storage[i].quantity = 0;
	}

	// fill slots with items
	for (int i=0; i<slot_number; i++) {
		bool found_slot = false;
		for (int j=0; j<slot_number; j++) {
			// search for empty slot with needed type. If item is not NULL, put it there
			if (equip_item[i] > 0 && inventory[EQUIPMENT].storage[j].item == 0) {
				if (items->items[equip_item[i]].type == slot_type[j]) {
					inventory[EQUIPMENT].storage[j].item = equip_item[i];
					inventory[EQUIPMENT].storage[j].quantity = (equip_quantity[i] > 0) ? equip_quantity[i] : 1;
					found_slot = true;
					break;
				}
			}
		}
		// couldn't find a slot, adding to inventory
		if (!found_slot && equip_item[i] > 0) {
			ItemStack stack;
			stack.item = equip_item[i];
			stack.quantity = (equip_quantity[i] > 0) ? equip_quantity[i] : 1;
			add(stack, CARRIED, -1, false);
		}
	}
	delete [] equip_item;
	delete [] equip_quantity;
}


MenuInventory::~MenuInventory() {
	delete closeButton;
}
