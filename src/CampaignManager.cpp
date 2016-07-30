/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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
 * class CampaignManager
 *
 * Contains data for story mode
 */

#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuInventory.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"

CampaignManager::CampaignManager()
	: status()
	, bonus_xp(0.0) {
}

/**
 * Take the savefile campaign= and convert to status array
 */
void CampaignManager::setAll(const std::string& s) {
	std::string str = s + ',';
	std::string token;
	while (str != "") {
		token = popFirstString(str);
		if (token != "") this->setStatus(token);
	}
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

bool CampaignManager::checkStatus(const std::string& s) {

	// avoid searching empty statuses
	if (s == "") return false;

	for (unsigned int i=0; i < status.size(); i++) {
		if (status[i] == s) return true;
	}
	return false;
}

void CampaignManager::setStatus(const std::string& s) {

	// avoid adding empty statuses
	if (s == "") return;

	// if it's already set, don't add it again
	if (checkStatus(s)) return;

	status.push_back(s);
	pc->stats.check_title = true;
}

void CampaignManager::unsetStatus(const std::string& s) {

	// avoid searching empty statuses
	if (s == "") return;

	std::vector<std::string>::iterator it;
	// see http://stackoverflow.com/a/223405
	for (it = status.end(); it != status.begin();) {
		--it;
		if ((*it) == s) {
			it = status.erase(it);
			return;
		}
		pc->stats.check_title = true;
	}
}

bool CampaignManager::checkCurrency(int quantity) {
	return menu->inv->inventory[CARRIED].contain(CURRENCY_ID, quantity);
}

bool CampaignManager::checkItem(int item_id) {
	if (menu->inv->inventory[CARRIED].contain(item_id))
		return true;
	else
		return menu->inv->inventory[EQUIPMENT].contain(item_id);
}

void CampaignManager::removeCurrency(int quantity) {
	menu->inv->removeCurrency(quantity);
	pc->log_msg.push(msg->get("%d %s removed.", quantity, CURRENCY));
	items->playSound(CURRENCY_ID);
}

void CampaignManager::removeItem(int item_id) {
	if (item_id < 0 || static_cast<unsigned>(item_id) >= items->items.size()) return;

	menu->inv->remove(item_id);
	pc->log_msg.push(msg->get("%s removed.", items->getItemName(item_id)));
	items->playSound(item_id);
}

void CampaignManager::rewardItem(ItemStack istack) {
	if (istack.empty())
		return;

	menu->inv->add(istack, CARRIED, -1, true, true);

	if (istack.item != CURRENCY_ID) {
		if (istack.quantity <= 1)
			pc->log_msg.push(msg->get("You receive %s.", items->getItemName(istack.item)));
		if (istack.quantity > 1)
			pc->log_msg.push(msg->get("You receive %s x%d.", istack.quantity, items->getItemName(istack.item)));
	}
}

void CampaignManager::rewardCurrency(int amount) {
	ItemStack stack;
	stack.item = CURRENCY_ID;
	stack.quantity = amount;

	pc->log_msg.push(msg->get("You receive %d %s.", amount, CURRENCY));
	rewardItem(stack);
}

void CampaignManager::rewardXP(int amount, bool show_message) {
	bonus_xp += (static_cast<float>(amount) * (100.0f + static_cast<float>(pc->stats.get(STAT_XP_GAIN)))) / 100.0f;
	pc->stats.addXP(static_cast<int>(bonus_xp));
	bonus_xp -= static_cast<float>(static_cast<int>(bonus_xp));
	pc->stats.refresh_stats = true;
	if (show_message) pc->log_msg.push(msg->get("You receive %d XP.", amount));
}

void CampaignManager::restoreHPMP(const std::string& s) {
	if (s == "hp") {
		pc->stats.hp = pc->stats.get(STAT_HP_MAX);
		pc->log_msg.push(msg->get("HP restored."));
	}
	else if (s == "mp") {
		pc->stats.mp = pc->stats.get(STAT_MP_MAX);
		pc->log_msg.push(msg->get("MP restored."));
	}
	else if (s == "hpmp") {
		pc->stats.hp = pc->stats.get(STAT_HP_MAX);
		pc->stats.mp = pc->stats.get(STAT_MP_MAX);
		pc->log_msg.push(msg->get("HP and MP restored."));
	}
	else if (s == "status") {
		pc->stats.effects.clearNegativeEffects();
		pc->log_msg.push(msg->get("Negative effects removed."));
	}
	else if (s == "all") {
		pc->stats.hp = pc->stats.get(STAT_HP_MAX);
		pc->stats.mp = pc->stats.get(STAT_MP_MAX);
		pc->stats.effects.clearNegativeEffects();
		pc->log_msg.push(msg->get("HP and MP restored, negative effects removed"));
	}
}

bool CampaignManager::checkAllRequirements(const Event_Component& ec) {
	if (ec.type == EC_REQUIRES_STATUS) {
		if (checkStatus(ec.s))
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_STATUS) {
		if (!checkStatus(ec.s))
			return true;
	}
	else if (ec.type == EC_REQUIRES_CURRENCY) {
		if (checkCurrency(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_CURRENCY) {
		if (!checkCurrency(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_ITEM) {
		if (checkItem(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_ITEM) {
		if (!checkItem(ec.x))
			return true;
	}
	else if (ec.type == EC_REQUIRES_LEVEL) {
		if (pc->stats.level >= ec.x)
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_LEVEL) {
		if (pc->stats.level < ec.x)
			return true;
	}
	else if (ec.type == EC_REQUIRES_CLASS) {
		if (pc->stats.character_class == ec.s)
			return true;
	}
	else if (ec.type == EC_REQUIRES_NOT_CLASS) {
		if (pc->stats.character_class != ec.s)
			return true;
	}
	else {
		// Event component is not a requirement check
		// treat it as if the "requirement" was met
		return true;
	}

	// requirement check failed
	return false;
}

CampaignManager::~CampaignManager() {
}
