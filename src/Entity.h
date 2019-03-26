/*
Copyright © 2011-2012 Clint Bellanger and kitano
Copyright © 2012-2015 Justin Jacobs

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
 * class Entity
 *
 * An Entity represents any character in the game - the player, allies, enemies
 * This base class handles logic common to all of these child classes
 */


#ifndef ENTITY_H
#define ENTITY_H

#include "CommonIncludes.h"
#include "StatBlock.h"
#include "Utils.h"

class Animation;
class AnimationSet;

class Entity {
protected:
	Image *sprites;

	void move_from_offending_tile();
	virtual void resetActiveAnimation();

public:
	enum {
		SOUND_HIT = 0,
		SOUND_DIE = 1,
		SOUND_CRITDIE = 2,
		SOUND_BLOCK = 3
	};

	Entity();
	Entity(const Entity& e);
	Entity& operator=(const Entity& e);
	virtual ~Entity();

	void loadSounds();
	void loadSoundsFromStatBlock(StatBlock *src_stats);
	void unloadSounds();
	void playAttackSound(const std::string& attack_name);
	void playSound(int sound_type);
	bool move();
	bool takeHit(Hazard &h);
	virtual void doRewards(int) {}

	// sound effects
	std::vector<std::pair<std::string, std::vector<SoundID> > > sound_attack;
	std::vector<SoundID> sound_hit;
	std::vector<SoundID> sound_die;
	std::vector<SoundID> sound_critdie;
	std::vector<SoundID> sound_block;
	SoundID sound_levelup;
	SoundID sound_lowhp;

	bool setAnimation(const std::string& animation);
	Animation *activeAnimation;
	AnimationSet *animationSet;

	StatBlock stats;
};

extern const int directionDeltaX[];
extern const int directionDeltaY[];
extern const float speedMultiplyer[];

#endif

