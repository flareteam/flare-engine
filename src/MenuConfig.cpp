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
 * MenuConfig
 *
 * Handle game Settings Menu
 */

#include "Avatar.h"
#include "CombatText.h"
#include "CommonIncludes.h"
#include "DeviceList.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateTitle.h"
#include "InputState.h"
#include "MenuConfig.h"
#include "MenuConfirm.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "Platform.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Stats.h"
#include "TooltipManager.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "Version.h"
#include "WidgetButton.h"
#include "WidgetCheckBox.h"
#include "WidgetHorizontalList.h"
#include "WidgetListBox.h"
#include "WidgetSlider.h"
#include "WidgetScrollBox.h"
#include "WidgetTabControl.h"

MenuConfig::ConfigOption::ConfigOption()
	: enabled(false)
	, label(NULL)
	, widget(NULL) {
}

MenuConfig::ConfigOption::~ConfigOption() {
}

MenuConfig::ConfigTab::ConfigTab()
	: scrollbox(NULL)
	, enabled_count(0) {
}

MenuConfig::ConfigTab::~ConfigTab() {
}

void MenuConfig::ConfigTab::setOptionWidgets(int index, WidgetLabel* lb, Widget* w, const std::string& lb_text) {
	if (!options[index].enabled) {
		options[index].enabled = true;
		enabled_count++;
	}
	options[index].label = lb;
	options[index].label->setText(lb_text);
	options[index].widget = w;
	options[index].widget->tablist_nav_right = true;
}

void MenuConfig::ConfigTab::setOptionEnabled(int index, bool enable) {
	if (options[index].enabled && !enable) {
		options[index].enabled = false;
		if (enabled_count > 0)
			enabled_count--;
	}
	else if (!options[index].enabled && enable) {
		options[index].enabled = true;
		enabled_count++;
	}
}

int MenuConfig::ConfigTab::getEnabledIndex(int option_index) {
	int r = -1;
	for (size_t i = 0; i < options.size(); ++i) {
		if (options[i].enabled)
			r++;
		if (i == static_cast<size_t>(option_index))
			break;
	}
	return (r == -1 ? 0 : r);
}

