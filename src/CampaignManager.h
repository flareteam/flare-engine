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
 * class CampaignManager
 *
 * Contains data for story mode
 */


#ifndef CAMPAIGN_MANAGER_H
#define CAMPAIGN_MANAGER_H

#include "CommonIncludes.h"
#include "ItemManager.h"

class Event_Component;
class StatBlock;

class CampaignManager {
public:
	CampaignManager();
	~CampaignManager();

	void setAll(const std::string& s);
	std::string getAll();
	bool checkStatus(const std::string& s);
	void setStatus(const std::string& s);
	void unsetStatus(const std::string& s);
	bool checkCurrency(int quantity);
	bool checkItem(int item_id);
	void removeCurrency(int quantity);
	void removeItem(int item_id);
	void rewardItem(ItemStack istack);
	void rewardCurrency(int amount);
	void rewardXP(int amount, bool show_message);
	void restoreHPMP(const std::string& s);
	bool checkAllRequirements(const Event_Component& ec);

	std::vector<std::string> status;
	std::queue<ItemStack> drop_stack;

	float bonus_xp;		// Fractional XP points not yet awarded (e.g. killing 1 XP enemies with a +25% ring)

	static const bool XP_SHOW_MSG = true;
};


#endif
