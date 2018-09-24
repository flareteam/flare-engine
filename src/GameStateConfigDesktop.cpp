/*
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
 * GameStateConfigDesktop
 *
 * Handle game Settings Menu (desktop computer settings)
 */

#include "CommonIncludes.h"
#include "DeviceList.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateConfigBase.h"
#include "GameStateConfigDesktop.h"
#include "GameStateTitle.h"
#include "InputState.h"
#include "MenuConfirm.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Stats.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetCheckBox.h"
#include "WidgetListBox.h"
#include "WidgetScrollBox.h"
#include "WidgetSlider.h"
#include "WidgetTabControl.h"

#include <limits.h>
#include <iomanip>

GameStateConfigDesktop::GameStateConfigDesktop(bool _enable_video_tab)
	: GameStateConfigBase(!GameStateConfigBase::DO_INIT)
	, renderer_lstb(new WidgetListBox(4, WidgetListBox::DEFAULT_FILE))
	, renderer_lb(new WidgetLabel())
	, fullscreen_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, fullscreen_lb(new WidgetLabel())
	, hwsurface_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, hwsurface_lb(new WidgetLabel())
	, vsync_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, vsync_lb(new WidgetLabel())
	, texture_filter_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, texture_filter_lb(new WidgetLabel())
	, dpi_scaling_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, dpi_scaling_lb(new WidgetLabel())
	, change_gamma_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, change_gamma_lb(new WidgetLabel())
	, gamma_sl(new WidgetSlider(WidgetSlider::DEFAULT_FILE))
	, gamma_lb(new WidgetLabel())
	, joystick_device_lstb(new WidgetListBox(10, WidgetListBox::DEFAULT_FILE))
	, joystick_device_lb(new WidgetLabel())
	, enable_joystick_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, enable_joystick_lb(new WidgetLabel())
	, mouse_move_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, mouse_move_lb(new WidgetLabel())
	, mouse_aim_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, mouse_aim_lb(new WidgetLabel())
	, no_mouse_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, no_mouse_lb(new WidgetLabel())
	, joystick_deadzone_sl(new WidgetSlider(WidgetSlider::DEFAULT_FILE))
	, joystick_deadzone_lb(new WidgetLabel())
	, input_scrollbox(NULL)
	, input_confirm(new MenuConfirm(msg->get("Clear"),msg->get("Assign:")))
	, input_confirm_ticks(0)
	, input_key(0)
	, key_count(0)
	, scrollpane_contents(0)
	, enable_video_tab(_enable_video_tab)
	, keybind_tip_ticks(0)
	, keybind_tip(new WidgetTooltip())
{
	// Allocate KeyBindings
	for (int i = 0; i < inpt->KEY_COUNT; i++) {
		keybinds_lb.push_back(new WidgetLabel());
		keybinds_lb[i]->setText(inpt->binding_name[i]);
		keybinds_lb[i]->setJustify(FontEngine::JUSTIFY_RIGHT);
	}
	for (int i = 0; i < inpt->KEY_COUNT * 3; i++) {
		keybinds_btn.push_back(new WidgetButton(WidgetButton::DEFAULT_FILE));
	}

	key_count = static_cast<unsigned>(keybinds_btn.size()/3);

	init();
}

GameStateConfigDesktop::~GameStateConfigDesktop() {
}

