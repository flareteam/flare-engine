/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
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
 * GameStateConfig
 *
 * Handle game Settings Menu
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameStateConfig.h"
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

using namespace std;

bool rescompare(const Rect &r1, const Rect &r2) {
	if (r1.w == r2.w) return r1.h > r2.h;
	return r1.w > r2.w;
}

GameStateConfig::GameStateConfig ()
	: GameState()
	, child_widget()
	, ok_button(NULL)
	, defaults_button(NULL)
	, cancel_button(NULL)
	, background(NULL)
	, tip_buf()
	, input_key(0)
	, check_resolution(true)
	, key_count(0) {

	Image *graphics;
	graphics = render_device->loadImage("images/menus/config.png");
	if (graphics) {
		background = graphics->createSprite();
		graphics->unref();
	}

	init();
	update();
}

void GameStateConfig::init() {

	tip = new WidgetTooltip();

	ok_button = new WidgetButton("images/menus/buttons/button_default.png");
	defaults_button = new WidgetButton("images/menus/buttons/button_default.png");
	cancel_button = new WidgetButton("images/menus/buttons/button_default.png");

	ok_button->label = msg->get("OK");
	ok_button->pos.x = VIEW_W_HALF - ok_button->pos.w/2;
	ok_button->pos.y = VIEW_H - (cancel_button->pos.h*3);
	ok_button->refresh();

	defaults_button->label = msg->get("Defaults");
	defaults_button->pos.x = VIEW_W_HALF - defaults_button->pos.w/2;
	defaults_button->pos.y = VIEW_H - (cancel_button->pos.h*2);
	defaults_button->refresh();

	cancel_button->label = msg->get("Cancel");
	cancel_button->pos.x = VIEW_W_HALF - cancel_button->pos.w/2;
	cancel_button->pos.y = VIEW_H - (cancel_button->pos.h);
	cancel_button->refresh();

	fullscreen_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	fullscreen_lb = new WidgetLabel();
	mouse_move_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	mouse_move_lb = new WidgetLabel();
	combat_text_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	combat_text_lb = new WidgetLabel();
	hwsurface_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	hwsurface_lb = new WidgetLabel();
	doublebuf_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	doublebuf_lb = new WidgetLabel();
	enable_joystick_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	enable_joystick_lb = new WidgetLabel();
	change_gamma_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	change_gamma_lb = new WidgetLabel();
	mouse_aim_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	mouse_aim_lb = new WidgetLabel();
	no_mouse_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	no_mouse_lb = new WidgetLabel();
	show_fps_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	show_fps_lb = new WidgetLabel();
	show_hotkeys_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	show_hotkeys_lb = new WidgetLabel();
	colorblind_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	colorblind_lb = new WidgetLabel();
	hardware_cursor_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	hardware_cursor_lb = new WidgetLabel();
	dev_mode_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	dev_mode_lb = new WidgetLabel();
	music_volume_sl = new WidgetSlider("images/menus/buttons/slider_default.png");
	music_volume_lb = new WidgetLabel();
	sound_volume_sl = new WidgetSlider("images/menus/buttons/slider_default.png");
	sound_volume_lb = new WidgetLabel();
	gamma_sl = new WidgetSlider("images/menus/buttons/slider_default.png");
	gamma_lb = new WidgetLabel();
	resolution_lb = new WidgetLabel();
	activemods_lstb = new WidgetListBox(10, "images/menus/buttons/listbox_default.png");
	activemods_lb = new WidgetLabel();
	inactivemods_lstb = new WidgetListBox(10, "images/menus/buttons/listbox_default.png");
	inactivemods_lb = new WidgetLabel();
	joystick_device_lstb = new WidgetListBox(10, "images/menus/buttons/listbox_default.png");
	joystick_device_lb = new WidgetLabel();
	language_lb = new WidgetLabel();
	hws_note_lb = new WidgetLabel();
	dbuf_note_lb = new WidgetLabel();
	test_note_lb = new WidgetLabel();
	handheld_note_lb = new WidgetLabel();
	activemods_shiftup_btn = new WidgetButton("images/menus/buttons/up.png");
	activemods_shiftdown_btn = new WidgetButton("images/menus/buttons/down.png");
	activemods_deactivate_btn = new WidgetButton("images/menus/buttons/button_default.png");
	inactivemods_activate_btn = new WidgetButton("images/menus/buttons/button_default.png");
	joystick_deadzone_sl = new WidgetSlider("images/menus/buttons/slider_default.png");
	joystick_deadzone_lb = new WidgetLabel();

	tabControl = new WidgetTabControl(6);
	tabControl->setMainArea(((VIEW_W - FRAME_W)/2)+3, (VIEW_H - FRAME_H)/2, FRAME_W, FRAME_H);
	frame = tabControl->getContentArea();

	// Define the header.
	tabControl->setTabTitle(0, msg->get("Video"));
	tabControl->setTabTitle(1, msg->get("Audio"));
	tabControl->setTabTitle(2, msg->get("Interface"));
	tabControl->setTabTitle(3, msg->get("Input"));
	tabControl->setTabTitle(4, msg->get("Keybindings"));
	tabControl->setTabTitle(5, msg->get("Mods"));
	tabControl->updateHeader();

	input_confirm = new MenuConfirm(msg->get("Clear"),msg->get("Assign: "));
	defaults_confirm = new MenuConfirm(msg->get("Defaults"),msg->get("Reset ALL settings?"));

	// Allocate KeyBindings
	for (int i = 0; i < inpt->key_count; i++) {
		keybinds_lb.push_back(new WidgetLabel());
		keybinds_lb[i]->set(inpt->binding_name[i]);
		keybinds_lb[i]->setJustify(JUSTIFY_RIGHT);
	}
	for (int i = 0; i < inpt->key_count * 3; i++) {
		keybinds_btn.push_back(new WidgetButton("images/menus/buttons/button_default.png"));
	}

	key_count = keybinds_btn.size()/3;

	// Allocate resolution list box
	if (getVideoModes() < 1) logError("Unable to get resolutions list!\n");
	resolution_lstb = new WidgetListBox(10, "images/menus/buttons/listbox_default.png");
	resolution_lstb->can_deselect = false;

	// Allocate Languages ListBox
	int langCount = getLanguagesNumber();
	language_ISO = std::vector<std::string>();
	language_full = std::vector<std::string>();
	language_ISO.resize(langCount);
	language_full.resize(langCount);
	language_lstb = new WidgetListBox(10, "images/menus/buttons/listbox_default.png");
	language_lstb->can_deselect = false;

	readConfig();

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

	fullscreen = FULLSCREEN;
	hwsurface = HWSURFACE;
	doublebuf = DOUBLEBUF;

	input_confirm_ticks = 0;

	// child widgets; aka everything except the tabs, OK, Defaults, and Cancel
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

	addChildWidget(music_volume_sl, AUDIO_TAB);
	addChildWidget(music_volume_lb, AUDIO_TAB);
	addChildWidget(sound_volume_sl, AUDIO_TAB);
	addChildWidget(sound_volume_lb, AUDIO_TAB);

	addChildWidget(combat_text_cb, INTERFACE_TAB);
	addChildWidget(combat_text_lb, INTERFACE_TAB);
	addChildWidget(show_fps_cb, INTERFACE_TAB);
	addChildWidget(show_fps_lb, INTERFACE_TAB);
	addChildWidget(show_hotkeys_cb, INTERFACE_TAB);
	addChildWidget(show_hotkeys_lb, INTERFACE_TAB);
	addChildWidget(colorblind_cb, INTERFACE_TAB);
	addChildWidget(colorblind_lb, INTERFACE_TAB);
	addChildWidget(hardware_cursor_cb, INTERFACE_TAB);
	addChildWidget(hardware_cursor_lb, INTERFACE_TAB);
	addChildWidget(dev_mode_cb, INTERFACE_TAB);
	addChildWidget(dev_mode_lb, INTERFACE_TAB);
	addChildWidget(language_lstb, INTERFACE_TAB);
	addChildWidget(language_lb, INTERFACE_TAB);

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

	addChildWidget(activemods_lstb, MODS_TAB);
	addChildWidget(activemods_lb, MODS_TAB);
	addChildWidget(inactivemods_lstb, MODS_TAB);
	addChildWidget(inactivemods_lb, MODS_TAB);
	addChildWidget(activemods_shiftup_btn, MODS_TAB);
	addChildWidget(activemods_shiftdown_btn, MODS_TAB);
	addChildWidget(activemods_deactivate_btn, MODS_TAB);
	addChildWidget(inactivemods_activate_btn, MODS_TAB);

	// Set up tab list for keyboard navigation
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

	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		input_scrollbox->addChildWidget(keybinds_btn[i]);
	}
}

