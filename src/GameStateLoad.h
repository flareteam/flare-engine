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
 *
 * Display the current save-game slots
 * Allow the player to continue a previous game
 * Allow the player to start a new game
 * Allow the player to abandon a previous game
 */

#ifndef GAMESTATELOAD_H
#define GAMESTATELOAD_H

#include "CommonIncludes.h"
#include "GameSlotPreview.h"
#include "GameState.h"
#include "StatBlock.h"
#include "Utils.h"
#include "WidgetLabel.h"

class ItemManager;
class MenuConfirm;
class WidgetButton;
class WidgetLabel;
class WidgetScrollBar;

class GameSlot {
public:
	unsigned id;

	StatBlock stats;
	std::string current_map;
	unsigned long time_played;

	std::vector<int> equipped;
	GameSlotPreview preview;
	Timer preview_turn_timer;

	WidgetLabel label_name;
	WidgetLabel label_level;
	WidgetLabel label_class;
	WidgetLabel label_map;
	WidgetLabel label_slot_number;

	GameSlot();
	~GameSlot();
};

class GameStateLoad : public GameState {
private:

	void loadGraphics();
	void loadPortrait(int slot);
	std::string getMapName(const std::string& map_filename);
	void updateButtons();
	void refreshWidgets();
	void logicLoading();
	void readGameSlots();
	void loadPreview(GameSlot *slot);

	void scrollUp();
	void scrollDown();
	void scrollToSelected();
	void refreshScrollBar();
	void setSelectedSlot(int slot);

	void refreshSavePaths();

	TabList tablist;

	WidgetButton *button_exit;
	WidgetButton *button_new;
	WidgetButton *button_load;
	WidgetButton *button_delete;
	WidgetLabel *label_loading;
	WidgetScrollBar *scrollbar;

	MenuConfirm *confirm;

	Sprite *background;
	Sprite *selection;
	Sprite *portrait_border;
	Sprite *portrait;
	std::vector<Rect> slot_pos;

	std::vector<GameSlot *> game_slots;

	bool loading_requested;
	bool loading;
	bool loaded;
	bool delete_items;

	LabelInfo name_pos;
	LabelInfo level_pos;
	LabelInfo class_pos;
	LabelInfo map_pos;
	LabelInfo slot_number_pos;
	Point sprites_pos;

	Rect portrait_dest;

	Rect gameslot_pos;

	int selected_slot;
	int visible_slots;
	int scroll_offset;
	bool has_scroll_bar;
	int game_slot_max;
	int text_trim_boundary;

public:
	GameStateLoad();
	~GameStateLoad();

	void logic();
	void render();
};

#endif
