/*
Copyright © 2011-2012 kitano
Copyright © 2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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
 * class MenuExit
 */

#ifndef MENU_EXIT_H
#define MENU_EXIT_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetButton.h"
#include "WidgetSlider.h"
#include "WidgetCheckBox.h"

class MenuExit : public Menu {
protected:
	void placeOptionWidgets(WidgetLabel *lb, Widget *w, int x1, int y1, int x2, int y2, std::string const& str);

	WidgetButton *buttonExit;
	WidgetButton *buttonClose;
	WidgetLabel title_lb;
	LabelInfo title;

	WidgetSlider *music_volume_sl;
	WidgetSlider *sound_volume_sl;

	WidgetLabel music_volume_lb;
	WidgetLabel sound_volume_lb;

	WidgetCheckBox *mute_music_volume_cb;
	WidgetCheckBox *mute_sound_volume_cb;

	bool exitClicked;

	std::vector<WidgetLabel*> option_labels;
	std::vector<Widget*> option_widgets;

public:
	MenuExit();
	~MenuExit();
	void align();

	void logic();
	virtual void render();

	bool isExitRequested() {
		return exitClicked;
	}

	bool reload_music;
};

#endif
