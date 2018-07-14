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

	std::string getString();
	void setFromString(const std::string& s);
};

namespace VersionInfo {
	const std::string NAME = "Flare";
	extern Version ENGINE;
	extern Version MIN;
	extern Version MAX;

	std::string createVersionReqString(Version& v1, Version& v2);
	std::string createVersionStringFull();
}

#endif

