/*
Copyright © 2011-2012 Clint Bellanger
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

#include "CommonIncludes.h"
#include "ModManager.h"
#include "Platform.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "Version.h"

#include <cassert>

Mod::Mod()
	: name("")
	, description("")
	, game("")
	, version(new Version())
	, engine_min_version(new Version(VERSION_MIN))
	, engine_max_version(new Version(VERSION_MAX)) {
}

Mod::~Mod() {
	delete version;
	delete engine_min_version;
	delete engine_max_version;

	for (size_t i = 0; i < depends_min.size(); ++i) {
		delete depends_min[i];
	}

	for (size_t i = 0; i < depends_max.size(); ++i) {
		delete depends_max[i];
	}
}

Mod::Mod(const Mod &mod) {
	version = new Version();
	engine_min_version = new Version();
	engine_max_version = new Version();

	*this = mod;
}

Mod& Mod::operator=(const Mod &mod) {
	if (this == &mod)
		return *this;

	name = mod.name;
	description = mod.description;
	game = mod.game;
	*version = *mod.version;
	*engine_min_version = *mod.engine_min_version;
	*engine_max_version = *mod.engine_max_version;
	depends = mod.depends;

	for (size_t i = 0; i < depends_min.size(); ++i) {
		delete depends_min[i];
	}
	depends_min.resize(mod.depends_min.size(), NULL);
	for (size_t i = 0; i < depends_min.size(); ++i) {
		depends_min[i] = new Version(*mod.depends_min[i]);
	}

	for (size_t i = 0; i < depends_max.size(); ++i) {
		delete depends_max[i];
	}
	depends_max.resize(mod.depends_max.size(), NULL);
	for (size_t i = 0; i < depends_max.size(); ++i) {
		depends_max[i] = new Version(*mod.depends_max[i]);
	}

	assert(depends.size() == depends_min.size());
	assert(depends.size() == depends_max.size());

	return *this;
}

bool Mod::operator== (const Mod &mod) const {
	return this->name == mod.name;
}

bool Mod::operator!= (const Mod &mod) const {
	return !(*this == mod);
}

ModManager::ModManager(const std::vector<std::string> *_cmd_line_mods)
	: cmd_line_mods(_cmd_line_mods)
{
	loc_cache.clear();
	mod_dirs.clear();
	mod_list.clear();
	setPaths();

	std::vector<std::string> mod_dirs_other;
	getDirList(settings->path_data + "mods", mod_dirs_other);
	getDirList(settings->path_user + "mods", mod_dirs_other);

	for (unsigned i=0; i<mod_dirs_other.size(); ++i) {
		if (find(mod_dirs.begin(), mod_dirs.end(), mod_dirs_other[i]) == mod_dirs.end())
			mod_dirs.push_back(mod_dirs_other[i]);
	}

	loadModList();
	applyDepends();

	std::stringstream ss;
	ss << "Active mods: ";
	for (size_t i = 0; i < mod_list.size(); ++i) {
		ss << mod_list[i].name;
		if (*mod_list[i].version != VERSION_MIN)
			ss << " (" << versionToString(*mod_list[i].version) << ")";
		if (i < mod_list.size()-1)
			ss << ", ";
	}
	logInfo(ss.str().c_str());
}

/**
 * The mod list is in either:
 * 1. [PATH_CONF]/mods.txt
 * 2. [PATH_DATA]/mods/mods.txt
 * The mods.txt file shows priority/load order for mods
 *
 * File format:
 * One mod folder name per line
 * Later mods override previous mods
 */
