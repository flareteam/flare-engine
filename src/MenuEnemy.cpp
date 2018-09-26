/*
Copyright © 2011-2012 Pavel Kirpichyov (Cheshire)
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2014 Justin Jacobs

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
 * MenuEnemy
 *
 * Handles the display of the Enemy bar on the HUD
 */

#include "CommonIncludes.h"
#include "Enemy.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "Menu.h"
#include "MenuEnemy.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

MenuEnemy::MenuEnemy()
	: bar_hp(NULL)
	, custom_text_pos(false)
	, enemy(NULL)
{
	// disappear after 10 seconds
	timeout.setDuration(settings->max_frames_per_sec * 10);

	// Load config settings
	FileParser infile;
	// @CLASS MenuEnemy|Description of menus/enemy.txt
	if(infile.open("menus/enemy.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			infile.val = infile.val + ',';

			// @ATTR bar_pos|rectangle|Position and dimensions of the health bar.
			if(infile.key == "bar_pos") {
				bar_pos = Parse::toRect(infile.val);
			}
			// @ATTR text_pos|label|Position of the text displaying the enemy's name and level.
			else if(infile.key == "text_pos") {
				custom_text_pos = true;
				text_pos = Parse::popLabelInfo(infile.val);
			}
			else {
				infile.error("MenuEnemy: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	loadGraphics();

	align();
}

void MenuEnemy::loadGraphics() {
	Image *graphics;

	setBackground("images/menus/enemy_bar.png");

	graphics = render_device->loadImage("images/menus/enemy_bar_hp.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		bar_hp = graphics->createSprite();
		graphics->unref();
	}
}

void MenuEnemy::handleNewMap() {
	enemy = NULL;
}

void MenuEnemy::logic() {
	// after a fixed amount of time, hide the enemy display
	timeout.tick();
	if (timeout.isEnd())
		enemy = NULL;
}

void MenuEnemy::render() {
	if (enemy == NULL) return;
	if (enemy->stats.corpse && enemy->stats.corpse_timer.isEnd()) return;

	Rect src, dest;
	src.w = bar_pos.w;
	src.h = bar_pos.h;

	dest.x = window_area.x+bar_pos.x;
	dest.y = window_area.y+bar_pos.y;
	dest.w = bar_pos.w;
	dest.h = bar_pos.h;

	int hp_bar_length = 0;
	if (enemy->stats.get(Stats::HP_MAX) == 0)
		hp_bar_length = 0;
	else if (bar_hp)
		hp_bar_length = (enemy->stats.hp * bar_hp->getGraphics()->getWidth()) / enemy->stats.get(Stats::HP_MAX);

	// draw hp bar background
	setBackgroundClip(src);
	setBackgroundDest(dest);
	Menu::render();

	// draw hp bar fill
	if (bar_hp) {
		src.w = hp_bar_length;
		src.h = bar_pos.h;
		bar_hp->setClipFromRect(src);
		bar_hp->setDestFromRect(dest);
		render_device->render(bar_hp);
	}

	if (!text_pos.hidden) {
		// enemy name display
		label_text.setText(msg->get("%s level %d", enemy->stats.name, enemy->stats.level));
		label_text.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

		if (custom_text_pos) {
			label_text.setPos(window_area.x + text_pos.x, window_area.y + text_pos.y);
			label_text.setJustify(text_pos.justify);
			label_text.setVAlign(text_pos.valign);
			label_text.setFont(text_pos.font_style);
		}
		else {
			label_text.setPos(window_area.x + bar_pos.x + bar_pos.w/2, window_area.y + bar_pos.y);
			label_text.setJustify(FontEngine::JUSTIFY_CENTER);
			label_text.setVAlign(LabelInfo::VALIGN_BOTTOM);
		}
		label_text.render();

		// HP display
		std::stringstream ss;
		ss.str("");
		if (enemy->stats.hp > 0) {
			ss << enemy->stats.hp << "/" << enemy->stats.get(Stats::HP_MAX);
		}
		else {
			if (enemy->stats.lifeform)
				ss << msg->get("Dead");
			else
				ss << msg->get("Destroyed");
		}
		label_stats.setText(ss.str());

		label_stats.setPos(window_area.x + bar_pos.x + bar_pos.w/2, window_area.y + bar_pos.y + bar_pos.h/2);
		label_stats.setJustify(FontEngine::JUSTIFY_CENTER);
		label_stats.setVAlign(LabelInfo::VALIGN_CENTER);
		label_stats.render();
	}
}

MenuEnemy::~MenuEnemy() {
	if (bar_hp)
		delete bar_hp;
}
