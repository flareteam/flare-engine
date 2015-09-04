/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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
 * class CampaignManager
 *
 * Contains data for story mode
 */

#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "MenuItemStorage.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"

CampaignManager::CampaignManager()
	: status()
	, log_msg("")
	, carried_items(NULL)
	, currency(NULL)
	, hero(NULL)
	, quest_update(true)
	, bonus_xp(0.0) {
}

void CampaignManager::clearAll() {
	// clear campaign data
	status.clear();
}

/**
 * Take the savefile campaign= and convert to status array
 */
void CampaignManager::setAll(std::string s) {
	std::string str = s + ',';
	std::string token;
	while (str != "") {
		token = popFirstString(str, ',');
		if (token != "") this->setStatus(token);
	}
	quest_update = true;
}

/**
 * Convert status array to savefile campaign= (status csv)
 */
std::string CampaignManager::getAll() {
	std::stringstream ss;
	ss.str("");
	for (unsigned int i=0; i < status.size(); i++) {
		ss << status[i];
		if (i < status.size()-1) ss << ',';
	}
	return ss.str();
}

bool CampaignManager::checkStatus(std::string s) {

	// avoid searching empty statuses
	if (s == "") return false;

	for (unsigned int i=0; i < status.size(); i++) {
		if (status[i] == s) return true;
	}
	return false;
}

void CampaignManager::setStatus(std::string s) {

	// avoid adding empty statuses
	if (s == "") return;

	// if it's already set, don't add it again
	if (checkStatus(s)) return;

	status.push_back(s);
	quest_update = true;
	hero->check_title = true;
}

void CampaignManager::unsetStatus(std::string s) {

	// avoid searching empty statuses
	if (s == "") return;

	std::vector<std::string>::iterator it;
	// see http://stackoverflow.com/a/223405
	for (it = status.end(); it != status.begin();) {
		--it;
		if ((*it) == s) {
			it = status.erase(it);
			quest_update = true;
			return;
		}
		hero->check_title = true;
	}
}

bool CampaignManager::checkCurrency(int quantity) {
	return carried_items->contain(CURRENCY_ID, quantity);
}

bool CampaignManager::checkItem(int item_id) {
	return carried_items->contain(item_id);
}

void CampaignManager::removeCurrency(int quantity) {
	for (int i=0; i<quantity; ++i) {
		carried_items->remove(CURRENCY_ID);
	}
	addMsg(msg->get("%d %s removed.", quantity, CURRENCY));
	items->playSound(CURRENCY_ID);
}

void CampaignManager::removeItem(int item_id) {
	if (item_id < 0 || static_cast<unsigned>(item_id) >= items->items.size()) return;

	carried_items->remove(item_id);
	addMsg(msg->get("%s removed.", items->getItemName(item_id)));
	items->playSound(item_id);
}

void CampaignManager::rewardItem(ItemStack istack) {
	if (istack.empty())
		return;

	if (carried_items->full(istack.item)) {
		drop_stack.push(istack);
	}
	else {
		carried_items->add(istack);

		if (istack.item != CURRENCY_ID) {
			if (istack.quantity <= 1)
				addMsg(msg->get("You receive %s.", items->getItemName(istack.item)));
			if (istack.quantity > 1)
				addMsg(msg->get("You receive %s x%d.", istack.quantity, items->getItemName(istack.item)));

			items->playSound(istack.item);
		}

		// if this item has a power, place it on the action bar if possible
		if (items->items[istack.item].type == "consumable" && items->items[istack.item].power > 0) {
			menu_act->addPower(items->items[istack.item].power, 0);
		}

	}
}

void CampaignManager::rewardCurrency(int amount) {
	ItemStack stack;
	stack.item = CURRENCY_ID;
	stack.quantity = amount;
	if (!carried_items->full(stack.item))
		addMsg(msg->get("You receive %d %s.", amount, CURRENCY));
	rewardItem(stack);
	items->playSound(CURRENCY_ID);
}

void CampaignManager::rewardXP(int amount, bool show_message) {
	bonus_xp += (static_cast<float>(amount) * (100.0f + static_cast<float>(hero->get(STAT_XP_GAIN)))) / 100.0f;
	hero->addXP(static_cast<int>(bonus_xp));
	bonus_xp -= static_cast<float>(static_cast<int>(bonus_xp));
	hero->refresh_stats = true;
	if (show_message) addMsg(msg->get("You receive %d XP.", amount));
}

void CampaignManager::restoreHPMP(std::string s) {
	if (s == "hp") {
		hero->hp = hero->get(STAT_HP_MAX);
		addMsg(msg->get("HP restored."));
	}
	else if (s == "mp") {
		hero->mp = hero->get(STAT_MP_MAX);
		addMsg(msg->get("MP restored."));
	}
	else if (s == "hpmp") {
		hero->hp = hero->get(STAT_HP_MAX);
		hero->mp = hero->get(STAT_MP_MAX);
		addMsg(msg->get("HP and MP restored."));
	}
	else if (s == "status") {
		hero->effects.clearNegativeEffects();
		addMsg(msg->get("Negative effects removed."));
	}
	else if (s == "all") {
		hero->hp = hero->get(STAT_HP_MAX);
		hero->mp = hero->get(STAT_MP_MAX);
		hero->effects.clearNegativeEffects();
		addMsg(msg->get("HP and MP restored, negative effects removed"));
	}
}

void CampaignManager::addMsg(const std::string& new_msg) {
	if (log_msg != "") log_msg += " ";
	log_msg += new_msg;
}

CampaignManager::~CampaignManager() {
}
