/*
Copyright © 2012 Stefan Beller

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

#include "CommonIncludes.h"
#include "SharedResources.h"

#include <cassert>

using namespace std;

AnimationSet *AnimationManager::getAnimationSet(const string& filename) {
	vector<string>::iterator found = find(names.begin(), names.end(), filename);
	if (found != names.end()) {
		int index = distance(names.begin(), found);
		if (sets[index] == 0) {
			sets[index] = new AnimationSet(filename);
		}
		return sets[index];
	}
	else {
		fprintf(stderr, "AnimationManager::getAnimationSet: %s not found\n", filename.c_str());
		SDL_Quit();
		exit(1);
		// return 0;
	}
}

AnimationManager::AnimationManager() {
}

AnimationManager::~AnimationManager() {
	cleanUp();
// NDEBUG is used by posix to disable assertions, so use the same MACRO.
#ifndef NDEBUG
	if (!names.empty()) {
		fprintf(stderr, "AnimationManager still holding these animations:\n");
		for (unsigned i = 0; i < names.size(); i++)
			fprintf(stderr, "%s %d\n", names[i].c_str(), counts[i]);
	}
	assert(names.size() == 0);
#endif
}

void AnimationManager::increaseCount(const std::string &name) {
	vector<string>::iterator found = find(names.begin(), names.end(), name);
	if (found != names.end()) {
		int index = distance(names.begin(), found);
		counts[index]++;
	}
	else {
		sets.push_back(0);
		names.push_back(name);
		counts.push_back(1);
	}
}

void AnimationManager::decreaseCount(const std::string &name) {

	vector<string>::iterator found = find(names.begin(), names.end(), name);
	if (found != names.end()) {
		int index = distance(names.begin(), found);
		counts[index]--;
	}
	else {
		fprintf(stderr, "AnimationManager::decreaseCount: %s not found\n", name.c_str());
		SDL_Quit();
		exit(1);
	}
}

void AnimationManager::cleanUp() {
	int i = sets.size() - 1;
	while (i >= 0) {
		if (counts[i] <= 0) {
			delete sets[i];
			counts.erase(counts.begin()+i);
			sets.erase(sets.begin()+i);
			names.erase(names.begin()+i);
		}
		--i;
	}
	imag->cleanUp();
}
