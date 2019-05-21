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
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetSlot.h"
#include "WidgetTabControl.h"
#include "WidgetTooltip.h"

MenuVendor::MenuVendor()
	: Menu()
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, tabControl(new WidgetTabControl())
	, slots_cols(1)
	, slots_rows(1)
	, activetab(ItemManager::VENDOR_BUY)
	, tip (new WidgetTooltip())
	, npc(NULL)
	, buyback_stock() {
	setBackground("images/menus/vendor.png");

	tabControl->setTabTitle(ItemManager::VENDOR_BUY, msg->get("Inventory"));
	tabControl->setTabTitle(ItemManager::VENDOR_SELL, msg->get("Buyback"));

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

	stock[ItemManager::VENDOR_BUY].initGrid(VENDOR_SLOTS, slots_area, slots_cols);
	stock[ItemManager::VENDOR_SELL].initGrid(VENDOR_SLOTS, slots_area, slots_cols);

	tablist.add(tabControl);
	tablist_buy.setPrevTabList(&tablist);
	tablist_sell.setPrevTabList(&tablist);

	tablist_buy.lock();
	tablist_sell.lock();

	for (unsigned i = 0; i < VENDOR_SLOTS; i++) {
		tablist_buy.add(stock[ItemManager::VENDOR_BUY].slots[i]);
	}
	for (unsigned i = 0; i < VENDOR_SLOTS; i++) {
		tablist_sell.add(stock[ItemManager::VENDOR_SELL].slots[i]);
	}

	align();
}

void MenuVendor::align() {
	Menu::align();

	label_vendor.setPos(window_area.x, window_area.y);

	Rect tabs_area = slots_area;
	tabs_area.x += window_area.x;
	tabs_area.y += window_area.y;

	tabControl->setMainArea(tabs_area.x, tabs_area.y - tabControl->getTabHeight());

	closeButton->setPos(window_area.x, window_area.y);

	stock[ItemManager::VENDOR_BUY].setPos(window_area.x, window_area.y);
	stock[ItemManager::VENDOR_SELL].setPos(window_area.x, window_area.y);
}

void MenuVendor::logic() {
	if (!visible) return;

	tablist.logic();
	tablist_buy.logic();
	tablist_sell.logic();

	if (stock[ItemManager::VENDOR_BUY].drag_prev_slot == -1 && stock[ItemManager::VENDOR_SELL].drag_prev_slot == -1)
		tabControl->logic();

	if (settings->touchscreen && activetab != tabControl->getActiveTab()) {
		tablist_buy.defocus();
		tablist_sell.defocus();
	}
	activetab = tabControl->getActiveTab();

	if (activetab == ItemManager::VENDOR_BUY)
		tablist.setNextTabList(&tablist_buy);
	else if (activetab == ItemManager::VENDOR_SELL)
		tablist.setNextTabList(&tablist_sell);

	if (settings->touchscreen) {
		if (activetab == ItemManager::VENDOR_BUY && tablist_buy.getCurrent() == -1)
			stock[ItemManager::VENDOR_BUY].current_slot = NULL;
		else if (activetab == ItemManager::VENDOR_SELL && tablist_sell.getCurrent() == -1)
			stock[ItemManager::VENDOR_SELL].current_slot = NULL;
	}

	if (closeButton->checkClick()) {
		setNPC(NULL);
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}
}

void MenuVendor::setTab(int tab) {
	if (settings->touchscreen && activetab != tab) {
		tablist_buy.defocus();
		tablist_sell.defocus();
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
	if (settings->touchscreen) {
		if (activetab == ItemManager::VENDOR_BUY)
			tablist_buy.setCurrent(stock[activetab].current_slot);
		else if (activetab == ItemManager::VENDOR_SELL)
			tablist_sell.setCurrent(stock[activetab].current_slot);
	}
	return stack;
}

/**
 * Cancel the dragging initiated by the clic()
 */
void MenuVendor::itemReturn(ItemStack stack) {
	items->playSound(stack.item);
	stock[activetab].itemReturn(stack);
	saveInventory();
}

void MenuVendor::add(ItemStack stack) {
	stack.can_buyback = true;

	// Remove the first item stack to make room
	if (stock[ItemManager::VENDOR_SELL].full(stack)) {
		stock[ItemManager::VENDOR_SELL][0].clear();
		sort(ItemManager::VENDOR_SELL);
	}
	items->playSound(stack.item);
	stock[ItemManager::VENDOR_SELL].add(stack, ItemStorage::NO_SLOT);
	saveInventory();
}

void MenuVendor::renderTooltips(const Point& position) {
	if (!visible || !Utils::isWithinRect(window_area, position))
		return;

	int vendor_view = (activetab == ItemManager::VENDOR_BUY) ? ItemManager::VENDOR_BUY : ItemManager::VENDOR_SELL;
	TooltipData tip_data = stock[activetab].checkTooltip(position, &pc->stats, vendor_view);
	tip->render(tip_data, position, TooltipData::STYLE_FLOAT);
}

/**
 * Save changes to the inventory back to the NPC
 * For persistent stock amounts and buyback (at least until
 * the player leaves this map)
 */
void MenuVendor::saveInventory() {
	for (unsigned i=0; i<VENDOR_SLOTS; i++) {
		if (npc) {
			npc->stock[i] = stock[ItemManager::VENDOR_BUY][i];
			buyback_stock[npc->filename][i] = stock[ItemManager::VENDOR_SELL][i];
		}
	}

}

void MenuVendor::sort(int type) {
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

	setTab(ItemManager::VENDOR_BUY);

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
	}
	npc->reset_buyback = false;

	sort(ItemManager::VENDOR_BUY);
	sort(ItemManager::VENDOR_SELL);

	if (!visible) {
		visible = true;
		snd->play(sfx_open, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
		npc->playSoundIntro();
	}
}

void MenuVendor::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = stock[activetab].drag_prev_slot;
	if (drag_prev_slot > -1) {
		stock[activetab].subtract(drag_prev_slot, quantity);
		saveInventory();
	}
}

void MenuVendor::lockTabControl() {
	tablist_buy.setPrevTabList(NULL);
	tablist_sell.setPrevTabList(NULL);
}

void MenuVendor::unlockTabControl() {
	tablist_buy.setPrevTabList(&tablist);
	tablist_sell.setPrevTabList(&tablist);
}

TabList* MenuVendor::getCurrentTabList() {
	if (tablist.getCurrent() != -1)
		return (&tablist);
	else if (tablist_buy.getCurrent() != -1) {
		return (&tablist_buy);
	}
	else if (tablist_sell.getCurrent() != -1) {
		return (&tablist_sell);
	}

	return NULL;
}

void MenuVendor::defocusTabLists() {
	tablist.defocus();
	tablist_buy.defocus();
	tablist_sell.defocus();
}

MenuVendor::~MenuVendor() {
	delete closeButton;
	delete tabControl;
	delete tip;
}

