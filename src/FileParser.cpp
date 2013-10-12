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

using namespace std;
bool new_section;
std::string section;
std::string key;
std::string val;

FileParser::FileParser()
	: current_index(0)
	, line("")
	, new_section(false)
	, section("")
	, key("")
	, val("") {
}

bool FileParser::open(const string& _filename, bool locateFileName, bool stopAfterFirstFile, const string &_errormessage) {
	filenames.clear();
	if (locateFileName) {
		if (stopAfterFirstFile) {
			filenames.push_back(mods->locate(_filename));
		}
		else {
			filenames = mods->list(_filename);
		}
	}
	else {
		if (!stopAfterFirstFile && isDirectory(_filename)) {
			getFileList(_filename, "txt", filenames);
		}
		else {
			filenames.push_back(_filename);
		}
	}
	current_index = 0;
	this->errormessage = _errormessage;

	// This should never happen, because when stopAfterFirstFile is set, we
	// expect to have only one file added above.
	if (stopAfterFirstFile)
		if (filenames.size() > 1)
			fprintf(stderr, "Error in FileParser::open logic. More files detected, although stopAfterFirstFile was set\n");

	if (filenames.size() == 0 && !errormessage.empty()) {
		fprintf(stderr, "%s: %s: No such file or directory!\n", _filename.c_str(), errormessage.c_str());
		return false;
	}

	const string current_filename = filenames[0];
	infile.open(current_filename.c_str(), ios::in);
	bool ret = infile.is_open();
	if (!ret && !errormessage.empty())
		fprintf(stderr, "%s: %s\n", errormessage.c_str(), current_filename.c_str());
	return ret;
}

void FileParser::close() {
	if (infile.is_open())
		infile.close();
}

/**
 * Advance to the next key pair
 * Take note if a new section header is encountered
 *
 * @return false if EOF, otherwise true
 */
bool FileParser::next() {

	string starts_with;
	new_section = false;

	while (current_index < filenames.size()) {
		while (infile.good()) {

			line = trim(getLine(infile));

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

			// this is a keypair. Perform basic parsing and return
			parse_key_pair(line, key, val);
			return true;
		}

		infile.close();

		current_index++;
		if (current_index == filenames.size()) return false;

		const string current_filename = filenames[current_index];
		infile.clear();
		infile.open(current_filename.c_str(), ios::in);
		if (!infile.is_open()) {
			if (!errormessage.empty())
				fprintf(stderr, "%s: %s\n", errormessage.c_str(), current_filename.c_str());
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
string FileParser::getRawLine() {
	line = "";

	if (infile.good()) {
		line = getLine(infile);
	}
	return line;
}

string FileParser::nextValue() {
	if (val == "") {
		return ""; // not found
	}
	string s;
	size_t seppos = val.find_first_of(',');
	size_t alt_seppos = val.find_first_of(';');
	if (alt_seppos != string::npos && alt_seppos < seppos)
		seppos = alt_seppos; // return the first ',' or ';'

	if (seppos == string::npos) {
		s = val;
		val = "";
	}
	else {
		s = val.substr(0, seppos);
		val = val.substr(seppos+1);
	}
	return s;
}

std::string FileParser::getFileName() {
	return filenames[current_index];
}

FileParser::~FileParser() {
	close();
}
