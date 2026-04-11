/*
Copyright © 2026 Justin Jacobs

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

#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "MenuRegionTitle.h"
#include "MessageEngine.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

#include <string>

MenuRegionTitle::MenuRegionTitle()
	: Menu()
	, title("")
{
	// default layout
	// default window width and height are 0, since we just need an anchor point for the label
	setWindowPos(0, 64);
	alignment = Utils::ALIGN_TOP;
	label.setJustify(FontEngine::JUSTIFY_CENTER);
	label.setBasePos(window_area.w/2, window_area.h/2, Utils::ALIGN_TOPLEFT);
	label.setFont("font_region_title");

	timer.setDuration(settings->max_frames_per_sec * 3);
	fade_in_timer.setDuration(settings->max_frames_per_sec);
	fade_out_timer.setDuration(settings->max_frames_per_sec);

	// Load config settings
	FileParser infile;
	// @CLASS MenuRegionTitle|Description of menus/region_title.txt
	if(infile.open("menus/region_title.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "label") {
				// @ATTR label_title|label|Position of the text displaying the map name.
				label.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			else if (infile.key == "hold_duration") {
				// @ATTR hold_duration|duration|Duration that the text is shown in between the fade animations.
				timer.setDuration(Parse::toDuration(infile.val));
			}
			else if (infile.key == "fade_in_duration") {
				// @ATTR fade_in_duration|duration|Duration of the "fade in" animation.
				fade_in_timer.setDuration(Parse::toDuration(infile.val));
			}
			else if (infile.key == "fade_out_duration") {
				// @ATTR fade_out_duration|duration|Duration of the "fade out" animation.
				fade_out_timer.setDuration(Parse::toDuration(infile.val));
			}
		}
		infile.close();
	}

	label.setText("");
	label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	if (!background)
		setBackground("images/menus/game_over.png");

	align();

	visible = false;
}

void MenuRegionTitle::align() {
	Menu::align();

	label.setPos(window_area.x, window_area.y);
}

void MenuRegionTitle::logic() {
	if (!visible || !enabled)
		return;

	if (timer.isEnd() && fade_out_timer.isEnd()) {
		visible = false;
	}
	else {
		if (timer.isBegin() && !fade_in_timer.isEnd()) {
			float normalized_time = static_cast<float>(fade_in_timer.getCurrent()) / static_cast<float>(fade_in_timer.getDuration());
			label.setAlpha(static_cast<uint8_t>(255.f * (1.f - normalized_time)));
			fade_in_timer.tick();
		}
		if (!timer.isEnd() && fade_in_timer.isEnd()) {
			label.setAlpha(255);
			timer.tick();
		}
		if (timer.isEnd() && !fade_out_timer.isEnd()) {
			float normalized_time = static_cast<float>(fade_out_timer.getCurrent()) / static_cast<float>(fade_out_timer.getDuration());
			label.setAlpha(static_cast<uint8_t>(255.f * normalized_time));
			fade_out_timer.tick();
		}
	}
}

void MenuRegionTitle::setTitle(const std::string& new_title) {
	if (!enabled)
		return;

	if (new_title != title) {
		title = new_title;
		label.setText(title);
		label.setAlpha(0);
		timer.reset(Timer::BEGIN);
		fade_in_timer.reset(Timer::BEGIN);
		fade_out_timer.reset(Timer::BEGIN);
		visible = true;
	}
}

void MenuRegionTitle::render() {
	if (!visible || !enabled)
		return;

	// background
	Menu::render();

	label.render();
}

MenuRegionTitle::~MenuRegionTitle() {
}

