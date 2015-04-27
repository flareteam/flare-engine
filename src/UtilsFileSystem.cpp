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

/**
 * UtilsFileSystem
 *
 * Various file system function wrappers. Abstracted here to hide OS-specific implementations
 */

#include "CommonIncludes.h"
#include "UtilsFileSystem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#endif

/**
 * Check to see if a directory/folder exists
 */
bool dirExists(const std::string &path) {
	struct stat st;
	return (stat(path.c_str(), &st) == 0);
}

bool pathExists(const std::string &path) {
	struct stat st;
	return (stat(path.c_str(), &st) == 0);
}

/**
 * Create this folder if it doesn't already exist
 */

void createDir(const std::string &path) {
	if (isDirectory(path))
		return;

#ifndef _WIN32
	// *nix implementation
	if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		perror("createDir");
	}
#endif

#ifdef _WIN32
	// win implementation
	std::string syscmd = std::string("mkdir \"") + path + std::string("\"");
	if (system(syscmd.c_str()) != 0) {
		perror("createDir");
	}
#endif
}

bool isDirectory(const std::string &path) {
	struct stat st;
	if (stat(path.c_str(), &st) == -1) {
		perror("isDirectory");
		return false;
	}
	else {
		return (st.st_mode & S_IFDIR) != 0;
	}
}

/**
 * Check to see if a file exists
 * The filename parameter should include the entire path to this file
 */
bool fileExists(const std::string &filename) {
	std::ifstream infile(filename.c_str());
	bool exists = infile.is_open();
	if (exists) infile.close();

	return exists;
}

/**
 * Returns a vector containing all filenames in a given folder with the given extension
 */
int getFileList(const std::string &dir, const std::string &ext, std::vector<std::string> &files) {

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
int getDirList(const std::string &dir, std::vector<std::string> &dirs) {

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

bool removeFile(const std::string &file) {
	if (remove(file.c_str()) != 0) {
		perror("removeFile");
		return false;
	}
	return true;
}

bool removeDir(const std::string &dir) {
	if (!isDirectory(dir))
		return false;

#ifndef _WIN32
	// *nix implementation
	if (rmdir(dir.c_str()) == -1) {
		perror("removeDir");
		return false;
	}
#endif

#ifdef _WIN32
	// win implementation
	std::string syscmd = std::string("rmdir \"") + dir + std::string("\"");
	system(syscmd.c_str());
#endif

	return true;
}

bool removeDirRecursive(const std::string &dir) {
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
std::string path(const std::stringstream* ss) {
	std::string path = ss->str();

	bool is_windows_path = false;

	int len = path.length();
	// fix mixed '\' and '/' on windows
	for (int i = 0; i < len; i++) {
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
