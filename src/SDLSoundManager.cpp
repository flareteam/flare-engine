/*
Copyright © 2013-2014 Henrik Andersson
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
 *
 * SDLSoundManager
 * SDL implementation of SoundManager
 *
**/

#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "ModManager.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SDLSoundManager.h"
#include "UtilsMath.h"

#include <math.h>

class Sound {
public:
	Mix_Chunk *chunk;
	Sound() :  chunk(0), refCnt(0) {}
private:
	friend class SDLSoundManager;
	int refCnt;
};

SDLSoundManager::SDLSoundManager()
	: SoundManager()
	, music(NULL)
	, music_filename("")
	, last_played_sid(-1)
{
	if (settings->audio && Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024)) {
		Utils::logError("SDLSoundManager: Error during Mix_OpenAudio: %s", SDL_GetError());
		settings->audio = false;
	}

	if (settings->audio) {
		Utils::logInfo("SoundManager: Using SDLSoundManager (SDL2, %s)", SDL_GetCurrentAudioDriver());
	}

	Mix_AllocateChannels(128);
	setVolumeSFX(settings->sound_volume);
}

SDLSoundManager::~SDLSoundManager() {
	unloadMusic();

	SDLSoundManager::SoundMapIterator it;
	while((it = sounds.begin()) != sounds.end())
		unload(it->first);

	Mix_CloseAudio();
}

void SDLSoundManager::logic(const FPoint& center) {

	PlaybackMapIterator it = playback.begin();
	if (it == playback.end())
		return;

	lastPos = center;

	std::vector<int> cleanup;

	while(it != playback.end()) {

		/* if sound is finished and should be unloaded add it to cleanup and continue with next */
		if (it->second.finished && it->second.cleanup) {
			cleanup.push_back(it->first);
			++it;
			continue;
		}

		/* dont process playback sounds without location */
		if (it->second.location.x == 0 && it->second.location.y == 0) {
			++it;
			continue;
		}

		/* control mixing playback depending on distance */
		float v = Utils::calcDist(center, it->second.location) / static_cast<float>(eset->misc.sound_falloff);
		if (it->second.loop) {
			if (v < 1.0 && it->second.paused) {
				Mix_Resume(it->first);
				it->second.paused = false;
			}
			else if (v > 1.0 && !it->second.paused) {
				Mix_Pause(it->first);
				it->second.paused = true;
				++it;
				continue;
			}
		}

		/* update sound mix with new distance/location to hero */
		v = std::min<float>(std::max<float>(v, 0.0f), 1.0f);
		Uint8 dist = Uint8(255.0 * v);

		SetChannelPosition(it->first, 0, dist);
		++it;
	}

	/* clenaup finished soundplayback */
	while (!cleanup.empty()) {

		it = playback.find(cleanup.back());

		unload(it->second.sid);

		/* find and erase virtual channel for playback if exists */
		VirtualChannelMapIterator vcit = channels.find(it->second.virtual_channel);
		if (vcit != channels.end())
			channels.erase(vcit);

		playback.erase(it);

		cleanup.pop_back();
	}
}

void SDLSoundManager::reset() {

	PlaybackMapIterator it = playback.begin();
	if (it == playback.end())
		return;

	while(it != playback.end()) {

		if (it->second.loop)
			Mix_HaltChannel(it->first);

		++it;
	}
	logic(FPoint(0,0));
}

SoundID SDLSoundManager::load(const std::string& filename, const std::string& errormessage) {

	Sound lsnd;
	SoundID sid = 0;
	SoundMapIterator it;

	if (!settings->audio)
		return 0;

	const std::string realfilename = mods->locate(filename);
	sid = Utils::hashString(realfilename);
	it = sounds.find(sid);
	if (it != sounds.end()) {
		it->second->refCnt++;
		return sid;
	}

	/* load non existing sound */
	lsnd.chunk = Mix_LoadWAV(realfilename.c_str());
	lsnd.refCnt = 1;
	if (!lsnd.chunk) {
		Utils::logError("SoundManager: %s: Loading sound %s (%s) failed: %s", errormessage.c_str(),
				realfilename.c_str(), filename.c_str(), Mix_GetError());
		return 0;
	}

	/* instantiate and add sound to manager */
	Sound *psnd = new Sound;
	*psnd = lsnd;
	sounds.insert(std::pair<SoundID,Sound *>(sid, psnd));

	return sid;
}

void SDLSoundManager::unload(SoundID sid) {

	SoundMapIterator it;
	it = sounds.find(sid);
	if (it == sounds.end())
		return;

	if (--it->second->refCnt == 0) {
		Mix_FreeChunk(it->second->chunk);
		delete it->second;
		sounds.erase(it);
	}
}