MenuConfig::MenuConfig (bool _is_game_state)
	: is_game_state(_is_game_state)
	, enable_gamestate_buttons(false)
	, hero(NULL)
	, child_widget()
	, tab_control(new WidgetTabControl())
	, ok_button(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, defaults_button(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, cancel_button(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, background(NULL)
	, input_confirm(new MenuConfirm(msg->get("Clear"),msg->get("Assign:")))
	, defaults_confirm(new MenuConfirm(msg->get("Defaults"), msg->get("Reset ALL settings?")))

	, pause_continue_lb(new WidgetLabel())
	, pause_continue_btn(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, pause_exit_lb(new WidgetLabel())
	, pause_exit_btn(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, pause_time_lb(new WidgetLabel())
	, pause_time_text(new WidgetLabel())

	, renderer_lstb(new WidgetHorizontalList())
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
	, parallax_layers_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, parallax_layers_lb(new WidgetLabel())
	, change_gamma_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, change_gamma_lb(new WidgetLabel())
	, gamma_sl(new WidgetSlider(WidgetSlider::DEFAULT_FILE))
	, gamma_lb(new WidgetLabel())

	, music_volume_sl(new WidgetSlider(WidgetSlider::DEFAULT_FILE))
	, music_volume_lb(new WidgetLabel())
	, sound_volume_sl(new WidgetSlider(WidgetSlider::DEFAULT_FILE))
	, sound_volume_lb(new WidgetLabel())

	, show_fps_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, show_fps_lb(new WidgetLabel())
	, hardware_cursor_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, hardware_cursor_lb(new WidgetLabel())
	, colorblind_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, colorblind_lb(new WidgetLabel())
	, dev_mode_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, dev_mode_lb(new WidgetLabel())
	, subtitles_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, subtitles_lb(new WidgetLabel())
	, loot_tooltip_lstb(new WidgetHorizontalList())
	, loot_tooltip_lb(new WidgetLabel())
	, minimap_lstb(new WidgetHorizontalList())
	, minimap_lb(new WidgetLabel())
	, statbar_labels_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, statbar_labels_lb(new WidgetLabel())
	, statbar_autohide_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, statbar_autohide_lb(new WidgetLabel())
	, combat_text_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, combat_text_lb(new WidgetLabel())
	, auto_equip_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, auto_equip_lb(new WidgetLabel())
	, entity_markers_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, entity_markers_lb(new WidgetLabel())
	, low_hp_warning_lstb(new WidgetHorizontalList())
	, low_hp_warning_lb(new WidgetLabel())
	, low_hp_threshold_lstb(new WidgetHorizontalList())
	, low_hp_threshold_lb(new WidgetLabel())

	, joystick_device_lstb(new WidgetHorizontalList())
	, joystick_device_lb(new WidgetLabel())
	, mouse_move_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, mouse_move_lb(new WidgetLabel())
	, mouse_aim_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, mouse_aim_lb(new WidgetLabel())
	, no_mouse_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, no_mouse_lb(new WidgetLabel())
	, mouse_move_swap_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, mouse_move_swap_lb(new WidgetLabel())
	, mouse_move_attack_cb(new WidgetCheckBox(WidgetCheckBox::DEFAULT_FILE))
	, mouse_move_attack_lb(new WidgetLabel())
	, joystick_deadzone_sl(new WidgetSlider(WidgetSlider::DEFAULT_FILE))
	, joystick_deadzone_lb(new WidgetLabel())

	, activemods_lstb(new WidgetListBox(10, WidgetListBox::DEFAULT_FILE))
	, activemods_lb(new WidgetLabel())
	, inactivemods_lstb(new WidgetListBox(10, WidgetListBox::DEFAULT_FILE))
	, inactivemods_lb(new WidgetLabel())
	, language_lstb(new WidgetHorizontalList())
	, language_lb(new WidgetLabel())
	, activemods_shiftup_btn(new WidgetButton("images/menus/buttons/up.png"))
	, activemods_shiftdown_btn(new WidgetButton("images/menus/buttons/down.png"))
	, activemods_deactivate_btn(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, inactivemods_activate_btn(new WidgetButton(WidgetButton::DEFAULT_FILE))

	, active_tab(0)
	, frame(0,0)
	, frame_offset(11,8)
	, tab_offset(3,0)
	, scrollpane_color(0,0,0,0)
	, scrollpane_padding(8, 40) // appropriate defaults for fantasycore widget sizes
	, scrollpane_separator_color(font->getColor(FontEngine::COLOR_WIDGET_DISABLED))
	, new_render_device(settings->render_device_name)
	, input_confirm_timer(settings->max_frames_per_sec * 10) // 10 seconds
	, input_key(0)
	, key_count(0)
	, keybind_tip_timer(settings->max_frames_per_sec * 5) // 5 seconds
	, keybind_tip(new WidgetTooltip())
	, clicked_accept(false)
	, clicked_cancel(false)
	, force_refresh_background(false)
	, reload_music(false)
	, clicked_pause_continue(false)
	, clicked_pause_exit(false)
{

	Image *graphics;
	graphics = render_device->loadImage("images/menus/config.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		background = graphics->createSprite();
		graphics->unref();
	}

	ok_button->setLabel(msg->get("OK"));
	defaults_button->setLabel(msg->get("Defaults"));
	cancel_button->setLabel(msg->get("Cancel"));

	pause_continue_btn->setLabel(msg->get("Continue"));
	setPauseExitText(MenuConfig::ENABLE_SAVE_GAME);
	pause_time_text->setText(Utils::getTimeString(0));
	pause_time_text->setJustify(FontEngine::JUSTIFY_RIGHT);
	pause_time_text->setVAlign(LabelInfo::VALIGN_CENTER);

	// Finish Mods ListBoxes setup
	activemods_lstb->multi_select = true;
	for (unsigned int i = 0; i < mods->mod_list.size() ; i++) {
		if (mods->mod_list[i].name != mods->FALLBACK_MOD)
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
		if (!skip_mod && mods->mod_dirs[i] != mods->FALLBACK_MOD) {
			Mod temp_mod = mods->loadMod(mods->mod_dirs[i]);
			inactivemods_lstb->append(mods->mod_dirs[i],createModTooltip(&temp_mod));
		}
	}
	inactivemods_lstb->sort();

	refreshJoysticks();

	// Allocate KeyBindings
	for (int i = 0; i < inpt->KEY_COUNT_USER * 3; i++) {
		keybinds_lb.push_back(new WidgetLabel());
		keybinds_btn.push_back(new WidgetButton(WidgetButton::DEFAULT_FILE));
	}

	key_count = static_cast<unsigned>(keybinds_btn.size()/3);

	// don't allow remapping the primary Main1 binding
	keybinds_btn[Input::MAIN1 * 3]->enabled = false;

	// set up loot tooltip setting
	loot_tooltip_lstb->append(msg->get("Default"), msg->get("Show all loot tooltips, except for those that would be obscured by the player or an enemy. Temporarily show all loot tooltips with 'Alt'."));
	loot_tooltip_lstb->append(msg->get("Show all"), msg->get("Always show loot tooltips. Temporarily hide all loot tooltips with 'Alt'."));
	loot_tooltip_lstb->append(msg->get("Hidden"), msg->get("Always hide loot tooltips, except for when a piece of loot is hovered with the mouse cursor. Temporarily show all loot tooltips with 'Alt'."));

	// set up minimap setting
	minimap_lstb->append(msg->get("Visible"), "");
	minimap_lstb->append(msg->get("Visible (2x zoom)"), "");
	minimap_lstb->append(msg->get("Hidden"), "");

	// set up low hp notification type combinantions
	std::string lhpw_prefix = msg->get("Controls the type of warning to be activated when the player is below the low health threshold.");
	std::string lhpw_warning1 = msg->get("- Display a message");
	std::string lhpw_warning2 = msg->get("- Play a sound");
	std::string lhpw_warning3 = msg->get("- Change the cursor");

	low_hp_warning_lstb->append(msg->get("Disabled"), lhpw_prefix);
	low_hp_warning_lstb->append(msg->get("All"), lhpw_prefix + "\n\n" + lhpw_warning1 + '\n' + lhpw_warning2 + '\n' + lhpw_warning3);
	low_hp_warning_lstb->append(msg->get("Message & Cursor"), lhpw_prefix + "\n\n" + lhpw_warning1 + '\n' + lhpw_warning3);
	low_hp_warning_lstb->append(msg->get("Message & Sound"), lhpw_prefix + "\n\n" + lhpw_warning1 + '\n' + lhpw_warning2);
	low_hp_warning_lstb->append(msg->get("Sound & Cursor"), lhpw_prefix + "\n\n" + lhpw_warning2 + '\n' + lhpw_warning3);
	low_hp_warning_lstb->append(msg->get("Message"), lhpw_prefix + "\n\n" + lhpw_warning1);
	low_hp_warning_lstb->append(msg->get("Cursor"), lhpw_prefix + "\n\n" + lhpw_warning3);
	low_hp_warning_lstb->append(msg->get("Sound"), lhpw_prefix + "\n\n" + lhpw_warning2);

	// set up low hp threshold combo
	for (unsigned int i = 1; i <= 10 ; ++i) {
		std::stringstream ss;
		ss << i * 5;
		low_hp_threshold_lstb->append(ss.str() + "%", msg->get("When the player's health drops below the given threshold, the low health notifications are triggered if one or more of them is enabled."));
	}

	init();

	render_device->setBackgroundColor(Color(0,0,0,0));
}

MenuConfig::~MenuConfig() {
	cleanup();
}

void MenuConfig::init() {
	tab_control->setTabTitle(EXIT_TAB, msg->get("Exit"));
	tab_control->setTabTitle(VIDEO_TAB, msg->get("Video"));
	tab_control->setTabTitle(AUDIO_TAB, msg->get("Audio"));
	tab_control->setTabTitle(INTERFACE_TAB, msg->get("Interface"));
	tab_control->setTabTitle(INPUT_TAB, msg->get("Input"));
	tab_control->setTabTitle(KEYBINDS_TAB, msg->get("Keybindings"));
	tab_control->setTabTitle(MODS_TAB, msg->get("Mods"));

	readConfig();

	cfg_tabs.resize(6);
	cfg_tabs[EXIT_TAB].options.resize(3);
	cfg_tabs[VIDEO_TAB].options.resize(Platform::Video::COUNT);
	cfg_tabs[AUDIO_TAB].options.resize(Platform::Audio::COUNT);
	cfg_tabs[INTERFACE_TAB].options.resize(Platform::Interface::COUNT);
	cfg_tabs[INPUT_TAB].options.resize(Platform::Input::COUNT);
	cfg_tabs[KEYBINDS_TAB].options.resize(inpt->KEY_COUNT_USER * 3);

	cfg_tabs[EXIT_TAB].setOptionWidgets(0, pause_continue_lb, pause_continue_btn, msg->get("Paused"));
	cfg_tabs[EXIT_TAB].setOptionWidgets(1, pause_exit_lb, pause_exit_btn, "");
	cfg_tabs[EXIT_TAB].setOptionWidgets(2, pause_time_lb, pause_time_text, msg->get("Time Played"));

	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::RENDERER, renderer_lb, renderer_lstb, msg->get("Renderer"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::FULLSCREEN, fullscreen_lb, fullscreen_cb, msg->get("Full Screen Mode"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::HWSURFACE, hwsurface_lb, hwsurface_cb, msg->get("Hardware surfaces"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::VSYNC, vsync_lb, vsync_cb, msg->get("V-Sync"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::TEXTURE_FILTER, texture_filter_lb, texture_filter_cb, msg->get("Texture Filtering"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::DPI_SCALING, dpi_scaling_lb, dpi_scaling_cb, msg->get("DPI scaling"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::PARALLAX_LAYERS, parallax_layers_lb, parallax_layers_cb, msg->get("Parallax Layers"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::ENABLE_GAMMA, change_gamma_lb, change_gamma_cb, msg->get("Allow changing gamma"));
	cfg_tabs[VIDEO_TAB].setOptionWidgets(Platform::Video::GAMMA, gamma_lb, gamma_sl, msg->get("Gamma"));

	cfg_tabs[AUDIO_TAB].setOptionWidgets(Platform::Audio::SFX, sound_volume_lb, sound_volume_sl, msg->get("Sound Volume"));
	cfg_tabs[AUDIO_TAB].setOptionWidgets(Platform::Audio::MUSIC, music_volume_lb, music_volume_sl, msg->get("Music Volume"));

	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::LANGUAGE, language_lb, language_lstb, msg->get("Language"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::SHOW_FPS, show_fps_lb, show_fps_cb, msg->get("Show FPS"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::HARDWARE_CURSOR, hardware_cursor_lb, hardware_cursor_cb, msg->get("Hardware mouse cursor"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::COLORBLIND, colorblind_lb, colorblind_cb, msg->get("Colorblind Mode"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::DEV_MODE, dev_mode_lb, dev_mode_cb, msg->get("Developer Mode"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::SUBTITLES, subtitles_lb, subtitles_cb, msg->get("Subtitles"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::LOOT_TOOLTIPS, loot_tooltip_lb, loot_tooltip_lstb, msg->get("Loot tooltip visibility"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::MINIMAP_MODE, minimap_lb, minimap_lstb, msg->get("Mini-map mode"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::STATBAR_LABELS, statbar_labels_lb, statbar_labels_cb, msg->get("Always show stat bar labels"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::STATBAR_AUTOHIDE, statbar_autohide_lb, statbar_autohide_cb, msg->get("Allow stat bar auto-hiding"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::COMBAT_TEXT, combat_text_lb, combat_text_cb, msg->get("Show combat text"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::AUTO_EQUIP, auto_equip_lb, auto_equip_cb, msg->get("Automatically equip items"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::ENTITY_MARKERS, entity_markers_lb, entity_markers_cb, msg->get("Show hidden entity markers"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::LOW_HP_WARNING_TYPE, low_hp_warning_lb, low_hp_warning_lstb, msg->get("Low health notification"));
	cfg_tabs[INTERFACE_TAB].setOptionWidgets(Platform::Interface::LOW_HP_THRESHOLD, low_hp_threshold_lb, low_hp_threshold_lstb, msg->get("Low health threshold"));


	cfg_tabs[INPUT_TAB].setOptionWidgets(Platform::Input::JOYSTICK, joystick_device_lb, joystick_device_lstb, msg->get("Joystick"));
	cfg_tabs[INPUT_TAB].setOptionWidgets(Platform::Input::MOUSE_MOVE, mouse_move_lb, mouse_move_cb, msg->get("Move hero using mouse"));
	cfg_tabs[INPUT_TAB].setOptionWidgets(Platform::Input::MOUSE_AIM, mouse_aim_lb, mouse_aim_cb, msg->get("Mouse aim"));
	cfg_tabs[INPUT_TAB].setOptionWidgets(Platform::Input::NO_MOUSE, no_mouse_lb, no_mouse_cb, msg->get("Do not use mouse"));
	cfg_tabs[INPUT_TAB].setOptionWidgets(Platform::Input::MOUSE_MOVE_SWAP, mouse_move_swap_lb, mouse_move_swap_cb, msg->get("Swap mouse movement button"));
	cfg_tabs[INPUT_TAB].setOptionWidgets(Platform::Input::MOUSE_MOVE_ATTACK, mouse_move_attack_lb, mouse_move_attack_cb, msg->get("Attack with mouse movement"));
	cfg_tabs[INPUT_TAB].setOptionWidgets(Platform::Input::JOYSTICK_DEADZONE, joystick_deadzone_lb, joystick_deadzone_sl, msg->get("Joystick Deadzone"));


	for (size_t i = 0; i < keybinds_btn.size(); ++i) {
		if (i % 3 == 0) {
			cfg_tabs[KEYBINDS_TAB].setOptionWidgets(static_cast<int>(i), keybinds_lb[i], keybinds_btn[i], inpt->binding_name[i/3]);
			// TODO since these are blank, don't allocate?
			cfg_tabs[KEYBINDS_TAB].setOptionWidgets(static_cast<int>(i+1), keybinds_lb[i+1], keybinds_btn[i+1], "");
			cfg_tabs[KEYBINDS_TAB].setOptionWidgets(static_cast<int>(i+2), keybinds_lb[i+2], keybinds_btn[i+2], "");

			keybinds_btn[i]->tooltip = msg->get("Primary binding: %s", inpt->binding_name[i/3].c_str());
			keybinds_btn[i+1]->tooltip = msg->get("Alternate binding: %s", inpt->binding_name[i/3].c_str());
			keybinds_btn[i+2]->tooltip = msg->get("Joystick binding: %s", inpt->binding_name[i/3].c_str());
		}
	}

	// disable some options
	if (!is_game_state) {
		cfg_tabs[VIDEO_TAB].setOptionEnabled(Platform::Video::RENDERER, false);
		cfg_tabs[VIDEO_TAB].setOptionEnabled(Platform::Video::HWSURFACE, false);
		cfg_tabs[VIDEO_TAB].setOptionEnabled(Platform::Video::VSYNC, false);
		cfg_tabs[VIDEO_TAB].setOptionEnabled(Platform::Video::TEXTURE_FILTER, false);

		cfg_tabs[INTERFACE_TAB].setOptionEnabled(Platform::Interface::LANGUAGE, false);
		cfg_tabs[INTERFACE_TAB].setOptionEnabled(Platform::Interface::DEV_MODE, false);

		tab_control->setEnabled(static_cast<unsigned>(MODS_TAB), false);
		tab_control->setEnabled(static_cast<unsigned>(EXIT_TAB), true);
		enable_gamestate_buttons = false;
	}
	else {
		cfg_tabs[EXIT_TAB].setOptionEnabled(0, false);
		cfg_tabs[EXIT_TAB].setOptionEnabled(1, false);
		tab_control->setEnabled(static_cast<unsigned>(EXIT_TAB), false);
		enable_gamestate_buttons = true;
	}

	// disable platform-specific options
	for (int i = 0; i < Platform::Video::COUNT; ++i) {
		if (!platform.config_video[i])
			cfg_tabs[VIDEO_TAB].setOptionEnabled(i, false);
	}
	for (int i = 0; i < Platform::Audio::COUNT; ++i) {
		if (!platform.config_audio[i])
			cfg_tabs[AUDIO_TAB].setOptionEnabled(i, false);
	}
	for (int i = 0; i < Platform::Interface::COUNT; ++i) {
		if (!platform.config_interface[i])
			cfg_tabs[INTERFACE_TAB].setOptionEnabled(i, false);
	}
	for (int i = 0; i < Platform::Input::COUNT; ++i) {
		if (!platform.config_input[i])
			cfg_tabs[INPUT_TAB].setOptionEnabled(i, false);
	}
	if (!platform.config_misc[Platform::Misc::KEYBINDS]) {
		for (size_t i = 0; i < keybinds_btn.size(); ++i) {
			cfg_tabs[KEYBINDS_TAB].setOptionEnabled(static_cast<int>(i), false);
		}
	}
	if (!platform.config_misc[Platform::Misc::MODS]) {
		tab_control->setEnabled(static_cast<unsigned>(MODS_TAB), false);
	}

	// place widgets
	for (size_t i = 0; i < cfg_tabs.size(); ++i) {
		if (cfg_tabs[i].enabled_count == 0) {
			tab_control->setEnabled(static_cast<unsigned>(i), false);
		}

		// set up scrollbox
		cfg_tabs[i].scrollbox = new WidgetScrollBox(scrollpane.w, scrollpane.h);
		cfg_tabs[i].scrollbox->setBasePos(scrollpane.x, scrollpane.y, Utils::ALIGN_TOPLEFT);
		cfg_tabs[i].scrollbox->bg = scrollpane_color;
		cfg_tabs[i].scrollbox->resize(scrollpane.w, cfg_tabs[i].enabled_count * scrollpane_padding.y);

		for (size_t j = 0; j < cfg_tabs[i].options.size(); ++j) {
			placeLabeledWidgetAuto(static_cast<int>(i), static_cast<int>(j));
		}
	}

	addChildWidgets();
	setupTabList();

	refreshWidgets();

	update();
}

void MenuConfig::readConfig() {
	//Load the menu configuration from file

	FileParser infile;
	if (infile.open("menus/config.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (parseKeyButtons(infile))
				continue;

			int x1 = Parse::popFirstInt(infile.val);
			int y1 = Parse::popFirstInt(infile.val);
			int x2 = Parse::popFirstInt(infile.val);
			int y2 = Parse::popFirstInt(infile.val);

			if (parseKey(infile, x1, y1, x2, y2))
				continue;
			else
				infile.error("MenuConfig: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// add description tooltips
	hwsurface_cb->tooltip = msg->get("Will try to store surfaces in video memory versus system memory. The effect this has on performance depends on the renderer.");
	vsync_cb->tooltip = msg->get("Prevents screen tearing. Disable if you experience \"stuttering\" in windowed mode or input lag.");
	dpi_scaling_cb->tooltip = msg->get("When enabled, this uses the screen DPI in addition to the window dimensions to scale the rendering resolution. Otherwise, only the window dimensions are used.");
	parallax_layers_cb->tooltip = msg->get("This enables parallax (non-tile) layers. Disabling this setting can improve performance in some cases.");
	change_gamma_cb->tooltip = msg->get("Enables the below setting that controls the screen gamma level. The behavior of the gamma setting can vary between platforms.");
	colorblind_cb->tooltip = msg->get("Provides additional text for information that is primarily conveyed through color.");
	statbar_autohide_cb->tooltip = msg->get("Some mods will automatically hide the stat bars when they are inactive. Disabling this option will keep them displayed at all times.");
	auto_equip_cb->tooltip = msg->get("When enabled, empty equipment slots will be filled with applicable items when they are obtained.");
	entity_markers_cb->tooltip = msg->get("Shows a marker above enemies, allies, and the player when they are obscured by tall objects.");
	no_mouse_cb->tooltip = msg->get("This allows the game to be controlled entirely with the keyboard (or joystick).");
	mouse_move_swap_cb->tooltip = msg->get("When 'Move hero using mouse' is enabled, this setting controls if 'Main1' or 'Main2' is used to move the hero. If enabled, 'Main2' will move the hero instead of 'Main1'.");
	mouse_move_attack_cb->tooltip = msg->get("When 'Move hero using mouse' is enabled, this setting controls if the Power assigned to the movement button can be used by targeting an enemy. If this setting is disabled, it is required to use 'Shift' to access the Power assigned to the movement button.");
	mouse_aim_cb->tooltip = msg->get("The player's attacks will be aimed in the direction of the mouse cursor when this is enabled.");
}

bool MenuConfig::parseKeyButtons(FileParser &infile) {
	// @CLASS MenuConfig|Description of menus/config.txt

	if (infile.key == "button_ok") {
		// @ATTR button_ok|int, int, alignment : X, Y, Alignment|Position of the "OK" button. Not used in the pause menu.
		int x = Parse::popFirstInt(infile.val);
		int y = Parse::popFirstInt(infile.val);
		int a = Parse::toAlignment(Parse::popFirstString(infile.val));
		ok_button->setBasePos(x, y, a);
	}
	else if (infile.key == "button_defaults") {
		// @ATTR button_defaults|int, int, alignment : X, Y, Alignment|Position of the "Defaults" button. Not used in the pause menu.
		int x = Parse::popFirstInt(infile.val);
		int y = Parse::popFirstInt(infile.val);
		int a = Parse::toAlignment(Parse::popFirstString(infile.val));
		defaults_button->setBasePos(x, y, a);
	}
	else if (infile.key == "button_cancel") {
		// @ATTR button_cancel|int, int, alignment : X, Y, Alignment|Position of the "Cancel" button. Not used in the pause menu.
		int x = Parse::popFirstInt(infile.val);
		int y = Parse::popFirstInt(infile.val);
		int a = Parse::toAlignment(Parse::popFirstString(infile.val));
		cancel_button->setBasePos(x, y, a);
	}
	else {
		return false;
	}

	return true;
}
bool MenuConfig::parseKey(FileParser &infile, int &x1, int &y1, int &x2, int &y2) {
	if (infile.key == "listbox_scrollbar_offset") {
		// @ATTR listbox_scrollbar_offset|int|Horizontal offset from the right of listboxes (mods, languages, etc) to place the scrollbar.
		activemods_lstb->scrollbar_offset = x1;
		inactivemods_lstb->scrollbar_offset = x1;
	}
	else if (infile.key == "frame_offset") {
		// @ATTR frame_offset|point|Offset for all the widgets contained under each tab.
		frame_offset.x = x1;
		frame_offset.y = y1;
	}
	else if (infile.key == "tab_offset") {
		// @ATTR tab_offset|point|Offset for the row of tabs.
		tab_offset.x = x1;
		tab_offset.y = y1;
	}
	else if (infile.key == "activemods") {
		// @ATTR activemods|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Active Mods" list box relative to the frame.
		placeLabeledWidget(activemods_lb, activemods_lstb, x1, y1, x2, y2, msg->get("Active Mods"));
		activemods_lb->setJustify(FontEngine::JUSTIFY_CENTER);
	}
	else if (infile.key == "activemods_height") {
		// @ATTR activemods_height|int|Number of visible rows for the "Active Mods" list box.
		activemods_lstb->setHeight(x1);
	}
	else if (infile.key == "inactivemods") {
		// @ATTR inactivemods|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Available Mods" list box relative to the frame.
		placeLabeledWidget(inactivemods_lb, inactivemods_lstb, x1, y1, x2, y2, msg->get("Available Mods"));
		inactivemods_lb->setJustify(FontEngine::JUSTIFY_CENTER);
	}
	else if (infile.key == "inactivemods_height") {
		// @ATTR inactivemods_height|int|Number of visible rows for the "Available Mods" list box.
		inactivemods_lstb->setHeight(x1);
	}
	else if (infile.key == "activemods_shiftup") {
		// @ATTR activemods_shiftup|point|Position of the button to shift mods up in "Active Mods" relative to the frame.
		activemods_shiftup_btn->setBasePos(x1, y1, Utils::ALIGN_TOPLEFT);
		activemods_shiftup_btn->refresh();
	}
	else if (infile.key == "activemods_shiftdown") {
		// @ATTR activemods_shiftdown|point|Position of the button to shift mods down in "Active Mods" relative to the frame.
		activemods_shiftdown_btn->setBasePos(x1, y1, Utils::ALIGN_TOPLEFT);
		activemods_shiftdown_btn->refresh();
	}
	else if (infile.key == "activemods_deactivate") {
		// @ATTR activemods_deactivate|point|Position of the "Disable" button relative to the frame.
		activemods_deactivate_btn->setLabel(msg->get("<< Disable"));
		activemods_deactivate_btn->setBasePos(x1, y1, Utils::ALIGN_TOPLEFT);
		activemods_deactivate_btn->refresh();
	}
	else if (infile.key == "inactivemods_activate") {
		// @ATTR inactivemods_activate|point|Position of the "Enable" button relative to the frame.
		inactivemods_activate_btn->setLabel(msg->get("Enable >>"));
		inactivemods_activate_btn->setBasePos(x1, y1, Utils::ALIGN_TOPLEFT);
		inactivemods_activate_btn->refresh();
	}
	else if (infile.key == "scrollpane") {
		// @ATTR scrollpane|rectangle|Position of the keybinding scrollbox relative to the frame.
		scrollpane.x = x1;
		scrollpane.y = y1;
		scrollpane.w = x2;
		scrollpane.h = y2;
	}
	else if (infile.key == "scrollpane_padding") {
		// @ATTR scrollpane_padding|int, int : Horizontal padding, Vertical padding|Pixel padding for each item listed in a tab's scroll box.
		scrollpane_padding.x = x1;
		scrollpane_padding.y = y1;
	}
	else if (infile.key == "scrollpane_separator_color") {
		// @ATTR scrollpane_separator_color|color|Color of the separator line in between scroll box items.
		scrollpane_separator_color.r = static_cast<Uint8>(x1);
		scrollpane_separator_color.g = static_cast<Uint8>(y1);
		scrollpane_separator_color.b = static_cast<Uint8>(x2);
	}
	else if (infile.key == "scrollpane_bg_color") {
		// @ATTR keybinds_bg_color|color, int: Color, Alpha|Background color and alpha for the keybindings scrollbox.
		scrollpane_color.r = static_cast<Uint8>(x1);
		scrollpane_color.g = static_cast<Uint8>(y1);
		scrollpane_color.b = static_cast<Uint8>(x2);
		scrollpane_color.a = static_cast<Uint8>(y2);
	}

	else return false;

	return true;
}

void MenuConfig::addChildWidgets() {
	for (size_t i = 0; i < cfg_tabs.size(); ++i) {
		for (size_t j = 0; j < cfg_tabs[i].options.size(); ++j) {
			if (cfg_tabs[i].options[j].enabled) {
				cfg_tabs[i].scrollbox->addChildWidget(cfg_tabs[i].options[j].widget);
				cfg_tabs[i].scrollbox->addChildWidget(cfg_tabs[i].options[j].label);
			}

			// only for memory management
			addChildWidget(cfg_tabs[i].options[j].widget, NO_TAB);
			addChildWidget(cfg_tabs[i].options[j].label, NO_TAB);
		}
	}

	addChildWidget(activemods_lstb, MODS_TAB);
	addChildWidget(activemods_lb, MODS_TAB);
	addChildWidget(inactivemods_lstb, MODS_TAB);
	addChildWidget(inactivemods_lb, MODS_TAB);
	addChildWidget(activemods_shiftup_btn, MODS_TAB);
	addChildWidget(activemods_shiftdown_btn, MODS_TAB);
	addChildWidget(activemods_deactivate_btn, MODS_TAB);
	addChildWidget(inactivemods_activate_btn, MODS_TAB);
}

void MenuConfig::setupTabList() {
	tablist.add(tab_control);
	tablist.setPrevTabList(&tablist_main);

	if (enable_gamestate_buttons) {
		tablist_main.add(ok_button);
		tablist_main.add(defaults_button);
		tablist_main.add(cancel_button);
	}
	tablist_main.setPrevTabList(&tablist);
	tablist_main.setNextTabList(&tablist);
	tablist_main.lock();

	tablist_exit.setScrollType(Widget::SCROLL_VERTICAL);
	tablist_exit.add(cfg_tabs[EXIT_TAB].scrollbox);
	tablist_exit.setPrevTabList(&tablist);
	tablist_exit.setNextTabList(&tablist_main);
	tablist_exit.lock();

	tablist_video.setScrollType(Widget::SCROLL_VERTICAL);
	tablist_video.add(cfg_tabs[VIDEO_TAB].scrollbox);
	tablist_video.setPrevTabList(&tablist);
	tablist_video.setNextTabList(&tablist_main);
	tablist_video.lock();

	tablist_audio.setScrollType(Widget::SCROLL_VERTICAL);
	tablist_audio.add(cfg_tabs[AUDIO_TAB].scrollbox);
	tablist_audio.setPrevTabList(&tablist);
	tablist_audio.setNextTabList(&tablist_main);
	tablist_audio.lock();

	tablist_interface.setScrollType(Widget::SCROLL_VERTICAL);
	tablist_interface.add(cfg_tabs[INTERFACE_TAB].scrollbox);
	tablist_interface.setPrevTabList(&tablist);
	tablist_interface.setNextTabList(&tablist_main);
	tablist_interface.lock();

	tablist_input.setScrollType(Widget::SCROLL_VERTICAL);
	tablist_input.add(cfg_tabs[INPUT_TAB].scrollbox);
	tablist_input.setPrevTabList(&tablist);
	tablist_input.setNextTabList(&tablist_main);
	tablist_input.lock();

	tablist_keybinds.setScrollType(Widget::SCROLL_VERTICAL);
	tablist_keybinds.add(cfg_tabs[KEYBINDS_TAB].scrollbox);
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

void MenuConfig::update() {
	updateVideo();
	updateAudio();
	updateInterface();
	updateInput();
	updateKeybinds();
	updateMods();
}

void MenuConfig::updateVideo() {
	fullscreen_cb->setChecked(settings->fullscreen);
	hwsurface_cb->setChecked(settings->hwsurface);
	vsync_cb->setChecked(settings->vsync);
	texture_filter_cb->setChecked(settings->texture_filter);
	dpi_scaling_cb->setChecked(settings->dpi_scaling);
	parallax_layers_cb->setChecked(settings->parallax_layers);
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

	cfg_tabs[VIDEO_TAB].scrollbox->refresh();
}

void MenuConfig::updateAudio() {
	if (settings->audio) {
		music_volume_sl->set(0, 128, settings->music_volume);
		snd->setVolumeMusic(settings->music_volume);
		sound_volume_sl->set(0, 128, settings->sound_volume);
		snd->setVolumeSFX(settings->sound_volume);
	}
	else {
		music_volume_sl->set(0,128,0);
		sound_volume_sl->set(0,128,0);
	}

	cfg_tabs[AUDIO_TAB].scrollbox->refresh();
}

void MenuConfig::updateInterface() {
	show_fps_cb->setChecked(settings->show_fps);
	colorblind_cb->setChecked(settings->colorblind);
	hardware_cursor_cb->setChecked(settings->hardware_cursor);
	dev_mode_cb->setChecked(settings->dev_mode);
	subtitles_cb->setChecked(settings->subtitles);
	statbar_labels_cb->setChecked(settings->statbar_labels);
	statbar_autohide_cb->setChecked(settings->statbar_autohide);
	combat_text_cb->setChecked(settings->combat_text);
	auto_equip_cb->setChecked(settings->auto_equip);
	entity_markers_cb->setChecked(settings->entity_markers);
	low_hp_warning_lstb->select(settings->low_hp_warning_type);
	low_hp_threshold_lstb->select((settings->low_hp_threshold/5)-1);

	loot_tooltip_lstb->select(settings->loot_tooltips);
	minimap_lstb->select(settings->minimap_mode);

	refreshLanguages();

	cfg_tabs[INTERFACE_TAB].scrollbox->refresh();
}

void MenuConfig::updateInput() {
	mouse_aim_cb->setChecked(settings->mouse_aim);
	no_mouse_cb->setChecked(settings->no_mouse);
	mouse_move_cb->setChecked(settings->mouse_move);
	mouse_move_swap_cb->setChecked(settings->mouse_move_swap);
	mouse_move_attack_cb->setChecked(settings->mouse_move_attack);

	if (settings->enable_joystick && inpt->getNumJoysticks() > 0) {
		inpt->initJoystick();
		joystick_device_lstb->select(settings->joystick_device + 1);
	}

	joystick_deadzone_sl->set(0, 32768, settings->joy_deadzone);

	cfg_tabs[INPUT_TAB].scrollbox->refresh();
}

void MenuConfig::updateKeybinds() {
	// now do labels for keybinds that are set
	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		if (i % 3 == 0) {
			keybinds_btn[i]->setLabel(inpt->getBindingString(i/3));
			keybinds_btn[i+1]->setLabel(inpt->getBindingString(i/3, InputState::BINDING_ALT));
			keybinds_btn[i+2]->setLabel(inpt->getBindingString(i/3, InputState::BINDING_JOYSTICK));

			keybinds_btn[i]->refresh();
			keybinds_btn[i+1]->refresh();
			keybinds_btn[i+2]->refresh();
		}
	}
	cfg_tabs[KEYBINDS_TAB].scrollbox->refresh();
}

void MenuConfig::updateMods() {
	activemods_lstb->refresh();
	inactivemods_lstb->refresh();
}

void MenuConfig::logic() {
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
		input_confirm_timer.tick();
		if (input_confirm_timer.isEnd())
			input_confirm->visible = false;
		return;
	}
	else {
		if (!logicMain())
			return;
	}

	// tab contents
	active_tab = tab_control->getActiveTab();

	if (active_tab == EXIT_TAB) {
		if (hero) {
			pause_time_text->setText(Utils::getTimeString(hero->time_played));
		}
		tablist.setNextTabList(&tablist_exit);
		logicExit();
	}
	if (active_tab == VIDEO_TAB) {
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

		// TODO remove this?
		if (platform.force_hardware_cursor) {
			// for some platforms, hardware mouse cursor can not be turned off
			settings->hardware_cursor = true;
			hardware_cursor_cb->setChecked(settings->hardware_cursor);
		}
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

bool MenuConfig::logicMain() {
	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (child_widget[i]->in_focus && optiontab[i] != NO_TAB) {
			tab_control->setActiveTab(optiontab[i]);
			break;
		}
	}

	// tabs & the bottom 3 main buttons
	tab_control->logic();
	tablist.logic();
	tablist_main.logic();
	tablist_exit.logic();
	tablist_video.logic();
	tablist_audio.logic();
	tablist_interface.logic();
	tablist_input.logic();
	tablist_keybinds.logic();
	tablist_mods.logic();

	if (enable_gamestate_buttons) {
		// Ok/Cancel Buttons
		if (ok_button->checkClick()) {
			clicked_accept = true;

			// MenuConfig deconstructed, proceed with caution
			return false;
		}
		else if (defaults_button->checkClick()) {
			defaults_confirm->visible = true;
			return true;
		}
		else if (cancel_button->checkClick() || (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL])) {
			clicked_cancel = true;

			// MenuConfig deconstructed, proceed with caution
			return false;
		}
	}

	return true;
}

void MenuConfig::logicDefaults() {
	defaults_confirm->logic();
	if (defaults_confirm->confirmClicked) {
		settings->fullscreen = false;
		settings->loadDefaults();
		eset->load();
		inpt->defaultQwertyKeyBindings();
		inpt->defaultJoystickBindings();
		update();
		render_device->windowResize();
		defaults_confirm->visible = false;
		defaults_confirm->confirmClicked = false;
	}
}

void MenuConfig::logicExit() {
	cfg_tabs[EXIT_TAB].scrollbox->logic();
	Point mouse = cfg_tabs[EXIT_TAB].scrollbox->input_assist(inpt->mouse);

	if (cfg_tabs[EXIT_TAB].options[0].enabled && pause_continue_btn->checkClickAt(mouse.x, mouse.y)) {
		clicked_pause_continue = true;
	}
	else if (cfg_tabs[EXIT_TAB].options[1].enabled && pause_exit_btn->checkClickAt(mouse.x, mouse.y)) {
		clicked_pause_exit = true;
	}
}

void MenuConfig::logicVideo() {
	cfg_tabs[VIDEO_TAB].scrollbox->logic();
	Point mouse = cfg_tabs[VIDEO_TAB].scrollbox->input_assist(inpt->mouse);

	if (cfg_tabs[VIDEO_TAB].options[Platform::Video::FULLSCREEN].enabled && fullscreen_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->fullscreen = fullscreen_cb->isChecked();
		render_device->setFullscreen(settings->fullscreen);
		inpt->window_resized = true;
		refreshWidgets();
	}
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::HWSURFACE].enabled && hwsurface_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->hwsurface = hwsurface_cb->isChecked();
	}
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::VSYNC].enabled && vsync_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->vsync = vsync_cb->isChecked();
	}
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::TEXTURE_FILTER].enabled && texture_filter_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->texture_filter = texture_filter_cb->isChecked();
	}
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::DPI_SCALING].enabled && dpi_scaling_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->dpi_scaling = dpi_scaling_cb->isChecked();
		render_device->windowResize();
		inpt->window_resized = true;
		refreshWidgets();
		force_refresh_background = true;
	}
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::PARALLAX_LAYERS].enabled && parallax_layers_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->parallax_layers = parallax_layers_cb->isChecked();
	}
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::ENABLE_GAMMA].enabled && change_gamma_cb->checkClickAt(mouse.x, mouse.y)) {
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
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::GAMMA].enabled && gamma_sl->checkClickAt(mouse.x, mouse.y)) {
		settings->gamma = static_cast<float>(gamma_sl->getValue()) * 0.1f;
		render_device->setGamma(settings->gamma);
	}
	else if (cfg_tabs[VIDEO_TAB].options[Platform::Video::RENDERER].enabled && renderer_lstb->checkClickAt(mouse.x, mouse.y)) {
		new_render_device = renderer_lstb->getValue();
	}
}

void MenuConfig::logicAudio() {
	cfg_tabs[AUDIO_TAB].scrollbox->logic();
	Point mouse = cfg_tabs[AUDIO_TAB].scrollbox->input_assist(inpt->mouse);

	if (settings->audio) {
		if (cfg_tabs[AUDIO_TAB].options[Platform::Audio::MUSIC].enabled && music_volume_sl->checkClickAt(mouse.x, mouse.y)) {
			if (settings->music_volume == 0)
				reload_music = true;
			settings->music_volume = static_cast<short>(music_volume_sl->getValue());
			snd->setVolumeMusic(settings->music_volume);
		}
		else if (cfg_tabs[AUDIO_TAB].options[Platform::Audio::SFX].enabled && sound_volume_sl->checkClickAt(mouse.x, mouse.y)) {
			settings->sound_volume = static_cast<short>(sound_volume_sl->getValue());
			snd->setVolumeSFX(settings->sound_volume);
		}
	}
}

void MenuConfig::logicInterface() {
	cfg_tabs[INTERFACE_TAB].scrollbox->logic();
	Point mouse = cfg_tabs[INTERFACE_TAB].scrollbox->input_assist(inpt->mouse);

	if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::LANGUAGE].enabled && language_lstb->checkClickAt(mouse.x, mouse.y)) {
		unsigned lang_id = language_lstb->getSelected();
		if (lang_id != language_lstb->getSize())
			settings->language = language_ISO[lang_id];
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::SHOW_FPS].enabled && show_fps_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->show_fps = show_fps_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::COLORBLIND].enabled && colorblind_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->colorblind = colorblind_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::HARDWARE_CURSOR].enabled && hardware_cursor_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->hardware_cursor = hardware_cursor_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::DEV_MODE].enabled && dev_mode_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->dev_mode = dev_mode_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::SUBTITLES].enabled && subtitles_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->subtitles = subtitles_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::LOOT_TOOLTIPS].enabled && loot_tooltip_lstb->checkClickAt(mouse.x, mouse.y)) {
		settings->loot_tooltips = static_cast<int>(loot_tooltip_lstb->getSelected());
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::MINIMAP_MODE].enabled && minimap_lstb->checkClickAt(mouse.x, mouse.y)) {
		settings->minimap_mode = static_cast<int>(minimap_lstb->getSelected());
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::STATBAR_LABELS].enabled && statbar_labels_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->statbar_labels = statbar_labels_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::STATBAR_AUTOHIDE].enabled && statbar_autohide_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->statbar_autohide = statbar_autohide_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::COMBAT_TEXT].enabled && combat_text_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->combat_text = combat_text_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::AUTO_EQUIP].enabled && auto_equip_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->auto_equip = auto_equip_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::ENTITY_MARKERS].enabled && entity_markers_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->entity_markers = entity_markers_cb->isChecked();
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::LOW_HP_WARNING_TYPE].enabled && low_hp_warning_lstb->checkClickAt(mouse.x, mouse.y)) {
		settings->low_hp_warning_type = (static_cast<int>(low_hp_warning_lstb->getSelected()));
	}
	else if (cfg_tabs[INTERFACE_TAB].options[Platform::Interface::LOW_HP_THRESHOLD].enabled && low_hp_threshold_lstb->checkClickAt(mouse.x, mouse.y)) {
		settings->low_hp_threshold = (static_cast<int>(low_hp_threshold_lstb->getSelected())+1)*5;
	}
}

