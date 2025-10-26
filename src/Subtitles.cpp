/*
Copyright © 2017 Justin Jacobs

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
 * class Subtitles
 *
 * Handles loading and displaying subtitles for sound files
 */

#include "FileParser.h"
#include "FontEngine.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Subtitles.h"
#include "Utils.h"
#include "UtilsParsing.h"

#include <cassert>

Subtitles::Subtitles()
	: current_id(-1)
	, visible(false)
	, background(NULL)
	, background_color(0,0,0,200)
	, visible_timer()
{
	FileParser infile;

	Subtitle temp;
	Subtitle *current = &temp;

	// @CLASS Subtitles|Description of soundfx/subtitles.txt
	if (infile.open("soundfx/subtitles.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section && infile.section == "subtitle") {
				temp = Subtitle();
				current = &temp;
			}

			if (infile.section == "style") {
				if (infile.key == "text_pos") {
					// @ATTR style.text_pos|label|Position and style of the subtitle text.
					label.setFromLabelInfo(Parse::popLabelInfo(infile.val));
				}
				else if (infile.key == "pos") {
					// @ATTR style.pos|point|Position of the subtitle text relative to alignment.
					label_pos = Parse::toPoint(infile.val);
				}
				else if (infile.key == "align") {
					// @ATTR style.align|alignment|Alignment of the subtitle text.
					label_alignment = Parse::toAlignment(infile.val);
				}
				else if (infile.key == "background_color") {
					// @ATTR style.background_color|color, int : Color, Alpha|Color and alpha of the subtitle background rectangle.
					background_color = Parse::toRGBA(infile.val);
				}
			}
			else if (infile.section == "subtitle") {
				// if we want to replace a list item by ID, the ID needs to be parsed first
				// but it is not essential if we're just adding to the list, so this is simply a warning
				if (infile.key != "id" && current->filename == 0) {
					infile.error("Subtitles: Expected 'id', but found '%s'.", infile.key.c_str());
				}

				if (infile.key == "id") {
					// @ATTR subtitle.id|filename|Filename of the sound file that will trigger this subtitle.
					unsigned long filename_hash = Utils::hashString(mods->locate(infile.val));

					bool found_id = false;
					for (size_t i = 0; i < subtitles.size(); ++i) {
						if (subtitles[i].filename == filename_hash) {
							current = &subtitles[i];
							found_id = true;
						}
					}
					if (!found_id) {
						subtitles.push_back(temp);
						current = &(subtitles.back());
						current->filename = filename_hash;
					}
				}
				else if (infile.key == "text") {
					// @ATTR subtitle.text|string|The subtitle text that will be displayed.
					current->text = msg->get(infile.val);
				}
			}
		}
	}

	label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));
}

Subtitles::~Subtitles()
{
	if (background)
		delete background;
}

void Subtitles::setTextByID(unsigned long id) {
	if (id == static_cast<unsigned long>(-1) && visible_timer.isEnd()) {
		current_id = -1;
		current_text = "";

		if (background) {
			delete background;
			background = NULL;
		}

		return;
	}

	for (size_t i = 0; i < subtitles.size(); ++i) {
		if (subtitles[i].filename == id) {
			current_id = id;
			current_text = subtitles[i].text;
			updateLabelAndBackground();

			// 1 second per 10 letters
			visible_timer.setDuration(static_cast<int>(current_text.length()) * (settings->max_frames_per_sec / 10));

			return;
		}
	}
}

void Subtitles::logic(unsigned long id) {
	setTextByID(id);

	visible_timer.tick();

	if (current_text.empty()) {
		visible = false;
		return;
	}
	else {
		visible = true;
	}

	updateLabelAndBackground();
}

void Subtitles::render() {
	if (!visible || !settings->subtitles)
		return;

	if (background) {
		render_device->render(background);
	}

	label.render();
}

void Subtitles::updateLabelAndBackground() {
	// position subtitle
	Rect r;
	r.x = label_pos.x;
	r.y = label_pos.y;
	Utils::alignToScreenEdge(label_alignment, &r);
	label.setPos(r.x, r.y);
	label.setText(current_text);

	// background is transparent, no need to create a background surface
	if (background_color.a == 0)
		return;

	// create padded background rectangle
	Rect old_background_rect = background_rect;
	background_rect = *label.getBounds();
	int padding = font->getLineHeight()/4;
	background_rect.x -= padding;
	background_rect.y -= padding;
	background_rect.w += padding*2;
	background_rect.h += padding*2;

	// update our background surface if needed
	if (!background || old_background_rect.w != background_rect.w || old_background_rect.h != background_rect.h) {
		if (background) {
			delete background;
			background = NULL;
		}

		// fill the background rectangle
		Image *temp = render_device->createImage(background_rect.w, background_rect.h);
		if (temp) {
			// translucent black background
			temp->fillWithColor(background_color);
			background = temp->createSprite();
			temp->unref();
		}
	}

	if (background) {
		background->setDest(background_rect.x, background_rect.y);
	}
}
