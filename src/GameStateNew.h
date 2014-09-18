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
 * GameStateNew
 *
 * Handle player choices when starting a new game
 * (e.g. character appearance)
 */


#pragma once
#ifndef GAMESTATENEW_H
#define GAMESTATENEW_H

#include "CommonIncludes.h"
#include "GameState.h"
#include "TooltipData.h"
#include "WidgetLabel.h"

class WidgetButton;
class WidgetCheckBox;
class WidgetInput;
class WidgetLabel;
class WidgetListBox;
class WidgetTooltip;

class GameStateNew : public GameState {
private:

	void loadGraphics();
	void loadPortrait(const std::string& portrait_filename);
	void loadOptions(const std::string& option_filename);
	std::string getClassTooltip(int index);
	void setName(const std::string& default_name);

	std::vector<std::string> base;
	std::vector<std::string> head;
	std::vector<std::string> portrait;
	std::vector<std::string> name;
	int current_option;

	Sprite *portrait_image;
	Sprite *portrait_border;
	WidgetButton *button_exit;
	WidgetButton *button_create;
	WidgetButton *button_next;
	WidgetButton *button_prev;
	WidgetLabel *label_portrait;
	WidgetLabel *label_name;
	WidgetInput *input_name;
	WidgetCheckBox *button_permadeath;
	WidgetLabel *label_permadeath;
	WidgetLabel *label_classlist;
	WidgetListBox *class_list;
	WidgetTooltip *tip;

	TabList tablist;

	Point name_pos;
	LabelInfo portrait_label;
	LabelInfo name_label;
	LabelInfo permadeath_label;
	LabelInfo classlist_label;
	Rect portrait_pos;
	bool show_classlist;
	TooltipData tip_buf;
	bool modified_name;
	bool delete_items;

	Color color_normal;

public:
	GameStateNew();
	~GameStateNew();
	void logic();
	void render();
	int game_slot;

};

#endif
