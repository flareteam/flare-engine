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
#include <queue>

typedef unsigned long SoundID;
typedef unsigned long StatusID;

class Avatar;
class FPoint; // needed for Point -> FPoint constructor

class Point {
public:
	int x, y;
	Point();
	Point(int _x, int _y);
	explicit Point(const FPoint& _fp);
};

class FPoint {
public:
	float x, y;
	FPoint();
	FPoint(float _x, float _y);
	explicit FPoint(Point _p);
	void align();
};

class Rect {
public:
	int x, y, w, h;
	Rect();
	Rect(int _x, int _y, int _w, int _h);
	explicit Rect(const SDL_Rect& _r);
	operator SDL_Rect() const;
};

class Color {
public:
	Uint8 r, g, b, a;
	Color();
	Color(Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a = 255);
	operator SDL_Color() const;
	bool operator ==(const Color &other);
	bool operator !=(const Color &other);
};

class Timer {
private:
	unsigned current;
	unsigned duration;
public:
	enum {
		END = 0,
		BEGIN = 1
	};

	Timer(unsigned _duration = 0);
	unsigned getCurrent();
	unsigned getDuration();
	void setCurrent(unsigned val);
	void setDuration(unsigned val);
	bool tick();
	bool isEnd();
	bool isBegin();
	void reset(int type);
};

namespace Utils {
	// Alignment: For aligning objects relative to the screen
	enum {
		ALIGN_TOPLEFT = 0,
		ALIGN_TOP = 1,
		ALIGN_TOPRIGHT = 2,
		ALIGN_LEFT = 3,
		ALIGN_CENTER = 4,
		ALIGN_RIGHT = 5,
		ALIGN_BOTTOMLEFT = 6,
		ALIGN_BOTTOM = 7,
		ALIGN_BOTTOMRIGHT = 8
	};

	extern int LOCK_INDEX;
	extern bool LOG_FILE_INIT;
	extern bool LOG_FILE_CREATED;
	extern std::string LOG_PATH;
	extern std::queue<std::pair<SDL_LogPriority, std::string> > LOG_MSG;

	FPoint screenToMap(int x, int y, float camx, float camy);
	Point mapToScreen(float x, float y, float camx, float camy);
	FPoint calcVector(const FPoint& pos, int direction, float dist);
	float calcDist(const FPoint& p1, const FPoint& p2);
	float calcTheta(float x1, float y1, float x2, float y2);
	unsigned char calcDirection(float x0, float y0, float x1, float y1);
	bool isWithinRadius(const FPoint& center, float radius, const FPoint& target);
	bool isWithinRect(const Rect& r, const Point& target);

	std::string abbreviateKilo(int amount);
	void alignToScreenEdge(int alignment, Rect *r);

	void logInfo(const char* format, ...);
	void logError(const char* format, ...);
	void logErrorDialog(const char* dialog_text, ...);
	void createLogFile();
	void Exit(int code);

	void createSaveDir(int slot);
	void removeSaveDir(int slot);

	Rect resizeToScreen(int w, int h, bool crop, int align);

	size_t stringFindCaseInsensitive(const std::string &_a, const std::string &_b);

	std::string floatToString(const float value, size_t precision);
	std::string getDurationString(const int duration, size_t precision);

	std::string substituteVarsInString(const std::string &_s, Avatar* avatar);

	FPoint clampDistance(float range, const FPoint& src, const FPoint& target);

	bool rectsOverlap(const Rect &a, const Rect &b);

	int rotateDirection(int direction, int val);

	std::string getTimeString(const unsigned long time);

	unsigned long hashString(const std::string& str);

	char* strdup(const std::string& str);

	void lockFileRead();
	void lockFileWrite(int increment);
	void lockFileCheck();
}

#endif
