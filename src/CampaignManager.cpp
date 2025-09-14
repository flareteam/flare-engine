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
#include "UtilsMath.h"
#include "UtilsParsing.h"

CampaignManager::CampaignManager()
	: bonus_xp(0.0)
	, random_status(0) {
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
	std::string output("");

	StatusMap::iterator it;
	for (it = status.begin(); it != status.end(); ++it) {
		if (it->second.first)
			output += it->second.second;

		StatusMap::iterator temp = it;
		++temp;
		if (temp != status.end() && temp->second.first) {
			output += ',';
		}
	}
	return output;
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

bool CampaignManager::checkItem(ItemStack istack) {
	if (menu->inv->inventory[MenuInventory::CARRIED].contain(istack.item, istack.quantity))
		return true;
	else
		return menu->inv->equipmentContain(istack.item, istack.quantity);
}

void CampaignManager::removeCurrency(int quantity) {
	int max_amount = std::min(quantity, menu->inv->currency);

	if (max_amount > 0) {
		menu->inv->removeCurrency(max_amount);
		pc->logMsg(msg->getv("%d %s removed.", max_amount, eset->loot.currency.c_str()), Avatar::MSG_UNIQUE);
		items->playSound(eset->misc.currency_id);
	}
}

void CampaignManager::removeItem(ItemStack istack) {
	if (istack.empty())
		return;

	if (istack.item == eset->misc.currency_id) {
		removeCurrency(istack.quantity);
		return;
	}

	int item_count = menu->inv->inventory[MenuInventory::CARRIED].count(istack.item) + menu->inv->inventory[MenuInventory::EQUIPMENT].count(istack.item);
	int max_amount = std::min(item_count, istack.quantity);

	if (menu->inv->remove(istack.item, max_amount)) {
		if (max_amount > 1)
			pc->logMsg(msg->getv("%s x%d removed.", items->getItemName(istack.item).c_str(), max_amount), Avatar::MSG_UNIQUE);
		else if (max_amount == 1)
			pc->logMsg(msg->getv("%s removed.", items->getItemName(istack.item).c_str()), Avatar::MSG_UNIQUE);

		if (max_amount > 0)
			items->playSound(istack.item);
	}
}

void CampaignManager::rewardItem(ItemStack istack) {
	if (istack.empty())
		return;

	menu->inv->add(istack, MenuInventory::CARRIED, ItemStorage::NO_SLOT, MenuInventory::ADD_PLAY_SOUND, MenuInventory::ADD_AUTO_EQUIP);

	if (istack.item == eset->misc.currency_id) {
		pc->logMsg(msg->getv("You receive %d %s.", istack.quantity, eset->loot.currency.c_str()), Avatar::MSG_UNIQUE);
	}
	else {
		if (istack.quantity > 1)
			pc->logMsg(msg->getv("You receive %s x%d.", items->getItemName(istack.item).c_str(), istack.quantity), Avatar::MSG_UNIQUE);
		else if (istack.quantity == 1)
			pc->logMsg(msg->getv("You receive %s.", items->getItemName(istack.item).c_str()), Avatar::MSG_UNIQUE);
	}
}

void CampaignManager::rewardCurrency(int amount) {
	ItemStack stack;
	stack.item = eset->misc.currency_id;
	stack.quantity = amount;

	rewardItem(stack);
}

void CampaignManager::rewardXP(float amount, bool show_message) {
	if (pc->block_xp_gain)
		return;

	bonus_xp += (amount * (100.0f + static_cast<float>(pc->stats.get(Stats::XP_GAIN)))) / 100.0f;

	int whole_xp = static_cast<int>(bonus_xp);
	pc->stats.addXP(whole_xp);
	bonus_xp -= static_cast<float>(whole_xp); // remainder

	pc->stats.refresh_stats = true;

	if (show_message)
		pc->logMsg(msg->getv("You receive %d XP.", static_cast<int>(amount)), Avatar::MSG_UNIQUE);
}

void CampaignManager::restoreHPMP(const std::string& s) {
	std::string restore_str = s;
	std::string restore_mode = Parse::popFirstString(restore_str);

	while (!restore_mode.empty()) {
		if (restore_mode == "hp") {
			pc->stats.hp = pc->stats.get(Stats::HP_MAX);
			pc->logMsg(msg->get("HP restored."), Avatar::MSG_UNIQUE);
		}
		else if (restore_mode == "mp") {
			pc->stats.mp = pc->stats.get(Stats::MP_MAX);
			pc->logMsg(msg->get("MP restored."), Avatar::MSG_UNIQUE);
		}
		else if (restore_mode == "hpmp") {
			pc->stats.hp = pc->stats.get(Stats::HP_MAX);
			pc->stats.mp = pc->stats.get(Stats::MP_MAX);
			pc->logMsg(msg->get("HP and MP restored."), Avatar::MSG_UNIQUE);
		}
		else if (restore_mode == "status") {
			pc->stats.effects.clearNegativeEffects(Effect::RESIST_ALL);
			pc->logMsg(msg->get("Negative effects removed."), Avatar::MSG_UNIQUE);
		}
		else if (restore_mode == "all") {
			pc->stats.hp = pc->stats.get(Stats::HP_MAX);
			pc->stats.mp = pc->stats.get(Stats::MP_MAX);
			pc->stats.effects.clearNegativeEffects(Effect::RESIST_ALL);
			pc->logMsg(msg->get("HP and MP restored, negative effects removed"), Avatar::MSG_UNIQUE);

			for (size_t i = 0; i < eset->resource_stats.list.size(); ++i) {
				pc->stats.resource_stats[i] = pc->stats.getResourceStat(i, EngineSettings::ResourceStats::STAT_BASE);
				pc->logMsg(eset->resource_stats.list[i].text_log_restore, Avatar::MSG_UNIQUE);
			}
		}
		else {
			for (size_t i = 0; i < eset->resource_stats.list.size(); ++i) {
				if (restore_mode == eset->resource_stats.list[i].ids[EngineSettings::ResourceStats::STAT_BASE]) {
					pc->stats.resource_stats[i] = pc->stats.getResourceStat(i, EngineSettings::ResourceStats::STAT_BASE);
					pc->logMsg(eset->resource_stats.list[i].text_log_restore, Avatar::MSG_UNIQUE);
				}
			}
		}

		restore_mode = Parse::popFirstString(restore_str);
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
		if (checkCurrency(ec.data[0].Int))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_CURRENCY) {
		if (!checkCurrency(ec.data[0].Int))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_ITEM) {
		if (checkItem(ItemStack(ec.id, ec.data[0].Int)))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_ITEM) {
		if (!checkItem(ItemStack(ec.id, ec.data[0].Int)))
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_LEVEL) {
		if (pc->stats.level >= ec.data[0].Int)
			return true;
	}
	else if (ec.type == EventComponent::REQUIRES_NOT_LEVEL) {
		if (pc->stats.level < ec.data[0].Int)
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

bool CampaignManager::checkRequirementsInVector(const std::vector<EventComponent>& ec_vec) {
	for (size_t i = 0; i < ec_vec.size(); ++i) {
		if (!checkAllRequirements(ec_vec[i]))
			return false;
	}

	return true;
}

void CampaignManager::randomStatusAppend(const StatusID s) {
	if (std::find(random_status_pool.begin(), random_status_pool.end(), s) == random_status_pool.end()) {
		if (random_status_pool.empty())
			random_status = s;

		random_status_pool.push_back(s);
	}
}

void CampaignManager::randomStatusClear() {
	random_status_pool.clear();
	random_status = 0;
}

void CampaignManager::randomStatusRoll() {
	if (random_status_pool.empty())
		return;

	random_status = random_status_pool[Math::randBetween(0, static_cast<int>(random_status_pool.size()) - 1)];
}

void CampaignManager::randomStatusSet() {
	if (random_status_pool.empty())
		return;

	setStatus(random_status);
}

void CampaignManager::randomStatusUnset() {
	if (random_status_pool.empty())
		return;

	unsetStatus(random_status);
}

CampaignManager::~CampaignManager() {
}
