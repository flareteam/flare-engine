/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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

/*
 * class EntityManager
 */


#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include "CommonIncludes.h"
#include "Utils.h"

class Animation;
class Entity;

class EntityManager {
protected:
	/**
	 * callee is responsible for deleting returned entity object
	 */
	size_t loadEntityPrototype(const std::string& type_id);

	std::vector<Entity> prototypes;

public:
	EntityManager();
	~EntityManager();

	Entity *getEntityPrototype(const std::string& type_id);

	void handleNewMap();
	void handleSpawn();
	bool checkPartyMembers();
	void logic();
	void addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);
	void checkEnemiesforXP();
	bool isCleared();
	void spawn(const std::string& entity_type, const Point& target);
	Entity *entityFocus(const Point& mouse, const FPoint& cam, bool alive_only);
	Entity* getNearestEntity(const FPoint& pos, bool get_corpse, float *saved_distance, float max_range);

	// vars
	std::vector<Entity*> entities;
	int hero_stealth;

	bool player_blocked;
	Timer player_blocked_timer;

	static const bool GET_CORPSE = true;
	static const bool IS_ALIVE = true;
};


#endif
