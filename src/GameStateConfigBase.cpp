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
 * GameStateConfigBase
 *
 * Handle game Settings Menu
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameStateConfigBase.h"
#include "GameStateTitle.h"
#include "MenuConfirm.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Stats.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetCheckBox.h"
#include "WidgetListBox.h"
#include "WidgetSlider.h"
#include "WidgetTabControl.h"
#include "WidgetTooltip.h"

#include <limits.h>
#include <iomanip>

GameStateConfigBase::GameStateConfigBase (bool do_init)
	: GameState()
	, child_widget()
	, ok_button(new WidgetButton())
	, defaults_button(new WidgetButton())
	, cancel_button(new WidgetButton())
	, background(NULL)
	, combat_text_cb(new WidgetCheckBox())
	, combat_text_lb(new WidgetLabel())
	, show_fps_cb(new WidgetCheckBox())
	, show_fps_lb(new WidgetLabel())
	, hardware_cursor_cb(new WidgetCheckBox())
	, hardware_cursor_lb(new WidgetLabel())
	, colorblind_cb(new WidgetCheckBox())
	, colorblind_lb(new WidgetLabel())
	, dev_mode_cb(new WidgetCheckBox())
	, dev_mode_lb(new WidgetLabel())
	, loot_tooltips_cb(new WidgetCheckBox())
	, loot_tooltips_lb(new WidgetLabel())
	, statbar_labels_cb(new WidgetCheckBox())
	, statbar_labels_lb(new WidgetLabel())
	, auto_equip_cb(new WidgetCheckBox())
	, auto_equip_lb(new WidgetLabel())
	, music_volume_sl(new WidgetSlider())
	, music_volume_lb(new WidgetLabel())
	, sound_volume_sl(new WidgetSlider())
	, sound_volume_lb(new WidgetLabel())
	, activemods_lstb(new WidgetListBox(10))
	, activemods_lb(new WidgetLabel())
	, inactivemods_lstb(new WidgetListBox(10))
	, inactivemods_lb(new WidgetLabel())
	, language_lstb(new WidgetListBox(10))
	, language_lb(new WidgetLabel())
	, activemods_shiftup_btn(new WidgetButton("images/menus/buttons/up.png"))
	, activemods_shiftdown_btn(new WidgetButton("images/menus/buttons/down.png"))
	, activemods_deactivate_btn(new WidgetButton())
	, inactivemods_activate_btn(new WidgetButton())
	, defaults_confirm(new MenuConfirm(msg->get("Defaults"), msg->get("Reset ALL settings?")))
	, tip(new WidgetTooltip())
	, tip_buf()
	, active_tab(0)
{

	// don't save settings if we close the game while in this menu
	save_settings_on_exit = false;

	Image *graphics;
	graphics = render_device->loadImage("images/menus/config.png");
	if (graphics) {
		background = graphics->createSprite();
		graphics->unref();
	}

	tab_control = new WidgetTabControl();

	ok_button->label = msg->get("OK");
	ok_button->setBasePos(0, -(cancel_button->pos.h*2), ALIGN_BOTTOM);
	ok_button->refresh();

	defaults_button->label = msg->get("Defaults");
	defaults_button->setBasePos(0, -(cancel_button->pos.h), ALIGN_BOTTOM);
	defaults_button->refresh();

	cancel_button->label = msg->get("Cancel");
	cancel_button->setBasePos(0, 0, ALIGN_BOTTOM);
	cancel_button->refresh();

	language_lstb->can_deselect = false;

	// Finish Mods ListBoxes setup
	activemods_lstb->multi_select = true;
	for (unsigned int i = 0; i < mods->mod_list.size() ; i++) {
		if (mods->mod_list[i].name != FALLBACK_MOD)
			activemods_lstb->append(mods->mod_list[i].name,createModTooltip(&mods->mod_list[i]));
	}

	inactivemods_lstb->multi_select = true;
	for (unsigned int i = 0; i<mods->mod_dirs.size(); i++) {
		bool skip_mod = false;
		for (unsigned int j = 0; j<mods->mod_list.size(); j++) {
			if (mods->mod_dirs[i] == mods->mod_list[j].name) {
				skip_mod = true;
				break;
			}
		}
		if (!skip_mod && mods->mod_dirs[i] != FALLBACK_MOD) {
			Mod temp_mod = mods->loadMod(mods->mod_dirs[i]);
			inactivemods_lstb->append(mods->mod_dirs[i],createModTooltip(&temp_mod));
		}
	}
	inactivemods_lstb->sort();

	if (do_init) {
		init();
	}
	else {
		// these will be initialized properly by a derevitive class (i.e. GameStateConfigDesktop)
		AUDIO_TAB = 0;
		INTERFACE_TAB = 0;
		MODS_TAB = 0;
	}
}

