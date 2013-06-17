/*
Copyright © 2012 Clint Bellanger

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
 * class EnemyBehavior
 *
 * Interface for enemy behaviors.
 * The behavior object is a component of Enemy.
 * Make AI decisions (movement, actions) for enemies.
 */


#pragma once
#ifndef ENEMY_BEHAVIOR_H
#define ENEMY_BEHAVIOR_H

#include "EnemyManager.h"

class Enemy;

class EnemyBehavior {
protected:
	Enemy *e;
public:
	EnemyBehavior(Enemy *_e);
	virtual ~EnemyBehavior();
	virtual void logic();
};

#endif
