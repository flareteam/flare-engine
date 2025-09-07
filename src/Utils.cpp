/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert
Copyright © 2012-2016 Justin Jacobs

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

#include "Avatar.h"
#include "EngineSettings.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "Platform.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <cmath>
#include <stdarg.h>
#include <ctype.h>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string.h>

int Utils::LOCK_INDEX = 0;

bool Utils::LOG_FILE_INIT = false;
bool Utils::LOG_FILE_CREATED = false;
std::string Utils::LOG_PATH;
std::queue<std::pair<SDL_LogPriority, std::string> > Utils::LOG_MSG;

/**
 * Point: A simple x/y coordinate structure
 */
Point::Point()
	: x(0)
	, y(0)
{}

Point::Point(int _x, int _y)
	: x(_x)
	, y(_y)
{}

Point::Point(const FPoint& _fp)
	: x(static_cast<int>(_fp.x))
	, y(static_cast<int>(_fp.y))
{}

/**
 * FPoint: A floating point version of Point
 */
FPoint::FPoint()
	: x(0)
	, y(0)
{}

FPoint::FPoint(float _x, float _y)
	: x(_x)
	, y(_y)
{}

FPoint::FPoint(Point _p)
	: x(static_cast<float>(_p.x))
	, y(static_cast<float>(_p.y))
{}

FPoint::FPoint(int _x, int _y)
	: x(static_cast<float>(_x))
	, y(static_cast<float>(_y))
{}

void FPoint::align() {
	// this rounds the float values to the nearest multiple of 1/(2^4)
	// 1/(2^4) was chosen because it's a "nice" floating point number, removing 99% of rounding errors
	x = floorf(x / 0.0625f) * 0.0625f;
	y = floorf(y / 0.0625f) * 0.0625f;
}

/**
 * Rect: A rectangle defined by the top-left x/y and the width/height
 */
Rect::Rect()
	: x(0)
	, y(0)
	, w(0)
	, h(0)
{}

Rect::Rect(int _x, int _y, int _w, int _h)
	: x(_x)
	, y(_y)
	, w(_w)
	, h(_h)
{}

Rect::Rect(const SDL_Rect& _r)
	: x(_r.x)
	, y(_r.y)
	, w(_r.w)
	, h(_r.h)
{}

Rect::operator SDL_Rect() const {
	SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	return r;
}

/**
 * Color: RGBA color; defaults to 100% opaque black
 */
Color::Color()
	: r(0)
	, g(0)
	, b(0)
	, a(255)
{}

Color::Color(Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a)
	: r(_r)
	, g(_g)
	, b(_b)
	, a(_a)
{}

Color::operator SDL_Color() const {
	SDL_Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;
	return c;
}

bool Color::operator ==(const Color &other) {
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator !=(const Color &other) {
	return !((*this) == other);
}

uint32_t Color::encodeRGBA() {
	uint32_t result = static_cast<uint32_t>(a);
	result |= static_cast<uint32_t>(r) << 24;
	result |= static_cast<uint32_t>(g) << 16;
	result |= static_cast<uint32_t>(b) << 8;
	return result;
}

void Color::decodeRGBA(const uint32_t encoded) {
	a = static_cast<uint8_t>(encoded);
	r = static_cast<uint8_t>(encoded >> 24);
	g = static_cast<uint8_t>(encoded >> 16);
	b = static_cast<uint8_t>(encoded >> 8);
}

Timer::Timer(unsigned _duration)
	: current(0)
	, duration(_duration)
{
}

unsigned Timer::getCurrent() {
	return current;
}

unsigned Timer::getDuration() {
	return duration;
}

void Timer::setCurrent(unsigned val) {
	current = val;
	if (current > duration)
		current = duration;
}

void Timer::setDuration(unsigned val) {
	current = duration = val;
}

bool Timer::tick() {
	if (current > 0)
		current--;

	if (current == 0)
		return true;

	return false;
}

bool Timer::isEnd() {
	return current == 0;
}

bool Timer::isBegin() {
	return current == duration;
}

void Timer::reset(int type) {
	if (type == Timer::END)
		current = 0;
	else if (type == Timer::BEGIN)
		current = duration;
}

FMinMax::FMinMax()
	: min(0)
	, max(0)
{}

bool Timer::isWholeSecond() {
	return current % settings->max_frames_per_sec == 0;
}

FPoint Utils::screenToMap(int x, int y, float camx, float camy) {
	FPoint r;
	if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
		float scrx = float(x - settings->view_w_half) * 0.5f;
		float scry = float(y - settings->view_h_half) * 0.5f;

		r.x = (eset->tileset.units_per_pixel_x * scrx) + (eset->tileset.units_per_pixel_y * scry) + camx;
		r.y = (eset->tileset.units_per_pixel_y * scry) - (eset->tileset.units_per_pixel_x * scrx) + camy;
	}
	else {
		r.x = static_cast<float>(x - settings->view_w_half) * (eset->tileset.units_per_pixel_x) + camx;
		r.y = static_cast<float>(y - settings->view_h_half) * (eset->tileset.units_per_pixel_y) + camy;
	}
	return r;
}

