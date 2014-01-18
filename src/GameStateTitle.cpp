/*
Copyright © 2011-2012 Clint Bellanger

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

#include "CommonIncludes.h"
#include "GameStateConfig.h"
#include "GameStateCutscene.h"
#include "GameStateLoad.h"
#include "GameStateTitle.h"
#include "Settings.h"
#include "SharedResources.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetScrollBox.h"
#include "UtilsMath.h"

GameStateTitle::GameStateTitle() : GameState() {

	exit_game = false;
	load_game = false;

	logo.setGraphics(render_device->loadGraphicSurface("images/menus/logo.png"));
	// display logo centered
	if (!logo.graphicsIsNull()) {
		logo.setDestX(VIEW_W_HALF - (logo.getGraphicsWidth()/2));
		logo.setDestY(VIEW_H_HALF - (logo.getGraphicsHeight()/2));
	}

	// set up buttons
	button_play = new WidgetButton("images/menus/buttons/button_default.png");
	button_exit = new WidgetButton("images/menus/buttons/button_default.png");
	button_cfg = new WidgetButton("images/menus/buttons/button_default.png");
	button_credits = new WidgetButton("images/menus/buttons/button_default.png");

	button_play->label = msg->get("Play Game");
	button_play->pos.x = VIEW_W_HALF - button_play->pos.w/2;
	button_play->pos.y = VIEW_H - (button_exit->pos.h*4);
	if (!ENABLE_PLAYGAME) {
		button_play->enabled = false;
		button_play->tooltip = msg->get("Enable a core mod to continue");
	}
	button_play->refresh();

	button_cfg->label = msg->get("Configuration");
	button_cfg->pos.x = VIEW_W_HALF - button_cfg->pos.w/2;
	button_cfg->pos.y = VIEW_H - (button_exit->pos.h*3);
	button_cfg->refresh();

	button_credits->label = msg->get("Credits");
	button_credits->pos.x = VIEW_W_HALF - button_credits->pos.w/2;
	button_credits->pos.y = VIEW_H - (button_exit->pos.h*2);
	button_credits->refresh();

	button_exit->label = msg->get("Exit Game");
	button_exit->pos.x = VIEW_W_HALF - button_exit->pos.w/2;
	button_exit->pos.y = VIEW_H - button_exit->pos.h;
	button_exit->refresh();

	// set up labels
	label_version = new WidgetLabel();
	label_version->set(VIEW_W, 0, JUSTIFY_RIGHT, VALIGN_TOP, RELEASE_VERSION, font->getColor("menu_normal"));

	// Setup tab order
	tablist.add(button_play);
	tablist.add(button_cfg);
	tablist.add(button_credits);
	tablist.add(button_exit);

	// Warning text box
	warning_box = NULL;
	if (GAME_FOLDER == "default") {
		std::string warning_text = msg->get("Warning: A game wasn't specified, falling back to the 'default' game. Did you forget the --game flag? (e.g. --game=flare-game). See --help for more details.");
		Point warning_size = font->calc_size(warning_text, VIEW_W/2);

		int warning_box_h = warning_size.y;
		clampCeil(warning_box_h, VIEW_H/2);
		warning_box = new WidgetScrollBox(VIEW_W/2, warning_box_h);
		warning_box->resize(warning_size.y);

		font->setFont("font_normal");
		font->renderShadowed(warning_text, 0, 0, JUSTIFY_LEFT, warning_box->contents.getGraphics(), VIEW_W/2, FONT_WHITE);
	}
}

void GameStateTitle::logic() {
	button_play->enabled = ENABLE_PLAYGAME;

	snd->logic(FPoint(0,0));

	if(inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
		inpt->lock[CANCEL] = true;
		exitRequested = true;
	}

	if (warning_box) {
		warning_box->logic();
	}

	tablist.logic();

	if (button_play->checkClick()) {
		delete requestedGameState;
		requestedGameState = new GameStateLoad();
	}
	else if (button_cfg->checkClick()) {
		delete requestedGameState;
		requestedGameState = new GameStateConfig();
	}
	else if (button_credits->checkClick()) {
		GameStateTitle *title = new GameStateTitle();
		GameStateCutscene *credits = new GameStateCutscene(title);

		if (!credits->load("credits.txt")) {
			delete credits;
			delete title;
		}
		else {
			delete requestedGameState;
			requestedGameState = credits;
		}
	}
	else if (button_exit->checkClick()) {
		exitRequested = true;
	}
}

void GameStateTitle::render() {
	// display logo
	render_device->render(logo);

	// display buttons
	button_play->render();
	button_cfg->render();
	button_credits->render();
	button_exit->render();

	// version number
	label_version->render();

	// warning text
	if (warning_box) {
		warning_box->render();
	}
}

GameStateTitle::~GameStateTitle() {
	logo.clearGraphics();
	delete button_play;
	delete button_cfg;
	delete button_credits;
	delete button_exit;
	delete label_version;
	delete warning_box;
}
