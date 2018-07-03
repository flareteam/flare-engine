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

#include "Avatar.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "MenuExit.h"
#include "MessageEngine.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "Settings.h"
#include "SoundManager.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetSlider.h"

MenuExit::MenuExit() : Menu() {

	buttonExit = new WidgetButton(WidgetButton::DEFAULT_FILE);
	buttonClose = new WidgetButton(WidgetButton::DEFAULT_FILE);

	// widgets for game options
	music_volume_sl = new WidgetSlider();
	sound_volume_sl = new WidgetSlider();

	// Load config settings
	FileParser infile;
	// @CLASS MenuExit|Description of menus/exit.txt
	if(infile.open("menus/exit.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "title") {
				// @ATTR title|label|Position of the "Paused" text.
				title = eatLabelInfo(infile.val);
			}
			else if (infile.key == "exit") {
				// @ATTR exit|point|Position of the "Save and Exit" button.
				Point p = toPoint(infile.val);
				buttonExit->setBasePos(p.x, p.y, ALIGN_TOPLEFT);
			}
			else if (infile.key == "continue") {
				// @ATTR continue|point|Position of the "Continue" button.
				Point p = toPoint(infile.val);
				buttonClose->setBasePos(p.x, p.y, ALIGN_TOPLEFT);
			}
			else if (infile.key == "music_volume") {
				// @ATTR music_volume|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Music Volume" slider relative to the frame.
				Rect r = toRect(infile.val);
				placeOptionWidgets(&music_volume_lb, music_volume_sl, r.x, r.y, r.w, r.h, msg->get("Music Volume"));
			}
			else if (infile.key == "sound_volume") {
				// @ATTR sound_volume|int, int, int, int : Label X, Label Y, Widget X, Widget Y|Position of the "Sound Volume" slider relative to the frame.
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

	exit_msg1 = msg->get("Save & Exit");
	exit_msg2 = msg->get("Exit");

	buttonExit->label = eset->misc.save_onexit ? exit_msg1 : exit_msg2;

	buttonClose->label = msg->get("Continue");

	setBackground("images/menus/pause_menu.png");

	if (settings->audio) {
		music_volume_sl->set(0, 128, settings->music_volume);
		sound_volume_sl->set(0, 128, settings->sound_volume);
	}
	else {
		music_volume_sl->set(0, 128, 0);
		sound_volume_sl->set(0, 128, 0);
	}

	tablist.add(buttonClose);
	tablist.add(buttonExit);
	tablist.add(music_volume_sl);
	tablist.add(sound_volume_sl);

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
		std::string title_str = msg->get("Paused") + " [" + getTimeString(pc->time_played, true) + "]";
		title_lb.set(title_str);

		tablist.logic();

		if (buttonExit->checkClick()) {
			exitClicked = true;
		}
		else if (buttonClose->checkClick()) {
			visible = false;
		}
		else if (settings->audio && music_volume_sl->checkClick()) {
			if (settings->music_volume == 0)
				reload_music = true;
			settings->music_volume = static_cast<short>(music_volume_sl->getValue());
			snd->setVolumeMusic(settings->music_volume);
		}
		else if (settings->audio && sound_volume_sl->checkClick()) {
			settings->sound_volume = static_cast<short>(sound_volume_sl->getValue());
			snd->setVolumeSFX(settings->sound_volume);
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
		w->setBasePos(x2, y2, ALIGN_TOPLEFT);
		option_widgets.push_back(w);
	}

	if (lb) {
		lb->setBasePos(x1, y1, ALIGN_TOPLEFT);
		lb->set(str);
		lb->setJustify(FontEngine::JUSTIFY_CENTER);
		option_labels.push_back(lb);
	}
}

void MenuExit::disableSave() {
	if (buttonExit) {
		buttonExit->label = exit_msg2;
		buttonExit->refresh();
	}
}

MenuExit::~MenuExit() {
	delete buttonExit;
	delete buttonClose;

	delete music_volume_sl;
	delete sound_volume_sl;
}

