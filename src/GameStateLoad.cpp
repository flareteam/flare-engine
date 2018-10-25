/*
Copyright © 2011-2012 Clint Bellanger
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
 * GameStateLoad
 */

#include "Avatar.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateLoad.h"
#include "GameStateTitle.h"
#include "GameStatePlay.h"
#include "GameStateNew.h"
#include "InputState.h"
#include "ItemManager.h"
#include "MenuConfirm.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "SaveLoad.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetScrollBar.h"

bool compareSaveDirs(const std::string& dir1, const std::string& dir2) {
	int first = Parse::toInt(dir1);
	int second = Parse::toInt(dir2);

	return first < second;
}

GameSlot::GameSlot()
	: id(0)
	, time_played(0)
	, preview_turn_timer(settings->max_frames_per_sec/2)
{
	preview_turn_timer.reset(Timer::BEGIN);
}

GameSlot::~GameSlot() {
}

GameStateLoad::GameStateLoad() : GameState()
	, background(NULL)
	, selection(NULL)
	, portrait_border(NULL)
	, portrait(NULL)
	, loading_requested(false)
	, loading(false)
	, loaded(false)
	, delete_items(true)
	, selected_slot(-1)
	, visible_slots(0)
	, scroll_offset(0)
	, has_scroll_bar(false)
	, game_slot_max(4)
	, text_trim_boundary(0) {

	if (items == NULL)
		items = new ItemManager();

	label_loading = new WidgetLabel();

	// Confirmation box to confirm deleting
	confirm = new MenuConfirm(msg->get("Delete Save"), msg->get("Delete this save?"));
	button_exit = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_exit->setLabel(msg->get("Exit to Title"));

	button_new = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_new->setLabel(msg->get("New Game"));
	button_new->enabled = true;

	button_load = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_load->setLabel(msg->get("Choose a Slot"));
	button_load->enabled = false;

	button_delete = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_delete->setLabel(msg->get("Delete Save"));
	button_delete->enabled = false;

	scrollbar = new WidgetScrollBar(WidgetScrollBar::DEFAULT_FILE);

	// Set up tab list
	tablist = TabList();
	tablist.setScrollType(Widget::SCROLL_HORIZONTAL);
	tablist.ignore_no_mouse = true;
	tablist.add(button_exit);
	tablist.add(button_new);

	// Read positions from config file
	FileParser infile;

	// @CLASS GameStateLoad|Description of menus/gameload.txt
	if (infile.open("menus/gameload.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR button_new|int, int, alignment : X, Y, Alignment|Position of the "New Game" button.
			if (infile.key == "button_new") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_new->setBasePos(x, y, a);
			}
			// @ATTR button_load|int, int, alignment : X, Y, Alignment|Position of the "Load Game" button.
			else if (infile.key == "button_load") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_load->setBasePos(x, y, a);
			}
			// @ATTR button_delete|int, int, alignment : X, Y, Alignment|Position of the "Delete Save" button.
			else if (infile.key == "button_delete") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_delete->setBasePos(x, y, a);
			}
			// @ATTR button_exit|int, int, alignment : X, Y, Alignment|Position of the "Exit to Title" button.
			else if (infile.key == "button_exit") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_exit->setBasePos(x, y, a);
			}
			// @ATTR portrait|rectangle|Position and dimensions of the portrait image.
			else if (infile.key == "portrait") {
				portrait_dest = Parse::toRect(infile.val);
			}
			// @ATTR gameslot|rectangle|Position and dimensions of the first game slot.
			else if (infile.key == "gameslot") {
				gameslot_pos = Parse::toRect(infile.val);
			}
			// @ATTR name|label|The label for the hero's name. Position is relative to game slot position.
			else if (infile.key == "name") {
				name_pos = Parse::popLabelInfo(infile.val);
			}
			// @ATTR level|label|The label for the hero's level. Position is relative to game slot position.
			else if (infile.key == "level") {
				level_pos = Parse::popLabelInfo(infile.val);
			}
			// @ATTR class|label|The label for the hero's class. Position is relative to game slot position.
			else if (infile.key == "class") {
				class_pos = Parse::popLabelInfo(infile.val);
			}
			// @ATTR map|label|The label for the hero's current location. Position is relative to game slot position.
			else if (infile.key == "map") {
				map_pos = Parse::popLabelInfo(infile.val);
			}
			// @ATTR slot_number|label|The label for the save slot index. Position is relative to game slot position.
			else if (infile.key == "slot_number") {
				slot_number_pos = Parse::popLabelInfo(infile.val);
			}
			// @ATTR loading_label|label|The label for the "Entering game world..."/"Loading saved game..." text.
			else if (infile.key == "loading_label") {
				label_loading->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR sprite|point|Position for the avatar preview image in each slot
			else if (infile.key == "sprite") {
				sprites_pos = Parse::toPoint(infile.val);
			}
			// @ATTR visible_slots|int|The maximum numbers of visible save slots.
			else if (infile.key == "visible_slots") {
				game_slot_max = Parse::toInt(infile.val);

				// can't have less than 1 game slot visible
				game_slot_max = std::max(game_slot_max, 1);
			}
			// @ATTR text_trim_boundary|int|The position of the right-side boundary where text will be shortened with an ellipsis. Position is relative to game slot position.
			else if (infile.key == "text_trim_boundary") {
				text_trim_boundary = Parse::toInt(infile.val);
			}
			else {
				infile.error("GameStateLoad: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// prevent text from overflowing on the right edge of game slots
	if (text_trim_boundary == 0 || text_trim_boundary > gameslot_pos.w)
		text_trim_boundary = gameslot_pos.w;

	button_new->refresh();
	button_load->refresh();
	button_delete->refresh();

	loadGraphics();
	readGameSlots();
	refreshSavePaths();

	refreshWidgets();
	updateButtons();

	// if we specified a slot to load at launch, load it now
	if (!settings->load_slot.empty()) {
		size_t load_slot_id = Parse::toInt(settings->load_slot) - 1;
		settings->load_slot.clear();

		if (load_slot_id < game_slots.size()) {
			setSelectedSlot(static_cast<int>(load_slot_id));
			loading_requested = true;
		}
	}
	else if (settings->prev_save_slot >= 0 && static_cast<size_t>(settings->prev_save_slot) < game_slots.size()) {
		setSelectedSlot(settings->prev_save_slot);
		scrollToSelected();
		updateButtons();
	}

	render_device->setBackgroundColor(Color(0,0,0,0));
}

void GameStateLoad::loadGraphics() {
	Image *graphics;

	graphics = render_device->loadImage("images/menus/game_slots.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		background = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/game_slot_select.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		selection = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/portrait_border.png", RenderDevice::ERROR_NORMAL);
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

	if (slot < 0 || static_cast<size_t>(slot) >= game_slots.size() || !game_slots[slot])
		return;

	if (game_slots[slot]->stats.name == "") return;

	graphics = render_device->loadImage(game_slots[slot]->stats.gfx_portrait, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		portrait = graphics->createSprite();
		portrait->setClip(0, 0, portrait_dest.w, portrait_dest.h);
		graphics->unref();
	}

}

void GameStateLoad::readGameSlots() {
	FileParser infile;
	std::stringstream filename;
	std::vector<std::string> save_dirs;

	Filesystem::getDirList(settings->path_user + "saves/" + eset->misc.save_prefix, save_dirs);
	std::sort(save_dirs.begin(), save_dirs.end(), compareSaveDirs);

	// save dirs can only be >= 1
	for (size_t i=save_dirs.size(); i>0; --i) {
		if (Parse::toInt(save_dirs[i-1]) < 1)
			save_dirs.erase(save_dirs.begin() + (i-1));
	}

	game_slots.resize(save_dirs.size(), NULL);

	visible_slots = (game_slot_max > static_cast<int>(game_slots.size()) ? static_cast<int>(game_slots.size()) : game_slot_max);

	for (size_t i=0; i<save_dirs.size(); ++i){
		// save data is stored in slot#/avatar.txt
		filename.str("");
		filename << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << save_dirs[i] << "/avatar.txt";

		if (!infile.open(filename.str(), !FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) continue;

		game_slots[i] = new GameSlot();
		game_slots[i]->id = Parse::toInt(save_dirs[i]);
		game_slots[i]->stats.hero = true;
		game_slots[i]->label_name.setFromLabelInfo(name_pos);
		game_slots[i]->label_level.setFromLabelInfo(level_pos);
		game_slots[i]->label_class.setFromLabelInfo(class_pos);
		game_slots[i]->label_map.setFromLabelInfo(map_pos);
		game_slots[i]->label_slot_number.setFromLabelInfo(slot_number_pos);

		while (infile.next()) {

			// load (key=value) pairs
			if (infile.key == "name")
				game_slots[i]->stats.name = infile.val;
			else if (infile.key == "class") {
				game_slots[i]->stats.character_class = Parse::popFirstString(infile.val);
				game_slots[i]->stats.character_subclass = Parse::popFirstString(infile.val);
			}
			else if (infile.key == "xp")
				game_slots[i]->stats.xp = Parse::toInt(infile.val);
			else if (infile.key == "build") {
				for (size_t j = 0; j < eset->primary_stats.list.size(); ++j) {
					game_slots[i]->stats.primary[j] = Parse::popFirstInt(infile.val);
				}
			}
			else if (infile.key == "equipped") {
				std::string repeat_val = Parse::popFirstString(infile.val);
				while (repeat_val != "") {
					game_slots[i]->equipped.push_back(Parse::toInt(repeat_val));
					repeat_val = Parse::popFirstString(infile.val);
				}
			}
			else if (infile.key == "option") {
				game_slots[i]->stats.gfx_base = Parse::popFirstString(infile.val);
				game_slots[i]->stats.gfx_head = Parse::popFirstString(infile.val);
				game_slots[i]->stats.gfx_portrait = Parse::popFirstString(infile.val);
			}
			else if (infile.key == "spawn") {
				game_slots[i]->current_map = getMapName(Parse::popFirstString(infile.val));
			}
			else if (infile.key == "permadeath") {
				game_slots[i]->stats.permadeath = Parse::toBool(infile.val);
			}
			else if (infile.key == "time_played") {
				game_slots[i]->time_played = Parse::toUnsignedLong(infile.val);
			}
		}
		infile.close();

		game_slots[i]->stats.recalc();
		game_slots[i]->stats.direction = 6;
		game_slots[i]->preview.setStatBlock(&(game_slots[i]->stats));

		loadPreview(game_slots[i]);
	}
}

std::string GameStateLoad::getMapName(const std::string& map_filename) {
	FileParser infile;
	if (!infile.open(map_filename, FileParser::MOD_FILE, FileParser::ERROR_NONE))
		return "";

	std::string map_name = "";

	while (map_name == "" && infile.next()) {
		if (infile.key == "title")
			map_name = msg->get(infile.val);
	}

	infile.close();
	return map_name;
}

void GameStateLoad::loadPreview(GameSlot* slot) {
	if (!slot) return;

	std::vector<std::string> img_gfx;
	std::vector<std::string> &preview_layer = slot->preview.layer_reference_order;

	// fall back to default if it exists
	for (unsigned int i=0; i<preview_layer.size(); i++) {
		bool exists = Filesystem::fileExists(mods->locate("animations/avatar/" + slot->stats.gfx_base + "/default_" + preview_layer[i] + ".txt"));
		if (exists) {
			img_gfx.push_back("default_" + preview_layer[i]);
		}
		else if (preview_layer[i] == "head") {
			img_gfx.push_back(slot->stats.gfx_head);
		}
		else {
			img_gfx.push_back("");
		}
	}

	for (unsigned int i=0; i<slot->equipped.size(); i++) {
		if (static_cast<unsigned>(slot->equipped[i]) > items->items.size()-1) {
			Utils::logError("GameStateLoad: Item in save slot %d with id=%d is out of bounds 1-%d. Your savegame is broken or you might be using an incompatible savegame/mod", slot->id, slot->equipped[i], static_cast<int>(items->items.size())-1);
			continue;
		}

		if (slot->equipped[i] > 0 && !preview_layer.empty() && static_cast<unsigned>(slot->equipped[i]) < items->items.size()) {
			std::vector<std::string>::iterator found = find(preview_layer.begin(), preview_layer.end(), items->items[slot->equipped[i]].type);
			if (found != preview_layer.end())
				img_gfx[distance(preview_layer.begin(), found)] = items->items[slot->equipped[i]].gfx;
		}
	}

	slot->preview.loadGraphics(img_gfx);
}


void GameStateLoad::logic() {

	if (inpt->window_resized)
		refreshWidgets();

	for (size_t i = 0; i < game_slots.size(); ++i) {
		if (!game_slots[i])
			continue;

		if (static_cast<int>(i) == selected_slot) {
			game_slots[i]->preview_turn_timer.tick();

			if (game_slots[i]->preview_turn_timer.isEnd()) {
				game_slots[i]->preview_turn_timer.reset(Timer::BEGIN);

				game_slots[i]->stats.direction++;
				if (game_slots[i]->stats.direction > 7)
					game_slots[i]->stats.direction = 0;
			}
		}
		game_slots[i]->preview.logic();
	}

	if (!confirm->visible) {
		tablist.logic();
		if (button_exit->checkClick() || (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL])) {
			inpt->lock[Input::CANCEL] = true;
			showLoading();
			setRequestedGameState(new GameStateTitle());
		}

		if (loading_requested) {
			loading = true;
			loading_requested = false;
			logicLoading();
		}

		if (button_new->checkClick()) {
			// create a new game
			showLoading();
			GameStateNew* newgame = new GameStateNew();
			newgame->game_slot = (game_slots.empty() ? 1 : game_slots.back()->id+1);
			delete_items = false;
			setRequestedGameState(newgame);
		}
		else if (button_load->checkClick()) {
			loading_requested = true;
		}
		else if (button_delete->checkClick()) {
			// Display pop-up to make sure save should be deleted
			confirm->visible = true;
			confirm->render();
		}
		else if (game_slots.size() > 0) {
			Rect scroll_area = slot_pos[0];
			scroll_area.h = slot_pos[0].h * game_slot_max;

			if (Utils::isWithinRect(scroll_area, inpt->mouse)) {
				if (inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
					for (int i=0; i<visible_slots; ++i) {
						if (Utils::isWithinRect(slot_pos[i], inpt->mouse)) {
							inpt->lock[Input::MAIN1] = true;
							setSelectedSlot(i + scroll_offset);
							updateButtons();
							break;
						}
					}
				}
				else if (inpt->scroll_up) {
					scrollUp();
				}
				else if (inpt->scroll_down) {
					scrollDown();
				}
			}
			else if (has_scroll_bar) {
				switch (scrollbar->checkClick()) {
					case 1:
						scrollUp();
						break;
					case 2:
						scrollDown();
						break;
					case 3:
						scroll_offset = scrollbar->getValue();
						if (scroll_offset >= static_cast<int>(game_slots.size()) - visible_slots) {
							scroll_offset = static_cast<int>(game_slots.size()) - visible_slots;
						}
						break;
					default:
						break;
				}
			}

			// Allow characters to be navigateable via up/down keys
			if (inpt->pressing[Input::UP] && !inpt->lock[Input::UP] && selected_slot > 0) {
				inpt->lock[Input::UP] = true;
				setSelectedSlot(selected_slot - 1);
				scrollToSelected();
				updateButtons();
			}
			else if (inpt->pressing[Input::DOWN] && !inpt->lock[Input::DOWN] && selected_slot < static_cast<int>(game_slots.size()) - 1) {
				inpt->lock[Input::DOWN] = true;
				setSelectedSlot(selected_slot + 1);
				scrollToSelected();
				updateButtons();
			}
		}
	}
	else if (confirm->visible) {
		confirm->logic();
		if (confirm->confirmClicked) {
			Utils::removeSaveDir(game_slots[selected_slot]->id);

			delete game_slots[selected_slot];
			game_slots[selected_slot] = NULL;
			game_slots.erase(game_slots.begin()+selected_slot);

			visible_slots = (game_slot_max > static_cast<int>(game_slots.size()) ? static_cast<int>(game_slots.size()) : game_slot_max);
			setSelectedSlot(-1);

			while (scroll_offset + visible_slots > static_cast<int>(game_slots.size())) {
				scroll_offset--;
			}

			updateButtons();

			confirm->visible = false;
			confirm->confirmClicked = false;

			refreshSavePaths();

			settings->prev_save_slot = -1;
		}
	}
}

void GameStateLoad::logicLoading() {
	// load an existing game
	inpt->lock_all = true;
	delete_items = false;
	showLoading();
	GameStatePlay* play = new GameStatePlay();
	play->resetGame();
	save_load->setGameSlot(game_slots[selected_slot]->id);
	save_load->loadGame();
	loaded = true;
	loading = false;
	setRequestedGameState(play);
}

void GameStateLoad::updateButtons() {
	loadPortrait(selected_slot);

	// check status of New Game button
	if (!Filesystem::fileExists(mods->locate("maps/spawn.txt"))) {
		button_new->enabled = false;
		tablist.remove(button_new);
		button_new->tooltip = msg->get("Enable a story mod to continue");
	}

	if (selected_slot >= 0 && game_slots[selected_slot]) {
		// slot selected: we can load/delete
		if (button_load->enabled == false) {
			button_load->enabled = true;
			tablist.add(button_load);
		}
		button_load->tooltip = "";

		if (button_delete->enabled == false) {
			button_delete->enabled = true;
			tablist.add(button_delete);
		}

		button_load->setLabel(msg->get("Load Game"));
		if (game_slots[selected_slot]->current_map == "") {
			if (!Filesystem::fileExists(mods->locate("maps/spawn.txt"))) {
				button_load->enabled = false;
				tablist.remove(button_load);
				button_load->tooltip = msg->get("Enable a story mod to continue");
			}
		}
	}
	else {
		// no slot selected: can't load/delete
		button_load->setLabel(msg->get("Choose a Slot"));
		button_load->enabled = false;
		tablist.remove(button_load);

		button_delete->enabled = false;
		tablist.remove(button_delete);
	}

	button_new->refresh();
	button_load->refresh();
	button_delete->refresh();

	refreshWidgets();
}

void GameStateLoad::refreshWidgets() {
	button_exit->setPos(0, 0);
	button_new->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	button_load->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	button_delete->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);

	if (portrait) {
		portrait->setDest(portrait_dest.x + ((settings->view_w - eset->resolutions.frame_w)/2), portrait_dest.y + ((settings->view_h - eset->resolutions.frame_h)/2));
	}

	slot_pos.resize(visible_slots);
	for (size_t i=0; i<slot_pos.size(); i++) {
		slot_pos[i].x = gameslot_pos.x + (settings->view_w - eset->resolutions.frame_w)/2;
		slot_pos[i].h = gameslot_pos.h;
		slot_pos[i].y = gameslot_pos.y + (settings->view_h - eset->resolutions.frame_h)/2 + (static_cast<int>(i) * gameslot_pos.h);
		slot_pos[i].w = gameslot_pos.w;
	}

	refreshScrollBar();
	confirm->align();
}

void GameStateLoad::scrollUp() {
	if (scroll_offset > 0)
		scroll_offset--;

	refreshScrollBar();
}

void GameStateLoad::scrollDown() {
	if (scroll_offset < static_cast<int>(game_slots.size()) - visible_slots)
		scroll_offset++;

	refreshScrollBar();
}

void GameStateLoad::scrollToSelected() {
	if (visible_slots == 0)
		return;

	scroll_offset = selected_slot - (selected_slot % visible_slots);

	if (scroll_offset < 0)
		scroll_offset = 0;
	else if (scroll_offset > static_cast<int>(game_slots.size()) - visible_slots)
		scroll_offset = selected_slot - visible_slots + 1;
}

void GameStateLoad::refreshScrollBar() {
	has_scroll_bar = (static_cast<int>(game_slots.size()) > game_slot_max);

	if (has_scroll_bar) {
		Rect scroll_pos;
		scroll_pos.x = slot_pos[0].x + slot_pos[0].w;
		scroll_pos.y = slot_pos[0].y;
		scroll_pos.w = scrollbar->pos_up.w;
		scroll_pos.h = (slot_pos[0].h * game_slot_max) - scrollbar->pos_down.h;
		scrollbar->refresh(scroll_pos.x, scroll_pos.y, scroll_pos.h, scroll_offset, static_cast<int>(game_slots.size()) - visible_slots);
	}
}

void GameStateLoad::render() {

	Rect src;
	Rect dest;


	// portrait
	if (selected_slot >= 0 && portrait != NULL && portrait_border != NULL) {
		render_device->render(portrait);
		dest.x = portrait->getDest().x;
		dest.y = portrait->getDest().y;
		portrait_border->setDestFromRect(dest);
		render_device->render(portrait_border);
	}

	std::stringstream ss;

	if (loading_requested || loading || loaded) {
		if ( loaded) {
			label_loading->setText(msg->get("Entering game world..."));
		}
		else {
			label_loading->setText(msg->get("Loading saved game..."));
		}

		label_loading->setPos((settings->view_w - eset->resolutions.frame_w) / 2, (settings->view_h - eset->resolutions.frame_h) / 2);
		label_loading->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));
		label_loading->render();
	}

	// display text
	for (int slot=0; slot<visible_slots; slot++) {
		int off_slot = slot+scroll_offset;

		// slot background
		if (background) {
			src.x = 0;
			src.y = (off_slot % 4) * gameslot_pos.h;
			dest.w = dest.h = 0;

			src.w = gameslot_pos.w;
			src.h = gameslot_pos.h;
			dest.x = slot_pos[slot].x;
			dest.y = slot_pos[slot].y;

			background->setClipFromRect(src);
			background->setDestFromRect(dest);
			render_device->render(background);
		}
		Point slot_dest = background->getDest();

		if (!game_slots[off_slot]) {
			WidgetLabel slot_error;
			slot_error.setFromLabelInfo(name_pos);
			slot_error.setPos(slot_pos[slot].x, slot_pos[slot].y);
			slot_error.setText(msg->get("Invalid save"));
			slot_error.setColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED));
			slot_error.render();
			continue;
		}

		Color name_color;
		if (game_slots[off_slot]->stats.permadeath)
			name_color = font->getColor(FontEngine::COLOR_HARDCORE_NAME);
		else
			name_color = font->getColor(FontEngine::COLOR_MENU_NORMAL);

		// name
		game_slots[off_slot]->label_name.setPos(slot_pos[slot].x, slot_pos[slot].y);
		game_slots[off_slot]->label_name.setText(game_slots[off_slot]->stats.name);
		game_slots[off_slot]->label_name.setColor(name_color);

		if (text_trim_boundary > 0 && game_slots[off_slot]->label_name.getBounds()->x + game_slots[off_slot]->label_name.getBounds()->w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_name.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_name.getBounds()->x - slot_dest.x));

		game_slots[off_slot]->label_name.render();

		// level
		ss.str("");
		ss << msg->get("Level %d", game_slots[off_slot]->stats.level);
		ss << " / " << Utils::getTimeString(game_slots[off_slot]->time_played);
		if (game_slots[off_slot]->stats.permadeath)
			ss << " / +";

		game_slots[off_slot]->label_level.setPos(slot_pos[slot].x, slot_pos[slot].y);
		game_slots[off_slot]->label_level.setText(ss.str());
		game_slots[off_slot]->label_level.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

		if (text_trim_boundary > 0 && game_slots[off_slot]->label_level.getBounds()->x + game_slots[off_slot]->label_level.getBounds()->w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_level.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_level.getBounds()->x - slot_dest.x));

		game_slots[off_slot]->label_level.render();

		// class
		game_slots[off_slot]->label_class.setPos(slot_pos[slot].x, slot_pos[slot].y);
		game_slots[off_slot]->label_class.setText(game_slots[off_slot]->stats.getLongClass());
		game_slots[off_slot]->label_class.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

		if (text_trim_boundary > 0 && game_slots[off_slot]->label_class.getBounds()->x + game_slots[off_slot]->label_class.getBounds()->w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_class.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_class.getBounds()->x - slot_dest.x));

		game_slots[off_slot]->label_class.render();

		// map
		game_slots[off_slot]->label_map.setPos(slot_pos[slot].x, slot_pos[slot].y);
		game_slots[off_slot]->label_map.setText(game_slots[off_slot]->current_map);
		game_slots[off_slot]->label_map.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

		if (text_trim_boundary > 0 && game_slots[off_slot]->label_map.getBounds()->x + game_slots[off_slot]->label_map.getBounds()->w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_map.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_map.getBounds()->x - slot_dest.x));

		game_slots[off_slot]->label_map.render();

		// render character preview
		dest.x = slot_pos[slot].x + sprites_pos.x;
		dest.y = slot_pos[slot].y + sprites_pos.y;
		game_slots[off_slot]->preview.setPos(Point(dest.x, dest.y));
		game_slots[off_slot]->preview.render();

		// slot number
		ss.str("");
		ss << "#" << off_slot + 1;

		game_slots[off_slot]->label_slot_number.setPos(slot_pos[slot].x, slot_pos[slot].y);
		game_slots[off_slot]->label_slot_number.setText(ss.str());
		game_slots[off_slot]->label_slot_number.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

		if (text_trim_boundary > 0 && game_slots[off_slot]->label_slot_number.getBounds()->x + game_slots[off_slot]->label_slot_number.getBounds()->w >= text_trim_boundary + slot_dest.x)
			game_slots[off_slot]->label_slot_number.setMaxWidth(text_trim_boundary - (game_slots[off_slot]->label_slot_number.getBounds()->x - slot_dest.x));

		game_slots[off_slot]->label_slot_number.render();
	}

	// display selection
	if (selected_slot >= scroll_offset && selected_slot < visible_slots+scroll_offset && selection != NULL) {
		selection->setDestFromRect(slot_pos[selected_slot-scroll_offset]);
		render_device->render(selection);
	}

	if (has_scroll_bar)
		scrollbar->render();

	// display warnings
	if (confirm->visible) confirm->render();

	// display buttons
	button_exit->render();
	button_new->render();
	button_load->render();
	button_delete->render();
}

