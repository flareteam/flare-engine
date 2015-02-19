/*
Copyright © 2013-2014 Henrik Andersson

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
SoundManager

SoundManager take care of loading and playing of sound effects,
each sound is references with a hash SoundID for playing. If a
sound is already loaded the SoundID for currently loaded sound
will be returned by SoundManager::load().

**/

#include "CommonIncludes.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "UtilsMath.h"

#include <locale>
#include <math.h>

class Sound {
public:
	Mix_Chunk *chunk;
	Sound() :  chunk(0), refCnt(0) {}
private:
	friend class SoundManager;
	int refCnt;
};

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

SoundManager::SoundManager() {
	Mix_AllocateChannels(128);
}

SoundManager::~SoundManager() {
	SoundManager::SoundMapIterator it;
	while((it = sounds.begin()) != sounds.end())
		unload(it->first);
}

void SoundManager::logic(FPoint c) {

	PlaybackMapIterator it = playback.begin();
	if (it == playback.end())
		return;

	lastPos = c;

	std::vector<int> cleanup;

	while(it != playback.end()) {

		/* if sound is finished add it to cleanup and continue with next */
		if (it->second.finished) {
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
		float v = calcDist(c, it->second.location) / (SOUND_FALLOFF);
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
		clamp(v, 0.0, 1.0);
		Uint8 dist = Uint8(255.0 * v);

		Mix_SetPosition(it->first, 0, dist);
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

void SoundManager::reset() {

	PlaybackMapIterator it = playback.begin();
	if (it == playback.end())
		return;

	while(it != playback.end()) {

		if (it->second.loop)
			Mix_HaltChannel(it->first);

		++it;
	}
	logic(Point(0,0));
}

SoundManager::SoundID SoundManager::load(const std::string& filename, const std::string& errormessage) {

	Sound lsnd;
	SoundID sid = 0;
	SoundMapIterator it;
	std::locale loc;

	if (!AUDIO || !SOUND_VOLUME)
		return 0;

	const std::collate<char>& coll = std::use_facet<std::collate<char> >(loc);
	const std::string realfilename = mods->locate(filename);

	/* create sid hash and check if already loaded */
	sid = coll.hash(realfilename.data(), realfilename.data()+realfilename.length());
	it = sounds.find(sid);
	if (it != sounds.end()) {
		it->second->refCnt++;
		return sid;
	}

	/* load non existing sound */
	lsnd.chunk = Mix_LoadWAV(realfilename.c_str());
	lsnd.refCnt = 1;
	if (!lsnd.chunk) {
		logError("SoundManager: %s: Loading sound %s (%s) failed: %s", errormessage.c_str(),
				realfilename.c_str(), filename.c_str(), Mix_GetError());
		return 0;
	}

	/* instantiate and add sound to manager */
	Sound *psnd = new Sound;
	*psnd = lsnd;
	sounds.insert(std::pair<SoundID,Sound *>(sid, psnd));

	return sid;
}

void SoundManager::unload(SoundManager::SoundID sid) {

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



void SoundManager::play(SoundManager::SoundID sid, std::string channel, FPoint pos, bool loop) {

	SoundMapIterator it;
	VirtualChannelMapIterator vcit = channels.end();

	if (!sid || !AUDIO || !SOUND_VOLUME)
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

	if (p.virtual_channel != GLOBAL_VIRTUAL_CHANNEL) {

		/* if playback exists, stop it befor playin next sound */
		vcit = channels.find(p.virtual_channel);
		if (vcit != channels.end())
			Mix_HaltChannel(vcit->second);

		vcit = channels.insert(std::pair<std::string, int>(p.virtual_channel, -1)).first;
	}

	// Let playback own a reference to prevent unloading playbacked sound.
	if (!loop)
		it->second->refCnt++;

	Mix_ChannelFinished(&channel_finished);
	int c = Mix_PlayChannel(-1, it->second->chunk, (loop ? -1 : 0));

	if (c == -1)
		logError("SoundManager: Failed to play sound, no more channels available.");

	// precalculate mixing volume if sound has a location
	Uint8 d = 0;
	if (p.location.x != 0 || p.location.y != 0) {
		float v = 255.0f * (calcDist(lastPos, p.location) / (SOUND_FALLOFF));
		clamp(v, 0.f, 255.f);
		d = Uint8(v);
	}

	Mix_SetPosition(c, 0, d);

	if (vcit != channels.end())
		vcit->second = c;

	playback.insert(std::pair<int, Playback>(c, p));
}

void SoundManager::on_channel_finished(int channel) {
	PlaybackMapIterator pit = playback.find(channel);
	if (pit == playback.end())
		return;

	pit->second.finished = true;

	Mix_SetPosition(channel, 0, 0);
}

void SoundManager::channel_finished(int channel) {
	snd->on_channel_finished(channel);
}
