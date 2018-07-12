/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
Copyright © 2013-2015 Justin Jacobs

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

#ifndef UTILS_PARSING_H
#define UTILS_PARSING_H

#include "Utils.h"

#include <string>
#include <typeinfo>

class LabelInfo;

namespace Parse {
	std::string trim(const std::string& s, const std::string& delimiters = " \f\n\r\t\v");

	std::string getSectionTitle(const std::string& s);
	void getKeyPair(const std::string& s, std::string& key, std::string& val);
	std::string stripCarriageReturn(const std::string& line);
	std::string getLine(std::ifstream& infile);
	bool tryParseValue(const std::type_info & type, const std::string & value, void * output);

	std::string toString(const std::type_info & type, void * value);
	int toInt(const std::string& s, int default_value = 0);
	float toFloat(const std::string &s, float default_value = 0.0);
	unsigned long toUnsignedLong(const std::string& s, unsigned long default_value = 0);
	bool toBool(std::string value);

	Point toPoint(std::string value);
	Rect toRect(std::string value);
	Color toRGB(std::string value);
	Color toRGBA(std::string value);

	int toDuration(const std::string& s);
	int toDirection(const std::string& s);
	int toAlignment(const std::string& s);

	int popFirstInt(std::string& s, char separator = 0);
	std::string popFirstString(std::string& s, char separator = 0);
	LabelInfo popLabelInfo(std::string val);
}

#endif
