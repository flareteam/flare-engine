/*
Copyright © 2011-2012 Thane Brimhall
Copyright © 2013 Henrik Andersson

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
 * class MessageEngine
 *
 * The MessageEngine class loads all of FLARE's internal messages from a configuration file
 * and returns them as human-readable strings.
 *
 * This class is primarily used for making sure FLARE is flexible and translatable.
 */

#include "CommonIncludes.h"
#include "GetText.h"
#include "SharedResources.h"
#include "Settings.h"

MessageEngine::MessageEngine() {
	GetText infile;

	std::vector<std::string> engineFiles = mods->list("languages/engine." + LANGUAGE + ".po");
	if (engineFiles.size() == 0 && LANGUAGE != "en")
		logError("MessageEngine: Unable to open basic translation files located in languages/engine.%s.po", LANGUAGE.c_str());

	for (unsigned i = 0; i < engineFiles.size(); ++i) {
		if (infile.open(engineFiles[i])) {
			while (infile.next() && !infile.fuzzy)
				messages.insert(std::pair<std::string, std::string>(infile.key, infile.val));
			infile.close();
		}
	}

	std::vector<std::string> dataFiles = mods->list("languages/data." + LANGUAGE + ".po");
	if (dataFiles.size() == 0 && LANGUAGE != "en")
		logError("MessageEngine: Unable to open basic translation files located in languages/data.%s.po", LANGUAGE.c_str());

	for (unsigned i = 0; i < dataFiles.size(); ++i) {
		if (infile.open(dataFiles[i])) {
			while (infile.next() && !infile.fuzzy)
				messages.insert(std::pair<std::string, std::string>(infile.key, infile.val));
			infile.close();
		}
	}
}

/*
 * Each of the get() functions returns the mapped value
 * They differ only on which variables they replace in the string - strings replace %s, integers replace %d
 */
std::string MessageEngine::get(const std::string& key) {
	std::string message = messages[key];
	if (message == "") message = key;
	return unescape(message);
}

std::string MessageEngine::get(const std::string& key, int i) {
	std::string message = messages[key];
	if (message == "") message = key;
	size_t index = message.find("%d");
	if (index != std::string::npos) message = message.replace(index, 2, str(i));
	return unescape(message);
}

std::string MessageEngine::get(const std::string& key, const std::string& s) {
	std::string message = messages[key];
	if (message == "") message = key;
	size_t index = message.find("%s");
	if (index != std::string::npos) message = message.replace(index, 2, s);
	return unescape(message);
}

std::string MessageEngine::get(const std::string& key, int i, const std::string& s) {
	std::string message = messages[key];
	if (message == "") message = key;
	size_t index = message.find("%d");
	if (index != std::string::npos) message = message.replace(index, 2, str(i));
	index = message.find("%s");
	if (index != std::string::npos) message = message.replace(index, 2, s);
	return unescape(message);
}

std::string MessageEngine::get(const std::string& key, int i, int j) {
	std::string message = messages[key];
	if (message == "") message = key;
	size_t index = message.find("%d");
	if (index != std::string::npos) message = message.replace(index, 2, str(i));
	index = message.find("%d");
	if (index != std::string::npos) message = message.replace(index, 2, str(j));
	return unescape(message);
}

// Changes an int into a string
std::string MessageEngine::str(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

// unescape c formatted string
std::string MessageEngine::unescape(std::string val) {

	// unescape percentage %% to %
	size_t pos;
	while ((pos = val.find("%%")) != std::string::npos)
		val = val.replace(pos, 2, "%");

	return val;
}
