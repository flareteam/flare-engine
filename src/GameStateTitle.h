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

#ifndef GAMESTATETITLE_H
#define GAMESTATETITLE_H

#include "GameState.h"
#include "Widget.h"

class WidgetButton;
class WidgetLabel;

class GameStateTitle : public GameState {
private:
	void refreshWidgets();

	Sprite *logo;
	WidgetButton *button_play;
	WidgetButton *button_exit;
	WidgetButton *button_cfg;
	WidgetButton *button_credits;
	WidgetLabel *label_version;

	TabList tablist;

	Point pos_logo;
	int align_logo;

public:
	GameStateTitle();
	~GameStateTitle();
	void logic();
	void render();

	// switch
	bool exit_game;
	bool load_game;

};

#endif

