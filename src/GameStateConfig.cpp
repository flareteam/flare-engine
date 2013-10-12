/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller

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

using namespace std;

bool rescompare(const SDL_Rect &r1, const SDL_Rect &r2) {
	if (r1.w == r2.w) return r1.h > r2.h;
	return r1.w > r2.w;
}

GameStateConfig::GameStateConfig ()
	: GameState()
	, child_widget()
	, ok_button(NULL)
	, defaults_button(NULL)
	, cancel_button(NULL)
	, tip_buf()
	, input_key(0)
	, check_resolution(true) {
	background = loadGraphicSurface("images/menus/config.png");

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

	mods_total = mods->mod_dirs.size();
	// Remove active mods from the available mods list
	for (unsigned int i = 0; i<mods->mod_list.size(); i++) {
		for (unsigned int j = 0; j<mods->mod_dirs.size(); j++) {
			if (mods->mod_list[i] == mods->mod_dirs[j] || FALLBACK_MOD == mods->mod_dirs[j]) mods->mod_dirs[j].erase();
		}
	}

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
	texture_quality_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	texture_quality_lb = new WidgetLabel();
	change_gamma_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	change_gamma_lb = new WidgetLabel();
	animated_tiles_cb = new WidgetCheckBox("images/menus/buttons/checkbox_default.png");
	animated_tiles_lb = new WidgetLabel();
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
	music_volume_sl = new WidgetSlider("images/menus/buttons/slider_default.png");
	music_volume_lb = new WidgetLabel();
	sound_volume_sl = new WidgetSlider("images/menus/buttons/slider_default.png");
	sound_volume_lb = new WidgetLabel();
	gamma_sl = new WidgetSlider("images/menus/buttons/slider_default.png");
	gamma_lb = new WidgetLabel();
	resolution_lb = new WidgetLabel();
	activemods_lstb = new WidgetListBox(mods_total, 10, "images/menus/buttons/listbox_default.png");
	activemods_lb = new WidgetLabel();
	inactivemods_lstb = new WidgetListBox(mods_total, 10, "images/menus/buttons/listbox_default.png");
	inactivemods_lb = new WidgetLabel();
	joystick_device_lstb = new WidgetListBox(SDL_NumJoysticks(), 10, "images/menus/buttons/listbox_default.png");
	joystick_device_lb = new WidgetLabel();
	language_lb = new WidgetLabel();
	hws_note_lb = new WidgetLabel();
	dbuf_note_lb = new WidgetLabel();
	anim_tiles_note_lb = new WidgetLabel();
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

	input_confirm = new MenuConfirm("",msg->get("Assign: "));
	defaults_confirm = new MenuConfirm(msg->get("Defaults"),msg->get("Reset ALL settings?"));
	resolution_confirm = new MenuConfirm(msg->get("OK"),msg->get("Use this resolution?"));

	// Allocate KeyBindings
	for (unsigned int i = 0; i < 29; i++) {
		settings_lb[i] = new WidgetLabel();
		settings_lb[i]->set(inpt->binding_name[i]);
		settings_lb[i]->setJustify(JUSTIFY_RIGHT);
	}
	for (unsigned int i = 0; i < 58; i++) {
		settings_key[i] = new WidgetButton("images/menus/buttons/button_default.png");
	}

	// Allocate resolution list box
	int resolutions = getVideoModes();
	if (resolutions < 1) fprintf(stderr, "Unable to get resolutions list!\n");
	resolution_lstb = new WidgetListBox(resolutions, 10, "images/menus/buttons/listbox_default.png");
	resolution_lstb->can_deselect = false;

	// Allocate Languages ListBox
	int langCount = getLanguagesNumber();
	language_ISO = std::vector<std::string>();
	language_full = std::vector<std::string>();
	language_ISO.resize(langCount);
	language_full.resize(langCount);
	language_lstb = new WidgetListBox(langCount, 10, "images/menus/buttons/listbox_default.png");
	language_lstb->can_deselect = false;

	readConfig();

	// Finish Mods ListBoxes setup
	activemods_lstb->multi_select = true;
	for (unsigned int i = 0; i < mods->mod_list.size() ; i++) {
		if (mods->mod_list[i] != FALLBACK_MOD)
			activemods_lstb->append(mods->mod_list[i],"");
	}
	child_widget.push_back(activemods_lstb);
	optiontab[child_widget.size()-1] = 5;

	inactivemods_lstb->multi_select = true;
	for (unsigned int i = 0; i < mods->mod_dirs.size(); i++) {
		inactivemods_lstb->append(mods->mod_dirs[i],"");
	}
	child_widget.push_back(inactivemods_lstb);
	optiontab[child_widget.size()-1] = 5;

	// Save the current resolution in case we want to revert back to it
	old_view_w = VIEW_W;
	old_view_h = VIEW_H;

	resolution_confirm_ticks = 0;
	input_confirm_ticks = 0;

	// Set up tab list
	tablist.add(ok_button);
	tablist.add(defaults_button);
	tablist.add(cancel_button);
	tablist.add(fullscreen_cb);
	tablist.add(hwsurface_cb);
	tablist.add(doublebuf_cb);
	tablist.add(change_gamma_cb);
	tablist.add(gamma_sl);
	tablist.add(texture_quality_cb);
	tablist.add(animated_tiles_cb);
	tablist.add(resolution_lstb);

	tablist.add(music_volume_sl);
	tablist.add(sound_volume_sl);

	tablist.add(combat_text_cb);
	tablist.add(show_fps_cb);
	tablist.add(show_hotkeys_cb);
	tablist.add(colorblind_cb);
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

	for (unsigned int i = 0; i < 58; i++) {
		input_scrollbox->addChildWidget(settings_key[i]);
	}
}