/**
 * Returns a point (in map units) of a given (x,y) tupel on the screen
 * when the camera is at a given position.
 */
Point Utils::mapToScreen(float x, float y, float camx, float camy) {
	Point r;

	// adjust to the center of the viewport
	// we do this calculation first to avoid negative integer division
	float adjust_x = (settings->view_w_half + 0.5f) * eset->tileset.units_per_pixel_x;
	float adjust_y = (settings->view_h_half + 0.5f) * eset->tileset.units_per_pixel_y;

	if (eset->tileset.orientation == eset->tileset.TILESET_ISOMETRIC) {
		r.x = int(floorf(((x - camx - y + camy + adjust_x)/eset->tileset.units_per_pixel_x)+0.5f));
		r.y = int(floorf(((x - camx + y - camy + adjust_y)/eset->tileset.units_per_pixel_y)+0.5f));
	}
	else if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL) {
		r.x = int((x - camx + adjust_x)/eset->tileset.units_per_pixel_x);
		r.y = int((y - camy + adjust_y)/eset->tileset.units_per_pixel_y);
	}
	return r;
}

/**
 * Apply parameter distance to position and direction
 */
FPoint Utils::calcVector(const FPoint& pos, int direction, float dist) {
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

float Utils::calcDist(const FPoint& p1, const FPoint& p2) {
	return sqrtf((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
}

/**
 * is target within the area defined by center and radius?
 */
bool Utils::isWithinRadius(const FPoint& center, float radius, const FPoint& target) {
	return (calcDist(center, target) < radius);
}

/**
 * is target within the area defined by rectangle r?
 */
bool Utils::isWithinRect(const Rect& r, const Point& target) {
	return target.x >= r.x && target.y >= r.y && target.x < r.x+r.w && target.y < r.y+r.h;
}

unsigned char Utils::calcDirection(float x0, float y0, float x1, float y1) {
	float theta = calcTheta(x0, y0, x1, y1);
	float val = theta / (static_cast<float>(M_PI)/4);
	int dir = static_cast<int>(((val < 0) ? ceilf(val-0.5f) : floorf(val+0.5f)) + 4);
	dir = (dir + 1) % 8;
	if (dir >= 0)
		return static_cast<unsigned char>(dir);
	else
		return 0;
}

// convert cartesian to polar theta where (x1,x2) is the origin
float Utils::calcTheta(float x1, float y1, float x2, float y2) {
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
		theta = atanf(dy/dx);
		if (dx < 0.0 && dy >= 0.0) theta += static_cast<float>(M_PI);
		if (dx < 0.0 && dy < 0.0) theta -= static_cast<float>(M_PI);
	}
	return theta;
}

std::string Utils::abbreviateKilo(int amount) {
	std::stringstream ss;
	if (amount < 1000)
		ss << amount;
	else
		ss << (amount/1000) << msg->get("k");

	return ss.str();
}

void Utils::alignToScreenEdge(int alignment, Rect *r) {
	if (!r) return;

	if (alignment == ALIGN_TOPLEFT) {
		// do nothing
	}
	else if (alignment == ALIGN_TOP) {
		r->x = (settings->view_w_half - r->w/2) + r->x;
	}
	else if (alignment == ALIGN_TOPRIGHT) {
		r->x = (settings->view_w - r->w) + r->x;
	}
	else if (alignment == ALIGN_LEFT) {
		r->y = (settings->view_h_half - r->h/2) + r->y;
	}
	else if (alignment == ALIGN_CENTER) {
		r->x = (settings->view_w_half - r->w/2) + r->x;
		r->y = (settings->view_h_half - r->h/2) + r->y;
	}
	else if (alignment == ALIGN_RIGHT) {
		r->x = (settings->view_w - r->w) + r->x;
		r->y = (settings->view_h_half - r->h/2) + r->y;
	}
	else if (alignment == ALIGN_BOTTOMLEFT) {
		r->y = (settings->view_h - r->h) + r->y;
	}
	else if (alignment == ALIGN_BOTTOM) {
		r->x = (settings->view_w_half - r->w/2) + r->x;
		r->y = (settings->view_h - r->h) + r->y;
	}
	else if (alignment == ALIGN_BOTTOMRIGHT) {
		r->x = (settings->view_w - r->w) + r->x;
		r->y = (settings->view_h - r->h) + r->y;
	}
	else if (alignment == ALIGN_FRAME_TOPLEFT) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + r->y;
	}
	else if (alignment == ALIGN_FRAME_TOP) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + (eset->resolutions.frame_w/2 - r->w/2) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + r->y;
	}
	else if (alignment == ALIGN_FRAME_TOPRIGHT) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + (eset->resolutions.frame_w - r->w) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + r->y;
	}
	else if (alignment == ALIGN_FRAME_LEFT) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + (eset->resolutions.frame_h/2 - r->h/2) + r->y;
	}
	else if (alignment == ALIGN_FRAME_CENTER) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + (eset->resolutions.frame_w/2 - r->w/2) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + (eset->resolutions.frame_h/2 - r->h/2) + r->y;
	}
	else if (alignment == ALIGN_FRAME_RIGHT) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + (eset->resolutions.frame_w - r->w) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + (eset->resolutions.frame_h/2 - r->h/2) + r->y;
	}
	else if (alignment == ALIGN_FRAME_BOTTOMLEFT) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + (eset->resolutions.frame_h - r->h) + r->y;
	}
	else if (alignment == ALIGN_FRAME_BOTTOM) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + (eset->resolutions.frame_w/2 - r->w/2) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + (eset->resolutions.frame_h - r->h) + r->y;
	}
	else if (alignment == ALIGN_FRAME_BOTTOMRIGHT) {
		r->x = ((settings->view_w - eset->resolutions.frame_w)/2) + (eset->resolutions.frame_w - r->w) + r->x;
		r->y = ((settings->view_h - eset->resolutions.frame_h)/2) + (eset->resolutions.frame_h - r->h) + r->y;
	}
	else {
		// do nothing
	}
}

