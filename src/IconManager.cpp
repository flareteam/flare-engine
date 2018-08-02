/*
Copyright © 2016 Justin Jacobs

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
#include "IconManager.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

IconSet::IconSet()
	: gfx(NULL)
	, id_begin(0)
	, id_end(0)
	, columns(1)
{
}

IconManager::IconManager()
	: current_set(NULL)
{
	FileParser infile;

	// @CLASS IconManager|Description of engine/icons.txt
	if (infile.open("engine/icons.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		while (infile.next()) {
			if (infile.key == "icon_set") {
				// @ATTR icon_set|repeatable(icon_id, filename) : First ID, Image file|Defines an icon graphics file to load, as well as the index of the first icon.
				int first_id = Parse::popFirstInt(infile.val);
				std::string filename = Parse::popFirstString(infile.val);

				icon_sets.resize(icon_sets.size()+1);
				if (!loadIconSet(icon_sets.back(), filename, first_id)) {
					icon_sets.pop_back();
				}
			}
			else if (infile.key == "text_offset") {
				// @ATTR text_offset|point|A pixel offset from the top-left to place item quantity text on icons.
				text_offset = Parse::toPoint(infile.val);
			}
		}
		infile.close();
	}

	if (icon_sets.empty()) {
		// no icons.txt file, so load icons.png legacy-style
		icon_sets.resize(1);
		if (!loadIconSet(icon_sets.back(), "images/icons/icons.png", 0)) {
			icon_sets.pop_back();
		}
	}
}

IconManager::~IconManager() {
	for (size_t i = 0; i < icon_sets.size(); ++i) {
		delete icon_sets[i].gfx;
	}
}

bool IconManager::loadIconSet(IconSet& iset, const std::string& filename, int first_id) {
	if (!render_device || eset->resolutions.icon_size == 0)
		return false;

	Image *graphics = render_device->loadImage(filename, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		iset.gfx = graphics->createSprite();
		graphics->unref();
	}

	if (iset.gfx) {
		int rows = iset.gfx->getGraphicsHeight() / eset->resolutions.icon_size;
		iset.columns = iset.gfx->getGraphicsWidth() / eset->resolutions.icon_size;

		if (iset.columns == 0) {
			// prevent divide-by-zero
			iset.columns = 1;
		}

		iset.id_begin = first_id;
		iset.id_end = first_id + (iset.columns * rows) - 1;

		return true;
	}

	return false;
}

void IconManager::setIcon(int icon_id, Point dest_pos) {
	if (icon_sets.empty()) {
		current_set = NULL;
		return;
	}

	for (size_t i = icon_sets.size(); i > 0; --i) {
		// we iterate backwards through the set list, since sets at the end have priority when sets overlap
		if (icon_id >= icon_sets[i-1].id_begin && icon_id <= icon_sets[i-1].id_end) {
			current_set = &icon_sets[i-1];
			break;
		}
		else if (i-1 == 0) {
			// we've reached the end of the set list, but could not find our icon
			current_set = NULL;
			return;
		}
	}

	int offset_id = icon_id - current_set->id_begin;
	current_src.x = (offset_id % current_set->columns) * eset->resolutions.icon_size;
	current_src.y = (offset_id / current_set->columns) * eset->resolutions.icon_size;
	current_src.w = current_src.h = eset->resolutions.icon_size;
	current_set->gfx->setClipFromRect(current_src);

	current_dest.x = dest_pos.x;
	current_dest.y = dest_pos.y;
	current_set->gfx->setDestFromRect(current_dest);

}

void IconManager::renderToImage(Image *img) {
	if (!current_set)
		return;

	if (img) {
		render_device->renderToImage(current_set->gfx->getGraphics(), current_src, img, current_dest);
	}
}

void IconManager::render() {
	if (!current_set)
		return;

	render_device->render(current_set->gfx);
}

