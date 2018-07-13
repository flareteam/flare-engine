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
#include "ItemManager.h"
#include "ItemStorage.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"

#include <limits.h>

ItemStorage::ItemStorage()
	: slot_number(0)
	, storage(NULL)
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
		storage[i].item = Parse::popFirstInt(item_list);
		// check if such item exists to avoid crash if savegame was modified manually
		if (storage[i].item < 0) {
			Utils::logError("ItemStorage: Item on position %d has negative id, skipping", i);
			storage[i].clear();
		}
		else if ((items->items.empty() && storage[i].item > 0) || static_cast<unsigned>(storage[i].item) > items->items.size()-1) {
			Utils::logError("ItemStorage: Item id (%d) out of bounds 1-%d, marking as unknown", storage[i].item, static_cast<int>(items->items.size()));
			items->addUnknownItem(storage[i].item);
		}
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
	if (!stack.empty()) {
		if (items->items.empty() || stack.item <= 0 || static_cast<unsigned>(stack.item) > items->items.size()-1) {
			items->addUnknownItem(stack.item);
		}

		int max_quantity = items->items[stack.item].max_quantity;
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
	storage[slot].quantity -= quantity;
	if (storage[slot].quantity <= 0) {
		storage[slot].clear();
	}
}

/**
 * Remove a quantity of a given item by its ID
 */
bool ItemStorage::remove(int item, int quantity) {
	if (item == 0)
		return false;

	const int lowest_quantity = INT_MAX;

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

int compareItemStack (const void *a, const void *b) {
	const ItemStack *i1 = static_cast<const ItemStack*>(a);
	const ItemStack *i2 = static_cast<const ItemStack*>(b);
	if ((*i1) > (*i2))
		return 1;
	else
		return -1;
}

void ItemStorage::sort() {
	qsort(storage, slot_number, sizeof(ItemStack), compareItemStack);
}

bool ItemStorage::full(ItemStack stack) {
	if (stack.empty())
		return false;

	for (int i=0; i<slot_number; i++) {
		if (storage[i].item == stack.item && storage[i].quantity < items->items[stack.item].max_quantity) {
			if (stack.quantity + storage[i].quantity >= items->items[stack.item].max_quantity) {
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
int ItemStorage::count(int item) {
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
bool ItemStorage::contain(int item, int quantity) {
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

ItemStorage::~ItemStorage() {
	delete[] storage;
}

