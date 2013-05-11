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
class PowerManager;
class MapRenderer;

class Enemy : public Entity {

public:
	Enemy(PowerManager *_powers, MapRenderer *_map, EnemyManager *_em);
	Enemy(const Enemy& e);
	~Enemy();
	bool lineOfSight();
	void logic();
	int faceNextBest(int mapx, int mapy);
	void newState(int state);
	virtual void doRewards(int source_type);
	void InstantDeath();
	void CheckSummonSustained();

	std::string type;

	virtual Renderable getRender();

	Hazard *haz;
	EnemyBehavior *eb;
	EnemyManager *enemies;

	// other flags
	bool reward_xp;
	bool instant_power;
	bool summoned;
	int summoned_power_index;
	int kill_source_type;

};


#endif