/**
 * These functions provide a unified way to log messages, printf-style
 */
void Utils::logInfo(const char* format, ...) {
	va_list args;

	va_start(args, format);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, format, args);
	va_end(args);

	char file_buf[BUFSIZ];
	va_start(args, format);
	vsnprintf(file_buf, BUFSIZ, format, args);
	va_end(args);

	if (!LOG_FILE_INIT) {
		LOG_MSG.push(std::pair<SDL_LogPriority, std::string>(SDL_LOG_PRIORITY_INFO, std::string(file_buf)));
	}
	else if (LOG_FILE_CREATED) {
		FILE *log_file = fopen(LOG_PATH.c_str(), "a");
		if (log_file) {
			fprintf(log_file, "INFO: ");
			fprintf(log_file, "%s", file_buf);
			fprintf(log_file, "\n");
			fclose(log_file);
		}
	}

}

void Utils::logError(const char* format, ...) {
	va_list args;

	va_start(args, format);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, format, args);
	va_end(args);

	char file_buf[BUFSIZ];
	va_start(args, format);
	vsnprintf(file_buf, BUFSIZ, format, args);
	va_end(args);

	if (!LOG_FILE_INIT) {
		LOG_MSG.push(std::pair<SDL_LogPriority, std::string>(SDL_LOG_PRIORITY_ERROR, std::string(file_buf)));
	}
	else if (LOG_FILE_CREATED) {
		FILE *log_file = fopen(LOG_PATH.c_str(), "a");
		if (log_file) {
			fprintf(log_file, "ERROR: ");
			fprintf(log_file, "%s", file_buf);
			fprintf(log_file, "\n");
			fclose(log_file);
		}
	}
}