void ModManager::loadModList() {
	bool found_any_mod = false;

	// Add the fallback mod by default
	// Note: if a default mod is not found in mod_dirs, the game will exit
	if (find(mod_dirs.begin(), mod_dirs.end(), FALLBACK_MOD) != mod_dirs.end()) {
		mod_list.push_back(loadMod(FALLBACK_MOD));
		found_any_mod = true;
	}

	// Add all other mods.
	if (!cmd_line_mods || cmd_line_mods->empty()) {
		std::ifstream infile;
		std::string line;
		std::string starts_with;

		std::string place1 = settings->path_conf + "mods.txt";
		std::string place2 = settings->path_data + "mods/mods.txt";

		infile.open(place1.c_str(), std::ios::in);

		if (!infile.is_open()) {
			infile.clear();
			infile.open(place2.c_str(), std::ios::in);
		}
		if (!infile.is_open()) {
			logError("ModManager: Error during loadModList() -- couldn't open mods.txt, to be located at:");
			logError("%s\n%s\n", place1.c_str(), place2.c_str());
		}

		while (infile.good()) {
			line = getLine(infile);

			// skip ahead if this line is empty
			if (line.length() == 0) continue;

			// skip comments
			starts_with = line.at(0);
			if (starts_with == "#") continue;

			// add the mod if it exists in the mods folder
			if (line != FALLBACK_MOD) {
				if (find(mod_dirs.begin(), mod_dirs.end(), line) != mod_dirs.end()) {
					mod_list.push_back(loadMod(line));
					found_any_mod = true;
				}
				else {
					logError("ModManager: Mod \"%s\" not found, skipping", line.c_str());
				}
			}
		}
		infile.close();
		infile.clear();
	}
	else {
		for (size_t i = 0; i < cmd_line_mods->size(); ++i) {
			std::string line = (*cmd_line_mods)[i];

			// add the mod if it exists in the mods folder
			if (line != FALLBACK_MOD) {
				if (find(mod_dirs.begin(), mod_dirs.end(), line) != mod_dirs.end()) {
					mod_list.push_back(loadMod(line));
					found_any_mod = true;
				}
				else {
					logError("ModManager: Mod \"%s\" not found, skipping", line.c_str());
				}
			}
		}

		saveMods();
	}

	if (!found_any_mod && mod_list.size() == 1) {
		logError("ModManager: Couldn't locate any Flare mod. Check if the game data are installed \
                  correctly. Expected to find the data in the $XDG_DATA_DIRS path, in \
				  /usr/local/share/flare/mods, or in the same folder as the executable. \
				  Try placing the mods folder in one of these locations.");
	}
}

/**
 * Find the location (mod file name) for this data file.
 * Use private loc_cache to prevent excessive disk I/O
 */
std::string ModManager::locate(const std::string& filename) {
	// if we have this location already cached, return it
	if (loc_cache.find(filename) != loc_cache.end()) {
		return loc_cache[filename];
	}

	// search through mods for the first instance of this filename
	std::string test_path;

	for (size_t i = mod_list.size(); i > 0; i--) {
		for (size_t j = 0; j < mod_paths.size(); j++) {
			test_path = mod_paths[j] + "mods/" + mod_list[i-1].name + "/" + filename;
			if (fileExists(test_path)) {
				loc_cache[filename] = test_path;
				return test_path;
			}
		}
	}

	// all else failing, simply return the filename if it exists
	test_path = settings->path_data + filename;
	if (!fileExists(test_path))
		test_path = "";

	return test_path;
}

void amendPathToVector(const std::string &path, std::vector<std::string> &vec) {
	if (pathExists(path)) {
		if (isDirectory(path)) {
			getFileList(path, "txt", vec);
		}
		else {
			vec.push_back(path);
		}
	}
}

std::vector<std::string> ModManager::list(const std::string &path, bool full_paths) {
	std::vector<std::string> ret;
	std::string test_path;

	for (size_t i = 0; i < mod_list.size(); ++i) {
		for (size_t j = mod_paths.size(); j > 0; j--) {
			test_path = mod_paths[j-1] + "mods/" + mod_list[i].name + "/" + path;
			amendPathToVector(test_path, ret);
		}
	}

	// we don't need to check for duplicates if there are no paths
	if (ret.empty()) return ret;

	if (!full_paths) {
		// reduce the each file path down to be relative to mods/
		for (unsigned i=0; i<ret.size(); ++i) {
			ret[i] = ret[i].substr(ret[i].rfind(path), ret[i].length());
		}

		// remove duplicates
		for (unsigned i = 0; i < ret.size(); ++i) {
			for (unsigned j = 0; j < i; ++j) {
				if (ret[i] == ret[j]) {
					ret.erase(ret.begin()+j);
					break;
				}
			}
		}
	}

	return ret;
}

void ModManager::setPaths() {
	// set some flags if directories are identical
	bool uniq_path_data = settings->path_user != settings->path_data;

	if (!settings->custom_path_data.empty()) {
		// if we're using a custom data path, give it priority
		// in fact, don't use PATH_DATA at all, since the two are equal if CUSTOM_PATH_DATA is set
		mod_paths.push_back(settings->custom_path_data);
		uniq_path_data = false;
	}
	mod_paths.push_back(settings->path_user);
	if (uniq_path_data) mod_paths.push_back(settings->path_data);
}

