/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson

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

#include "CommonIncludes.h"
#include "Utils.h"
#include <typeinfo>

std::string trim(std::string s, const std::string& delimiters = " \f\n\r\t\v");
std::string trim_left_inplace(std::string s, const std::string& delimiters = " \f\n\r\t\v");
std::string trim_right_inplace(std::string s, const std::string& delimiters = " \f\n\r\t\v");
int parse_duration(const std::string& s);
int parse_direction(const std::string& s);
ALIGNMENT parse_alignment(const std::string& s);
std::string parse_section_title(const std::string& s);
void parse_key_pair(const std::string& s, std::string& key, std::string& val);
int popFirstInt(std::string& s, char separator = ',');
std::string popFirstString(std::string& s, char separator = ',');
std::string getNextToken(const std::string& s, size_t& cursor, char separator);
std::string stripCarriageReturn(const std::string& line);
std::string getLine(std::ifstream& infile);
bool tryParseValue(const std::type_info & type, const char * value, void * output);
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

#endif
