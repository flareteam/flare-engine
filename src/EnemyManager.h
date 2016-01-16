/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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
 * class EnemyManager
 */


#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include "Settings.h"
#include "Enemy.h"
#include "Utils.h"
#include "CampaignManager.h"

class EnemyManager {
private:

	void loadAnimations(Enemy *e);

	std::vector<std::string> anim_prefixes;
	std::vector<std::vector<Animation*> > anim_entities;

	/**
	 * callee is responsible for deleting returned enemy object
	 */
	Enemy *getEnemyPrototype(const std::string& type_id);
	size_t loadEnemyPrototype(const std::string& type_id);

	std::vector<Enemy> prototypes;

public:
	EnemyManager();
	~EnemyManager();
	void handleNewMap();
	void handleSpawn();
	void handlePartyBuff();
	bool checkPartyMembers();
	void logic();
	void addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);
	void checkEnemiesforXP();
	bool isCleared();
	Enemy *enemyFocus(const Point& mouse, const FPoint& cam, bool alive_only);
	Enemy *getNearestEnemy(const FPoint& pos);

	// vars
	std::vector<Enemy*> enemies;
	int hero_stealth;

	bool player_blocked;
	int player_blocked_ticks;
};


#endif
