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

#include "EnemyManager.h"
#include "FileParser.h"
#include "MenuDevConsole.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

#include <limits>

MenuDevConsole::MenuDevConsole()
	: Menu()
	, first_open(false)
	, input_scrollback_pos(0)
{

	button_close = new WidgetButton("images/menus/buttons/button_x.png");
	tablist.add(button_close);

	input_box = new WidgetInput("images/menus/input_console.png");
	tablist.add(input_box);

	button_confirm = new WidgetButton();
	button_confirm->label = msg->get("Execute");
	tablist.add(button_confirm);

	// Load config settings
	FileParser infile;
	// @CLASS MenuDevConsole|Description of menus/devconsole.txt
	if(infile.open("menus/devconsole.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = toPoint(infile.val);
				button_close->setBasePos(pos.x, pos.y);
			}
			// @ATTR label_title|label|Position of the "Developer Console" label.
			else if(infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR confirm|point|Position of the "Execute" button.
			else if(infile.key == "confirm") {
				Point pos = toPoint(infile.val);
				button_confirm->setBasePos(pos.x, pos.y);
			}
			// @ATTR input|point|Position of the command entry widget.
			else if(infile.key == "input") {
				Point pos = toPoint(infile.val);
				input_box->setBasePos(pos.x, pos.y);
			}
			// @ATTR history|rectangle|Position and dimensions of the command history.
			else if(infile.key == "history") history_area = toRect(infile.val);

			else infile.error("MenuDevConsole: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	log_history = new WidgetLog(history_area.w, history_area.h);
	log_history->setBasePos(history_area.x, history_area.y);
	tablist.add(log_history->getWidget());

	setBackground("images/menus/dev_console.png");

	color_echo = font->getColor("widget_disabled");
	color_error = font->getColor("menu_penalty");
	color_hint = font->getColor("menu_bonus");

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

	label.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Developer Console"), font->getColor("menu_normal"));
}

void MenuDevConsole::logic() {
	if (visible) {
		if (!first_open) {
			first_open = true;
			log_history->add("exec \"msg=Hello World\"");
			log_history->add(msg->get("Arguments with spaces should be enclosed with double quotes. Example:"));
			log_history->add(msg->get("Type 'help' to get a list of commands. "));
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
		else if (input_box->edit_mode && inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
			inpt->lock[ACCEPT] = true;
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
	}
}

void MenuDevConsole::render() {
	if (visible) {
		// background
		Menu::render();

		label.render();
		button_close->render();
		button_confirm->render();
		input_box->render();
		log_history->render();
	}
}

bool MenuDevConsole::inputFocus() {
	return visible && input_box->edit_mode;
}

void MenuDevConsole::reset() {
	input_box->setText("");
	input_box->edit_mode = true;
	// log_history->clear();
}

void MenuDevConsole::execute() {
	std::string command = input_box->getText();
	if (command == "") return;

	input_scrollback.push_back(command);
	input_scrollback_pos = input_scrollback.size();
	input_box->setText("");

	log_history->add(command, false, &color_echo);

	std::vector<std::string> args;
	command += ' ';

	std::string arg = popFirstString(command, ' ');
	while (arg != "") {
		args.push_back(arg);

		if (!command.empty() && command.at(0) == '"') {
			command = command.substr(1); // remove first quote
			arg = popFirstString(command, '"');
			command = command.substr(1); // remove trailing space
		}
		else {
			arg = popFirstString(command, ' ');
		}
	}


	if (args.empty()) {
		return;
	}

	if (args[0] == "help") {
		log_history->add("toggle_hud - " + msg->get("turns on/off all of the HUD elements"), false);
		log_history->add("toggle_devhud - " + msg->get("turns on/off the developer hud"), false);
		log_history->add("respec - " + msg->get("resets the player to level 1, with no stat or skill points spent"), false);
		log_history->add("list_maps - " + msg->get("Prints out all the map filenames located in the \"maps/\" directory."), false);
		log_history->add("list_status - " + msg->get("Prints out the active campaign statuses that match a search term. No search term will list all active statuses"), false);
		log_history->add("list_items - " + msg->get("Prints a list of items that match a search term. No search term will list all items"), false);
		log_history->add("exec - " + msg->get("parses a series of event components and executes them as a single event"), false);
		log_history->add("clear - " + msg->get("clears the command history"), false);
		log_history->add("help - " + msg->get("displays this text"), false);
	}
	else if (args[0] == "clear") {
		log_history->clear();
	}
	else if (args[0] == "toggle_devhud") {
		DEV_HUD = !DEV_HUD;
		log_history->add(msg->get("Toggled the developer hud"), false);
	}
	else if (args[0] == "toggle_hud") {
		SHOW_HUD = !SHOW_HUD;
		log_history->add(msg->get("Toggled the hud"), false);
	}
	else if (args[0] == "respec") {
		pc->stats.level = 1;
		pc->stats.xp = 0;
		for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
			pc->stats.primary[i] = 1;
			pc->stats.primary_additional[i] = 0;
		}
		pc->stats.powers_list.clear();
		pc->stats.powers_passive.clear();
		pc->stats.effects.clearEffects();
		menu_powers->resetToBasePowers();
		menu_powers->applyPowerUpgrades();
		menu_act->clear();
		pc->respawn = true; // re-applies equipment, also revives the player
		pc->stats.refresh_stats = true;
	}
	else if (args[0] == "list_status") {
		std::string search_terms;
		for (size_t i=1; i<args.size(); i++) {
			search_terms += args[i];

			if (i+1 != args.size())
				search_terms += ' ';
		}

		std::vector<size_t> matching_ids;

		for (size_t i=0; i<camp->status.size(); ++i) {
			if (!search_terms.empty() && stringFindCaseInsensitive(camp->status[i], search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				log_history->add(camp->status[matching_ids[i-1]]);
			}

			log_history->setMaxMessages(); // reset
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
			if (!search_terms.empty() && stringFindCaseInsensitive(item_name, search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				size_t id = matching_ids[i-1];

				Color item_color = items->getItemColor(static_cast<int>(id));
				ss.str("");
				ss << items->getItemName(static_cast<int>(id)) << " (" << id <<")";
				log_history->add(ss.str(), false, &item_color);
			}

			log_history->setMaxMessages(); // reset
		}
	}
	else if (args[0] == "list_maps") {
		std::vector<std::string> map_filenames = mods->list("maps", false);

		for (size_t i=0; i<map_filenames.size(); ++i) {
			// Remove "maps/" from all of the filenames so that it doesn't affect search results
			// We'll still print out the "maps/" later during the output
			map_filenames[i].erase(0,5);
		}

		std::sort(map_filenames.begin(), map_filenames.end());

		std::stringstream ss;

		std::string search_terms;
		for (size_t i=1; i<args.size(); i++) {
			search_terms += args[i];

			if (i+1 != args.size())
				search_terms += ' ';
		}

		std::vector<size_t> matching_ids;

		for (size_t i=0; i<map_filenames.size(); ++i) {
			if (!search_terms.empty() && stringFindCaseInsensitive(map_filenames[i], search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				log_history->add("maps/" + map_filenames[matching_ids[i-1]], false);
			}

			log_history->setMaxMessages(); // reset
		}
	}
	else if (args[0] == "exec") {
		if (args.size() > 1) {
			Event evnt;

			for (size_t i = 1; i < args.size(); ++i) {
				std::string key = popFirstString(args[i], '=');
				std::string val = args[i];

				if (!EventManager::loadEventComponentString(key, val, &evnt, NULL)) {
					log_history->add(msg->get("ERROR: '%s' is not a valid event key", key.c_str()), false, &color_error);
				}
			}

			if (EventManager::isActive(evnt)) {
				EventManager::executeEvent(evnt);
			}
		}
		else {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
			log_history->add(msg->get("HINT: ") + args[0] + msg->get(" <key>=<val> <key>=<val> ..."), false, &color_hint);
		}
	}
	else {
		log_history->add(msg->get("ERROR: Unknown command"), false, &color_error);
		log_history->add(msg->get("HINT: Type help"), false, &color_hint);
	}
}
