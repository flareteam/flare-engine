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
 *
 * Display the current save-game slots
 * Allow the player to continue a previous game
 * Allow the player to start a new game
 * Allow the player to abandon a previous game
 */

#ifndef GAMESTATELOAD_H
#define GAMESTATELOAD_H

#include "Animation.h"
#include "CommonIncludes.h"
#include "GameState.h"
#include "StatBlock.h"
#include "WidgetLabel.h"

class ItemManager;
class MenuConfirm;
class WidgetButton;
class WidgetLabel;
class WidgetScrollBar;

const int GAME_SLOT_MAX = 4;

class GameSlot {
public:
	unsigned id;

	StatBlock stats;
	std::string current_map;

	std::vector<int> equipped;
	std::vector<Sprite *> sprites;

	WidgetLabel label_name;
	WidgetLabel label_level;
	WidgetLabel label_class;
	WidgetLabel label_map;

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
	void refreshScrollBar();

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
	std::vector<std::string> preview_layer;
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
	LabelInfo loading_pos;
	Point sprites_pos;

	Rect portrait_dest;

	// animation info
	int current_frame;
	int frame_ticker;
	int stance_ticks_per_frame;
	int stance_duration;
	animation_type stance_type;

	Rect gameslot_pos;
	Rect preview_pos;

	Color color_normal;

	int selected_slot;
	int visible_slots;
	int scroll_offset;
	bool has_scroll_bar;

public:
	GameStateLoad();
	~GameStateLoad();

	void logic();
	void render();
};

#endif
