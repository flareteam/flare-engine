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
 * class ItemStorage
 */

#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "ItemManager.h"
#include "ItemStorage.h"
#include "MessageEngine.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

#include <limits.h>

ItemStorage::ItemStorage()
	: slot_number(0)
	, next_sort_mode(SORT_NONE + 1)
	, storage(NULL)
	, sort_tooltip(NULL)
{}

void ItemStorage::init(int _slot_number) {
	if (storage && slot_number == _slot_number)
		return; // already initialized

	slot_number = _slot_number;

	if (storage)
		delete storage;

	storage = new ItemStack[slot_number];

	for( int i=0; i<slot_number; i++) {
		storage[i].clear();
	}
}

ItemStack & ItemStorage::operator [] (int slot) {
	return storage[slot];
}

/**
 * Take the savefile CSV list of items id and convert to storage array
 */
void ItemStorage::setItems(const std::string& s) {
	std::string item_list = s + ',';
	for (int i=0; i<slot_number; i++) {
		storage[i].item = items->verifyID(Parse::toItemID(Parse::popFirstString(item_list)), NULL, ItemManager::VERIFY_ALLOW_ZERO, ItemManager::VERIFY_ALLOCATE);
	}
}

/**
 * Take the savefile CSV list of items quantities and convert to storage array
 */
void ItemStorage::setQuantities(const std::string& s) {
	std::string quantity_list = s + ',';
	for (int i=0; i<slot_number; i++) {
		storage[i].quantity = Parse::popFirstInt(quantity_list);
		if (storage[i].quantity < 0) {
			Utils::logError("ItemStorage: Items quantity on position %d is negative, setting to zero", i);
			storage[i].quantity = 0;
		}
	}
}

void ItemStorage::setForeign(bool is_foreign) {
	for (int i = 0; i < slot_number; ++i) {
		if (items->isValid(storage[i].item) && items->items[storage[i].item]->parent) {
			items->items[storage[i].item]->is_foreign = is_foreign;
		}
	}
}

int ItemStorage::getSlotNumber() {
	return slot_number;
}

/**
 * Convert storage array to a CSV list of items id for savefile
 */
std::string ItemStorage::getItems() {
	std::stringstream ss;
	ss.str("");
	for (int i=0; i<slot_number; i++) {
		ss << storage[i].item;
		if (i < slot_number-1) ss << ',';
	}
	return ss.str();
}

/**
 * Convert storage array to a CSV list of items quantities for savefile
 */
std::string ItemStorage::getQuantities() {
	std::stringstream ss;
	ss.str("");
	for (int i=0; i<slot_number; i++) {
		ss << storage[i].quantity;
		if (i < slot_number-1) ss << ',';
	}
	return ss.str();
}

void ItemStorage::clear() {
	for( int i=0; i<slot_number; i++) {
		storage[i].clear();
	}
}

/**
 * Insert item into first available carried slot, preferably in the optionnal specified slot
 * Returns an ItemStack containing anything that couldn't fit
 *
 * @param ItemStack Stack of items
 * @param slot Slot number where it will try to store the item
 */
ItemStack ItemStorage::add( ItemStack stack, int slot) {
	if (!stack.empty() && items->isValid(stack.item)) {
		next_sort_mode = SORT_NONE + 1;
		refreshSortTooltip();

		int max_quantity = items->items[stack.item]->max_quantity;
		if (slot > -1) {
			// a slot is specified
			if (storage[slot].item != 0 && storage[slot].item != stack.item) {
				// the proposed slot isn't available
				slot = -1;
			}
		}
		if (slot == -1) {
			// first search of stack to complete if the item is stackable
			int i = 0;
			while (max_quantity > 1 && slot == -1 && i < slot_number) {
				if (storage[i].item == stack.item && storage[i].quantity < max_quantity && storage[i].can_buyback == stack.can_buyback) {
					slot = i;
				}
				i++;
			}
			// then an empty slot
			i = 0;
			while (slot == -1 && i < slot_number) {
				if (storage[i].empty()) {
					slot = i;
				}
				i++;
			}
		}
		if (slot != -1) {
			// Add
			int quantity_added = std::min( stack.quantity, max_quantity - storage[slot].quantity);
			storage[slot].item = stack.item;
			storage[slot].can_buyback = stack.can_buyback;
			storage[slot].quantity += quantity_added;
			stack.quantity -= quantity_added;
			// Add back the remaining, recursivly, until there's no more left to add or we run out of space.
			if (stack.quantity > 0) {
				return add(stack, NO_SLOT);
			}
			// everything added successfully, so return an empty ItemStack
			return ItemStack();
		}
		else {
			// Returns an ItemStack containing the remaining quantity if we run out of space.
			// This stack will likely be dropped on the ground
			return stack;
		}
	}
	return ItemStack();
}

/**
 * Subtract an item from the specified slot, or remove it if it's the last
 *
 * @param slot Slot number
 */
