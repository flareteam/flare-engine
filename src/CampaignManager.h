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

class Status {
public:
	enum {
		EXPR_OP_UNKNOWN = 0,
		EXPR_OP_COMP_EQ,
		EXPR_OP_COMP_GTEQ,
		EXPR_OP_COMP_LTEQ,
		EXPR_OP_COMP_NEQ,
		EXPR_OP_COMP_GT,
		EXPR_OP_COMP_LT,
		EXPR_OP_ASSIGN,
		EXPR_OP_ADD,
		EXPR_OP_SUB,
		EXPR_OP_MUL,
		EXPR_OP_DIV,
		EXPR_OP_MOD,
		EXPR_OP_NONE,
	};

	bool enabled;
	bool is_expression;
	std::string id;
	int value;

	Status()
		: enabled(false)
		, is_expression(false)
		, id("")
		, value(0)
	{}
};

class CampaignManager {
public:
	CampaignManager();
	~CampaignManager();

	StatusID registerStatus(const std::string& s);
	void readAll(const std::string& s);
	std::string writeAll();
	bool checkStatus(const StatusID s);
	void setStatus(const StatusID s);
	void unsetStatus(const StatusID s);
	void resetAllStatuses();
	void getSetStatusStrings(std::vector<std::string>& status_strings);
	bool checkCurrency(int quantity);
	bool checkItem(ItemStack istack);
	void removeCurrency(int quantity);
	void removeItem(ItemStack istack);
	void rewardItem(ItemStack istack);
	void rewardCurrency(int amount);
	void rewardXP(float amount, bool show_message);
	void restoreHPMP(const std::string& s);
	bool checkAllRequirements(const EventComponent& ec);
	bool checkRequirementsInVector(const std::vector<EventComponent>& ec_vec);

	void randomStatusAppend(const StatusID s);
	void randomStatusClear();
	void randomStatusRoll();
	void randomStatusSet();
	void randomStatusUnset();

	bool checkStatusEx(const EventComponent *ec);
	void setStatusEx(const EventComponent *ec);
	const Status& getStatusByID(const std::string& s);
	int getStatusValue(StatusID id);

	void parseStatus(const std::string expr_raw, EventComponent *e);
	void parseStatusList(std::string& s, int type, std::vector<EventComponent>& dest);

	std::queue<ItemStack> drop_stack;

	float bonus_xp;		// Fractional XP points not yet awarded (e.g. killing 1 XP enemies with a +25% ring)

	static const bool XP_SHOW_MSG = true;

private:
	typedef std::map<StatusID, Status> StatusMap;

	StatusMap status;

	std::vector<StatusID> random_status_pool;
	StatusID random_status;
};


#endif
