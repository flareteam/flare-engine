/*
Copyright © 2011-2012 kitano
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

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

#include "FileParser.h"
#include "MenuExit.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"

MenuExit::MenuExit() : Menu() {

	buttonExit = new WidgetButton();
	buttonClose = new WidgetButton();

	// widgets for game options
	music_volume_sl = new WidgetSlider();
	sound_volume_sl = new WidgetSlider();

	// Load config settings
	// TODO attribute docs
	FileParser infile;
	if(infile.open("menus/exit.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "title") {
				title = eatLabelInfo(infile.val);
			}
			else if (infile.key == "exit") {
				Point p = toPoint(infile.val);
				buttonExit->setBasePos(p.x, p.y);
			}
			else if (infile.key == "continue") {
				Point p = toPoint(infile.val);
				buttonClose->setBasePos(p.x, p.y);
			}
			else if (infile.key == "music_volume") {
				Rect r = toRect(infile.val);
				placeOptionWidgets(&music_volume_lb, music_volume_sl, r.x, r.y, r.w, r.h, msg->get("Music Volume"));
			}
			else if (infile.key == "sound_volume") {
				Rect r = toRect(infile.val);
				placeOptionWidgets(&sound_volume_lb, sound_volume_sl, r.x, r.y, r.w, r.h, msg->get("Sound Volume"));
			}
			else
				infile.error("MenuExit: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	exitClicked = false;
	reload_music = false;

	if (SAVE_ONEXIT)
		buttonExit->label = msg->get("Save & Exit");
	else
		buttonExit->label = msg->get("Exit");

	buttonClose->label = msg->get("Continue");

	setBackground("images/menus/pause_menu.png");

	if (AUDIO) {
		music_volume_sl->set(0, 128, MUSIC_VOLUME);
		sound_volume_sl->set(0, 128, SOUND_VOLUME);
	}
	else {
		music_volume_sl->set(0, 128, 0);
		sound_volume_sl->set(0, 128, 0);
	}

	tablist.add(buttonExit);
	tablist.add(buttonClose);

	align();
}

void MenuExit::align() {
	Menu::align();

	title_lb.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Paused"), font->getColor("menu_normal"), title.font_style);

	buttonExit->setPos(window_area.x, window_area.y);
	buttonExit->refresh();

	buttonClose->setPos(window_area.x, window_area.y);
	buttonClose->refresh();

	for (size_t i=0; i<option_labels.size(); ++i) {
		option_labels[i]->setPos(window_area.x, window_area.y);
	}

	for (size_t i=0; i<option_widgets.size(); ++i) {
		option_widgets[i]->setPos(window_area.x, window_area.y);
	}
}

void MenuExit::logic() {
	if (visible) {
		tablist.logic();

		if (buttonExit->checkClick()) {
			exitClicked = true;
		}
		else if (buttonClose->checkClick()) {
			visible = false;
		}
		else if (AUDIO && music_volume_sl->checkClick()) {
			if (MUSIC_VOLUME == 0)
				reload_music = true;
			MUSIC_VOLUME = static_cast<short>(music_volume_sl->getValue());
			snd->setVolumeMusic(MUSIC_VOLUME);
		}
		else if (AUDIO && sound_volume_sl->checkClick()) {
			SOUND_VOLUME = static_cast<short>(sound_volume_sl->getValue());
			snd->setVolumeSFX(SOUND_VOLUME);
		}
	}
}

void MenuExit::render() {
	if (visible) {
		// background
		Menu::render();

		title_lb.render();

		buttonExit->render();
		buttonClose->render();

		for (size_t i=0; i<option_labels.size(); ++i) {
			option_labels[i]->render();
		}

		for (size_t i=0; i<option_widgets.size(); ++i) {
			option_widgets[i]->render();
		}
	}
}

void MenuExit::placeOptionWidgets(WidgetLabel *lb, Widget *w, int x1, int y1, int x2, int y2, std::string const& str) {
	if (w) {
		w->setBasePos(x2, y2);
		option_widgets.push_back(w);
	}

	if (lb) {
		lb->setBasePos(x1, y1);
		lb->set(str);
		lb->setJustify(JUSTIFY_CENTER);
		option_labels.push_back(lb);
	}
}

MenuExit::~MenuExit() {
	delete buttonExit;
	delete buttonClose;

	delete music_volume_sl;
	delete sound_volume_sl;
}

