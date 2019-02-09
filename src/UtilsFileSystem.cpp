/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014-2016 Justin Jacobs

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
 * UtilsFileSystem
 *
 * Various file system function wrappers. Abstracted here to hide OS-specific implementations
 */

#include "CommonIncludes.h"
#include "Platform.h"
#include "UtilsFileSystem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

/**
 * Check to see if a directory/folder exists
 */
bool Filesystem::pathExists(const std::string &path) {
	struct stat st;
	return (stat(removeTrailingSlash(path).c_str(), &st) == 0);
}

/**
 * Create this folder if it doesn't already exist
 */

void Filesystem::createDir(const std::string &path) {
	if (isDirectory(path, false))
		return;

	platform.dirCreate(path);
}

/**
 * Check to see if a file exists
 * The filename parameter should include the entire path to this file
 */
bool Filesystem::fileExists(const std::string &filename) {
	if (isDirectory(filename, false)) return false;

	std::ifstream infile(filename.c_str());
	bool exists = infile.is_open();
	if (exists) infile.close();

	return exists;
}

/**
 * Returns a vector containing all filenames in a given folder with the given extension
 */
int Filesystem::getFileList(const std::string &dir, const std::string &ext, std::vector<std::string> &files) {

	DIR *dp;
	struct dirent *dirp;

	if((dp  = opendir(dir.c_str())) == NULL)
		return errno;

	size_t extlen = ext.length();
	while ((dirp = readdir(dp)) != NULL) {
		std::string filename = std::string(dirp->d_name);
		if (filename.length() > extlen)
			if(filename.substr(filename.length() - extlen,extlen) == ext)
				files.push_back(dir + "/" + filename);
	}
	closedir(dp);
	return 0;
}

/**
 * Returns a vector containing all directory names in a given directory
 */
int Filesystem::getDirList(const std::string &dir, std::vector<std::string> &dirs) {

	DIR *dp;
	struct dirent *dirp;
	struct stat st;

	if((dp  = opendir(dir.c_str())) == NULL) {
		return errno;
	}

	while ((dirp = readdir(dp)) != NULL) {
		//	do not use dirp->d_type, it's not portable
		std::string directory = std::string(dirp->d_name);
		std::string mod_dir = dir + "/" + directory;
		if ((stat(mod_dir.c_str(), &st) != -1)
				&& S_ISDIR(st.st_mode)
				&& directory != "."
				&& directory != ".."
		   ) {
			dirs.push_back(directory);
		}
	}
	closedir(dp);
	return 0;
}

bool Filesystem::isDirectory(const std::string &path, bool show_error) {
	struct stat st;
	if (stat(path.c_str(), &st) == -1) {
		if (show_error) {
			std::string error_msg = "Filesystem::isDirectory (" + path + ")";
			perror(error_msg.c_str());
		}
		return false;
	}
	else {
		return (st.st_mode & S_IFDIR) != 0;
	}
}

bool Filesystem::removeFile(const std::string &file) {
	if (remove(file.c_str()) != 0) {
		std::string error_msg = "Filesystem::removeFile (" + file + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

bool Filesystem::removeDir(const std::string &dir) {
	if (!isDirectory(dir))
		return false;

	return platform.dirRemove(dir);
}

bool Filesystem::removeDirRecursive(const std::string &dir) {
	std::vector<std::string> dir_list;
	std::vector<std::string> file_list;

	getDirList(dir, dir_list);
	while (!dir_list.empty()) {
		removeDirRecursive(dir + "/" + dir_list.back());
		dir_list.pop_back();
	}

	getFileList(dir, "txt", file_list);
	while (!file_list.empty()) {
		removeFile(file_list.back());
		file_list.pop_back();
	}

	removeDir(dir);

	return true;
}

/**
 * Convert from stringstream to filesystem path string in an os-independent fashion
 */
std::string Filesystem::convertSlashes(const std::stringstream* ss) {
	std::string path = ss->str();

	bool is_windows_path = false;

	size_t len = path.length();
	// fix mixed '\' and '/' on windows
	for (size_t i = 0; i < len; i++) {
		if (path[i] == '\\') {
			is_windows_path = true;
		}
		if (is_windows_path && path[i] == '/') {
			// isDirectory does not like trailing '\', so terminate string if last char
			if (i == len - 1) {
				path[i] = 0;
			}
			else {
				path[i] = '\\';
			}
		}
	}

	return path;
}

bool Filesystem::renameFile(const std::string &oldfile, const std::string &newfile) {
	if (rename(oldfile.c_str(), newfile.c_str()) != 0) {
		std::string error_msg = "Filesystem::renameFile (" + oldfile + " -> " + newfile + ")";
		perror(error_msg.c_str());
		return false;
	}
	return true;
}

std::string Filesystem::removeTrailingSlash(const std::string& path) {
	// windows
	if (!path.empty() && path.at(path.length()-1) == '\\')
		return path.substr(0, path.length()-1);

	// everything else
	// allow for *nix root /
	else if (path.length() > 1 && path.at(path.length()-1) == '/')
		return path.substr(0, path.length()-1);

	// no trailing slash found
	else
		return path;
}
