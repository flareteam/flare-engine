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
 * FileParser
 *
 * Abstract the generic key-value pair ini-style file format
 */

#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include "CommonIncludes.h"

class FileParser {
private:
	std::vector<std::string> filenames;
	unsigned current_index;
	std::string errormessage;

	std::ifstream infile;
	std::string line;

	unsigned line_number;

public:
	FileParser();
	~FileParser();

	/**
	 * @brief open
	 * @param filename
	 * The generic filename to be opened. This generic filename will be located
	 * by the ModManager.
	 * If this is a directory, all files in this directory will be opened.
	 *
	 * @param errormessage
	 * Optional parameter, will be printed to stderr together with the filename
	 * if an error occurs. If errormessage is empty, there will be no output to
	 * stderr in any case.
	 *
	 * @param locateFileName
	 * If this parameter is set to true, the filename will not be interpreted as
	 * a generic locatable filename and the ModManager is used to locate the
	 * actual filename before opening the file. It is true by default.
	 * If this is set to false, then the filename is interpreted as is.
	 *
	 * @return true if file could be opened successfully for reading.
	 */
	bool open(const std::string& filename, bool locateFileName = true, const std::string &errormessage = "Could not open text file");

	void close();
	bool next();
	std::string nextValue(); // next value inside one line.
	std::string getRawLine();
	void error(const char* format, ...);
	void incrementLineNum();

	/**
	 * @brief new_section is set to true whenever a new [section] starts. If opening
	 * multiple files it is also true whenever a new file is opened. Note: This
	 * applies to only the second and any subsequent file. The first file doesn't
	 * set new_section to true for the first line.
	 */
	bool new_section;
	std::string section;
	std::string key;
	std::string val;
};

#endif