void GameStateConfigDesktop::init() {
	if (enable_video_tab) {
		VIDEO_TAB = 0;
		AUDIO_TAB = 1;
		INTERFACE_TAB = 2;
		INPUT_TAB = 3;
		KEYBINDS_TAB = 4;
		MODS_TAB = 5;
	}
	else {
		AUDIO_TAB = 0;
		INTERFACE_TAB = 1;
		INPUT_TAB = 2;
		KEYBINDS_TAB = 3;
		MODS_TAB = 4;
	}

	if (enable_video_tab) {
		tab_control->setTabTitle(VIDEO_TAB, msg->get("Video"));
	}
	tab_control->setTabTitle(AUDIO_TAB, msg->get("Audio"));
	tab_control->setTabTitle(INTERFACE_TAB, msg->get("Interface"));
	tab_control->setTabTitle(INPUT_TAB, msg->get("Input"));
	tab_control->setTabTitle(KEYBINDS_TAB, msg->get("Keybindings"));
	tab_control->setTabTitle(MODS_TAB, msg->get("Mods"));

	readConfig();

	// Allocate KeyBindings ScrollBox
	input_scrollbox = new WidgetScrollBox(scrollpane.w, scrollpane.h);
	input_scrollbox->setBasePos(scrollpane.x, scrollpane.y, Utils::ALIGN_TOPLEFT);
	input_scrollbox->bg = scrollpane_color;
	input_scrollbox->resize(scrollpane.w, scrollpane_contents);

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

	refreshWidgets();

	update();
}

