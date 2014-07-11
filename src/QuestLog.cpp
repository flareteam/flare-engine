/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller

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

#include "CommonIncludes.h"
#include "FileParser.h"
#include "Menu.h"
#include "MenuLog.h"
#include "QuestLog.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"

using namespace std;


QuestLog::QuestLog(MenuLog *_log) {
	log = _log;

	newQuestNotification = false;
	resetQuestNotification = false;
	loadAll();
}

QuestLog::~QuestLog() {
}

/**
 * Load each [mod]/quests/index.txt file
 */
void QuestLog::loadAll() {
	// load each items.txt file. Individual item IDs can be overwritten with mods.
	vector<string> files = mods->list("quests", false);
	for (unsigned int i = 0; i < files.size(); i++)
		load(files[i]);
}

/**
 * Load the quests in the specific quest file.
 * Searches for the last-defined such file in all mods
 *
 * @param filename The quest file name and extension, no path
 */
void QuestLog::load(const std::string& filename) {
	FileParser infile;
	// @CLASS QuestLog|Description of quest files in quests/
	if (!infile.open(filename))
		return;

	while (infile.next()) {
		if (infile.new_section) {
			if (infile.section == "quest")
				quests.push_back(vector<Event_Component>());
		}
		// @ATTR quest.requires_status|string|Quest requires this campaign status
		// @ATTR quest.requires_not_status|string|Quest requires not having this campaign status.
		// @ATTR quest.quest_text|string|Text that gets displayed in the Quest log when this quest is active.
		if (!quests.empty()) {
			Event_Component ev;
			ev.type = infile.key;
			ev.s = msg->get(infile.val);
			quests.back().push_back(ev);
		}
	}
	infile.close();
}

void QuestLog::logic() {
	if (camp->quest_update) {
		resetQuestNotification = true;
		camp->quest_update = false;
		createQuestList();
	}
}

/**
 * All active quests are placed in the Quest tab of the Log Menu
 */
void QuestLog::createQuestList() {
	log->clear(LOG_TYPE_QUESTS);

	for (unsigned int i=0; i<quests.size(); i++) {
		for (unsigned int j=0; j<quests[i].size(); j++) {

			// check requirements
			// break (skip to next dialog node) if any requirement fails
			// if we reach an event that is not a requirement, succeed

			if (quests[i][j].type == "requires_status") {
				if (!camp->checkStatus(quests[i][j].s)) break;
			}
			else if (quests[i][j].type == "requires_not_status") {
				if (camp->checkStatus(quests[i][j].s)) break;
			}
			else if (quests[i][j].type == "quest_text") {
				log->add(quests[i][j].s, LOG_TYPE_QUESTS, false);
				newQuestNotification = true;
				break;
			}
			else if (quests[i][j].type == "") {
				break;
			}
		}
	}
}