void GameStateLoad::setSelectedSlot(int slot) {
	if (selected_slot != -1 && static_cast<size_t>(selected_slot) < game_slots.size() && game_slots[selected_slot]) {
		game_slots[selected_slot]->stats.direction = 6;
		game_slots[selected_slot]->preview_turn_timer.reset(Timer::BEGIN);
		game_slots[selected_slot]->preview.setAnimation("stance");
	}

	if (slot != -1 && static_cast<size_t>(slot) < game_slots.size() && game_slots[slot]) {
		game_slots[slot]->stats.direction = 6;
		game_slots[slot]->preview_turn_timer.reset(Timer::BEGIN);
		game_slots[slot]->preview.setAnimation("run");
	}

	selected_slot = slot;
}

void GameStateLoad::refreshSavePaths() {
	for (size_t i = 0; i < game_slots.size(); ++i) {
		if (game_slots[i] && game_slots[i]->id != i+1) {
			std::stringstream oldpath, newpath;
			oldpath << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << game_slots[i]->id;
			newpath << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << i+1;
			if (Filesystem::renameFile(oldpath.str(), newpath.str())) {
				game_slots[i]->id = static_cast<unsigned>(i+1);
			}
		}
	}
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
	delete button_new;
	delete button_load;
	delete button_delete;

	if (delete_items) {
		delete items;
		items = NULL;
	}

	for (size_t i = 0; i < game_slots.size(); ++i) {
		delete game_slots[i];
	}
	game_slots.clear();

	delete label_loading;
	delete scrollbar;
	delete confirm;
}