void MenuConfig::logicInput() {
	cfg_tabs[INPUT_TAB].scrollbox->logic();
	Point mouse = cfg_tabs[INPUT_TAB].scrollbox->input_assist(inpt->mouse);

	if (inpt->joysticks_changed) {
		disableJoystickOptions();
		refreshJoysticks();
		inpt->joysticks_changed = false;
	}

	if (cfg_tabs[INPUT_TAB].options[Platform::Input::MOUSE_MOVE].enabled && mouse_move_cb->checkClickAt(mouse.x, mouse.y)) {
		if (mouse_move_cb->isChecked()) {
			settings->mouse_move = true;
			enableMouseOptions();
		}
		else settings->mouse_move=false;
	}
	else if (cfg_tabs[INPUT_TAB].options[Platform::Input::MOUSE_AIM].enabled && mouse_aim_cb->checkClickAt(mouse.x, mouse.y)) {
		if (mouse_aim_cb->isChecked()) {
			settings->mouse_aim = true;
			enableMouseOptions();
		}
		else settings->mouse_aim=false;
	}
	else if (cfg_tabs[INPUT_TAB].options[Platform::Input::NO_MOUSE].enabled && no_mouse_cb->checkClickAt(mouse.x, mouse.y)) {
		if (no_mouse_cb->isChecked()) {
			settings->no_mouse = true;
			disableMouseOptions();
		}
		else settings->no_mouse = false;
	}
	else if (cfg_tabs[INPUT_TAB].options[Platform::Input::MOUSE_MOVE_SWAP].enabled && mouse_move_swap_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->mouse_move_swap = mouse_move_swap_cb->isChecked();
	}
	else if (cfg_tabs[INPUT_TAB].options[Platform::Input::MOUSE_MOVE_ATTACK].enabled && mouse_move_attack_cb->checkClickAt(mouse.x, mouse.y)) {
		settings->mouse_move_attack = mouse_move_attack_cb->isChecked();
	}
	else if (cfg_tabs[INPUT_TAB].options[Platform::Input::JOYSTICK_DEADZONE].enabled && joystick_deadzone_sl->checkClickAt(mouse.x, mouse.y)) {
		settings->joy_deadzone = joystick_deadzone_sl->getValue();
	}
	else if (cfg_tabs[INPUT_TAB].options[Platform::Input::JOYSTICK].enabled && joystick_device_lstb->checkClickAt(mouse.x, mouse.y)) {
		settings->joystick_device = static_cast<int>(joystick_device_lstb->getSelected()) - 1;
		if (settings->joystick_device != -1) {
			settings->enable_joystick = true;
			if (inpt->getNumJoysticks() > 0) {
				inpt->initJoystick();
			}
		}
		else {
			settings->enable_joystick = false;
		}
	}
}

