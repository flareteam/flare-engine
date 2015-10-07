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

#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"

#include <cmath>
#include <stdarg.h>
#include <ctype.h>
#include <iomanip>

Point floor(FPoint fp) {
	Point result;
	result.x = int(fp.x);
	result.y = int(fp.y);
	return result;
}

FPoint screen_to_map(int x, int y, float camx, float camy) {
	FPoint r;
	if (TILESET_ORIENTATION == TILESET_ISOMETRIC) {
		float scrx = float(x - VIEW_W_HALF) * 0.5f;
		float scry = float(y - VIEW_H_HALF) * 0.5f;

		r.x = (UNITS_PER_PIXEL_X * scrx) + (UNITS_PER_PIXEL_Y * scry) + camx;
		r.y = (UNITS_PER_PIXEL_Y * scry) - (UNITS_PER_PIXEL_X * scrx) + camy;
	}
	else {
		r.x = static_cast<float>(x - VIEW_W_HALF) * (UNITS_PER_PIXEL_X) + camx;
		r.y = static_cast<float>(y - VIEW_H_HALF) * (UNITS_PER_PIXEL_Y) + camy;
	}
	return r;
}

/**
 * Returns a point (in map units) of a given (x,y) tupel on the screen
 * when the camera is at a given position.
 */
Point map_to_screen(float x, float y, float camx, float camy) {
	Point r;

	// adjust to the center of the viewport
	// we do this calculation first to avoid negative integer division
	float adjust_x = (VIEW_W_HALF + 0.5f) * UNITS_PER_PIXEL_X;
	float adjust_y = (VIEW_H_HALF + 0.5f) * UNITS_PER_PIXEL_Y;

	if (TILESET_ORIENTATION == TILESET_ISOMETRIC) {
		r.x = int(floor(((x - camx - y + camy + adjust_x)/UNITS_PER_PIXEL_X)+0.5f));
		r.y = int(floor(((x - camx + y - camy + adjust_y)/UNITS_PER_PIXEL_Y)+0.5f));
	}
	else { //TILESET_ORTHOGONAL
		r.x = int((x - camx + adjust_x)/UNITS_PER_PIXEL_X);
		r.y = int((y - camy + adjust_y)/UNITS_PER_PIXEL_Y);
	}
	return r;
}

Point center_tile(Point p) {
	if (TILESET_ORIENTATION == TILESET_ORTHOGONAL) {
		p.x += TILE_W_HALF;
		p.y += TILE_H_HALF;
	}
	else //TILESET_ISOMETRIC
		p.y += TILE_H_HALF;
	return p;
}

FPoint collision_to_map(Point p) {
	FPoint ret;
	ret.x = static_cast<float>(p.x) + 0.5f;
	ret.y = static_cast<float>(p.y) + 0.5f;
	return ret;
}

Point map_to_collision(FPoint p) {
	Point ret;
	ret.x = int(p.x);
	ret.y = int(p.y);
	return ret;
}

/**
 * Apply parameter distance to position and direction
 */
FPoint calcVector(FPoint pos, int direction, float dist) {
	FPoint p;
	p.x = pos.x;
	p.y = pos.y;

	float dist_straight = dist;
	float dist_diag = dist * 0.7071f; //  1/sqrt(2)

	switch (direction) {
		case 0:
			p.x -= dist_diag;
			p.y += dist_diag;
			break;
		case 1:
			p.x -= dist_straight;
			break;
		case 2:
			p.x -= dist_diag;
			p.y -= dist_diag;
			break;
		case 3:
			p.y -= dist_straight;
			break;
		case 4:
			p.x += dist_diag;
			p.y -= dist_diag;
			break;
		case 5:
			p.x += dist_straight;
			break;
		case 6:
			p.x += dist_diag;
			p.y += dist_diag;
			break;
		case 7:
			p.y += dist_straight;
			break;
	}
	return p;
}

float calcDist(FPoint p1, FPoint p2) {
	return static_cast<float>(sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y)));
}

/**
 * is target within the area defined by center and radius?
 */
bool isWithin(FPoint center, float radius, FPoint target) {
	return (calcDist(center, target) < radius);
}

/**
 * is target within the area defined by rectangle r?
 */
bool isWithin(Rect r, Point target) {
	return target.x >= r.x && target.y >= r.y && target.x < r.x+r.w && target.y < r.y+r.h;
}

unsigned char calcDirection(const FPoint &src, const FPoint &dst) {
	return calcDirection(src.x, src.y, dst.x, dst.y);
}

unsigned char calcDirection(float x0, float y0, float x1, float y1) {
	float theta = calcTheta(x0, y0, x1, y1);
	float val = theta / (static_cast<float>(M_PI)/4);
	int dir = static_cast<int>(((val < 0) ? ceil(val-0.5) : floor(val+0.5)) + 4);
	dir = (dir + 1) % 8;
	if (dir >= 0 && dir < 8)
		return static_cast<unsigned char>(dir);
	else
		return 0;
}

