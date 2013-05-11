/*
Copyright © 2011-2012 Clint Bellanger

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

/*
class ModManager

ModManager maintains a list of active mods and provides functions for checking
mods in priority order when loading data files.
*/

#pragma once
#ifndef MOD_MANAGER_H
#define MOD_MANAGER_H

#define FALLBACK_MOD "default"

#include "CommonIncludes.h"

class ModManager {
private:
	void loadModList();

	std::map<std::string,std::string> loc_cache;

public:
	ModManager();
	~ModManager();

	// Returns the filename within the latest mod, in which the provided generic
	// filename was found.
	std::string locate(const std::string& filename);

	// Returns a list of filenames, going through all mods, in which the provided
	// generic filename is found.
	// The list is ordered the same way as locate() is searching for files, so
	// files being at later positions in the list should overwrite previous files
	std::vector<std::string> list(const std::string& path);

	std::vector<std::string> mod_dirs;
	std::vector<std::string> mod_list;
};

#endif

