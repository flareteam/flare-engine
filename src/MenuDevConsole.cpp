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
#include "Entity.h"
#include "EntityManager.h"
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
#include "NPC.h"
#include "NPCManager.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetInput.h"
#include "WidgetLog.h"
#include "WidgetScrollBar.h"

#include <limits>
#include <math.h>

MenuDevConsole::MenuDevConsole()
	: Menu()
	, first_open(false)
	, input_scrollback_pos(0)
	, set_shortcut_slot(0)
	, console_height(0.4f)
	, resize_handle(NULL)
	, dragging_resize(false)
	, font_name("font_regular")
	, font_bold_name("font_bold")
	, background_color(0, 0, 0, 200)
	, resize_handle_color(255, 255, 255, 63)
{
	// dorkster: dumb work-around for the fact that I made WidgetScrollBox report dimensions without the scrollbar
	// So we create a temporary scrollbar to get the dimensions we'll need later.
	WidgetScrollBar* scrollbar = new WidgetScrollBar(WidgetScrollBar::DEFAULT_FILE);
	scrollbar_w = scrollbar->getBounds().w;
	delete scrollbar;

	distance_timer.setDuration(settings->max_frames_per_sec);

	button_close = new WidgetButton("images/menus/buttons/button_x.png");
	tablist.add(button_close);

	// Load config settings
	FileParser infile;
	// @CLASS MenuDevConsole|Description of menus/devconsole.txt
	if(infile.open("menus/devconsole.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			// @ATTR font|predefined_string|Regular font to use. Defaults to "font_regular".
			if (infile.key == "font") {
				font_name = infile.val;
			}
			// @ATTR font_bold|predefined_string|Regular font to use. Defaults to "font_bold".
			else if (infile.key == "font_bold") {
				font_bold_name = infile.val;
			}
			// @ATTR color_background|color, alpha|Background fill color for menu.
			else if (infile.key == "color_background") {
				background_color = Parse::toRGBA(infile.val);
			}
			// @ATTR color_resize_handle|color, alpha|Fill color for resize handle at the bottom of the menu.
			else if (infile.key == "color_resize_handle") {
				resize_handle_color = Parse::toRGBA(infile.val);
			}

			else infile.error("MenuDevConsole: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	input_box = new WidgetInput(WidgetInput::NO_FILE);
	input_box->setFontName(font_name);

	tablist.add(input_box);

	log_history = new WidgetLog(1,1);
	log_history->setFontNames(font_name, font_bold_name);
	tablist.add(log_history->getWidget());

	align();
	reset();
	input_box->accept_to_defocus = false;
}

MenuDevConsole::~MenuDevConsole() {
	delete button_close;
	delete input_box;
	delete log_history;
	delete resize_handle;
}

void MenuDevConsole::align() {
	window_area.x = 0;
	window_area.y = 0;
	window_area.w = settings->view_w;
	window_area.h = static_cast<int>(settings->view_h * console_height);

	setBackgroundColor(background_color);

	button_close->setBasePos(0, 0, Utils::ALIGN_TOPRIGHT);
	button_close->setPos(window_area.x, window_area.y);

	input_box->setBasePos(0, 0, Utils::ALIGN_TOPLEFT);
	input_box->setPos(window_area.x, window_area.y);
	input_box->resize(window_area.w - button_close->pos.w);

	int log_y_offset = std::max(input_box->pos.h, button_close->pos.h);
	int resize_handle_size = static_cast<int>(settings->view_h * 0.02);

	log_history->setBasePos(0, log_y_offset, Utils::ALIGN_TOPLEFT);
	log_history->resize(window_area.w - scrollbar_w, window_area.h - log_y_offset - resize_handle_size);
	log_history->setPos(window_area.x, window_area.y);

	resize_area.x = window_area.x;
	resize_area.y = window_area.y + window_area.h - resize_handle_size;
	resize_area.w = window_area.w;
	resize_area.h = resize_handle_size;

	if (resize_handle && resize_handle->getGraphicsWidth() != resize_area.w) {
		delete resize_handle;
		resize_handle = NULL;
	}

	if (!resize_handle) {
		Image *temp = render_device->createImage(resize_area.w, resize_area.h);
		if (temp) {
			temp->fillWithColor(resize_handle_color);
			resize_handle = temp->createSprite();
			temp->unref();
		}
	}
	resize_handle->setDestFromRect(resize_area);
}

void MenuDevConsole::logic() {
	if (!visible && first_open && log_history->isEmpty()) {
		first_open = false;
	}

	// handle shortcut keys
	// these are supposed to work even if the console is hidden
	if (inpt->pressing[Input::DEVELOPER_CMD_1] && ! inpt->lock[Input::DEVELOPER_CMD_1]) {
		inpt->lock[Input::DEVELOPER_CMD_1] = true;

		input_box->setText(settings->dev_cmd_1);
		execute();
		return;
	}
	else if (inpt->pressing[Input::DEVELOPER_CMD_2] && ! inpt->lock[Input::DEVELOPER_CMD_2]) {
		inpt->lock[Input::DEVELOPER_CMD_2] = true;

		input_box->setText(settings->dev_cmd_2);
		execute();
		return;
	}
	else if (inpt->pressing[Input::DEVELOPER_CMD_3] && ! inpt->lock[Input::DEVELOPER_CMD_3]) {
		inpt->lock[Input::DEVELOPER_CMD_3] = true;

		input_box->setText(settings->dev_cmd_3);
		execute();
		return;
	}

	if (visible) {
		if (!first_open) {
			first_open = true;
			log_history->add(msg->getv("Use '%s' to inspect with the cursor.", inpt->getBindingString(Input::MAIN2).c_str()), WidgetLog::MSG_NORMAL);

			log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
			log_history->add("exec \"msg=Hello World\"", WidgetLog::MSG_NORMAL);

			log_history->add(msg->get("Arguments with spaces should be enclosed with double quotes. Example:"), WidgetLog::MSG_NORMAL);
			log_history->add(msg->get("Type 'help' to get a list of commands.") + ' ', WidgetLog::MSG_NORMAL);

			log_history->setNextStyle(WidgetLog::FONT_BOLD);
			log_history->add(msg->get("Developer Console"), WidgetLog::MSG_NORMAL);
		}

		if (!input_box->edit_mode) {
			tablist.logic();
		}

		input_box->logic();
		log_history->logic();

		if (dragging_resize && !inpt->lock[Input::MAIN1])
			dragging_resize = false;

		if (inpt->pressing[Input::CANCEL]) {
			visible = false;
			input_box->edit_mode = false;
			input_box->logic();
			reset();
		}
		else if (button_close->checkClick()) {
			visible = false;
			reset();
		}
		else if (input_box->edit_mode && inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
			inpt->lock[Input::ACCEPT] = true;
			execute();
		}
		else if (input_box->edit_mode && inpt->pressing[Input::TEXTEDIT_UP] && !inpt->lock[Input::TEXTEDIT_UP]) {
			inpt->lock[Input::TEXTEDIT_UP] = true;
			if (!input_scrollback.empty()) {
				if (input_scrollback_pos != 0)
					input_scrollback_pos--;
				input_box->setText(input_scrollback[input_scrollback_pos]);
			}
		}
		else if (input_box->edit_mode && inpt->pressing[Input::TEXTEDIT_DOWN] && !inpt->lock[Input::TEXTEDIT_DOWN]) {
			inpt->lock[Input::TEXTEDIT_DOWN] = true;
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
		else if ((inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1] && Utils::isWithinRect(resize_area, inpt->mouse)) || dragging_resize) {
			inpt->lock[Input::MAIN1] = true;
			dragging_resize = true;
			console_height = std::min(1.0f, std::max(0.25f, static_cast<float>(inpt->mouse.y) / static_cast<float>(settings->view_h)));
			align();
		}

		bool mouse_in_window = Utils::isWithinRect(window_area, inpt->mouse);

		if (inpt->pressing[Input::MAIN2] && !inpt->lock[Input::MAIN2] && !mouse_in_window) {
			inpt->lock[Input::MAIN2] = true;
			target = Utils::screenToMap(inpt->mouse.x,  inpt->mouse.y, mapr->cam.pos.x, mapr->cam.pos.y);

			log_history->addSeparator();

			// print cursor position in map units & pixels
			std::stringstream ss;
			if (!mapr->collider.isOutsideMap(floorf(target.x), floorf(target.y))) {
				getTileInfo();
				getEntityInfo();
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

		if (inpt->pressing[Input::MAIN2] && !mouse_in_window) {
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

void MenuDevConsole::getEntityInfo() {
	std::stringstream ss;
	// enemies & ally creatures
	for (size_t i = 0; i < entitym->entities.size(); ++i) {
		const Entity* e = entitym->entities[i];
		if (!(static_cast<int>(target.x) == static_cast<int>(e->stats.pos.x) && static_cast<int>(target.y) == static_cast<int>(e->stats.pos.y)))
			continue;

		ss.str("");
		ss << msg->get("Entity") << ": " << e->stats.name << "  |  X=" << e->stats.pos.x << ", Y=" << e->stats.pos.y;
		log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
		log_history->add(ss.str(), WidgetLog::MSG_NORMAL);

		// TODO print more entity data
	}

	// non-ally NPCs
	for (size_t i = 0; i < npcs->npcs.size(); ++i) {
		const Entity* e = npcs->npcs[i];
		if (!(static_cast<int>(target.x) == static_cast<int>(e->stats.pos.x) && static_cast<int>(target.y) == static_cast<int>(e->stats.pos.y)))
			continue;

		ss.str("");
		ss << msg->get("Entity") << ": " << e->stats.name << "  |  X=" << e->stats.pos.x << ", Y=" << e->stats.pos.y;
		log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
		log_history->add(ss.str(), WidgetLog::MSG_NORMAL);

		// TODO print more entity data
	}
}

void MenuDevConsole::render() {
	if (!visible)
		return;

	// background
	Menu::render();

	button_close->render();
	input_box->render();

	if (!dragging_resize)
		log_history->render();

	render_device->render(resize_handle);
}

bool MenuDevConsole::inputFocus() {
	return visible && input_box->edit_mode;
}

void MenuDevConsole::reset() {
	input_box->setText("");
	input_box->edit_mode = true;
	set_shortcut_slot = 0;
	// log_history->clear();
}

void MenuDevConsole::closeWindow() {
	visible = false;
	input_box->edit_mode = false;
	input_box->logic();
	reset();
	tablist.defocus();
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

	bool starts_with_slash = (command.at(0) == '/');
	if (starts_with_slash) {
		command = "exec " + Parse::trim(command.substr(1)); // remove the slash
	}
	command = Parse::trim(command);

	// setting a dev shortcut command; no need to process the command
	if (set_shortcut_slot > 0) {
		if (set_shortcut_slot == 1) {
			settings->dev_cmd_1 = command;
		}
		else if (set_shortcut_slot == 2) {
			settings->dev_cmd_2 = command;
		}
		else if (set_shortcut_slot == 3) {
			settings->dev_cmd_3 = command;
		}
		set_shortcut_slot = 0;
		settings->saveSettings();
		log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
		log_history->add(msg->get("Shortcut saved."), WidgetLog::MSG_UNIQUE);
		return;
	}

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
		log_history->add("set_shortcut - " + msg->get("Assign a console command to a shortcut key."), WidgetLog::MSG_UNIQUE);
		log_history->add("exec - " + msg->get("parses a series of event components and executes them as a single event"), WidgetLog::MSG_UNIQUE);
		log_history->add("/ - " + msg->get("parses a series of event components and executes them as a single event"), WidgetLog::MSG_UNIQUE);
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

		for (size_t i = 1; i < items->items.size(); ++i) {
			Item* item = items->items[i];

			if (!item || !item->has_name)
				continue;

			std::string item_name = items->getItemName(i);
			if (!search_terms.empty() && Utils::stringFindCaseInsensitive(item_name, search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				size_t id = matching_ids[i-1];

				ss.str("");
				ss << items->getItemName(id) << " (" << id <<")";
				log_history->setNextColor(items->getItemColor(id));
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

		for (size_t i = 1; i < powers->powers.size(); ++i) {
			if (!powers->powers[i])
				continue;

			if (!search_terms.empty() && Utils::stringFindCaseInsensitive(powers->powers[i]->name, search_terms) == std::string::npos)
				continue;

			matching_ids.push_back(i);
		}

		if (!matching_ids.empty()) {
			log_history->setMaxMessages(static_cast<unsigned>(matching_ids.size()));

			for (size_t i=matching_ids.size(); i>0; i--) {
				size_t id = matching_ids[i-1];

				ss.str("");
				ss << powers->powers[id]->name << " (" << id <<")";
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
	else if (args[0] == "set_shortcut") {
		if (args.size() != 2) {
			log_history->add(msg->getv("3 | <%s> | %s", inpt->getBindingString(Input::DEVELOPER_CMD_3).c_str(), settings->dev_cmd_3.c_str()), WidgetLog::MSG_UNIQUE);
			log_history->add(msg->getv("2 | <%s> | %s", inpt->getBindingString(Input::DEVELOPER_CMD_2).c_str(), settings->dev_cmd_2.c_str()), WidgetLog::MSG_UNIQUE);
			log_history->add(msg->getv("1 | <%s> | %s", inpt->getBindingString(Input::DEVELOPER_CMD_1).c_str(), settings->dev_cmd_1.c_str()), WidgetLog::MSG_UNIQUE);
			log_history->add(msg->get("Current shortcuts:"), WidgetLog::MSG_UNIQUE);
			log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
			log_history->add(msg->get("HINT:") + ' ' + args[0] + " [1-3]", WidgetLog::MSG_UNIQUE);
		}
		else {
			int custom_cmd_index = Parse::toInt(args[1]);
			if (custom_cmd_index >= 0 && custom_cmd_index < 4) {
				set_shortcut_slot = custom_cmd_index;
				log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_BONUS));
				log_history->add(msg->getv("Enter the command you wish to save to slot %d.", set_shortcut_slot), WidgetLog::MSG_UNIQUE);
			}
			else {
				log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_PENALTY));
				log_history->add(msg->get("ERROR: Not a valid shortcut slot."), WidgetLog::MSG_UNIQUE);
			}
		}
	}
	else if (starts_with_slash || args[0] == "exec") {
		if (args.size() > 1) {
			Event evnt;

			for (size_t i = 1; i < args.size(); ++i) {
				std::string key = Parse::popFirstString(args[i], '=');
				std::string val = args[i];

				if (!EventManager::loadEventComponentString(key, val, &evnt, NULL)) {
					log_history->setNextColor(font->getColor(FontEngine::COLOR_MENU_PENALTY));
					log_history->add(msg->getv("ERROR: '%s' is not a valid event key", key.c_str()), WidgetLog::MSG_UNIQUE);
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
