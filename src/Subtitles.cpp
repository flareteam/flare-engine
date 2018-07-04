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

#include <locale>
#include <cassert>

Subtitles::Subtitles()
	: current_id(-1)
	, visible(false)
	, background(NULL)
	, background_color(0,0,0,200)
	, visible_ticks(0)
{
	FileParser infile;
	// @CLASS Subtitles|Description of soundfx/subtitles.txt
	if (infile.open("soundfx/subtitles.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section && infile.section == "subtitle") {
				filename.resize(filename.size()+1);
				text.resize(text.size()+1);
			}
			else if (infile.section == "style") {
				if (infile.key == "text_pos") {
					// @ATTR style.text_pos|label|Position and style of the subtitle text.
					text_pos = eatLabelInfo(infile.val);
					label.setFromLabelInfo(text_pos);
				}
				else if (infile.key == "pos") {
					// @ATTR style.pos|point|Position of the subtitle text relative to alignment.
					label_pos = toPoint(infile.val);
				}
				else if (infile.key == "align") {
					// @ATTR style.align|alignment|Alignment of the subtitle text.
					label_alignment = parse_alignment(infile.val);
				}
				else if (infile.key == "background_color") {
					// @ATTR style.background_color|color, int : Color, Alpha|Color and alpha of the subtitle background rectangle.
					background_color = toRGBA(infile.val);
				}
			}

			if (filename.empty() || text.empty())
				continue;

			if (infile.key == "id") {
				// @ATTR subtitle.id|filename|Filename of the sound file that will trigger this subtitle.
				std::locale loc;
				const std::collate<char>& coll = std::use_facet<std::collate<char> >(loc);
				const std::string realfilename = mods->locate(infile.val);

				unsigned long sid = coll.hash(realfilename.data(), realfilename.data()+realfilename.length());
				filename.back() = sid;
			}
			else if (infile.key == "text") {
				// @ATTR subtitle.text|string|The subtitle text that will be displayed.
				text.back() = msg->get(infile.val);
			}
		}
	}

	label.setColor(font->getColor("menu_normal"));

	assert(filename.size() == text.size());
}

Subtitles::~Subtitles()
{
	if (background)
		delete background;
}

void Subtitles::setTextByID(unsigned long id) {
	if (id == static_cast<unsigned long>(-1) && visible_ticks == 0) {
		current_id = -1;
		current_text = "";

		if (background) {
			delete background;
			background = NULL;
		}

		return;
	}

	for (size_t i = 0; i < filename.size(); ++i) {
		if (filename[i] == id) {
			current_id = id;
			current_text = text[i];
			updateLabelAndBackground();

			// 1 second per 10 letters
			visible_ticks = static_cast<int>(current_text.length()) * (settings->max_frames_per_sec / 10);

			return;
		}
	}
}

void Subtitles::logic(unsigned long id) {
	if (!settings->subtitles)
		return;

	setTextByID(id);

	if (visible_ticks > 0)
		visible_ticks--;

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
	if (!visible)
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
	alignToScreenEdge(label_alignment, &r);
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
