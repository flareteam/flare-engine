/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller

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
 * class Enemy
 */


#pragma once
#ifndef ENEMY_H
#define ENEMY_H

#include "CommonIncludes.h"
#include "Entity.h"
#include "Utils.h"

#include <queue>

class EnemyBehavior;
class Hazard;

class Enemy : public Entity {

public:
	Enemy();
	Enemy(const Enemy& e);
	~Enemy();
	bool lineOfSight();
	void logic();
	int faceNextBest(float mapx, float mapy);
	void newState(int state);
	virtual void doRewards(int source_type);
	void InstantDeath();

	std::string type;

	virtual Renderable getRender();

	Hazard *haz;
	EnemyBehavior *eb;

	// other flags
	bool reward_xp;
	bool instant_power;
	int kill_source_type;

};


#endif

