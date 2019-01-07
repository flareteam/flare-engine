/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2014-2016 Justin Jacobs

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
 * GameStateConfig
 *
 * Handle game Settings Menu
 */

#include "CombatText.h"
#include "DeviceList.h"
#include "EngineSettings.h"
#include "FontEngine.h"
#include "GameStateConfig.h"
#include "GameStateTitle.h"
#include "InputState.h"
#include "MenuConfig.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Stats.h"
#include "TooltipManager.h"

GameStateConfig::GameStateConfig ()
	: GameState()
	, menu_config(new MenuConfig(MenuConfig::IS_GAME_STATE))
{

	// don't save settings if we close the game while in this menu
	save_settings_on_exit = false;
}

GameStateConfig::~GameStateConfig() {
	delete menu_config;
}

void GameStateConfig::logic() {
	menu_config->logic();

	if (menu_config->force_refresh_background) {
		force_refresh_background = true;
		menu_config->force_refresh_background = false;
	}
	if (menu_config->reload_music) {
		reload_music = true;
		menu_config->reload_music = false;
	}

	if (menu_config->clicked_accept) {
		menu_config->clicked_accept = false;
		logicAccept();
	}
	else if (menu_config->clicked_cancel) {
		menu_config->clicked_cancel = false;
		logicCancel();
	}
}

void GameStateConfig::logicAccept() {
	// new_render_device = renderer_lstb->getValue();
	std::string new_render_device = menu_config->getRenderDevice();

	if (menu_config->setMods()) {
		snd->unloadMusic();
		reload_music = true;
		reload_backgrounds = true;
		delete mods;
		mods = new ModManager(NULL);
		settings->prev_save_slot = -1;
	}
	delete msg;
	msg = new MessageEngine();
	inpt->saveKeyBindings();
	inpt->setKeybindNames();
	eset->load();
	Stats::init();
	refreshFont();
	if ((settings->enable_joystick) && (inpt->getNumJoysticks() > 0)) {
		inpt->initJoystick();
	}
	menu_config->cleanup();

	showLoading();
	// need to delete the "Loading..." message here, as we're recreating our render context
	if (loading_tip) {
		delete loading_tip;
		loading_tip = NULL;
	}

	delete tooltipm;

	// we can't replace the render device in-place, so soft-reset the game
	if (new_render_device != settings->render_device_name) {
		settings->render_device_name = new_render_device;
		inpt->done = true;
		settings->soft_reset = true;
	}

	render_device->createContext();
	tooltipm = new TooltipManager();
	settings->saveSettings();
	setRequestedGameState(new GameStateTitle());
}

void GameStateConfig::logicCancel() {
	inpt->lock[Input::CANCEL] = true;
	settings->loadSettings();
	inpt->loadKeyBindings();
	delete msg;
	msg = new MessageEngine();
	inpt->setKeybindNames();
	eset->load();
	Stats::init();
	refreshFont();
	menu_config->update();
	menu_config->cleanup();
	render_device->windowResize();
	render_device->updateTitleBar();
	showLoading();
	setRequestedGameState(new GameStateTitle());
}

void GameStateConfig::render() {
	if (requestedGameState != NULL) {
		// we're in the process of switching game states, so skip rendering
		return;
	}

	menu_config->render();
}

void GameStateConfig::refreshFont() {
	delete font;
	font = getFontEngine();
	delete comb;
	comb = new CombatText();
}