void GameStateConfigDesktop::readConfig() {
	FileParser infile;
	if (infile.open("menus/config.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (parseKeyButtons(infile))
				continue;

			int x1 = Parse::popFirstInt(infile.val);
			int y1 = Parse::popFirstInt(infile.val);
			int x2 = Parse::popFirstInt(infile.val);
			int y2 = Parse::popFirstInt(infile.val);

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

	hwsurface_cb->tooltip = msg->get("Will try to store surfaces in video memory versus system memory. The effect this has on performance depends on the renderer.");
	vsync_cb->tooltip = msg->get("Prevents screen tearing. Disable if you experience \"stuttering\" in windowed mode or input lag.");
	dpi_scaling_cb->tooltip = msg->get("When enabled, this uses the screen DPI in addition to the window dimensions to scale the rendering resolution. Otherwise, only the window dimensions are used.");
	change_gamma_cb->tooltip = msg->get("Experimental");
	no_mouse_cb->tooltip = msg->get("For handheld devices");
}

bool GameStateConfigDesktop::parseKeyDesktop(FileParser &infile, int &x1, int &y1, int &x2, int &y2) {
	// @CLASS GameStateConfigDesktop|Description of menus/config.txt

	int keybind_num = -1;

	if (infile.key == "listbox_scrollbar_offset") {
		// overrides same key in GameStateConfigBase
		renderer_lstb->scrollbar_offset = x1;
		joystick_device_lstb->scrollbar_offset = x1;
		activemods_lstb->scrollbar_offset = x1;
		inactivemods_lstb->scrollbar_offset = x1;
		language_lstb->scrollbar_offset = x1;
	}
	else if (infile.key == "renderer") {
		// @ATTR renderer|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Renderer" list box relative to the frame.
		placeLabeledWidget(renderer_lb, renderer_lstb, x1, y1, x2, y2, msg->get("Renderer"));

		renderer_lstb->can_select = true;
		renderer_lstb->multi_select = false;
		renderer_lstb->can_deselect = false;

		refreshRenderers();

		renderer_lb->setJustify(FontEngine::JUSTIFY_CENTER);
	}
	else if (infile.key == "renderer_height") {
		// @ATTR renderer_height|int|Number of visible rows for the "Renderer" list box.
		renderer_lstb->setHeight(x1);
	}
	else if (infile.key == "fullscreen") {
		// @ATTR fullscreen|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Full Screen Mode" checkbox relative to the frame.
		placeLabeledWidget(fullscreen_lb, fullscreen_cb, x1, y1, x2, y2, msg->get("Full Screen Mode"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "mouse_move") {
		// @ATTR mouse_move|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Move hero using mouse" checkbox relative to the frame.
		placeLabeledWidget(mouse_move_lb, mouse_move_cb, x1, y1, x2, y2, msg->get("Move hero using mouse"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "hwsurface") {
		// @ATTR hwsurface|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Hardware surfaces" checkbox relative to the frame.
		placeLabeledWidget(hwsurface_lb, hwsurface_cb, x1, y1, x2, y2, msg->get("Hardware surfaces"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "vsync") {
		// @ATTR vsync|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "V-Sync" checkbox relative to the frame.
		placeLabeledWidget(vsync_lb, vsync_cb, x1, y1, x2, y2, msg->get("V-Sync"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "texture_filter") {
		// @ATTR texture_filter|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Texture Filtering" checkbox relative to the frame.
		placeLabeledWidget(texture_filter_lb, texture_filter_cb, x1, y1, x2, y2, msg->get("Texture Filtering"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "dpi_scaling") {
		// @ATTR dpi_scaling|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "DPI scaling" checkbox relative to the frame.
		placeLabeledWidget(dpi_scaling_lb, dpi_scaling_cb, x1, y1, x2, y2, msg->get("DPI scaling"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "change_gamma") {
		// @ATTR change_gamma|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Allow changing gamma" checkbox relative to the frame.
		placeLabeledWidget(change_gamma_lb, change_gamma_cb, x1, y1, x2, y2, msg->get("Allow changing gamma"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "gamma") {
		// @ATTR gamma|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Gamma" slider relative to the frame.
		placeLabeledWidget(gamma_lb, gamma_sl, x1, y1, x2, y2, msg->get("Gamma"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "enable_joystick") {
		// @ATTR enable_joystick|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Use joystick" checkbox relative to the frame.
		placeLabeledWidget(enable_joystick_lb, enable_joystick_cb, x1, y1, x2, y2, msg->get("Use joystick"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "joystick_device") {
		// @ATTR joystick_device|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Joystick" list box relative to the frame.
		placeLabeledWidget(joystick_device_lb, joystick_device_lstb, x1, y1, x2, y2, msg->get("Joystick"));

		for(int i = 0; i < inpt->getNumJoysticks(); i++) {
			std::string joystick_name = inpt->getJoystickName(i);
			if (joystick_name != "")
				joystick_device_lstb->append(joystick_name, joystick_name);
		}

		joystick_device_lb->setJustify(FontEngine::JUSTIFY_CENTER);
	}
	else if (infile.key == "joystick_device_height") {
		// @ATTR joystick_device_height|int|Number of visible rows for the "Joystick" list box.
		joystick_device_lstb->setHeight(x1);
	}
	else if (infile.key == "mouse_aim") {
		// @ATTR mouse_aim|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Mouse aim" checkbox relative to the frame.
		placeLabeledWidget(mouse_aim_lb, mouse_aim_cb, x1, y1, x2, y2, msg->get("Mouse aim"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "no_mouse") {
		// @ATTR no_mouse|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Do not use mouse" checkbox relative to the frame.
		placeLabeledWidget(no_mouse_lb, no_mouse_cb, x1, y1, x2, y2, msg->get("Do not use mouse"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "joystick_deadzone") {
		// @ATTR joystick_deadzone|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Joystick Deadzone" slider relative to the frame.
		placeLabeledWidget(joystick_deadzone_lb, joystick_deadzone_sl, x1, y1, x2, y2, msg->get("Joystick Deadzone"), FontEngine::JUSTIFY_RIGHT);
	}
	else if (infile.key == "secondary_offset") {
		// @ATTR secondary_offset|point|Offset of the second (and third) columns of keybinds.
		secondary_offset.x = x1;
		secondary_offset.y = y1;
	}
	else if (infile.key == "keybinds_bg_color") {
		// @ATTR keybinds_bg_color|color|Background color for the keybindings scrollbox.
		scrollpane_color.r = static_cast<Uint8>(x1);
		scrollpane_color.g = static_cast<Uint8>(y1);
		scrollpane_color.b = static_cast<Uint8>(x2);
	}
	else if (infile.key == "keybinds_bg_alpha") {
		// @ATTR keybinds_bg_alpha|int|Alpha value for the keybindings scrollbox background color.
		scrollpane_color.a = static_cast<Uint8>(x1);
	}
	else if (infile.key == "scrollpane") {
		// @ATTR scrollpane|rectangle|Position of the keybinding scrollbox relative to the frame.
		scrollpane.x = x1;
		scrollpane.y = y1;
		scrollpane.w = x2;
		scrollpane.h = y2;
	}
	else if (infile.key == "scrollpane_contents") {
		// @ATTR scrollpane_contents|int|The vertical size of the keybinding scrollbox's contents.
		scrollpane_contents = x1;
	}

	// @ATTR cancel|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Cancel" keybind relative to the keybinding scrollbox.
	else if (infile.key == "cancel") keybind_num = Input::CANCEL;
	// @ATTR accept|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Accept" keybind relative to the keybinding scrollbox.
	else if (infile.key == "accept") keybind_num = Input::ACCEPT;
	// @ATTR up|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Up" keybind relative to the keybinding scrollbox.
	else if (infile.key == "up") keybind_num = Input::UP;
	// @ATTR down|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Down" keybind relative to the keybinding scrollbox.
	else if (infile.key == "down") keybind_num = Input::DOWN;
	// @ATTR left|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Left" keybind relative to the keybinding scrollbox.
	else if (infile.key == "left") keybind_num = Input::LEFT;
	// @ATTR right|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Right" keybind relative to the keybinding scrollbox.
	else if (infile.key == "right") keybind_num = Input::RIGHT;
	// @ATTR bar1|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar1" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar1") keybind_num = Input::BAR_1;
	// @ATTR bar2|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar2" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar2") keybind_num = Input::BAR_2;
	// @ATTR bar3|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar3" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar3") keybind_num = Input::BAR_3;
	// @ATTR bar4|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar4" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar4") keybind_num = Input::BAR_4;
	// @ATTR bar5|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar5" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar5") keybind_num = Input::BAR_5;
	// @ATTR bar6|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar6" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar6") keybind_num = Input::BAR_6;
	// @ATTR bar7|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar7" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar7") keybind_num = Input::BAR_7;
	// @ATTR Bar8|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar8" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar8") keybind_num = Input::BAR_8;
	// @ATTR bar9|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar9" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar9") keybind_num = Input::BAR_9;
	// @ATTR bar0|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Bar0" keybind relative to the keybinding scrollbox.
	else if (infile.key == "bar0") keybind_num = Input::BAR_0;
	// @ATTR main1|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Main1" keybind relative to the keybinding scrollbox.
	else if (infile.key == "main1") keybind_num = Input::MAIN1;
	// @ATTR main2|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Main2" keybind relative to the keybinding scrollbox.
	else if (infile.key == "main2") keybind_num = Input::MAIN2;
	// @ATTR character|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Character" keybind relative to the keybinding scrollbox.
	else if (infile.key == "character") keybind_num = Input::CHARACTER;
	// @ATTR inventory|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Inventory" keybind relative to the keybinding scrollbox.
	else if (infile.key == "inventory") keybind_num = Input::INVENTORY;
	// @ATTR powers|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Powers" keybind relative to the keybinding scrollbox.
	else if (infile.key == "powers") keybind_num = Input::POWERS;
	// @ATTR log|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Log" keybind relative to the keybinding scrollbox.
	else if (infile.key == "log") keybind_num = Input::LOG;
	// @ATTR ctrl|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Ctrl" keybind relative to the keybinding scrollbox.
	else if (infile.key == "ctrl") keybind_num = Input::CTRL;
	// @ATTR shift|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Shift" keybind relative to the keybinding scrollbox.
	else if (infile.key == "shift") keybind_num = Input::SHIFT;
	// @ATTR alt|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Alt" keybind relative to the keybinding scrollbox.
	else if (infile.key == "alt") keybind_num = Input::ALT;
	// @ATTR delete|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Delete" keybind relative to the keybinding scrollbox.
	else if (infile.key == "delete") keybind_num = Input::DEL;
	// @ATTR actionbar|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Accept" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar") keybind_num = Input::ACTIONBAR;
	// @ATTR actionbar_back|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Left" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_back") keybind_num = Input::ACTIONBAR_BACK;
	// @ATTR actionbar_forward|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Right" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_forward") keybind_num = Input::ACTIONBAR_FORWARD;
	// @ATTR actionbar_use|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "ActionBar Use" keybind relative to the keybinding scrollbox.
	else if (infile.key == "actionbar_use") keybind_num = Input::ACTIONBAR_USE;
	// @ATTR developer_menu|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Developer Menu" keybind relative to the keybinding scrollbox.
	else if (infile.key == "developer_menu") keybind_num = Input::DEVELOPER_MENU;

	else return false;

	if (keybind_num > -1 && static_cast<unsigned>(keybind_num) < keybinds_lb.size() && static_cast<unsigned>(keybind_num) < keybinds_btn.size()) {
		//keybindings
		keybinds_lb[keybind_num]->setPos(x1, y1);
		keybinds_btn[keybind_num]->pos.x = x2;
		keybinds_btn[keybind_num]->pos.y = y2;
	}

	return true;
}

void GameStateConfigDesktop::addChildWidgetsDesktop() {
	if (enable_video_tab) {
		addChildWidget(renderer_lstb, VIDEO_TAB);
		addChildWidget(renderer_lb, VIDEO_TAB);
		addChildWidget(fullscreen_cb, VIDEO_TAB);
		addChildWidget(fullscreen_lb, VIDEO_TAB);
		addChildWidget(hwsurface_cb, VIDEO_TAB);
		addChildWidget(hwsurface_lb, VIDEO_TAB);
		addChildWidget(vsync_cb, VIDEO_TAB);
		addChildWidget(vsync_lb, VIDEO_TAB);
		addChildWidget(texture_filter_cb, VIDEO_TAB);
		addChildWidget(texture_filter_lb, VIDEO_TAB);
		addChildWidget(dpi_scaling_cb, VIDEO_TAB);
		addChildWidget(dpi_scaling_lb, VIDEO_TAB);
		addChildWidget(change_gamma_cb, VIDEO_TAB);
		addChildWidget(change_gamma_lb, VIDEO_TAB);
		addChildWidget(gamma_sl, VIDEO_TAB);
		addChildWidget(gamma_lb, VIDEO_TAB);
	}

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

	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		input_scrollbox->addChildWidget(keybinds_btn[i]);
	}
}

void GameStateConfigDesktop::setupTabList() {
	tablist.add(tab_control);
	tablist.setPrevTabList(&tablist_main);

	tablist_main.add(ok_button);
	tablist_main.add(defaults_button);
	tablist_main.add(cancel_button);
	tablist_main.setPrevTabList(&tablist);
	tablist_main.setNextTabList(&tablist);
	tablist_main.lock();

	if (enable_video_tab) {
		tablist_video.add(fullscreen_cb);
		tablist_video.add(hwsurface_cb);
		tablist_video.add(vsync_cb);
		tablist_video.add(texture_filter_cb);
		tablist_video.add(dpi_scaling_cb);
		tablist_video.add(change_gamma_cb);
		tablist_video.add(gamma_sl);
		tablist_video.add(renderer_lstb);
		tablist_video.setPrevTabList(&tablist);
		tablist_video.setNextTabList(&tablist_main);
		tablist_video.lock();
	}

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
	tablist_interface.add(statbar_labels_cb);
	tablist_interface.add(language_lstb);
	tablist_interface.setPrevTabList(&tablist);
	tablist_interface.setNextTabList(&tablist_main);
	tablist_interface.lock();

	tablist_input.add(enable_joystick_cb);
	tablist_input.add(mouse_move_cb);
	tablist_input.add(mouse_aim_cb);
	tablist_input.add(no_mouse_cb);
	tablist_input.add(joystick_deadzone_sl);
	tablist_input.add(joystick_device_lstb);
	tablist_input.setPrevTabList(&tablist);
	tablist_input.setNextTabList(&tablist_main);
	tablist_input.lock();

	tablist_keybinds.add(input_scrollbox);
	tablist_keybinds.setPrevTabList(&tablist);
	tablist_keybinds.setNextTabList(&tablist_main);
	tablist_keybinds.lock();

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

void GameStateConfigDesktop::update() {
	GameStateConfigBase::update();

	updateVideo();
	updateInput();
	updateKeybinds();
}

void GameStateConfigDesktop::updateVideo() {
	fullscreen_cb->setChecked(settings->fullscreen);
	hwsurface_cb->setChecked(settings->hwsurface);
	vsync_cb->setChecked(settings->vsync);
	texture_filter_cb->setChecked(settings->texture_filter);
	dpi_scaling_cb->setChecked(settings->dpi_scaling);
	change_gamma_cb->setChecked(settings->change_gamma);

	if (settings->change_gamma) {
		render_device->setGamma(settings->gamma);
	}
	else {
		settings->gamma = 1.0;
		gamma_sl->enabled = false;
		render_device->resetGamma();
	}
	gamma_sl->set(GAMMA_MIN, GAMMA_MAX, static_cast<int>(settings->gamma * 10.0));

	refreshRenderers();
}

void GameStateConfigDesktop::updateInput() {
	enable_joystick_cb->setChecked(settings->enable_joystick);
	mouse_aim_cb->setChecked(settings->mouse_aim);
	no_mouse_cb->setChecked(settings->no_mouse);
	mouse_move_cb->setChecked(settings->mouse_move);

	if (settings->enable_joystick && inpt->getNumJoysticks() > 0) {
		inpt->initJoystick();
		joystick_device_lstb->select(settings->joystick_device);
	}
	joystick_device_lstb->jumpToSelected();

	joystick_deadzone_sl->set(0, 32768, settings->joy_deadzone);
}

void GameStateConfigDesktop::updateKeybinds() {
	// now do labels for keybinds that are set
	for (unsigned int i = 0; i < key_count; i++) {
		keybinds_btn[i]->label = inpt->getBindingString(i);
		keybinds_btn[i]->refresh();
	}
	for (unsigned int i = key_count; i < key_count*2; i++) {
		keybinds_btn[i]->label = inpt->getBindingString(i-key_count, InputState::BINDING_ALT);
		keybinds_btn[i]->refresh();
	}
	for (unsigned int i = key_count*2; i < keybinds_btn.size(); i++) {
		keybinds_btn[i]->label = inpt->getBindingString(i-(key_count*2), InputState::BINDING_JOYSTICK);
		keybinds_btn[i]->refresh();
	}
	input_scrollbox->refresh();
}

void GameStateConfigDesktop::logic() {
	if (inpt->window_resized)
		refreshWidgets();

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

	if (enable_video_tab && active_tab == VIDEO_TAB) {
		tablist.setNextTabList(&tablist_video);
		logicVideo();
	}
	else if (active_tab == AUDIO_TAB) {
		tablist.setNextTabList(&tablist_audio);
		logicAudio();
	}
	else if (active_tab == INTERFACE_TAB) {
		tablist.setNextTabList(&tablist_interface);
		logicInterface();
	}
	else if (active_tab == INPUT_TAB) {
		tablist.setNextTabList(&tablist_input);
		logicInput();
	}
	else if (active_tab == KEYBINDS_TAB) {
		tablist.setNextTabList(&tablist_keybinds);
		logicKeybinds();
	}
	else if (active_tab == MODS_TAB) {
		tablist.setNextTabList(&tablist_mods);
		logicMods();
	}
}

bool GameStateConfigDesktop::logicMain() {
	if (GameStateConfigBase::logicMain()) {
		if (enable_video_tab) {
			tablist_video.logic();
		}
		tablist_input.logic();
		tablist_keybinds.logic();
		return true;
	}

	return false;
}

void GameStateConfigDesktop::logicVideo() {
	if (fullscreen_cb->checkClick()) {
		settings->fullscreen = fullscreen_cb->isChecked();
	}
	else if (hwsurface_cb->checkClick()) {
		settings->hwsurface = hwsurface_cb->isChecked();
	}
	else if (vsync_cb->checkClick()) {
		settings->vsync = vsync_cb->isChecked();
	}
	else if (texture_filter_cb->checkClick()) {
		settings->texture_filter = texture_filter_cb->isChecked();
	}
	else if (dpi_scaling_cb->checkClick()) {
		settings->dpi_scaling = dpi_scaling_cb->isChecked();
		render_device->windowResize();
		refreshWidgets();
		force_refresh_background = true;
	}
	else if (change_gamma_cb->checkClick()) {
		settings->change_gamma = change_gamma_cb->isChecked();
		if (settings->change_gamma) {
			gamma_sl->enabled = true;
		}
		else {
			settings->gamma = 1.0;
			gamma_sl->enabled = false;
			gamma_sl->set(GAMMA_MIN, GAMMA_MAX, static_cast<int>(settings->gamma * 10.0));
			render_device->resetGamma();
		}
	}
	else if (gamma_sl->checkClick()) {
		settings->gamma = static_cast<float>(gamma_sl->getValue()) * 0.1f;
		render_device->setGamma(settings->gamma);
	}
	else if (renderer_lstb->checkClick()) {
		new_render_device = renderer_lstb->getValue();
	}
}

void GameStateConfigDesktop::logicInput() {
	if (inpt->joysticks_changed) {
		disableJoystickOptions();
		joystick_device_lstb->clear();
		for(int i = 0; i < inpt->getNumJoysticks(); i++) {
			std::string joystick_name = inpt->getJoystickName(i);
			if (joystick_name != "")
				joystick_device_lstb->append(joystick_name, joystick_name);
		}
		inpt->joysticks_changed = false;
	}

	if (mouse_move_cb->checkClick()) {
		if (mouse_move_cb->isChecked()) {
			settings->mouse_move = true;
			enableMouseOptions();
		}
		else settings->mouse_move=false;
	}
	else if (mouse_aim_cb->checkClick()) {
		if (mouse_aim_cb->isChecked()) {
			settings->mouse_aim = true;
			enableMouseOptions();
		}
		else settings->mouse_aim=false;
	}
	else if (no_mouse_cb->checkClick()) {
		if (no_mouse_cb->isChecked()) {
			settings->no_mouse = true;
			disableMouseOptions();
		}
		else settings->no_mouse = false;
	}
	else if (enable_joystick_cb->checkClick()) {
		if (enable_joystick_cb->isChecked()) {
			settings->enable_joystick = true;
			if (inpt->getNumJoysticks() > 0) {
				settings->joystick_device = 0;
				inpt->initJoystick();
				joystick_device_lstb->select(settings->joystick_device);
			}

			if (inpt->getNumJoysticks() > 0)
				joystick_device_lstb->jumpToSelected();
		}
		else {
			disableJoystickOptions();
		}
	}
	else if (joystick_deadzone_sl->checkClick()) {
		settings->joy_deadzone = joystick_deadzone_sl->getValue();
	}
	else if (joystick_device_lstb->checkClick()) {
		settings->joystick_device = joystick_device_lstb->getSelected();
		if (settings->joystick_device != -1) {
			settings->enable_joystick = true;
			if (inpt->getNumJoysticks() > 0) {
				inpt->initJoystick();
			}
		}
		else {
			settings->enable_joystick = false;
		}
		enable_joystick_cb->setChecked(settings->enable_joystick);
	}
}

void GameStateConfigDesktop::logicKeybinds() {
	input_scrollbox->logic();
	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		if (i >= static_cast<unsigned int>(inpt->KEY_COUNT * 2)) {
			keybinds_btn[i]->enabled = settings->enable_joystick;
			keybinds_btn[i]->refresh();
		}
		Point mouse = input_scrollbox->input_assist(inpt->mouse);
		if (keybinds_btn[i]->checkClickAt(mouse.x,mouse.y)) {
			std::string confirm_msg;
			confirm_msg = msg->get("Assign:") + ' ' + inpt->binding_name[i%key_count];
			delete input_confirm;
			input_confirm = new MenuConfirm(msg->get("Clear"),confirm_msg);
			input_confirm_ticks = settings->max_frames_per_sec * 10; // 10 seconds
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

	if (active_tab == KEYBINDS_TAB && !keybind_msg.empty()) {
		TooltipData keybind_tip_data;
		keybind_tip_data.addText(keybind_msg);

		if (keybind_tip_ticks == 0)
			keybind_tip_ticks = settings->max_frames_per_sec * 5;

		if (keybind_tip_ticks > 0) {
			keybind_tip->render(keybind_tip_data, Point(settings->view_w, 0), TooltipData::STYLE_FLOAT);
			keybind_tip_ticks--;
		}

		if (keybind_tip_ticks == 0) {
			keybind_msg.clear();
		}
	}
	else {
		keybind_msg.clear();
		keybind_tip_ticks = 0;
	}
}

void GameStateConfigDesktop::refreshWidgets() {
	GameStateConfigBase::refreshWidgets();

	input_scrollbox->setPos(frame.x, frame.y);

	input_confirm->align();
}

void GameStateConfigDesktop::confirmKey(int button) {
	inpt->pressing[button] = false;
	inpt->lock[button] = false;

	input_confirm->visible = false;
	input_confirm_ticks = 0;

	updateKeybinds();
}

void GameStateConfigDesktop::scanKey(int button) {
	int column = button / key_count;
	int real_button = button % key_count;

	// clear the keybind if the user clicks "Clear" in the dialog
	if (input_confirm->visible && input_confirm->confirmClicked) {
		inpt->setKeybind(-1, real_button, column, keybind_msg);
		confirmKey(real_button);
		return;
	}

	if (input_confirm->visible && !input_confirm->isWithinButtons) {
		// keyboard & mouse
		if (column == InputState::BINDING_DEFAULT || column == InputState::BINDING_ALT) {
			if (inpt->last_button != -1) {
				// mouse
				inpt->setKeybind(inpt->last_button, real_button, column, keybind_msg);
				confirmKey(real_button);
			}
			else if (inpt->last_key != -1) {
				// keyboard
				inpt->setKeybind(inpt->last_key, real_button, column, keybind_msg);
				confirmKey(real_button);
			}
		}
		// joystick
		else if (column == InputState::BINDING_JOYSTICK && inpt->last_joybutton != -1) {
			inpt->setKeybind(inpt->last_joybutton, real_button, column, keybind_msg);
			confirmKey(real_button);
		}
		else if (column == InputState::BINDING_JOYSTICK && inpt->last_joyaxis != -1) {
			inpt->setKeybind(inpt->last_joyaxis, real_button, column, keybind_msg);
			confirmKey(real_button);
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
	if (keybind_tip != NULL) {
		delete keybind_tip;
		keybind_tip = NULL;
	}
}

void GameStateConfigDesktop::enableMouseOptions() {
	settings->no_mouse = false;
	no_mouse_cb->setChecked(settings->no_mouse);
}

void GameStateConfigDesktop::disableMouseOptions() {
	settings->mouse_aim = false;
	mouse_aim_cb->setChecked(settings->mouse_aim);

	settings->mouse_move = false;
	mouse_move_cb->setChecked(settings->mouse_move);

	settings->no_mouse = true;
	no_mouse_cb->setChecked(settings->no_mouse);
}

void GameStateConfigDesktop::disableJoystickOptions() {
	settings->enable_joystick = false;
	enable_joystick_cb->setChecked(settings->enable_joystick);

	for (int i=0; i<joystick_device_lstb->getSize(); i++)
		joystick_device_lstb->deselect(i);

	if (inpt->getNumJoysticks() > 0)
		joystick_device_lstb->refresh();
}

void GameStateConfigDesktop::refreshRenderers() {
	renderer_lstb->clear();

	std::vector<std::string> rd_name, rd_desc;
	createRenderDeviceList(msg, rd_name, rd_desc);

	for (size_t i = 0; i < rd_name.size(); ++i) {
		renderer_lstb->append(rd_name[i], rd_desc[i]);
		if (rd_name[i] == settings->render_device_name) {
			renderer_lstb->select(static_cast<int>(i));
		}
	}

	renderer_lstb->jumpToSelected();
}
