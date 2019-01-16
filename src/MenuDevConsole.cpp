/*
Copyright © 2014-2016 Justin Jacobs

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
 * class MenuDevConsole
 */

#include "Avatar.h"
#include "CampaignManager.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "EventManager.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "MapRenderer.h"
#include "MenuActionBar.h"
#include "MenuDevConsole.h"
#include "MenuManager.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetInput.h"
#include "WidgetLog.h"

#include <limits>
#include <math.h>

MenuDevConsole::MenuDevConsole()
	: Menu()
	, first_open(false)
	, input_scrollback_pos(0)
{
	distance_timer.setDuration(settings->max_frames_per_sec);

	button_close = new WidgetButton("images/menus/buttons/button_x.png");
	tablist.add(button_close);

	input_box = new WidgetInput("images/menus/input_console.png");
	tablist.add(input_box);

	button_confirm = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_confirm->setLabel(msg->get("Execute"));
	tablist.add(button_confirm);

	// Load config settings
	FileParser infile;
	// @CLASS MenuDevConsole|Description of menus/devconsole.txt
	if(infile.open("menus/devconsole.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = Parse::toPoint(infile.val);
				button_close->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR label_title|label|Position of the "Developer Console" label.
			else if(infile.key == "label_title") {
				label.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR confirm|point|Position of the "Execute" button.
			else if(infile.key == "confirm") {
				Point pos = Parse::toPoint(infile.val);
				button_confirm->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR input|point|Position of the command entry widget.
			else if(infile.key == "input") {
				Point pos = Parse::toPoint(infile.val);
				input_box->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR history|rectangle|Position and dimensions of the command history.
			else if(infile.key == "history") history_area = Parse::toRect(infile.val);

			else infile.error("MenuDevConsole: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	label.setText(msg->get("Developer Console"));
	label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	log_history = new WidgetLog(history_area.w, history_area.h);
	log_history->setBasePos(history_area.x, history_area.y, Utils::ALIGN_TOPLEFT);
	tablist.add(log_history->getWidget());

	setBackground("images/menus/dev_console.png");

	align();
	reset();
	input_box->accept_to_defocus = false;
}

MenuDevConsole::~MenuDevConsole() {
	delete button_close;
	delete button_confirm;
	delete input_box;
	delete log_history;
}

void MenuDevConsole::align() {
	Menu::align();

	button_close->setPos(window_area.x, window_area.y);
	button_confirm->setPos(window_area.x, window_area.y);
	input_box->setPos(window_area.x, window_area.y);
	log_history->setPos(window_area.x, window_area.y);

	label.setPos(window_area.x, window_area.y);
}

void MenuDevConsole::logic() {
	if (!visible && first_open && log_history->isEmpty()) {
		first_open = false;
	}

	if (visible) {
		if (!first_open) {
			first_open = true;
			log_history->add(msg->get("Use '%s' to inspect with the cursor.", inpt->getBindingString(Input::MAIN2)), WidgetLog::MSG_NORMAL);

			log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
			log_history->add("exec \"msg=Hello World\"", WidgetLog::MSG_NORMAL);

			log_history->add(msg->get("Arguments with spaces should be enclosed with double quotes. Example:"), WidgetLog::MSG_NORMAL);
			log_history->add(msg->get("Type 'help' to get a list of commands.") + ' ', WidgetLog::MSG_NORMAL);

			if (label.isHidden()) {
				log_history->setNextStyle(WidgetLog::FONT_BOLD);
				log_history->add(msg->get("Developer Console"), WidgetLog::MSG_NORMAL);
			}
		}

		if (!input_box->edit_mode) {
			tablist.logic();
		}

		input_box->logic();
		log_history->logic();

		if (button_close->checkClick()) {
			visible = false;
			reset();
		}
		else if (button_confirm->checkClick()) {
			execute();
		}
		else if (input_box->edit_mode && inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
			inpt->lock[Input::ACCEPT] = true;
			execute();
		}
		else if (input_box->edit_mode && inpt->pressing_up) {
			inpt->pressing_up = false;
			if (!input_scrollback.empty()) {
				if (input_scrollback_pos != 0)
					input_scrollback_pos--;
				input_box->setText(input_scrollback[input_scrollback_pos]);
			}
		}
		else if (input_box->edit_mode && inpt->pressing_down) {
			inpt->pressing_down = false;
			if (!input_scrollback.empty()) {
				input_scrollback_pos++;
				if (input_scrollback_pos < input_scrollback.size()) {
					input_box->setText(input_scrollback[input_scrollback_pos]);
				}
				else {
					input_scrollback_pos = input_scrollback.size();
					input_box->setText("");
				}
			}
		}

		if (inpt->pressing[Input::MAIN2] && !inpt->lock[Input::MAIN2]) {
			inpt->lock[Input::MAIN2] = true;
			target = Utils::screenToMap(inpt->mouse.x,  inpt->mouse.y, mapr->cam.x, mapr->cam.y);

			log_history->addSeparator();

			// print cursor position in map units & pixels
			std::stringstream ss;
			if (!mapr->collider.isOutsideMap(floorf(target.x), floorf(target.y))) {
				getTileInfo();
				getEnemyInfo();
				getPlayerInfo();

				ss << "X=" << target.x << ", Y=" << target.y;
				ss << "  |  X=" << inpt->mouse.x << msg->get("px") << ", Y=" << inpt->mouse.y << msg->get("px");
				log_history->add(ss.str(), WidgetLog::MSG_NORMAL);
			}
			else {
				ss << "X=" << inpt->mouse.x << msg->get("px") << ", Y=" << inpt->mouse.y << msg->get("px");
				log_history->add(ss.str(), WidgetLog::MSG_NORMAL);
			}
		}

		if (inpt->pressing[Input::MAIN2]) {
			distance_timer.tick();

			// print target distance from the player
			if (distance_timer.isEnd()) {
				std::stringstream ss;
				ss << msg->get("Distance") << ": " << Utils::calcDist(target, pc->stats.pos);
				log_history->add(ss.str(), WidgetLog::MSG_NORMAL);
			}
		}
		else {
			distance_timer.reset(Timer::BEGIN);
		}
	}
}

void MenuDevConsole::getPlayerInfo() {
	if (!(static_cast<int>(target.x) == static_cast<int>(pc->stats.pos.x) && static_cast<int>(target.y) == static_cast<int>(pc->stats.pos.y)))
		return;

	std::stringstream ss;
	ss << msg->get("Entity") << ": " << pc->stats.name << "  |  X=" << pc->stats.pos.x << ", Y=" << pc->stats.pos.y;
	log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
	log_history->add(ss.str(), WidgetLog::MSG_NORMAL);

	// TODO print more player data
}

void MenuDevConsole::getTileInfo() {
	Point tile(target);

	std::stringstream ss;
	for (size_t i = 0; i < mapr->layers.size(); ++i) {
		if (mapr->layers[i][tile.x][tile.y] == 0)
			continue;
		ss.str("");
		ss << "    " << mapr->layernames[i] << "=" << mapr->layers[i][tile.x][tile.y];
		log_history->add(ss.str(), WidgetLog::MSG_NORMAL);
	}

	ss.str("");
	ss << "    " << "collision=" << mapr->collider.colmap[tile.x][tile.y] << " (";
	switch(mapr->collider.colmap[tile.x][tile.y]) {
		case MapCollision::BLOCKS_NONE: ss << msg->get("none"); break;
		case MapCollision::BLOCKS_ALL: ss << msg->get("wall"); break;
		case MapCollision::BLOCKS_MOVEMENT: ss << msg->get("short wall / pit"); break;
		case MapCollision::BLOCKS_ALL_HIDDEN: ss << msg->get("wall"); break;
		case MapCollision::BLOCKS_MOVEMENT_HIDDEN: ss << msg->get("short wall / pit"); break;
		case MapCollision::BLOCKS_ENTITIES: ss << msg->get("entity"); break;
		case MapCollision::BLOCKS_ENEMIES: ss << msg->get("entity, ally"); break;
		default: ss << msg->get("none");
	}
	ss << ")";
	log_history->add(ss.str(), WidgetLog::MSG_NORMAL);

	ss.str("");
	ss << msg->get("Tile") << ": X=" << tile.x <<  ", Y=" << tile.y;
	log_history->add(ss.str(), WidgetLog::MSG_NORMAL);
}

void MenuDevConsole::getEnemyInfo() {
	std::stringstream ss;
	for (size_t i = 0; i < enemym->enemies.size(); ++i) {
		const Enemy* e = enemym->enemies[i];
		if (!(static_cast<int>(target.x) == static_cast<int>(e->stats.pos.x) && static_cast<int>(target.y) == static_cast<int>(e->stats.pos.y)))
			continue;

		ss.str("");
		ss << msg->get("Entity") << ": " << e->stats.name << "  |  X=" << e->stats.pos.x << ", Y=" << e->stats.pos.y;
		log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
		log_history->add(ss.str(), WidgetLog::MSG_NORMAL);

		// TODO print more enemy data
	}
}

void MenuDevConsole::render() {
	if (!visible)
		return;

	// background
	Menu::render();

	label.render();
	button_close->render();
	button_confirm->render();
	input_box->render();
	log_history->render();
}

bool MenuDevConsole::inputFocus() {
	return visible && input_box->edit_mode;
}

void MenuDevConsole::reset() {
	input_box->setText("");
	input_box->edit_mode = true;
	// log_history->clear();
}

void MenuDevConsole::closeWindow() {
	visible = false;
	input_box->edit_mode = false;
	input_box->logic();
	reset();
}

void MenuDevConsole::execute() {
	std::string command = input_box->getText();
	if (command == "") return;

	input_scrollback.push_back(command);
	input_scrollback_pos = input_scrollback.size();
	input_box->setText("");

	log_history->addSeparator();
	log_history->setNextColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED));
	log_history->add(command, WidgetLog::MSG_UNIQUE);

	std::vector<std::string> args;
	command += ' ';

	std::string arg = Parse::popFirstString(command, ' ');
	while (arg != "") {
		args.push_back(arg);

		if (!command.empty() && command.at(0) == '"') {
			command = command.substr(1); // remove first quote
			arg = Parse::popFirstString(command, '"');
			command = command.substr(1); // remove trailing space
		}
		else {
			arg = Parse::popFirstString(command, ' ');
		}
	}


	if (args.empty()) {
		return;
	}

	if (args[0] == "help") {
		log_history->add("add_power - " + msg->get("adds a power to the action bar"), WidgetLog::MSG_UNIQUE);
		log_history->add("toggle_fps - " + msg->get("turns on/off the display of the FPS counter"), WidgetLog::MSG_UNIQUE);
		log_history->add("toggle_hud - " + msg->get("turns on/off all of the HUD elements"), WidgetLog::MSG_UNIQUE);
		log_history->add("toggle_devhud - " + msg->get("turns on/off the developer hud"), WidgetLog::MSG_UNIQUE);
		log_history->add("list_powers - " + msg->get("Prints a list of powers that match a search term. No search term will list all items"), WidgetLog::MSG_UNIQUE);
		log_history->add("list_maps - " + msg->get("Prints out all the map filenames located in the \"maps/\" directory."), WidgetLog::MSG_UNIQUE);
		log_history->add("list_status - " + msg->get("Prints out the active campaign statuses that match a search term. No search term will list all active statuses"), WidgetLog::MSG_UNIQUE);
		log_history->add("list_items - " + msg->get("Prints a list of items that match a search term. No search term will list all items"), WidgetLog::MSG_UNIQUE);
		log_history->add("exec - " + msg->get("parses a series of event components and executes them as a single event"), WidgetLog::MSG_UNIQUE);
		log_history->add("clear - " + msg->get("clears the command history"), WidgetLog::MSG_UNIQUE);
		log_history->add("help - " + msg->get("displays this text"), WidgetLog::MSG_UNIQUE);
	}
	else if (args[0] == "clear") {
		log_history->clear();
	}
	else if (args[0] == "toggle_devhud") {
		settings->dev_hud = !settings->dev_hud;
		log_history->add(msg->get("Toggled the developer hud"), WidgetLog::MSG_UNIQUE);
	}
	else if (args[0] == "toggle_hud") {
		settings->show_hud = !settings->show_hud;
		log_history->add(msg->get("Toggled the hud"), WidgetLog::MSG_UNIQUE);
	}
	else if (args[0] == "toggle_fps") {
		settings->show_fps = !settings->show_fps;
		log_history->add(msg->get("Toggled the FPS counter"), WidgetLog::MSG_UNIQUE);
	}
	else if (args[0] == "list_status") {
		std::string search_terms;
		for (size_t i=1; i<args.size(); i++) {
			search_terms += args[i];

			if (i+1 != args.size())
				search_terms += ' ';
		}
		std::vector<size_t> matching_ids;
		std::vector<std::string> status_strings;
		camp->getSetStatusStrings(status_strings);

		for (size_t i=0; i<status_strings.size(); ++i) {
			if (!search_terms.empty() && Utils::stringFindCaseInsensitive(status_strings[i], search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				log_history->add(status_strings[matching_ids[i-1]], WidgetLog::MSG_NORMAL);
			}

			log_history->setMaxMessages(WidgetLog::MAX_MESSAGES); // reset
		}
	}
	else if (args[0] == "list_items") {
		std::stringstream ss;

		std::string search_terms;
		for (size_t i=1; i<args.size(); i++) {
			search_terms += args[i];

			if (i+1 != args.size())
				search_terms += ' ';
		}

		std::vector<size_t> matching_ids;

		for (size_t i=1; i<items->items.size(); ++i) {
			if (!items->items[i].has_name)
				continue;

			std::string item_name = items->getItemName(static_cast<int>(i));
			if (!search_terms.empty() && Utils::stringFindCaseInsensitive(item_name, search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				size_t id = matching_ids[i-1];

				ss.str("");
				ss << items->getItemName(static_cast<int>(id)) << " (" << id <<")";
				log_history->setNextColor(items->getItemColor(static_cast<int>(id)));
				log_history->add(ss.str(), WidgetLog::MSG_UNIQUE);
			}

			log_history->setMaxMessages(WidgetLog::MAX_MESSAGES); // reset
		}
	}
	else if (args[0] == "list_maps") {
		std::vector<std::string> map_filenames = mods->list("maps", !ModManager::LIST_FULL_PATHS);

		for (size_t i=0; i<map_filenames.size(); ++i) {
			// Remove "maps/" from all of the filenames so that it doesn't affect search results
			// We'll still print out the "maps/" later during the output
			map_filenames[i].erase(0,5);
		}

		std::sort(map_filenames.begin(), map_filenames.end());

		std::string search_terms;
		for (size_t i=1; i<args.size(); i++) {
			search_terms += args[i];

			if (i+1 != args.size())
				search_terms += ' ';
		}

		std::vector<size_t> matching_ids;

		for (size_t i=0; i<map_filenames.size(); ++i) {
			if (!search_terms.empty() && Utils::stringFindCaseInsensitive(map_filenames[i], search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				log_history->add("maps/" + map_filenames[matching_ids[i-1]], WidgetLog::MSG_UNIQUE);
			}

			log_history->setMaxMessages(WidgetLog::MAX_MESSAGES); // reset
		}
	}
	else if (args[0] == "list_powers") {
		std::stringstream ss;

		std::string search_terms;
		for (size_t i=1; i<args.size(); i++) {
			search_terms += args[i];

			if (i+1 != args.size())
				search_terms += ' ';
		}

		std::vector<size_t> matching_ids;

		for (size_t i=1; i<powers->powers.size(); ++i) {
			if (powers->powers[i].is_empty)
				continue;

			std::string item_name = powers->powers[i].name;
			if (!search_terms.empty() && Utils::stringFindCaseInsensitive(item_name, search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				size_t id = matching_ids[i-1];

				ss.str("");
				ss << powers->powers[id].name << " (" << id <<")";
				log_history->add(ss.str(), WidgetLog::MSG_UNIQUE);
			}

			log_history->setMaxMessages(WidgetLog::MAX_MESSAGES); // reset
		}
	}
	else if (args[0] == "add_power") {
		if (args.size() != 2) {
			log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_PENALTY));
			log_history->add(msg->get("ERROR: Incorrect number of arguments"), WidgetLog::MSG_UNIQUE);
			log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
			log_history->add(msg->get("HINT:") + ' ' + args[0] + ' ' + msg->get("<id>"), WidgetLog::MSG_UNIQUE);
		}
		else {
			menu->act->addPower(Parse::toInt(args[1]), MenuActionBar::USE_EMPTY_SLOT);
		}
	}
	else if (args[0] == "exec") {
		if (args.size() > 1) {
			Event evnt;

			for (size_t i = 1; i < args.size(); ++i) {
				std::string key = Parse::popFirstString(args[i], '=');
				std::string val = args[i];

				if (!EventManager::loadEventComponentString(key, val, &evnt, NULL)) {
					log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_PENALTY));
					log_history->add(msg->get("ERROR: '%s' is not a valid event key", key), WidgetLog::MSG_UNIQUE);
				}
			}

			if (EventManager::isActive(evnt)) {
				EventManager::executeEvent(evnt);
			}
		}
		else {
			log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_PENALTY));
			log_history->add(msg->get("ERROR: Too few arguments"), WidgetLog::MSG_UNIQUE);
			log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
			log_history->add(msg->get("HINT:") + ' ' + args[0] + ' ' + msg->get("<key>=<val> <key>=<val> ..."), WidgetLog::MSG_UNIQUE);
		}
	}
	else {
		log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_PENALTY));
		log_history->add(msg->get("ERROR: Unknown command"), WidgetLog::MSG_UNIQUE);
		log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
		log_history->add(msg->get("HINT: Type help"), WidgetLog::MSG_UNIQUE);
	}
}
