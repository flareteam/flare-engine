/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
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
 * class MenuStash
 */

#include "Avatar.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "ItemManager.h"
#include "Menu.h"
#include "MenuStash.h"
#include "MessageEngine.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "TooltipManager.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetSlot.h"
#include "WidgetTabControl.h"

MenuStash::MenuStash()
	: Menu()
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, tab_control(new WidgetTabControl())
	, activetab(STASH_PRIVATE)
	, drag_prev_tab(-1)
	, stock()
	, updated(false)
{

	tab_control->setTabTitle(STASH_PRIVATE, msg->get("Private"));
	if (!pc->stats.permadeath)
		tab_control->setTabTitle(STASH_SHARED, msg->get("Shared"));

	setBackground("images/menus/stash.png");

	int slots_cols = 8; // default if menus/stash.txt::stash_cols not set
	int slots_rows = 8; // default if menus/stash.txt::slots_rows not set

	// Load config settings
	FileParser infile;
	// @CLASS MenuStash|Description of menus/stash.txt
	if (infile.open("menus/stash.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if (infile.key == "close") {
				Point pos = Parse::toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR slots_area|point|Position of the top-left slot.
			else if (infile.key == "slots_area") {
				slots_area.x = Parse::popFirstInt(infile.val);
				slots_area.y = Parse::popFirstInt(infile.val);
			}
			// @ATTR stash_cols|int|The number of columns for the grid of slots.
			else if (infile.key == "stash_cols") {
				slots_cols = std::max(1, Parse::toInt(infile.val));
			}
			// @ATTR stash_rows|int|The number of rows for the grid of slots.
			else if (infile.key == "stash_rows") {
				slots_rows = std::max(1, Parse::toInt(infile.val));
			}
			// @ATTR label_title|label|Position of the "Stash" label.
			else if (infile.key == "label_title") {
				label_title.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR currency|label|Position of the label displaying the amount of currency stored in the stash.
			else if (infile.key == "currency") {
				label_currency.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			else {
				infile.error("MenuStash: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	label_title.setText(msg->get("Stash"));
	label_title.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	label_currency.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	int stash_slots = slots_cols * slots_rows;
	slots_area.w = slots_cols * eset->resolutions.icon_size;
	slots_area.h = slots_rows * eset->resolutions.icon_size;

	stock[STASH_PRIVATE].initGrid(stash_slots, slots_area, slots_cols);
	stock[STASH_SHARED].initGrid(stash_slots, slots_area, slots_cols);

	tablist.add(tab_control);
	tablist_private.setPrevTabList(&tablist);
	tablist_shared.setPrevTabList(&tablist);

	tablist_private.lock();
	tablist_shared.lock();

	for (int i = 0; i < stash_slots; i++) {
		tablist_private.add(stock[STASH_PRIVATE].slots[i]);
		tablist_shared.add(stock[STASH_SHARED].slots[i]);
	}

	align();
}

void MenuStash::align() {
	Menu::align();

	Rect tabs_area = slots_area;
	tabs_area.x += window_area.x;
	tabs_area.y += window_area.y;

	tab_control->setMainArea(tabs_area.x, tabs_area.y - tab_control->getTabHeight());

	closeButton->setPos(window_area.x, window_area.y);
	stock[STASH_PRIVATE].setPos(window_area.x, window_area.y);
	stock[STASH_SHARED].setPos(window_area.x, window_area.y);

	label_title.setPos(window_area.x, window_area.y);
	label_currency.setPos(window_area.x, window_area.y);
}

void MenuStash::logic() {
	if (!visible) return;

	tablist.logic();
	tablist_private.logic();
	tablist_shared.logic();

	// disable tab control if we're dragging something from one of the stash stocks
	if (stock[STASH_PRIVATE].drag_prev_slot == -1 && stock[STASH_SHARED].drag_prev_slot == -1)
		tab_control->logic();

	if (settings->touchscreen && activetab != tab_control->getActiveTab()) {
		tablist_private.defocus();
		tablist_shared.defocus();
	}
	activetab = tab_control->getActiveTab();

	if (activetab == MenuStash::STASH_PRIVATE)
		tablist.setNextTabList(&tablist_private);
	else if (activetab == MenuStash::STASH_SHARED)
		tablist.setNextTabList(&tablist_shared);

	if (settings->touchscreen) {
		if (activetab == MenuStash::STASH_PRIVATE && tablist_private.getCurrent() == -1)
			stock[MenuStash::STASH_PRIVATE].current_slot = NULL;
		else if (activetab == MenuStash::STASH_SHARED && tablist_shared.getCurrent() == -1)
			stock[MenuStash::STASH_SHARED].current_slot = NULL;
	}

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}
}

void MenuStash::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// text overlay
	label_title.render();
	if (!label_currency.isHidden()) {
		label_currency.setText(msg->get("%d %s", stock[activetab].count(eset->misc.currency_id), eset->loot.currency));
		label_currency.render();
	}

	tab_control->render();

	// show stock
	stock[activetab].render();
}

/**
 * Dragging and dropping an item can be used to rearrange the stash
 */
bool MenuStash::drop(const Point& position, ItemStack stack) {
	if (stack.empty()) {
		return true;
	}

	int slot;
	int drag_prev_slot;
	bool success = true;

	items->playSound(stack.item);

	slot = stock[activetab].slotOver(position);
	drag_prev_slot = stock[activetab].drag_prev_slot;

	if (slot == -1) {
		success = add(stack, slot, !ADD_PLAY_SOUND);
	}
	else if (drag_prev_slot != -1) {
		if (stock[activetab][slot].item == stack.item || stock[activetab][slot].empty()) {
			// Drop the stack, merging if needed
			success = add(stack, slot, !ADD_PLAY_SOUND);
		}
		else if (drag_prev_slot != -1 && stock[activetab][drag_prev_slot].empty()) {
			// Check if the previous slot is free (could still be used if SHIFT was used).
			// Swap the two stacks
			itemReturn(stock[activetab][slot]);
			stock[activetab][slot] = stack;
			updated = true;
		}
		else {
			itemReturn(stack);
			updated = true;
		}
	}
	else {
		success = add(stack, slot, !ADD_PLAY_SOUND);
	}

	return success;
}

bool MenuStash::add(ItemStack stack, int slot, bool play_sound) {
	if (stack.empty()) {
		return true;
	}

	if (play_sound) {
		items->playSound(stack.item);
	}

	if (items->items[stack.item].quest_item) {
		pc->logMsg(msg->get("Can not store quest items in the stash."), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}
	else if (items->items[stack.item].no_stash == Item::NO_STASH_ALL) {
		pc->logMsg(msg->get("This item can not be stored in the stash."), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}
	else if (activetab == STASH_PRIVATE && items->items[stack.item].no_stash == Item::NO_STASH_PRIVATE) {
		pc->logMsg(msg->get("This item can not be stored in the private stash."), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}
	else if (activetab == STASH_SHARED && items->items[stack.item].no_stash == Item::NO_STASH_SHARED) {
		pc->logMsg(msg->get("This item can not be stored in the shared stash."), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}

	ItemStack leftover = stock[activetab].add(stack, slot);
	if (!leftover.empty()) {
		if (leftover.quantity != stack.quantity) {
			updated = true;
		}
		pc->logMsg(msg->get("Stash is full."), Avatar::MSG_NORMAL);
		drop_stack.push(leftover);
		return false;
	}
	else {
		updated = true;
	}

	return true;
}

/**
 * Start dragging a vendor item
 * Players can drag an item to their inventory.
 */
ItemStack MenuStash::click(const Point& position) {
	ItemStack stack = stock[activetab].click(position);
	if (settings->touchscreen) {
		if (activetab == STASH_PRIVATE)
			tablist_private.setCurrent(stock[activetab].current_slot);
		else if (activetab == STASH_SHARED)
			tablist_shared.setCurrent(stock[activetab].current_slot);
	}
	return stack;
}

/**
 * Cancel the dragging initiated by the click()
 */
void MenuStash::itemReturn(ItemStack stack) {
	stock[activetab].itemReturn(stack);
}

void MenuStash::renderTooltips(const Point& position) {
	if (!visible || !Utils::isWithinRect(window_area, position))
		return;

	TooltipData tip_data = stock[activetab].checkTooltip(position, &pc->stats, ItemManager::PLAYER_INV);
	tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
}

void MenuStash::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = stock[activetab].drag_prev_slot;
	if (drag_prev_slot > -1) {
		stock[activetab].subtract(drag_prev_slot, quantity);
	}
}

void MenuStash::validate(std::queue<ItemStack>& global_drop_stack) {
	for (int tab = 0; tab < 2; ++tab) {
		for (int i = 0; i < stock[activetab].getSlotNumber(); ++i) {
			if (stock[tab][i].empty())
				continue;

			ItemStack stack = stock[tab][i];
			int no_stash = items->items[stack.item].no_stash;
			if (items->items[stack.item].quest_item || no_stash == Item::NO_STASH_ALL || (tab == STASH_PRIVATE && no_stash == Item::NO_STASH_PRIVATE) || (tab == STASH_SHARED && no_stash == Item::NO_STASH_SHARED)) {
				pc->logMsg(msg->get("Can not store item in stash: %s", items->getItemName(stack.item).c_str()), Avatar::MSG_NORMAL);
				global_drop_stack.push(stack);
				stock[tab][i].clear();
				updated = true;
			}
		}
	}
}

void MenuStash::enableSharedTab(bool permadeath) {
	if (permadeath)
		tab_control->setEnabled(STASH_SHARED, false);
}

void MenuStash::setTab(int tab) {
	if (settings->touchscreen && activetab != tab) {
		tablist_private.defocus();
		tablist_shared.defocus();
	}
	tab_control->setActiveTab(tab);
	activetab = tab;
}

void MenuStash::lockTabControl() {
	tablist_private.setPrevTabList(NULL);
	tablist_shared.setPrevTabList(NULL);
}

void MenuStash::unlockTabControl() {
	tablist_private.setPrevTabList(&tablist);
	tablist_shared.setPrevTabList(&tablist);
}

TabList* MenuStash::getCurrentTabList() {
	if (tablist.getCurrent() != -1)
		return (&tablist);
	else if (tablist_private.getCurrent() != -1) {
		return (&tablist_private);
	}
	else if (tablist_shared.getCurrent() != -1) {
		return (&tablist_shared);
	}

	return NULL;
}

void MenuStash::defocusTabLists() {
	tablist.defocus();
	tablist_private.defocus();
	tablist_shared.defocus();
}

MenuStash::~MenuStash() {
	delete closeButton;
}