void MenuConfig::logicKeybinds() {
	cfg_tabs[KEYBINDS_TAB].scrollbox->logic();
	Point mouse = cfg_tabs[KEYBINDS_TAB].scrollbox->input_assist(inpt->mouse);

	for (unsigned int i = 0; i < keybinds_btn.size(); i++) {
		if ((i+1) % 3 == 0) {
			keybinds_btn[i]->enabled = settings->enable_joystick;
			keybinds_btn[i]->refresh();
		}

		if (!cfg_tabs[KEYBINDS_TAB].options[i].enabled)
			continue;

		if (keybinds_btn[i]->checkClickAt(mouse.x,mouse.y)) {
			std::string confirm_msg;
			confirm_msg = msg->get("Assign:") + ' ' + inpt->binding_name[i/3];
			delete input_confirm;
			input_confirm = new MenuConfirm(msg->get("Clear"),confirm_msg);
			input_confirm_timer.reset(Timer::BEGIN);
			input_confirm->visible = true;
			input_key = i;
			inpt->last_button = -1;
			inpt->last_key = -1;
			inpt->last_joybutton = -1;
		}
	}
}

void MenuConfig::logicMods() {
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

void MenuConfig::render() {
	int tabheight = tab_control->getTabHeight();
	Rect pos;
	pos.x = (settings->view_w - eset->resolutions.frame_w)/2;
	pos.y = (settings->view_h - eset->resolutions.frame_h)/2 + tabheight - tabheight/16;

	if (background) {
		background->setDestFromRect(pos);
		render_device->render(background);
	}

	tab_control->render();

	if (enable_gamestate_buttons) {
		// render OK/Defaults/Cancel buttons
		ok_button->render();
		cancel_button->render();
		defaults_button->render();
	}

	renderTabContents();
	renderDialogs();
}

void MenuConfig::renderTabContents() {
	if (active_tab == EXIT_TAB) {
		if (cfg_tabs[active_tab].scrollbox->update) {
			cfg_tabs[active_tab].scrollbox->refresh();

			// only draw a separator between the buttons and the time played text
			Image* render_target = cfg_tabs[active_tab].scrollbox->contents->getGraphics();
			render_target->drawLine(scrollpane_padding.x, 2 * scrollpane_padding.y, scrollpane.w - scrollpane_padding.x - 1, 2 * scrollpane_padding.y, scrollpane_separator_color);
		}
		cfg_tabs[active_tab].scrollbox->render();
	}
	else if (active_tab <= KEYBINDS_TAB) {
		if (cfg_tabs[active_tab].scrollbox->update) {
			cfg_tabs[active_tab].scrollbox->refresh();

			Image* render_target = cfg_tabs[active_tab].scrollbox->contents->getGraphics();

			for (int i = 1; i < cfg_tabs[active_tab].enabled_count; ++i) {
				if (active_tab == KEYBINDS_TAB) {
					if (i % 3 != 0)
						continue;
				}
				render_target->drawLine(scrollpane_padding.x, i * scrollpane_padding.y, scrollpane.w - scrollpane_padding.x - 1, i * scrollpane_padding.y, scrollpane_separator_color);
			}
		}
		cfg_tabs[active_tab].scrollbox->render();
	}

	for (unsigned int i = 0; i < child_widget.size(); i++) {
		if (optiontab[i] == active_tab && optiontab[i] != NO_TAB)
			child_widget[i]->render();
	}

}

void MenuConfig::renderDialogs() {
	if (defaults_confirm->visible)
		defaults_confirm->render();

	if (input_confirm->visible)
		input_confirm->render();

	if (active_tab == KEYBINDS_TAB && !keybind_msg.empty()) {
		TooltipData keybind_tip_data;
		keybind_tip_data.addText(keybind_msg);

		if (keybind_tip_timer.isEnd())
			keybind_tip_timer.reset(Timer::BEGIN);

		keybind_tip_timer.tick();

		if (!keybind_tip_timer.isEnd()) {
			keybind_tip->render(keybind_tip_data, Point(settings->view_w, 0), TooltipData::STYLE_FLOAT);
		}
		else {
			keybind_msg.clear();
		}
	}
	else {
		keybind_msg.clear();
		keybind_tip_timer.reset(Timer::END);
	}
}

void MenuConfig::placeLabeledWidget(WidgetLabel *lb, Widget *w, int x1, int y1, int x2, int y2, std::string const& str, int justify) {
	if (w) {
		w->setBasePos(x2, y2, Utils::ALIGN_TOPLEFT);
	}

	if (lb) {
		lb->setBasePos(x1, y1, Utils::ALIGN_TOPLEFT);
		lb->setText(str);
		lb->setJustify(justify);
	}
}

void MenuConfig::placeLabeledWidgetAuto(int tab, int cfg_index) {
	WidgetLabel *lb = cfg_tabs[tab].options[cfg_index].label;
	Widget *w = cfg_tabs[tab].options[cfg_index].widget;
	int enabled_index = cfg_tabs[tab].getEnabledIndex(cfg_index);

	if (w) {
		int y_offset = std::max(scrollpane_padding.y - w->pos.h, 0) / 2;
		w->setBasePos(scrollpane.w - w->pos.w - scrollpane_padding.x, (enabled_index * scrollpane_padding.y) + y_offset, Utils::ALIGN_TOPLEFT);
		w->setPos(0,0);
	}

	if (lb) {
		lb->setBasePos(scrollpane_padding.x, (enabled_index * scrollpane_padding.y) + scrollpane_padding.y / 2, Utils::ALIGN_TOPLEFT);
		lb->setPos(0, 0);
		lb->setVAlign(LabelInfo::VALIGN_CENTER);
	}
}

void MenuConfig::refreshWidgets() {
	tab_control->setMainArea(((settings->view_w - eset->resolutions.frame_w)/2) + tab_offset.x, ((settings->view_h - eset->resolutions.frame_h)/2) + tab_offset.y);

	frame.x = ((settings->view_w - eset->resolutions.frame_w)/2) + frame_offset.x;
	frame.y = ((settings->view_h - eset->resolutions.frame_h)/2) + tab_control->getTabHeight() + frame_offset.y;

	for (unsigned i=0; i<child_widget.size(); ++i) {
		if (optiontab[i] != NO_TAB)
			child_widget[i]->setPos(frame.x, frame.y);
	}

	ok_button->setPos(0, 0);
	defaults_button->setPos(0, 0);
	cancel_button->setPos(0, 0);

	defaults_confirm->align();

	for (size_t i = 0; i < cfg_tabs.size(); ++i) {
		cfg_tabs[i].scrollbox->setPos(frame.x, frame.y);
	}

	input_confirm->align();
}

void MenuConfig::addChildWidget(Widget *w, int tab) {
	child_widget.push_back(w);
	optiontab.push_back(tab);
}

void MenuConfig::refreshLanguages() {
	language_ISO.clear();
	language_lstb->clear();

	FileParser infile;
	if (infile.open("engine/languages.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		int i = 0;
		while (infile.next()) {
			std::string key = infile.key;
			if (key != "") {
				language_ISO.push_back(key);
				language_lstb->append(infile.val, infile.val + " [" + infile.key + "]");

				if (language_ISO.back() == settings->language) {
					language_lstb->select(i);
				}

				i++;
			}
		}
		infile.close();
	}
}

void MenuConfig::refreshFont() {
	delete font;
	font = getFontEngine();
	delete comb;
	comb = new CombatText();
}

void MenuConfig::enableMods() {
	for (int i=0; i<inactivemods_lstb->getSize(); i++) {
		if (inactivemods_lstb->isSelected(i)) {
			activemods_lstb->append(inactivemods_lstb->getValue(i),inactivemods_lstb->getTooltip(i));
			inactivemods_lstb->remove(i);
			i--;
		}
	}
}

void MenuConfig::disableMods() {
	for (int i=0; i<activemods_lstb->getSize(); i++) {
		if (activemods_lstb->isSelected(i) && activemods_lstb->getValue(i) != mods->FALLBACK_MOD) {
			inactivemods_lstb->append(activemods_lstb->getValue(i),activemods_lstb->getTooltip(i));
			activemods_lstb->remove(i);
			i--;
		}
	}
	inactivemods_lstb->sort();
}

bool MenuConfig::setMods() {
	// Save new mods list and return true if modlist was changed. Else return false

	std::vector<Mod> temp_list = mods->mod_list;
	mods->mod_list.clear();
	mods->mod_list.push_back(mods->loadMod(mods->FALLBACK_MOD));

	for (int i=0; i<activemods_lstb->getSize(); i++) {
		if (activemods_lstb->getValue(i) != "")
			mods->mod_list.push_back(mods->loadMod(activemods_lstb->getValue(i)));
	}

	mods->applyDepends();

	if (mods->mod_list != temp_list) {
		mods->saveMods();
		return true;
	}
	else {
		return false;
	}
}

std::string MenuConfig::createModTooltip(Mod *mod) {
	std::string ret = "";
	if (mod) {
		std::string mod_ver = (*mod->version == VersionInfo::MIN) ? "" : mod->version->getString();
		std::string engine_ver = VersionInfo::createVersionReqString(*mod->engine_min_version, *mod->engine_max_version);

		ret = mod->name + '\n';

		std::string mod_description = mod->getLocaleDescription(settings->language);
		if (!mod_description.empty()) {
			ret += '\n';
			ret += mod_description + '\n';
		}

		bool middle_section = false;
		if (!mod_ver.empty()) {
			middle_section = true;
			ret += '\n';
			ret += msg->get("Version:") + ' ' + mod_ver;
		}
		if (!mod->game.empty() && mod->game != mods->FALLBACK_GAME) {
			middle_section = true;
			ret += '\n';
			ret += msg->get("Game:") + ' ' + mod->game;
		}
		if (!engine_ver.empty()) {
			middle_section = true;
			ret += '\n';
			ret += msg->get("Engine version:") + ' ' + engine_ver;
		}

		if (middle_section)
			ret += '\n';

		if (!mod->depends.empty()) {
			ret += '\n';
			ret += msg->get("Requires mods:") + '\n';
			for (size_t i=0; i<mod->depends.size(); ++i) {
				ret += "-  " + mod->depends[i];
				std::string depend_ver = VersionInfo::createVersionReqString(*mod->depends_min[i], *mod->depends_max[i]);
				if (depend_ver != "")
					ret += " (" + depend_ver + ")";
				if (i < mod->depends.size()-1)
					ret += '\n';
			}
		}

		if (!ret.empty() && ret[ret.size() - 1] == '\n')
			ret.erase(ret.begin() + ret.size() - 1);
	}
	return ret;
}

void MenuConfig::confirmKey(int button) {
	inpt->pressing[button] = false;
	inpt->lock[button] = false;

	input_confirm->visible = false;
	input_confirm_timer.reset(Timer::END);
	keybind_tip_timer.reset(Timer::END);

	updateKeybinds();
}

void MenuConfig::scanKey(int button) {
	int column = button % 3;
	int real_button = button / 3;

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

void MenuConfig::enableMouseOptions() {
	settings->no_mouse = false;
	no_mouse_cb->setChecked(settings->no_mouse);
}

void MenuConfig::disableMouseOptions() {
	settings->mouse_aim = false;
	mouse_aim_cb->setChecked(settings->mouse_aim);

	settings->mouse_move = false;
	mouse_move_cb->setChecked(settings->mouse_move);

	settings->no_mouse = true;
	no_mouse_cb->setChecked(settings->no_mouse);
}

void MenuConfig::disableJoystickOptions() {
	settings->enable_joystick = false;

	joystick_device_lstb->select(0);

	if (inpt->getNumJoysticks() > 0)
		joystick_device_lstb->refresh();
}

void MenuConfig::refreshRenderers() {
	renderer_lstb->clear();

	std::vector<std::string> rd_name, rd_desc;
	createRenderDeviceList(msg, rd_name, rd_desc);

	for (size_t i = 0; i < rd_name.size(); ++i) {
		renderer_lstb->append(rd_name[i], rd_desc[i]);
		if (rd_name[i] == settings->render_device_name) {
			renderer_lstb->select(static_cast<int>(i));
		}
	}
}

void MenuConfig::refreshJoysticks() {
	joystick_device_lstb->clear();
	joystick_device_lstb->append(msg->get("(none)"), "");
	joystick_device_lstb->enabled = inpt->getNumJoysticks() > 0;

	for (int i = 0; i < inpt->getNumJoysticks(); ++i) {
		std::string joystick_name = inpt->getJoystickName(i);
		if (joystick_name != "")
			joystick_device_lstb->append(joystick_name, joystick_name);
	}

	joystick_device_lstb->refresh();
}

std::string MenuConfig::getRenderDevice() {
	return renderer_lstb->getValue();
}

void MenuConfig::setPauseExitText(bool enable_save) {
	pause_exit_btn->setLabel((eset->misc.save_onexit && enable_save) ? msg->get("Save & Exit") : msg->get("Exit"));
}

void MenuConfig::resetSelectedTab() {
	update();

	tab_control->setActiveTab(0);

	for (size_t i = 0; i < cfg_tabs.size(); ++i) {
		cfg_tabs[i].scrollbox->scrollToTop();
	}

	tablist.defocus();
	tablist_main.defocus();
	tablist_exit.defocus();
	tablist_video.defocus();
	tablist_audio.defocus();
	tablist_interface.defocus();
	tablist_input.defocus();
	tablist_keybinds.defocus();
	tablist_mods.defocus();
}

void MenuConfig::cleanup() {
	if (background) {
		delete background;
		background = NULL;
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

void MenuConfig::cleanupTabContents() {
	for (std::vector<Widget*>::iterator iter = child_widget.begin(); iter != child_widget.end(); ++iter) {
		if (*iter != NULL) {
			delete (*iter);
			*iter = NULL;
		}
	}
	child_widget.clear();

	for (size_t i = 0; i < cfg_tabs.size(); ++i) {
		if (cfg_tabs[i].scrollbox != NULL) {
			delete cfg_tabs[i].scrollbox;
			cfg_tabs[i].scrollbox = NULL;
		}
	}
}

void MenuConfig::cleanupDialogs() {
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

void MenuConfig::setHero(Avatar* _hero) {
	hero = _hero;
}