Mod ModManager::loadMod(const std::string& name) {
	Mod mod;
	std::ifstream infile;
	std::string starts_with, line, key, val;

	mod.name = name;

	// @CLASS ModManager|Description of mod settings.txt
	for (unsigned i=0; i<mod_paths.size(); ++i) {
		std::string path = mod_paths[i] + "mods/" + name + "/settings.txt";
		infile.open(path.c_str(), std::ios::in);

		while (infile.good()) {
			line = getLine(infile);
			key = "";
			val = "";

			// skip ahead if this line is empty
			if (line.length() == 0) continue;

			// skip comments
			starts_with = line.at(0);
			if (starts_with == "#") continue;

			parse_key_pair(line, key, val);

			if (key == "description") {
				// @ATTR description|string|Some text describing the mod.
				mod.description = val;
			}
			else if (key == "version") {
				// @ATTR version|version|The version number of this mod.
				*mod.version = stringToVersion(val);
			}
			else if (key == "requires") {
				// @ATTR requires|list(string)|A comma-separated list of the mods that are required in order to use this mod. The dependency version requirements can also be specified and separated by colons (e.g. fantasycore:0.1:2.0).
				std::string dep;
				val = val + ',';
				while ((dep = popFirstString(val)) != "") {
					std::string dep_full = dep + "::";

					mod.depends.push_back(popFirstString(dep_full, ':'));

					Version dep_min = stringToVersion(popFirstString(dep_full, ':'));
					Version dep_max = stringToVersion(popFirstString(dep_full, ':'));

					if (dep_min != VERSION_MIN && dep_max != VERSION_MIN && dep_min > dep_max)
						dep_max = dep_min;

					// empty min version also happens to be the default for min
					mod.depends_min.push_back(new Version(dep_min));

					if (dep_max == VERSION_MIN)
						mod.depends_max.push_back(new Version(VERSION_MAX));
					else
						mod.depends_max.push_back(new Version(dep_max));

					assert(mod.depends.size() == mod.depends_min.size());
					assert(mod.depends.size() == mod.depends_max.size());
				}
			}
			else if (key == "game") {
				// @ATTR game|string|The game which this mod belongs to (e.g. flare-game).
				mod.game = val;
			}
			else if (key == "engine_version_min") {
				// @ATTR engine_version_min|version|The minimum engine version required to use this mod.
				*mod.engine_min_version = stringToVersion(val);
			}
			else if (key == "engine_version_max") {
				// @ATTR engine_version_max|version|The maximum engine version required to use this mod.
				*mod.engine_max_version = stringToVersion(val);
			}
			else {
				logError("ModManager: Mod '%s' contains invalid key: '%s'", name.c_str(), key.c_str());
			}
		}
		if (infile.good()) {
			infile.close();
			infile.clear();
			break;
		}
		else {
			infile.close();
			infile.clear();
		}
	}

	// ensure that engine min version <= engine max version
	if (*mod.engine_min_version != VERSION_MIN && *mod.engine_min_version > *mod.engine_max_version)
		*mod.engine_max_version = *mod.engine_min_version;

	return mod;
}

