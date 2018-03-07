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

#ifndef GAMESTATECONFIGDESKTOP_H
#define GAMESTATECONFIGDESKTOP_H

#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameState.h"
#include "GameStateConfigBase.h"
#include "TooltipData.h"

class MenuConfirm;
class Widget;
class WidgetButton;
class WidgetCheckBox;
class WidgetInput;
class WidgetLabel;
class WidgetListBox;
class WidgetScrollBox;
class WidgetSlider;
class WidgetTabControl;
class WidgetTooltip;

class GameStateConfigDesktop : public GameStateConfigBase {
public:
	explicit GameStateConfigDesktop(bool _enable_video_tab);
	~GameStateConfigDesktop();

private:
	short VIDEO_TAB;
	short INPUT_TAB;
	short KEYBINDS_TAB;

	void init();
	void readConfig();
	bool parseKeyDesktop(FileParser &infile, int &x1, int &y1, int &x2, int &y2);
	void addChildWidgetsDesktop();
	void setupTabList();

	void update();
	void updateVideo();
	void updateInput();
	void updateKeybinds();

	void logic();
	bool logicMain();
	void logicVideo();
	void logicInput();
	void logicKeybinds();

	void renderTabContents();
	void renderDialogs();
	void renderTooltips(TooltipData& tip_new);

	void refreshWidgets();

	void cleanupTabContents();
	void cleanupDialogs();

	TabList tablist_video;
	TabList tablist_input;
	TabList tablist_keybinds;

	void scanKey(int button);

	void enableMouseOptions();
	void disableMouseOptions();
	void disableJoystickOptions();

	WidgetCheckBox      * fullscreen_cb;
	WidgetLabel         * fullscreen_lb;
	WidgetCheckBox      * hwsurface_cb;
	WidgetLabel         * hwsurface_lb;
	WidgetCheckBox      * vsync_cb;
	WidgetLabel         * vsync_lb;
	WidgetCheckBox      * texture_filter_cb;
	WidgetLabel         * texture_filter_lb;
	WidgetCheckBox      * change_gamma_cb;
	WidgetLabel         * change_gamma_lb;
	WidgetSlider        * gamma_sl;
	WidgetLabel         * gamma_lb;
	WidgetListBox       * joystick_device_lstb;
	WidgetLabel         * joystick_device_lb;
	WidgetCheckBox      * enable_joystick_cb;
	WidgetLabel         * enable_joystick_lb;
	WidgetCheckBox      * mouse_move_cb;
	WidgetLabel         * mouse_move_lb;
	WidgetCheckBox      * mouse_aim_cb;
	WidgetLabel         * mouse_aim_lb;
	WidgetCheckBox      * no_mouse_cb;
	WidgetLabel         * no_mouse_lb;
	WidgetSlider        * joystick_deadzone_sl;
	WidgetLabel         * joystick_deadzone_lb;

	std::vector<Rect> video_modes;

	std::vector<WidgetLabel *> keybinds_lb;
	std::vector<WidgetButton *> keybinds_btn;

	WidgetScrollBox     * input_scrollbox;
	MenuConfirm         * input_confirm;

	int input_confirm_ticks;
	int input_key;
	unsigned key_count;

	Rect scrollpane;
	Color scrollpane_color;
	int scrollpane_contents;
	Point secondary_offset;

	bool enable_video_tab;
};

#endif

