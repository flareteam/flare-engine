/*
Copyright © 2014 Justin Jacobs
Copyright © 2014 Henrik Andersson

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
#include "InputState.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

CursorManager::CursorManager()
	: show_cursor(true)
	, cursor_normal(NULL)
	, cursor_interact(NULL)
	, cursor_talk(NULL)
	, cursor_attack(NULL)
	, cursor_lhp_normal(NULL)
	, cursor_lhp_interact(NULL)
	, cursor_lhp_talk(NULL)
	, cursor_lhp_attack(NULL)
	, cursor_current(NULL)
	, offset_current(NULL) {
	Image *graphics;
	FileParser infile;
	// @CLASS CursorManager|Description of engine/mouse_cursor.txt
	if (infile.open("engine/mouse_cursor.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		while (infile.next()) {
			if (infile.key == "normal") {
				// @ATTR normal|filename|Filename of an image for the normal cursor.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_normal = graphics->createSprite();
					graphics->unref();
				}
				offset_normal = Parse::toPoint(infile.val);
			}
			else if (infile.key == "interact") {
				// @ATTR interact|filename|Filename of an image for the object interaction cursor.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_interact = graphics->createSprite();
					graphics->unref();
				}
				offset_interact = Parse::toPoint(infile.val);
			}
			else if (infile.key == "talk") {
				// @ATTR talk|filename|Filename of an image for the NPC interaction cursor.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_talk = graphics->createSprite();
					graphics->unref();
				}
				offset_talk = Parse::toPoint(infile.val);
			}
			else if (infile.key == "attack") {
				// @ATTR attack|filename|Filename of an image for the cursor when attacking enemies.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_attack = graphics->createSprite();
					graphics->unref();
				}
				offset_attack = Parse::toPoint(infile.val);
			}
			else if (infile.key == "lowhp_normal") {
				// @ATTR lowhp_normal|filename|Filename of an image for the normal cursor when health is low.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_lhp_normal = graphics->createSprite();
					graphics->unref();
				}
				offset_lhp_normal = Parse::toPoint(infile.val);
			}
			else if (infile.key == "lowhp_interact") {
				// @ATTR lowhp_interact|filename|Filename of an image for the object interaction cursor when health is low.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_lhp_interact = graphics->createSprite();
					graphics->unref();
				}
				offset_lhp_interact = Parse::toPoint(infile.val);
			}
			else if (infile.key == "lowhp_talk") {
				// @ATTR lowhp_talk|filename|Filename of an image for the NPC interaction cursor when health is low.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_lhp_talk = graphics->createSprite();
					graphics->unref();
				}
				offset_lhp_talk = Parse::toPoint(infile.val);
			}
			else if (infile.key == "lowhp_attack") {
				// @ATTR lowhp_attack|filename|Filename of an image for the cursor when attacking enemies and health is low.
				graphics = render_device->loadImage(Parse::popFirstString(infile.val), RenderDevice::ERROR_NORMAL);
				if (graphics) {
					cursor_lhp_attack = graphics->createSprite();
					graphics->unref();
				}
				offset_lhp_attack = Parse::toPoint(infile.val);
			}


			else {
				infile.error("CursorManager: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
}

CursorManager::~CursorManager() {
	if (cursor_normal) delete cursor_normal;
	if (cursor_interact) delete cursor_interact;
	if (cursor_talk) delete cursor_talk;
	if (cursor_attack) delete cursor_attack;
	if (cursor_lhp_normal) delete cursor_lhp_normal;
	if (cursor_lhp_interact) delete cursor_lhp_interact;
	if (cursor_lhp_talk) delete cursor_lhp_talk;
	if (cursor_lhp_attack) delete cursor_lhp_attack;
}

void CursorManager::logic() {
	if (!show_cursor)
		return;

	if (settings->hardware_cursor) {
		inpt->showCursor();
		return;
	}

	cursor_current = NULL;
	offset_current = NULL;

	if (cursor_normal) {
		inpt->hideCursor();
		cursor_current = cursor_normal;
		offset_current = &offset_normal;
	}
	else {
		// system cursor
		inpt->showCursor();
	}
}

void CursorManager::render() {
	if (settings->hardware_cursor || !show_cursor) return;

	if (cursor_current != NULL) {
		if (offset_current != NULL) {
			cursor_current->setDest(inpt->mouse.x+offset_current->x, inpt->mouse.y+offset_current->y);
		}
		else {
			cursor_current->setDest(inpt->mouse.x, inpt->mouse.y);
		}

		render_device->render(cursor_current);
	}
}

void CursorManager::setCursor(int type) {
	if (settings->hardware_cursor) return;

	if (type == CURSOR_INTERACT && cursor_interact) {
		inpt->hideCursor();
		cursor_current = cursor_interact;
		offset_current = &offset_interact;
	}
	else if (type == CURSOR_TALK && cursor_talk) {
		inpt->hideCursor();
		cursor_current = cursor_talk;
		offset_current = &offset_talk;
	}
	else if (type == CURSOR_ATTACK && cursor_attack) {
		inpt->hideCursor();
		cursor_current = cursor_attack;
		offset_current = &offset_attack;
	}

	else if (type == CURSOR_LHP_INTERACT && cursor_lhp_interact) {
		inpt->hideCursor();
		cursor_current = cursor_lhp_interact;
		offset_current = &offset_lhp_interact;
	}
	else if (type == CURSOR_LHP_TALK && cursor_lhp_talk) {
		inpt->hideCursor();
		cursor_current = cursor_lhp_talk;
		offset_current = &offset_lhp_talk;
	}
	else if (type == CURSOR_LHP_ATTACK && cursor_lhp_attack) {
		inpt->hideCursor();
		cursor_current = cursor_lhp_attack;
		offset_current = &offset_lhp_attack;
	}
	else if (type == CURSOR_LHP_NORMAL && cursor_lhp_normal) {
		inpt->hideCursor();
		cursor_current = cursor_lhp_normal;
		offset_current = &offset_lhp_normal;
	}
	else if (cursor_normal) {
		inpt->hideCursor();
		cursor_current = cursor_normal;
		offset_current = &offset_normal;
	}
	else {
		// system cursor
		cursor_current = NULL;
		inpt->showCursor();
	}
}