void ModManager::applyDepends() {
	std::vector<Mod> new_mods;
	bool finished = true;
	std::string game;
	if (!mod_list.empty())
		game = mod_list.back().game;

	for (unsigned i=0; i<mod_list.size(); i++) {
		// skip the mod if the game doesn't match
		if (game != FALLBACK_GAME && mod_list[i].game != FALLBACK_GAME && mod_list[i].game != game && mod_list[i].name != FALLBACK_MOD) {
			logError("ModManager: Tried to enable \"%s\", but failed. Game does not match \"%s\".", mod_list[i].name.c_str(), game.c_str());
			continue;
		}

		// skip the mod if it's incompatible with this engine version
		if (*mod_list[i].engine_min_version > ENGINE_VERSION || ENGINE_VERSION > *mod_list[i].engine_max_version) {
			logError("ModManager: Tried to enable \"%s\", but failed. Not compatible with engine version %s.", mod_list[i].name.c_str(), versionToString(ENGINE_VERSION).c_str());
			continue;
		}

		// skip the mod if it's already in the new_mods list
		if (std::find(new_mods.begin(), new_mods.end(), mod_list[i]) != new_mods.end()) {
			continue;
		}

		bool depends_met = true;

		for (unsigned j=0; j<mod_list[i].depends.size(); j++) {
			bool found_depend = false;

			// try to add the dependecy to the new_mods list
			for (unsigned k=0; k<new_mods.size(); k++) {
				if (new_mods[k].name == mod_list[i].depends[j]) {
					found_depend = true;
				}
				if (!found_depend) {
					// if we don't already have this dependency, try to load it from the list of available mods
					if (find(mod_dirs.begin(), mod_dirs.end(), mod_list[i].depends[j]) != mod_dirs.end()) {
						Mod new_depend = loadMod(mod_list[i].depends[j]);
						if (game != FALLBACK_GAME && new_depend.game != FALLBACK_GAME && new_depend.game != game) {
							logError("ModManager: Tried to enable dependency \"%s\" for \"%s\", but failed. Game does not match \"%s\".", new_depend.name.c_str(), mod_list[i].name.c_str(), game.c_str());
							depends_met = false;
							break;
						}
						else if (*new_depend.engine_min_version > ENGINE_VERSION || ENGINE_VERSION > *new_depend.engine_max_version) {
							logError("ModManager: Tried to enable dependency \"%s\" for \"%s\", but failed. Not compatible with engine version %s.", new_depend.name.c_str(), mod_list[i].name.c_str(), versionToString(ENGINE_VERSION).c_str());
							depends_met = false;
							break;
						}
						else if (*new_depend.version < *mod_list[i].depends_min[j] || *new_depend.version > *mod_list[i].depends_max[j]) {
							logError("ModManager: Tried to enable dependency \"%s\" for \"%s\", but failed. Version \"%s\" is required, but only version \"%s\" is available.",
									new_depend.name.c_str(),
									mod_list[i].name.c_str(),
									createVersionReqString(*mod_list[i].depends_min[j], *mod_list[i].depends_max[j]).c_str(),
									versionToString(*new_depend.version).c_str()
							);
							depends_met = false;
							break;
						}
						else if (std::find(new_mods.begin(), new_mods.end(), new_depend) == new_mods.end()) {
							logError("ModManager: Mod \"%s\" requires the \"%s\" mod. Enabling \"%s\" now.", mod_list[i].name.c_str(), mod_list[i].depends[j].c_str(), mod_list[i].depends[j].c_str());
							new_mods.push_back(new_depend);
							finished = false;
							break;
						}
					}
					else {
						logError("ModManager: Could not find mod \"%s\", which is required by mod \"%s\". Disabling \"%s\" now.", mod_list[i].depends[j].c_str(), mod_list[i].name.c_str(), mod_list[i].name.c_str());
						depends_met = false;
						break;
					}
				}
			}
			if (!depends_met) break;
		}

		if (depends_met) {
			if (std::find(new_mods.begin(), new_mods.end(), mod_list[i]) == new_mods.end()) {
				new_mods.push_back(mod_list[i]);
			}
		}
	}

	mod_list = new_mods;

	// run recursivly until no more dependencies need to be met
	if (!finished)
		applyDepends();
}

bool ModManager::haveFallbackMod() {
	for (unsigned i=0; i<mod_list.size(); ++i) {
		if (mod_list[i].name == FALLBACK_MOD)
			return true;
	}
	return false;
}

void ModManager::saveMods() {
	std::ofstream outfile;
	outfile.open((settings->path_conf + "mods.txt").c_str(), std::ios::out);

	if (outfile.is_open()) {
		// comment
		outfile << "## flare-engine mods list file ##" << "\n";

		outfile << "# Mods lower on the list will overwrite data in the entries higher on the list" << "\n";
		outfile << "\n";

		for (unsigned int i = 0; i < mod_list.size(); i++) {
			if (mod_list[i].name != FALLBACK_MOD)
				outfile << mod_list[i].name << "\n";
		}
	}
	if (outfile.bad())
		logError("GameStateConfigBase: Unable to save mod list into file. No write access or disk is full!");

	outfile.close();
	outfile.clear();

	PLATFORM.FSCommit();

}

void ModManager::resetModConfig() {
	std::string config_path = settings->path_conf + "mods.txt";
	logError("ModManager: Game data is either missing or misconfigured. Deleting '%s' in attempt to recover.", config_path.c_str());
	removeFile(config_path);
}

ModManager::~ModManager() {
}
