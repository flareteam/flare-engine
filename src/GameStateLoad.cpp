/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014 Henrik Andersson

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
 * GameStateLoad
 */

#include "Avatar.h"
#include "FileParser.h"
#include "GameStateLoad.h"
#include "GameStateTitle.h"
#include "GameStatePlay.h"
#include "GameStateNew.h"
#include "ItemManager.h"
#include "MenuConfirm.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

GameStateLoad::GameStateLoad() : GameState()
	, background(NULL)
	, selection(NULL)
	, portrait_border(NULL)
	, portrait(NULL)
	, loading_requested(false)
	, loading(false)
	, loaded(false)
	, delete_items(true)
	, current_frame(0)
	, frame_ticker(0)
	, stance_ticks_per_frame(1)
	, stance_duration(1)
	, stance_type(PLAY_ONCE)
	, load_game(false)
	, selected_slot(-1) {

	if (items == NULL)
		items = new ItemManager();

	label_loading = new WidgetLabel();

	for (int i = 0; i < GAME_SLOT_MAX; i++) {
		label_name[i] = new WidgetLabel();
		label_level[i] = new WidgetLabel();
		label_map[i] = new WidgetLabel();
	}

	// Confirmation box to confirm deleting
	confirm = new MenuConfirm(msg->get("Delete Save"), msg->get("Delete this save?"));
	button_exit = new WidgetButton("images/menus/buttons/button_default.png");
	button_exit->label = msg->get("Exit to Title");
	button_exit->pos.x = VIEW_W_HALF - button_exit->pos.w/2;
	button_exit->pos.y = VIEW_H - button_exit->pos.h;
	button_exit->refresh();

	button_action = new WidgetButton("images/menus/buttons/button_default.png");
	button_action->label = msg->get("Choose a Slot");
	button_action->enabled = false;

	button_alternate = new WidgetButton("images/menus/buttons/button_default.png");
	button_alternate->label = msg->get("Delete Save");
	button_alternate->enabled = false;

	// Set up tab list
	tablist = TabList(HORIZONTAL);
	tablist.add(button_exit);

	// Read positions from config file
	FileParser infile;

	// @CLASS GameStateLoad|Description of menus/gameload.txt
	if (infile.open("menus/gameload.txt")) {
		while (infile.next()) {
			// @ATTR action_button|x (integer), y (integer)|Position of the "New Game"/"Load Game" button.
			if (infile.key == "action_button") {
				button_action->pos.x = popFirstInt(infile.val);
				button_action->pos.y = popFirstInt(infile.val);
			}
			// @ATTR alternate_button|x (integer), y (integer)|Position of the "Delete Save" button.
			else if (infile.key == "alternate_button") {
				button_alternate->pos.x = popFirstInt(infile.val);
				button_alternate->pos.y = popFirstInt(infile.val);
			}
			// @ATTR portrait|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the portrait image.
			else if (infile.key == "portrait") {
				portrait_dest = toRect(infile.val);
				portrait_dest.x += (VIEW_W - FRAME_W) / 2;
				portrait_dest.y += (VIEW_H - FRAME_H) / 2;
			}
			// @ATTR gameslot|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the first game slot.
			else if (infile.key == "gameslot") {
				gameslot_pos = toRect(infile.val);
			}
			// @ATTR preview|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the preview area in the first game slot. Only 'h' is used?
			else if (infile.key == "preview") {
				preview_pos = toRect(infile.val);
			}
			// @ATTR name|label|The label for the hero's name. Position is relative to game slot position.
			else if (infile.key == "name") {
				name_pos = eatLabelInfo(infile.val);
			}
			// @ATTR level|label|The label for the hero's level. Position is relative to game slot position.
			else if (infile.key == "level") {
				level_pos = eatLabelInfo(infile.val);
			}
			// @ATTR map|label|The label for the hero's current location. Position is relative to game slot position.
			else if (infile.key == "map") {
				map_pos = eatLabelInfo(infile.val);
			}
			// @ATTR loading_label|label|The label for the "Entering game world..."/"Loading saved game..." text.
			else if (infile.key == "loading_label") {
				loading_pos = eatLabelInfo(infile.val);
			}
			// @ATTR sprite|x (integer), y (integer)|Position for the avatar preview image in each slot
			else if (infile.key == "sprite") {
				sprites_pos = toPoint(infile.val);
			}
			else {
				infile.error("GameStateLoad: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// get displayable types list
	bool found_layer = false;
	if (infile.open("engine/hero_layers.txt")) {
		while(infile.next()) {
			if (infile.key == "layer") {
				unsigned dir = popFirstInt(infile.val);
				if (dir != 6) continue;
				else found_layer = true;

				std::string layer = popFirstString(infile.val);
				while (layer != "") {
					preview_layer.push_back(layer);
					layer = popFirstString(infile.val);
				}
			}
		}
		infile.close();
	}
	if (!found_layer) logError("GameStateLoad: Could not find layers for direction 6\n");

	button_action->pos.x += (VIEW_W - FRAME_W)/2;
	button_action->pos.y += (VIEW_H - FRAME_H)/2;
	button_action->refresh();

	button_alternate->pos.x += (VIEW_W - FRAME_W)/2;
	button_alternate->pos.y += (VIEW_H - FRAME_H)/2;
	button_alternate->refresh();

	for (int i=0; i<GAME_SLOT_MAX; i++) {
		current_map[i] = "";
	}

	loadGraphics();
	readGameSlots();

	for (int i=0; i<GAME_SLOT_MAX; i++) {
		slot_pos[i].x = gameslot_pos.x + (VIEW_W - FRAME_W)/2;
		slot_pos[i].h = gameslot_pos.h;
		slot_pos[i].y = gameslot_pos.y + (VIEW_H - FRAME_H)/2 + (i * gameslot_pos.h);
		slot_pos[i].w = gameslot_pos.w;
	}

	// animation data
	int stance_frames = 0;
	if (infile.open("animations/hero.txt")) {
		while (infile.next()) {
			if (infile.section == "stance") {
				if (infile.key == "frames") {
					stance_frames = toInt(infile.val);
				}
				else if (infile.key == "duration") {
					stance_duration = parse_duration(infile.val);
				}
				else if (infile.key == "type") {
					if (infile.val == "play_once")
						stance_type = PLAY_ONCE;
					else if (infile.val == "looped")
						stance_type = LOOPED;
					else if (infile.val == "back_forth")
						stance_type = BACK_FORTH;
				}
				else {
					infile.error("GameStateLoad: '%s' is not a valid key.", infile.key.c_str());
				}
			}
		}
		infile.close();
	}

	stance_frames = std::max(1, stance_frames);
	stance_ticks_per_frame = std::max(1, (stance_duration / stance_frames));

	color_normal = font->getColor("menu_normal");
}

void GameStateLoad::loadGraphics() {
	Image *graphics;

	graphics = render_device->loadImage("images/menus/game_slots.png");
	if (graphics) {
		background = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/game_slot_select.png",
			   "Couldn't load image", false);
	if (graphics) {
		selection = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/portrait_border.png",
			   "Couldn't load image", false);
	if (graphics) {
		portrait_border = graphics->createSprite();
		graphics->unref();
	}
}

void GameStateLoad::loadPortrait(int slot) {

	Image *graphics;

	if (portrait) {
		delete portrait;
		portrait = NULL;
	}

	if (slot < 0) return;

	if (stats[slot].name == "") return;

	graphics = render_device->loadImage(stats[slot].gfx_portrait);
	if (graphics) {
		portrait = graphics->createSprite();
		portrait->setDestX(portrait_dest.x);
		portrait->setDestY(portrait_dest.y);
		portrait->setClipW(portrait_dest.w);
		portrait->setClipH(portrait_dest.h);
		graphics->unref();
	}

}

void GameStateLoad::readGameSlots() {
	for (int i=0; i<GAME_SLOT_MAX; i++) {
		readGameSlot(i);
	}
}

std::string GameStateLoad::getMapName(const std::string& map_filename) {
	FileParser infile;
	if (!infile.open(map_filename, true, "")) return "";
	std::string map_name = "";

	while (map_name == "" && infile.next()) {
		if (infile.key == "title")
			map_name = msg->get(infile.val);
	}

	infile.close();
	return map_name;
}

void GameStateLoad::readGameSlot(int slot) {

	std::stringstream filename;
	FileParser infile;

	// abort if not a valid slot number
	if (slot < 0 || slot >= GAME_SLOT_MAX) return;

	// save slots are named save#.txt
	filename << PATH_USER;
	if (SAVE_PREFIX.length() > 0)
		filename << SAVE_PREFIX << "_";
	filename << "save" << (slot+1) << ".txt";

	if (!infile.open(filename.str(),false, "")) return;

	while (infile.next()) {

		// load (key=value) pairs
		if (infile.key == "name")
			stats[slot].name = infile.val;
		else if (infile.key == "class") {
			stats[slot].character_class = infile.nextValue();
			stats[slot].character_subclass = infile.nextValue();
		}
		else if (infile.key == "xp")
			stats[slot].xp = toInt(infile.val);
		else if (infile.key == "build") {
			stats[slot].physical_character = toInt(infile.nextValue());
			stats[slot].mental_character = toInt(infile.nextValue());
			stats[slot].offense_character = toInt(infile.nextValue());
			stats[slot].defense_character = toInt(infile.nextValue());
		}
		else if (infile.key == "equipped") {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				equipped[slot].push_back(toInt(repeat_val));
				repeat_val = infile.nextValue();
			}
		}
		else if (infile.key == "option") {
			stats[slot].gfx_base = infile.nextValue();
			stats[slot].gfx_head = infile.nextValue();
			stats[slot].gfx_portrait = infile.nextValue();
		}
		else if (infile.key == "spawn") {
			current_map[slot] = getMapName(infile.nextValue());
		}
		else if (infile.key == "permadeath") {
			stats[slot].permadeath = toBool(infile.val);
		}
	}
	infile.close();

	stats[slot].recalc();
	loadPreview(slot);

}

void GameStateLoad::loadPreview(int slot) {

	Image *graphics;
	std::vector<std::string> img_gfx;

	for (unsigned int i=0; i<sprites[slot].size(); i++) {
		if (sprites[slot][i])
			delete sprites[slot][i];
	}
	sprites[slot].clear();

	// fall back to default if it exists
	for (unsigned int i=0; i<preview_layer.size(); i++) {
		bool exists = fileExists(mods->locate("animations/avatar/" + stats[slot].gfx_base + "/default_" + preview_layer[i] + ".txt"));
		if (exists) {
			img_gfx.push_back("default_" + preview_layer[i]);
		}
		else if (preview_layer[i] == "head") {
			img_gfx.push_back(stats[slot].gfx_head);
		}
		else {
			img_gfx.push_back("");
		}
	}

	for (unsigned int i=0; i<equipped[slot].size(); i++) {
		if ((unsigned)equipped[slot][i] > items->items.size()-1) {
			logError("GameStateLoad: Item in save slot %d with id=%d is out of bounds 1-%d. Your savegame is broken or you might be using an incompatible savegame/mod\n", slot+1, equipped[slot][i], (int)items->items.size()-1);
			continue;
		}

		if (equipped[slot][i] > 0 && !preview_layer.empty()) {
			std::vector<std::string>::iterator found = find(preview_layer.begin(), preview_layer.end(), items->items[equipped[slot][i]].type);
			if (found != preview_layer.end())
				img_gfx[distance(preview_layer.begin(), found)] = items->items[equipped[slot][i]].gfx;
		}
	}

	// composite the hero graphic
	sprites[slot].resize(img_gfx.size());
	for (unsigned int i=0; i<img_gfx.size(); i++) {
		if (img_gfx[i] == "")
			continue;

		graphics = render_device->loadImage("images/avatar/" + stats[slot].gfx_base + "/preview/" + img_gfx[i] + ".png");
		sprites[slot][i] = NULL;
		if (graphics) {
			sprites[slot][i] = graphics->createSprite();
			sprites[slot][i]->setClip(0, 0,
									  sprites[slot][i]->getGraphicsWidth(),
									  sprites[slot][i]->getGraphicsHeight());
			graphics->unref();
		}
	}

}


void GameStateLoad::logic() {

	// animate the avatar preview images
	if (stance_type == PLAY_ONCE && frame_ticker < stance_duration) {
		current_frame = frame_ticker / stance_ticks_per_frame;
		frame_ticker++;
	}
	else {
		if (stance_type == LOOPED) {
			if (frame_ticker == stance_duration) frame_ticker = 0;
			current_frame = frame_ticker / stance_ticks_per_frame;
		}
		else if (stance_type == BACK_FORTH) {
			if (frame_ticker == stance_duration*2) frame_ticker = 0;
			if (frame_ticker < stance_duration)
				current_frame = frame_ticker / stance_ticks_per_frame;
			else
				current_frame = ((stance_duration*2) - frame_ticker -1) / stance_ticks_per_frame;
		}
		frame_ticker++;
	}

	if (!confirm->visible) {
		tablist.logic(true);
		if (button_exit->checkClick() || (inpt->pressing[CANCEL] && !inpt->lock[CANCEL])) {
			inpt->lock[CANCEL] = true;
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
		}

		if (loading_requested) {
			loading = true;
			loading_requested = false;
			logicLoading();
		}

		if (button_action->checkClick()) {
			if (stats[selected_slot].name == "") {
				// create a new game
				GameStateNew* newgame = new GameStateNew();
				newgame->game_slot = selected_slot + 1;
				requestedGameState = newgame;
				delete_items = false;
			}
			else {
				loading_requested = true;
			}
		}
		if (button_alternate->checkClick()) {
			// Display pop-up to make sure save should be deleted
			confirm->visible = true;
			confirm->render();
		}
		// check clicking game slot
		if (inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
			for (int i=0; i<GAME_SLOT_MAX; i++) {
				if (isWithin(slot_pos[i], inpt->mouse)) {
					inpt->lock[MAIN1] = true;
					selected_slot = i;
					updateButtons();
				}
			}
		}

		// Allow characters to be navigateable via up/down keys
		if (inpt->pressing[UP] && !inpt->lock[UP]) {
			inpt->lock[UP] = true;
			selected_slot = (--selected_slot < 0) ? GAME_SLOT_MAX - 1 : selected_slot;
			updateButtons();
		}

		if (inpt->pressing[DOWN] && !inpt->lock[DOWN]) {
			inpt->lock[DOWN] = true;
			selected_slot = (++selected_slot == GAME_SLOT_MAX) ? 0 : selected_slot;
			updateButtons();
		}

	}
	else if (confirm->visible) {
		confirm->logic();
		if (confirm->confirmClicked) {
			std::stringstream filename;
			filename.str("");
			filename << PATH_USER;
			if (SAVE_PREFIX.length() > 0)
				filename << SAVE_PREFIX << "_";
			filename << "save" << (selected_slot+1) << ".txt";

			if (remove(filename.str().c_str()) != 0)
				logError("GameStateLoad: Error deleting save from path");

			if (stats[selected_slot].permadeath) {
				// Remove stash
				std::stringstream ss;
				ss.str("");
				ss << PATH_USER;
				if (SAVE_PREFIX.length() > 0)
					ss << SAVE_PREFIX << "_";
				ss << "stash_HC" << (selected_slot+1) << ".txt";
				if (remove(ss.str().c_str()) != 0)
					logError("GameStateLoad: Error deleting hardcore stash in slot %d\n", selected_slot+1);
			}

			stats[selected_slot] = StatBlock();
			readGameSlot(selected_slot);

			updateButtons();

			confirm->visible = false;
			confirm->confirmClicked = false;
		}
	}
}

void GameStateLoad::logicLoading() {
	// load an existing game
	delete_items = false;
	GameStatePlay* play = new GameStatePlay();
	play->resetGame();
	play->game_slot = selected_slot + 1;
	play->loadGame();
	play->loadPowerTree();
	requestedGameState = play;
	loaded = true;
	loading = false;
}

void GameStateLoad::updateButtons() {
	loadPortrait(selected_slot);

	if (button_action->enabled == false) {
		button_action->enabled = true;
		tablist.add(button_action);
	}
	button_action->tooltip = "";
	if (stats[selected_slot].name == "") {
		button_action->label = msg->get("New Game");
		if (!fileExists(mods->locate("maps/spawn.txt"))) {
			button_action->enabled = false;
			tablist.remove(button_action);
			button_action->tooltip = msg->get("Enable a story mod to continue");
		}
		button_alternate->enabled = false;
		tablist.remove(button_alternate);
	}
	else {
		if (button_alternate->enabled == false) {
			button_alternate->enabled = true;
			tablist.add(button_alternate);
		}
		button_action->label = msg->get("Load Game");
		if (current_map[selected_slot] == "") {
			if (!fileExists(mods->locate("maps/spawn.txt"))) {
				button_action->enabled = false;
				tablist.remove(button_action);
				button_action->tooltip = msg->get("Enable a story mod to continue");
			}
		}
	}
	button_action->refresh();
	button_alternate->refresh();
}

void GameStateLoad::render() {

	Rect src;
	Rect dest;

	// display background
	src.w = gameslot_pos.w;
	src.h = gameslot_pos.h * GAME_SLOT_MAX;
	src.x = src.y = 0;
	dest.x = slot_pos[0].x;
	dest.y = slot_pos[0].y;

	// display background
	if (background != NULL) {
		background->setClip(src);
		background->setDest(dest);
		render_device->render(background);
	}

	// display selection
	if (selected_slot >= 0 && selection != NULL) {
		selection->setDest(slot_pos[selected_slot]);
		render_device->render(selection);
	}


	// portrait
	if (selected_slot >= 0 && portrait != NULL && portrait_border != NULL) {
		render_device->render(portrait);
		dest.x = int(portrait->getDest().x);
		dest.y = int(portrait->getDest().y);
		portrait_border->setDest(dest);
		render_device->render(portrait_border);
	}

	Point label;
	std::stringstream ss;

	if (loading_requested || loading || loaded) {
		label.x = loading_pos.x + (VIEW_W - FRAME_W)/2;
		label.y = loading_pos.y + (VIEW_H - FRAME_H)/2;

		if ( loaded) {
			label_loading->set(msg->get("Entering game world..."));
		}
		else {
			label_loading->set(msg->get("Loading saved game..."));
		}

		label_loading->set(label.x, label.y, loading_pos.justify, loading_pos.valign, label_loading->get(), color_normal, loading_pos.font_style);
		label_loading->render();
	}

	Color color_permadeath_enabled = font->getColor("hardcore_color_name");
	// display text
	for (int slot=0; slot<GAME_SLOT_MAX; slot++) {
		if (stats[slot].name != "") {
			Color color_used = stats[slot].permadeath ? color_permadeath_enabled : color_normal;

			// name
			label.x = slot_pos[slot].x + name_pos.x;
			label.y = slot_pos[slot].y + name_pos.y;
			label_name[slot]->set(label.x, label.y, name_pos.justify, name_pos.valign, stats[slot].name, color_used, name_pos.font_style);
			label_name[slot]->render();

			// level
			ss.str("");
			label.x = slot_pos[slot].x + level_pos.x;
			label.y = slot_pos[slot].y + level_pos.y;
			ss << msg->get("Level %d %s", stats[slot].level, stats[slot].getShortClass());
			if (stats[slot].permadeath)
				ss << ", " + msg->get("Permadeath");
			label_level[slot]->set(label.x, label.y, level_pos.justify, level_pos.valign, ss.str(), color_normal, level_pos.font_style);
			label_level[slot]->render();

			// map
			label.x = slot_pos[slot].x + map_pos.x;
			label.y = slot_pos[slot].y + map_pos.y;
			label_map[slot]->set(label.x, label.y, map_pos.justify, map_pos.valign, current_map[slot], color_normal, map_pos.font_style);
			label_map[slot]->render();

			// render character preview
			dest.x = slot_pos[slot].x + sprites_pos.x;
			dest.y = slot_pos[slot].y + sprites_pos.y;
			src.x = current_frame * preview_pos.h;
			src.y = 0;
			src.w = src.h = preview_pos.h;

			for (unsigned int i=0; i<sprites[slot].size(); i++) {
				if (sprites[slot][i] == NULL) continue;
				sprites[slot][i]->setClip(src);
				sprites[slot][i]->setDest(dest);
				render_device->render(sprites[slot][i]);
			}
		}
		else {
			label.x = slot_pos[slot].x + name_pos.x;
			label.y = slot_pos[slot].y + name_pos.y;
			label_name[slot]->set(label.x, label.y, name_pos.justify, name_pos.valign, msg->get("Empty Slot"), color_normal, name_pos.font_style);
			label_name[slot]->render();
		}
	}
	// display warnings
	if (confirm->visible) confirm->render();

	// display buttons
	button_exit->render();
	button_action->render();
	button_alternate->render();
}

GameStateLoad::~GameStateLoad() {
	if (background)
		delete background;
	if (selection)
		delete selection;
	if (portrait_border)
		delete portrait_border;
	if (portrait)
		delete portrait;

	delete button_exit;
	delete button_action;
	delete button_alternate;

	if (delete_items) {
		delete items;
		items = NULL;
	}

	for (int slot=0; slot<GAME_SLOT_MAX; slot++) {
		for (unsigned int i=0; i<sprites[slot].size(); i++) {
			delete sprites[slot][i];
		}
		sprites[slot].clear();
	}
	for (int i=0; i<GAME_SLOT_MAX; i++) {
		delete label_name[i];
		delete label_level[i];
		delete label_map[i];
	}
	delete label_loading;
	delete confirm;
}
