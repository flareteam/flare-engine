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

#ifndef VERSION_H
#define VERSION_H

#include <string>

class Version {
public:
	unsigned short x; // major
	unsigned short y; // minor
	unsigned short z; // patch

	Version(unsigned short _x = 0, unsigned short _y = 0, unsigned short _z = 0);
	~Version();

	bool operator==(const Version& v);
	bool operator!=(const Version& v);
	bool operator>(const Version&v);
	bool operator>=(const Version&v);
	bool operator<(const Version&v);
	bool operator<=(const Version&v);
};

const std::string VERSION_NAME = "Flare";
extern Version ENGINE_VERSION;
extern Version VERSION_MIN;
extern Version VERSION_MAX;

std::string versionToString(const Version& v);
Version stringToVersion(const std::string& s);
std::string createVersionReqString(Version& v1, Version& v2);
std::string createVersionStringFull();

#endif

