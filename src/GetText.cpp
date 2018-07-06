/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
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

#include "GetText.h"
#include "UtilsParsing.h"

GetText::GetText()
	: line("")
	, key("")
	, val("")
	, fuzzy(false) {
}

bool GetText::open(const std::string& filename) {
	infile.open(filename.c_str(), std::ios::in);
	return infile.is_open();
}

void GetText::close() {
	if (infile.is_open())
		infile.close();
	infile.clear();
}

// Turns all \" into just "
// Turns all \n into a newline char
std::string GetText::sanitize(const std::string& message) {
	std::string new_message = message;
	size_t pos = 0;
	while ((pos = new_message.find("\\\"")) != std::string::npos) {
		new_message = new_message.substr(0, pos) + new_message.substr(pos+1);
	}
	while ((pos = new_message.find("\\n")) != std::string::npos) {
		new_message = new_message.substr(0, pos) + '\n' + new_message.substr(pos+2);
	}
	return new_message;
}

/**
 * Advance to the next key pair
 *
 * @return false if EOF, otherwise true
 */
bool GetText::next() {

	key = "";
	val = "";

	fuzzy = false;

	while (infile.good()) {
		line = Parse::getLine(infile);

		// check if comment and if fuzzy
		if (line.compare(0, 2, "#,") == 0 && line.find("fuzzy") != std::string::npos)
			fuzzy = true;

		// this is a key
		if (line.compare(0, 5, "msgid") == 0) {
			// grab only what's contained in the quotes
			key = line.substr(6);
			key = key.substr(1, key.length()-2); //strips off "s
			key = sanitize(key);

			if (key != "")
				continue;
			else {
				// It is a multi-line value, unless it is the first msgid, in which case it will be empty
				// and it will be ignored when finding the matching msgstr, so no big deal.
				line = Parse::getLine(infile);
				while(!line.empty() && line[0] == '\"') {
					// We remove the double quotes.
					key += line.substr(1, line.length()-2);
					key = sanitize(key);
					line = Parse::getLine(infile);
				}
			}
		}

		// this is a value
		if (line.compare(0, 6, "msgstr") == 0) {
			// grab only what's contained in the quotes
			val = line.substr(7);
			val = val.substr(1, val.length()-2); //strips off "s
			val = sanitize(val);

			// handle keypairs
			if (key != "") {
				if(val != "") { // One-line value found.
					return true;
				}
				else { // Might be a multi-line value.
					line = Parse::getLine(infile);
					while(!line.empty() && line[0] == '\"') {
						// We remove the double quotes.
						val += line.substr(1, line.length()-2);
						val = sanitize(val);
						line = Parse::getLine(infile);
					}
					if(val != "") { // It was a multi-line value indeed.
						return true;
					}
				}
			}
			else {
				// key is empty; likely the po header
				// reset the fuzzy state for the next msgid
				fuzzy = false;
			}
		}
	}

	// hit the end of file
	return false;
}

GetText::~GetText() {
	close();
}
