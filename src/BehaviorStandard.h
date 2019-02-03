/*
Copyright © 2012 Clint Bellanger
Copyright © 2013-2014 Justin Jacobs

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

#ifndef BEHAVIOR_STANDARD_H
#define BEHAVIOR_STANDARD_H

#include "EnemyBehavior.h"
#include "Utils.h"

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
	FPoint getWanderPoint();

protected:
	//variables for patfinding
	std::vector<FPoint> path;
	FPoint prev_target;
	bool collided;
	bool path_found;
	int chance_calc_path;

	float target_dist;
	FPoint pursue_pos;
	// targeting vars
	bool los;
	//when fleeing, the enemy moves away from the pursue_pos
	bool fleeing;
	bool move_to_safe_dist;
	Timer turn_timer;

public:
	explicit BehaviorStandard(Enemy *_e);
	void logic();

};

#endif
