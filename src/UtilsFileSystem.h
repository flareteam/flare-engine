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


#pragma once
#ifndef UTILS_FILE_SYSTEM_H
#define UTILS_FILE_SYSTEM_H

#include "CommonIncludes.h"

bool dirExists(const std::string &path);
bool pathExists(const std::string &path);
void createDir(std::string path);
bool fileExists(std::string filename);
int getFileList(std::string dir, std::string ext, std::vector<std::string> &files);
int getDirList(std::string dir, std::vector<std::string> &dirs);


bool isDirectory(const std::string &path);

#endif
