/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2012-2015 Justin Jacobs

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


#ifndef ENEMY_H
#define ENEMY_H

#include "CommonIncludes.h"
#include "Entity.h"
#include "Utils.h"

class EnemyBehavior;

class Enemy : public Entity {

public:
	Enemy();
	Enemy(const Enemy& e);
	Enemy& operator=(const Enemy& e);
	~Enemy();
	void logic();
	unsigned char faceNextBest(float mapx, float mapy);
	virtual void doRewards(int source_type);

	std::string type;

	Renderable getRender();

	EnemyBehavior *eb;

	// other flags
	bool reward_xp;
	bool instant_power;
	int kill_source_type;

};


#endif

