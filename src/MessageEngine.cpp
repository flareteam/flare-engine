/*
Copyright © 2011-2012 Thane Brimhall
Copyright © 2013 Henrik Andersson
Copyright © 2013-2016 Justin Jacobs

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
#include "MessageEngine.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "Settings.h"

MessageEngine::MessageEngine() {
	Utils::logInfo("MessageEngine: Using language '%s'", settings->language.c_str());

	GetText infile;

	std::vector<std::string> engineFiles = mods->list("languages/engine." + settings->language + ".po", ModManager::LIST_FULL_PATHS);
	if (engineFiles.empty() && settings->language != "en")
		Utils::logError("MessageEngine: Unable to open basic translation files located in languages/engine.%s.po", settings->language.c_str());

	for (unsigned i = 0; i < engineFiles.size(); ++i) {
		if (infile.open(engineFiles[i])) {
			while (infile.next()) {
				if (!infile.fuzzy)
					messages.insert(std::pair<std::string, std::string>(infile.key, infile.val));
			}
			infile.close();
		}
	}

	std::vector<std::string> dataFiles = mods->list("languages/data." + settings->language + ".po", ModManager::LIST_FULL_PATHS);
	if (dataFiles.empty() && settings->language != "en")
		Utils::logError("MessageEngine: Unable to open basic translation files located in languages/data.%s.po", settings->language.c_str());

	for (unsigned i = 0; i < dataFiles.size(); ++i) {
		if (infile.open(dataFiles[i])) {
			while (infile.next()) {
				if (!infile.fuzzy)
					messages.insert(std::pair<std::string, std::string>(infile.key, infile.val));
			}
			infile.close();
		}
	}
}

/*
 * MessageEngine::get() uses a limited C format syntax
 * - %d is for all integer values
 * - %s is for C strings (no std::string!)
 * - %% is a literal percent sign
 */
std::string MessageEngine::get(const std::string key, ...) {
	std::string message = messages[key];
	if (message == "") message = key;

	va_list args;
	va_start(args, key);

	size_t index = message.find('%');
	while (index < message.size()) {
		if (index + 1 == message.size())
			break;

		if (message[index + 1] == 'd') {
			// all integer values
			int64_t val = va_arg(args, int64_t);
			std::stringstream ss;
			ss << val;
			message = message.replace(index, 2, ss.str());
		}
		else if (message[index + 1] == 's') {
			// C strings
			const char* val = va_arg(args, const char*);
			message = message.replace(index, 2, std::string(val));
		}
		else if (message[index + 1] == '%') {
			// unescape literal percent signs
			message = message.replace(index, 2, "%");
		}

		index = message.find('%', index + 1);
	}

	va_end(args);

	return message;
}

