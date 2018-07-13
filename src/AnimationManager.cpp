/*
Copyright © 2012 Stefan Beller
Copyright © 2014-2015 Justin Jacobs

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

#include "AnimationManager.h"
#include "AnimationSet.h"
#include "CommonIncludes.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "SharedResources.h"

#include <cassert>

AnimationSet *AnimationManager::getAnimationSet(const std::string& filename) {
	std::vector<std::string>::iterator found = find(names.begin(), names.end(), filename);
	if (found != names.end()) {
		size_t index = static_cast<size_t>(distance(names.begin(), found));
		if (sets[index] == 0) {
			sets[index] = new AnimationSet(filename);
		}
		return sets[index];
	}
	else {
		Utils::logError("AnimationManager::getAnimationSet(): %s not found", filename.c_str());
		Utils::logErrorDialog("AnimationManager::getAnimationSet(): %s not found", filename.c_str());
		mods->resetModConfig();
		Utils::Exit(1);
		return NULL;
	}
}

AnimationManager::AnimationManager() {
}

AnimationManager::~AnimationManager() {
	cleanUp();
// NDEBUG is used by posix to disable assertions, so use the same MACRO.
#ifndef NDEBUG
	if (!names.empty()) {
		Utils::logError("AnimationManager: Still holding these animations:");
		for (size_t i = 0; i < names.size(); i++) {
			Utils::logError("%s %d", names[i].c_str(), counts[i]);
		}
	}
	assert(names.size() == 0);
#endif
}

void AnimationManager::increaseCount(const std::string &name) {
	std::vector<std::string>::iterator found = find(names.begin(), names.end(), name);
	if (found != names.end()) {
		size_t index = static_cast<size_t>(distance(names.begin(), found));
		counts[index]++;
	}
	else {
		sets.push_back(0);
		names.push_back(name);
		counts.push_back(1);
	}
}

void AnimationManager::decreaseCount(const std::string &name) {

	std::vector<std::string>::iterator found = find(names.begin(), names.end(), name);
	if (found != names.end()) {
		size_t index = static_cast<size_t>(distance(names.begin(), found));
		counts[index]--;
	}
	else {
		Utils::logError("AnimationManager::decreaseCount(): %s not found", name.c_str());
		Utils::logErrorDialog("AnimationManager::decreaseCount(): %s not found", name.c_str());
		Utils::Exit(1);
	}
}

void AnimationManager::cleanUp() {
	int i = static_cast<int>(sets.size()) - 1;
	while (i >= 0) {
		if (counts[i] <= 0) {
			delete sets[i];
			counts.erase(counts.begin()+i);
			sets.erase(sets.begin()+i);
			names.erase(names.begin()+i);
		}
		--i;
	}
}
