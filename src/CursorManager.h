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

#ifndef CURSORMANAGER_H
#define CURSORMANAGER_H

#include "CommonIncludes.h"
#include "Utils.h"

typedef	enum {
	CURSOR_NORMAL,
	CURSOR_INTERACT,
	CURSOR_TALK,
	CURSOR_ATTACK
} CURSOR_TYPE;

class CursorManager {
public:
	CursorManager ();
	~CursorManager ();
	void logic();
	void render();
	void setCursor(CURSOR_TYPE type);

private:
	Sprite *cursor_normal;
	Sprite *cursor_interact;
	Sprite *cursor_talk;
	Sprite *cursor_attack;

	Point offset_normal;
	Point offset_interact;
	Point offset_talk;
	Point offset_attack;

	Sprite *cursor_current;
	Point* offset_current;
};

#endif

