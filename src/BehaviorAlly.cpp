/*
Copyright © 2013 Ryan Dansie
Copyright © 2014-2015 Justin Jacobs

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

#include "BehaviorAlly.h"
#include "Enemy.h"
#include "SharedGameResources.h"
#include "UtilsMath.h"

const float ALLY_FLEE_DISTANCE = 2;
const float ALLY_FOLLOW_DISTANCE_WALK = 5.5;
const float ALLY_FOLLOW_DISTANCE_STOP = 5;
const float ALLY_TELEPORT_DISTANCE = 40;

const unsigned short BLOCK_TICKS = 10;

BehaviorAlly::BehaviorAlly(Enemy *_e) : BehaviorStandard(_e) {
}

BehaviorAlly::~BehaviorAlly() {
}

void BehaviorAlly::findTarget() {
	// stunned minions can't act
	if (e->stats.effects.stun) return;

	// check distance and line of sight between minion and hero
	if (pc->stats.alive)
		hero_dist = calcDist(e->stats.pos, pc->stats.pos);
	else
		hero_dist = 0;

	//if the minion gets too far, transport it to the player pos
	if(hero_dist > ALLY_TELEPORT_DISTANCE && !e->stats.in_combat) {
		mapr->collider.unblock(e->stats.pos.x, e->stats.pos.y);
		e->stats.pos.x = pc->stats.pos.x;
		e->stats.pos.y = pc->stats.pos.y;
		mapr->collider.block(e->stats.pos.x, e->stats.pos.y, true);
		hero_dist = 0;
	}

	bool enemies_in_combat = false;
	//enter combat because enemy is targeting the player or a summon
	for (unsigned int i=0; i < enemym->enemies.size(); i++) {
		if(enemym->enemies[i]->stats.in_combat && !enemym->enemies[i]->stats.hero_ally) {
			Enemy* enemy = enemym->enemies[i];

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

	// aggressive creatures are always in combat
	if (e->stats.combat_style == COMBAT_AGGRESSIVE)
		e->stats.in_combat = true;

	//the default target is the player
	if(!e->stats.in_combat) {
		pursue_pos.x = pc->stats.pos.x;
		pursue_pos.y = pc->stats.pos.y;
		target_dist = hero_dist;
	}

	// check line-of-sight
	if (target_dist < e->stats.threat_range && pc->stats.alive)
		los = mapr->collider.line_of_sight(e->stats.pos.x, e->stats.pos.y, pursue_pos.x, pursue_pos.y);
	else
		los = false;

	//if the player is blocked, all summons which the player is facing to move away for the specified frames
	//need to set the flag player_blocked so that other allies know to get out of the way as well
	//if hero is facing the summon
	if(ENABLE_ALLY_COLLISION_AI) {
		if(!enemym->player_blocked && hero_dist < ALLY_FLEE_DISTANCE
				&& mapr->collider.is_facing(pc->stats.pos.x,pc->stats.pos.y,pc->stats.direction,e->stats.pos.x,e->stats.pos.y)) {
			enemym->player_blocked = true;
			enemym->player_blocked_ticks = BLOCK_TICKS;
		}

		if(enemym->player_blocked && !e->stats.in_combat
				&& mapr->collider.is_facing(pc->stats.pos.x,pc->stats.pos.y,pc->stats.direction,e->stats.pos.x,e->stats.pos.y)) {
			fleeing = true;
			pursue_pos = pc->stats.pos;
		}
	}

	if(e->stats.effects.fear) fleeing = true;

	// If we have a successful chance_flee roll, try to move to a safe distance
	if (
			e->stats.in_combat &&
			e->stats.cur_state == ENEMY_STANCE &&
			!move_to_safe_dist && hero_dist < e->stats.flee_range &&
			hero_dist >= e->stats.melee_range &&
			percentChance(e->stats.chance_flee) &&
			flee_cooldown == 0
		)
	{
		move_to_safe_dist = true;
	}

	if (move_to_safe_dist) fleeing = true;

	if (fleeing) {
		FPoint target_pos = pursue_pos;

		std::vector<int> flee_dirs;

		int middle_dir = calcDirection(target_pos.x, target_pos.y, e->stats.pos.x, e->stats.pos.y);
		for (int i = -2; i <= 2; ++i) {
			int test_dir = rotateDirection(middle_dir, i);

			FPoint test_pos = calcVector(e->stats.pos, test_dir, 1);
			if (mapr->collider.is_valid_position(test_pos.x, test_pos.y, e->stats.movement_type, false)) {
				if (test_dir == e->stats.direction) {
					// if we're already moving in a good direction, favor it over other directions
					flee_dirs.clear();
					flee_dirs.push_back(test_dir);
					break;
				}
				else {
					flee_dirs.push_back(test_dir);
				}
			}
		}

		if (flee_dirs.empty()) {
			// trapped and can't move
			move_to_safe_dist = false;
			fleeing = false;
		}
		else {
			int index = randBetween(0, static_cast<int>(flee_dirs.size())-1);
			pursue_pos = calcVector(e->stats.pos, flee_dirs[index], 1);

			if (flee_ticks == 0) {
				flee_ticks = e->stats.flee_duration;
			}
		}
	}
}

void BehaviorAlly::checkMoveStateStance() {

	// If the enemy is capable of fleeing and is at a safe distance, have it hold its position instead of moving
	if (hero_dist >= e->stats.flee_range && e->stats.chance_flee > 0) return;

	// try to move to the target if we're either:
	// 1. too far away and chance_pursue roll succeeds
	// 2. within range, but lack line-of-sight (required to attack)
	bool should_move_to_target = (target_dist > e->stats.melee_range && percentChance(e->stats.chance_pursue)) || (target_dist <= e->stats.melee_range && !los) || (hero_dist > ALLY_FOLLOW_DISTANCE_WALK);

	if (should_move_to_target || fleeing) {
		if(e->stats.in_combat && target_dist > e->stats.melee_range) {
			if (e->move())
				e->stats.cur_state = ENEMY_MOVE;
		}

		if((!e->stats.in_combat && hero_dist > ALLY_FOLLOW_DISTANCE_WALK) || fleeing) {
			if (e->move()) {
				e->stats.cur_state = ENEMY_MOVE;
			}
			else {
				collided = true;
				unsigned char prev_direction = e->stats.direction;

				// hit an obstacle, try the next best angle
				e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
				if (e->move()) {
					e->stats.cur_state = ENEMY_MOVE;
				}
				else e->stats.direction = prev_direction;
			}
		}
	}
}

void BehaviorAlly::checkMoveStateMove() {
	bool can_attack = true;

	if (e->stats.cooldown_ticks > 0) {
		can_attack = false;
	}
	else {
		can_attack = false;
		for (size_t i = 0; i < e->stats.powers_ai.size(); ++i) {
			if (e->stats.powers_ai[i].ticks == 0) {
				can_attack = true;
				break;
			}
		}
	}
	// in order to prevent infinite fleeing, we re-roll our chance to flee after a certain duration
	bool stop_fleeing = can_attack && fleeing && flee_ticks == 0 && !percentChance(e->stats.chance_flee);

	if (!stop_fleeing && flee_ticks == 0) {
		// if the roll to continue fleeing succeeds, but the flee duration has expired, we don't want to reset the duration to the full amount
		// instead, we scehdule the next re-roll to happen on the next frame
		// this will continue until a roll fails, returning to the stance state
		flee_ticks = 1;
	}

	//if close enough to hero, stop miving
	if((hero_dist < ALLY_FOLLOW_DISTANCE_STOP && !e->stats.in_combat && !fleeing)
			|| (target_dist < e->stats.melee_range && e->stats.in_combat && !fleeing)
			|| (move_to_safe_dist && target_dist >= e->stats.threat_range/2)
			|| stop_fleeing)
	{
		if (stop_fleeing) {
			flee_cooldown = e->stats.flee_cooldown;
		}
		e->stats.cur_state = ENEMY_STANCE;
		move_to_safe_dist = false;
		fleeing = false;
	}

	// try to continue moving
	else if (!e->move()) {
		collided = true;
		unsigned char prev_direction = e->stats.direction;
		// hit an obstacle.  Try the next best angle
		e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
		if (!e->move()) {
			//this prevents an ally trying to move perpendicular to a bridge if the player gets close to it in a certain position and gets blocked
			if(enemym->player_blocked && !e->stats.in_combat) {
				e->stats.direction = pc->stats.direction;
				if (!e->move()) {
					e->stats.cur_state = ENEMY_STANCE;
					e->stats.direction = prev_direction;
				}
			}
			else {
				e->stats.cur_state = ENEMY_STANCE;
				e->stats.direction = prev_direction;
			}
		}
	}
}