GameStateConfigBase::~GameStateConfigBase() {
	cleanup();
}

void GameStateConfigBase::init() {
	AUDIO_TAB = 0;
	INTERFACE_TAB = 1;
	MODS_TAB = 2;

	tab_control->setTabTitle(AUDIO_TAB, msg->get("Audio"));
	tab_control->setTabTitle(INTERFACE_TAB, msg->get("Interface"));
	tab_control->setTabTitle(MODS_TAB, msg->get("Mods"));
	tab_control->updateHeader();

	readConfig();

	addChildWidgets();
	setupTabList();

	refreshWidgets();

	update();
}

void GameStateConfigBase::readConfig() {
	//Load the menu configuration from file

	FileParser infile;
	if (infile.open("menus/config.txt")) {
		while (infile.next()) {
			int x1 = popFirstInt(infile.val);
			int y1 = popFirstInt(infile.val);
			int x2 = popFirstInt(infile.val);
			int y2 = popFirstInt(infile.val);

			if (parseKey(infile, x1, y1, x2, y2))
				continue;
			else if (parseStub(infile))
				continue;
			else {
				infile.error("GameStateConfigBase: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
}

bool GameStateConfigBase::parseKey(FileParser &infile, int &x1, int &y1, int &x2, int &y2) {
	// @CLASS GameStateConfigBase|Description of menus/config.txt

	if (infile.key == "listbox_scrollbar_offset") {
		// @ATTR listbox_scrollbar_offset|int|Horizontal offset from the right of listboxes (mods, languages, etc) to place the scrollbar.
		activemods_lstb->scrollbar_offset = x1;
		inactivemods_lstb->scrollbar_offset = x1;
		language_lstb->scrollbar_offset = x1;
	}
	else if (infile.key == "music_volume") {
		// @ATTR music_volume|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Music Volume" slider relative to the frame.
		placeLabeledWidget(music_volume_lb, music_volume_sl, x1, y1, x2, y2, msg->get("Music Volume"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "sound_volume") {
		// @ATTR sound_volume|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Sound Volume" slider relative to the frame.
		placeLabeledWidget(sound_volume_lb, sound_volume_sl, x1, y1, x2, y2, msg->get("Sound Volume"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "language") {
		// @ATTR language|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Language" list box relative to the frame.
		placeLabeledWidget(language_lb, language_lstb, x1, y1, x2, y2, msg->get("Language"));
	}
	else if (infile.key == "combat_text") {
		// @ATTR combat_text|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Show combat text" checkbox relative to the frame.
		placeLabeledWidget(combat_text_lb, combat_text_cb, x1, y1, x2, y2, msg->get("Show combat text"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "show_fps") {
		// @ATTR show_fps|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Show FPS" checkbox relative to the frame.
		placeLabeledWidget(show_fps_lb, show_fps_cb, x1, y1, x2, y2, msg->get("Show FPS"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "colorblind") {
		// @ATTR colorblind|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Colorblind Mode" checkbox relative to the frame.
		placeLabeledWidget(colorblind_lb, colorblind_cb, x1, y1, x2, y2, msg->get("Colorblind Mode"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "hardware_cursor") {
		// @ATTR hardware_cursor|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Hardware mouse cursor" checkbox relative to the frame.
		placeLabeledWidget(hardware_cursor_lb, hardware_cursor_cb, x1, y1, x2, y2, msg->get("Hardware mouse cursor"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "dev_mode") {
		// @ATTR dev_mode|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Developer Mode" checkbox relative to the frame.
		placeLabeledWidget(dev_mode_lb, dev_mode_cb, x1, y1, x2, y2, msg->get("Developer Mode"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "loot_tooltips") {
		// @ATTR loot_tooltips|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Always show loot labels" checkbox relative to the frame.
		placeLabeledWidget(loot_tooltips_lb, loot_tooltips_cb, x1, y1, x2, y2, msg->get("Always show loot labels"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "statbar_labels") {
		// @ATTR statbar_labels|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Always show stat bar labels" checkbox relative to the frame.
		placeLabeledWidget(statbar_labels_lb, statbar_labels_cb, x1, y1, x2, y2, msg->get("Always show stat bar labels"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "auto_equip") {
		// @ATTR auto_equip|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Automatically equip items" checkbox relative to the frame.
		placeLabeledWidget(auto_equip_lb, auto_equip_cb, x1, y1, x2, y2, msg->get("Automatically equip items"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "activemods") {
		// @ATTR activemods|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Active Mods" list box relative to the frame.
		placeLabeledWidget(activemods_lb, activemods_lstb, x1, y1, x2, y2, msg->get("Active Mods"));
	}
	else if (infile.key == "inactivemods") {
		// @ATTR inactivemods|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Available Mods" list box relative to the frame.
		placeLabeledWidget(inactivemods_lb, inactivemods_lstb, x1, y1, x2, y2, msg->get("Available Mods"));
	}
	else if (infile.key == "activemods_shiftup") {
		// @ATTR activemods_shiftup|point|Position of the button to shift mods up in "Active Mods" relative to the frame.
		activemods_shiftup_btn->setBasePos(x1, y1);
		activemods_shiftup_btn->refresh();
	}
	else if (infile.key == "activemods_shiftdown") {
		// @ATTR activemods_shiftdown|point|Position of the button to shift mods down in "Active Mods" relative to the frame.
		activemods_shiftdown_btn->setBasePos(x1, y1);
		activemods_shiftdown_btn->refresh();
	}
	else if (infile.key == "activemods_deactivate") {
		// @ATTR activemods_deactivate|point|Position of the "Disable" button relative to the frame.
		activemods_deactivate_btn->label = msg->get("<< Disable");
		activemods_deactivate_btn->setBasePos(x1, y1);
		activemods_deactivate_btn->refresh();
	}
	else if (infile.key == "inactivemods_activate") {
		// @ATTR inactivemods_activate|point|Position of the "Enable" button relative to the frame.
		inactivemods_activate_btn->label = msg->get("Enable >>");
		inactivemods_activate_btn->setBasePos(x1, y1);
		inactivemods_activate_btn->refresh();
	}
	else {
		return false;
	}

	return true;
}

bool GameStateConfigBase::parseStub(FileParser &infile) {
	// not used for base configuration
	// checking them here prevents getting an "invalid key" warning
	if (infile.key == "fullscreen");
	else if (infile.key == "mouse_move");
	else if (infile.key == "hwsurface");
	else if (infile.key == "vsync");
	else if (infile.key == "texture_filter");
	else if (infile.key == "enable_joystick");
	else if (infile.key == "change_gamma");
	else if (infile.key == "mouse_aim");
	else if (infile.key == "no_mouse");
	else if (infile.key == "gamma");
	else if (infile.key == "joystick_deadzone");
	else if (infile.key == "resolution");
	else if (infile.key == "joystick_device");
	else if (infile.key == "hws_note");
	else if (infile.key == "dbuf_note");
	else if (infile.key == "test_note");
	else if (infile.key == "handheld_note");
	else if (infile.key == "secondary_offset");
	else if (infile.key == "keybinds_bg_color");
	else if (infile.key == "scrollpane");
	else if (infile.key == "scrollpane_contents");
	else if (infile.key == "cancel");
	else if (infile.key == "accept");
	else if (infile.key == "up");
	else if (infile.key == "down");
	else if (infile.key == "left");
	else if (infile.key == "right");
	else if (infile.key == "bar1");
	else if (infile.key == "bar2");
	else if (infile.key == "bar3");
	else if (infile.key == "bar4");
	else if (infile.key == "bar5");
	else if (infile.key == "bar6");
	else if (infile.key == "bar7");
	else if (infile.key == "bar8");
	else if (infile.key == "bar9");
	else if (infile.key == "bar0");
	else if (infile.key == "main1");
	else if (infile.key == "main2");
	else if (infile.key == "character");
	else if (infile.key == "inventory");
	else if (infile.key == "powers");
	else if (infile.key == "log");
	else if (infile.key == "ctrl");
	else if (infile.key == "shift");
	else if (infile.key == "alt");
	else if (infile.key == "delete");
	else if (infile.key == "actionbar");
	else if (infile.key == "actionbar_back");
	else if (infile.key == "actionbar_forward");
	else if (infile.key == "actionbar_use");
	else if (infile.key == "developer_menu");
	else return false;

	return true;
}

void GameStateConfigBase::addChildWidgets() {
	addChildWidget(music_volume_sl, AUDIO_TAB);
	addChildWidget(music_volume_lb, AUDIO_TAB);
	addChildWidget(sound_volume_sl, AUDIO_TAB);
	addChildWidget(sound_volume_lb, AUDIO_TAB);

	addChildWidget(combat_text_cb, INTERFACE_TAB);
	addChildWidget(combat_text_lb, INTERFACE_TAB);
	addChildWidget(show_fps_cb, INTERFACE_TAB);
	addChildWidget(show_fps_lb, INTERFACE_TAB);
	addChildWidget(colorblind_cb, INTERFACE_TAB);
	addChildWidget(colorblind_lb, INTERFACE_TAB);
	addChildWidget(hardware_cursor_cb, INTERFACE_TAB);
	addChildWidget(hardware_cursor_lb, INTERFACE_TAB);
	addChildWidget(dev_mode_cb, INTERFACE_TAB);
	addChildWidget(dev_mode_lb, INTERFACE_TAB);
	addChildWidget(loot_tooltips_cb, INTERFACE_TAB);
	addChildWidget(loot_tooltips_lb, INTERFACE_TAB);
	addChildWidget(statbar_labels_cb, INTERFACE_TAB);
	addChildWidget(statbar_labels_lb, INTERFACE_TAB);
	addChildWidget(auto_equip_cb, INTERFACE_TAB);
	addChildWidget(auto_equip_lb, INTERFACE_TAB);
	addChildWidget(language_lstb, INTERFACE_TAB);
	addChildWidget(language_lb, INTERFACE_TAB);

	addChildWidget(activemods_lstb, MODS_TAB);
	addChildWidget(activemods_lb, MODS_TAB);
	addChildWidget(inactivemods_lstb, MODS_TAB);
	addChildWidget(inactivemods_lb, MODS_TAB);
	addChildWidget(activemods_shiftup_btn, MODS_TAB);
	addChildWidget(activemods_shiftdown_btn, MODS_TAB);
	addChildWidget(activemods_deactivate_btn, MODS_TAB);
	addChildWidget(inactivemods_activate_btn, MODS_TAB);
}

void GameStateConfigBase::setupTabList() {
	tablist.add(tab_control);
	tablist.setPrevTabList(&tablist_main);

	tablist_main.add(ok_button);
	tablist_main.add(defaults_button);
	tablist_main.add(cancel_button);
	tablist_main.setPrevTabList(&tablist);
	tablist_main.setNextTabList(&tablist);
	tablist_main.lock();

	tablist_audio.add(music_volume_sl);
	tablist_audio.add(sound_volume_sl);
	tablist_audio.setPrevTabList(&tablist);
	tablist_audio.setNextTabList(&tablist_main);
	tablist_audio.lock();

	tablist_interface.add(combat_text_cb);
	tablist_interface.add(show_fps_cb);
	tablist_interface.add(colorblind_cb);
	tablist_interface.add(hardware_cursor_cb);
	tablist_interface.add(dev_mode_cb);
	tablist_interface.add(loot_tooltips_cb);
	tablist_interface.add(auto_equip_cb);
	tablist_interface.add(language_lstb);
	tablist_interface.setPrevTabList(&tablist);
	tablist_interface.setNextTabList(&tablist_main);
	tablist_interface.lock();

	tablist_mods.add(inactivemods_lstb);
	tablist_mods.add(activemods_lstb);
	tablist_mods.add(inactivemods_activate_btn);
	tablist_mods.add(activemods_deactivate_btn);
	tablist_mods.add(activemods_shiftup_btn);
	tablist_mods.add(activemods_shiftdown_btn);
	tablist_mods.setPrevTabList(&tablist);
	tablist_mods.setNextTabList(&tablist_main);
	tablist_mods.lock();
}

void GameStateConfigBase::update() {
	updateAudio();
	updateInterface();
	updateMods();
}

void GameStateConfigBase::updateAudio() {
	if (AUDIO) {
		music_volume_sl->set(0,128,MUSIC_VOLUME);
		snd->setVolumeMusic(MUSIC_VOLUME);
		sound_volume_sl->set(0,128,SOUND_VOLUME);
		snd->setVolumeSFX(SOUND_VOLUME);
	}
	else {
		music_volume_sl->set(0,128,0);
		sound_volume_sl->set(0,128,0);
	}
}

void GameStateConfigBase::updateInterface() {
	if (COMBAT_TEXT) combat_text_cb->Check();
	else combat_text_cb->unCheck();

	if (SHOW_FPS) show_fps_cb->Check();
	else show_fps_cb->unCheck();

	if (COLORBLIND) colorblind_cb->Check();
	else colorblind_cb->unCheck();

	if (HARDWARE_CURSOR) hardware_cursor_cb->Check();
	else hardware_cursor_cb->unCheck();

	if (DEV_MODE) dev_mode_cb->Check();
	else dev_mode_cb->unCheck();

	if (LOOT_TOOLTIPS) loot_tooltips_cb->Check();
	else loot_tooltips_cb->unCheck();

	if (STATBAR_LABELS) statbar_labels_cb->Check();
	else statbar_labels_cb->unCheck();

	if (AUTO_EQUIP) auto_equip_cb->Check();
	else auto_equip_cb->unCheck();

	refreshLanguages();
}

void GameStateConfigBase::updateMods() {
	activemods_lstb->refresh();
	inactivemods_lstb->refresh();
}

void GameStateConfigBase::logic() {
	if (inpt->window_resized)
		refreshWidgets();

	if (defaults_confirm->visible) {
		// reset defaults confirmation
		logicDefaults();
		return;
	}
	else {
		if (!logicMain())
			return;
	}

	// tab contents
	active_tab = tab_control->getActiveTab();

	if (active_tab == AUDIO_TAB) {
		tablist.setNextTabList(&tablist_audio);
		logicAudio();
	}
	else if (active_tab == INTERFACE_TAB) {
		tablist.setNextTabList(&tablist_interface);
		logicInterface();

		// by default, hardware mouse cursor can not be turned off
		// that is because this class is used as-is on non-desktop platforms
		hardware_cursor_cb->Check();
		HARDWARE_CURSOR = true;
	}
	else if (active_tab == MODS_TAB) {
		tablist.setNextTabList(&tablist_mods);
		logicMods();
	}
}

bool GameStateConfigBase::logicMain() {
	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (child_widget[i]->in_focus) {
			tab_control->setActiveTab(optiontab[i]);
			break;
		}
	}

	// tabs & the bottom 3 main buttons
	tab_control->logic();
	tablist.logic();
	tablist_main.logic();
	tablist_audio.logic();
	tablist_interface.logic();
	tablist_mods.logic();

	// Ok/Cancel Buttons
	if (ok_button->checkClick()) {
		logicAccept();

		// GameStateConfigBase deconstructed, proceed with caution
		return false;
	}
	else if (defaults_button->checkClick()) {
		defaults_confirm->visible = true;
		return true;
	}
	else if (cancel_button->checkClick() || (inpt->pressing[CANCEL] && !inpt->lock[CANCEL])) {
		logicCancel();

		// GameStateConfigBase deconstructed, proceed with caution
		return false;
	}

	return true;
}

void GameStateConfigBase::logicDefaults() {
	defaults_confirm->logic();
	if (defaults_confirm->confirmClicked) {
		FULLSCREEN = false;
		loadDefaults();
		loadMiscSettings();
		inpt->defaultQwertyKeyBindings();
		inpt->defaultJoystickBindings();
		update();
		defaults_confirm->visible = false;
		defaults_confirm->confirmClicked = false;
	}
}

void GameStateConfigBase::logicAccept() {
	delete msg;
	msg = new MessageEngine();
	inpt->saveKeyBindings();
	inpt->setKeybindNames();
	if (setMods()) {
		snd->unloadMusic();
		reload_music = true;
		reload_backgrounds = true;
		delete mods;
		mods = new ModManager(NULL);
		loadTilesetSettings();
	}
	loadMiscSettings();
	setStatNames();
	refreshFont();
	if ((ENABLE_JOYSTICK) && (inpt->getNumJoysticks() > 0)) {
		inpt->initJoystick();
	}
	cleanup();
	render_device->createContext();
	saveSettings();
	delete requestedGameState;
	requestedGameState = new GameStateTitle();
}

void GameStateConfigBase::logicCancel() {
	inpt->lock[CANCEL] = true;
	loadSettings();
	inpt->loadKeyBindings();
	delete msg;
	msg = new MessageEngine();
	loadMiscSettings();
	setStatNames();
	update();
	cleanup();
	render_device->updateTitleBar();
	delete requestedGameState;
	requestedGameState = new GameStateTitle();
}

void GameStateConfigBase::logicAudio() {
	if (AUDIO) {
		if (music_volume_sl->checkClick()) {
			if (MUSIC_VOLUME == 0)
				reload_music = true;
			MUSIC_VOLUME = static_cast<short>(music_volume_sl->getValue());
			snd->setVolumeMusic(MUSIC_VOLUME);
		}
		else if (sound_volume_sl->checkClick()) {
			SOUND_VOLUME = static_cast<short>(sound_volume_sl->getValue());
			snd->setVolumeSFX(SOUND_VOLUME);
		}
	}
}

void GameStateConfigBase::logicInterface() {
	if (combat_text_cb->checkClick()) {
		if (combat_text_cb->isChecked()) COMBAT_TEXT=true;
		else COMBAT_TEXT=false;
	}
	else if (language_lstb->checkClick()) {
		int lang_id = language_lstb->getSelected();
		if (lang_id != -1)
			LANGUAGE = language_ISO[lang_id];
	}
	else if (show_fps_cb->checkClick()) {
		if (show_fps_cb->isChecked()) SHOW_FPS=true;
		else SHOW_FPS=false;
	}
	else if (colorblind_cb->checkClick()) {
		if (colorblind_cb->isChecked()) COLORBLIND=true;
		else COLORBLIND=false;
	}
	else if (hardware_cursor_cb->checkClick()) {
		if (hardware_cursor_cb->isChecked()) HARDWARE_CURSOR=true;
		else HARDWARE_CURSOR=false;
	}
	else if (dev_mode_cb->checkClick()) {
		if (dev_mode_cb->isChecked()) DEV_MODE=true;
		else DEV_MODE=false;
	}
	else if (loot_tooltips_cb->checkClick()) {
		if (loot_tooltips_cb->isChecked()) LOOT_TOOLTIPS=true;
		else LOOT_TOOLTIPS=false;
	}
	else if (statbar_labels_cb->checkClick()) {
		if (statbar_labels_cb->isChecked()) STATBAR_LABELS=true;
		else STATBAR_LABELS=false;
	}
	else if (auto_equip_cb->checkClick()) {
		if (auto_equip_cb->isChecked()) AUTO_EQUIP=true;
		else AUTO_EQUIP=false;
	}
}

void GameStateConfigBase::logicMods() {
	if (activemods_lstb->checkClick()) {
		//do nothing
	}
	else if (inactivemods_lstb->checkClick()) {
		//do nothing
	}
	else if (activemods_shiftup_btn->checkClick()) {
		activemods_lstb->shiftUp();
	}
	else if (activemods_shiftdown_btn->checkClick()) {
		activemods_lstb->shiftDown();
	}
	else if (activemods_deactivate_btn->checkClick()) {
		disableMods();
	}
	else if (inactivemods_activate_btn->checkClick()) {
		enableMods();
	}
}

void GameStateConfigBase::render() {
	if (requestedGameState != NULL) {
		// we're in the process of switching game states, so skip rendering
		return;
	}

	int tabheight = tab_control->getTabHeight();
	Rect	pos;
	pos.x = (VIEW_W-FRAME_W)/2;
	pos.y = (VIEW_H-FRAME_H)/2 + tabheight - tabheight/16;

	if (background) {
		background->setDest(pos);
		render_device->render(background);
	}

	tab_control->render();

	// render OK/Defaults/Cancel buttons
	ok_button->render();
	cancel_button->render();
	defaults_button->render();

	renderTabContents();
	renderDialogs();

	// Get tooltips for listboxes
	// This isn't very elegant right now
	// In the future, we'll want to get tooltips for all widget types
	TooltipData tip_new;
	renderTooltips(tip_new);

	if (!tip_new.isEmpty()) {
		if (!tip_new.compare(&tip_buf)) {
			tip_buf.clear();
			tip_buf = tip_new;
		}
		tip->render(tip_buf, inpt->mouse, STYLE_FLOAT);
	}
}

void GameStateConfigBase::renderTabContents() {
	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (optiontab[i] == active_tab) child_widget[i]->render();
	}

}

void GameStateConfigBase::renderDialogs() {
	if (defaults_confirm->visible)
		defaults_confirm->render();
}

void GameStateConfigBase::renderTooltips(TooltipData& tip_new) {
	if (active_tab == INTERFACE_TAB && tip_new.isEmpty()) tip_new = language_lstb->checkTooltip(inpt->mouse);
	if (active_tab == MODS_TAB && tip_new.isEmpty()) tip_new = activemods_lstb->checkTooltip(inpt->mouse);
	if (active_tab == MODS_TAB && tip_new.isEmpty()) tip_new = inactivemods_lstb->checkTooltip(inpt->mouse);
}

void GameStateConfigBase::placeLabeledWidget(WidgetLabel *lb, Widget *w, int x1, int y1, int x2, int y2, std::string const& str, int justify) {
	if (w) {
		w->setBasePos(x2, y2);
	}

	if (lb) {
		lb->setBasePos(x1, y1);
		lb->set(str);
		lb->setJustify(justify);
	}
}

void GameStateConfigBase::refreshWidgets() {
	tab_control->setMainArea(((VIEW_W - FRAME_W)/2)+3, (VIEW_H - FRAME_H)/2, FRAME_W, FRAME_H);
	tab_control->updateHeader();
	frame = tab_control->getContentArea();

	for (unsigned i=0; i<child_widget.size(); ++i) {
		child_widget[i]->setPos(frame.x, frame.y);
	}

	ok_button->setPos();
	defaults_button->setPos();
	cancel_button->setPos();

	defaults_confirm->align();
}

void GameStateConfigBase::addChildWidget(Widget *w, int tab) {
	child_widget.push_back(w);
	optiontab.push_back(tab);
}

void GameStateConfigBase::refreshLanguages() {
	language_ISO.clear();
	language_lstb->clear();

	FileParser infile;
	if (infile.open("engine/languages.txt")) {
		int i = 0;
		while (infile.next()) {
			std::string key = infile.key;
			if (key != "") {
				language_ISO.push_back(key);
				language_lstb->append(infile.val, "");

				if (language_ISO.back() == LANGUAGE) {
					language_lstb->select(i);
				}

				i++;
			}
		}
		infile.close();
	}

	language_lstb->refresh();
}

void GameStateConfigBase::refreshFont() {
	delete font;
	font = getFontEngine();
	delete comb;
	comb = new CombatText();
}

void GameStateConfigBase::enableMods() {
	for (int i=0; i<inactivemods_lstb->getSize(); i++) {
		if (inactivemods_lstb->isSelected(i)) {
			activemods_lstb->append(inactivemods_lstb->getValue(i),inactivemods_lstb->getTooltip(i));
			inactivemods_lstb->remove(i);
			i--;
		}
	}
}

void GameStateConfigBase::disableMods() {
	for (int i=0; i<activemods_lstb->getSize(); i++) {
		if (activemods_lstb->isSelected(i) && activemods_lstb->getValue(i) != FALLBACK_MOD) {
			inactivemods_lstb->append(activemods_lstb->getValue(i),activemods_lstb->getTooltip(i));
			activemods_lstb->remove(i);
			i--;
		}
	}
	inactivemods_lstb->sort();
}

bool GameStateConfigBase::setMods() {
	// Save new mods list. Return true if modlist was changed. Else return false

	std::vector<Mod> temp_list = mods->mod_list;
	mods->mod_list.clear();
	mods->mod_list.push_back(mods->loadMod(FALLBACK_MOD));

	for (int i=0; i<activemods_lstb->getSize(); i++) {
		if (activemods_lstb->getValue(i) != "")
			mods->mod_list.push_back(mods->loadMod(activemods_lstb->getValue(i)));
	}

	mods->applyDepends();
	mods->saveMods();

	if (mods->mod_list != temp_list)
		return true;
	else
		return false;
}

std::string GameStateConfigBase::createModTooltip(Mod *mod) {
	std::string ret = "";
	if (mod) {
		std::string min_version = "";
		std::string max_version = "";
		std::stringstream ss;

		if (mod->min_version_major > 0 || mod->min_version_minor > 0) {
			ss << mod->min_version_major << "." << std::setfill('0') << std::setw(2) << mod->min_version_minor;
			min_version = ss.str();
			ss.str("");
		}


		if (mod->max_version_major < INT_MAX || mod->max_version_minor < INT_MAX) {
			ss << mod->max_version_major << "." << std::setfill('0') << std::setw(2) << mod->max_version_minor;
			max_version = ss.str();
			ss.str("");
		}

		ret = mod->description;
		if (mod->game != "" && mod->game != FALLBACK_GAME) {
			if (ret != "") ret += '\n';
			ret += msg->get("Game: ");
			ret += mod->game;
		}
		if (min_version != "" || max_version != "") {
			if (ret != "") ret += '\n';
			ret += msg->get("Requires version: ");
			if (min_version != "" && min_version != max_version) {
				ret += min_version;
				if (max_version != "") {
					ret += " - ";
					ret += max_version;
				}
			}
			else if (max_version != "") {
				ret += max_version;
			}
		}
		if (!mod->depends.empty()) {
			if (ret != "") ret += '\n';
			ret += msg->get("Requires mods: ");
			for (unsigned i=0; i<mod->depends.size(); ++i) {
				ret += mod->depends[i];
				if (i < mod->depends.size()-1)
					ret += ", ";
			}
		}
	}
	return ret;
}

void GameStateConfigBase::cleanup() {
	if (background) {
		delete background;
		background = NULL;
	}

	tip_buf.clear();
	if (tip != NULL) {
		delete tip;
		tip = NULL;
	}

	if (tab_control != NULL) {
		delete tab_control;
		tab_control = NULL;
	}

	if (ok_button != NULL) {
		delete ok_button;
		ok_button = NULL;
	}
	if (defaults_button != NULL) {
		delete defaults_button;
		defaults_button = NULL;
	}
	if (cancel_button != NULL) {
		delete cancel_button;
		cancel_button = NULL;
	}

	cleanupTabContents();
	cleanupDialogs();

	language_ISO.clear();
}

void GameStateConfigBase::cleanupTabContents() {
	for (std::vector<Widget*>::iterator iter = child_widget.begin(); iter != child_widget.end(); ++iter) {
		if (*iter != NULL) {
			delete (*iter);
			*iter = NULL;
		}
	}
	child_widget.clear();
}

void GameStateConfigBase::cleanupDialogs() {
	if (defaults_confirm != NULL) {
		delete defaults_confirm;
		defaults_confirm = NULL;
	}
}