void Utils::logErrorDialog(const char* dialog_text, ...) {
	char pre_buf[BUFSIZ];
	char buf[BUFSIZ];
	snprintf(pre_buf, BUFSIZ, "%s%s", "FLARE Error\n", dialog_text);

	va_list args;
	va_start(args, dialog_text);
	vsnprintf(buf, BUFSIZ, pre_buf, args);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "FLARE Error", buf, NULL);
	va_end(args);
}

void Utils::createLogFile() {
	LOG_PATH = settings->path_conf + "/flare_log.txt";

	// always create a new log file on each launch
	if (Filesystem::fileExists(LOG_PATH)) {
		Filesystem::removeFile(LOG_PATH);
	}

	FILE *log_file = fopen(LOG_PATH.c_str(), "w+");
	if (log_file) {
		LOG_FILE_CREATED = true;
		fprintf(log_file, "### Flare log file\n\n");

		while (!LOG_MSG.empty()) {
			if (LOG_MSG.front().first == SDL_LOG_PRIORITY_INFO)
				fprintf(log_file, "INFO: ");
			else if (LOG_MSG.front().first == SDL_LOG_PRIORITY_ERROR)
				fprintf(log_file, "ERROR: ");

			fprintf(log_file, "%s", LOG_MSG.front().second.c_str());
			fprintf(log_file, "\n");

			LOG_MSG.pop();
		}
		fclose(log_file);
	}
	else {
		while (!LOG_MSG.empty())
			LOG_MSG.pop();

		logError("Utils: Could not create log file.");
	}

	LOG_FILE_INIT = true;
}

void Utils::Exit(int code) {
	SDL_Quit();
	lockFileWrite(-1);
	exit(code);
}

void Utils::createSaveDir(int slot) {
	// game slots are currently 1-4
	if (slot == 0) return;

	std::stringstream ss;
	ss << settings->path_user << "saves/" << eset->misc.save_prefix << "/";

	Filesystem::createDir(ss.str());

	ss << slot;
	Filesystem::createDir(ss.str());

	Filesystem::createDir(ss.str() + "/fow/");

	Filesystem::createDir(ss.str() + "/maps/");
}

void Utils::removeSaveDir(int slot) {
	// game slots are currently 1-4
	if (slot == 0) return;

	std::stringstream ss;
	ss << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << slot;

	if (Filesystem::isDirectory(ss.str())) {
		Filesystem::removeDirRecursive(ss.str());
	}
}

Rect Utils::resizeToScreen(int w, int h, bool crop, int align) {
	Rect r;

	// fit to height
	float ratio = settings->view_h / static_cast<float>(h);
	r.w = static_cast<int>(static_cast<float>(w) * ratio);
	r.h = settings->view_h;

	if (!crop) {
		// fit to width
		if (r.w > settings->view_w) {
			ratio = settings->view_w / static_cast<float>(w);
			r.h = static_cast<int>(static_cast<float>(h) * ratio);
			r.w = settings->view_w;
		}
	}

	alignToScreenEdge(align, &r);

	return r;
}

