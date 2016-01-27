/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 davidriod
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
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
 * GameStateConfigBase
 *
 * Handle game Settings Menu
 */

#ifndef GAMESTATECONFIGBASE_H
#define GAMESTATECONFIGBASE_H

#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameState.h"
#include "TooltipData.h"

class MenuConfirm;
class Widget;
class WidgetButton;
class WidgetCheckBox;
class WidgetLabel;
class WidgetListBox;
class WidgetSlider;
class WidgetTabControl;
class WidgetTooltip;

class GameStateConfigBase : public GameState {
public:
	short AUDIO_TAB;
	short INTERFACE_TAB;
	short MODS_TAB;

	GameStateConfigBase(bool do_init = true);
	~GameStateConfigBase();

	virtual void init();
	virtual void readConfig();
	bool parseKey(FileParser &infile, int &x1, int &y1, int &x2, int &y2);
	bool parseStub(FileParser &infile);
	void addChildWidgets();
	virtual void setupTabList();

	virtual void update();
	void updateAudio();
	void updateInterface();
	void updateMods();

	virtual void logic();
	virtual bool logicMain();
	void logicDefaults();
	virtual void logicAccept();
	void logicCancel();
	void logicAudio();
	void logicInterface();
	void logicMods();

	void render();
	virtual void renderTabContents();
	virtual void renderDialogs();
	virtual void renderTooltips(TooltipData& tip_new);

	void placeLabeledWidget(WidgetLabel* lb, Widget* w, int x1, int y1, int x2, int y2, std::string const& str, int justify = JUSTIFY_LEFT);
	virtual void refreshWidgets();
	void addChildWidget(Widget *w, int tab);
	void refreshLanguages();
	void refreshFont();

	void enableMods();
	void disableMods();
	bool setMods();
	std::string createModTooltip(Mod *mod);

	void cleanup();
	virtual void cleanupTabContents();
	virtual void cleanupDialogs();

	TabList tablist;
	TabList tablist_main;
	TabList tablist_audio;
	TabList tablist_interface;
	TabList tablist_mods;

	std::vector<int>      optiontab;
	std::vector<Widget*>  child_widget;
	WidgetTabControl    * tab_control;
	WidgetButton        * ok_button;
	WidgetButton        * defaults_button;
	WidgetButton        * cancel_button;
	Sprite              * background;

	WidgetCheckBox      * combat_text_cb;
	WidgetLabel         * combat_text_lb;
	WidgetCheckBox      * show_fps_cb;
	WidgetLabel         * show_fps_lb;
	WidgetCheckBox      * hardware_cursor_cb;
	WidgetLabel         * hardware_cursor_lb;
	WidgetCheckBox      * colorblind_cb;
	WidgetLabel         * colorblind_lb;
	WidgetCheckBox      * dev_mode_cb;
	WidgetLabel         * dev_mode_lb;
	WidgetCheckBox      * show_target_cb;
	WidgetLabel         * show_target_lb;
	WidgetCheckBox      * loot_tooltips_cb;
	WidgetLabel         * loot_tooltips_lb;
	WidgetCheckBox      * statbar_labels_cb;
	WidgetLabel         * statbar_labels_lb;
	WidgetSlider        * music_volume_sl;
	WidgetLabel         * music_volume_lb;
	WidgetSlider        * sound_volume_sl;
	WidgetLabel         * sound_volume_lb;
	WidgetListBox       * activemods_lstb;
	WidgetLabel         * activemods_lb;
	WidgetListBox       * inactivemods_lstb;
	WidgetLabel         * inactivemods_lb;
	WidgetListBox       * language_lstb;
	WidgetLabel         * language_lb;
	WidgetButton        * activemods_shiftup_btn;
	WidgetButton        * activemods_shiftdown_btn;
	WidgetButton        * activemods_deactivate_btn;
	WidgetButton        * inactivemods_activate_btn;

	MenuConfirm         * defaults_confirm;

	WidgetTooltip       * tip;
	TooltipData         tip_buf;

	int active_tab;

	Rect frame;
	std::vector<std::string> language_ISO;
};

#endif

