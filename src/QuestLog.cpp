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

QuestLog::QuestLog(MenuLog *_log) {
	log = _log;

	newQuestNotification = false;
	loadAll();
}

QuestLog::~QuestLog() {
}

/**
 * Load each [mod]/quests/index.txt file
 */
void QuestLog::loadAll() {
	// load each items.txt file. Individual item IDs can be overwritten with mods.
	std::vector<std::string> files = mods->list("quests", false);
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

	quest_names.resize(quest_names.size()+1);
	quest_names.back() = "";

	while (infile.next()) {
		if (infile.new_section) {
			if (infile.section == "quest") {
				quests.push_back(std::vector<Event_Component>());
			}
		}

		if (infile.section == "") {
			if (infile.key == "name") {
				// @ATTR name|string|A displayed name for this quest.
				quest_names.back() = msg->get(infile.val);
			}

			continue;
		}

		if (quests.empty())
			continue;

		Event ev;
		if (infile.key == "requires_status") {
			// @ATTR quest.requires_status|string|Quest requires this campaign status
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_status") {
			// @ATTR quest.requires_not_status|string|Quest requires not having this campaign status.
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_level") {
			// @ATTR quest.requires_level|integer|Quest requires hero level
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_level") {
			// @ATTR quest.requires_not_level|integer|Quest requires not hero level
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_currency") {
			// @ATTR quest.requires_currency|integer|Quest requires atleast this much currency
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_currency") {
			// @ATTR quest.requires_not_currency|integer|Quest requires no more than this much currency
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_item") {
			// @ATTR quest.requires_item|integer,...|Quest requires specific item (not equipped)
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_item") {
			// @ATTR quest.requires_not_item|integer,...|Quest requires not having a specific item (not equipped)
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_class") {
			// @ATTR quest.requires_class|string|Quest requires this base class
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_class") {
			// @ATTR quest.requires_not_class|string|Quest requires not this base class
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "quest_text") {
			// @ATTR quest.quest_text|string|Text that gets displayed in the Quest log when this quest is active.
			Event_Component ec;
			ec.type = EC_QUEST_TEXT;
			ec.s = msg->get(infile.val);

			// quest group id
			ec.x = static_cast<int>(quest_names.size()-1);

			ev.components.push_back(ec);
		}
		else {
			logError("QuestLog: %s is not a valid key.", infile.key.c_str());
		}

		for (size_t i=0; i<ev.components.size(); ++i) {
			if (ev.components[i].type != EC_NONE)
				quests.back().push_back(ev.components[i]);
		}
	}
	infile.close();
}

void QuestLog::logic() {
	createQuestList();
}

/**
 * All active quests are placed in the Quest tab of the Log Menu
 */
void QuestLog::createQuestList() {
	std::vector<size_t> temp_quest_ids;

	// check quest requirements
	for (size_t i=0; i<quests.size(); i++) {
		bool requirements_met = false;

		for (size_t j=0; j<quests[i].size(); j++) {
			if (quests[i][j].type == EC_QUEST_TEXT) {
				continue;
			}
			else {
				// check requirements
				// break (skip to next dialog node) if any requirement fails
				// if we reach an event that is not a requirement, succeed
				if (!camp->checkAllRequirements(quests[i][j])) {
					requirements_met = false;
					break;
				}
			}

			requirements_met = true;
		}

		if (requirements_met) {
			// passed requirement checks, add ID to active quest list
			temp_quest_ids.push_back(i);
		}
	}

	// check if we actually need to update the quest log
	bool refresh_quest_list = false;
	if (temp_quest_ids.size() != active_quest_ids.size()) {
		refresh_quest_list = true;
	}
	else {
		for (size_t i=0; i<temp_quest_ids.size(); ++i) {
			if (temp_quest_ids[i] != active_quest_ids[i]) {
				refresh_quest_list = true;
				break;
			}
		}
	}

	// update the quest log
	if (refresh_quest_list) {
		active_quest_ids = temp_quest_ids;
		newQuestNotification = true;

		log->clear(LOG_TYPE_QUESTS);

		for (size_t i=active_quest_ids.size(); i>0; i--) {
			size_t k = active_quest_ids[i-1];

			size_t i_next = (i > 1) ? i-2 : 0;
			size_t k_next = active_quest_ids[i_next];

			// get the group id of the next active quest
			int next_quest_id = 0;
			for (size_t j=0; j<quests[k_next].size(); j++) {
				if (quests[k_next][j].type == EC_QUEST_TEXT) {
					next_quest_id = quests[k_next][j].x;
					break;
				}
			}

			for (size_t j=0; j<quests[k].size(); j++) {
				if (quests[k][j].type == EC_QUEST_TEXT) {
					log->add(quests[k][j].s, LOG_TYPE_QUESTS, false);

					if (next_quest_id != quests[k][j].x) {
						if (quest_names[quests[k][j].x] != "")
							log->add(quest_names[quests[k][j].x], LOG_TYPE_QUESTS, false, NULL, WIDGETLOG_FONT_BOLD);

						log->addSeparator(LOG_TYPE_QUESTS);
					}
					else if (i == 1) {
						if (quest_names[quests[k][j].x] != "")
							log->add(quest_names[quests[k][j].x], LOG_TYPE_QUESTS, false, NULL, WIDGETLOG_FONT_BOLD);
					}

					break;
				}
			}
		}
	}
}
