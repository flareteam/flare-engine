/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

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

#include "CommonIncludes.h"
#include "UtilsParsing.h"
#include "Settings.h"
#include <cstdlib>
#include <typeinfo>
#include <math.h>

std::string trim(std::string s, const std::string& delimiters) {
	return trim_left_inplace(trim_right_inplace(s, delimiters), delimiters);
}

std::string trim_left_inplace(std::string s, const std::string& delimiters) {
	return s.erase(0, s.find_first_not_of(delimiters));
}

std::string trim_right_inplace(std::string s, const std::string& delimiters) {
	return s.erase(s.find_last_not_of(delimiters) + 1);
}

/**
 * Parse a duration string and return duration in frames.
 */
int parse_duration(const std::string& s) {
	int val = 0;
	std::string suffix = "";
	std::stringstream ss;
	ss.str(s);
	ss >> val;
	ss >> suffix;

	if (val == 0)
		return val;
	else if (suffix == "s")
		val *= MAX_FRAMES_PER_SEC;
	else {
		if (suffix != "ms")
			logError("UtilsParsing: Duration of '%d' does not have a suffix. Assuming 'ms'.", val);
		val = static_cast<int>(floor((static_cast<float>(val*MAX_FRAMES_PER_SEC) / 1000.f) + 0.5f));
	}

	// round back up to 1 if we rounded down to 0 for ms
	if (val < 1) val = 1;

	return val;
}

int parse_direction(const std::string& s) {
	int dir;

	if (s == "N")
		dir = 3;
	else if (s == "NE")
		dir = 4;
	else if (s == "E")
		dir = 5;
	else if (s == "SE")
		dir = 6;
	else if (s == "S")
		dir = 7;
	else if (s == "SW")
		dir = 0;
	else if (s == "W")
		dir = 1;
	else if (s == "NW")
		dir = 2;
	else {
		dir = toInt(s);
		if (dir < 0 || dir > 7) {
			logError("UtilsParsing: Direction '%d' is not within range 0-7.");
			dir = 0;
		}
	}

	return dir;
}

ALIGNMENT parse_alignment(const std::string &s) {
	ALIGNMENT align = ALIGN_TOPLEFT;

	if (s == "topleft")
		align = ALIGN_TOPLEFT;
	else if (s == "top")
		align = ALIGN_TOP;
	else if (s == "topright")
		align = ALIGN_TOPRIGHT;
	else if (s == "left")
		align = ALIGN_LEFT;
	else if (s == "center")
		align = ALIGN_CENTER;
	else if (s == "right")
		align = ALIGN_RIGHT;
	else if (s == "bottomleft")
		align = ALIGN_BOTTOMLEFT;
	else if (s == "bottom")
		align = ALIGN_BOTTOM;
	else if (s == "bottomright")
		align = ALIGN_BOTTOMRIGHT;

	return align;
}

std::string parse_section_title(const std::string& s) {
	size_t bracket = s.find_first_of(']');
	if (bracket == std::string::npos) return ""; // not found
	return s.substr(1, bracket-1);
}

void parse_key_pair(const std::string& s, std::string &key, std::string &val) {
	size_t separator = s.find_first_of('=');
	if (separator == std::string::npos) {
		key = "";
		val = "";
		return; // not found
	}
	key = s.substr(0, separator);
	val = s.substr(separator+1, s.length());
	key = trim(key);
	val = trim(val);
}

/**
 * Given a string that starts with a decimal number then a comma
 * Return that int, and modify the string to remove the num and comma
 *
 * This is basically a really lazy "split" replacement
 */
int popFirstInt(std::string &s, char separator) {
	int num = 0;
	size_t seppos = s.find_first_of(separator);
	if (seppos == std::string::npos) {
		num = toInt(s);
		s = "";
	}
	else {
		num = toInt(s.substr(0, seppos));
		s = s.substr(seppos+1, s.length());
	}
	return num;
}

std::string popFirstString(std::string &s, char separator) {
	std::string outs = "";
	size_t seppos = s.find_first_of(separator);
	if (seppos == std::string::npos) {
		outs = s;
		s = "";
	}
	else {
		outs = s.substr(0, seppos);
		s = s.substr(seppos+1, s.length());
	}
	return outs;
}

// similar to popFirstString but does not alter the input string
std::string getNextToken(const std::string& s, size_t &cursor, char separator) {
	size_t seppos = s.find_first_of(separator, cursor);
	if (seppos == std::string::npos) { // not found
		cursor = std::string::npos;
		return "";
	}
	std::string outs = s.substr(cursor, seppos-cursor);
	cursor = seppos+1;
	return outs;
}

