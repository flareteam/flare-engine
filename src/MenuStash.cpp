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

MenuStashTab::MenuStashTab(const std::string& _id, const std::string& _name, const std::string& _filename, bool _is_private)
	: id(_id)
	, name(_name)
	, filename(_filename)
	, is_private(_is_private)
	, is_legacy(false)
	, updated(false)
	, stock()
{}

MenuStashTab::~MenuStashTab() {
}

MenuStash::MenuStash()
	: Menu()
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, tab_control(new WidgetTabControl())
	, activetab(0)
	, tabs()
{
	setBackground("images/menus/stash.png");

	int slots_cols = 8; // default if menus/stash.txt::stash_cols not set
	int slots_rows = 8; // default if menus/stash.txt::slots_rows not set

	// Load config settings
	FileParser infile;
	// @CLASS MenuStash|Description of menus/stash.txt
	if (infile.open("menus/stash.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (infile.new_section) {
				if (infile.section == "tab") {
					std::stringstream id_str, name_str, filename_str;

					// default tab settings
					id_str << "Stash " << tabs.size();
					name_str << msg->get("Stash") << " " << tabs.size();
					filename_str << "stash_tab_" << tabs.size() << ".txt";

					tabs.push_back(MenuStashTab(id_str.str(), name_str.str(), filename_str.str(), MenuStashTab::IS_PRIVATE));
				}
			}

			if (infile.section == "") {
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
			else if (infile.section == "tab") {
				// don't load any settings for the "Private" and "Shared" tabs
				if (tabs.back().is_legacy)
					continue;

				if (infile.key == "name") {
					// @ATTR tab.name|["Private", "Shared", string]|The displayed name of this tab. It is also used to determine the filename of the stash file that the engine will create. 'Private' and 'Shared' will use their legacy filenames for compatibility.
					std::string name_str = infile.val;
					tabs.back().id = name_str;
					tabs.back().name = msg->get(name_str);

					if (name_str == "Private") {
						tabs.back().filename = "stash_HC.txt";
						tabs.back().is_private = true;
						tabs.back().is_legacy = true;
					}
					else if (name_str == "Shared") {
						tabs.back().filename = "stash.txt";
						tabs.back().is_private = false;
						tabs.back().is_legacy = true;
					}
					else {
						// generate a filename
						std::stringstream ss;
						ss << std::hex << "stash_" << Utils::hashString(name_str) << ".txt";
						tabs.back().filename = ss.str();
					}
				}
				else if (infile.key == "is_private") {
					// @ATTR tab.is_private|bool|If true, this stash will not be shared across other saves.
					tabs.back().is_private = Parse::toBool(infile.val);
				}
				else {
					infile.error("MenuStash: '%s' is not a valid key.", infile.key.c_str());
				}
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

	tablist.add(tab_control);

	// default tabs if none are defined in the config file
	if (tabs.empty()) {
		tabs.push_back(MenuStashTab("Private", msg->get("Private"), "stash_HC.txt", MenuStashTab::IS_PRIVATE));
		tabs.push_back(MenuStashTab("Shared", msg->get("Shared"), "stash.txt", !MenuStashTab::IS_PRIVATE));
	}

	// ensure that characters have access to at least one private stash
	// this needs to be done because permadeath characters ONLY have access to private stashes
	bool private_exists = false;
	for (size_t i = 0; i < tabs.size(); ++i) {
		if (tabs[i].is_private) {
			private_exists = true;
			break;
		}
	}
	if (!private_exists) {
		tabs.push_back(MenuStashTab("Private", msg->get("Private"), "stash_HC.txt", MenuStashTab::IS_PRIVATE));
	}

	for (size_t i = 0; i < tabs.size(); ++i) {
		tabs[i].stock.initGrid(stash_slots, slots_area, slots_cols);
		tab_control->setTabTitle(static_cast<unsigned>(i), tabs[i].name);

		// "tablist" keyboard navigation
		tabs[i].tablist.setPrevTabList(&tablist);
		tabs[i].tablist.lock();
		for (int j = 0; j < stash_slots; ++j) {
			tabs[i].tablist.add(tabs[i].stock.slots[j]);
		}
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

	for (size_t i = 0; i < tabs.size(); ++i) {
		tabs[i].stock.setPos(window_area.x, window_area.y);
	}

	label_title.setPos(window_area.x, window_area.y);
	label_currency.setPos(window_area.x, window_area.y);
}

void MenuStash::logic() {
	if (!visible) return;

	tablist.logic();
	for (size_t i = 0; i < tabs.size(); ++i) {
		tabs[i].tablist.logic();
	}

	// disable tab control if we're dragging something from one of the stash stocks
	bool dragging = false;
	for (size_t i = 0; i < tabs.size(); ++i) {
		if (tabs[i].stock.drag_prev_slot != -1) {
			dragging = true;
			break;
		}
	}
	if (!dragging)
		tab_control->logic();

	if (settings->touchscreen && activetab != static_cast<size_t>(tab_control->getActiveTab())) {
		for (size_t i = 0; i < tabs.size(); ++i) {
			tabs[i].tablist.defocus();
		}
	}
	activetab = tab_control->getActiveTab();

	tablist.setNextTabList(&(tabs[activetab].tablist));

	if (settings->touchscreen) {
		if (tabs[activetab].tablist.getCurrent() == -1)
			tabs[activetab].stock.current_slot = NULL;
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
		label_currency.setText(msg->get("%d %s", tabs[activetab].stock.count(eset->misc.currency_id), eset->loot.currency));
		label_currency.render();
	}

	tab_control->render();

	// show stock
	tabs[activetab].stock.render();
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

	slot = tabs[activetab].stock.slotOver(position);
	drag_prev_slot = tabs[activetab].stock.drag_prev_slot;

	if (slot == -1) {
		success = add(stack, slot, !ADD_PLAY_SOUND);
	}
	else if (drag_prev_slot != -1) {
		if (tabs[activetab].stock[slot].item == stack.item || tabs[activetab].stock[slot].empty()) {
			// Drop the stack, merging if needed
			success = add(stack, slot, !ADD_PLAY_SOUND);
		}
		else if (tabs[activetab].stock[drag_prev_slot].empty()) {
			// Check if the previous slot is free (could still be used if SHIFT was used).
			// Swap the two stacks
			itemReturn(tabs[activetab].stock[slot]);
			tabs[activetab].stock[slot] = stack;
			tabs[activetab].updated = true;
		}
		else {
			itemReturn(stack);
			tabs[activetab].updated = true;
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

	if (items->items[stack.item].no_stash == Item::NO_STASH_ALL) {
		pc->logMsg(msg->get("This item can not be stored in the stash."), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}
	else if (tabs[activetab].is_private && items->items[stack.item].no_stash == Item::NO_STASH_PRIVATE) {
		pc->logMsg(msg->get("This item can not be stored in the private stash."), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}
	else if (!tabs[activetab].is_private && items->items[stack.item].no_stash == Item::NO_STASH_SHARED) {
		pc->logMsg(msg->get("This item can not be stored in the shared stash."), Avatar::MSG_NORMAL);
		drop_stack.push(stack);
		return false;
	}

	ItemStack leftover = tabs[activetab].stock.add(stack, slot);
	if (!leftover.empty()) {
		if (leftover.quantity != stack.quantity) {
			tabs[activetab].updated = true;
		}
		pc->logMsg(msg->get("Stash is full."), Avatar::MSG_NORMAL);
		drop_stack.push(leftover);
		return false;
	}
	else {
		tabs[activetab].updated = true;
	}

	return true;
}

/**
 * Start dragging a vendor item
 * Players can drag an item to their inventory.
 */
ItemStack MenuStash::click(const Point& position) {
	ItemStack stack = tabs[activetab].stock.click(position);
	if (settings->touchscreen) {
		tabs[activetab].tablist.setCurrent(tabs[activetab].stock.current_slot);
	}
	return stack;
}

/**
 * Cancel the dragging initiated by the click()
 */
void MenuStash::itemReturn(ItemStack stack) {
	tabs[activetab].stock.itemReturn(stack);
}

void MenuStash::renderTooltips(const Point& position) {
	if (!visible || !Utils::isWithinRect(window_area, position))
		return;

	TooltipData tip_data = tabs[activetab].stock.checkTooltip(position, &pc->stats, ItemManager::PLAYER_INV);
	tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
}

void MenuStash::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = tabs[activetab].stock.drag_prev_slot;
	if (drag_prev_slot > -1) {
		tabs[activetab].stock.subtract(drag_prev_slot, quantity);
	}
}

void MenuStash::validate(std::queue<ItemStack>& global_drop_stack) {
	for (size_t tab = 0; tab < tabs.size(); ++tab) {
		for (int i = 0; i < tabs[activetab].stock.getSlotNumber(); ++i) {
			if (tabs[tab].stock[i].empty())
				continue;

			ItemStack stack = tabs[tab].stock[i];
			int no_stash = items->items[stack.item].no_stash;
			if (no_stash == Item::NO_STASH_ALL || (tabs[tab].is_private && no_stash == Item::NO_STASH_PRIVATE) || (!tabs[tab].is_private && no_stash == Item::NO_STASH_SHARED)) {
				pc->logMsg(msg->get("Can not store item in stash: %s", items->getItemName(stack.item).c_str()), Avatar::MSG_NORMAL);
				global_drop_stack.push(stack);
				tabs[tab].stock[i].clear();
				tabs[tab].updated = true;
			}
		}
	}
}

bool MenuStash::checkUpdates() {
	bool updated = false;

	for (size_t i = 0; i < tabs.size(); ++i) {
		if (tabs[i].updated) {
			tabs[i].updated = false;

			if (eset->misc.save_onstash == EngineSettings::Misc::SAVE_ONSTASH_ALL)
				updated = true;
			else if (tabs[i].is_private && eset->misc.save_onstash == EngineSettings::Misc::SAVE_ONSTASH_PRIVATE)
				updated = true;
			else if (!tabs[i].is_private && eset->misc.save_onstash == EngineSettings::Misc::SAVE_ONSTASH_SHARED)
				updated = true;
		}
	}

	return updated;
}

void MenuStash::enableSharedTab(bool permadeath) {
	for (size_t i = 0; i < tabs.size(); ++i) {
		tab_control->setEnabled(static_cast<unsigned>(i), (!permadeath || tabs[i].is_private));
	}
}

void MenuStash::setTab(size_t tab) {
	if (settings->touchscreen && activetab != tab) {
		for (size_t i = 0; i < tabs.size(); ++i) {
			tabs[i].tablist.defocus();
		}
	}
	if (tab >= tabs.size()) {
		tab_control->setActiveTab(0);
		activetab = 0;
	}
	else {
		tab_control->setActiveTab(static_cast<unsigned>(tab));
		activetab = tab;
	}
}

size_t MenuStash::getTab() {
	return activetab;
}

void MenuStash::lockTabControl() {
	for (size_t i = 0; i < tabs.size(); ++i) {
		tabs[i].tablist.setPrevTabList(NULL);
	}
}

void MenuStash::unlockTabControl() {
	for (size_t i = 0; i < tabs.size(); ++i) {
		tabs[i].tablist.setPrevTabList(&tablist);
	}
}

TabList* MenuStash::getCurrentTabList() {
	if (tablist.getCurrent() != -1) {
		return (&tablist);
	}
	else {
		for (size_t i = 0; i < tabs.size(); ++i) {
			if (tabs[i].tablist.getCurrent() != -1)
				return (&(tabs[i].tablist));
		}
	}

	return NULL;
}

void MenuStash::defocusTabLists() {
	tablist.defocus();
	for (size_t i = 0; i < tabs.size(); ++i) {
		tabs[i].tablist.defocus();
	}
}

MenuStash::~MenuStash() {
	delete closeButton;
	delete tab_control;
}

