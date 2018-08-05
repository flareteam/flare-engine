/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2016 Justin Jacobs

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
class MapCollision;
class Power;
class StatBlock;

class Hazard {
public:
	explicit Hazard(MapCollision *_collider);
	Hazard(const Hazard& other);
	Hazard & operator= (const Hazard& other);
	~Hazard();

	void logic();
	bool hasEntity(Entity*);
	void addEntity(Entity*);
	void loadAnimation(const std::string &s);
	void setAngle(const float& _angle);
	bool isDangerousNow();
	void addRenderable(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);
	void setPower(size_t power_index);

	bool active;
	bool remove_now;
	bool hit_wall;
	bool relative_pos;
	bool sfx_hit_played;

	int dmg_min;
	int dmg_max;
	int crit_chance;
	int accuracy;
	int source_type;
	float base_speed;
	int lifespan; // ticks down to zero
	int animationKind;	// either a direction or option/random
	int delay_frames;
	float angle; // in radians

	StatBlock *src_stats;
	Power *power;
	size_t power_index;

	FPoint pos;
	FPoint speed;
	FPoint pos_offset;

	// for linking hazards together, e.g. repeaters
	Hazard* parent;
	std::vector<Hazard*> children;

private:
    void reflect();

	const MapCollision *collider;
	Animation *activeAnimation;
	std::string animation_name;

	// Keeps track of entities already hit
	std::vector<Entity*> entitiesCollided;
};

#endif