void ItemStorage::subtract(int slot, int quantity) {
	next_sort_mode = SORT_NONE + 1;
	refreshSortTooltip();

	storage[slot].quantity -= quantity;
	if (storage[slot].quantity <= 0) {
		storage[slot].clear();
	}
}

/**
 * Remove a quantity of a given item by its ID
 */
bool ItemStorage::remove(ItemID item, int quantity) {
	if (item == 0)
		return false;

	const int lowest_quantity = INT_MAX;

	next_sort_mode = SORT_NONE + 1;
	refreshSortTooltip();

	while (quantity > 0) {
		int lowest_slot = -1;

		for (int i=0; i<slot_number; i++) {
			if (storage[i].item == item && storage[i].quantity < lowest_quantity) {
				lowest_slot = i;
			}
		}

		// could not find item id, can't remove anything
		if (lowest_slot == -1)
			return false;

		if (quantity <= storage[lowest_slot].quantity) {
			// take from a single stack
			subtract(lowest_slot, quantity);
			quantity = 0;
		}
		else {
			// need to take from another stack
			quantity = quantity - storage[lowest_slot].quantity;
			subtract(lowest_slot, storage[lowest_slot].quantity);
		}
	}
	return true;
}

// used for sorting an array of ItemStack via qsort
// not to be confused with ItemManager::compareItemStack(), which is meant for C++ std::sort
int ItemStorage::compareItemStack (const void *a, const void *b) {
	int currency_compare = compareCurrencyItemStack(a, b);
	if (currency_compare != 0)
		return currency_compare;

	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);

	if ((*i1) > (*i2))
		return 1;
	else
		return -1;
}

int ItemStorage::compareItemStackByType (const void *a, const void *b) {
	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);
	if (items && i1->item != 0 && i2->item != 0) {
		int currency_compare = compareCurrencyItemStack(a, b);
		if (currency_compare != 0)
			return currency_compare;

		Item* item_a = items->items[i1->item];
		Item* item_b = items->items[i2->item];

		if (item_a->type == 0 && item_b->type != 0)
			return 1;
		else if (item_a->type != 0 && item_b->type == 0)
			return -1;
		else if (item_a->type > item_b->type)
			return 1;
		else if (item_a->type == item_b->type && item_a->icon > item_b->icon)
			return 1;
		else if (item_a->type == item_b->type && item_a->icon == item_b->icon && i1->quantity < i2->quantity)
			return 1;
		else if (item_a->type == item_b->type && item_a->icon == item_b->icon && i1->quantity == i2->quantity)
			return compareItemStack(a, b);
		else
			return -1;
	}
	return compareItemStack(a, b);
}

int ItemStorage::compareItemStackByBuyPrice (const void *a, const void *b) {
	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);
	if (items && i1->item != 0 && i2->item != 0) {
		int currency_compare = compareCurrencyItemStack(a, b);
		if (currency_compare != 0)
			return currency_compare;

		Item* item_a = items->items[i1->item];
		Item* item_b = items->items[i2->item];

		int price_a = item_a->getPrice(!ItemManager::USE_VENDOR_RATIO);
		int price_b = item_b->getPrice(!ItemManager::USE_VENDOR_RATIO);
		if (price_a < price_b)
			return 1;
		else if (price_a == price_b)
			return compareItemStackByType(a, b);
		else
			return -1;
	}
	return compareItemStackByType(a, b);
}

int ItemStorage::compareItemStackBySellPrice (const void *a, const void *b) {
	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);
	if (items && i1->item != 0 && i2->item != 0) {
		int currency_compare = compareCurrencyItemStack(a, b);
		if (currency_compare != 0)
			return currency_compare;

		Item* item_a = items->items[i1->item];
		Item* item_b = items->items[i2->item];

		int price_a = item_a->getSellPrice(ItemManager::DEFAULT_SELL_PRICE);
		int price_b = item_b->getSellPrice(ItemManager::DEFAULT_SELL_PRICE);
		if (price_a < price_b)
			return 1;
		else if (price_a == price_b)
			return compareItemStackByType(a, b);
		else
			return -1;
	}
	return compareItemStackByType(a, b);
}

int ItemStorage::compareItemStackByQuality (const void *a, const void *b) {
	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);
	if (items && i1->item != 0 && i2->item != 0) {
		int currency_compare = compareCurrencyItemStack(a, b);
		if (currency_compare != 0)
			return currency_compare;

		Item* item_a = items->items[i1->item];
		Item* item_b = items->items[i2->item];

		if (item_a->quality > item_b->quality)
			return 1;
		else if (item_a->quality == item_b->quality)
			return compareItemStackByType(a, b);
		else
			return -1;
	}
	return compareItemStackByType(a, b);
}

