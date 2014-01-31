/*
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

#include "CursorManager.h"
#include "FileParser.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

CursorManager::CursorManager()
	: cursor_current(NULL)
	, offset_current(NULL)
{
	FileParser infile;
	if (infile.open("engine/mouse_cursor.txt")) {
		while (infile.next()) {
			infile.val = infile.val + ',';

			if (infile.key == "normal") {
				cursor_normal.setGraphics(render_device->loadGraphicSurface(eatFirstString(infile.val,',')));
				offset_normal.x = eatFirstInt(infile.val, ',');
				offset_normal.y = eatFirstInt(infile.val, ',');
			}
			else if (infile.key == "interact") {
				cursor_interact.setGraphics(render_device->loadGraphicSurface(eatFirstString(infile.val,',')));
				offset_interact.x = eatFirstInt(infile.val, ',');
				offset_interact.y = eatFirstInt(infile.val, ',');
			}
			else if (infile.key == "talk") {
				cursor_talk.setGraphics(render_device->loadGraphicSurface(eatFirstString(infile.val,',')));
				offset_talk.x = eatFirstInt(infile.val, ',');
				offset_talk.y = eatFirstInt(infile.val, ',');
			}
			else if (infile.key == "attack") {
				cursor_attack.setGraphics(render_device->loadGraphicSurface(eatFirstString(infile.val,',')));
				offset_attack.x = eatFirstInt(infile.val, ',');
				offset_attack.y = eatFirstInt(infile.val, ',');
			}
		}
		infile.close();
	}
}

CursorManager::~CursorManager() {
	cursor_normal.clearGraphics();
	cursor_interact.clearGraphics();
	cursor_talk.clearGraphics();
	cursor_attack.clearGraphics();
}

void CursorManager::logic() {
	cursor_current = NULL;
	offset_current = NULL;

	if (!cursor_normal.graphicsIsNull()) {
		inpt->hideCursor();
		cursor_current = &cursor_normal;
		offset_current = &offset_normal;
	}
	else {
		// system cursor
		inpt->showCursor();
	}
}

void CursorManager::render() {
	if (cursor_current != NULL) {
		if (offset_current != NULL) {
			cursor_current->setDest(inpt->mouse.x+offset_current->x, inpt->mouse.y+offset_current->y);
		}
		else {
			cursor_current->setDest(inpt->mouse.x, inpt->mouse.y);
		}

		render_device->render(*cursor_current);
	}
}

void CursorManager::setCursor(CURSOR_TYPE type) {
	if (type == CURSOR_INTERACT && !cursor_interact.graphicsIsNull()) {
		inpt->hideCursor();
		cursor_current = &cursor_interact;
		offset_current = &offset_interact;
	}
	else if (type == CURSOR_TALK && !cursor_talk.graphicsIsNull()) {
		inpt->hideCursor();
		cursor_current = &cursor_talk;
		offset_current = &offset_talk;
	}
	else if (type == CURSOR_ATTACK && !cursor_attack.graphicsIsNull()) {
		inpt->hideCursor();
		cursor_current = &cursor_attack;
		offset_current = &offset_attack;
	}
	else if (!cursor_normal.graphicsIsNull()) {
		inpt->hideCursor();
		cursor_current = &cursor_normal;
		offset_current = &offset_normal;
	}
	else {
		// system cursor
		cursor_current = NULL;
		inpt->showCursor();
	}
}
