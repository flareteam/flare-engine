/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

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
 * GameStateNew
 *
 * Handle player choices when starting a new game
 * (e.g. character appearance)
 */

#include "Avatar.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateNew.h"
#include "GameStateLoad.h"
#include "GameStatePlay.h"
#include "InputState.h"
#include "ItemManager.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetCheckBox.h"
#include "WidgetInput.h"
#include "WidgetLabel.h"
#include "WidgetListBox.h"

GameStateNew::GameStateNew()
	: GameState()
	, current_option(0)
	, portrait_image(NULL)
	, portrait_border(NULL)
	, show_classlist(true)
	, modified_name(false)
	, delete_items(true)
	, random_option(false)
	, random_class(false)
	, game_slot(0)
{
	// set up buttons
	button_exit = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_exit->setLabel(msg->get("Cancel"));

	button_create = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_create->setLabel(msg->get("Create"));
	button_create->enabled = false;
	button_create->refresh();

	button_prev = new WidgetButton("images/menus/buttons/left.png");
	button_next = new WidgetButton("images/menus/buttons/right.png");

	button_randomize = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_randomize->setLabel(msg->get("Randomize"));

	input_name = new WidgetInput(WidgetInput::DEFAULT_FILE);
	input_name->max_length = 20;

	button_permadeath = new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE);
	if (eset->death_penalty.permadeath) {
		button_permadeath->enabled = false;
		button_permadeath->setChecked(true);
	}

	class_list = new WidgetListBox(12, WidgetListBox::DEFAULT_FILE);
	class_list->can_deselect = false;

	// set up labels
	label_portrait = new WidgetLabel();
	label_portrait->setText(msg->get("Choose a Portrait"));
	label_portrait->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	label_name = new WidgetLabel();
	label_name->setText(msg->get("Choose a Name"));
	label_name->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	label_permadeath = new WidgetLabel();
	label_permadeath->setText(msg->get("Permadeath?"));
	label_permadeath->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	label_classlist = new WidgetLabel();
	label_classlist->setText(msg->get("Choose a Class"));
	label_classlist->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	// Read positions from config file
	FileParser infile;

	// @CLASS GameStateNew: Layout|Description of menus/gamenew.txt
	if (infile.open("menus/gamenew.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR button_prev|int, int, alignment : X, Y, Alignment|Position of button to choose the previous preset hero.
			if (infile.key == "button_prev") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_prev->setBasePos(x, y, a);
			}
			// @ATTR button_next|int, int, alignment : X, Y, Alignment|Position of button to choose the next preset hero.
			else if (infile.key == "button_next") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_next->setBasePos(x, y, a);
			}
			// @ATTR button_exit|int, int, alignment : X, Y, Alignment|Position of "Cancel" button.
			else if (infile.key == "button_exit") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_exit->setBasePos(x, y, a);
			}
			// @ATTR button_create|int, int, alignment : X, Y, Alignment|Position of "Create" button.
			else if (infile.key == "button_create") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_create->setBasePos(x, y, a);
			}
			// @ATTR button_permadeath|int, int, alignment : X, Y, Alignment|Position of checkbox for toggling permadeath.
			else if (infile.key == "button_permadeath") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_permadeath->setBasePos(x, y, a);
			}
			// @ATTR bytton_randomize|int, int, alignment : X, Y, Alignment|Position of the "Randomize" button.
			else if (infile.key == "button_randomize") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_randomize->setBasePos(x, y, a);
			}
			// @ATTR name_input|int, int, alignment : X, Y, Alignment|Position of the hero name textbox.
			else if (infile.key == "name_input") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				input_name->setBasePos(x, y, a);
			}
			// @ATTR portrait_label|label|Label for the "Choose a Portrait" text.
			else if (infile.key == "portrait_label") {
				label_portrait->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR name_label|label|Label for the "Choose a Name" text.
			else if (infile.key == "name_label") {
				label_name->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR permadeath_label|label|Label for the "Permadeath?" text.
			else if (infile.key == "permadeath_label") {
				label_permadeath->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR classlist_label|label|Label for the "Choose a Class" text.
			else if (infile.key == "classlist_label") {
				label_classlist->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR classlist_height|int|Number of visible rows for the class list widget.
			else if (infile.key == "classlist_height") {
				class_list->setHeight(Parse::popFirstInt(infile.val));
			}
			// @ATTR portrait|rectangle|Position and dimensions of the portrait image.
			else if (infile.key == "portrait") {
				portrait_pos = Parse::toRect(infile.val);
			}
			// @ATTR class_list|int, int, alignment : X, Y, Alignment|Position of the class list.
			else if (infile.key == "class_list") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				class_list->setBasePos(x, y, a);
			}
			// @ATTR show_classlist|bool|Allows hiding the class list.
			else if (infile.key == "show_classlist") {
				show_classlist = Parse::toBool(infile.val);
			}
			// @ATTR random_option|bool|Initially picks a random character option (aka portrait/name).
			else if (infile.key == "random_option") {
				random_option = Parse::toBool(infile.val);
			}
			// @ATTR random_class|bool|Initially picks a random character class.
			else if (infile.key == "random_class") {
				random_class = Parse::toBool(infile.val);
			}
			else {
				infile.error("GameStateNew: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// set up class list
	for (unsigned i = 0; i < eset->hero_classes.list.size(); i++) {
		class_list->append(msg->get(eset->hero_classes.list[i].name), getClassTooltip(i));
	}

	if (!eset->hero_classes.list.empty()) {
		int class_index = 0;
		if (random_class)
			class_index = static_cast<int>(rand() % eset->hero_classes.list.size());

		class_list->select(class_index);
	}

	loadGraphics();
	loadOptions("hero_options.txt");

	if (random_option)
		setHeroOption(OPTION_RANDOM);
	else
		setHeroOption(OPTION_CURRENT);

	// Set up tab list
	tablist.ignore_no_mouse = true;
	tablist.add(button_exit);
	tablist.add(button_create);
	tablist.add(input_name);
	tablist.add(button_permadeath);
	tablist.add(button_randomize);
	tablist.add(button_prev);
	tablist.add(button_next);
	tablist.add(class_list);

	refreshWidgets();

	render_device->setBackgroundColor(Color(0,0,0,0));
}

void GameStateNew::loadGraphics() {
	Image *graphics;

	graphics = render_device->loadImage("images/menus/portrait_border.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		portrait_border = graphics->createSprite();
		graphics->unref();
	}
}

void GameStateNew::loadPortrait(const std::string& portrait_filename) {

	Image *graphics;

	if (portrait_image)
		delete portrait_image;

	portrait_image = NULL;
	graphics = render_device->loadImage(portrait_filename, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		portrait_image = graphics->createSprite();
		portrait_image->setDestFromRect(portrait_pos);
		graphics->unref();
	}
}

/**
 * Load body type "base" and portrait/head "portrait" options from a config file
 *
 * @param filename File containing entries for option=base,look
 */
void GameStateNew::loadOptions(const std::string& filename) {
	FileParser fin;
	// @CLASS GameStateNew: Hero options|Description of engine/hero_options.txt
	if (!fin.open("engine/" + filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) return;

	int cur_index = -1;
	while (fin.next()) {
		// @ATTR option|int, string, string, filename, string : Index, Base, Head, Portrait, Name|A default body, head, portrait, and name for a hero.
		if (fin.key == "option") {
			cur_index = std::max(0, Parse::popFirstInt(fin.val));

			if (static_cast<size_t>(cur_index + 1) > hero_options.size()) {
				hero_options.resize(cur_index + 1);
			}

			hero_options[cur_index].base = Parse::popFirstString(fin.val);
			hero_options[cur_index].head = Parse::popFirstString(fin.val);
			hero_options[cur_index].portrait = Parse::popFirstString(fin.val);
			hero_options[cur_index].name = msg->get(Parse::popFirstString(fin.val));

			all_options.push_back(cur_index);
		}
	}
	fin.close();

	if (hero_options.empty()) {
		hero_options.resize(1);
	}

	std::sort(all_options.begin(), all_options.end());
}

/**
 * If the name text box is empty or hasn't been user-modified, set the name
 *
 * @param default_name The name we want to use for the hero
 */
void GameStateNew:: setName(const std::string& default_name) {
	if (input_name->getText() == "" || !modified_name) {
		input_name->setText(default_name);
		modified_name = false;
	}
}

void GameStateNew::setHeroOption(int dir) {
	std::vector<int> *available_options = &all_options;

	// get the available options from the currently selected class
	int class_index;
	if ( (class_index = class_list->getSelected()) != -1) {
		if (static_cast<size_t>(class_index) < eset->hero_classes.list.size() && !eset->hero_classes.list[class_index].options.empty()) {
			available_options = &(eset->hero_classes.list[class_index].options);
		}
	}

	if (dir == OPTION_CURRENT) {
		// don't change current_option unless required
		if (std::find(available_options->begin(), available_options->end(), current_option) == available_options->end()) {
			if (random_option && available_options != &all_options) {
				size_t rand_index = rand() % available_options->size();
				current_option = available_options->at(rand_index);
			}
			else {
				current_option = available_options->front();
			}
		}
	}
	else if (dir == OPTION_NEXT) {
		// increment current_option
		std::vector<int>::iterator it = std::find(available_options->begin(), available_options->end(), current_option);
		if (it == available_options->end()) {
			current_option = available_options->front();
		}
		else {
			++it;
			if (it != available_options->end())
				current_option = (*it);
			else
				current_option = available_options->front();
		}
	}
	else if (dir == OPTION_PREV) {
		// decrement current_option
		std::vector<int>::iterator it = std::find(available_options->begin(), available_options->end(), current_option);
		if (it == available_options->begin()) {
			current_option = available_options->back();
		}
		else {
			--it;
			current_option = (*it);
		}
	}
	else if (dir == OPTION_RANDOM && !available_options->empty()) {
		size_t rand_index = rand() % available_options->size();
		current_option = available_options->at(rand_index);
	}

	loadPortrait(hero_options[current_option].portrait);
	setName(hero_options[current_option].name);
}

void GameStateNew::logic() {

	if (inpt->window_resized)
		refreshWidgets();

	if (!input_name->edit_mode)
		tablist.logic();

	input_name->logic();

	button_permadeath->checkClick();
	if (show_classlist && class_list->checkClick()) {
		setHeroOption(OPTION_CURRENT);
	}

	// require character name
	if (input_name->getText() == "") {
		if (button_create->enabled) {
			button_create->enabled = false;
			button_create->refresh();
		}
	}
	else {
		if (!button_create->enabled) {
			button_create->enabled = true;
			button_create->refresh();
		}
	}

	if ((inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) || button_exit->checkClick()) {
		if (inpt->pressing[Input::CANCEL])
			inpt->lock[Input::CANCEL] = true;
		delete_items = false;
		showLoading();
		setRequestedGameState(new GameStateLoad());
	}

	if (button_create->checkClick()) {
		// start the new game
		inpt->lock_all = true;
		delete_items = false;
		showLoading();
		GameStatePlay* play = new GameStatePlay();
		Avatar *avatar = pc;
		avatar->stats.gfx_base = hero_options[current_option].base;
		avatar->stats.gfx_head = hero_options[current_option].head;
		avatar->stats.gfx_portrait = hero_options[current_option].portrait;
		avatar->stats.name = input_name->getText();
		avatar->stats.permadeath = button_permadeath->isChecked();
		save_load->setGameSlot(game_slot);
		play->resetGame();
		save_load->loadClass(class_list->getSelected());
		setRequestedGameState(play);
	}

	// scroll through portrait options
	if (button_next->checkClick()) {
		setHeroOption(OPTION_NEXT);
	}
	else if (button_prev->checkClick()) {
		setHeroOption(OPTION_PREV);
	}

	if (button_randomize->checkClick()) {
		if (!eset->hero_classes.list.empty()) {
			int class_index = static_cast<int>(rand() % eset->hero_classes.list.size());
			class_list->select(class_index);
		}
		setHeroOption(OPTION_RANDOM);
	}

	if (input_name->getText() != hero_options[current_option].name)
		modified_name = true;
}

void GameStateNew::refreshWidgets() {
	button_exit->setPos(0, 0);
	button_create->setPos(0, 0);

	button_prev->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	button_next->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	button_permadeath->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	button_randomize->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	class_list->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);

	label_portrait->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	label_name->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	label_permadeath->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	label_classlist->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);

	input_name->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
}

void GameStateNew::render() {

	// display buttons
	button_exit->render();
	button_create->render();
	button_prev->render();
	button_next->render();
	input_name->render();
	button_permadeath->render();
	button_randomize->render();

	// display portrait option
	Rect src;
	Rect dest;

	src.w = dest.w = portrait_pos.w;
	src.h = dest.h = portrait_pos.h;
	src.x = src.y = 0;
	dest.x = portrait_pos.x + (settings->view_w - eset->resolutions.frame_w)/2;
	dest.y = portrait_pos.y + (settings->view_h - eset->resolutions.frame_h)/2;

	if (portrait_image) {
		portrait_image->setClipFromRect(src);
		portrait_image->setDestFromRect(dest);
		render_device->render(portrait_image);
		portrait_border->setClipFromRect(src);
		portrait_border->setDestFromRect(dest);
		render_device->render(portrait_border);
	}

	// display labels
	label_portrait->render();
	label_name->render();
	label_permadeath->render();

	// display class list
	if (show_classlist) {
		label_classlist->render();
		class_list->render();
	}

}

std::string GameStateNew::getClassTooltip(int index) {
	std::string tooltip;
	if (eset->hero_classes.list[index].description != "") tooltip += msg->get(eset->hero_classes.list[index].description);
	return tooltip;
}

GameStateNew::~GameStateNew() {
	if (portrait_image)
		delete portrait_image;

	if (portrait_border)
		delete portrait_border;

	if (delete_items) {
		delete items;
		items = NULL;
	}

	delete button_exit;
	delete button_create;
	delete button_next;
	delete button_prev;
	delete button_randomize;
	delete label_portrait;
	delete label_name;
	delete input_name;
	delete button_permadeath;
	delete label_permadeath;
	delete label_classlist;
	delete class_list;
}
