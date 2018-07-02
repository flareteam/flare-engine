/*
Copyright © 2013 Henrik Andersson
Copyright © 2015-2016 Justin Jacobs

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

#include "CommonIncludes.h"
#include "Utils.h"

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
	const std::string DEFAULT_CHANNEL = "__global__";
	const FPoint NO_POS = FPoint(0,0);
	static const bool LOOP = true;

	virtual ~SoundManager() {};

	virtual SoundID load(const std::string& filename, const std::string& errormessage) = 0;
	virtual void unload(SoundID) = 0;
	virtual void play(SoundID, const std::string& channel, const FPoint& pos, bool loop) = 0;
	virtual void pauseAll() = 0;
	virtual void resumeAll() = 0;
	virtual void setVolumeSFX(int value) = 0;

	virtual void loadMusic(const std::string& filename) = 0;
	virtual void unloadMusic() = 0;
	virtual void playMusic() = 0;
	virtual void stopMusic() = 0;
	virtual void setVolumeMusic(int value) = 0;
	virtual bool isPlayingMusic() = 0;

	virtual void logic(const FPoint& center) = 0;
	virtual void reset() = 0;

	virtual SoundID getLastPlayedSID() = 0;
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

	SoundID sid;
	std::string virtual_channel;
	FPoint location;
	bool loop;
	bool paused;
	bool finished;
};

#endif
