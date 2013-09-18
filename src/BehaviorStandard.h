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

#pragma once
#ifndef BEHAVIOR_STANDARD_H
#define BEHAVIOR_STANDARD_H

#include "EnemyBehavior.h"
#include "Utils.h"

using namespace std;

class Enemy;
class Point;

class BehaviorStandard : public EnemyBehavior {
private:

	// logic steps
	void doUpkeep();
	virtual void findTarget();
	void checkPower();
	void checkMove();
	virtual void checkMoveStateStance();
	virtual void checkMoveStateMove();
	void updateState();

protected:
    //variables for patfinding
	vector<FPoint> path;
	FPoint prev_target;
	bool collided;
	bool path_found;
	int chance_calc_path;

	float hero_dist;
	float target_dist;
	FPoint pursue_pos;
	// targeting vars
	bool los;
	//when fleeing, the enemy moves away from the pursue_pos
	bool fleeing;
	bool move_to_safe_dist;

public:
	BehaviorStandard(Enemy *_e);
	void logic();

};

#endif
