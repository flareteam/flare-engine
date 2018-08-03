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

#include "Avatar.h"
#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "EventManager.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuInventory.h"
#include "MessageEngine.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"

CampaignManager::CampaignManager()
	: bonus_xp(0.0) {
}

StatusID CampaignManager::registerStatus(const std::string& s) {
	if (s.empty())
		return 0;

	StatusID new_id = Utils::hashString(s);

	// check if this status was already registered
	StatusMap::iterator it;
	it = status.find(new_id);
	if (it != status.end())
		return it->first;

	// register a new status
	status[new_id].first = false;
	status[new_id].second = s;
	return new_id;
}

/**
 * Take the savefile campaign= and convert to status array
 */
void CampaignManager::setAll(const std::string& s) {
	std::string str = s + ',';
	std::string token;
	while (!str.empty()) {
		token = Parse::popFirstString(str);
		if (!token.empty())
			setStatus(registerStatus(token));
	}
}

/**
 * Convert status array to savefile campaign= (status csv)
 */
std::string CampaignManager::getAll() {
	std::stringstream ss;
	ss.str("");

	StatusMap::iterator it;
	for (it = status.begin(); it != status.end(); ++it) {
		if (it->second.first)
			ss << it->second.second;

		StatusMap::iterator temp = it;
		temp++;
		if (temp != status.end() && temp->second.first) {
			ss << ',';
		}
	}
	return ss.str();
}

bool CampaignManager::checkStatus(const StatusID s) {
	StatusMap::iterator it;
	it = status.find(s);
	if (it != status.end() && it->second.first)
		return true;

	return false;
}

void CampaignManager::setStatus(const StatusID s) {
	// if it's already set, don't set it again
	if (checkStatus(s)) return;

	status[s].first = true;
	pc->stats.check_title = true;
}

void CampaignManager::unsetStatus(const StatusID s) {
	// if it's already unset, don't unset it again
	if (!checkStatus(s)) return;

	status[s].first = false;
	pc->stats.check_title = true;
}

void CampaignManager::resetAllStatuses() {
	StatusMap::iterator it;
	for (it = status.begin(); it != status.end(); ++it) {
		it->second.first = false;
	}
}

void CampaignManager::getSetStatusStrings(std::vector<std::string>& status_strings) {
	StatusMap::iterator it;
	for (it = status.begin(); it != status.end(); ++it) {
		if (it->second.first)
			status_strings.push_back(it->second.second);
	}
}

bool CampaignManager::checkCurrency(int quantity) {
	return menu->inv->inventory[MenuInventory::CARRIED].contain(eset->misc.currency_id, quantity);
}

bool CampaignManager::checkItem(int item_id) {
	if (menu->inv->inventory[MenuInventory::CARRIED].contain(item_id, 1))
		return true;
	else
		return menu->inv->inventory[MenuInventory::EQUIPMENT].contain(item_id, 1);
}

void CampaignManager::removeCurrency(int quantity) {
	int max_amount = std::min(quantity, menu->inv->currency);

	if (max_amount > 0) {
		menu->inv->removeCurrency(max_amount);
		pc->logMsg(msg->get("%d %s removed.", max_amount, eset->loot.currency), Avatar::MSG_UNIQUE);
		items->playSound(eset->misc.currency_id);
	}
}

void CampaignManager::removeItem(int item_id) {
	if (item_id < 0 || static_cast<unsigned>(item_id) >= items->items.size()) return;

	if (menu->inv->remove(item_id)) {
		pc->logMsg(msg->get("%s removed.", items->getItemName(item_id)), Avatar::MSG_UNIQUE);
		items->playSound(item_id);
	}
}

void CampaignManager::rewardItem(ItemStack istack) {
	if (istack.empty())
		return;

	menu->inv->add(istack, MenuInventory::CARRIED, ItemStorage::NO_SLOT, MenuInventory::ADD_PLAY_SOUND, MenuInventory::ADD_AUTO_EQUIP);

	if (istack.item != eset->misc.currency_id) {
		if (istack.quantity <= 1)
			pc->logMsg(msg->get("You receive %s.", items->getItemName(istack.item)), Avatar::MSG_UNIQUE);
		if (istack.quantity > 1)
			pc->logMsg(msg->get("You receive %s x%d.", items->getItemName(istack.item), istack.quantity), Avatar::MSG_UNIQUE);
	}
}

void CampaignManager::rewardCurrency(int amount) {
	ItemStack stack;
	stack.item = eset->misc.currency_id;
	stack.quantity = amount;

	pc->logMsg(msg->get("You receive %d %s.", amount, eset->loot.currency), Avatar::MSG_UNIQUE);
	rewardItem(stack);
}

void CampaignManager::rewardXP(int amount, bool show_message) {
	bonus_xp += (static_cast<float>(amount) * (100.0f + static_cast<float>(pc->stats.get(Stats::XP_GAIN)))) / 100.0f;
	pc->stats.addXP(static_cast<int>(bonus_xp));
	bonus_xp -= static_cast<float>(static_cast<int>(bonus_xp));
	pc->stats.refresh_stats = true;
	if (show_message) pc->logMsg(msg->get("You receive %d XP.", amount), Avatar::MSG_UNIQUE);
}

void CampaignManager::restoreHPMP(const std::string& s) {
	if (s == "hp") {
		pc->stats.hp = pc->stats.get(Stats::HP_MAX);
		pc->logMsg(msg->get("HP restored."), Avatar::MSG_UNIQUE);
	}
	else if (s == "mp") {
		pc->stats.mp = pc->stats.get(Stats::MP_MAX);
		pc->logMsg(msg->get("MP restored."), Avatar::MSG_UNIQUE);
	}
	else if (s == "hpmp") {
		pc->stats.hp = pc->stats.get(Stats::HP_MAX);
		pc->stats.mp = pc->stats.get(Stats::MP_MAX);
		pc->logMsg(msg->get("HP and MP restored."), Avatar::MSG_UNIQUE);
	}
	else if (s == "status") {
		pc->stats.effects.clearNegativeEffects();
		pc->logMsg(msg->get("Negative effects removed."), Avatar::MSG_UNIQUE);
	}
	else if (s == "all") {
		pc->stats.hp = pc->stats.get(Stats::HP_MAX);
		pc->stats.mp = pc->stats.get(Stats::MP_MAX);
		pc->stats.effects.clearNegativeEffects();
		pc->logMsg(msg->get("HP and MP restored, negative effects removed"), Avatar::MSG_UNIQUE);
	}
}

bool CampaignManager::checkAllRequirements(const EventComponent& ec) {
	if (ec.type == EventComponent::REQUIRES_STATUS) {
		if (checkStatus(ec.status))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_STATUS) {
		if (!checkStatus(ec.status))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_CURRENCY) {
		if (checkCurrency(ec.x))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_CURRENCY) {
		if (!checkCurrency(ec.x))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_ITEM) {
		if (checkItem(ec.x))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_ITEM) {
		if (!checkItem(ec.x))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_LEVEL) {
		if (pc->stats.level >= ec.x)
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_LEVEL) {
		if (pc->stats.level < ec.x)
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_CLASS) {
		if (pc->stats.character_class == ec.s)
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_CLASS) {
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
