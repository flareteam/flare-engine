/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2015 Justin Jacobs

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

#ifndef UTILS_FILE_SYSTEM_H
#define UTILS_FILE_SYSTEM_H

#include <string>

namespace Filesystem {
	bool pathExists(const std::string &path);
	void createDir(const std::string &path);
	bool fileExists(const std::string &filename);
	int getFileList(const std::string &dir, const std::string &ext, std::vector<std::string> &files);
	int getDirList(const std::string &dir, std::vector<std::string> &dirs);

	bool isDirectory(const std::string &path, bool show_error = true);

	bool removeFile(const std::string &file);
	bool removeDir(const std::string &dir);
	bool removeDirRecursive(const std::string &dir);

	std::string convertSlashes(const std::stringstream* ss);

	bool renameFile(const std::string &oldfile, const std::string &newfile);

	std::string removeTrailingSlash(const std::string& path);
}


#endif
