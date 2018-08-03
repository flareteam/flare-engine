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
#include "Utils.h"

class EventComponent;
class StatBlock;

class CampaignManager {
public:
	typedef std::map<StatusID, std::pair<bool, std::string> > StatusMap;

	CampaignManager();
	~CampaignManager();

	StatusID registerStatus(const std::string& s);
	void setAll(const std::string& s);
	std::string getAll();
	bool checkStatus(const StatusID s);
	void setStatus(const StatusID s);
	void unsetStatus(const StatusID s);
	void resetAllStatuses();
	void getSetStatusStrings(std::vector<std::string>& status_strings);
	bool checkCurrency(int quantity);
	bool checkItem(int item_id);
	void removeCurrency(int quantity);
	void removeItem(int item_id);
	void rewardItem(ItemStack istack);
	void rewardCurrency(int amount);
	void rewardXP(int amount, bool show_message);
	void restoreHPMP(const std::string& s);
	bool checkAllRequirements(const EventComponent& ec);

	std::queue<ItemStack> drop_stack;

	float bonus_xp;		// Fractional XP points not yet awarded (e.g. killing 1 XP enemies with a +25% ring)

	static const bool XP_SHOW_MSG = true;

private:
	StatusMap status;
};


#endif