// strip carriage return if exists
std::string stripCarriageReturn(const std::string& line) {
	if (line.length() > 0) {
		if ('\r' == line.at(line.length()-1)) {
			return line.substr(0, line.length()-1);
		}
	}
	return line;
}

std::string getLine(std::ifstream &infile) {
	std::string line;
	// This is the standard way to check whether a read failed.
	if (!getline(infile, line))
		return "";
	line = stripCarriageReturn(line);
	return line;
}

bool tryParseValue(const std::type_info & type, const char * value, void * output) {
	return tryParseValue(type, std::string(value), output);
}

bool tryParseValue(const std::type_info & type, const std::string & value, void * output) {

	std::stringstream stream(value);

	if (type == typeid(bool)) {
		stream>>(bool&)*((bool*)output);
	}
	else if (type == typeid(int)) {
		stream>>(int&)*((int*)output);
	}
	else if (type == typeid(unsigned int)) {
		stream>>(unsigned int&)*((unsigned int*)output);
	}
	else if (type == typeid(short)) {
		stream>>(short&)*((short*)output);
	}
	else if (type == typeid(unsigned short)) {
		stream>>(unsigned short&)*((unsigned short*)output);
	}
	else if (type == typeid(char)) {
		stream>>(char&)*((char*)output);
	}
	else if (type == typeid(unsigned char)) {
		stream>>(unsigned char&)*((unsigned char*)output);
	}
	else if (type == typeid(float)) {
		stream>>(float&)*((float*)output);
	}
	else if (type == typeid(std::string)) {
		*((std::string *)output) = value;
	}
	else {
		logError("UtilsParsing: %s: a required type is not defined!", __FUNCTION__);
		return false;
	}

	return !stream.fail();
}

std::string toString(const std::type_info & type, void * value) {

	std::stringstream stream;

	if (type == typeid(bool)) {
		stream<<*((bool*)value);
	}
	else if (type == typeid(int)) {
		stream<<*((int*)value);
	}
	else if (type == typeid(unsigned int)) {
		stream<<*((unsigned int*)value);
	}
	else if (type == typeid(short)) {
		stream<<*((short*)value);
	}
	else if (type == typeid(unsigned short)) {
		stream<<*((unsigned short*)value);
	}
	else if (type == typeid(char)) {
		stream<<*((char*)value);
	}
	else if (type == typeid(unsigned char)) {
		stream<<*((unsigned char*)value);
	}
	else if (type == typeid(float)) {
		stream<<*((float*)value);
	}
	else if (type == typeid(std::string)) {
		return (std::string &)*((std::string *)value);
	}
	else {
		logError("UtilsParsing: %s: a required type is not defined!", __FUNCTION__);
		return "";
	}

	return stream.str();
}

int toInt(const std::string& s, int default_value) {
	int result;
	if (!(std::stringstream(s) >> result))
		result = default_value;
	return result;
}

float toFloat(const std::string& s, float default_value) {
	float result;
	if (!(std::stringstream(s) >> result))
		result = default_value;
	return result;
}

unsigned long toUnsignedLong(const std::string& s, unsigned long  default_value) {
	unsigned long result;
	if (!(std::stringstream(s) >> result))
		result = default_value;
	return result;
}

bool toBool(std::string value) {
	trim(value);

	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
	if (value == "true") return true;
	if (value == "yes") return true;
	if (value == "1") return true;
	if (value == "false") return false;
	if (value == "no") return false;
	if (value == "0") return false;

	logError("UtilsParsing: %s %s doesn't know how to handle %s", __FILE__, __FUNCTION__, value.c_str());
	return false;
}

Point toPoint(std::string value) {
	Point p;
	p.x = popFirstInt(value);
	p.y = popFirstInt(value);
	return p;
}

Rect toRect(std::string value) {
	Rect r;
	r.x = popFirstInt(value);
	r.y = popFirstInt(value);
	r.w = popFirstInt(value);
	r.h = popFirstInt(value);
	return r;
}

Color toRGB(std::string value) {
	Color c;
	c.r = static_cast<Uint8>(popFirstInt(value));
	c.g = static_cast<Uint8>(popFirstInt(value));
	c.b = static_cast<Uint8>(popFirstInt(value));
	return c;
}

Color toRGBA(std::string value) {
	Color c;
	c.r = static_cast<Uint8>(popFirstInt(value));
	c.g = static_cast<Uint8>(popFirstInt(value));
	c.b = static_cast<Uint8>(popFirstInt(value));
	c.a = static_cast<Uint8>(popFirstInt(value));
	return c;
}
