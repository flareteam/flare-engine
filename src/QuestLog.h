/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014-2015 Justin Jacobs

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
 * class QuestLog
 *
 * Helper text to remind the player of active quests
 */

#ifndef QUEST_LOG_H
#define QUEST_LOG_H

#include "CommonIncludes.h"
#include "Utils.h"

class MenuLog;

class QuestLog {
private:
	class Quest {
	public:
		std::string name;
		StatusID complete_status;
	};

	MenuLog *log;

	// inner vector is a chain of events per quest, outer vector is a
	// list of quests.
	std::vector<std::vector<EventComponent> >quest_sections;

	std::vector<size_t> active_quest_ids;
	std::vector<size_t> complete_quest_ids;
	std::vector<Quest> quests;

public:
	explicit QuestLog(MenuLog *_log);
	~QuestLog();
	void loadAll();
	void load(const std::string& filename);
	void logic();
	void createQuestList();
	bool newQuestNotification;
};

#endif
