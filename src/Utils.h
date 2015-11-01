/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert

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
 * Utils
 *
 * Various utility structures, enums, function
 */

#ifndef UTILS_H
#define UTILS_H

#include <SDL.h>
#include <stdint.h>
#include <string>

class Avatar;

class Point {
public:
	int x, y;
	Point() : x(0), y(0) {}
	Point(int _x, int _y) : x(_x), y(_y) {}
};

class FPoint {
public:
	float x, y;
	FPoint(Point _p) : x(static_cast<float>(_p.x)), y(static_cast<float>(_p.y)) {}
	FPoint() : x(0), y(0) {}
	FPoint(float _x, float _y) : x(_x), y(_y) {}
};

class Rect {
public:
	int x, y, w, h;
	Rect() : x(0), y(0), w(0), h(0) {}
	Rect(SDL_Rect _r) : x(_r.x), y(_r.y), w(_r.w), h(_r.h) {}
	operator SDL_Rect() {
		SDL_Rect r;
		r.x = x;
		r.y = y;
		r.w = w;
		r.h = h;
		return r;
	}
};

class Color {
public:
	Uint8 r, g, b, a;
	Color() : r(0), g(0), b(0), a(255) {}
	Color(Uint8 _r, Uint8 _g, Uint8 _b) : r(_r), g(_g), b(_b), a(255) {}
	Color(Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a) : r(_r), g(_g), b(_b), a(_a) {}
	operator SDL_Color() {
		SDL_Color c;
		c.r = r;
		c.g = g;
		c.b = b;
		c.a = a;
		return c;
	}
	bool operator ==(const Color &other) {
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}
	bool operator !=(const Color &other) {
		return !((*this) == other);
	}
};

typedef enum {
	ALIGN_TOPLEFT = 0,
	ALIGN_TOP = 1,
	ALIGN_TOPRIGHT = 2,
	ALIGN_LEFT = 3,
	ALIGN_CENTER = 4,
	ALIGN_RIGHT = 5,
	ALIGN_BOTTOMLEFT = 6,
	ALIGN_BOTTOM = 7,
	ALIGN_BOTTOMRIGHT = 8
}ALIGNMENT;

typedef enum {
	EC_NONE = 0,
	EC_TOOLTIP = 1,
	EC_POWER_PATH = 2,
	EC_POWER_DAMAGE = 3,
	EC_INTERMAP = 4,
	EC_INTRAMAP = 5,
	EC_MAPMOD = 6,
	EC_SOUNDFX = 7,
	EC_LOOT = 8,
	EC_LOOT_COUNT = 9,
	EC_MSG = 10,
	EC_SHAKYCAM = 11,
	EC_REQUIRES_STATUS = 12,
	EC_REQUIRES_NOT_STATUS = 13,
	EC_REQUIRES_LEVEL = 14,
	EC_REQUIRES_NOT_LEVEL = 15,
	EC_REQUIRES_CURRENCY = 16,
	EC_REQUIRES_NOT_CURRENCY = 17,
	EC_REQUIRES_ITEM = 18,
	EC_REQUIRES_NOT_ITEM = 19,
	EC_REQUIRES_CLASS = 20,
	EC_REQUIRES_NOT_CLASS = 21,
	EC_SET_STATUS = 22,
	EC_UNSET_STATUS = 23,
	EC_REMOVE_CURRENCY = 24,
	EC_REMOVE_ITEM = 25,
	EC_REWARD_XP = 26,
	EC_REWARD_CURRENCY = 27,
	EC_REWARD_ITEM = 28,
	EC_RESTORE = 29,
	EC_POWER = 30,
	EC_SPAWN = 31,
	EC_STASH = 32,
	EC_NPC = 33,
	EC_MUSIC = 34,
	EC_CUTSCENE = 35,
	EC_REPEAT = 36,
	EC_SAVE_GAME = 37,
	EC_BOOK = 38,
	EC_NPC_ID = 39,
	EC_NPC_HOTSPOT = 40,
	EC_NPC_DIALOG_THEM = 41,
	EC_NPC_DIALOG_YOU = 42,
	EC_NPC_VOICE = 43,
	EC_NPC_DIALOG_TOPIC = 44,
	EC_NPC_DIALOG_GROUP = 45,
	EC_NPC_ALLOW_MOVEMENT = 46,
	EC_QUEST_TEXT = 47,
	EC_WAS_INSIDE_EVENT_AREA = 48
}EVENT_COMPONENT_TYPE;

class Event_Component {
public:
	EVENT_COMPONENT_TYPE type;
	std::string s;
	int x;
	int y;
	int z;
	int a;
	int b;
	int c;

	Event_Component()
		: type(EC_NONE)
		, s("")
		, x(0)
		, y(0)
		, z(0)
		, a(0)
		, b(0)
		, c(0) {
	}
};

class EffectDef {
public:
	std::string id;
	std::string type;
	int icon;
	std::string animation;
	bool can_stack;
	bool render_above;

	EffectDef()
		: id("")
		, type("")
		, icon(-1)
		, animation("")
		, can_stack(true)
		, render_above(false) {
	}
};

Point floor(FPoint fp);
FPoint screen_to_map(int x, int y, float camx, float camy);
Point map_to_screen(float x, float y, float camx, float camy);
Point center_tile(Point p);
Point map_to_collision(FPoint p);
FPoint collision_to_map(Point p);
FPoint calcVector(FPoint pos, int direction, float dist);
float calcDist(FPoint p1, FPoint p2);
float calcTheta(float x1, float y1, float x2, float y2);
unsigned char calcDirection(float x0, float y0, float x1, float y1);
unsigned char calcDirection(const FPoint &src, const FPoint &dst);
bool isWithin(FPoint center, float radius, FPoint target);
bool isWithin(Rect r, Point target);

std::string abbreviateKilo(int amount);
void alignToScreenEdge(ALIGNMENT alignment, Rect *r);
void alignFPoint(FPoint *pos);

void logInfo(const char* format, ...);
void logError(const char* format, ...);
void logErrorDialog(const char* dialog_text);
void Exit(int code);

void createSaveDir(int slot);
void removeSaveDir(int slot);

Rect resizeToScreen(int w, int h, bool crop, ALIGNMENT align);

size_t stringFindCaseInsensitive(const std::string &_a, const std::string &_b);

std::string getDurationString(const int& duration);

std::string substituteVarsInString(const std::string &_s, Avatar* avatar = NULL);

#endif
