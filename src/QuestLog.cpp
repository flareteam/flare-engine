/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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
 * class QuestLog
 *
 * Helper text to remind the player of active quests
 */

#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "EventManager.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "Menu.h"
#include "MenuLog.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "QuestLog.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

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
	std::vector<std::string> files = mods->list("quests", !ModManager::LIST_FULL_PATHS);
	std::sort(files.begin(), files.end());
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
	if (!infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return;

	quests.resize(quests.size()+1);

	while (infile.next()) {
		if (infile.new_section) {
			if (infile.section == "quest") {
				quest_sections.push_back(std::vector<EventComponent>());
			}
		}

		if (infile.section == "") {
			if (infile.key == "name") {
				// @ATTR name|string|A displayed name for this quest.
				quests.back().name = msg->get(infile.val);
			}
			else if (infile.key == "complete_status") {
				// @ATTR complete_status|string|If this status is set, the quest will be displayed as completed.
				quests.back().complete_status = camp->registerStatus(infile.val);
			}

			continue;
		}

		if (quest_sections.empty())
			continue;

		Event ev;
		if (infile.key == "requires_status") {
			// @ATTR quest.requires_status|list(string)|Quest requires this campaign status
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_status") {
			// @ATTR quest.requires_not_status|list(string)|Quest requires not having this campaign status.
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_level") {
			// @ATTR quest.requires_level|int|Quest requires hero level
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_level") {
			// @ATTR quest.requires_not_level|int|Quest requires not hero level
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_currency") {
			// @ATTR quest.requires_currency|int|Quest requires atleast this much currency
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_currency") {
			// @ATTR quest.requires_not_currency|int|Quest requires no more than this much currency
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_item") {
			// @ATTR quest.requires_item|list(item_id)|Quest requires specific item (not equipped)
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_item") {
			// @ATTR quest.requires_not_item|list(item_id)|Quest requires not having a specific item (not equipped)
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_class") {
			// @ATTR quest.requires_class|predefined_string|Quest requires this base class
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_class") {
			// @ATTR quest.requires_not_class|predefined_string|Quest requires not this base class
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "quest_text") {
			// @ATTR quest.quest_text|string|Text that gets displayed in the Quest log when this quest is active.
			EventComponent ec;
			ec.type = EventComponent::QUEST_TEXT;
			ec.s = msg->get(infile.val);

			// quest group id
			ec.x = static_cast<int>(quests.size()-1);

			ev.components.push_back(ec);
		}
		else {
			Utils::logError("QuestLog: %s is not a valid key.", infile.key.c_str());
		}

		for (size_t i=0; i<ev.components.size(); ++i) {
			if (ev.components[i].type != EventComponent::NONE)
				quest_sections.back().push_back(ev.components[i]);
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
	std::vector<size_t> temp_complete_quest_ids;

	// check quest requirements
	for (size_t i=0; i<quest_sections.size(); i++) {
		bool requirements_met = false;

		for (size_t j=0; j<quest_sections[i].size(); j++) {
			if (quest_sections[i][j].type == EventComponent::QUEST_TEXT) {
				continue;
			}
			else {
				// check requirements
				// break (skip to next dialog node) if any requirement fails
				// if we reach an event that is not a requirement, succeed
				if (!camp->checkAllRequirements(quest_sections[i][j])) {
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

	// check quest completion status
	for (size_t i=0; i<quests.size(); ++i) {
		if (!quests[i].name.empty() && quests[i].complete_status != 0) {
			if (camp->checkStatus(quests[i].complete_status)) {
				temp_complete_quest_ids.push_back(i);
			}
		}
	}

	// check if we actually need to update the quest log
	bool refresh_quest_list = false;
	if (temp_quest_ids != active_quest_ids || temp_complete_quest_ids != complete_quest_ids)
		refresh_quest_list = true;

	// update the quest log
	if (refresh_quest_list) {
		active_quest_ids = temp_quest_ids;
		complete_quest_ids = temp_complete_quest_ids;
		newQuestNotification = true;

		log->clear(MenuLog::TYPE_QUESTS);

		bool complete_header = false;
		for (size_t i = complete_quest_ids.size(); i > 0; i--) {
			if (!complete_header) {
				complete_header = true;
			}
			log->setNextColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED), MenuLog::TYPE_QUESTS);
			log->add(quests[complete_quest_ids[i-1]].name, MenuLog::TYPE_QUESTS, WidgetLog::MSG_UNIQUE);
		}

		if (complete_header) {
			std::stringstream ss;
			ss << msg->get("Completed Quests") << " (" << complete_quest_ids.size() << ")";
			log->setNextStyle(WidgetLog::FONT_BOLD, MenuLog::TYPE_QUESTS);
			log->setNextColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED), MenuLog::TYPE_QUESTS);
			log->add(ss.str(), MenuLog::TYPE_QUESTS, WidgetLog::MSG_UNIQUE);
			if (!active_quest_ids.empty())
				log->addSeparator(MenuLog::TYPE_QUESTS);
		}

		for (size_t i=active_quest_ids.size(); i>0; i--) {
			size_t k = active_quest_ids[i-1];

			size_t i_next = (i > 1) ? i-2 : 0;
			size_t k_next = active_quest_ids[i_next];

			// get the group id of the next active quest
			int next_quest_id = 0;
			for (size_t j=0; j<quest_sections[k_next].size(); j++) {
				if (quest_sections[k_next][j].type == EventComponent::QUEST_TEXT) {
					next_quest_id = quest_sections[k_next][j].x;
					break;
				}
			}

			for (size_t j=0; j<quest_sections[k].size(); j++) {
				if (quest_sections[k][j].type == EventComponent::QUEST_TEXT) {
					log->add(quest_sections[k][j].s, MenuLog::TYPE_QUESTS, WidgetLog::MSG_UNIQUE);

					int quest_id = quest_sections[k][j].x;
					if (next_quest_id != quest_id) {
						if (!quests[quest_id].name.empty()) {
							log->setNextStyle(WidgetLog::FONT_BOLD, MenuLog::TYPE_QUESTS);
							log->add(quests[quest_id].name, MenuLog::TYPE_QUESTS, WidgetLog::MSG_UNIQUE);
						}

						log->addSeparator(MenuLog::TYPE_QUESTS);
					}
					else if (i == 1) {
						if (!quests[quest_id].name.empty()) {
							log->setNextStyle(WidgetLog::FONT_BOLD, MenuLog::TYPE_QUESTS);
							log->add(quests[quest_id].name, MenuLog::TYPE_QUESTS, WidgetLog::MSG_UNIQUE);
						}
					}

					break;
				}
			}
		}
	}
}