int ItemStorage::compareItemStackByLevel (const void *a, const void *b) {
	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);
	if (items && i1->item != 0 && i2->item != 0) {
		int currency_compare = compareCurrencyItemStack(a, b);
		if (currency_compare != 0)
			return currency_compare;

		Item* item_a = items->items[i1->item];
		Item* item_b = items->items[i2->item];

		if (item_a->level == 0 && item_b->level != 0)
			return 1;
		else if (item_a->level != 0 && item_b->level == 0)
			return -1;
		else if (item_a->level > item_b->level)
			return -1;
		else if (item_a->level == item_b->level)
			return compareItemStackByType(a, b);
		else
			return 1;
	}
	return compareItemStackByType(a, b);
}

int ItemStorage::compareCurrencyItemStack(const void *a, const void *b) {
	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);

	if (eset) {
		if (i1->item == eset->misc.currency_id && i2->item != eset->misc.currency_id)
			return -1;
		else if (i1->item != eset->misc.currency_id && i2->item == eset->misc.currency_id)
			return 1;
		else if (i1->item == eset->misc.currency_id && i2->item == eset->misc.currency_id)
			return (i1->quantity < i2->quantity);
	}

	return 0;
}

void ItemStorage::sort(int mode) {
	if (mode == SORT_ID)
		qsort(storage, slot_number, sizeof(ItemStack), ItemStorage::compareItemStack);
	else if (mode == SORT_TYPE)
		qsort(storage, slot_number, sizeof(ItemStack), ItemStorage::compareItemStackByType);
	else if (mode == SORT_BUY_PRICE)
		qsort(storage, slot_number, sizeof(ItemStack), ItemStorage::compareItemStackByBuyPrice);
	else if (mode == SORT_SELL_PRICE)
		qsort(storage, slot_number, sizeof(ItemStack), ItemStorage::compareItemStackBySellPrice);
	else if (mode == SORT_QUALITY)
		qsort(storage, slot_number, sizeof(ItemStack), ItemStorage::compareItemStackByQuality);
	else if (mode == SORT_LEVEL)
		qsort(storage, slot_number, sizeof(ItemStack), ItemStorage::compareItemStackByLevel);

	// anything else is treated as SORT_NONE and no sorting is done

	refreshSortTooltip();
}

void ItemStorage::sortNext() {
	sort(next_sort_mode);

	next_sort_mode++;
	if (next_sort_mode == SORT_ID)
		next_sort_mode = SORT_NONE + 1;

	refreshSortTooltip();
}

bool ItemStorage::full(ItemStack stack) {
	if (stack.empty() || !items->isValid(stack.item))
		return false;

	for (int i=0; i<slot_number; i++) {
		if (storage[i].item == stack.item && items->isValid(stack.item) && storage[i].quantity < items->items[stack.item]->max_quantity) {
			if (stack.quantity + storage[i].quantity >= items->items[stack.item]->max_quantity) {
				stack.quantity -= storage[i].quantity;
				continue;
			}
			return false;
		}
		if (storage[i].empty()) {
			return false;
		}
	}
	return true;
}

/**
 * Get the number of the specified item carried (not equipped)
 */
int ItemStorage::count(ItemID item) {
	int item_count=0;
	for (int i=0; i<slot_number; i++) {
		if (storage[i].item == item) {
			item_count += storage[i].quantity;
		}
	}
	return item_count;
}

/**
 * Check to see if the given item is equipped
 */
bool ItemStorage::contain(ItemID item, int quantity) {
	int total_quantity = 0;
	for (int i=0; i<slot_number; i++) {
		if (storage[i].item == item)
			total_quantity += storage[i].quantity;
		if (total_quantity >= quantity)
			return true;
	}
	return false;
}

/**
 * Clear slots that contain an item, but have a quantity of 0
 */
void ItemStorage::clean() {
	for (int i=0; i<slot_number; i++) {
		if (storage[i].item > 0 && storage[i].quantity < 1) {
			Utils::logInfo("ItemStorage: Removing item with id %d, which has a quantity of 0",storage[i].item);
			storage[i].clear();
		}
		else if (storage[i].item == 0 && storage[i].quantity != 0) {
			Utils::logInfo("ItemStorage: Removing item with id 0, which has a quantity of %d", storage[i].quantity);
			storage[i].clear();
		}
	}
}

/**
 * Returns true if the storage is empty
 */
bool ItemStorage::empty() {
	if (!storage)
		return true;

	for (int i = 0; i < slot_number; ++i) {
		if (!storage[i].empty())
			return false;
	}

	return true;
}

void ItemStorage::refreshSortTooltip() {
	if (!sort_tooltip)
		return;

	if (next_sort_mode == SORT_TYPE)
		(*sort_tooltip) = msg->get("Sort by: Type");
	else if (next_sort_mode == SORT_QUALITY)
		(*sort_tooltip) = msg->get("Sort by: Quality");
	else if (next_sort_mode == SORT_LEVEL)
		(*sort_tooltip) = msg->get("Sort by: Level");
	else if (next_sort_mode == SORT_SELL_PRICE)
		(*sort_tooltip) = msg->get("Sort by: Sell Price");
}

ItemStorage::~ItemStorage() {
	delete[] storage;
}

