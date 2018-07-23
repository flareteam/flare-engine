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
 * GameStateNew
 *
 * Handle player choices when starting a new game
 * (e.g. character appearance)
 */

#ifndef GAMESTATENEW_H
#define GAMESTATENEW_H

#include "CommonIncludes.h"
#include "GameState.h"
#include "WidgetLabel.h"

class WidgetButton;
class WidgetCheckBox;
class WidgetInput;
class WidgetLabel;
class WidgetListBox;

class HeroOption {
public:
	HeroOption() {}
	~HeroOption() {}

	std::string base;
	std::string head;
	std::string portrait;
	std::string name;
};

class GameStateNew : public GameState {
private:
	enum {
		OPTION_CURRENT = 0,
		OPTION_PREV = 1,
		OPTION_NEXT = 2,
		OPTION_RANDOM = 3
	};

	void loadGraphics();
	void loadPortrait(const std::string& portrait_filename);
	void loadOptions(const std::string& option_filename);
	std::string getClassTooltip(int index);
	void setName(const std::string& default_name);
	void setHeroOption(int dir);
	void refreshWidgets();

	std::vector<HeroOption> hero_options;
	int current_option;

	Sprite *portrait_image;
	Sprite *portrait_border;
	WidgetButton *button_exit;
	WidgetButton *button_create;
	WidgetButton *button_next;
	WidgetButton *button_prev;
	WidgetButton *button_randomize;
	WidgetLabel *label_portrait;
	WidgetLabel *label_name;
	WidgetInput *input_name;
	WidgetCheckBox *button_permadeath;
	WidgetLabel *label_permadeath;
	WidgetLabel *label_classlist;
	WidgetListBox *class_list;

	TabList tablist;

	Rect portrait_pos;
	bool show_classlist;
	bool modified_name;
	bool delete_items;
	bool random_option;
	bool random_class;

	std::vector<int> all_options;

public:
	GameStateNew();
	~GameStateNew();
	void logic();
	void render();
	int game_slot;

};

#endif
