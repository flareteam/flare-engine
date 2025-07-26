/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateConfig.h"
#include "GameStateCutscene.h"
#include "GameStateLoad.h"
#include "GameStateNew.h"
#include "GameStateTitle.h"
#include "InputState.h"
#include "MenuConfig.h"
#include "MenuConfirm.h"
#include "MessageEngine.h"
#include "Platform.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "WidgetButton.h"
#include "WidgetHorizontalList.h"
#include "WidgetLabel.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "Version.h"

GameStateTitle::GameStateTitle()
	: GameState()
	, logo(NULL)
	, button_play(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, button_exit(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, button_cfg(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, button_credits(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, label_version(new WidgetLabel())
	, menu_movement_type(NULL)
	, align_logo(Utils::ALIGN_CENTER)
	, exit_game(false)
	, load_game(false)
{
	FileParser infile;
	// @CLASS GameStateTitle|Description of menus/gametitle.txt
	if (infile.open("menus/gametitle.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR logo|filename, int, int, alignment : Image file, X, Y, Alignment|Filename and position of the main logo image.
			if (infile.key == "logo") {
				Image *graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NONE);
				if (graphics) {
 				    logo = graphics->createSprite();
					graphics->unref();

					pos_logo.x = Parse::popFirstInt(infile.val);
					pos_logo.y = Parse::popFirstInt(infile.val);
					align_logo = Parse::toAlignment(Parse::popFirstString(infile.val));
				}
			}
			// @ATTR play_pos|int, int, alignment : X, Y, Alignment|Position of the "Play Game" button.
			else if (infile.key == "play_pos") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_play->setBasePos(x, y, a);
			}
			// @ATTR config_pos|int, int, alignment : X, Y, Alignment|Position of the "Configuration" button.
			else if (infile.key == "config_pos") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_cfg->setBasePos(x, y, a);
			}
			// @ATTR credits_pos|int, int, alignment : X, Y, Alignment|Position of the "Credits" button.
			else if (infile.key == "credits_pos") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_credits->setBasePos(x, y, a);
			}
			// @ATTR exit_pos|int, int, alignment : X, Y, Alignment|Position of the "Exit Game" button.
			else if (infile.key == "exit_pos") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				int a = Parse::toAlignment(Parse::popFirstString(infile.val));
				button_exit->setBasePos(x, y, a);
			}
			else {
				infile.error("GameStateTitle: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	button_play->setLabel(msg->get("Play Game"));
	button_play->refresh();

	button_cfg->setLabel(msg->get("Configuration"));
	button_cfg->refresh();

	button_credits->setLabel(msg->get("Credits"));
	button_credits->refresh();

	button_exit->setLabel(msg->get("Exit Game"));
	button_exit->refresh();

	// set up labels
	label_version->setJustify(FontEngine::JUSTIFY_RIGHT);
	label_version->setText(VersionInfo::createVersionStringFull());
	label_version->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	// Setup tab order
	tablist.add(button_play);
	tablist.add(button_cfg);
	tablist.add(button_credits);
	tablist.add(button_exit);

	// Core mod not selected dialogue
	prompt_select_mods = new MenuConfirm();
	prompt_select_mods->setTitle(msg->get("Enable a core mod to continue"));
	prompt_select_mods->action_list->append(msg->get("Mods"), "");
	prompt_select_mods->action_list->append(msg->get("Cancel"), "");

	refreshWidgets();
	force_refresh_background = true;

	if (eset->gameplay.enable_playgame && !settings->load_slot.empty()) {
		showLoading();
		setRequestedGameState(new GameStateLoad());
	}

	render_device->setBackgroundColor(Color(0,0,0,0));

	// NOTE The presence of the mouse move setting is used to determine if the
	// movement type dialog is displayed. Is this adequate?
	if (!settings->move_type_dimissed && platform.config_input[Platform::Input::MOUSE_MOVE]) {
		menu_movement_type = new MenuConfirm();
		menu_movement_type->setTitle(msg->get("Use mouse to move player?"));
		menu_movement_type->action_list->append(msg->get("No"), "");
		menu_movement_type->action_list->append(msg->get("Yes"), "");
		menu_movement_type->show();
		inpt->mode = InputState::MODE_JOYSTICK;
	}

}

void GameStateTitle::logic() {
	if (inpt->window_resized)
		refreshWidgets();

	snd->logic(FPoint(0,0));

	if (menu_movement_type && menu_movement_type->visible) {
		menu_movement_type->logic();

		if (menu_movement_type->clicked_confirm) {
			if (menu_movement_type->action_list->getSelected() == PROMPT_SELECT_MOUSEMOVE_NO) {
				settings->mouse_move = false;

				menu_movement_type->visible = false;
				menu_movement_type->clicked_confirm = false;

				settings->move_type_dimissed = true;
				settings->saveSettings();
			} else if (menu_movement_type->action_list->getSelected() == PROMPT_SELECT_MOUSEMOVE_YES) {
				settings->mouse_move = true;

				menu_movement_type->visible = false;
				menu_movement_type->clicked_confirm = false;

				settings->move_type_dimissed = true;
				settings->saveSettings();
			}
		}
		else if (menu_movement_type->clicked_cancel) {
			settings->mouse_move = false;

			menu_movement_type->clicked_cancel = false;

			settings->move_type_dimissed = true;
			settings->saveSettings();
		}
	}
	else if (prompt_select_mods && prompt_select_mods->visible) {
		prompt_select_mods->logic();
		if (prompt_select_mods->clicked_confirm) {
			if (prompt_select_mods->action_list->getSelected() == PROMPT_SELECT_MODS_OK) {
				showLoading();
				GameStateConfig* config = new GameStateConfig();
				config->setActiveTab(MenuConfig::MODS_TAB);
				setRequestedGameState(config);

				prompt_select_mods->visible = false;
				prompt_select_mods->clicked_confirm = false;
			} else if (prompt_select_mods->action_list->getSelected() == PROMPT_SELECT_MODS_CANCEL) {
				prompt_select_mods->visible = false;
				prompt_select_mods->clicked_confirm = false;
			}

		}
	}
	else {
		if(inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) {
			inpt->lock[Input::CANCEL] = true;
			exitRequested = true;
		}

		tablist.logic();

		bool play_clicked = button_play->checkClick();

		if (!inpt->usingMouse() && tablist.getCurrent() == -1) {
			tablist.getNext(!TabList::GET_INNER, TabList::WIDGET_SELECT_AUTO);
		}

		if (play_clicked && !eset->gameplay.enable_playgame) {
			prompt_select_mods->show();
		}
		else if (play_clicked) {
			showLoading();

			// if we don't have any saves, go directly to GameStateNew
			std::vector<std::string> save_dirs;
			Filesystem::getDirList(settings->path_user + "saves/" + eset->misc.save_prefix, save_dirs);
			if (save_dirs.size() == 0) {
				GameStateNew* newgame = new GameStateNew();
				newgame->game_slot = 1;
				setRequestedGameState(newgame);
			}
			else {
				setRequestedGameState(new GameStateLoad());
			}
		}
		else if (button_cfg->checkClick()) {
			showLoading();
			setRequestedGameState(new GameStateConfig());
		}
		else if (button_credits->checkClick()) {
			showLoading();
			GameStateTitle *title = new GameStateTitle();
			GameStateCutscene *credits = new GameStateCutscene(title);

			if (!credits->load("cutscenes/credits.txt")) {
				delete credits;
				delete title;
			}
			else {
				setRequestedGameState(credits);
			}
		}
		else if (platform.has_exit_button && button_exit->checkClick()) {
			exitRequested = true;
		}
	}

}

void GameStateTitle::refreshWidgets() {
	if (logo) {
		Rect r;
		r.x = pos_logo.x;
		r.y = pos_logo.y;
		r.w = logo->getGraphicsWidth();
		r.h = logo->getGraphicsHeight();
		Utils::alignToScreenEdge(align_logo, &r);
		logo->setDestFromRect(r);
	}

	button_play->setPos(0, 0);
	button_cfg->setPos(0, 0);
	button_credits->setPos(0, 0);
	button_exit->setPos(0, 0);

	label_version->setPos(settings->view_w, 0);

	if (menu_movement_type)
		menu_movement_type->align();
	if (prompt_select_mods)
		prompt_select_mods->align();
}

void GameStateTitle::render() {
	if (!menu_movement_type || !menu_movement_type->visible) {
		// display logo
		render_device->render(logo);

		// display buttons
		button_play->render();
		button_cfg->render();
		button_credits->render();

		if (prompt_select_mods && prompt_select_mods->visible)
			prompt_select_mods->render();

		if (platform.has_exit_button)
			button_exit->render();
		}
	else {
		menu_movement_type->render();
	}

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
	delete menu_movement_type;
	if (prompt_select_mods) delete prompt_select_mods;
}
