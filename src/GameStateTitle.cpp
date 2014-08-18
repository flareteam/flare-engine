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

#include "CommonIncludes.h"
#include "FileParser.h"
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
#include "UtilsParsing.h"

GameStateTitle::GameStateTitle()
	: GameState()
	, logo(NULL)
	, exit_game(false)
	, load_game(false)
{

	// set up buttons
	button_play = new WidgetButton("images/menus/buttons/button_default.png");
	button_exit = new WidgetButton("images/menus/buttons/button_default.png");
	button_cfg = new WidgetButton("images/menus/buttons/button_default.png");
	button_credits = new WidgetButton("images/menus/buttons/button_default.png");

	FileParser infile;
	// @CLASS GameStateTitle|Description of menus/gametitle.txt
	if (infile.open("menus/gametitle.txt")) {
		while (infile.next()) {
			// @ATTR logo|filename (string), x (integer), y (integer), align (alignment)|Filename and position of the main logo image.
			if (infile.key == "logo") {
				Image *graphics;
				graphics = render_device->loadImage(popFirstString(infile.val), "");
				if (graphics) {
					Rect r;
 				        logo = graphics->createSprite();

					r.x = popFirstInt(infile.val);
					r.y = popFirstInt(infile.val);
					r.w = logo->getGraphicsWidth();
					r.h = logo->getGraphicsHeight();
					alignToScreenEdge(popFirstString(infile.val), &r);
					logo->setDestX(r.x);
					logo->setDestY(r.y);
					graphics->unref();
				}
			}
			// @ATTR play_pos|x (integer), y (integer), align (alignment)|Position of the "Play Game" button.
			else if (infile.key == "play_pos") {
				button_play->pos.x = popFirstInt(infile.val);
				button_play->pos.y = popFirstInt(infile.val);
				alignToScreenEdge(popFirstString(infile.val), &(button_play->pos));
			}
			// @ATTR config_pos|x (integer), y (integer), align (alignment)|Position of the "Configuration" button.
			else if (infile.key == "config_pos") {
				button_cfg->pos.x = popFirstInt(infile.val);
				button_cfg->pos.y = popFirstInt(infile.val);
				alignToScreenEdge(popFirstString(infile.val), &(button_cfg->pos));
			}
			// @ATTR credits_pos|x (integer), y (integer), align (alignment)|Position of the "Credits" button.
			else if (infile.key == "credits_pos") {
				button_credits->pos.x = popFirstInt(infile.val);
				button_credits->pos.y = popFirstInt(infile.val);
				alignToScreenEdge(popFirstString(infile.val), &(button_credits->pos));
			}
			// @ATTR exit_pos|x (integer), y (integer), align (alignment)|Position of the "Exit Game" button.
			else if (infile.key == "exit_pos") {
				button_exit->pos.x = popFirstInt(infile.val);
				button_exit->pos.y = popFirstInt(infile.val);
				alignToScreenEdge(popFirstString(infile.val), &(button_exit->pos));
			}
			else {
				infile.error("GameStateTitle: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	button_play->label = msg->get("Play Game");
	if (!ENABLE_PLAYGAME) {
		button_play->enabled = false;
		button_play->tooltip = msg->get("Enable a core mod to continue");
	}
	button_play->refresh();

	button_cfg->label = msg->get("Configuration");
	button_cfg->refresh();

	button_credits->label = msg->get("Credits");
	button_credits->refresh();

	button_exit->label = msg->get("Exit Game");
	button_exit->refresh();

	// set up labels
	label_version = new WidgetLabel();
	label_version->set(VIEW_W, 0, JUSTIFY_RIGHT, VALIGN_TOP, getVersionString(), font->getColor("menu_normal"));

	// Setup tab order
	tablist.add(button_play);
	tablist.add(button_cfg);
	tablist.add(button_credits);
	tablist.add(button_exit);
}

void GameStateTitle::logic() {
	button_play->enabled = ENABLE_PLAYGAME;

	snd->logic(FPoint(0,0));

	if(inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
		inpt->lock[CANCEL] = true;
		exitRequested = true;
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

		if (!credits->load("cutscenes/credits.txt")) {
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
}

GameStateTitle::~GameStateTitle() {
	if (logo) delete logo;
	delete button_play;
	delete button_cfg;
	delete button_credits;
	delete button_exit;
	delete label_version;
}
