/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MenuVendor
 */

#include "Avatar.h"
#include "Entity.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "ItemManager.h"
#include "ItemStorage.h"
#include "Menu.h"
#include "MenuVendor.h"
#include "MessageEngine.h"
#include "NPC.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "TooltipManager.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetSlot.h"
#include "WidgetTabControl.h"
#include "WidgetTooltip.h"

MenuVendor::MenuVendor()
	: Menu()
	, closeButton(new WidgetButton(WidgetButton::CLOSE_FILE))
	, tabControl(new WidgetTabControl())
	, slots_cols(1)
	, slots_rows(1)
	, activetab(ItemManager::VENDOR_BUY)
	, sort_stock_buy(ItemStorage::SORT_NONE)
	, sort_stock_craft(ItemStorage::SORT_NONE)
	, tip (new WidgetTooltip())
	, npc(NULL)
	, buyback_stock()
	, sell_enabled(true)
{
	tabControl->setupTab(ItemManager::VENDOR_BUY, msg->get("Inventory"), &tablist_tabs[ItemManager::VENDOR_BUY]);
	tabControl->setupTab(ItemManager::VENDOR_SELL, msg->get("Buyback"), &tablist_tabs[ItemManager::VENDOR_SELL]);
	tabControl->setupTab(ItemManager::VENDOR_CRAFT, msg->get("Craft"), &tablist_tabs[ItemManager::VENDOR_CRAFT]);

	// Load config settings
	FileParser infile;
	// @CLASS MenuVendor|Description of menus/vendor.txt
	if(infile.open("menus/vendor.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = Parse::toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR slots_area|point|Position of the top-left slot.
			else if(infile.key == "slots_area") {
				slots_area.x = Parse::popFirstInt(infile.val);
				slots_area.y = Parse::popFirstInt(infile.val);
			}
			// @ATTR vendor_cols|int|The number of columns in the grid of slots.
			else if (infile.key == "vendor_cols") {
				slots_cols = std::max(1, Parse::toInt(infile.val));
			}
			// @ATTR vendor_rows|int|The number of rows in the grid of slots.
			else if (infile.key == "vendor_rows") {
				slots_rows = std::max(1, Parse::toInt(infile.val));
			}
			// @ATTR label_title|label|The position of the text that displays the NPC's name.
			else if (infile.key == "label_title") {
				label_vendor.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR sort_stock_buy|["none", "type", "quality", "level", "price", "id"]|Sorts all vendors' "Buy" stock by the given metric.
			else if (infile.key == "sort_stock_buy") {
				if (infile.val == "none")
					sort_stock_buy = ItemStorage::SORT_NONE;
				else if (infile.val == "type")
					sort_stock_buy = ItemStorage::SORT_TYPE;
				else if (infile.val == "quality")
					sort_stock_buy = ItemStorage::SORT_QUALITY;
				else if (infile.val == "level")
					sort_stock_buy = ItemStorage::SORT_LEVEL;
				else if (infile.val == "price")
					sort_stock_buy = ItemStorage::SORT_BUY_PRICE;
				else if (infile.val == "id")
					sort_stock_buy = ItemStorage::SORT_ID;
			}
			// @ATTR sort_stock_craft|["none", "type", "quality", "level", "price", "id"]|Sorts all vendors' "Craft" stock by the given metric.
			else if (infile.key == "sort_stock_craft") {
				if (infile.val == "none")
					sort_stock_craft = ItemStorage::SORT_NONE;
				else if (infile.val == "type")
					sort_stock_craft = ItemStorage::SORT_TYPE;
				else if (infile.val == "quality")
					sort_stock_craft = ItemStorage::SORT_QUALITY;
				else if (infile.val == "level")
					sort_stock_craft = ItemStorage::SORT_LEVEL;
				else if (infile.val == "price")
					sort_stock_craft = ItemStorage::SORT_BUY_PRICE;
				else if (infile.val == "id")
					sort_stock_craft = ItemStorage::SORT_ID;
			}
			else {
				infile.error("MenuVendor: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	label_vendor.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	VENDOR_SLOTS = slots_cols * slots_rows;
	slots_area.w = slots_cols * eset->resolutions.icon_size;
	slots_area.h = slots_rows * eset->resolutions.icon_size;

	for (int i = 0; i < TAB_COUNT; ++i) {
		stock[i].initGrid(VENDOR_SLOTS, slots_area, slots_cols);
		tablist_tabs[i].lock();

		for (unsigned j = 0; j < VENDOR_SLOTS; ++j) {
			tablist_tabs[i].add(stock[i].slots[j]);
		}

	}

	// since we never subtract items from the craft tab, we set the max quantity to 1 in order to hide the quantity display on the icons
	stock[ItemManager::VENDOR_CRAFT].max_quantity_is_one = true;

	// we need to behave like we're holding Shift without actually doing so
	stock[ItemManager::VENDOR_CRAFT].click_subtracts_item = false;

	tablist.setNextTabList(&tablist_tabs[ItemManager::VENDOR_BUY]);

	if (!background)
		setBackground("images/menus/vendor.png");

	align();
}

void MenuVendor::align() {
	Menu::align();

	label_vendor.setPos(window_area.x, window_area.y);

	Rect tabs_area = slots_area;
	tabs_area.x += window_area.x;
	tabs_area.y += window_area.y;

	tabControl->setMainArea(tabs_area.x, tabs_area.y - tabControl->getTabHeight(), tabs_area.w);

	closeButton->setPos(window_area.x, window_area.y);

	stock[ItemManager::VENDOR_BUY].setPos(window_area.x, window_area.y);
	stock[ItemManager::VENDOR_SELL].setPos(window_area.x, window_area.y);
	stock[ItemManager::VENDOR_CRAFT].setPos(window_area.x, window_area.y);
}

void MenuVendor::logic() {
	if (!visible) return;

	tablist.logic();
	for (int i = 0; i < TAB_COUNT; ++i) {
		tablist_tabs[i].logic();
	}

	if (stock[ItemManager::VENDOR_BUY].drag_prev_slot == -1 && stock[ItemManager::VENDOR_SELL].drag_prev_slot == -1 && stock[ItemManager::VENDOR_CRAFT].drag_prev_slot == -1)
		tabControl->logic();

	if (inpt->usingTouchscreen() && activetab != tabControl->getActiveTab()) {
		for (int i = 0; i < TAB_COUNT; ++i) {
			tablist_tabs[i].defocus();
		}
	}
	activetab = tabControl->getActiveTab();

	for (int i = 0; i < TAB_COUNT; ++i) {
		if (activetab == i) {
			tablist_tabs[i].unlock();
			tablist.setNextTabList(&tablist_tabs[i]);
		}
		else {
			tablist_tabs[i].lock();
		}
	}

	if (inpt->usingTouchscreen()) {
		for (int i = 0; i < TAB_COUNT; ++i) {
			if (activetab == i && tablist_tabs[i].getCurrent() == -1) {
				stock[i].current_slot = NULL;
			}
		}
	}

	if (closeButton->checkClick()) {
		setNPC(NULL);
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}
}

void MenuVendor::setTab(int tab) {
	if (inpt->usingTouchscreen() && activetab != tab) {
		for (int i = 0; i < TAB_COUNT; ++i) {
			tablist_tabs[i].defocus();
		}
	}
	tabControl->setActiveTab(tab);
	activetab = tab;
}

void MenuVendor::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// text overlay
	label_vendor.render();

	// render tabs
	tabControl->render();

	// show stock
	stock[activetab].render();
}

/**
 * Start dragging a vendor item
 * Players can drag an item to their inventory to purchase.
 */
ItemStack MenuVendor::click(const Point& position) {
	ItemStack stack = stock[activetab].click(position);
	saveInventory();
	if (inpt->usingTouchscreen()) {
		for (int i = 0; i < TAB_COUNT; ++i) {
			if (activetab == i) {
				tablist_tabs[i].setCurrent(stock[i].current_slot);
			}
		}
	}
	return stack;
}

/**
 * Cancel the dragging initiated by the clic()
 */
void MenuVendor::itemReturn(ItemStack stack) {
	if (activetab == ItemManager::VENDOR_CRAFT) {
		resetDrag();
		return;
	}

	stock[activetab].itemReturn(stack);
	saveInventory();
}

void MenuVendor::add(ItemStack stack) {
	stack.can_buyback = true;

	// Remove the first item stack to make room
	if (stock[ItemManager::VENDOR_SELL].full(stack)) {
		stock[ItemManager::VENDOR_SELL][0].clear();
		moveEmptySlotsToEnd(ItemManager::VENDOR_SELL);
	}
	items->playSound(stack.item);
	stock[ItemManager::VENDOR_SELL].add(stack, ItemStorage::NO_SLOT);
	saveInventory();
}

void MenuVendor::renderTooltips(const Point& position) {
	if (!visible || !Utils::isWithinRect(window_area, position))
		return;

	TooltipData tip_data = stock[activetab].checkTooltip(position, &pc->stats, activetab, ItemManager::TOOLTIP_INPUT_HINT);
	tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
}

/**
 * Save changes to the inventory back to the NPC
 * For persistent stock amounts and buyback (at least until
 * the player leaves this map)
 */
void MenuVendor::saveInventory() {
	if (npc) {
		for (unsigned i=0; i<VENDOR_SLOTS; i++) {
			npc->stock[i] = stock[ItemManager::VENDOR_BUY][i];
			buyback_stock[npc->filename][i] = stock[ItemManager::VENDOR_SELL][i];
			npc->craft_stock[i] = stock[ItemManager::VENDOR_CRAFT][i];
		}
	}
	else {
		Utils::logError("MenuVendor: saveInventory() failed, unknown NPC.");
	}
}

void MenuVendor::moveEmptySlotsToEnd(int type) {
	for (unsigned i=0; i<VENDOR_SLOTS; i++) {
		ItemStack temp = stock[type][i];
		stock[type][i].clear();
		if (!temp.empty())
			stock[type].add(temp, ItemStorage::NO_SLOT);
	}
}

void MenuVendor::setNPC(NPC* _npc) {
	npc = _npc;

	if (_npc == NULL) {
		visible = false;
		return;
	}

	label_vendor.setText(msg->get("Vendor") + " - " + npc->name);

	buyback_stock[npc->filename].init(NPC::VENDOR_MAX_STOCK);

	for (unsigned i=0; i<VENDOR_SLOTS; i++) {
		stock[ItemManager::VENDOR_BUY][i] = npc->stock[i];
		if (npc->reset_buyback) {
			// this occurs on the first interaction with an NPC after map load
			if (eset->misc.keep_buyback_on_map_change)
				buyback_stock[npc->filename][i].can_buyback = false;
			else
				buyback_stock[npc->filename][i].clear();
		}
		stock[ItemManager::VENDOR_SELL][i] = buyback_stock[npc->filename][i];

		stock[ItemManager::VENDOR_CRAFT][i] = npc->craft_stock[i];
		stock[ItemManager::VENDOR_CRAFT][i].quantity = 1;
	}
	npc->reset_buyback = false;

	for (int i = 0; i < TAB_COUNT; ++i) {
		moveEmptySlotsToEnd(i);
		tabControl->setEnabled(i, npc->vendor_tab_enabled[i]);
	}

	sell_enabled = npc->vendor_tab_enabled[ItemManager::VENDOR_SELL];

	for (int i = 0; i < TAB_COUNT; ++i) {
		if (npc->vendor_tab_enabled[i]) {
			setTab(i);
			break;
		}
	}

	stock[ItemManager::VENDOR_BUY].sort(sort_stock_buy);
	stock[ItemManager::VENDOR_CRAFT].sort(sort_stock_craft);

	if (!visible) {
		visible = true;
		snd->play(sfx_open, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
		npc->playSoundIntro();
	}

	align();
}

void MenuVendor::removeFromPrevSlot(int quantity) {
	if (activetab == ItemManager::VENDOR_CRAFT) {
		resetDrag();
		return;
	}

	int drag_prev_slot = stock[activetab].drag_prev_slot;
	if (drag_prev_slot > -1) {
		stock[activetab].subtract(drag_prev_slot, quantity);
		saveInventory();
	}
}

void MenuVendor::lockTabControl() {
	for (int i = 0; i < TAB_COUNT; ++i) {
		tablist_tabs[i].setPrevTabList(NULL);
	}
}

void MenuVendor::unlockTabControl() {
	for (int i = 0; i < TAB_COUNT; ++i) {
		tablist_tabs[i].setPrevTabList(&tablist);
	}
}

TabList* MenuVendor::getCurrentTabList() {
	if (tablist.getCurrent() != -1)
		return (&tablist);

	for (int i = 0; i < TAB_COUNT; ++i) {
		if (tablist_tabs[i].getCurrent() != -1) {
			return (&tablist_tabs[i]);
		}
	}

	return NULL;
}

void MenuVendor::defocusTabLists() {
	tablist.defocus();
	for (int i = 0; i < TAB_COUNT; ++i) {
		tablist_tabs[i].defocus();
	}
}

void MenuVendor::resetDrag() {
	for (int i = 0; i < TAB_COUNT; ++i) {
		stock[i].drag_prev_slot = -1;
	}
}

MenuVendor::~MenuVendor() {
	delete closeButton;
	delete tabControl;
	delete tip;
}