// convert cartesian to polar theta where (x1,x2) is the origin
float calcTheta(float x1, float y1, float x2, float y2) {
	// calculate base angle
	float dx = x2 - x1;
	float dy = y2 - y1;
	float exact_dx = x2 - x1;
	float theta;

	// convert cartesian to polar coordinates
	if (exact_dx == 0) {
		if (dy > 0.0) theta = static_cast<float>(M_PI)/2.0f;
		else theta = static_cast<float>(-M_PI)/2.0f;
	}
	else {
		theta = static_cast<float>(atan(dy/dx));
		if (dx < 0.0 && dy >= 0.0) theta += static_cast<float>(M_PI);
		if (dx < 0.0 && dy < 0.0) theta -= static_cast<float>(M_PI);
	}
	return theta;
}

std::string abbreviateKilo(int amount) {
	std::stringstream ss;
	if (amount < 1000)
		ss << amount;
	else
		ss << (amount/1000) << msg->get("k");

	return ss.str();
}

void alignToScreenEdge(ALIGNMENT alignment, Rect *r) {
	if (!r) return;

	if (alignment == ALIGN_TOPLEFT) {
		// do nothing
	}
	else if (alignment == ALIGN_TOP) {
		r->x = (VIEW_W_HALF-r->w/2)+r->x;
	}
	else if (alignment == ALIGN_TOPRIGHT) {
		r->x = (VIEW_W-r->w)+r->x;
	}
	else if (alignment == ALIGN_LEFT) {
		r->y = (VIEW_H_HALF-r->h/2)+r->y;
	}
	else if (alignment == ALIGN_CENTER) {
		r->x = (VIEW_W_HALF-r->w/2)+r->x;
		r->y = (VIEW_H_HALF-r->h/2)+r->y;
	}
	else if (alignment == ALIGN_RIGHT) {
		r->x = (VIEW_W-r->w)+r->x;
		r->y = (VIEW_H_HALF-r->h/2)+r->y;
	}
	else if (alignment == ALIGN_BOTTOMLEFT) {
		r->y = (VIEW_H-r->h)+r->y;
	}
	else if (alignment == ALIGN_BOTTOM) {
		r->x = (VIEW_W_HALF-r->w/2)+r->x;
		r->y = (VIEW_H-r->h)+r->y;
	}
	else if (alignment == ALIGN_BOTTOMRIGHT) {
		r->x = (VIEW_W-r->w)+r->x;
		r->y = (VIEW_H-r->h)+r->y;
	}
	else {
		// do nothing
	}
}

/**
 * Given a floating Point pos, the decimal is rounded to the nearest multiple of 1/(2^4)
 * 1/(2^4) was chosen because it's a "nice" floating point number, removing 99% of rounding errors
 */
void alignFPoint(FPoint *pos) {
	if (!pos) return;

	pos->x = static_cast<float>(floor(pos->x / 0.0625f) * 0.0625f);
	pos->y = static_cast<float>(floor(pos->y / 0.0625f) * 0.0625f);
}


/**
 * These functions provide a unified way to log messages, printf-style
 */
void logInfo(const char* format, ...) {
	va_list args;

	va_start(args, format);

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, format, args);
#else
	printf("INFO: ");
	vprintf(format, args);
	printf("\n");
#endif

	va_end(args);
}

void logError(const char* format, ...) {
	va_list args;

	va_start(args, format);

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, format, args);
#else
	printf("ERROR: ");
	vprintf(format, args);
	printf("\n");
#endif

	va_end(args);
}

void logErrorDialog(const char* dialog_text) {
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "FLARE", dialog_text, NULL);
#endif
}

void Exit(int code) {
	SDL_Quit();
	exit(code);
}
void createSaveDir(int slot) {
	// game slots are currently 1-4
	if (slot == 0) return;

	std::stringstream ss;
	ss << PATH_USER << "saves/" << SAVE_PREFIX << "/";

	createDir(path(&ss));

	ss << slot;
	createDir(path(&ss));
}

void removeSaveDir(int slot) {
	// game slots are currently 1-4
	if (slot == 0) return;

	std::stringstream ss;
	ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << slot;

	if (isDirectory(path(&ss))) {
		removeDirRecursive(path(&ss));
	}
}

Rect resizeToScreen(int w, int h, bool crop, ALIGNMENT align) {
	Rect r;

	// fit to height
	float ratio = VIEW_H / static_cast<float>(h);
	r.w = static_cast<int>(static_cast<float>(w) * ratio);
	r.h = VIEW_H;

	if (!crop) {
		// fit to width
		if (r.w > VIEW_W) {
			ratio = VIEW_W / static_cast<float>(w);
			r.h = static_cast<int>(static_cast<float>(h) * ratio);
			r.w = VIEW_W;
		}
	}

	alignToScreenEdge(align, &r);

	return r;
}

size_t stringFindCaseInsensitive(const std::string &_a, const std::string &_b) {
	std::string a;
	std::string b;

	for (size_t i=0; i<_a.size(); ++i) {
		a += static_cast<char>(tolower(static_cast<int>(_a[i])));
	}

	for (size_t i=0; i<_b.size(); ++i) {
		b += static_cast<char>(tolower(static_cast<int>(_b[i])));
	}

	return a.find(b);
}

std::string getDurationString(const int& duration) {
	float real_duration = static_cast<float>(duration) / MAX_FRAMES_PER_SEC;

	std::stringstream ss;
	ss << std::setprecision(3) << real_duration;

	if (real_duration == 1.f) {
		return msg->get("%s second", ss.str().c_str());
	}
	else {
		return msg->get("%s seconds", ss.str().c_str());
	}
}

