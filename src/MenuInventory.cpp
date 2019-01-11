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
 * class MenuInventory
 */

#include "Avatar.h"
#include "CommonIncludes.h"
#include "EffectManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "Hazard.h"
#include "ItemManager.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "MenuInventory.h"
#include "MenuManager.h"
#include "MenuPowers.h"
#include "MessageEngine.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "StatBlock.h"
#include "TooltipManager.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetSlot.h"

MenuInventory::MenuInventory()
	: MAX_EQUIPPED(4)
	, MAX_CARRIED(64)
	, carried_cols(4)
	, carried_rows(4)
	, tap_to_activate_timer(settings->max_frames_per_sec / 3)
	, activated_slot(-1)
	, activated_item(0)
	, currency(0)
	, drag_prev_src(-1)
	, changed_equipment(true)
	, inv_ctrl(CTRL_NONE)
	, show_book("")
{
	visible = false;

	setBackground("images/menus/inventory.png");

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Load config settings
	FileParser infile;
	// @CLASS MenuInventory|Description of menus/inventory.txt
	if (infile.open("menus/inventory.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = Parse::toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR equipment_slot|repeatable(int, int, string) : X, Y, Slot Type|Position and item type of an equipment slot.
			else if(infile.key == "equipment_slot") {
				Rect area;
				Point pos;

				pos.x = area.x = Parse::popFirstInt(infile.val);
				pos.y = area.y = Parse::popFirstInt(infile.val);
				area.w = area.h = eset->resolutions.icon_size;
				equipped_area.push_back(area);
				equipped_pos.push_back(pos);
				slot_type.push_back(Parse::popFirstString(infile.val));
			}
			// @ATTR carried_area|point|Position of the first normal inventory slot.
			else if(infile.key == "carried_area") {
				Point pos;
				carried_pos.x = carried_area.x = Parse::popFirstInt(infile.val);
				carried_pos.y = carried_area.y = Parse::popFirstInt(infile.val);
			}
			// @ATTR carried_cols|int|The number of columns for the normal inventory.
			else if (infile.key == "carried_cols") carried_cols = std::max(1, Parse::toInt(infile.val));
			// @ATTR carried_rows|int|The number of rows for the normal inventory.
			else if (infile.key == "carried_rows") carried_rows = std::max(1, Parse::toInt(infile.val));
			// @ATTR label_title|label|Position of the "Inventory" label.
			else if (infile.key == "label_title") {
				label_inventory.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR currency|label|Position of the label that displays the total currency being carried.
			else if (infile.key == "currency") {
				label_currency.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR help|rectangle|A mouse-over area that displays some help text for inventory shortcuts.
			else if (infile.key == "help") help_pos = Parse::toRect(infile.val);

			else infile.error("MenuInventory: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	MAX_EQUIPPED = static_cast<int>(equipped_area.size());
	MAX_CARRIED = carried_cols * carried_rows;

	carried_area.w = carried_cols * eset->resolutions.icon_size;
	carried_area.h = carried_rows * eset->resolutions.icon_size;

	label_inventory.setText(msg->get("Inventory"));
	label_inventory.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	label_currency.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	inventory[EQUIPMENT].initFromList(MAX_EQUIPPED, equipped_area, slot_type);
	inventory[CARRIED].initGrid(MAX_CARRIED, carried_area, carried_cols);

	for (int i = 0; i < MAX_EQUIPPED; i++) {
		tablist.add(inventory[EQUIPMENT].slots[i]);
	}
	for (int i = 0; i < MAX_CARRIED; i++) {
		tablist.add(inventory[CARRIED].slots[i]);
	}

	align();
}

void MenuInventory::align() {
	Menu::align();

	for (int i=0; i<MAX_EQUIPPED; i++) {
		equipped_area[i].x = equipped_pos[i].x + window_area.x;
		equipped_area[i].y = equipped_pos[i].y + window_area.y;
	}

	carried_area.x = carried_pos.x + window_area.x;
	carried_area.y = carried_pos.y + window_area.y;

	inventory[EQUIPMENT].setPos(window_area.x, window_area.y);
	inventory[CARRIED].setPos(window_area.x, window_area.y);

	closeButton->setPos(window_area.x, window_area.y);

	label_inventory.setPos(window_area.x, window_area.y);
	label_currency.setPos(window_area.x, window_area.y);
}

void MenuInventory::logic() {

	// if the player has just died, the penalty is half his current currency.
	if (pc->stats.death_penalty && eset->death_penalty.enabled) {
		std::string death_message = "";

		// remove a % of currency
		if (eset->death_penalty.currency > 0) {
			if (currency > 0)
				removeCurrency((currency * eset->death_penalty.currency) / 100);
			death_message += msg->get("Lost %d%% of %s.", eset->death_penalty.currency, eset->loot.currency) + ' ';
		}

		// remove a % of either total xp or xp since the last level
		if (eset->death_penalty.xp > 0) {
			if (pc->stats.xp > 0)
				pc->stats.xp -= (pc->stats.xp * eset->death_penalty.xp) / 100;
			death_message += msg->get("Lost %d%% of total XP.", eset->death_penalty.xp) + ' ';
		}
		else if (eset->death_penalty.xp_current > 0) {
			if (pc->stats.xp - eset->xp.getLevelXP(pc->stats.level) > 0)
				pc->stats.xp -= ((pc->stats.xp - eset->xp.getLevelXP(pc->stats.level)) * eset->death_penalty.xp_current) / 100;
			death_message += msg->get("Lost %d%% of current level XP.", eset->death_penalty.xp_current) + ' ';
		}

		// prevent down-leveling from removing too much xp
		if (pc->stats.xp < eset->xp.getLevelXP(pc->stats.level))
			pc->stats.xp = eset->xp.getLevelXP(pc->stats.level);

		// remove a random carried item
		if (eset->death_penalty.item) {
			std::vector<int> removable_items;
			removable_items.clear();
			for (int i=0; i < MAX_EQUIPPED; i++) {
				if (!inventory[EQUIPMENT][i].empty()) {
					if (!items->items[inventory[EQUIPMENT][i].item].quest_item)
						removable_items.push_back(inventory[EQUIPMENT][i].item);
				}
			}
			for (int i=0; i < MAX_CARRIED; i++) {
				if (!inventory[CARRIED][i].empty()) {
					if (!items->items[inventory[CARRIED][i].item].quest_item)
						removable_items.push_back(inventory[CARRIED][i].item);
				}
			}
			if (!removable_items.empty()) {
				size_t random_item = static_cast<size_t>(rand()) % removable_items.size();
				remove(removable_items[random_item]);
				death_message += msg->get("Lost %s.",items->getItemName(removable_items[random_item]));
			}
		}

		pc->logMsg(death_message, Avatar::MSG_NORMAL);

		pc->stats.death_penalty = false;
	}

	// a copy of currency is kept in stats, to help with various situations
	pc->stats.currency = currency = inventory[CARRIED].count(eset->misc.currency_id);

	// check close button
	if (visible) {
		tablist.logic();

		if (closeButton->checkClick()) {
			visible = false;
			snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
		}

		if (drag_prev_src == -1) {
			clearHighlight();
		}
	}

	tap_to_activate_timer.tick();
}

void MenuInventory::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// text overlay
	label_inventory.render();

	if (!label_currency.isHidden()) {
		label_currency.setText(msg->get("%d %s", currency, eset->loot.currency));
		label_currency.render();
	}

	inventory[EQUIPMENT].render();
	inventory[CARRIED].render();
}

int MenuInventory::areaOver(const Point& position) {
	if (Utils::isWithinRect(carried_area, position)) {
		return CARRIED;
	}
	else {
		for (unsigned int i=0; i<equipped_area.size(); i++) {
			if (Utils::isWithinRect(equipped_area[i], position)) {
				return EQUIPMENT;
			}
		}
	}

	// point is inside the inventory menu, but not over a slot
	if (Utils::isWithinRect(window_area, position)) {
		return NO_AREA;
	}

	return -2;
}

/**
 * If mousing-over an item with a tooltip, return that tooltip data.
 *
 * @param mouse The x,y screen coordinates of the mouse cursor
 */
void MenuInventory::renderTooltips(const Point& position) {
	if (!visible || !Utils::isWithinRect(window_area, position))
		return;

	int area = areaOver(position);
	int slot = -1;
	TooltipData tip_data;

	if (area < 0) {
		if (position.x >= window_area.x + help_pos.x && position.y >= window_area.y+help_pos.y && position.x < window_area.x+help_pos.x+help_pos.w && position.y < window_area.y+help_pos.y+help_pos.h) {
			tip_data.addText(msg->get("Pick up item(s):") + " " + inpt->getBindingString(Input::MAIN1));
			tip_data.addText(msg->get("Use or equip item:") + " " + inpt->getBindingString(Input::MAIN2) + "\n");
			tip_data.addText(msg->get("%s modifiers", inpt->getBindingString(Input::MAIN1)));
			tip_data.addText(msg->get("Select a quantity of item:") + " " + inpt->getBindingString(Input::SHIFT) + " / " + inpt->getBindingString(Input::SHIFT, InputState::BINDING_ALT));

			if (inv_ctrl == CTRL_STASH)
				tip_data.addText(msg->get("Stash item stack:") + " " + inpt->getBindingString(Input::CTRL) + " / " + inpt->getBindingString(Input::CTRL, InputState::BINDING_ALT));
			else if (inv_ctrl == CTRL_VENDOR || (eset->misc.sell_without_vendor && inv_ctrl != CTRL_STASH))
				tip_data.addText(msg->get("Sell item stack:") + " " + inpt->getBindingString(Input::CTRL) + " / " + inpt->getBindingString(Input::CTRL, InputState::BINDING_ALT));
		}
		tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
	}
	else {
		slot = inventory[area].slotOver(position);
	}

	if (slot == -1)
		return;

	tip_data.clear();

	if (inventory[area][slot].item > 0) {
		tip_data = inventory[area].checkTooltip(position, &pc->stats, ItemManager::PLAYER_INV);
	}
	else if (area == EQUIPMENT && inventory[area][slot].empty()) {
		tip_data.addText(msg->get(items->getItemType(slot_type[slot])));
	}

	tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
}

/**
 * Click-start dragging in the inventory
 */
ItemStack MenuInventory::click(const Point& position) {
	ItemStack item;

	drag_prev_src = areaOver(position);
	if (drag_prev_src > -1) {
		item = inventory[drag_prev_src].click(position);

		if (settings->touchscreen) {
			tablist.setCurrent(inventory[drag_prev_src].current_slot);
			tap_to_activate_timer.reset(Timer::BEGIN);
		}

		if (item.empty()) {
			drag_prev_src = -1;
			return item;
		}

		// if dragging equipment, prepare to change stats/sprites
		if (drag_prev_src == EQUIPMENT) {
			if (pc->stats.humanoid) {
				updateEquipment(inventory[EQUIPMENT].drag_prev_slot);
			}
			else {
				itemReturn(item);
				item.clear();
			}
		}
	}

	return item;
}

/**
 * Return dragged item to previous slot
 */
void MenuInventory::itemReturn(ItemStack stack) {
	if (drag_prev_src == -1) {
		add(stack, CARRIED, ItemStorage::NO_SLOT, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
	}
	else {
		int prev_slot = inventory[drag_prev_src].drag_prev_slot;
		inventory[drag_prev_src].itemReturn(stack);
		// if returning equipment, prepare to change stats/sprites
		if (drag_prev_src == EQUIPMENT) {
			updateEquipment(prev_slot);
		}
	}
	drag_prev_src = -1;
}

/**
 * Dragging and dropping an item can be used to rearrange the inventory
 * and equip items
 */
bool MenuInventory::drop(const Point& position, ItemStack stack) {
	items->playSound(stack.item);

	bool success = true;

	int area = areaOver(position);
	if (area < 0) {
		if (drag_prev_src == -1) {
			success = add(stack, CARRIED, ItemStorage::NO_SLOT, !ADD_PLAY_SOUND, ADD_AUTO_EQUIP);
		}
		else {
			// not dropped into a slot. Just return it to the previous slot.
			itemReturn(stack);
		}
		return success;
	}

	int slot = inventory[area].slotOver(position);
	if (slot == -1) {
		if (drag_prev_src == -1) {
			success = add(stack, CARRIED, ItemStorage::NO_SLOT, !ADD_PLAY_SOUND, ADD_AUTO_EQUIP);
		}
		else {
			// not dropped into a slot. Just return it to the previous slot.
			itemReturn(stack);
		}
		return success;
	}

	int drag_prev_slot = -1;
	if (drag_prev_src != -1)
		drag_prev_slot = inventory[drag_prev_src].drag_prev_slot;

	if (area == EQUIPMENT) { // dropped onto equipped item

		// make sure the item is going to the correct slot
		// we match slot_type to stack.item's type to place items in the proper slots
		// also check to see if the hero meets the requirements
		if (slot_type[slot] == items->items[stack.item].type && items->requirementsMet(&pc->stats, stack.item) && pc->stats.humanoid && inventory[EQUIPMENT].slots[slot]->enabled) {
			if (inventory[area][slot].item == stack.item) {
				// Merge the stacks
				success = add(stack, area, slot, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
			}
			else {
				// Swap the two stacks
				if (!inventory[area][slot].empty())
					itemReturn(inventory[area][slot]);
				inventory[area][slot] = stack;
				updateEquipment(slot);
				applyEquipment();

				// if this item has a power, place it on the action bar if possible
				if (items->items[stack.item].power > 0) {
					menu_act->addPower(items->items[stack.item].power, 0);
				}
			}
		}
		else {
			// equippable items only belong to one slot, for the moment
			itemReturn(stack); // cancel
			updateEquipment(slot);
			applyEquipment();
		}
	}
	else if (area == CARRIED) {
		// dropped onto carried item

		if (drag_prev_src == CARRIED) {
			if (slot != drag_prev_slot) {
				if (inventory[area][slot].item == stack.item) {
					// Merge the stacks
					success = add(stack, area, slot, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
				}
				else if (inventory[area][slot].empty()) {
					// Drop the stack
					inventory[area][slot] = stack;
				}
				else if (drag_prev_slot != -1 && inventory[drag_prev_src][drag_prev_slot].empty()) {
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

				// allow reading books on touchscreen devices
				// since touch screens don't have right-click, we use a "tap" (drop on same slot quickly) to activate
				// NOTE: the quantity must be 1, since the number picker appears when tapping on a stack of more than 1 item
				// NOTE: we only support activating books since equipment activation doesn't work for some reason
				// NOTE: Consumables are usually in stacks > 1, so we ignore those as well for consistency
				if (settings->touchscreen && !tap_to_activate_timer.isEnd() && !items->items[stack.item].book.empty() && stack.quantity == 1) {
					activate(position);
				}
			}
		}
		else {
			if (inventory[area][slot].item == stack.item || drag_prev_src == -1) {
				// Merge the stacks
				success = add(stack, area, slot, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
			}
			else if (inventory[area][slot].empty()) {
				// Drop the stack
				inventory[area][slot] = stack;
			}
			else if(
				inventory[EQUIPMENT][drag_prev_slot].empty()
				&& inventory[CARRIED][slot].item != stack.item
				&& items->items[inventory[CARRIED][slot].item].type == slot_type[drag_prev_slot]
				&& items->requirementsMet(&pc->stats, inventory[CARRIED][slot].item)
			) { // The whole equipped stack is dropped on an empty carried slot or on a wearable item
				// Swap the two stacks
				itemReturn(inventory[area][slot]);
				updateEquipment(drag_prev_slot);

				// if this item has a power, place it on the action bar if possible
				if (items->items[inventory[EQUIPMENT][drag_prev_slot].item].power > 0) {
					menu_act->addPower(items->items[inventory[EQUIPMENT][drag_prev_slot].item].power, 0);
				}

				inventory[area][slot] = stack;

				applyEquipment();
			}
			else {
				itemReturn(stack); // cancel
			}
		}
	}

	drag_prev_src = -1;

	return success;
}

/**
 * Right-clicking on a usable item in the inventory causes it to activate.
 * e.g. drink a potion
 * e.g. equip an item
 */
void MenuInventory::activate(const Point& position) {
	ItemStack stack;
	FPoint nullpt;
	nullpt.x = nullpt.y = 0;

	// clicked a carried item
	int slot = inventory[CARRIED].slotOver(position);
	if (slot == -1)
		return;

	// empty slot
	if (inventory[CARRIED][slot].empty())
		return;

	// if the item is a book, open it
	if (!items->items[inventory[CARRIED][slot].item].book.empty()) {
		show_book = items->items[inventory[CARRIED][slot].item].book;
	}
	// use a consumable item
	else if (!items->items[inventory[CARRIED][slot].item].quest_item &&
             items->items[inventory[CARRIED][slot].item].type == "consumable" &&
	         items->items[inventory[CARRIED][slot].item].power > 0) {

		int power_id = items->items[inventory[CARRIED][slot].item].power;

		// equipment might want to replace powers, so do it here
		for (int i = 0; i < inventory[EQUIPMENT].getSlotNumber(); ++i) {
			int id = inventory[EQUIPMENT][i].item;

			for (size_t j = 0; j < items->items[id].replace_power.size(); ++j) {
				if (power_id == items->items[id].replace_power[j].x) {
					power_id = items->items[id].replace_power[j].y;
					break;
				}
			}
		}

		// if the power consumes items, make sure we have enough
		for (size_t i = 0; i < powers->powers[power_id].required_items.size(); ++i) {
			if (powers->powers[power_id].required_items[i].id > 0 &&
			    powers->powers[power_id].required_items[i].quantity > inventory[CARRIED].count(powers->powers[power_id].required_items[i].id))
			{
				pc->logMsg(msg->get("You don't have enough of the required item."), Avatar::MSG_NORMAL);
				return;
			}

			if (powers->powers[power_id].required_items[i].id == inventory[CARRIED][slot].item) {
				activated_slot = slot;
				activated_item = inventory[CARRIED][slot].item;
			}
		}

		// check power & item requirements
		if (!pc->stats.canUsePower(power_id, !StatBlock::CAN_USE_PASSIVE) || !pc->power_cooldown_timers[power_id].isEnd()) {
			pc->logMsg(msg->get("You can't use this item right now."), Avatar::MSG_NORMAL);
			return;
		}

		pc->power_cooldown_timers[power_id].setDuration(powers->powers[power_id].cooldown);

		// if this item requires targeting it can't be used this way
		if (!powers->powers[power_id].requires_targeting) {
			powers->activate(power_id, &pc->stats, nullpt);
		}
		else {
			// let player know this can only be used from the action bar
			pc->logMsg(msg->get("This item can only be used from the action bar."), Avatar::MSG_NORMAL);
		}

	}
	// equip an item
	else if (pc->stats.humanoid && items->items[inventory[CARRIED][slot].item].type != "") {
		int equip_slot = getEquipSlotFromItem(inventory[CARRIED].storage[slot].item, !ONLY_EMPTY_SLOTS);

		if (equip_slot >= 0) {
			stack = click(position);

			if (inventory[EQUIPMENT][equip_slot].item == stack.item) {
				// Merge the stacks
				add(stack, EQUIPMENT, equip_slot, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
			}
			else if (inventory[EQUIPMENT][equip_slot].empty()) {
				// Drop the stack
				inventory[EQUIPMENT][equip_slot] = stack;
			}
			else {
				if (inventory[CARRIED][slot].empty()) { // Don't forget this slot may have been emptied by the click()
					// Swap the two stacks
					itemReturn(inventory[EQUIPMENT][equip_slot]);
				}
				else {
					// Drop the equipped item anywhere
					add(inventory[EQUIPMENT][equip_slot], CARRIED, ItemStorage::NO_SLOT, ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
				}
				inventory[EQUIPMENT][equip_slot] = stack;
			}

			updateEquipment(equip_slot);
			items->playSound(inventory[EQUIPMENT][equip_slot].item);

			// if this item has a power, place it on the action bar if possible
			if (items->items[stack.item].power > 0) {
				menu_act->addPower(items->items[stack.item].power, 0);
			}

			applyEquipment();
		}
		else if (equip_slot == -1) {
			Utils::logError("MenuInventory: Can't find equip slot, corresponding to type %s", items->items[inventory[CARRIED][slot].item].type.c_str());
		}
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
bool MenuInventory::add(ItemStack stack, int area, int slot, bool play_sound, bool auto_equip) {
	if (stack.empty())
		return true;

	bool success = true;

	if (play_sound)
		items->playSound(stack.item);

	if (auto_equip && settings->auto_equip) {
		int equip_slot = getEquipSlotFromItem(stack.item, ONLY_EMPTY_SLOTS);
		bool disabled_slots_empty = true;

		// if this item would disable non-empty slots, don't auto-equip it
		for (size_t i = 0; i < items->items[stack.item].disable_slots.size(); ++i) {
			for (int j = 0; j < MAX_EQUIPPED; ++j) {
				if (!inventory[EQUIPMENT].storage[j].empty() && slot_type[j] == items->items[stack.item].disable_slots[i]) {
					disabled_slots_empty = false;
				}
			}
		}

		if (equip_slot >= 0 && inventory[EQUIPMENT].slots[equip_slot]->enabled && disabled_slots_empty) {
			area = EQUIPMENT;
			slot = equip_slot;
		}

	}

	if (area == CARRIED) {
		ItemStack leftover = inventory[CARRIED].add(stack, slot);
		if (!leftover.empty()) {
			if (items->items[stack.item].quest_item) {
				// quest items can't be dropped, so find a non-quest item in the inventory to drop
				const int max_q = items->items[stack.item].max_quantity;
				int slots_to_clear = 1;
				if (max_q > 0)
					slots_to_clear = leftover.quantity + (leftover.quantity % max_q) / max_q;

				for (int i = MAX_CARRIED-1; i >=0; --i) {
					if (items->items[inventory[CARRIED].storage[i].item].quest_item)
						continue;

					drop_stack.push(inventory[CARRIED].storage[i]);
					inventory[CARRIED].storage[i].clear();

					slots_to_clear--;
					if (slots_to_clear <= 0)
						break;
				}

				if (slots_to_clear > 0) {
					// inventory is full of quest items! we have to drop this now...
					drop_stack.push(leftover);
				}
				else {
					add(leftover, CARRIED, slot, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
				}
			}
			else {
				drop_stack.push(leftover);
			}
			pc->logMsg(msg->get("Inventory is full."), Avatar::MSG_NORMAL);
			success = false;
		}
	}
	else if (area == EQUIPMENT) {
		ItemStack &dest = inventory[EQUIPMENT].storage[slot];
		ItemStack leftover;
		leftover.item = stack.item;

		if (!dest.empty() && dest.item != stack.item) {
			// items don't match, so just add the stack to the carried area
			leftover.quantity = stack.quantity;
		}
		else if (dest.quantity + stack.quantity > items->items[stack.item].max_quantity) {
			// items match, so attempt to merge the stacks. Any leftover will be added to the carried area
			leftover.quantity = dest.quantity + stack.quantity - items->items[stack.item].max_quantity;
			stack.quantity = items->items[stack.item].max_quantity - dest.quantity;
			if (stack.quantity > 0) {
				add(stack, EQUIPMENT, slot, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
			}
		}
		else {
			// put the item in the appropriate equipment slot
			inventory[EQUIPMENT].add(stack, slot);
			updateEquipment(slot);
			leftover.clear();
		}

		if (!leftover.empty()) {
			add(leftover, CARRIED, ItemStorage::NO_SLOT, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
		}

		applyEquipment();
	}

	// if this item has a power, place it on the action bar if possible
	if (success && items->items[stack.item].type == "consumable" && items->items[stack.item].power > 0) {
		menu_act->addPower(items->items[stack.item].power, 0);
	}

	drag_prev_src = -1;

	return success;
}

/**
 * Remove one given item from the player's inventory.
 */
bool MenuInventory::remove(int item) {
	if (activated_item != 0 && activated_slot != -1 && item == activated_item) {
		inventory[CARRIED].subtract(activated_slot, 1);
		activated_item = 0;
		activated_slot = -1;
	}
	else if(!inventory[CARRIED].remove(item, 1)) {
		if (!inventory[EQUIPMENT].remove(item, 1)) {
			return false;
		}
		else {
			applyEquipment();
		}
	}

	return true;
}

void MenuInventory::removeFromPrevSlot(int quantity) {
	if (drag_prev_src > -1 && inventory[drag_prev_src].drag_prev_slot > -1) {
		int drag_prev_slot = inventory[drag_prev_src].drag_prev_slot;
		inventory[drag_prev_src].subtract(drag_prev_slot, quantity);
		if (inventory[drag_prev_src].storage[drag_prev_slot].empty()) {
			if (drag_prev_src == EQUIPMENT)
				updateEquipment(inventory[EQUIPMENT].drag_prev_slot);
		}
	}
}

/**
 * Add currency item
 */
void MenuInventory::addCurrency(int count) {
	if (count > 0) {
		ItemStack stack;
		stack.item = eset->misc.currency_id;
		stack.quantity = count;
		add(stack, CARRIED, ItemStorage::NO_SLOT, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
	}
}

/**
 * Remove currency item
 */
void MenuInventory::removeCurrency(int count) {
	inventory[CARRIED].remove(eset->misc.currency_id, count);
}

/**
 * Check if there is enough currency to buy the given stack, and if so remove it from the current total and add the stack.
 * (Handle the drop into the equipment area, but add() don't handle it well in all circonstances. MenuManager::logic() allow only into the carried area.)
 */
bool MenuInventory::buy(ItemStack stack, int tab, bool dragging) {
	if (stack.empty()) {
		return true;
	}

	int value_each;
	if (tab == ItemManager::VENDOR_BUY) value_each = items->items[stack.item].getPrice();
	else value_each = items->items[stack.item].getSellPrice(stack.can_buyback);

	int count = value_each * stack.quantity;
	if( inventory[CARRIED].count(eset->misc.currency_id) >= count) {
		stack.can_buyback = false;

		if (dragging) {
			drop(inpt->mouse, stack);
		}
		else {
			add(stack, CARRIED, ItemStorage::NO_SLOT, ADD_PLAY_SOUND, ADD_AUTO_EQUIP);
		}

		removeCurrency(count);
		items->playSound(eset->misc.currency_id);
		return true;
	}
	else {
		pc->logMsg(msg->get("Not enough %s.", eset->loot.currency), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}
}

/**
 * Sell a specific stack of items
 */
bool MenuInventory::sell(ItemStack stack) {
	if (stack.empty()) {
		return false;
	}

	// can't sell currency
	if (stack.item == eset->misc.currency_id) return false;

	// items that have no price cannot be sold
	if (items->items[stack.item].getPrice() == 0) {
		items->playSound(stack.item);
		pc->logMsg(msg->get("This item can not be sold."), Avatar::MSG_NORMAL);
		return false;
	}

	// quest items can not be sold
	if (items->items[stack.item].quest_item) {
		items->playSound(stack.item);
		pc->logMsg(msg->get("This item can not be sold."), Avatar::MSG_NORMAL);
		return false;
	}

	int value_each = items->items[stack.item].getSellPrice(ItemManager::DEFAULT_SELL_PRICE);
	int value = value_each * stack.quantity;
	addCurrency(value);
	items->playSound(eset->misc.currency_id);
	drag_prev_src = -1;
	return true;
}

void MenuInventory::updateEquipment(int slot) {

	if (slot == -1) {
		// This should never happen, but ignore it if it does
		return;
	}
	else {
		changed_equipment = true;
	}
}

/**
 * Given the equipped items, calculate the hero's stats
 */
void MenuInventory::applyEquipment() {
	if (items->items.empty())
		return;

	int item_id;

	// calculate bonuses to basic stats, added by items
	bool checkRequired = true;
	while(checkRequired) {
		checkRequired = false;

		for (size_t j = 0; j < eset->primary_stats.list.size(); ++j) {
			pc->stats.primary_additional[j] = 0;
		}

		for (int i = 0; i < MAX_EQUIPPED; i++) {
			item_id = inventory[EQUIPMENT].storage[i].item;
			const Item &item = items->items[item_id];
			unsigned bonus_counter = 0;
			while (bonus_counter < item.bonus.size()) {
				for (size_t j = 0; j < eset->primary_stats.list.size(); ++j) {
					if (item.bonus[bonus_counter].base_index == static_cast<int>(j))
						pc->stats.primary_additional[j] += item.bonus[bonus_counter].value;
				}

				bonus_counter++;
			}
		}

		// calculate bonuses. added by item sets
		std::vector<int> set;
		std::vector<int> quantity;
		std::vector<int>::iterator it;

		for (int i=0; i<MAX_EQUIPPED; i++) {
			item_id = inventory[EQUIPMENT].storage[i].item;
			it = std::find(set.begin(), set.end(), items->items[item_id].set);
			if (items->items[item_id].set > 0 && it != set.end()) {
				quantity[std::distance(set.begin(), it)] += 1;
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

				for (size_t j = 0; j < eset->primary_stats.list.size(); ++j) {
					if (temp_set.bonus[bonus_counter].base_index == static_cast<int>(j))
						pc->stats.primary_additional[j] += temp_set.bonus[bonus_counter].value;
				}
			}
		}
		// check that each equipped item fit requirements
		for (int i = 0; i < MAX_EQUIPPED; i++) {
			if (!items->requirementsMet(&pc->stats, inventory[EQUIPMENT].storage[i].item)) {
				add(inventory[EQUIPMENT].storage[i], CARRIED, ItemStorage::NO_SLOT, ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
				inventory[EQUIPMENT].storage[i].clear();
				checkRequired = true;
			}
		}
	}

	// defaults
	for (unsigned i=0; i<pc->stats.powers_list_items.size(); ++i) {
		int id = pc->stats.powers_list_items[i];
		// pc->stats.hp > 0 is hack to keep on_death revive passives working
		if (powers->powers[id].passive && pc->stats.hp > 0)
			pc->stats.effects.removeEffectPassive(id);
	}
	pc->stats.powers_list_items.clear();

	// reset wielding vars
	pc->stats.equip_flags.clear();

	// remove all effects and bonuses added by items
	pc->stats.effects.clearItemEffects();

	// reset power level bonuses
	menu->pow->clearBonusLevels();

	applyItemStats();
	applyItemSetBonuses();


	// enable all slots by default
	for (int i=0; i<MAX_EQUIPPED; ++i) {
		inventory[EQUIPMENT].slots[i]->enabled = true;
	}
	// disable any incompatible slots, unequipping items if neccessary
	for (int i=0; i<MAX_EQUIPPED; ++i) {
		int id = inventory[EQUIPMENT][i].item;
		for (unsigned j=0; j<items->items[id].disable_slots.size(); ++j) {
			for (int k=0; k<MAX_EQUIPPED; ++k) {
				if (slot_type[k] == items->items[id].disable_slots[j]) {
					if (!inventory[EQUIPMENT].storage[k].empty()) {
						add(inventory[EQUIPMENT].storage[k], CARRIED, ItemStorage::NO_SLOT, ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
						inventory[EQUIPMENT].storage[k].clear();
						updateEquipment(k);
						applyEquipment();
					}
					inventory[EQUIPMENT].slots[k]->enabled = false;
				}
			}
		}
	}
	// update stat display
	pc->stats.refresh_stats = true;
}

void MenuInventory::applyItemStats() {
	if (items->items.empty())
		return;

	// reset additional values
	for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
		pc->stats.dmg_min_add[i] = pc->stats.dmg_max_add[i] = 0;
	}
	pc->stats.absorb_min_add = pc->stats.absorb_max_add = 0;

	// apply stats from all items
	for (int i=0; i<MAX_EQUIPPED; i++) {
		int item_id = inventory[EQUIPMENT].storage[i].item;
		const Item &item = items->items[item_id];

		// apply base stats
		for (size_t j = 0; j < eset->damage_types.list.size(); ++j) {
			pc->stats.dmg_min_add[j] += item.dmg_min[j];
			pc->stats.dmg_max_add[j] += item.dmg_max[j];
		}

		// set equip flags
		for (unsigned j=0; j<item.equip_flags.size(); ++j) {
			pc->stats.equip_flags.insert(item.equip_flags[j]);
		}

		// apply absorb bonus
		pc->stats.absorb_min_add += item.abs_min;
		pc->stats.absorb_max_add += item.abs_max;

		// apply various bonuses
		unsigned bonus_counter = 0;
		while (bonus_counter < item.bonus.size()) {
			applyBonus(&item.bonus[bonus_counter]);
			bonus_counter++;
		}

		// add item powers
		if (item.power > 0) {
			pc->stats.powers_list_items.push_back(item.power);
			if (pc->stats.effects.triggered_others)
				powers->activateSinglePassive(&pc->stats, item.power);
		}
	}
}

void MenuInventory::applyItemSetBonuses() {
	// calculate bonuses. added by item sets
	std::vector<int> set;
	std::vector<int> quantity;
	std::vector<int>::iterator it;

	for (int i=0; i<MAX_EQUIPPED; i++) {
		int item_id = inventory[EQUIPMENT].storage[i].item;
		it = std::find(set.begin(), set.end(), items->items[item_id].set);
		if (items->items[item_id].set > 0 && it != set.end()) {
			quantity[std::distance(set.begin(), it)] += 1;
		}
		else if (items->items[item_id].set > 0) {
			set.push_back(items->items[item_id].set);
			quantity.push_back(1);
		}
	}
	// apply item set bonuses
	for (size_t i = 0; i < set.size(); ++i) {
		ItemSet& temp_set = items->item_sets[set[i]];
		for (size_t j = 0; j < temp_set.bonus.size(); ++j) {
			if (temp_set.bonus[j].requirement > quantity[i])
				continue;
			applyBonus(&temp_set.bonus[j]);
		}
	}
}

void MenuInventory::applyBonus(const BonusData* bdata) {
	EffectDef ed;

	if (bdata->is_speed) {
		ed.id = ed.type = "speed";
	}
	else if (bdata->is_attack_speed) {
		ed.id = ed.type = "attack_speed";
	}
	else if (bdata->stat_index != -1) {
		ed.id = ed.type = Stats::KEY[bdata->stat_index];
	}
	else if (bdata->damage_index_min != -1) {
		ed.id = ed.type = eset->damage_types.list[bdata->damage_index_min].min;
	}
	else if (bdata->damage_index_max != -1) {
		ed.id = ed.type = eset->damage_types.list[bdata->damage_index_max].max;
	}
	else if (bdata->resist_index != -1) {
		ed.id = ed.type = eset->elements.list[bdata->resist_index].id + "_resist";
	}
	else if (bdata->base_index > -1 && static_cast<size_t>(bdata->base_index) < eset->primary_stats.list.size()) {
		ed.id = ed.type = eset->primary_stats.list[bdata->base_index].id;
	}
	else if (bdata->power_id > 0) {
		menu->pow->addBonusLevels(bdata->power_id, bdata->value);
		return; // don't add item effect
	}

	pc->stats.effects.addItemEffect(ed, 0, bdata->value);
}

int MenuInventory::getEquippedCount() {
	return static_cast<int>(equipped_area.size());
}

int MenuInventory::getTotalSlotCount() {
	return MAX_CARRIED + MAX_EQUIPPED;
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
		inventory[EQUIPMENT].storage[i].clear();
	}

	// fill slots with items
	for (int i=0; i<slot_number; i++) {
		bool found_slot = false;
		for (int j=0; j<slot_number; j++) {
			// search for empty slot with needed type. If item is not NULL, put it there
			if (equip_item[i] > 0 && inventory[EQUIPMENT].storage[j].empty()) {
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
			add(stack, CARRIED, ItemStorage::NO_SLOT, !ADD_PLAY_SOUND, !ADD_AUTO_EQUIP);
		}
	}
	delete [] equip_item;
	delete [] equip_quantity;
}

int MenuInventory::getMaxPurchasable(ItemStack item, int vendor_tab) {
	if (vendor_tab == ItemManager::VENDOR_BUY)
		return currency / items->items[item.item].getPrice();
	else if (vendor_tab == ItemManager::VENDOR_SELL)
		return currency / items->items[item.item].getSellPrice(item.can_buyback);
	else
		return 0;
}

int MenuInventory::getEquipSlotFromItem(int item, bool only_empty_slots) {
	if (!items->requirementsMet(&pc->stats, item))
		return -2;

	int equip_slot = -1;

	// find first empty(or just first) slot for item to equip
	for (int i = 0; i < MAX_EQUIPPED; i++) {
		if (slot_type[i] == items->items[item].type) {
			if (inventory[EQUIPMENT].storage[i].empty()) {
				// empty and matching, no need to search more
				equip_slot = i;
				break;
			}
			else if (!only_empty_slots && equip_slot == -1) {
				// non-empty and matching
				equip_slot = i;
			}
		}
	}

	return equip_slot;
}

int MenuInventory::getPowerMod(int meta_power) {
	for (int i = 0; i < inventory[EQUIPMENT].getSlotNumber(); ++i) {
		int id = inventory[EQUIPMENT][i].item;

		for (size_t j=0; j<items->items[id].replace_power.size(); j++) {
			if (items->items[id].replace_power[j].x == meta_power && items->items[id].replace_power[j].y != meta_power) {
				return items->items[id].replace_power[j].y;
			}
		}
	}

	return 0;
}

MenuInventory::~MenuInventory() {
	delete closeButton;
}
