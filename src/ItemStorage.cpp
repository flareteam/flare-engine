/*
Copyright © 2011-2012 Clint Bellanger

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

void ItemStorage::init(int _slot_number) {
	slot_number = _slot_number;

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
void ItemStorage::setItems(std::string s) {
	s = s + ',';
	for (int i=0; i<slot_number; i++) {
		storage[i].item = popFirstInt(s, ',');
		// check if such item exists to avoid crash if savegame was modified manually
		if (storage[i].item < 0) {
			logError("ItemStorage: Item on position %d has negative id, skipping\n", i);
			storage[i].clear();
		}
		else if ((unsigned)storage[i].item > items->items.size()-1) {
			logError("ItemStorage: Item id (%d) out of bounds 1-%d, marking as unknown\n", storage[i].item, (int)items->items.size());
			items->addUnknownItem(storage[i].item);
		}
		else if (storage[i].item != 0 && items->items[storage[i].item].name == "") {
			logError("ItemStorage: Item with id=%d. found on position %d does not exist, marking as unknown\n", storage[i].item, i);
			items->addUnknownItem(storage[i].item);
		}
	}
}

/**
 * Take the savefile CSV list of items quantities and convert to storage array
 */
void ItemStorage::setQuantities(std::string s) {
	s = s + ',';
	for (int i=0; i<slot_number; i++) {
		storage[i].quantity = popFirstInt(s, ',');
		if (storage[i].quantity < 0) {
			logError("ItemStorage: Items quantity on position %d is negative, setting to zero\n", i);
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
		int max_quantity = items->items[stack.item].max_quantity;
		if (slot > -1) {
			// a slot is specified
			if (storage[slot].item != 0 && storage[slot].item != stack.item) {
				// the proposed slot isn't available
				slot = -1;
			}
		}
		else {
			// first search of stack to complete if the item is stackable
			int i = 0;
			while (max_quantity > 1 && slot == -1 && i < slot_number) {
				if (storage[i].item == stack.item && storage[i].quantity < max_quantity) {
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
			storage[slot].quantity += quantity_added;
			stack.quantity -= quantity_added;
			// Add back the remaining, recursivly, until there's no more left to add or we run out of space.
			if (stack.quantity > 0) {
				return add(stack);
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
 * Substract an item from the specified slot, or remove it if it's the last
 *
 * @param slot Slot number
 */
void ItemStorage::substract(int slot, int quantity) {
	storage[slot].quantity -= quantity;
	if (storage[slot].quantity <= 0) {
		storage[slot].clear();
	}
}

/**
 * Remove one given item
 */
bool ItemStorage::remove(int item) {
	for (int i=0; i<slot_number; i++) {
		if (storage[i].item == item) {
			substract(i, 1);
			return true;
		}
	}
	return false;
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

bool ItemStorage::full(int item) {
	ItemStack stack;
	stack.item = item;
	stack.quantity = 1;
	return full(stack);
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
			logInfo("ItemStorage: Removing item with id %d, which has a quantity of 0\n",storage[i].item);
			storage[i].clear();
		}
		else if (storage[i].item == 0 && storage[i].quantity != 0) {
			logInfo("ItemStorage: Removing item with id 0, which has a quantity of %d\n", storage[i].quantity);
			storage[i].clear();
		}
	}
}

ItemStorage::~ItemStorage() {
	delete[] storage;
}

