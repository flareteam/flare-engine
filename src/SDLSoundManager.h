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

/**
 * class SDLSoundManager
 */

#ifndef SDL_SOUND_MANAGER_H
#define SDL_SOUND_MANAGER_H

#ifdef __EMSCRIPTEN__
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif

#include "SoundManager.h"

class SDLSoundManager : public SoundManager {
public:
	SDLSoundManager();
	~SDLSoundManager();

	SoundID load(const std::string& filename, const std::string& errormessage);
	void unload(SoundID);
	void play(SoundID, const std::string& channel, const FPoint& pos, bool loop, bool cleanup = true);
	void pauseChannel(const std::string& channel);
	void pauseAll();
	void resumeAll();
	void setVolumeSFX(int value);

	void loadMusic(const std::string& filename);
	void unloadMusic();
	void playMusic();
	void stopMusic();
	void setVolumeMusic(int value);
	bool isPlayingMusic();

	void logic(const FPoint& center);
	void reset();

	SoundID getLastPlayedSID();

private:
	typedef std::map<std::string, int> VirtualChannelMap;
	typedef VirtualChannelMap::iterator VirtualChannelMapIterator;

	typedef std::map<SoundID, class Sound *> SoundMap;
	typedef SoundMap::iterator SoundMapIterator;

	typedef std::map<int, class Playback> PlaybackMap;
	typedef PlaybackMap::iterator PlaybackMapIterator;

	static void channel_finished(int channel);
	void on_channel_finished(int channel);

	int SetChannelPosition(int channel, Sint16 angle, Uint8 distance);

	SoundMap sounds;
	VirtualChannelMap channels;
	PlaybackMap playback;
	FPoint lastPos;

	Mix_Music* music;
	std::string music_filename;

	SoundID last_played_sid;
};

#endif
