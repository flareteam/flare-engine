/*
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

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "Utils.h"
#include <stdint.h>

const std::string GLOBAL_VIRTUAL_CHANNEL = "__global__";

/**
 * class SoundManager
 *
 * SoundManager takes care of loading and playing of sound effects,
 * each sound is referenced with a hash SoundID for playing. If a
 * sound is already loaded, the SoundID for currently loaded sound
 * will be returned by SoundManager::load().
**/
class SoundManager {
public:
	typedef unsigned long SoundID;

	virtual ~SoundManager() {};

	virtual SoundManager::SoundID load(const std::string& filename, const std::string& errormessage) = 0;
	virtual void unload(SoundManager::SoundID) = 0;
	virtual void play(SoundManager::SoundID, std::string channel = GLOBAL_VIRTUAL_CHANNEL, FPoint pos = FPoint(0,0), bool loop = false) = 0;

	virtual void logic(FPoint center) = 0;
	virtual void reset() = 0;
};

/**
 * class Playback
 *
 * Playback class is used for creating playback objects,
 * it includes API independent sound id returned by SoundManager::load(), sound location,
 * sound duration properties and virtual channel name, on which sound should be played
**/
class Playback {
public:
	Playback()
		: sid(-1)
		, location(FPoint())
		, loop(false)
		, paused(false)
		, finished(false) {
	}

	SoundManager::SoundID sid;
	std::string virtual_channel;
	FPoint location;
	bool loop;
	bool paused;
	bool finished;
};

#endif
