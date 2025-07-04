/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
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

#ifndef MENUCONFIG_H
#define MENUCONFIG_H

#include "CommonIncludes.h"
#include "TooltipData.h"
#include "Widget.h"

class Avatar;
class FileParser;
class MenuConfirm;
class Mod;
class Widget;
class WidgetButton;
class WidgetCheckBox;
class WidgetHorizontalList;
class WidgetLabel;
class WidgetListBox;
class WidgetScrollBox;
class WidgetSlider;
class WidgetTabControl;
class WidgetTooltip;

class MenuConfig {
private:
	class ConfigOption {
	public:
		ConfigOption();
		~ConfigOption();

		bool enabled;
		WidgetLabel* label;
		Widget* widget;
	};

	class ConfigTab {
	public:
		ConfigTab();
		~ConfigTab();
		void setOptionWidgets(int index, WidgetLabel* lb, Widget* w, const std::string& lb_text);
		void setOptionEnabled(int index, bool enable);
		int getEnabledIndex(int option_index);

		WidgetScrollBox* scrollbox;
		int enabled_count;
		std::vector<ConfigOption> options;
	};

	static const int TOUCH_SCALE_MIN = 75;
	static const int TOUCH_SCALE_MAX = 125;

	enum {
		DEFAULTS_CONFIRM_OPTION_NO = 0,
		DEFAULTS_CONFIRM_OPTION_YES = 1,
	};

	enum {
		INPUT_CONFIRM_OPTION_NEW = 0,
		INPUT_CONFIRM_OPTION_CLEAR = 1,
	};

	std::vector<ConfigTab> cfg_tabs;

	bool is_game_state;
	bool enable_gamestate_buttons;
	Avatar *hero;

	std::vector<unsigned short> frame_limits;
	std::vector<unsigned short> virtual_heights;

	bool keybinds_visible_equipswap;
	std::vector<bool> keybinds_visible_actionbar;
	std::vector<bool> keybinds_visible_menus;

	bool mod_filter_unknown;

public:
	static const bool IS_GAME_STATE = true;
	static const bool ENABLE_SAVE_GAME = true;

	static const short TAB_COUNT = 8;
	enum {
		EXIT_TAB = 0,
		VIDEO_TAB = 1,
		AUDIO_TAB = 2,
		GAME_TAB = 3,
		INTERFACE_TAB = 4,
		INPUT_TAB = 5,
		KEYBINDS_TAB = 6,
		MODS_TAB = 7
	};
	static const short NO_TAB = -1;

	enum {
		EXIT_OPTION_CONTINUE = 0,
		EXIT_OPTION_SAVE = 1,
		EXIT_OPTION_EXIT = 2,
		EXIT_OPTION_TIME_PLAYED = 3,
	};

	explicit MenuConfig(bool _is_game_state);
	~MenuConfig();

	void init();
	void readConfig();
	bool parseKeyButtons(FileParser &infile);
	bool parseKey(FileParser &infile, int &x1, int &y1, int &x2, int &y2);
	void addChildWidgets();
	void setupTabList();

	void update();
	void updateVideo();
	void updateAudio();
	void updateGame();
	void updateInterface();
	void updateInput();
	void updateKeybinds();
	void updateMods();

	void logic();
	bool logicMain();
	void logicDefaults();
	void logicExit();
	void logicVideo();
	void logicAudio();
	void logicGame();
	void logicInterface();
	void logicInput();
	void logicKeybinds();
	void logicMods();

	void render();
	void renderTabContents();
	void renderDialogs();

	void placeLabeledWidget(WidgetLabel* lb, Widget* w, int x1, int y1, int x2, int y2, std::string const& str, int justify = 0);
	void placeLabeledWidgetAuto(int tab, int cfg_index);
	void refreshWidgets();
	void refreshWindowSize();
	void addChildWidget(Widget *w, int tab);
	void refreshRenderers();
	void refreshJoysticks();
	void refreshLanguages();
	void refreshFont();
	std::string getRenderDevice();
	void setPauseExitText(bool enable_save);
	void setPauseSaveEnabled(bool enable_save);
	void resetSelectedTab();
	int getActiveTab();
	void setActiveTab(unsigned tab);

	void confirmKey(int action);
	void scanKey(int action);

	void enableMouseOptions();
	void disableMouseOptions();

	void enableMods();
	void disableMods();
	bool setMods();
	void filterMods();
	std::string createModTooltip(Mod *mod);

	void cleanup();
	void cleanupTabContents();
	void cleanupDialogs();

	void setHero(Avatar* _hero);

	bool setFrameLimit();

	TabList tablist;
	TabList tablist_main;
	TabList tablist_exit;
	TabList tablist_video;
	TabList tablist_audio;
	TabList tablist_game;
	TabList tablist_interface;
	TabList tablist_input;
	TabList tablist_keybinds;
	TabList tablist_mods;

	std::vector<TabList*> tablists;

	std::vector<int> optiontab;
	std::vector<Widget*> child_widget;

	WidgetTabControl           * tab_control;
	WidgetButton               * ok_button;
	WidgetButton               * defaults_button;
	WidgetButton               * cancel_button;
	Sprite                     * background;
	MenuConfirm                * input_confirm;
	MenuConfirm                * defaults_confirm;

	WidgetLabel                * pause_continue_lb;
	WidgetButton               * pause_continue_btn;
	WidgetLabel                * pause_exit_lb;
	WidgetButton               * pause_exit_btn;
	WidgetLabel                * pause_save_lb;
	WidgetButton               * pause_save_btn;
	WidgetLabel                * pause_time_lb;
	WidgetLabel                * pause_time_text;

