#include "BehaviorAlly.h"
#include "Enemy.h"

const unsigned short MINIMUM_FOLLOW_DISTANCE_LOWER = 100;
const unsigned short MINIMUM_FOLLOW_DISTANCE = 250;
const unsigned short MAXIMUM_FOLLOW_DISTANCE = 2000;

const unsigned short BLOCK_TICKS = 10;

BehaviorAlly::BehaviorAlly(Enemy *_e, EnemyManager *_em) : BehaviorStandard(_e, _em) {
}

BehaviorAlly::~BehaviorAlly() {
}

void BehaviorAlly::findTarget() {
	// stunned minions can't act
	if (e->stats.effects.stun) return;

	// check distance and line of sight between minion and hero
	if (e->stats.hero_alive)
		hero_dist = calcDist(e->stats.pos, e->stats.hero_pos);
	else
		hero_dist = 0;

	//if the minion gets too far, transport it to the player pos
	if(hero_dist > MAXIMUM_FOLLOW_DISTANCE && !e->stats.in_combat) {
		e->map->collider.unblock(e->stats.pos.x, e->stats.pos.y);
		e->stats.pos.x = e->stats.hero_pos.x;
		e->stats.pos.y = e->stats.hero_pos.y;
		hero_dist = 0;
	}

	bool enemies_in_combat = false;
	//enter combat because enemy is targeting the player or a summon
	for (unsigned int i=0; i < enemies->enemies.size(); i++) {
		if(enemies->enemies[i]->stats.in_combat && !enemies->enemies[i]->stats.hero_ally) {
			Enemy* enemy = enemies->enemies[i];

			//now work out the distance to the enemy and compare it to the distance to the current targer (we want to target the closest enemy)
			if(enemies_in_combat) {
				float enemy_dist = calcDist(e->stats.pos, enemy->stats.pos);
				if (enemy_dist < target_dist) {
					pursue_pos.x = enemy->stats.pos.x;
					pursue_pos.y = enemy->stats.pos.y;
					target_dist = enemy_dist;
				}
			}
			else {
				//minion is not already chasig another enemy so chase this one
				pursue_pos.x = enemy->stats.pos.x;
				pursue_pos.y = enemy->stats.pos.y;
				target_dist = calcDist(e->stats.pos, enemy->stats.pos);
			}

			e->stats.in_combat = true;
			enemies_in_combat = true;
		}
	}


	//break combat if the player gets too far or all enemies die
	if(!enemies_in_combat)
		e->stats.in_combat = false;

	//the default target is the player
	if(!e->stats.in_combat) {
		pursue_pos.x = e->stats.hero_pos.x;
		pursue_pos.y = e->stats.hero_pos.y;
		target_dist = hero_dist;
	}

	// check line-of-sight
	if (target_dist < e->stats.threat_range && e->stats.hero_alive)
		los = e->map->collider.line_of_sight(e->stats.pos.x, e->stats.pos.y, pursue_pos.x, pursue_pos.y);
	else
		los = false;

	//if the player is blocked, all summons which the player is facing to move away for the specified frames
	//need to set the flag player_blocked so that other allies know to get out of the way as well
	//if hero is facing the summon
	if(ENABLE_ALLY_COLLISION_AI) {
		if(!enemies->player_blocked && hero_dist < MINIMUM_FOLLOW_DISTANCE_LOWER
				&& e->map->collider.is_facing(e->stats.hero_pos.x,e->stats.hero_pos.y,e->stats.hero_direction,e->stats.pos.x,e->stats.pos.y)) {
			enemies->player_blocked = true;
			enemies->player_blocked_ticks = BLOCK_TICKS;
		}

		if(enemies->player_blocked && !e->stats.in_combat
				&& e->map->collider.is_facing(e->stats.hero_pos.x,e->stats.hero_pos.y,e->stats.hero_direction,e->stats.pos.x,e->stats.pos.y)) {
			fleeing = true;
			pursue_pos = e->stats.hero_pos;
		}
	}

	if(e->stats.effects.fear) fleeing = true;

}


void BehaviorAlly::checkMoveStateStance() {

	if(e->stats.in_combat && target_dist > e->stats.melee_range)
		e->newState(ENEMY_MOVE);

	if((!e->stats.in_combat && hero_dist > MINIMUM_FOLLOW_DISTANCE) || fleeing) {
		if (e->move()) {
			e->newState(ENEMY_MOVE);
		}
		else {
			int prev_direction = e->stats.direction;

			// hit an obstacle, try the next best angle
			e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
			if (e->move()) {
				e->newState(ENEMY_MOVE);
			}
			else e->stats.direction = prev_direction;
		}
	}
}


void BehaviorAlly::checkMoveStateMove() {
	//if close enough to hero, stop miving
	if(hero_dist < MINIMUM_FOLLOW_DISTANCE && !e->stats.in_combat && !fleeing) {
		e->newState(ENEMY_STANCE);
	}

	// try to continue moving
	else if (!e->move()) {
		int prev_direction = e->stats.direction;
		// hit an obstacle.  Try the next best angle
		e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
		if (!e->move()) {
			//this prevents an ally trying to move perpendicular to a bridge if the player gets close to it in a certain position and gets blocked
			if(enemies->player_blocked && !e->stats.in_combat) {
				e->stats.direction = e->stats.hero_direction;
				if (!e->move()) {
					e->stats.direction = prev_direction;
				}
			}
			else {
				e->stats.direction = prev_direction;
			}
		}
	}
}

