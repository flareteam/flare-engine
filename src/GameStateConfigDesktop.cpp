/*
Copyright © 2014 Justin Jacobs

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
 * GameStateConfigDesktop
 *
 * Handle game Settings Menu (desktop computer settings)
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameStateConfigBase.h"
#include "GameStateConfigDesktop.h"
#include "GameStateTitle.h"
#include "GameStateResolution.h"
#include "MenuConfirm.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetCheckBox.h"
#include "WidgetListBox.h"
#include "WidgetScrollBox.h"
#include "WidgetSlider.h"
#include "WidgetTabControl.h"
#include "WidgetTooltip.h"

#include <limits.h>
#include <iomanip>

bool rescompare(const Rect &r1, const Rect &r2) {
	if (r1.w == r2.w) return r1.h > r2.h;
	return r1.w > r2.w;
}

GameStateConfigDesktop::GameStateConfigDesktop()
	: GameStateConfigBase(false)
	, resolution_lstb(new WidgetListBox(10))
	, resolution_lb(new WidgetLabel())
	, fullscreen_cb(new WidgetCheckBox())
	, fullscreen_lb(new WidgetLabel())
	, hwsurface_cb(new WidgetCheckBox())
	, hwsurface_lb(new WidgetLabel())
	, doublebuf_cb(new WidgetCheckBox())
	, doublebuf_lb(new WidgetLabel())
	, change_gamma_cb(new WidgetCheckBox())
	, change_gamma_lb(new WidgetLabel())
	, gamma_sl(new WidgetSlider())
	, gamma_lb(new WidgetLabel())
	, hws_note_lb(new WidgetLabel())
	, dbuf_note_lb(new WidgetLabel())
	, test_note_lb(new WidgetLabel())
	, joystick_device_lstb(new WidgetListBox(10))
	, joystick_device_lb(new WidgetLabel())
	, enable_joystick_cb(new WidgetCheckBox())
	, enable_joystick_lb(new WidgetLabel())
	, mouse_move_cb(new WidgetCheckBox())
	, mouse_move_lb(new WidgetLabel())
	, mouse_aim_cb(new WidgetCheckBox())
	, mouse_aim_lb(new WidgetLabel())
	, no_mouse_cb(new WidgetCheckBox())
	, no_mouse_lb(new WidgetLabel())
	, joystick_deadzone_sl(new WidgetSlider())
	, joystick_deadzone_lb(new WidgetLabel())
	, handheld_note_lb(new WidgetLabel())
	, input_scrollbox(NULL)
	, input_confirm(new MenuConfirm(msg->get("Clear"),msg->get("Assign: ")))
	, input_confirm_ticks(0)
	, input_key(0)
	, key_count(0)
	, fullscreen(FULLSCREEN)
	, hwsurface(HWSURFACE)
	, doublebuf(DOUBLEBUF)
	, scrollpane_contents(0)
{
	// Populate the resolution list
	if (getVideoModes() < 1)
		logError("GameStateConfigDesktop: Unable to get resolutions list!");

	// Allocate KeyBindings
	for (int i = 0; i < inpt->key_count; i++) {
		keybinds_lb.push_back(new WidgetLabel());
		keybinds_lb[i]->set(inpt->binding_name[i]);
		keybinds_lb[i]->setJustify(JUSTIFY_RIGHT);
	}
	for (int i = 0; i < inpt->key_count * 3; i++) {
		keybinds_btn.push_back(new WidgetButton());
	}

	key_count = keybinds_btn.size()/3;

	init();
}

GameStateConfigDesktop::~GameStateConfigDesktop() {
}

void GameStateConfigDesktop::init() {
	VIDEO_TAB = 0;
	AUDIO_TAB = 1;
	INTERFACE_TAB = 2;
	INPUT_TAB = 3;
	KEYBINDS_TAB = 4;
	MODS_TAB = 5;

	tab_control->setTabTitle(VIDEO_TAB, msg->get("Video"));
	tab_control->setTabTitle(AUDIO_TAB, msg->get("Audio"));
	tab_control->setTabTitle(INTERFACE_TAB, msg->get("Interface"));
	tab_control->setTabTitle(INPUT_TAB, msg->get("Input"));
	tab_control->setTabTitle(KEYBINDS_TAB, msg->get("Keybindings"));
	tab_control->setTabTitle(MODS_TAB, msg->get("Mods"));
	tab_control->updateHeader();

	readConfig();

	// Allocate KeyBindings ScrollBox
	input_scrollbox = new WidgetScrollBox(scrollpane.w, scrollpane.h);
	input_scrollbox->pos.x = scrollpane.x + frame.x;
	input_scrollbox->pos.y = scrollpane.y + frame.y;
	input_scrollbox->bg.r = scrollpane_color.r;
	input_scrollbox->bg.g = scrollpane_color.g;
	input_scrollbox->bg.b = scrollpane_color.b;
	input_scrollbox->transparent = false;
	input_scrollbox->resize(scrollpane_contents);

	// Set positions of secondary key bindings
	for (unsigned int i = key_count; i < key_count*2; i++) {
		keybinds_btn[i]->pos.x = keybinds_btn[i-key_count]->pos.x + secondary_offset.x;
		keybinds_btn[i]->pos.y = keybinds_btn[i-key_count]->pos.y + secondary_offset.y;
	}

	// Set positions of joystick bindings
	for (unsigned int i = key_count*2; i < keybinds_btn.size(); i++) {
		keybinds_btn[i]->pos.x = keybinds_btn[i-(key_count*2)]->pos.x + (secondary_offset.x*2);
		keybinds_btn[i]->pos.y = keybinds_btn[i-(key_count*2)]->pos.y + (secondary_offset.y*2);
	}

	addChildWidgets();
	addChildWidgetsDesktop();
	setupTabList();

	update();
}

void GameStateConfigDesktop::readConfig() {
	FileParser infile;
	if (infile.open("menus/config.txt")) {
		while (infile.next()) {
			int x1 = popFirstInt(infile.val);
			int y1 = popFirstInt(infile.val);
			int x2 = popFirstInt(infile.val);
			int y2 = popFirstInt(infile.val);

			if (parseKeyDesktop(infile, x1, y1, x2, y2))
				continue;
			else if (parseKey(infile, x1, y1, x2, y2))
				continue;
			else {
				infile.error("GameStateConfigDesktop: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
}

bool GameStateConfigDesktop::parseKeyDesktop(FileParser &infile, int &x1, int &y1, int &x2, int &y2) {
	// @CLASS GameStateConfigDesktop|Description of menus/config.txt

	int keybind_num = -1;

	if (infile.key == "listbox_scrollbar_offset") {
		// overrides same key in GameStateConfigBase
		joystick_device_lstb->scrollbar_offset = x1;
		resolution_lstb->scrollbar_offset = x1;
		activemods_lstb->scrollbar_offset = x1;
		inactivemods_lstb->scrollbar_offset = x1;
		language_lstb->scrollbar_offset = x1;
	}
	else if (infile.key == "resolution") {
		// @ATTR resolution|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Resolution" list box relative to the frame.
		placeLabeledWidget(resolution_lb, resolution_lstb, x1, y1, x2, y2, msg->get("Resolution"));
	}
	else if (infile.key == "fullscreen") {
		// @ATTR fullscreen|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Full Screen Mode" checkbox relative to the frame.
		placeLabeledWidget(fullscreen_lb, fullscreen_cb, x1, y1, x2, y2, msg->get("Full Screen Mode"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "mouse_move") {
		// @ATTR mouse_move|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Move hero using mouse" checkbox relative to the frame.
		placeLabeledWidget(mouse_move_lb, mouse_move_cb, x1, y1, x2, y2, msg->get("Move hero using mouse"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "hwsurface") {
		// @ATTR hwsurface|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Hardware surfaces" checkbox relative to the frame.
		placeLabeledWidget(hwsurface_lb, hwsurface_cb, x1, y1, x2, y2, msg->get("Hardware surfaces"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "doublebuf") {
		// @ATTR doublebuf|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Double buffering" checkbox relative to the frame.
		placeLabeledWidget(doublebuf_lb, doublebuf_cb, x1, y1, x2, y2, msg->get("Double buffering"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "change_gamma") {
		// @ATTR change_gamma|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Allow changing gamma" checkbox relative to the frame.
		placeLabeledWidget(change_gamma_lb, change_gamma_cb, x1, y1, x2, y2, msg->get("Allow changing gamma"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "gamma") {
		// @ATTR gamma|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Gamma" slider relative to the frame.
		placeLabeledWidget(gamma_lb, gamma_sl, x1, y1, x2, y2, msg->get("Gamma"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "hws_note") {
		// @ATTR hws_note|x (integer), y (integer)|Position of the "Disable for performance" label (next to Hardware surfaces) relative to the frame.
		hws_note_lb->setX(frame.x + x1);
		hws_note_lb->setY(frame.y + y1);
		hws_note_lb->set(msg->get("Disable for performance"));
	}
	else if (infile.key == "dbuf_note") {
		// @ATTR dbuf_note|x (integer), y (integer)|Position of the "Disable for performance" label (next to Double buffering) relative to the frame.
		dbuf_note_lb->setX(frame.x + x1);
		dbuf_note_lb->setY(frame.y + y1);
		dbuf_note_lb->set(msg->get("Disable for performance"));
	}
	else if (infile.key == "test_note") {
		// @ATTR test_note|x (integer), y (integer)|Position of the "Experimental" label relative to the frame.
		test_note_lb->setX(frame.x + x1);
		test_note_lb->setY(frame.y + y1);
		test_note_lb->set(msg->get("Experimental"));
	}
	else if (infile.key == "enable_joystick") {
		// @ATTR enable_joystick|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Use joystick" checkbox relative to the frame.
		placeLabeledWidget(enable_joystick_lb, enable_joystick_cb, x1, y1, x2, y2, msg->get("Use joystick"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "joystick_device") {
		// @ATTR joystick_device|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Joystick" list box relative to the frame.
		placeLabeledWidget(joystick_device_lb, joystick_device_lstb, x1, y1, x2, y2, msg->get("Joystick"));

		for(int i = 0; i < SDL_NumJoysticks(); i++) {
			std::string joystick_name = inpt->getJoystickName(i);
			if (joystick_name != "")
				joystick_device_lstb->append(joystick_name, joystick_name);
		}
	}
	else if (infile.key == "mouse_aim") {
		// @ATTR mouse_aim|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Mouse aim" checkbox relative to the frame.
		placeLabeledWidget(mouse_aim_lb, mouse_aim_cb, x1, y1, x2, y2, msg->get("Mouse aim"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "no_mouse") {
		// @ATTR no_mouse|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Do not use mouse" checkbox relative to the frame.
		placeLabeledWidget(no_mouse_lb, no_mouse_cb, x1, y1, x2, y2, msg->get("Do not use mouse"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "joystick_deadzone") {
		// @ATTR joystick_deadzone|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Joystick Deadzone" slider relative to the frame.
		placeLabeledWidget(joystick_deadzone_lb, joystick_deadzone_sl, x1, y1, x2, y2, msg->get("Joystick Deadzone"), JUSTIFY_RIGHT);
	}
	else if (infile.key == "handheld_note") {
		// @ATTR handheld_note|x (integer), y (integer)|Position of the "For handheld devices" label relative to the frame.
		handheld_note_lb->setX(frame.x + x1);
		handheld_note_lb->setY(frame.y + y1);
		handheld_note_lb->set(msg->get("For handheld devices"));
	}
	else if (infile.key == "secondary_offset") {
		// @ATTR secondary_offset|x (integer), y (integer)|Offset of the second (and third) columns of keybinds.
		secondary_offset.x = x1;
		secondary_offset.y = y1;
	}
	else if (infile.key == "keybinds_bg_color") {
		// @ATTR keybinds_bg_color|r (integer), g (integer), b (integer)|Background color for the keybindings scrollbox.
		scrollpane_color.r = x1;
		scrollpane_color.g = y1;
		scrollpane_color.b = x2;
	}
	else if (infile.key == "scrollpane") {
		// @ATTR scrollpane|x (integer), y (integer), w (integer), h (integer)|Position of the keybinding scrollbox relative to the frame.
		scrollpane.x = x1;
		scrollpane.y = y1;
		scrollpane.w = x2;
		scrollpane.h = y2;
	}
	else if (infile.key == "scrollpane_contents") {
		// @ATTR scrollpane_contents|integer|The vertical size of the keybinding scrollbox's contents.
		scrollpane_contents = x1;
	}

	// @ATTR cancel|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Cancel" keybind relative to the keybinding scrollbox.
	else if (infile.key == "cancel") keybind_num = CANCEL;
	// @ATTR accept|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Accept" keybind relative to the keybinding scrollbox.
	else if (infile.key == "accept") keybind_num = ACCEPT;
	// @ATTR up|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Up" keybind relative to the keybinding scrollbox.
	else if (infile.key == "up") keybind_num = UP;
	// @ATTR down|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Down" keybind relative to the keybinding scrollbox.
	else if (infile.key == "down") keybind_num = DOWN;
	// @ATTR left|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Left" keybind relative to the keybinding scrollbox.
	else if (infile.key == "left") keybind_num = LEFT;
	// @ATTR right|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Right" keybind relative to the keybinding scrollbox.
	else if (infile.key == "right") keybind_num = RIGHT;
	// @ATTR bar1|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar1" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar1") keybind_num = BAR_1;
	// @ATTR bar2|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar2" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar2") keybind_num = BAR_2;
	// @ATTR bar3|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar3" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar3") keybind_num = BAR_3;
	// @ATTR bar4|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar4" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar4") keybind_num = BAR_4;
	// @ATTR bar5|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar5" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar5") keybind_num = BAR_5;
	// @ATTR bar6|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar6" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar6") keybind_num = BAR_6;
	// @ATTR bar7|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar7" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar7") keybind_num = BAR_7;
	// @ATTR Bar8|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar8" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar8") keybind_num = BAR_8;
	// @ATTR bar9|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar9" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar9") keybind_num = BAR_9;
	// @ATTR bar0|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Bar0" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar0") keybind_num = BAR_0;
	// @ATTR main1|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Main1" keybind relative to the keybinding scrollbox.
	else if (infile.key == "main1") keybind_num = MAIN1;
	// @ATTR main2|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Main2" keybind relative to the keybinding scrollbox.
	else if (infile.key == "main2") keybind_num = MAIN2;
	// @ATTR character|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Character" keybind relative to the keybinding scrollbox.
	else if (infile.key == "character") keybind_num = CHARACTER;
	// @ATTR inventory|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Inventory" keybind relative to the keybinding scrollbox.
	else if (infile.key == "inventory") keybind_num = INVENTORY;
	// @ATTR powers|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Powers" keybind relative to the keybinding scrollbox.
	else if (infile.key == "powers") keybind_num = POWERS;
	// @ATTR log|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Log" keybind relative to the keybinding scrollbox.
	else if (infile.key == "log") keybind_num = LOG;
	// @ATTR ctrl|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Ctrl" keybind relative to the keybinding scrollbox.
	else if (infile.key == "ctrl") keybind_num = CTRL;
	// @ATTR shift|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Shift" keybind relative to the keybinding scrollbox.
	else if (infile.key == "shift") keybind_num = SHIFT;
	// @ATTR alt|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Alt" keybind relative to the keybinding scrollbox.
	else if (infile.key == "alt") keybind_num = ALT;
	// @ATTR delete|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Delete" keybind relative to the keybinding scrollbox.
	else if (infile.key == "delete") keybind_num = DEL;
	// @ATTR actionbar|label x (integer), label y (integer), x (integer), y (integer)|Position of the "ActionBar Accept" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar") keybind_num = ACTIONBAR;
	// @ATTR actionbar_back|label x (integer), label y (integer), x (integer), y (integer)|Position of the "ActionBar Left" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_back") keybind_num = ACTIONBAR_BACK;
	// @ATTR actionbar_forward|label x (integer), label y (integer), x (integer), y (integer)|Position of the "ActionBar Right" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_forward") keybind_num = ACTIONBAR_FORWARD;
	// @ATTR actionbar_use|label x (integer), label y (integer), x (integer), y (integer)|Position of the "ActionBar Use" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_use") keybind_num = ACTIONBAR_USE;
	// @ATTR developer_menu|label x (integer), label y (integer), x (integer), y (integer)|Position of the "Developer Menu" keybind relative to the keybinding scrollbox.
	else if (infile.key == "developer_menu") keybind_num = DEVELOPER_MENU;

	else return false;

	if (keybind_num > -1 && (unsigned)keybind_num < keybinds_lb.size() && (unsigned)keybind_num < keybinds_btn.size()) {
		//keybindings
		keybinds_lb[keybind_num]->setX(x1);
		keybinds_lb[keybind_num]->setY(y1);
		keybinds_btn[keybind_num]->pos.x = x2;
		keybinds_btn[keybind_num]->pos.y = y2;
	}

	return true;
}

void GameStateConfigDesktop::addChildWidgetsDesktop() {
	addChildWidget(fullscreen_cb, VIDEO_TAB);
	addChildWidget(fullscreen_lb, VIDEO_TAB);
	addChildWidget(hwsurface_cb, VIDEO_TAB);
	addChildWidget(hwsurface_lb, VIDEO_TAB);
	addChildWidget(doublebuf_cb, VIDEO_TAB);
	addChildWidget(doublebuf_lb, VIDEO_TAB);
	addChildWidget(change_gamma_cb, VIDEO_TAB);
	addChildWidget(change_gamma_lb, VIDEO_TAB);
	addChildWidget(gamma_sl, VIDEO_TAB);
	addChildWidget(gamma_lb, VIDEO_TAB);
	addChildWidget(resolution_lstb, VIDEO_TAB);
	addChildWidget(resolution_lb, VIDEO_TAB);
	addChildWidget(hws_note_lb, VIDEO_TAB);
	addChildWidget(dbuf_note_lb, VIDEO_TAB);
	addChildWidget(test_note_lb, VIDEO_TAB);

	addChildWidget(mouse_move_cb, INPUT_TAB);
	addChildWidget(mouse_move_lb, INPUT_TAB);
	addChildWidget(enable_joystick_cb, INPUT_TAB);
	addChildWidget(enable_joystick_lb, INPUT_TAB);
	addChildWidget(mouse_aim_cb, INPUT_TAB);
	addChildWidget(mouse_aim_lb, INPUT_TAB);
	addChildWidget(no_mouse_cb, INPUT_TAB);
	addChildWidget(no_mouse_lb, INPUT_TAB);
	addChildWidget(joystick_deadzone_sl, INPUT_TAB);
	addChildWidget(joystick_deadzone_lb, INPUT_TAB);
	addChildWidget(joystick_device_lstb, INPUT_TAB);
	addChildWidget(joystick_device_lb, INPUT_TAB);
	addChildWidget(handheld_note_lb, INPUT_TAB);

	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		input_scrollbox->addChildWidget(keybinds_btn[i]);
	}
}

void GameStateConfigDesktop::setupTabList() {
	tablist.add(ok_button);
	tablist.add(defaults_button);
	tablist.add(cancel_button);
	tablist.add(fullscreen_cb);
	tablist.add(hwsurface_cb);
	tablist.add(doublebuf_cb);
	tablist.add(change_gamma_cb);
	tablist.add(gamma_sl);
	tablist.add(resolution_lstb);

	tablist.add(music_volume_sl);
	tablist.add(sound_volume_sl);

	tablist.add(combat_text_cb);
	tablist.add(show_fps_cb);
	tablist.add(colorblind_cb);
	tablist.add(show_hotkeys_cb);
	tablist.add(hardware_cursor_cb);
	tablist.add(dev_mode_cb);
	tablist.add(show_target_cb);
	tablist.add(loot_tooltips_cb);
	tablist.add(language_lstb);

	tablist.add(enable_joystick_cb);
	tablist.add(mouse_move_cb);
	tablist.add(mouse_aim_cb);
	tablist.add(no_mouse_cb);
	tablist.add(joystick_deadzone_sl);
	tablist.add(joystick_device_lstb);

	tablist.add(input_scrollbox);

	tablist.add(inactivemods_lstb);
	tablist.add(activemods_lstb);
	tablist.add(inactivemods_activate_btn);
	tablist.add(activemods_deactivate_btn);
	tablist.add(activemods_shiftup_btn);
	tablist.add(activemods_shiftdown_btn);
}

void GameStateConfigDesktop::update() {
	GameStateConfigBase::update();

	updateVideo();
	updateInput();
	updateKeybinds();
}

void GameStateConfigDesktop::updateVideo() {
	if (FULLSCREEN) fullscreen_cb->Check();
	else fullscreen_cb->unCheck();
	if (HWSURFACE) hwsurface_cb->Check();
	else hwsurface_cb->unCheck();
	if (DOUBLEBUF) doublebuf_cb->Check();
	else doublebuf_cb->unCheck();
	if (CHANGE_GAMMA) change_gamma_cb->Check();
	else {
		change_gamma_cb->unCheck();
		GAMMA = 1.0;
		gamma_sl->enabled = false;
	}
	gamma_sl->set(5,20,(int)(GAMMA*10.0));
	render_device->setGamma(GAMMA);

	std::stringstream list_mode;
	unsigned int resolutions = getVideoModes();
	for (unsigned int i=0; i<resolutions; ++i) {
		list_mode << video_modes[i].w << "x" << video_modes[i].h;
		resolution_lstb->append(list_mode.str(),"");
		if (video_modes[i].w == VIEW_W && video_modes[i].h == VIEW_H) resolution_lstb->selected[i] = true;
		else resolution_lstb->selected[i] = false;
		list_mode.str("");
	}
	resolution_lstb->refresh();
}

void GameStateConfigDesktop::updateInput() {
	if (ENABLE_JOYSTICK) enable_joystick_cb->Check();
	else enable_joystick_cb->unCheck();
	if (MOUSE_AIM) mouse_aim_cb->Check();
	else mouse_aim_cb->unCheck();
	if (NO_MOUSE) no_mouse_cb->Check();
	else no_mouse_cb->unCheck();
	if (MOUSE_MOVE) mouse_move_cb->Check();
	else mouse_move_cb->unCheck();

	SDL_Init(SDL_INIT_JOYSTICK);
	if (ENABLE_JOYSTICK && SDL_NumJoysticks() > 0) {
		SDL_JoystickClose(joy);
		joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
		joystick_device_lstb->selected[JOYSTICK_DEVICE] = true;
	}
	joystick_device_lstb->refresh();

	joystick_deadzone_sl->set(0,32768,JOY_DEADZONE);
}

void GameStateConfigDesktop::updateKeybinds() {
	// reset all keybind labels to "(none)"
	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		keybinds_btn[i]->label = msg->get("(none)");
	}

	// now do labels for keybinds that are set
	for (unsigned int i = 0; i < key_count; i++) {
		if (inpt->binding[i] >= 0) {
			if (inpt->binding[i] < 8) {
				keybinds_btn[i]->label = inpt->mouse_button[inpt->binding[i]-1];
			}
			else {
				keybinds_btn[i]->label = inpt->getKeyName(inpt->binding[i]);
			}
		}
		keybinds_btn[i]->refresh();
	}
	for (unsigned int i = key_count; i < key_count*2; i++) {
		if (inpt->binding_alt[i-key_count] >= 0) {
			if (inpt->binding_alt[i-key_count] < 8) {
				keybinds_btn[i]->label = inpt->mouse_button[inpt->binding_alt[i-key_count]-1];
			}
			else {
				keybinds_btn[i]->label = inpt->getKeyName(inpt->binding_alt[i-key_count]);
			}
		}
		keybinds_btn[i]->refresh();
	}
	for (unsigned int i = key_count*2; i < keybinds_btn.size(); i++) {
		if (inpt->binding_joy[i-(key_count*2)] >= 0) {
			keybinds_btn[i]->label = msg->get("Button %d", inpt->binding_joy[i-(key_count*2)]);
		}
		keybinds_btn[i]->refresh();
	}
	input_scrollbox->refresh();
}

void GameStateConfigDesktop::logic() {
	if (defaults_confirm->visible) {
		// reset defaults confirmation
		logicDefaults();
		return;
	}
	else if (input_confirm->visible) {
		// assign a keybind
		input_confirm->logic();
		scanKey(input_key);
		input_confirm_ticks--;
		if (input_confirm_ticks == 0) input_confirm->visible = false;
		return;
	}
	else {
		if (!logicMain())
			return;
	}

	// tab contents
	active_tab = tab_control->getActiveTab();

	if (active_tab == VIDEO_TAB)
		logicVideo();
	else if (active_tab == AUDIO_TAB)
		logicAudio();
	else if (active_tab == INTERFACE_TAB)
		logicInterface();
	else if (active_tab == INPUT_TAB)
		logicInput();
	else if (active_tab == KEYBINDS_TAB)
		logicKeybinds();
	else if (active_tab == MODS_TAB)
		logicMods();
}

void GameStateConfigDesktop::logicAccept() {
	std::string resolution_value = resolution_lstb->getValue();
	int width = popFirstInt(resolution_value, 'x');
	int height = popFirstInt(resolution_value, 'x');

	// In case of a custom resolution, the listbox might have nothing selected
	// So we just use whatever the current view area is
	if (width == 0 || height == 0) {
		width = VIEW_W;
		height = VIEW_H;
	}

	delete msg;
	msg = new MessageEngine();
	inpt->saveKeyBindings();
	inpt->setKeybindNames();
	if (setMods()) {
		reload_music = true;
		delete mods;
		mods = new ModManager();
		loadTilesetSettings();
		SharedResources::loadIcons();
		delete curs;
		curs = new CursorManager();
	}
	loadMiscSettings();
	refreshFont();
	if ((ENABLE_JOYSTICK) && (SDL_NumJoysticks() > 0)) {
		SDL_JoystickClose(joy);
		joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
	}
	cleanup();
	saveSettings();
	render_device->updateTitleBar();
	delete requestedGameState;
	requestedGameState = new GameStateResolution(width, height, fullscreen, hwsurface, doublebuf);
}

void GameStateConfigDesktop::logicVideo() {
	if (fullscreen_cb->checkClick()) {
		if (fullscreen_cb->isChecked()) FULLSCREEN=true;
		else FULLSCREEN=false;
	}
	else if (hwsurface_cb->checkClick()) {
		if (hwsurface_cb->isChecked()) HWSURFACE=true;
		else HWSURFACE=false;
	}
	else if (doublebuf_cb->checkClick()) {
		if (doublebuf_cb->isChecked()) DOUBLEBUF=true;
		else DOUBLEBUF=false;
	}
	else if (change_gamma_cb->checkClick()) {
		if (change_gamma_cb->isChecked()) {
			CHANGE_GAMMA=true;
			gamma_sl->enabled = true;
		}
		else {
			CHANGE_GAMMA=false;
			GAMMA = 1.0;
			gamma_sl->enabled = false;
			gamma_sl->set(5,20,(int)(GAMMA*10.0));
			render_device->setGamma(GAMMA);
		}
	}
	else if (resolution_lstb->checkClick()) {
		// nothing to do here: resolution value changes next frame.
	}
	else if (CHANGE_GAMMA) {
		gamma_sl->enabled = true;
		if (gamma_sl->checkClick()) {
			GAMMA=(gamma_sl->getValue())*0.1f;
			render_device->setGamma(GAMMA);
		}
	}
}

void GameStateConfigDesktop::logicInput() {
	if (mouse_move_cb->checkClick()) {
		if (mouse_move_cb->isChecked()) {
			MOUSE_MOVE=true;
			no_mouse_cb->unCheck();
			NO_MOUSE=false;
		}
		else MOUSE_MOVE=false;
	}
	else if (mouse_aim_cb->checkClick()) {
		if (mouse_aim_cb->isChecked()) {
			MOUSE_AIM=true;
			no_mouse_cb->unCheck();
			NO_MOUSE=false;
		}
		else MOUSE_AIM=false;
	}
	else if (no_mouse_cb->checkClick()) {
		if (no_mouse_cb->isChecked()) {
			NO_MOUSE=true;
			mouse_aim_cb->unCheck();
			MOUSE_AIM=false;
			mouse_move_cb->unCheck();
			MOUSE_MOVE=false;
		}
		else NO_MOUSE=false;
	}
	else if (enable_joystick_cb->checkClick()) {
		if (enable_joystick_cb->isChecked()) {
			ENABLE_JOYSTICK=true;
			if (SDL_NumJoysticks() > 0) {
				JOYSTICK_DEVICE = 0;
				SDL_JoystickClose(joy);
				joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
				joystick_device_lstb->selected[JOYSTICK_DEVICE] = true;
			}
		}
		else {
			ENABLE_JOYSTICK=false;
			for (int i=0; i<joystick_device_lstb->getSize(); i++)
				joystick_device_lstb->selected[i] = false;
		}
		if (SDL_NumJoysticks() > 0) joystick_device_lstb->refresh();
	}
	else if (joystick_deadzone_sl->checkClick()) {
		JOY_DEADZONE = joystick_deadzone_sl->getValue();
	}
	else if (joystick_device_lstb->checkClick()) {
		JOYSTICK_DEVICE = joystick_device_lstb->getSelected();
		if (JOYSTICK_DEVICE != -1) {
			enable_joystick_cb->Check();
			ENABLE_JOYSTICK=true;
			if (SDL_NumJoysticks() > 0) {
				SDL_JoystickClose(joy);
				joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
			}
		}
		else {
			enable_joystick_cb->unCheck();
			ENABLE_JOYSTICK = false;
		}
	}
}

void GameStateConfigDesktop::logicKeybinds() {
	input_scrollbox->logic();
	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		if (keybinds_btn[i]->pressed || keybinds_btn[i]->hover) input_scrollbox->update = true;
		Point mouse = input_scrollbox->input_assist(inpt->mouse);
		if (keybinds_btn[i]->checkClick(mouse.x,mouse.y)) {
			std::string confirm_msg;
			confirm_msg = msg->get("Assign: ") + inpt->binding_name[i%key_count];
			delete input_confirm;
			input_confirm = new MenuConfirm(msg->get("Clear"),confirm_msg);
			input_confirm_ticks = MAX_FRAMES_PER_SEC * 10; // 10 seconds
			input_confirm->visible = true;
			input_key = i;
			inpt->last_button = -1;
			inpt->last_key = -1;
			inpt->last_joybutton = -1;
		}
	}
}

void GameStateConfigDesktop::renderTabContents() {
	if (active_tab == KEYBINDS_TAB) {
		if (input_scrollbox->update) {
			input_scrollbox->refresh();
		}
		input_scrollbox->render();
		for (unsigned int i = 0; i < keybinds_lb.size(); i++) {
			keybinds_lb[i]->local_frame = input_scrollbox->pos;
			keybinds_lb[i]->local_offset.y = input_scrollbox->getCursor();
			keybinds_lb[i]->render();
		}
	}

	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (optiontab[i] == active_tab) child_widget[i]->render();
	}
}

void GameStateConfigDesktop::renderDialogs() {
	GameStateConfigBase::renderDialogs();

	if (input_confirm->visible)
		input_confirm->render();
}

void GameStateConfigDesktop::renderTooltips(TooltipData& tip_new) {
	GameStateConfigBase::renderTooltips(tip_new);

	if (active_tab == VIDEO_TAB && tip_new.isEmpty()) tip_new = resolution_lstb->checkTooltip(inpt->mouse);
	if (active_tab == INPUT_TAB && tip_new.isEmpty()) tip_new = joystick_device_lstb->checkTooltip(inpt->mouse);
}

int GameStateConfigDesktop::getVideoModes() {
	video_modes.clear();

	// Set predefined modes
	const unsigned int cm_count = 5;
	Rect common_modes[cm_count];
	common_modes[0].w = 640;
	common_modes[0].h = 480;
	common_modes[1].w = 800;
	common_modes[1].h = 600;
	common_modes[2].w = 1024;
	common_modes[2].h = 768;
	common_modes[3].w = VIEW_W;
	common_modes[3].h = VIEW_H;
	common_modes[4].w = MIN_VIEW_W;
	common_modes[4].h = MIN_VIEW_H;

	// Get available fullscreen/hardware modes
	render_device->listModes(video_modes);

	for (unsigned i=0; i<cm_count; ++i) {
		video_modes.push_back(common_modes[i]);
		if (common_modes[i].w < MIN_VIEW_W || common_modes[i].h < MIN_VIEW_H) {
			video_modes.pop_back();
		}
		else {
			for (unsigned j=0; j<video_modes.size()-1; ++j) {
				if (video_modes[j].w == common_modes[i].w && video_modes[j].h == common_modes[i].h) {
					video_modes.pop_back();
					break;
				}
			}
		}
	}

	std::sort(video_modes.begin(), video_modes.end(), rescompare);

	return video_modes.size();
}

void GameStateConfigDesktop::scanKey(int button) {
	// clear the keybind if the user presses CTRL+Delete
	if (input_confirm->visible && input_confirm->confirmClicked) {
		if ((unsigned)button < key_count) inpt->binding[button] = -1;
		else if ((unsigned)button < key_count*2) inpt->binding_alt[button%key_count] = -1;
		else if ((unsigned)button < key_count*3) inpt->binding_joy[button%key_count] = -1;

		inpt->pressing[button%key_count] = false;
		inpt->lock[button%key_count] = false;

		keybinds_btn[button]->label = msg->get("(none)");
		input_confirm->visible = false;
		input_confirm_ticks = 0;
		keybinds_btn[button]->refresh();
		return;
	}

	if (input_confirm->visible && !input_confirm->isWithinButtons) {
		// keyboard & mouse
		if ((unsigned)button < key_count*2) {
			if (inpt->last_button != -1 && inpt->last_button < 8) {
				if ((unsigned)button < key_count) inpt->binding[button] = inpt->last_button;
				else inpt->binding_alt[button-key_count] = inpt->last_button;

				inpt->pressing[button%key_count] = false;
				inpt->lock[button%key_count] = false;

				keybinds_btn[button]->label = inpt->mouse_button[inpt->last_button-1];
				input_confirm->visible = false;
				input_confirm_ticks = 0;
				keybinds_btn[button]->refresh();
				return;
			}
			if (inpt->last_key != -1) {
				if ((unsigned)button < key_count) inpt->binding[button] = inpt->last_key;
				else inpt->binding_alt[button-key_count] = inpt->last_key;

				inpt->pressing[button%key_count] = false;
				inpt->lock[button%key_count] = false;

				keybinds_btn[button]->label = inpt->getKeyName(inpt->last_key);
				input_confirm->visible = false;
				input_confirm_ticks = 0;
				keybinds_btn[button]->refresh();
				return;
			}
		}
		// joystick
		else if ((unsigned)button >= key_count*2 && inpt->last_joybutton != -1) {
			inpt->binding_joy[button-(key_count*2)] = inpt->last_joybutton;

			keybinds_btn[button]->label = msg->get("Button %d", inpt->binding_joy[button-(key_count*2)]);
			input_confirm->visible = false;
			input_confirm_ticks = 0;
			keybinds_btn[button]->refresh();
		}
	}
}

void GameStateConfigDesktop::cleanupTabContents() {
	for (std::vector<Widget*>::iterator iter = child_widget.begin(); iter != child_widget.end(); ++iter) {
		if (*iter != NULL) {
			delete (*iter);
			*iter = NULL;
		}
	}
	child_widget.clear();

	for (unsigned int i = 0; i < keybinds_lb.size(); i++) {
		if (keybinds_lb[i] != NULL) {
			delete keybinds_lb[i];
			keybinds_lb[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		if (keybinds_btn[i] != NULL) {
			delete keybinds_btn[i];
			keybinds_btn[i] = NULL;
		}
	}

	if (input_scrollbox != NULL) {
		delete input_scrollbox;
		input_scrollbox = NULL;
	}
}

void GameStateConfigDesktop::cleanupDialogs() {
	if (defaults_confirm != NULL) {
		delete defaults_confirm;
		defaults_confirm = NULL;
	}
	if (input_confirm != NULL) {
		delete input_confirm;
		input_confirm = NULL;
	}
}
