/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2015 Justin Jacobs

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

#include "FileParser.h"
#include "ModManager.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

#include <stdarg.h>

FileParser::FileParser()
	: current_index(0)
	, is_mod_file(false)
	, error_mode(ERROR_NORMAL)
	, line("")
	, line_number(0)
	, include_fp(NULL)
	, new_section(false)
	, section("")
	, key("")
	, val("") {
}

bool FileParser::open(const std::string& _filename, bool _is_mod_file, int _error_mode) {
	is_mod_file = _is_mod_file;
	error_mode = _error_mode;

	filenames.clear();
	if (is_mod_file) {
		filenames = mods->list(_filename, ModManager::LIST_FULL_PATHS);
	}
	else {
		filenames.push_back(_filename);
	}
	current_index = 0;
	line_number = 0;

	if (filenames.empty()) {
		if (error_mode != ERROR_NONE)
			Utils::logError("FileParser: Could not open text file: %s: No such file or directory!", _filename.c_str());
		return false;
	}

	bool ret = false;

	// Cycle through all filenames from the end, stopping when a file is to overwrite all further files.
	for (size_t i=filenames.size(); i>0; i--) {
		infile.open(filenames[i-1].c_str(), std::ios::in);
		ret = infile.is_open();

		if (ret) {
			// This will be the first file to be parsed. Seek to the start of the file and leave it open.
			if (infile.good() && Parse::trim(Parse::getLine(infile)) != "APPEND") {
				std::string test_line;

				// get the first non-comment, non blank line
				while (infile.good()) {
					test_line = Parse::trim(Parse::getLine(infile));
					if (test_line.length() == 0) continue;
					else if (test_line.at(0) == '#') continue;
					else break;
				}

				if (test_line != "APPEND") {
					current_index = static_cast<unsigned>(i)-1;
					infile.clear(); // reset flags
					infile.seekg(0, std::ios::beg);
					break;
				}
			}

			// don't close the final file if it's the only one with an "APPEND" line
			if (i > 1) {
				infile.close();
				infile.clear();
			}
		}
		else {
			if (error_mode != ERROR_NONE)
				Utils::logError("FileParser: Could not open text file: %s", filenames[i-1].c_str());
			infile.clear();
		}
	}

	return ret;
}

void FileParser::close() {
	if (include_fp) {
		include_fp->close();
		delete include_fp;
		include_fp = NULL;
	}

	if (infile.is_open())
		infile.close();
	infile.clear();
}

/**
 * Advance to the next key pair
 * Take note if a new section header is encountered
 *
 * @return false if EOF, otherwise true
 */
bool FileParser::next() {

	std::string starts_with;
	new_section = false;

	while (current_index < filenames.size()) {
		while (infile.good()) {
			if (include_fp) {
				if (include_fp->next()) {
					new_section = include_fp->new_section;
					section = include_fp->section;
					key = include_fp->key;
					val = include_fp->val;
					return true;
				}
				else {
					include_fp->close();
					delete include_fp;
					include_fp = NULL;
					continue;
				}
			}

			line = Parse::trim(Parse::getLine(infile));
			line_number++;

			// skip ahead if this line is empty
			if (line.length() == 0) continue;

			starts_with = line.at(0);

			// skip ahead if this line is a comment
			if (starts_with == "#") continue;

			// set new section if this line is a section declaration
			if (starts_with == "[") {
				new_section = true;
				section = Parse::getSectionTitle(line);

				// keep searching for a key-pair
				continue;
			}

			// skip the string used to combine files
			if (line == "APPEND") continue;

			// read from a separate file
			std::size_t first_space = line.find(' ');

			if (first_space != std::string::npos) {
				std::string directive = line.substr(0, first_space);

				if (directive == "INCLUDE") {
					std::string tmp = line.substr(first_space+1);

					include_fp = new FileParser();
					if (!include_fp || !include_fp->open(tmp, is_mod_file, error_mode)) {
						delete include_fp;
						include_fp = NULL;
					}

					// INCLUDE file will inherit the current section
					include_fp->section = section;

					continue;
				}
			}

			// this is a keypair. Perform basic parsing and return
			Parse::getKeyPair(line, key, val);
			return true;
		}

		infile.close();
		infile.clear();

		current_index++;
		if (current_index == filenames.size()) return false;

		line_number = 0;
		const std::string current_filename = filenames[current_index];
		infile.open(current_filename.c_str(), std::ios::in);
		if (!infile.is_open()) {
			if (error_mode != ERROR_NONE)
				Utils::logError("FileParser: Could not open text file: %s", current_filename.c_str());
			infile.clear();
			return false;
		}
		// a new file starts a new section
		new_section = true;
	}

	// hit the end of file
	return false;
}

/**
 * Get an unparsed, unfiltered line from the input file
 */
std::string FileParser::getRawLine() {
	line = "";

	if (infile.good()) {
		line = Parse::getLine(infile);
	}
	return line;
}

void FileParser::error(const char* format, ...) {
	char buffer[4096];
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	errorBuf(buffer);
}

void FileParser::errorBuf(const char* buffer) {
	if (include_fp) {
		include_fp->errorBuf(buffer);
	}
	else {
		std::stringstream ss;
		ss << "[" << filenames[current_index] << ":" << line_number << "] " << buffer;
		Utils::logError(ss.str().c_str());
	}
}

void FileParser::incrementLineNum() {
	line_number++;
}

FileParser::~FileParser() {
	close();
}