	WidgetHorizontalList       * renderer_lstb;
	WidgetLabel                * renderer_lb;
	WidgetCheckBox             * fullscreen_cb;
	WidgetLabel                * fullscreen_lb;
	WidgetCheckBox             * hwsurface_cb;
	WidgetLabel                * hwsurface_lb;
	WidgetCheckBox             * vsync_cb;
	WidgetLabel                * vsync_lb;
	WidgetCheckBox             * texture_filter_cb;
	WidgetLabel                * texture_filter_lb;
	WidgetCheckBox             * dpi_scaling_cb;
	WidgetLabel                * dpi_scaling_lb;
	WidgetCheckBox             * parallax_layers_cb;
	WidgetLabel                * parallax_layers_lb;
	WidgetHorizontalList       * frame_limit_lstb;
	WidgetLabel                * frame_limit_lb;
	WidgetHorizontalList       * max_render_size_lstb;
	WidgetLabel                * max_render_size_lb;

	WidgetSlider               * music_volume_sl;
	WidgetLabel                * music_volume_lb;
	WidgetSlider               * sound_volume_sl;
	WidgetLabel                * sound_volume_lb;
	WidgetCheckBox             * mute_on_focus_loss_cb;
	WidgetLabel                * mute_on_focus_loss_lb;

	WidgetCheckBox             * auto_equip_cb;
	WidgetLabel                * auto_equip_lb;
	WidgetHorizontalList       * auto_loot_lstb;
	WidgetLabel                * auto_loot_lb;
	WidgetHorizontalList       * low_hp_warning_lstb;
	WidgetLabel                * low_hp_warning_lb;
	WidgetHorizontalList       * low_hp_threshold_lstb;
	WidgetLabel                * low_hp_threshold_lb;

	WidgetCheckBox             * show_fps_cb;
	WidgetLabel                * show_fps_lb;
	WidgetCheckBox             * hardware_cursor_cb;
	WidgetLabel                * hardware_cursor_lb;
	WidgetCheckBox             * colorblind_cb;
	WidgetLabel                * colorblind_lb;
	WidgetCheckBox             * dev_mode_cb;
	WidgetLabel                * dev_mode_lb;
	WidgetCheckBox             * subtitles_cb;
	WidgetLabel                * subtitles_lb;
	WidgetHorizontalList       * loot_tooltip_lstb;
	WidgetLabel                * loot_tooltip_lb;
	WidgetHorizontalList       * minimap_lstb;
	WidgetLabel                * minimap_lb;
	WidgetCheckBox             * statbar_labels_cb;
	WidgetLabel                * statbar_labels_lb;
	WidgetCheckBox             * statbar_autohide_cb;
	WidgetLabel                * statbar_autohide_lb;
	WidgetCheckBox             * combat_text_cb;
	WidgetLabel                * combat_text_lb;
	WidgetCheckBox             * entity_markers_cb;
	WidgetLabel                * entity_markers_lb;
	WidgetCheckBox             * item_compare_tips_cb;
	WidgetLabel                * item_compare_tips_lb;
	WidgetCheckBox             * pause_on_focus_loss_cb;
	WidgetLabel                * pause_on_focus_loss_lb;


	WidgetHorizontalList       * joystick_device_lstb;
	WidgetLabel                * joystick_device_lb;
	WidgetCheckBox             * mouse_move_cb;
	WidgetLabel                * mouse_move_lb;
	WidgetCheckBox             * mouse_aim_cb;
	WidgetLabel                * mouse_aim_lb;
	WidgetCheckBox             * no_mouse_cb;
	WidgetLabel                * no_mouse_lb;
	WidgetCheckBox             * mouse_move_swap_cb;
	WidgetLabel                * mouse_move_swap_lb;
	WidgetCheckBox             * mouse_move_attack_cb;
	WidgetLabel                * mouse_move_attack_lb;
	WidgetSlider               * joystick_deadzone_sl;
	WidgetLabel                * joystick_deadzone_lb;
	WidgetCheckBox             * joystick_rumble_cb;
	WidgetLabel                * joystick_rumble_lb;
	WidgetCheckBox             * touch_controls_cb;
	WidgetLabel                * touch_controls_lb;
	WidgetSlider               * touch_scale_sl;
	WidgetLabel                * touch_scale_lb;

	WidgetListBox              * activemods_lstb;
	WidgetLabel                * activemods_lb;
	WidgetListBox              * inactivemods_lstb;
	WidgetLabel                * inactivemods_lb;
	WidgetHorizontalList       * language_lstb;
	WidgetLabel                * language_lb;
	WidgetButton               * activemods_shiftup_btn;
	WidgetButton               * activemods_shiftdown_btn;
	WidgetButton               * activemods_deactivate_btn;
	WidgetButton               * inactivemods_activate_btn;
	WidgetHorizontalList       * inactivemods_filter_lstb;

	int active_tab;

	Point frame;
	Point frame_offset;
	Point tab_offset;
	Point background_offset;
	Rect scrollpane;
	Color scrollpane_color;
	Point scrollpane_padding;
	Color scrollpane_separator_color;
	Point secondary_offset;

	std::vector<std::string> language_ISO;

	std::string new_render_device;
	std::vector<Rect> video_modes;

	std::vector<WidgetLabel *> keybinds_lb;
	std::vector<WidgetHorizontalList *> keybinds_lstb;

	Timer input_confirm_timer;
	int input_action;

	std::string keybind_msg;
	Timer keybind_tip_timer;
	WidgetTooltip* keybind_tip;

	// flags for GameState*
	bool clicked_accept;
	bool clicked_cancel;
	bool force_refresh_background;
	bool reload_music;
	bool clicked_pause_continue;
	bool clicked_pause_exit;
	bool clicked_pause_save;
};

#endif

