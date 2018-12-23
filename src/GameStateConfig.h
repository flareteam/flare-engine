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
 * GameStateConfig
 *
 * Handle game Settings Menu
 */

#ifndef GAMESTATECONFIG_H
#define GAMESTATECONFIG_H

#include "CommonIncludes.h"
#include "GameState.h"
#include "TooltipData.h"
#include "Widget.h"

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

class GameStateConfig : public GameState {
private:
	static const int GAMMA_MIN = 5;
	static const int GAMMA_MAX = 15;

	static const int CFG_VIDEO_COUNT = 9;
	enum {
		CFG_VIDEO_RENDERER,
		CFG_VIDEO_FULLSCREEN,
		CFG_VIDEO_HWSURFACE,
		CFG_VIDEO_VSYNC,
		CFG_VIDEO_TEXTURE_FILTER,
		CFG_VIDEO_DPI_SCALING,
		CFG_VIDEO_PARALLAX_LAYERS,
		CFG_VIDEO_ENABLE_GAMMA,
		CFG_VIDEO_GAMMA
	};

	static const int CFG_AUDIO_COUNT = 2;
	enum {
		CFG_AUDIO_SFX,
		CFG_AUDIO_MUSIC
	};

public:
	short VIDEO_TAB;
	short AUDIO_TAB;
	short INTERFACE_TAB;
	short INPUT_TAB;
	short KEYBINDS_TAB;
	short MODS_TAB;

	explicit GameStateConfig();
	~GameStateConfig();

	void init();
	void readConfig();
	bool parseKeyButtons(FileParser &infile);
	bool parseKey(FileParser &infile, int &x1, int &y1, int &x2, int &y2);
	void addChildWidgets();
	void setupTabList();

	void update();
	void updateVideo();
	void updateAudio();
	void updateInterface();
	void updateInput();
	void updateKeybinds();
	void updateMods();

	void logic();
	bool logicMain();
	void logicDefaults();
	void logicAccept();
	void logicCancel();
	void logicVideo();
	void logicAudio();
	void logicInterface();
	void logicInput();
	void logicKeybinds();
	void logicMods();

	void render();
	void renderTabContents();
	void renderDialogs();

	void placeLabeledWidget(WidgetLabel* lb, Widget* w, int x1, int y1, int x2, int y2, std::string const& str, int justify = 0);
	void placeLabeledWidgetAuto(int cfg_index, WidgetLabel *lb, Widget *w, std::string const& str);
	void refreshWidgets();
	void addChildWidget(Widget *w, int tab);
	void refreshRenderers();
	void refreshLanguages();
	void refreshFont();

	void confirmKey(int button);
	void scanKey(int button);

	void enableMouseOptions();
	void disableMouseOptions();
	void disableJoystickOptions();

	void enableMods();
	void disableMods();
	bool setMods();
	std::string createModTooltip(Mod *mod);

	void cleanup();
	void cleanupTabContents();
	void cleanupDialogs();

	TabList tablist;
	TabList tablist_main;
	TabList tablist_video;
	TabList tablist_audio;
	TabList tablist_interface;
	TabList tablist_input;
	TabList tablist_keybinds;
	TabList tablist_mods;

	std::vector<int> optiontab;
	std::vector<Widget*> child_widget;

	WidgetTabControl           * tab_control;
	WidgetButton               * ok_button;
	WidgetButton               * defaults_button;
	WidgetButton               * cancel_button;
	Sprite                     * background;
	WidgetScrollBox            * video_scrollbox;
	WidgetScrollBox            * audio_scrollbox;
	WidgetScrollBox            * input_scrollbox;
	MenuConfirm                * input_confirm;
	MenuConfirm                * defaults_confirm;

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
	WidgetCheckBox             * change_gamma_cb;
	WidgetLabel                * change_gamma_lb;
	WidgetSlider               * gamma_sl;
	WidgetLabel                * gamma_lb;

	WidgetSlider               * music_volume_sl;
	WidgetLabel                * music_volume_lb;
	WidgetSlider               * sound_volume_sl;
	WidgetLabel                * sound_volume_lb;

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

	int active_tab;

	Point frame;
	Point frame_offset;
	Point tab_offset;
	Rect scrollpane;
	Color scrollpane_color;
	int scrollpane_contents;
	Point scrollpane_padding;
	Color scrollpane_separator_color;
	Point secondary_offset;

	std::vector<std::string> language_ISO;

	std::string new_render_device;
	std::vector<Rect> video_modes;

	std::vector<WidgetLabel *> keybinds_lb;
	std::vector<WidgetButton *> keybinds_btn;

	Timer input_confirm_timer;
	int input_key;
	unsigned key_count;

	std::string keybind_msg;
	Timer keybind_tip_timer;
	WidgetTooltip* keybind_tip;
};

#endif

