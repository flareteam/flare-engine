/*
Copyright © 2011-2012 Clint Bellanger

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
 * class Hazard
 *
 * Stand-alone object that can harm the hero or creatures
 * These are generated whenever something makes any attack
 */

#ifndef HAZARD_H
#define HAZARD_H

class Entity;

#include "CommonIncludes.h"
#include "Utils.h"

class Animation;
class StatBlock;
class MapCollision;

// the spell/power's source type: eg. which team did it come from?
const int SOURCE_TYPE_HERO = 0;
const int SOURCE_TYPE_NEUTRAL = 1;
const int SOURCE_TYPE_ENEMY = 2;
const int SOURCE_TYPE_ALLY = 3;

class Hazard {
private:
	const MapCollision *collider;
	// Keeps track of entities already hit
	std::vector<Entity*> entitiesCollided;
	Animation *activeAnimation;
	std::string animation_name;

public:
	Hazard(MapCollision *_collider);

	Hazard(const Hazard&); // not implemented! If you implement this, take care to create a real copy of the pointers, such as Animation.
	Hazard & operator= (const Hazard & other); // same as copy constructor!

	~Hazard();

	StatBlock *src_stats;

	void logic();

	bool hasEntity(Entity*);
	void addEntity(Entity*);

	void loadAnimation(const std::string &s);

	void setAngle(const float& _angle);

	int dmg_min;
	int dmg_max;
	int crit_chance;
	int accuracy;
	int source_type;
	bool target_party;

	FPoint pos;
	FPoint speed;
	FPoint pos_offset;
	bool relative_pos;
	float base_speed;
	float angle; // in radians
	int base_lifespan;
	int lifespan; // ticks down to zero
	float radius;
	int power_index;

	int animationKind;	// direction or other, it is a specific value according to
	// some hazard animations are 8-directional
	// some hazard animations have random/varietal options

	bool isDangerousNow();
	void addRenderable(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	bool on_floor; // rendererable goes on the floor layer
	int delay_frames;
	bool complete_animation; // if not multitarget but hitting a creature, still complete the animation?

	// these work in conjunction
	// if the attack is not multitarget, set active=false
	// only process active hazards for collision
	bool multitarget;
	bool active;

	bool multihit;
	bool expire_with_caster;

	bool remove_now;
	bool hit_wall;

	// after effects of various powers
	int hp_steal;
	int mp_steal;

	bool trait_armor_penetration;
	int trait_crits_impaired;
	int trait_elemental;
	bool beacon;
	bool missile;
	bool directional;

	// pre/post power effects
	int post_power;
	int wall_power;

	// targeting by movement type
	bool target_movement_normal;
	bool target_movement_flying;
	bool target_movement_intangible;

	bool walls_block_aoe;

	// soundfx
	unsigned long sfx_hit;
	bool sfx_hit_enable;
	bool sfx_hit_played;

	// loot
	std::vector<Event_Component> loot;

	// for linking hazards together, e.g. repeaters
	Hazard* parent;
	std::vector<Hazard*> children;
};

#endif
