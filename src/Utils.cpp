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

int LOCK_INDEX = 0;

bool LOG_FILE_INIT = false;
bool LOG_FILE_CREATED = false;
std::string LOG_PATH;
std::queue<std::pair<SDL_LogPriority, std::string> > LOG_MSG;

Point FPointToPoint(const FPoint& fp) {
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

FPoint collision_to_map(const Point& p) {
	FPoint ret;
	ret.x = static_cast<float>(p.x) + 0.5f;
	ret.y = static_cast<float>(p.y) + 0.5f;
	return ret;
}

Point map_to_collision(const FPoint& p) {
	Point ret;
	ret.x = int(p.x);
	ret.y = int(p.y);
	return ret;
}

/**
 * Apply parameter distance to position and direction
 */
FPoint calcVector(const FPoint& pos, int direction, float dist) {
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

float calcDist(const FPoint& p1, const FPoint& p2) {
	return static_cast<float>(sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y)));
}

/**
 * is target within the area defined by center and radius?
 */
bool isWithinRadius(const FPoint& center, float radius, const FPoint& target) {
	return (calcDist(center, target) < radius);
}

/**
 * is target within the area defined by rectangle r?
 */
bool isWithinRect(const Rect& r, const Point& target) {
	return target.x >= r.x && target.y >= r.y && target.x < r.x+r.w && target.y < r.y+r.h;
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
		theta = atanf(dy/dx);
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

void logError(const char* format, ...) {
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

void logErrorDialog(const char* dialog_text, ...) {
	char pre_buf[BUFSIZ];
	char buf[BUFSIZ];
	snprintf(pre_buf, BUFSIZ, "%s%s", "FLARE Error\n", dialog_text);

	va_list args;
	va_start(args, dialog_text);
	vsnprintf(buf, BUFSIZ, pre_buf, args);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "FLARE Error", buf, NULL);
	va_end(args);
}

void createLogFile() {
	LOG_PATH = PATH_CONF + "/flare_log.txt";

	// always create a new log file on each launch
	if (fileExists(LOG_PATH)) {
		removeFile(LOG_PATH);
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

void Exit(int code) {
	SDL_Quit();
	lockFileWrite(-1);
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

std::string floatToString(const float value, size_t precision) {
	std::stringstream ss;
	ss << value;
	std::string temp = ss.str();

	size_t decimal = temp.find(".");
	if (decimal != std::string::npos && temp.length() > decimal + precision + 1) {
		temp = temp.substr(0, decimal + precision + 1);
	}

	return temp;
}

std::string getDurationString(const int duration, size_t precision) {
	float real_duration = static_cast<float>(duration) / MAX_FRAMES_PER_SEC;
	std::string temp = floatToString(real_duration, precision);

	if (real_duration == 1.f) {
		return msg->get("%s second", temp.c_str());
	}
	else {
		return msg->get("%s seconds", temp.c_str());
	}
}

std::string substituteVarsInString(const std::string &_s, Avatar* avatar) {
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
		else if (var == "${INPUT_CONTINUE}") {
			s.replace(begin, var_len, inpt->getContinueString());
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
FPoint clampDistance(float range, const FPoint& src, const FPoint& target) {
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
bool rectsOverlap(const Rect &a, const Rect &b) {
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

int rotateDirection(int direction, int val) {
	direction += val;
	if (direction > 7)
		direction -= 7;
	else if (direction < 0)
		direction += 7;

	return direction;
}

std::string getTimeString(const unsigned long time, bool show_seconds) {
	std::stringstream ss;
	unsigned long hours = (time / 60) / 60;
	if (hours < 100)
		ss << std::setfill('0') << std::setw(2) << hours;
	else
		ss << hours;

	ss << ":";
	unsigned long minutes = (time / 60) % 60;
	ss << std::setfill('0') << std::setw(2) << minutes;

	if (show_seconds) {
		ss << ":";
		unsigned long seconds = time % 60;
		ss << std::setfill('0') << std::setw(2) << seconds;
	}

	return ss.str();
}

void lockFileRead() {
	if (!platform_options.has_lock_file)
		return;

	std::string lock_file_path = PATH_CONF + "flare_lock";

	std::ifstream infile;
	infile.open(lock_file_path.c_str(), std::ios::in);

	while (infile.good()) {
		std::string line = getLine(infile);

		if (line.length() == 0 || line.at(0) == '#')
			continue;

		LOCK_INDEX = toInt(line);
	}

	infile.close();
	infile.clear();

	if (LOCK_INDEX < 0)
		LOCK_INDEX = 0;
}

void lockFileWrite(int increment) {
	if (!platform_options.has_lock_file)
		return;

	std::string lock_file_path = PATH_CONF + "flare_lock";

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

void lockFileCheck() {
	if (!platform_options.has_lock_file)
		return;

	std::string lock_file_path = PATH_CONF + "flare_lock";
	LOCK_INDEX = 0;

	lockFileRead();

	if (LOCK_INDEX > 0){
		const SDL_MessageBoxButtonData buttons[] = {
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT|SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Quit" },
			{ 0, 1, "Continue" },
			{ 0, 2, "Reset Lock File" },
		};
		const SDL_MessageBoxData messageboxdata = {
			SDL_MESSAGEBOX_INFORMATION,
			NULL,
			"Flare",
			"Flare appears to already be running.\nYou may either 'Quit' Flare (recommended) or 'Continue' to launch a new instance.\n\nIf Flare is NOT already running, you can use 'Reset Lock File' to fix it.",
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
	}

	lockFileWrite(1);
}