size_t Utils::stringFindCaseInsensitive(const std::string &_a, const std::string &_b) {
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

std::string Utils::floatToString(const float value, size_t precision) {
	size_t format_buffer_size = 16;
	char format_buffer[format_buffer_size];
	snprintf(format_buffer, format_buffer_size, "%%.%df", static_cast<int>(precision));

	size_t buffer_size = 1024;
	char buffer[buffer_size];
	snprintf(buffer, buffer_size, format_buffer, value);
	std::string temp(buffer);

	// remove trailing zeros (and the separator if it is not needed)
	if (temp.find(".") != std::string::npos || temp.find(",") != std::string::npos) {
		size_t i = temp.length();
		size_t new_length = i;
		while (i > 0) {
			i--;
			if (temp[i] == '0')
				new_length = i;
			else if (temp[i] != '.' && temp[i] != ',') {
				break;
			}
			else {
				new_length = i;
				break;
			}
		}
		temp = temp.substr(0, new_length);
	}

	return temp;
}

std::string Utils::getDurationString(const int duration, size_t precision) {
	float real_duration = static_cast<float>(duration) / settings->max_frames_per_sec;
	std::string temp = floatToString(real_duration, precision);

	if (real_duration == 1.f) {
		return msg->getv("%s second", temp.c_str());
	}
	else {
		return msg->getv("%s seconds", temp.c_str());
	}
}

std::string Utils::substituteVarsInString(const std::string &_s, Avatar* avatar) {
	std::string s = _s;

	size_t begin = s.find("${");
	while (begin != std::string::npos) {
		size_t end = s.find("}");

		if (end == std::string::npos)
			break;

		size_t var_len = end-begin+1;
		std::string var = s.substr(begin,var_len);

		if (avatar && var == "${AVATAR_NAME}") {
			s.replace(begin, var_len, avatar->stats.name);
		}
		else if (avatar && var == "${AVATAR_CLASS}") {
			s.replace(begin, var_len, avatar->stats.getShortClass());
		}
		else if (var == "${INPUT_MOVEMENT}") {
			s.replace(begin, var_len, inpt->getMovementString());
		}
		else if (var == "${INPUT_ATTACK}") {
			s.replace(begin, var_len, inpt->getAttackString());
		}
		else {
			logError("'%s' is not a valid string variable name.", var.c_str());
			// strip the brackets from the variable
			s.replace(begin, var_len, var.substr(2, var.length()-3));
		}

		begin = s.find("${");
	}

	return s;
}

/**
 * Keep two points within a certain range
 */
FPoint Utils::clampDistance(float range, const FPoint& src, const FPoint& target) {
	FPoint limit_target = target;

	if (range > 0) {
		if (src.x+range < target.x)
			limit_target.x = src.x+range;
		if (src.x-range > target.x)
			limit_target.x = src.x-range;
		if (src.y+range < target.y)
			limit_target.y = src.y+range;
		if (src.y-range > target.y)
			limit_target.y = src.y-range;
	}

	return limit_target;
}

/**
 * Compares two rectangles and returns true if they overlap
 */
bool Utils::rectsOverlap(const Rect &a, const Rect &b) {
	Point a_1(a.x, a.y);
	Point a_2(a.x + a.w, a.y);
	Point a_3(a.x, a.y + a.h);
	Point a_4(a.x + a.w, a.y + a.h);

	Point b_1(b.x, b.y);
	Point b_2(b.x + b.w, b.y);
	Point b_3(b.x, b.y + b.h);
	Point b_4(b.x + b.w, b.y + b.h);

	bool a_in_b = isWithinRect(b, a_1) || isWithinRect(b, a_2) || isWithinRect(b, a_3) || isWithinRect(b, a_4);
	bool b_in_a = isWithinRect(a, b_1) || isWithinRect(a, b_2) || isWithinRect(a, b_3) || isWithinRect(a, b_4);

	return a_in_b || b_in_a;
}

int Utils::rotateDirection(int direction, int val) {
	direction += val;
	if (direction > 7)
		direction -= 7;
	else if (direction < 0)
		direction += 7;

	return direction;
}

std::string Utils::getTimeString(const unsigned long time) {
	std::stringstream ss;
	unsigned long hours = (time / 60) / 60;
	if (hours < 100)
		ss << std::setfill('0') << std::setw(2) << hours;
	else
		ss << hours;

	ss << ":";
	unsigned long minutes = (time / 60) % 60;
	ss << std::setfill('0') << std::setw(2) << minutes;

	ss << ":";
	unsigned long seconds = time % 60;
	ss << std::setfill('0') << std::setw(2) << seconds;

	return ss.str();
}

unsigned long Utils::hashString(const std::string& str) {
	std::locale loc;
	const std::collate<char>& coll = std::use_facet<std::collate<char> >(loc);
	return coll.hash(str.data(), str.data() + str.length());
}

char* Utils::strdup(const std::string& str) {
	size_t length = str.length() + 1;
	char *x = static_cast<char*>(malloc(length));
	if (!x)
		return NULL;
	memcpy(x, str.c_str(), length);
	return x;
}

void Utils::lockFileRead() {
	if (!platform.has_lock_file)
		return;

	std::string lock_file_path = Filesystem::convertSlashes(settings->path_conf + "flare_lock");

	std::ifstream infile;
	infile.open(lock_file_path.c_str(), std::ios::in);

	while (infile.good()) {
		std::string line = Parse::getLine(infile);

		if (line.length() == 0 || line.at(0) == '#')
			continue;

		LOCK_INDEX = Parse::toInt(line);
	}

	infile.close();
	infile.clear();

	if (LOCK_INDEX < 0)
		LOCK_INDEX = 0;
}

void Utils::lockFileWrite(int increment) {
	if (!platform.has_lock_file)
		return;

	std::string lock_file_path = settings->path_conf + "flare_lock";

	if (increment < 0) {
		if (LOCK_INDEX == 0)
			return;

		// refresh LOCK_INDEX in case any other instances were closed while this instance was running
		lockFileRead();
	}

	std::ofstream outfile;
	outfile.open(lock_file_path.c_str(), std::ios::out);
	if (outfile.is_open()) {
		LOCK_INDEX += increment;
		outfile << "# Flare lock file. Counts instances of Flare" << std::endl;
		outfile << LOCK_INDEX << std::endl;
		outfile.close();
		outfile.clear();
	}
}

void Utils::lockFileCheck() {
	if (!platform.has_lock_file)
		return;

	LOCK_INDEX = 0;

	lockFileRead();

	if (LOCK_INDEX > 0){
		const SDL_MessageBoxButtonData buttons[] = {
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT|SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Quit" },
			{ 0, 1, "Continue" },
			{ 0, 2, "Reset" },
			{ 0, 3, "Safe Video" },
		};
		const SDL_MessageBoxData messageboxdata = {
			SDL_MESSAGEBOX_INFORMATION,
			NULL,
			"Flare",
			"Flare is unable to launch properly. This may be because it did not exit properly, or because there is another instance running.\n\nIf Flare crashed, it is recommended to try 'Safe Video' mode. This will try launching Flare with the minimum video settings.\n\nIf Flare is already running, you may:\n- 'Quit' Flare (safe, recommended)\n- 'Continue' to launch another copy of Flare.\n- 'Reset' the counter which tracks the number of copies of Flare that are currently running.\n  If this dialog is shown every time you launch Flare, this option should fix it.",
			static_cast<int>(SDL_arraysize(buttons)),
			buttons,
			NULL
		};
		int buttonid = 0;
		SDL_ShowMessageBox(&messageboxdata, &buttonid);
		if (buttonid == 0) {
			lockFileWrite(1);
			Exit(1);
		}
		else if (buttonid == 2) {
			LOCK_INDEX = 0;
		}
		else if (buttonid == 3) {
			LOCK_INDEX = 0;
			settings->safe_video = true;
		}
	}

	lockFileWrite(1);
}

void Utils::setSDL_RGBA(Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	*rmask = 0xff000000;
	*gmask = 0x00ff0000;
	*bmask = 0x0000ff00;
	*amask = 0x000000ff;
#else
	*rmask = 0x000000ff;
	*gmask = 0x0000ff00;
	*bmask = 0x00ff0000;
	*amask = 0xff000000;
#endif
}

std::string Utils::createMinMaxString(float min, float max, size_t precision) {
	std::string r;
	if (min < max) {
		if (min != max)
			r = floatToString(min, precision) + '-';
		r += floatToString(max, precision);
	}
	else {
		r = floatToString(min, precision);
	}
	return r;
}