void SDLSoundManager::play(SoundID sid, const std::string& channel, const FPoint& pos, bool loop, bool cleanup) {

	SoundMapIterator it;
	VirtualChannelMapIterator vcit = channels.end();

	// since last_played_sid is primarily used for subtitles, it doesn't make sense to count looped sounds
	if (!loop && sid)
		last_played_sid = sid;

	if (!sid || !settings->audio)
		return;

	it = sounds.find(sid);
	if (it == sounds.end())
		return;

	/* create playback object and start playback of sound chunk */
	Playback p;
	p.sid = sid;
	p.location = pos;
	p.virtual_channel = channel;
	p.loop = loop;
	p.finished = false;
	p.cleanup = cleanup;

	if (p.virtual_channel != DEFAULT_CHANNEL) {

		/* if playback exists, stop it before playin next sound */
		vcit = channels.find(p.virtual_channel);
		if (vcit != channels.end()) {
			// temporarily disable the channel finish callback to avoid setting the 'finished' flag when stopping the channel
			if (!cleanup)
				Mix_ChannelFinished(NULL);

			Mix_HaltChannel(vcit->second);
		}

		vcit = channels.insert(std::pair<std::string, int>(p.virtual_channel, -1)).first;
	}

	// Let playback own a reference to prevent unloading playbacked sound.
	if (!loop)
		it->second->refCnt++;

	Mix_ChannelFinished(&channel_finished);
	int c = Mix_PlayChannel(-1, it->second->chunk, (loop ? -1 : 0));

	if (c == -1)
		Utils::logError("SoundManager: Failed to play sound, no more channels available.");

	// precalculate mixing volume if sound has a location
	Uint8 d = 0;
	if (p.location.x != 0 || p.location.y != 0) {
		float v = 255.0f * (Utils::calcDist(lastPos, p.location) / static_cast<float>(eset->misc.sound_falloff));
		v = std::min<float>(std::max<float>(v, 0.0f), 255.0f);
		d = Uint8(v);
	}

	SetChannelPosition(c, 0, d);

	if (vcit != channels.end())
		vcit->second = c;

	playback.insert(std::pair<int, Playback>(c, p));
}

void SDLSoundManager::pauseChannel(const std::string& channel) {
	VirtualChannelMapIterator vcit = channels.end();

	vcit = channels.find(channel);
	if (vcit != channels.end()) {
		Mix_Pause(vcit->second);
	}
}

void SDLSoundManager::pauseAll() {
	Mix_Pause(-1);
	Mix_PauseMusic();
}

void SDLSoundManager::resumeAll() {
	Mix_Resume(-1);
	Mix_ResumeMusic();
}

void SDLSoundManager::on_channel_finished(int channel) {
	PlaybackMapIterator pit = playback.find(channel);
	if (pit == playback.end())
		return;

	pit->second.finished = true;

	SetChannelPosition(channel, 0, 0);
}

void SDLSoundManager::channel_finished(int channel) {
	static_cast<SDLSoundManager*>(snd)->on_channel_finished(channel);
}

void SDLSoundManager::setVolumeSFX(int value) {
	Mix_Volume(-1, value);
}

void SDLSoundManager::loadMusic(const std::string& filename) {
	if (!settings->audio)
		return;

	if (filename == music_filename) {
		if (!isPlayingMusic())
			playMusic();
		return;
	}

	unloadMusic();

	if (filename == "")
		return;

	music = Mix_LoadMUS(mods->locate(filename).c_str());
	if (music) {
		music_filename = filename;
		playMusic();
	}
	else {
		Utils::logError("SoundManager: Couldn't load music file '%s': %s", filename.c_str(), Mix_GetError());
	}
}

void SDLSoundManager::unloadMusic() {
	stopMusic();
	if (music) Mix_FreeMusic(music);
	music = NULL;
	music_filename = "";
}

void SDLSoundManager::playMusic() {
	if (!settings->audio || !music) return;

	Mix_VolumeMusic(settings->music_volume);
	Mix_PlayMusic(music, -1);
}

void SDLSoundManager::stopMusic() {
	if (!settings->audio || !music) return;

	Mix_HaltMusic();
}

void SDLSoundManager::setVolumeMusic(int value) {
	if (!settings->audio || !music) return;

	Mix_VolumeMusic(value);
}

bool SDLSoundManager::isPlayingMusic() {
	return (settings->audio && music && settings->music_volume > 0 && Mix_PlayingMusic());
}

int SDLSoundManager::SetChannelPosition(int channel, Sint16 angle, Uint8 distance) {
#ifdef __EMSCRIPTEN__
	// TODO fix for Emscripten
	return 0;
#else
	return Mix_SetPosition(channel, angle, distance);
#endif
}

SoundID SDLSoundManager::getLastPlayedSID() {
	SoundID ret = last_played_sid;
	last_played_sid = -1;
	return ret;
}

