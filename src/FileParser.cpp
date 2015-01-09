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

#include "FileParser.h"
#include "UtilsParsing.h"
#include "UtilsFileSystem.h"
#include "SharedResources.h"

#include <stdarg.h>

FileParser::FileParser()
	: current_index(0)
	, line("")
	, line_number(0)
	, new_section(false)
	, section("")
	, key("")
	, val("") {
}

bool FileParser::open(const std::string& _filename, bool locateFileName, const std::string &_errormessage) {
	filenames.clear();
	if (locateFileName) {
		filenames = mods->list(_filename);
	}
	else {
		filenames.push_back(_filename);
	}
	current_index = 0;
	line_number = 0;
	this->errormessage = _errormessage;

	if (filenames.size() == 0 && !errormessage.empty()) {
		logError("FileParser: %s: %s: No such file or directory!", _filename.c_str(), errormessage.c_str());
		return false;
	}

	bool ret = false;

	// Cycle through all filenames from the end, stopping when a file is to overwrite all further files.
	for (unsigned i=filenames.size(); i>0; i--) {
		infile.open(filenames[i-1].c_str(), std::ios::in);
		ret = infile.is_open();

		if (ret) {
			// This will be the first file to be parsed. Seek to the start of the file and leave it open.
			if (infile.good() && trim(getLine(infile)) != "APPEND") {
				std::string test_line;

				// get the first non-comment, non blank line
				while (infile.good()) {
					test_line = trim(getLine(infile));
					if (test_line.length() == 0) continue;
					else if (test_line.at(0) == '#') continue;
					else break;
				}

				if (test_line != "APPEND") {
					current_index = i-1;
					infile.seekg(std::ios::beg);
					break;
				}
			}

			// don't close the final file if it's the only one with an "APPEND" line
			if (i > 1)
			{
				infile.close();
				infile.clear();
			}
		}
		else {
			if (!errormessage.empty())
				logError("FileParser: %s: %s", errormessage.c_str(), filenames[i-1].c_str());
			infile.clear();
		}
	}

	return ret;
}

void FileParser::close() {
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

			line = trim(getLine(infile));
			line_number++;

			// skip ahead if this line is empty
			if (line.length() == 0) continue;

			starts_with = line.at(0);

			// skip ahead if this line is a comment
			if (starts_with == "#") continue;

			// set new section if this line is a section declaration
			if (starts_with == "[") {
				new_section = true;
				section = parse_section_title(line);

				// keep searching for a key-pair
				continue;
			}

			// skip the string used to combine files
			if (line == "APPEND") continue;

			// this is a keypair. Perform basic parsing and return
			parse_key_pair(line, key, val);
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
			if (!errormessage.empty())
				logError("FileParser: %s: %s", errormessage.c_str(), current_filename.c_str());
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
		line = getLine(infile);
	}
	return line;
}

std::string FileParser::nextValue() {
	if (val == "") {
		return ""; // not found
	}
	std::string s;
	size_t seppos = val.find_first_of(',');
	size_t alt_seppos = val.find_first_of(';');
	if (alt_seppos != std::string::npos && alt_seppos < seppos)
		seppos = alt_seppos; // return the first ',' or ';'

	if (seppos == std::string::npos) {
		s = val;
		val = "";
	}
	else {
		s = val.substr(0, seppos);
		val = val.substr(seppos+1);
	}
	return s;
}

void FileParser::error(const char* format, ...) {
	char buffer[4096];
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	std::stringstream ss;
	ss << "[" << filenames[current_index] << ":" << line_number << "] " << buffer;
	logError(ss.str().c_str());
}

void FileParser::incrementLineNum() {
	line_number++;
}

FileParser::~FileParser() {
	close();
}