void GameStateConfig::readConfig () {
	//Load the menu configuration from file

	int offset_x = 0;
	int offset_y = 0;

	FileParser infile;
	if (infile.open("menus/config.txt")) {
		while (infile.next()) {

			int x1 = popFirstInt(infile.val);
			int y1 = popFirstInt(infile.val);
			int x2 = popFirstInt(infile.val);
			int y2 = popFirstInt(infile.val);

			int keybind_num = -1;

			if (infile.key == "listbox_scrollbar_offset") {
				activemods_lstb->scrollbar_offset = x1;
				inactivemods_lstb->scrollbar_offset = x1;
				joystick_device_lstb->scrollbar_offset = x1;
				resolution_lstb->scrollbar_offset = x1;
				language_lstb->scrollbar_offset = x1;
			}
			//checkboxes
			else if (infile.key == "fullscreen") {
				placeLabeledWidget(fullscreen_lb, fullscreen_cb, x1, y1, x2, y2, msg->get("Full Screen Mode"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "mouse_move") {
				placeLabeledWidget(mouse_move_lb, mouse_move_cb, x1, y1, x2, y2, msg->get("Move hero using mouse"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "combat_text") {
				placeLabeledWidget(combat_text_lb, combat_text_cb, x1, y1, x2, y2, msg->get("Show combat text"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "hwsurface") {
				placeLabeledWidget(hwsurface_lb, hwsurface_cb, x1, y1, x2, y2, msg->get("Hardware surfaces"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "doublebuf") {
				placeLabeledWidget(doublebuf_lb, doublebuf_cb, x1, y1, x2, y2, msg->get("Double buffering"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "enable_joystick") {
				placeLabeledWidget(enable_joystick_lb, enable_joystick_cb, x1, y1, x2, y2, msg->get("Use joystick"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "change_gamma") {
				placeLabeledWidget(change_gamma_lb, change_gamma_cb, x1, y1, x2, y2, msg->get("Allow changing gamma"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "mouse_aim") {
				placeLabeledWidget(mouse_aim_lb, mouse_aim_cb, x1, y1, x2, y2, msg->get("Mouse aim"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "no_mouse") {
				placeLabeledWidget(no_mouse_lb, no_mouse_cb, x1, y1, x2, y2, msg->get("Do not use mouse"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "show_fps") {
				placeLabeledWidget(show_fps_lb, show_fps_cb, x1, y1, x2, y2, msg->get("Show FPS"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "show_hotkeys") {
				placeLabeledWidget(show_hotkeys_lb, show_hotkeys_cb, x1, y1, x2, y2, msg->get("Show Hotkeys Labels"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "colorblind") {
				placeLabeledWidget(colorblind_lb, colorblind_cb, x1, y1, x2, y2, msg->get("Colorblind Mode"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "hardware_cursor") {
				placeLabeledWidget(hardware_cursor_lb, hardware_cursor_cb, x1, y1, x2, y2, msg->get("Hardware mouse cursor"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "dev_mode") {
				placeLabeledWidget(dev_mode_lb, dev_mode_cb, x1, y1, x2, y2, msg->get("Developer Mode"), JUSTIFY_RIGHT);
			}
			//sliders
			else if (infile.key == "music_volume") {
				placeLabeledWidget(music_volume_lb, music_volume_sl, x1, y1, x2, y2, msg->get("Music Volume"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "sound_volume") {
				placeLabeledWidget(sound_volume_lb, sound_volume_sl, x1, y1, x2, y2, msg->get("Sound Volume"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "gamma") {
				placeLabeledWidget(gamma_lb, gamma_sl, x1, y1, x2, y2, msg->get("Gamma"), JUSTIFY_RIGHT);
			}
			else if (infile.key == "joystick_deadzone") {
				placeLabeledWidget(joystick_deadzone_lb, joystick_deadzone_sl, x1, y1, x2, y2, msg->get("Joystick Deadzone"), JUSTIFY_RIGHT);
			}
			//listboxes
			else if (infile.key == "resolution") {
				placeLabeledWidget(resolution_lb, resolution_lstb, x1, y1, x2, y2, msg->get("Resolution"));
			}
			else if (infile.key == "activemods") {
				placeLabeledWidget(activemods_lb, activemods_lstb, x1, y1, x2, y2, msg->get("Active Mods"));
			}
			else if (infile.key == "inactivemods") {
				placeLabeledWidget(inactivemods_lb, inactivemods_lstb, x1, y1, x2, y2, msg->get("Available Mods"));
			}
			else if (infile.key == "joystick_device") {
				placeLabeledWidget(joystick_device_lb, joystick_device_lstb, x1, y1, x2, y2, msg->get("Joystick"));

				for(int i = 0; i < SDL_NumJoysticks(); i++) {
					std::string joystick_name = inpt->getJoystickName(i);
					if (joystick_name != "")
						joystick_device_lstb->append(joystick_name, joystick_name);
				}
			}
			else if (infile.key == "language") {
				placeLabeledWidget(language_lb, language_lstb, x1, y1, x2, y2, msg->get("Language"));
			}
			// buttons begin
			else if (infile.key == "cancel") keybind_num = CANCEL;
			else if (infile.key == "accept") keybind_num = ACCEPT;
			else if (infile.key == "up") keybind_num = UP;
			else if (infile.key == "down") keybind_num = DOWN;
			else if (infile.key == "left") keybind_num = LEFT;
			else if (infile.key == "right") keybind_num = RIGHT;
			else if (infile.key == "bar1") keybind_num = BAR_1;
			else if (infile.key == "bar2") keybind_num = BAR_2;
			else if (infile.key == "bar3") keybind_num = BAR_3;
			else if (infile.key == "bar4") keybind_num = BAR_4;
			else if (infile.key == "bar5") keybind_num = BAR_5;
			else if (infile.key == "bar6") keybind_num = BAR_6;
			else if (infile.key == "bar7") keybind_num = BAR_7;
			else if (infile.key == "bar8") keybind_num = BAR_8;
			else if (infile.key == "bar9") keybind_num = BAR_9;
			else if (infile.key == "bar0") keybind_num = BAR_0;
			else if (infile.key == "main1") keybind_num = MAIN1;
			else if (infile.key == "main2") keybind_num = MAIN2;
			else if (infile.key == "character") keybind_num = CHARACTER;
			else if (infile.key == "inventory") keybind_num = INVENTORY;
			else if (infile.key == "powers") keybind_num = POWERS;
			else if (infile.key == "log") keybind_num = LOG;
			else if (infile.key == "ctrl") keybind_num = CTRL;
			else if (infile.key == "shift") keybind_num = SHIFT;
			else if (infile.key == "delete") keybind_num = DEL;
			else if (infile.key == "actionbar") keybind_num = ACTIONBAR;
			else if (infile.key == "actionbar_back") keybind_num = ACTIONBAR_BACK;
			else if (infile.key == "actionbar_forward") keybind_num = ACTIONBAR_FORWARD;
			else if (infile.key == "actionbar_use") keybind_num = ACTIONBAR_USE;
			else if (infile.key == "developer_menu") keybind_num = DEVELOPER_MENU;
			// buttons end

			else if (infile.key == "hws_note") {
				hws_note_lb->setX(frame.x + x1);
				hws_note_lb->setY(frame.y + y1);
				hws_note_lb->set(msg->get("Disable for performance"));
			}
			else if (infile.key == "dbuf_note") {
				dbuf_note_lb->setX(frame.x + x1);
				dbuf_note_lb->setY(frame.y + y1);
				dbuf_note_lb->set(msg->get("Disable for performance"));
			}
			else if (infile.key == "test_note") {
				test_note_lb->setX(frame.x + x1);
				test_note_lb->setY(frame.y + y1);
				test_note_lb->set(msg->get("Experimental"));
			}
			else if (infile.key == "handheld_note") {
				handheld_note_lb->setX(frame.x + x1);
				handheld_note_lb->setY(frame.y + y1);
				handheld_note_lb->set(msg->get("For handheld devices"));
			}
			//buttons
			else if (infile.key == "activemods_shiftup") {
				activemods_shiftup_btn->pos.x = frame.x + x1;
				activemods_shiftup_btn->pos.y = frame.y + y1;
				activemods_shiftup_btn->refresh();
			}
			else if (infile.key == "activemods_shiftdown") {
				activemods_shiftdown_btn->pos.x = frame.x + x1;
				activemods_shiftdown_btn->pos.y = frame.y + y1;
				activemods_shiftdown_btn->refresh();
			}
			else if (infile.key == "activemods_deactivate") {
				activemods_deactivate_btn->label = msg->get("<< Disable");
				activemods_deactivate_btn->pos.x = frame.x + x1;
				activemods_deactivate_btn->pos.y = frame.y + y1;
				activemods_deactivate_btn->refresh();
			}
			else if (infile.key == "inactivemods_activate") {
				inactivemods_activate_btn->label = msg->get("Enable >>");
				inactivemods_activate_btn->pos.x = frame.x + x1;
				inactivemods_activate_btn->pos.y = frame.y + y1;
				inactivemods_activate_btn->refresh();
			}
			else if (infile.key == "secondary_offset") {
				offset_x = x1;
				offset_y = y1;
			}
			else if (infile.key == "keybinds_bg_color") {
				// background color for keybinds scrollbox
				scrollpane_color.r = x1;
				scrollpane_color.g = y1;
				scrollpane_color.b = x2;
			}
			else if (infile.key == "scrollpane") {
				scrollpane.x = x1;
				scrollpane.y = y1;
				scrollpane.w = x2;
				scrollpane.h = y2;
			}
			else if (infile.key == "scrollpane_contents") {
				scrollpane_contents = x1;
			}

			if (keybind_num > -1 && (unsigned)keybind_num < keybinds_lb.size() && (unsigned)keybind_num < keybinds_btn.size()) {
				//keybindings
				keybinds_lb[keybind_num]->setX(x1);
				keybinds_lb[keybind_num]->setY(y1);
				keybinds_btn[keybind_num]->pos.x = x2;
				keybinds_btn[keybind_num]->pos.y = y2;
			}

		}
		infile.close();
	}

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
		keybinds_btn[i]->pos.x = keybinds_btn[i-key_count]->pos.x + offset_x;
		keybinds_btn[i]->pos.y = keybinds_btn[i-key_count]->pos.y + offset_y;
	}

	// Set positions of joystick bindings
	for (unsigned int i = key_count*2; i < keybinds_btn.size(); i++) {
		keybinds_btn[i]->pos.x = keybinds_btn[i-(key_count*2)]->pos.x + (offset_x*2);
		keybinds_btn[i]->pos.y = keybinds_btn[i-(key_count*2)]->pos.y + (offset_y*2);
	}
}

void GameStateConfig::update () {
	if (FULLSCREEN) fullscreen_cb->Check();
	else fullscreen_cb->unCheck();
	if (AUDIO) {
		music_volume_sl->set(0,128,MUSIC_VOLUME);
		Mix_VolumeMusic(MUSIC_VOLUME);
		sound_volume_sl->set(0,128,SOUND_VOLUME);
		Mix_Volume(-1, SOUND_VOLUME);
	}
	else {
		music_volume_sl->set(0,128,0);
		sound_volume_sl->set(0,128,0);
	}
	if (MOUSE_MOVE) mouse_move_cb->Check();
	else mouse_move_cb->unCheck();
	if (COMBAT_TEXT) combat_text_cb->Check();
	else combat_text_cb->unCheck();
	if (HWSURFACE) hwsurface_cb->Check();
	else hwsurface_cb->unCheck();
	if (DOUBLEBUF) doublebuf_cb->Check();
	else doublebuf_cb->unCheck();
	if (ENABLE_JOYSTICK) enable_joystick_cb->Check();
	else enable_joystick_cb->unCheck();
	if (CHANGE_GAMMA) change_gamma_cb->Check();
	else {
		change_gamma_cb->unCheck();
		GAMMA = 1.0;
		gamma_sl->enabled = false;
	}
	gamma_sl->set(5,20,(int)(GAMMA*10.0));
	render_device->setGamma(GAMMA);

	if (MOUSE_AIM) mouse_aim_cb->Check();
	else mouse_aim_cb->unCheck();
	if (NO_MOUSE) no_mouse_cb->Check();
	else no_mouse_cb->unCheck();
	if (SHOW_FPS) show_fps_cb->Check();
	else show_fps_cb->unCheck();
	if (SHOW_HOTKEYS) show_hotkeys_cb->Check();
	else show_hotkeys_cb->unCheck();
	if (COLORBLIND) colorblind_cb->Check();
	else colorblind_cb->unCheck();
	if (HARDWARE_CURSOR) hardware_cursor_cb->Check();
	else hardware_cursor_cb->unCheck();
	if (DEV_MODE) dev_mode_cb->Check();
	else dev_mode_cb->unCheck();

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

	SDL_Init(SDL_INIT_JOYSTICK);
	if (ENABLE_JOYSTICK && SDL_NumJoysticks() > 0) {
		SDL_JoystickClose(joy);
		joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
		joystick_device_lstb->selected[JOYSTICK_DEVICE] = true;
	}
	joystick_device_lstb->refresh();

	joystick_deadzone_sl->set(0,32768,JOY_DEADZONE);

	if (!getLanguagesList()) logError("Unable to get languages list!\n");
	for (int i=0; i < getLanguagesNumber(); i++) {
		language_lstb->append(language_full[i],"");
		if (language_ISO[i] == LANGUAGE) language_lstb->selected[i] = true;
	}
	language_lstb->refresh();

	activemods_lstb->refresh();
	inactivemods_lstb->refresh();

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

void GameStateConfig::logic () {
	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (input_scrollbox->in_focus && !input_confirm->visible) {
			tabControl->setActiveTab(KEYBINDS_TAB);
			break;
		}
		else if (child_widget[i]->in_focus) {
			tabControl->setActiveTab(optiontab[i]);
			break;
		}
	}

	check_resolution = true;

	std::string resolution_value = resolution_lstb->getValue();
	int width = popFirstInt(resolution_value, 'x');
	int height = popFirstInt(resolution_value, 'x');

	// In case of a custom resolution, the listbox might have nothing selected
	// So we just use whatever the current view area is
	if (width == 0 || height == 0) {
		width = VIEW_W;
		height = VIEW_H;
	}

	if (defaults_confirm->visible) {
		defaults_confirm->logic();
		if (defaults_confirm->confirmClicked) {
			check_resolution = false;
			FULLSCREEN = 0;
			loadDefaults();
			loadMiscSettings();
			inpt->defaultQwertyKeyBindings();
			inpt->defaultJoystickBindings();
			delete msg;
			msg = new MessageEngine();
			update();
			defaults_confirm->visible = false;
			defaults_confirm->confirmClicked = false;
		}
	}

	if (!input_confirm->visible && !defaults_confirm->visible) {
		tabControl->logic();
		tablist.logic();

		// Ok/Cancel Buttons
		if (ok_button->checkClick()) {
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
			delete requestedGameState;
			requestedGameState = new GameStateResolution(width, height, fullscreen, hwsurface, doublebuf);

			// We need to be carful here. GameStateConfig has been deconstructed,
			// so we need to avoid accessing any dynamically allocated objects
			return;
		}
		else if (defaults_button->checkClick()) {
			defaults_confirm->visible = true;
		}
		else if (cancel_button->checkClick() || (inpt->pressing[CANCEL] && !inpt->lock[CANCEL])) {
			inpt->lock[CANCEL] = true;
			check_resolution = false;
			loadSettings();
			loadMiscSettings();
			inpt->loadKeyBindings();
			delete msg;
			msg = new MessageEngine();
			update();
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
		}
	}

	int active_tab = tabControl->getActiveTab();

	// tab 0 (video)
	if (active_tab == VIDEO_TAB && !defaults_confirm->visible) {
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
			; // nothing to do here: resolution value changes next frame.
		}
		else if (CHANGE_GAMMA) {
			gamma_sl->enabled = true;
			if (gamma_sl->checkClick()) {
				GAMMA=(gamma_sl->getValue())*0.1f;
				render_device->setGamma(GAMMA);
			}
		}
	}
	// tab 1 (audio)
	else if (active_tab == AUDIO_TAB && !defaults_confirm->visible) {
		if (AUDIO) {
			if (music_volume_sl->checkClick()) {
				if (MUSIC_VOLUME == 0)
					reload_music = true;
				MUSIC_VOLUME=music_volume_sl->getValue();
				Mix_VolumeMusic(MUSIC_VOLUME);
			}
			else if (sound_volume_sl->checkClick()) {
				SOUND_VOLUME=sound_volume_sl->getValue();
				Mix_Volume(-1, SOUND_VOLUME);
			}
		}
	}
	// tab 2 (interface)
	else if (active_tab == INTERFACE_TAB && !defaults_confirm->visible) {
		if (combat_text_cb->checkClick()) {
			if (combat_text_cb->isChecked()) COMBAT_TEXT=true;
			else COMBAT_TEXT=false;
		}
		else if (language_lstb->checkClick()) {
			LANGUAGE = language_ISO[language_lstb->getSelected()];
			delete msg;
			msg = new MessageEngine();
		}
		else if (show_fps_cb->checkClick()) {
			if (show_fps_cb->isChecked()) SHOW_FPS=true;
			else SHOW_FPS=false;
		}
		else if (show_hotkeys_cb->checkClick()) {
			if (show_hotkeys_cb->isChecked()) SHOW_HOTKEYS=true;
			else SHOW_HOTKEYS=false;
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
	}
	// tab 3 (input)
	else if (active_tab == INPUT_TAB && !defaults_confirm->visible) {
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
	// tab 4 (keybindings)
	else if (active_tab == KEYBINDS_TAB && !defaults_confirm->visible) {
		if (input_confirm->visible) {
			input_confirm->logic();
			scanKey(input_key);
			input_confirm_ticks--;
			if (input_confirm_ticks == 0) input_confirm->visible = false;
		}
		else {
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
	}
	// tab 5 (mods)
	else if (active_tab == MODS_TAB && !defaults_confirm->visible) {
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
}

void GameStateConfig::render () {
	if (requestedGameState != NULL) {
		// we're in the process of switching game states, so skip rendering
		return;
	}

	int tabheight = tabControl->getTabHeight();
	Rect	pos;
	pos.x = (VIEW_W-FRAME_W)/2;
	pos.y = (VIEW_H-FRAME_H)/2 + tabheight - tabheight/16;

	if (background) {
		background->setDest(pos);
		render_device->render(background);
	}

	tabControl->render();

	// render OK/Defaults/Cancel buttons
	ok_button->render();
	cancel_button->render();
	defaults_button->render();

	int active_tab = tabControl->getActiveTab();

	// render keybindings tab
	if (active_tab == 4) {
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

	if (input_confirm->visible) input_confirm->render();
	if (defaults_confirm->visible) defaults_confirm->render();

	// Get tooltips for listboxes
	// This isn't very elegant right now
	// In the future, we'll want to get tooltips for all widget types
	TooltipData tip_new;
	if (active_tab == 0 && tip_new.isEmpty()) tip_new = resolution_lstb->checkTooltip(inpt->mouse);
	if (active_tab == 2 && tip_new.isEmpty()) tip_new = language_lstb->checkTooltip(inpt->mouse);
	if (active_tab == 3 && tip_new.isEmpty()) tip_new = joystick_device_lstb->checkTooltip(inpt->mouse);
	if (active_tab == 5 && tip_new.isEmpty()) tip_new = activemods_lstb->checkTooltip(inpt->mouse);
	if (active_tab == 5 && tip_new.isEmpty()) tip_new = inactivemods_lstb->checkTooltip(inpt->mouse);

	if (!tip_new.isEmpty()) {
		if (!tip_new.compare(&tip_buf)) {
			tip_buf.clear();
			tip_buf = tip_new;
		}
		tip->render(tip_buf, inpt->mouse, STYLE_FLOAT);
	}

}

int GameStateConfig::getVideoModes() {
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

bool GameStateConfig::getLanguagesList() {
	FileParser infile;
	if (infile.open("engine/languages.txt")) {
		unsigned int i=0;
		while (infile.next()) {
			language_ISO[i] = infile.key;
			language_full[i] = infile.nextValue();
			i += 1;
		}
		infile.close();
	}

	return true;
}

int GameStateConfig::getLanguagesNumber() {
	int languages_num = 0;
	FileParser infile;
	if (infile.open("engine/languages.txt")) {
		while (infile.next()) {
			languages_num += 1;
		}
		infile.close();
	}

	return languages_num;
}

void GameStateConfig::refreshFont() {
	delete font;
	font = new FontEngine();
}

/**
 * Activate mods
 */
void GameStateConfig::enableMods() {
	for (int i=0; i<inactivemods_lstb->getSize(); i++) {
		if (inactivemods_lstb->selected[i]) {
			activemods_lstb->append(inactivemods_lstb->getValue(i),inactivemods_lstb->getTooltip(i));
			inactivemods_lstb->remove(i);
			i--;
		}
	}
}

/**
 * Deactivate mods
 */
void GameStateConfig::disableMods() {
	for (int i=0; i<activemods_lstb->getSize(); i++) {
		if (activemods_lstb->selected[i] && activemods_lstb->getValue(i) != FALLBACK_MOD) {
			inactivemods_lstb->append(activemods_lstb->getValue(i),activemods_lstb->getTooltip(i));
			activemods_lstb->remove(i);
			i--;
		}
	}
}

/**
 * Save new mods list. Return true if modlist was changed. Else return false
 */
bool GameStateConfig::setMods() {
	vector<Mod> temp_list = mods->mod_list;
	mods->mod_list.clear();
	mods->mod_list.push_back(mods->loadMod(FALLBACK_MOD));
	for (int i=0; i<activemods_lstb->getSize(); i++) {
		if (activemods_lstb->getValue(i) != "")
			mods->mod_list.push_back(mods->loadMod(activemods_lstb->getValue(i)));
	}
	mods->applyDepends();
	ofstream outfile;
	outfile.open((PATH_CONF + "mods.txt").c_str(), ios::out);

	if (outfile.is_open()) {

		outfile<<"# Mods lower on the list will overwrite data in the entries higher on the list"<<"\n";
		outfile<<"\n";

		for (unsigned int i = 0; i < mods->mod_list.size(); i++) {
			if (mods->mod_list[i].name != FALLBACK_MOD)
				outfile<<mods->mod_list[i].name<<"\n";
		}
	}
	if (outfile.bad()) logError("Unable to save mod list into file. No write access or disk is full!\n");
	outfile.close();
	outfile.clear();
	if (mods->mod_list != temp_list) return true;
	else return false;
}

/**
 * Scan key binding
 */
void GameStateConfig::scanKey(int button) {
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

void GameStateConfig::cleanup() {
	if (background) {
		delete background;
		background = NULL;
	}
	tip_buf.clear();
	if (tip != NULL) {
		delete tip;
		tip = NULL;
	}
	if (tabControl != NULL) {
		delete tabControl;
		tabControl = NULL;
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
	if (input_scrollbox != NULL) {
		delete input_scrollbox;
		input_scrollbox = NULL;
	}
	if (input_confirm != NULL) {
		delete input_confirm;
		input_confirm = NULL;
	}
	if (defaults_confirm != NULL) {
		delete defaults_confirm;
		defaults_confirm = NULL;
	}

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

	language_ISO.clear();
	language_full.clear();
}

GameStateConfig::~GameStateConfig() {
	cleanup();
}

void GameStateConfig::placeLabeledWidget(WidgetLabel *lb, Widget *w, int x1, int y1, int x2, int y2, std::string const& str, int justify) {
	w->pos.x = frame.x + x2;
	w->pos.y = frame.y + y2;

	lb->setX(frame.x + x1);
	lb->setY(frame.y + y1);
	lb->set(str);
	lb->setJustify(justify);
}

std::string GameStateConfig::createModTooltip(Mod *mod) {
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
		if (mod->game != "") {
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
		if (mod->depends.size() > 0) {
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

void GameStateConfig::addChildWidget(Widget *w, int tab) {
	child_widget.push_back(w);
	optiontab.push_back(tab);
}

