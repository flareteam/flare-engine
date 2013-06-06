/*
Copyright 2012 Clint Bellanger

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
 * class EntityBehavior
 *
 * Interface for entity behaviors.
 * The behavior object is a component of Entities.
 * Make AI decisions (movement, actions) for entities.
 */

#pragma once
#ifndef ENTITY_BEHAVIOR_H
#define ENTITY_BEHAVIOR_H

#include "EnemyManager.h"

class Enemy;

class EntityBehavior {
protected:
	Enemy *e;
	EnemyManager *enemies;
public:
	EntityBehavior(Enemy *_e, EnemyManager *_em);
	virtual ~EntityBehavior();
	virtual void logic();
};

#endif
