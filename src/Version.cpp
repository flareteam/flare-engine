/*
Copyright © 2018 Justin Jacobs

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
 * Version number
 */

#include "MessageEngine.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "Version.h"

#include <limits.h>
#include <sstream>
#include <iomanip>

#include <SDL.h>

Version VersionInfo::ENGINE(1, 12, 7);
Version VersionInfo::MIN(0, 0, 0);
Version VersionInfo::MAX(USHRT_MAX, USHRT_MAX, USHRT_MAX);

Version::Version(unsigned short _x, unsigned short _y, unsigned short _z)
	: x(_x)
	, y(_y)
	, z(_z) {
}

Version::~Version() {
}

bool Version::operator==(const Version& v) {
	return x == v.x && y == v.y && z == v.z;
}

bool Version::operator!=(const Version& v) {
	return !(*this == v);
}

bool Version::operator>(const Version& v) {
	if (x > v.x)
		return true;
	else if (x == v.x && y > v.y)
		return true;
	else if (x == v.x && y == v.y && z > v.z)
		return true;
	else
		return false;
}

bool Version::operator>=(const Version& v) {
	return (*this == v || *this > v);
}

bool Version::operator<(const Version& v) {
	return !(*this >= v);
}

bool Version::operator<=(const Version& v) {
	return (*this == v || *this < v);
}

std::string Version::getString() {
	std::stringstream ss;

	// major
	ss << x << '.';

	// minor
	if (y >= 100 || y == 0) {
		ss << y;
	}
	else {
		ss << std::setfill('0') << std::setw(2);
		ss << y;
	}

	// don't bother printing if there's no patch version
	if (z == 0)
		return ss.str();

	ss << '.';

	// patch
	if (z >= 100) {
		ss << z;
	}
	else {
		ss << std::setfill('0') << std::setw(2);
		ss << z;
	}

	return ss.str();
}

void Version::setFromString(const std::string& s) {
	std::string val = s + '.';

	x = static_cast<unsigned short>(Parse::popFirstInt(val, '.'));

	std::string str_y = Parse::popFirstString(val, '.');
	y = static_cast<unsigned short>(Parse::toInt(str_y));
	if (str_y.length() == 1)
		y = static_cast<unsigned short>(y * 10);

	std::string str_z = Parse::popFirstString(val, '.');
	z = static_cast<unsigned short>(Parse::toInt(str_z));
	if (str_z.length() == 1)
		z = static_cast<unsigned short>(z * 10);
}

std::string VersionInfo::createVersionReqString(Version& v1, Version& v2) {
	std::string min_version = (v1 == MIN) ? "" : v1.getString();
	std::string max_version = (v2 == MAX) ? "" : v2.getString();
	std::string ret;

	if (min_version != "" || max_version != "") {
		if (min_version == max_version) {
			ret += min_version;
		}
		else if (min_version != "" && max_version != "") {
			ret += min_version + " - " + max_version;
		}
		else if (min_version != "") {
			ret += min_version + ' ' + (msg ? msg->get("or newer") : "or newer");
		}
		else if (max_version != "") {
			ret += max_version + ' ' + (msg ? msg->get("or older") : "or older");
		}
	}

	return ret;
}

std::string VersionInfo::createVersionStringFull() {
	// example output: Flare 1.0 (Linux)
	return NAME + " " + ENGINE.getString() + " (" + std::string(SDL_GetPlatform()) + ")";
}
