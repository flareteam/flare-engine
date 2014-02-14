/*
Copyright © 2011-2012 Clint Bellanger and kitano

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


#pragma once
#ifndef ENTITY_H
#define ENTITY_H

#include "CommonIncludes.h"
#include "StatBlock.h"

class Animation;
class AnimationSet;

class Entity {
protected:
	Image *sprites;

public:
	Entity();
	Entity(const Entity&);
	virtual ~Entity();

	bool move();
	bool takeHit(const Hazard &h);
	virtual void resetActiveAnimation();
	virtual void doRewards(int) {}

	// sound effects flags
	bool sfx_phys;
	bool sfx_ment;

	bool sfx_hit;
	bool sfx_die;
	bool sfx_critdie;
	bool sfx_block;

	// Each child of Entity defines its own rendering method
	virtual Renderable getRender() = 0;

	bool setAnimation(const std::string& animation);
	Animation *activeAnimation;
	AnimationSet *animationSet;

	StatBlock stats;
};

extern const int directionDeltaX[];
extern const int directionDeltaY[];
extern const float speedMultiplyer[];

#endif

