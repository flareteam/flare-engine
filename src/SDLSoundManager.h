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

/**
 * class SDLSoundManager
 */

#ifndef SDL_SOUND_MANAGER_H
#define SDL_SOUND_MANAGER_H

#include <SDL_mixer.h>

#include "SoundManager.h"

class SDLSoundManager : public SoundManager {
public:
	SDLSoundManager();
	~SDLSoundManager();

	SoundManager::SoundID load(const std::string& filename, const std::string& errormessage);
	void unload(SoundManager::SoundID);
	void play(SoundManager::SoundID, std::string channel = GLOBAL_VIRTUAL_CHANNEL, FPoint pos = FPoint(0,0), bool loop = false);
	void pauseAll();
	void resumeAll();
	void setVolumeSFX(int value);

	void loadMusic(const std::string& filename);
	void unloadMusic();
	void playMusic();
	void stopMusic();
	void setVolumeMusic(int value);
	bool isPlayingMusic();

	void logic(FPoint center);
	void reset();

private:
	typedef std::map<std::string, int> VirtualChannelMap;
	typedef VirtualChannelMap::iterator VirtualChannelMapIterator;

	typedef std::map<SoundID, class Sound *> SoundMap;
	typedef SoundMap::iterator SoundMapIterator;

	typedef std::map<int, class Playback> PlaybackMap;
	typedef PlaybackMap::iterator PlaybackMapIterator;

	static void channel_finished(int channel);
	void on_channel_finished(int channel);

	SoundMap sounds;
	VirtualChannelMap channels;
	PlaybackMap playback;
	FPoint lastPos;

	Mix_Music* music;
	std::string music_filename;
};

#endif