void GameStateConfig::readConfig () {
	//Load the menu configuration from file

	int offset_x = 0;
	int offset_y = 0;

	FileParser infile;
	if (infile.open("menus/config.txt")) {
		while (infile.next()) {

			infile.val = infile.val + ',';
			int x1 = eatFirstInt(infile.val, ',');
			int y1 = eatFirstInt(infile.val, ',');
			int x2 = eatFirstInt(infile.val, ',');
			int y2 = eatFirstInt(infile.val, ',');

			int setting_num = -1;

			if (infile.key == "listbox_scrollbar_offset") {
				activemods_lstb->scrollbar_offset = x1;
				inactivemods_lstb->scrollbar_offset = x1;
				joystick_device_lstb->scrollbar_offset = x1;
				resolution_lstb->scrollbar_offset = x1;
				language_lstb->scrollbar_offset = x1;
			}
			//checkboxes
			else if (infile.key == "fullscreen") {
				placeLabeledCheckbox( fullscreen_lb, fullscreen_cb, x1, y1, x2, y2, msg->get("Full Screen Mode"), 0);
			}
			else if (infile.key == "mouse_move") {
				placeLabeledCheckbox( mouse_move_lb, mouse_move_cb, x1, y1, x2, y2, msg->get("Move hero using mouse"), 3);
			}
			else if (infile.key == "combat_text") {
				placeLabeledCheckbox( combat_text_lb, combat_text_cb, x1, y1, x2, y2, msg->get("Show combat text"), 2);
			}
			else if (infile.key == "hwsurface") {
				placeLabeledCheckbox( hwsurface_lb, hwsurface_cb, x1, y1, x2, y2, msg->get("Hardware surfaces"), 0);
			}
			else if (infile.key == "doublebuf") {
				placeLabeledCheckbox( doublebuf_lb, doublebuf_cb, x1, y1, x2, y2, msg->get("Double buffering"), 0);
			}
			else if (infile.key == "enable_joystick") {
				placeLabeledCheckbox( enable_joystick_lb, enable_joystick_cb, x1, y1, x2, y2, msg->get("Use joystick"), 3);
			}
			else if (infile.key == "texture_quality") {
				placeLabeledCheckbox( texture_quality_lb, texture_quality_cb, x1, y1, x2, y2, msg->get("High Quality Textures"), 0);
			}
			else if (infile.key == "change_gamma") {
				placeLabeledCheckbox( change_gamma_lb, change_gamma_cb, x1, y1, x2, y2, msg->get("Allow changing gamma"), 0);
			}
			else if (infile.key == "animated_tiles") {
				placeLabeledCheckbox( animated_tiles_lb, animated_tiles_cb, x1, y1, x2, y2, msg->get("Animated tiles"), 0);
			}
			else if (infile.key == "mouse_aim") {
				placeLabeledCheckbox( mouse_aim_lb, mouse_aim_cb, x1, y1, x2, y2, msg->get("Mouse aim"), 3);
			}
			else if (infile.key == "no_mouse") {
				placeLabeledCheckbox( no_mouse_lb, no_mouse_cb, x1, y1, x2, y2, msg->get("Do not use mouse"), 3);
			}
			else if (infile.key == "show_fps") {
				placeLabeledCheckbox( show_fps_lb, show_fps_cb, x1, y1, x2, y2, msg->get("Show FPS"), 2);
			}
			else if (infile.key == "show_hotkeys") {
				placeLabeledCheckbox( show_hotkeys_lb, show_hotkeys_cb, x1, y1, x2, y2, msg->get("Show Hotkeys Labels"), 2);
			}
			else if (infile.key == "colorblind") {
				placeLabeledCheckbox( colorblind_lb, colorblind_cb, x1, y1, x2, y2, msg->get("Colorblind Mode"), 2);
			}
			//sliders
			else if (infile.key == "music_volume") {
				music_volume_sl->pos.x = frame.x + x2;
				music_volume_sl->pos.y = frame.y + y2;
				child_widget.push_back(music_volume_sl);
				optiontab[child_widget.size()-1] = 1;

				music_volume_lb->setX(frame.x + x1);
				music_volume_lb->setY(frame.y + y1);
				music_volume_lb->set(msg->get("Music Volume"));
				music_volume_lb->setJustify(JUSTIFY_RIGHT);
				child_widget.push_back(music_volume_lb);
				optiontab[child_widget.size()-1] = 1;
			}
			else if (infile.key == "sound_volume") {
				sound_volume_sl->pos.x = frame.x + x2;
				sound_volume_sl->pos.y = frame.y + y2;
				child_widget.push_back(sound_volume_sl);
				optiontab[child_widget.size()-1] = 1;

				sound_volume_lb->setX(frame.x + x1);
				sound_volume_lb->setY(frame.y + y1);
				sound_volume_lb->set(msg->get("Sound Volume"));
				sound_volume_lb->setJustify(JUSTIFY_RIGHT);
				child_widget.push_back(sound_volume_lb);
				optiontab[child_widget.size()-1] = 1;
			}
			else if (infile.key == "gamma") {
				gamma_sl->pos.x = frame.x + x2;
				gamma_sl->pos.y = frame.y + y2;
				child_widget.push_back(gamma_sl);
				optiontab[child_widget.size()-1] = 0;

				gamma_lb->setX(frame.x + x1);
				gamma_lb->setY(frame.y + y1);
				gamma_lb->set(msg->get("Gamma"));
				gamma_lb->setJustify(JUSTIFY_RIGHT);
				child_widget.push_back(gamma_lb);
				optiontab[child_widget.size()-1] = 0;
			}
			else if (infile.key == "joystick_deadzone") {
				joystick_deadzone_sl->pos.x = frame.x + x2;
				joystick_deadzone_sl->pos.y = frame.y + y2;
				child_widget.push_back(joystick_deadzone_sl);
				optiontab[child_widget.size()-1] = 3;

				joystick_deadzone_lb->setX(frame.x + x1);
				joystick_deadzone_lb->setY(frame.y + y1);
				joystick_deadzone_lb->set(msg->get("Joystick Deadzone"));
				joystick_deadzone_lb->setJustify(JUSTIFY_RIGHT);
				child_widget.push_back(joystick_deadzone_lb);
				optiontab[child_widget.size()-1] = 3;
			}
			//listboxes
			else if (infile.key == "resolution") {
				resolution_lstb->pos.x = frame.x + x2;
				resolution_lstb->pos.y = frame.y + y2;
				child_widget.push_back(resolution_lstb);
				optiontab[child_widget.size()-1] = 0;

				resolution_lb->setX(frame.x + x1);
				resolution_lb->setY(frame.y + y1);
				resolution_lb->set(msg->get("Resolution"));
				child_widget.push_back(resolution_lb);
				optiontab[child_widget.size()-1] = 0;

			}
			else if (infile.key == "activemods") {
				activemods_lstb->pos.x = frame.x + x2;
				activemods_lstb->pos.y = frame.y + y2;

				activemods_lb->setX(frame.x + x1);
				activemods_lb->setY(frame.y + y1);
				activemods_lb->set(msg->get("Active Mods"));
				child_widget.push_back(activemods_lb);
				optiontab[child_widget.size()-1] = 5;
			}
			else if (infile.key == "inactivemods") {
				inactivemods_lstb->pos.x = frame.x + x2;
				inactivemods_lstb->pos.y = frame.y + y2;

				inactivemods_lb->setX(frame.x + x1);
				inactivemods_lb->setY(frame.y + y1);
				inactivemods_lb->set(msg->get("Available Mods"));
				child_widget.push_back(inactivemods_lb);
				optiontab[child_widget.size()-1] = 5;
			}
			else if (infile.key == "joystick_device") {
				joystick_device_lstb->pos.x = frame.x + x2;
				joystick_device_lstb->pos.y = frame.y + y2;
				for(int i = 0; i < SDL_NumJoysticks(); i++) {
					if (SDL_JoystickName(i) != NULL)
						joystick_device_lstb->append(SDL_JoystickName(i),SDL_JoystickName(i));
				}
				child_widget.push_back(joystick_device_lstb);
				optiontab[child_widget.size()-1] = 3;

				joystick_device_lb->setX(frame.x + x1);
				joystick_device_lb->setY(frame.y + y1);
				joystick_device_lb->set(msg->get("Joystick"));
				child_widget.push_back(joystick_device_lb);
				optiontab[child_widget.size()-1] = 3;
			}
			else if (infile.key == "language") {
				language_lstb->pos.x = frame.x + x2;
				language_lstb->pos.y = frame.y + y2;
				child_widget.push_back(language_lstb);
				optiontab[child_widget.size()-1] = 2;

				language_lb->setX(frame.x + x1);
				language_lb->setY(frame.y + y1);
				language_lb->set(msg->get("Language"));
				child_widget.push_back(language_lb);
				optiontab[child_widget.size()-1] = 2;
			}
			// buttons begin
			else if (infile.key == "cancel") setting_num = CANCEL;
			else if (infile.key == "accept") setting_num = ACCEPT;
			else if (infile.key == "up") setting_num = UP;
			else if (infile.key == "down") setting_num = DOWN;
			else if (infile.key == "left") setting_num = LEFT;
			else if (infile.key == "right") setting_num = RIGHT;
			else if (infile.key == "bar1") setting_num = BAR_1;
			else if (infile.key == "bar2") setting_num = BAR_2;
			else if (infile.key == "bar3") setting_num = BAR_3;
			else if (infile.key == "bar4") setting_num = BAR_4;
			else if (infile.key == "bar5") setting_num = BAR_5;
			else if (infile.key == "bar6") setting_num = BAR_6;
			else if (infile.key == "bar7") setting_num = BAR_7;
			else if (infile.key == "bar8") setting_num = BAR_8;
			else if (infile.key == "bar9") setting_num = BAR_9;
			else if (infile.key == "bar0") setting_num = BAR_0;
			else if (infile.key == "main1") setting_num = MAIN1;
			else if (infile.key == "main2") setting_num = MAIN2;
			else if (infile.key == "character") setting_num = CHARACTER;
			else if (infile.key == "inventory") setting_num = INVENTORY;
			else if (infile.key == "powers") setting_num = POWERS;
			else if (infile.key == "log") setting_num = LOG;
			else if (infile.key == "ctrl") setting_num = CTRL;
			else if (infile.key == "shift") setting_num = SHIFT;
			else if (infile.key == "delete") setting_num = DEL;
			else if (infile.key == "actionbar") setting_num = ACTIONBAR;
			else if (infile.key == "actionbar_back") setting_num = ACTIONBAR_BACK;
			else if (infile.key == "actionbar_forward") setting_num = ACTIONBAR_FORWARD;
			else if (infile.key == "actionbar_use") setting_num = ACTIONBAR_USE;
			// buttons end

			else if (infile.key == "hws_note") {
				hws_note_lb->setX(frame.x + x1);
				hws_note_lb->setY(frame.y + y1);
				hws_note_lb->set(msg->get("Disable for performance"));
				child_widget.push_back(hws_note_lb);
				optiontab[child_widget.size()-1] = 0;
			}
			else if (infile.key == "dbuf_note") {
				dbuf_note_lb->setX(frame.x + x1);
				dbuf_note_lb->setY(frame.y + y1);
				dbuf_note_lb->set(msg->get("Disable for performance"));
				child_widget.push_back(dbuf_note_lb);
				optiontab[child_widget.size()-1] = 0;
			}
			else if (infile.key == "anim_tiles_note") {
				anim_tiles_note_lb->setX(frame.x + x1);
				anim_tiles_note_lb->setY(frame.y + y1);
				anim_tiles_note_lb->set(msg->get("Disable for performance"));
				child_widget.push_back(anim_tiles_note_lb);
				optiontab[child_widget.size()-1] = 0;
			}
			else if (infile.key == "test_note") {
				test_note_lb->setX(frame.x + x1);
				test_note_lb->setY(frame.y + y1);
				test_note_lb->set(msg->get("Experimental"));
				child_widget.push_back(test_note_lb);
				optiontab[child_widget.size()-1] = 0;
			}
			else if (infile.key == "handheld_note") {
				handheld_note_lb->setX(frame.x + x1);
				handheld_note_lb->setY(frame.y + y1);
				handheld_note_lb->set(msg->get("For handheld devices"));
				child_widget.push_back(handheld_note_lb);
				optiontab[child_widget.size()-1] = 3;
			}
			//buttons
			else if (infile.key == "activemods_shiftup") {
				activemods_shiftup_btn->pos.x = frame.x + x1;
				activemods_shiftup_btn->pos.y = frame.y + y1;
				activemods_shiftup_btn->refresh();
				child_widget.push_back(activemods_shiftup_btn);
				optiontab[child_widget.size()-1] = 5;
			}
			else if (infile.key == "activemods_shiftdown") {
				activemods_shiftdown_btn->pos.x = frame.x + x1;
				activemods_shiftdown_btn->pos.y = frame.y + y1;
				activemods_shiftdown_btn->refresh();
				child_widget.push_back(activemods_shiftdown_btn);
				optiontab[child_widget.size()-1] = 5;
			}
			else if (infile.key == "activemods_deactivate") {
				activemods_deactivate_btn->label = msg->get("<< Disable");
				activemods_deactivate_btn->pos.x = frame.x + x1;
				activemods_deactivate_btn->pos.y = frame.y + y1;
				activemods_deactivate_btn->refresh();
				child_widget.push_back(activemods_deactivate_btn);
				optiontab[child_widget.size()-1] = 5;
			}
			else if (infile.key == "inactivemods_activate") {
				inactivemods_activate_btn->label = msg->get("Enable >>");
				inactivemods_activate_btn->pos.x = frame.x + x1;
				inactivemods_activate_btn->pos.y = frame.y + y1;
				inactivemods_activate_btn->refresh();
				child_widget.push_back(inactivemods_activate_btn);
				optiontab[child_widget.size()-1] = 5;
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

			if (setting_num > -1 && setting_num < 29) {
				//keybindings
				settings_lb[setting_num]->setX(x1);
				settings_lb[setting_num]->setY(y1);
				settings_key[setting_num]->pos.x = x2;
				settings_key[setting_num]->pos.y = y2;
			}

		}
		infile.close();
	}

	// Load the MenuConfirm positions and alignments from menus/menus.txt
	if (infile.open("menus/menus.txt")) {
		int menu_index = -1;
		while (infile.next()) {
			if (infile.key == "id") {
				if (infile.val == "confirm") menu_index = 0;
				else menu_index = -1;
			}

			if (menu_index == -1)
				continue;

			if (infile.key == "layout") {
				infile.val = infile.val + ',';
				menuConfirm_area.x = eatFirstInt(infile.val, ',');
				menuConfirm_area.y = eatFirstInt(infile.val, ',');
				menuConfirm_area.w = eatFirstInt(infile.val, ',');
				menuConfirm_area.h = eatFirstInt(infile.val, ',');
			}

			if (infile.key == "align") {
				menuConfirm_align = infile.val;
			}
		}
		infile.close();
	}

	defaults_confirm->window_area = menuConfirm_area;
	defaults_confirm->alignment = menuConfirm_align;
	defaults_confirm->align();
	defaults_confirm->update();

	resolution_confirm->window_area = menuConfirm_area;
	resolution_confirm->alignment = menuConfirm_align;
	resolution_confirm->align();
	resolution_confirm->update();

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
	for (unsigned int i = 29; i < 58; i++) {
		settings_key[i]->pos.x = settings_key[i-29]->pos.x + offset_x;
		settings_key[i]->pos.y = settings_key[i-29]->pos.y + offset_y;
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
	if (TEXTURE_QUALITY) texture_quality_cb->Check();
	else texture_quality_cb->unCheck();
	if (CHANGE_GAMMA) change_gamma_cb->Check();
	else {
		change_gamma_cb->unCheck();
		GAMMA = 1.0;
		gamma_sl->enabled = false;
	}
	gamma_sl->set(5,20,(int)(GAMMA*10.0));
	SDL_SetGamma(GAMMA,GAMMA,GAMMA);

	if (ANIMATED_TILES) animated_tiles_cb->Check();
	else animated_tiles_cb->unCheck();
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

	if (!getLanguagesList()) fprintf(stderr, "Unable to get languages list!\n");
	for (int i=0; i < getLanguagesNumber(); i++) {
		language_lstb->append(language_full[i],"");
		if (language_ISO[i] == LANGUAGE) language_lstb->selected[i] = true;
	}
	language_lstb->refresh();

	activemods_lstb->refresh();
	inactivemods_lstb->refresh();

	for (unsigned int i = 0; i < 29; i++) {
		if (inpt->binding[i] < 8) {
			settings_key[i]->label = inpt->mouse_button[inpt->binding[i]-1];
		}
		else {
			settings_key[i]->label = SDL_GetKeyName((SDLKey)inpt->binding[i]);
		}
		settings_key[i]->refresh();
	}
	for (unsigned int i = 29; i < 58; i++) {
		if (inpt->binding_alt[i-29] < 8) {
			settings_key[i]->label = inpt->mouse_button[inpt->binding_alt[i-29]-1];
		}
		else {
			settings_key[i]->label = SDL_GetKeyName((SDLKey)inpt->binding_alt[i-29]);
		}
		settings_key[i]->refresh();
	}
	input_scrollbox->refresh();
}

void GameStateConfig::logic () {
	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (input_scrollbox->in_focus && !input_confirm->visible) {
			tabControl->setActiveTab(4);
			break;
		}
		else if (child_widget[i]->in_focus) {
			tabControl->setActiveTab(optiontab[i]);
			break;
		}
	}

	check_resolution = true;

	std::string resolution_value;
	resolution_value = resolution_lstb->getValue() + 'x'; // add x to have last element readable
	int width = eatFirstInt(resolution_value, 'x');
	int height = eatFirstInt(resolution_value, 'x');

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

	if (resolution_confirm->visible || resolution_confirm->cancelClicked) {
		resolution_confirm->logic();
		resolution_confirm_ticks--;
		if (resolution_confirm->confirmClicked) {
			saveSettings();
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
		}
		else if (resolution_confirm->cancelClicked || resolution_confirm_ticks == 0) {
			applyVideoSettings(old_view_w, old_view_h);
			saveSettings();
			delete requestedGameState;
			requestedGameState = new GameStateConfig();
		}
	}

	if (!input_confirm->visible && !defaults_confirm->visible && !resolution_confirm->visible) {
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
			}
			loadMiscSettings();
			refreshFont();
			if ((ENABLE_JOYSTICK) && (SDL_NumJoysticks() > 0)) {
				SDL_JoystickClose(joy);
				joy = SDL_JoystickOpen(JOYSTICK_DEVICE);
			}
			applyVideoSettings(width, height);
			if (width != old_view_w || height != old_view_h) {
				resolution_confirm->window_area = menuConfirm_area;
				resolution_confirm->align();
				resolution_confirm->update();
				resolution_confirm_ticks = MAX_FRAMES_PER_SEC * 10; // 10 seconds
			}
			else {
				saveSettings();
				delete requestedGameState;
				requestedGameState = new GameStateTitle();
			}
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
	if (active_tab == 0 && !defaults_confirm->visible) {
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
		else if (texture_quality_cb->checkClick()) {
			if (texture_quality_cb->isChecked()) TEXTURE_QUALITY=true;
			else TEXTURE_QUALITY=false;
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
				SDL_SetGamma(GAMMA,GAMMA,GAMMA);
			}
		}
		else if (animated_tiles_cb->checkClick()) {
			if (animated_tiles_cb->isChecked()) ANIMATED_TILES=true;
			else ANIMATED_TILES=false;
		}
		else if (resolution_lstb->checkClick()) {
			; // nothing to do here: resolution value changes next frame.
		}
		else if (CHANGE_GAMMA) {
			gamma_sl->enabled = true;
			if (gamma_sl->checkClick()) {
				GAMMA=(gamma_sl->getValue())*0.1f;
				SDL_SetGamma(GAMMA,GAMMA,GAMMA);
			}
		}
	}
	// tab 1 (audio)
	else if (active_tab == 1 && !defaults_confirm->visible) {
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
	else if (active_tab == 2 && !defaults_confirm->visible) {
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
	}
	// tab 3 (input)
	else if (active_tab == 3 && !defaults_confirm->visible) {
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
	else if (active_tab == 4 && !defaults_confirm->visible) {
		if (input_confirm->visible) {
			input_confirm->logic();
			scanKey(input_key);
			input_confirm_ticks--;
			if (input_confirm_ticks == 0) input_confirm->visible = false;
		}
		else {
			input_scrollbox->logic();
			for (unsigned int i = 0; i < 58; i++) {
				if (settings_key[i]->pressed || settings_key[i]->hover) input_scrollbox->update = true;
				Point mouse = input_scrollbox->input_assist(inpt->mouse);
				if (settings_key[i]->checkClick(mouse.x,mouse.y)) {
					std::string confirm_msg;
					if (i < 29)
						confirm_msg = msg->get("Assign: ") + inpt->binding_name[i];
					else
						confirm_msg = msg->get("Assign: ") + inpt->binding_name[i-29];
					delete input_confirm;
					input_confirm = new MenuConfirm("",confirm_msg);
					input_confirm->window_area = menuConfirm_area;
					input_confirm->alignment = menuConfirm_align;
					input_confirm->align();
					input_confirm->update();
					input_confirm_ticks = MAX_FRAMES_PER_SEC * 10; // 10 seconds
					input_confirm->visible = true;
					input_key = i;
					inpt->last_button = -1;
					inpt->last_key = -1;
				}
			}
		}
	}
	// tab 5 (mods)
	else if (active_tab == 5 && !defaults_confirm->visible) {
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
	if (resolution_confirm->visible) {
		resolution_confirm->render();
		return;
	}

	int tabheight = tabControl->getTabHeight();
	SDL_Rect	pos;
	pos.x = (VIEW_W-FRAME_W)/2;
	pos.y = (VIEW_H-FRAME_H)/2 + tabheight - tabheight/16;

	SDL_BlitSurface(background,NULL,screen,&pos);

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
			for (unsigned int i = 0; i < 29; i++) {
				settings_lb[i]->render(input_scrollbox->contents);
			}
		}
		input_scrollbox->render();
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
	SDL_Rect common_modes[cm_count];
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
	SDL_Rect** detect_modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

	// Check if there are any modes available
	if (detect_modes == (SDL_Rect**)0) {
		fprintf(stderr, "No modes available!\n");
		return 0;
	}

	// Check if our resolution is restricted
	if (detect_modes == (SDL_Rect**)-1) {
		fprintf(stderr, "All resolutions available.\n");
	}

	for (unsigned i=0; detect_modes[i]; ++i) {
		video_modes.push_back(*detect_modes[i]);
		if (detect_modes[i]->w < MIN_VIEW_W || detect_modes[i]->h < MIN_VIEW_H) {
			// make sure the resolution fits in the constraints of MIN_VIEW_W and MIN_VIEW_H
			video_modes.pop_back();
		}
		else {
			// check previous resolutions for duplicates. If one is found, drop the one we just added
			for (unsigned j=0; j<video_modes.size()-1; ++j) {
				if (video_modes[j].w == detect_modes[i]->w && video_modes[j].h == detect_modes[i]->h) {
					video_modes.pop_back();
					break;
				}
			}
		}
	}

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
 * Tries to apply the selected video settings, reverting back to the old settings upon failure
 */
bool GameStateConfig::applyVideoSettings(int width, int height) {
	if (MIN_VIEW_W > width && MIN_VIEW_H > height) {
		fprintf (stderr, "A mod is requiring a minimum resolution of %dx%d\n", MIN_VIEW_W, MIN_VIEW_H);
		if (width < MIN_VIEW_W) width = MIN_VIEW_W;
		if (height < MIN_VIEW_H) height = MIN_VIEW_H;
	}

	// Attempt to apply the new settings
	setupSDLVideoMode(width, height);

	// If the new settings fail, revert to the old ones
	if (!screen) {
		fprintf (stderr, "Error during SDL_SetVideoMode: %s\n", SDL_GetError());
		setupSDLVideoMode(VIEW_W, VIEW_H);
		return false;

	}
	else {

		// If the new settings succeed, adjust the view area
		VIEW_W = width;
		VIEW_W_HALF = width/2;
		VIEW_H = height;
		VIEW_H_HALF = height/2;

		resolution_confirm->visible = true;

		return true;
	}
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
	vector<string> temp_list = mods->mod_list;
	mods->mod_list.clear();
	for (int i=0; i<activemods_lstb->getSize(); i++) {
		if (activemods_lstb->getValue(i) != "") mods->mod_list.push_back(activemods_lstb->getValue(i));
	}
	ofstream outfile;
	outfile.open((PATH_CONF + "mods.txt").c_str(), ios::out);

	if (outfile.is_open()) {

		outfile<<"# Mods lower on the list will overwrite data in the entries higher on the list"<<"\n";
		outfile<<"\n";

		for (unsigned int i = 0; i < mods->mod_list.size(); i++) {
			outfile<<mods->mod_list[i]<<"\n";
		}
	}
	if (outfile.bad()) fprintf(stderr, "Unable to save mod list into file. No write access or disk is full!\n");
	outfile.close();
	outfile.clear();
	if (mods->mod_list != temp_list) return true;
	else return false;
}

/**
 * Scan key binding
 */
void GameStateConfig::scanKey(int button) {
	if (input_confirm->visible) {
		if (inpt->last_button != -1 && inpt->last_button < 8) {
			if (button < 29) inpt->binding[button] = inpt->last_button;
			else inpt->binding_alt[button-29] = inpt->last_button;

			settings_key[button]->label = inpt->mouse_button[inpt->last_button-1];
			input_confirm->visible = false;
			input_confirm_ticks = 0;
			settings_key[button]->refresh();
			return;
		}
		if (inpt->last_key != -1) {
			if (button < 29) inpt->binding[button] = inpt->last_key;
			else inpt->binding_alt[button-29] = inpt->last_key;

			settings_key[button]->label = SDL_GetKeyName((SDLKey)inpt->last_key);
			input_confirm->visible = false;
			input_confirm_ticks = 0;
			settings_key[button]->refresh();
			return;
		}
	}
}

GameStateConfig::~GameStateConfig() {
	tip_buf.clear();
	delete tip;
	delete tabControl;
	delete ok_button;
	delete defaults_button;
	delete cancel_button;
	delete input_scrollbox;
	delete input_confirm;
	delete defaults_confirm;
	delete resolution_confirm;

	SDL_FreeSurface(background);

	for (std::vector<Widget*>::iterator iter = child_widget.begin(); iter != child_widget.end(); ++iter) {
		delete (*iter);
	}
	child_widget.clear();

	for (unsigned int i = 0; i < 29; i++) {
		delete settings_lb[i];
	}
	for (unsigned int i = 0; i < 58; i++) {
		delete settings_key[i];
	}

	language_ISO.clear();
	language_full.clear();

}

void GameStateConfig::placeLabeledCheckbox( WidgetLabel* lb, WidgetCheckBox* cb, int x1, int y1, int x2, int y2, std::string const& str, int tab ) {
	cb->pos.x = frame.x + x2;
	cb->pos.y = frame.y + y2;
	child_widget.push_back(cb);
	optiontab[child_widget.size()-1] = tab;

	lb->setX(frame.x + x1);
	lb->setY(frame.y + y1);
	lb->set(str);
	lb->setJustify(JUSTIFY_RIGHT);
	child_widget.push_back(lb);
	optiontab[child_widget.size()-1] = tab;
}
