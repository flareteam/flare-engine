/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2012-2016 Justin Jacobs

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

#include "Animation.h"
#include "Avatar.h"
#include "BehaviorStandard.h"
#include "CommonIncludes.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "EngineSettings.h"
#include "MapRenderer.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsMath.h"

BehaviorStandard::BehaviorStandard(Enemy *_e)
	: EnemyBehavior(_e)
	, path()
	, prev_target()
	, collided(false)
	, path_found(false)
	, chance_calc_path(0)
	, target_dist(0)
	, pursue_pos(-1, -1)
	, los(false)
	, fleeing(false)
	, move_to_safe_dist(false)
	, turn_timer()
{
}

/**
 * One frame of logic for this behavior
 */
void BehaviorStandard::logic() {

	// skip all logic if the enemy is dead and no longer animating
	if (e->stats.corpse) {
		e->stats.corpse_timer.tick();
		return;
	}

	if (!e->stats.hero_ally) {
		if (Utils::calcDist(e->stats.pos, pc->stats.pos) <= settings->encounter_dist)
			e->stats.encountered = true;

		if (!e->stats.encountered)
			return;
	}

	doUpkeep();
	findTarget();
	checkPower();
	checkMove();
	updateState();

	fleeing = false;
}

/**
 * Various upkeep on stats
 */
void BehaviorStandard::doUpkeep() {
	// activate all passive powers
	if (e->stats.hp > 0 || e->stats.effects.triggered_death)
		powers->activatePassives(&e->stats);

	e->stats.logic();

	// bleeding to death
	if (e->stats.bleed_source_type != -1) {
		e->doRewards(e->stats.bleed_source_type);
		e->stats.bleed_source_type = -1;
	}

	// check for teleport powers
	if (e->stats.teleportation) {

		mapr->collider.unblock(e->stats.pos.x,e->stats.pos.y);

		e->stats.pos.x = e->stats.teleport_destination.x;
		e->stats.pos.y = e->stats.teleport_destination.y;

		mapr->collider.block(e->stats.pos.x,e->stats.pos.y, e->stats.hero_ally);

		e->stats.teleportation = false;
	}
}

/**
 * Locate the player and set various targeting info
 */
void BehaviorStandard::findTarget() {
	// dying enemies can't target anything
	if (e->stats.cur_state == StatBlock::ENEMY_DEAD || e->stats.cur_state == StatBlock::ENEMY_CRITDEAD) return;

	float stealth_threat_range = (e->stats.threat_range * (100 - static_cast<float>(e->stats.hero_stealth))) / 100;

	// stunned enemies can't act
	if (e->stats.effects.stun) return;

	StatBlock *target_stats = NULL;

	// check distance and line of sight between enemy and hero
	// by default, the enemy pursues the hero directly
	if (pc->stats.alive) {
		target_dist = Utils::calcDist(e->stats.pos, pc->stats.pos);
		target_stats = &pc->stats;
	}
	else {
		target_dist = 0;
	}

	for (size_t i = 0; i < enemym->enemies.size(); ++i) {
		if (!enemym->enemies[i]->stats.corpse && enemym->enemies[i]->stats.hero_ally) {
			//now work out the distance to the minion and compare it to the distance to the current targer (we want to target the closest ally)
			float ally_dist = Utils::calcDist(e->stats.pos, enemym->enemies[i]->stats.pos);
			if (ally_dist < target_dist) {
				target_stats = &enemym->enemies[i]->stats;
				target_dist = ally_dist;
			}
		}
	}

	// check line-of-sight
	if (target_stats && target_dist < e->stats.threat_range && pc->stats.alive)
		los = mapr->collider.lineOfSight(e->stats.pos.x, e->stats.pos.y, target_stats->pos.x, target_stats->pos.y);
	else
		los = false;

	// aggressive enemies are always in combat
	if (!e->stats.in_combat && e->stats.combat_style == StatBlock::COMBAT_AGGRESSIVE) {
		e->stats.join_combat = true;
	}

	// check entering combat (because the player got too close)
	bool close_to_target = false;
	if (&pc->stats == target_stats)
		close_to_target = target_dist < stealth_threat_range;
	else if (target_stats)
		close_to_target = target_dist < e->stats.threat_range;

	if (e->stats.alive && !e->stats.in_combat && los && close_to_target && e->stats.combat_style != StatBlock::COMBAT_PASSIVE) {
		e->stats.join_combat = true;
	}

	// check entering combat (because the player hit the enemy)
	if (e->stats.join_combat) {
		e->stats.in_combat = true;

		StatBlock::AIPower* ai_power = e->stats.getAIPower(StatBlock::AI_POWER_BEACON);
		if (ai_power != NULL) {
			powers->activate(ai_power->id, &e->stats, e->stats.pos); //emit beacon
		}

		ai_power = e->stats.getAIPower(StatBlock::AI_POWER_JOIN_COMBAT);
		if (ai_power != NULL) {
			e->stats.cur_state = StatBlock::ENEMY_POWER;
			e->stats.activated_power = ai_power;
		}

		e->stats.join_combat = false;
	}

	// check exiting combat (player died or got too far away)
	if (e->stats.in_combat && target_dist > (e->stats.threat_range_far) && !e->stats.join_combat && e->stats.combat_style != StatBlock::COMBAT_AGGRESSIVE) {
		e->stats.in_combat = false;
	}

	// check exiting combat (player or enemy died)
	if ((!e->stats.alive || !pc->stats.alive) && e->stats.combat_style != StatBlock::COMBAT_AGGRESSIVE) {
		e->stats.in_combat = false;
	}

	if (target_stats)
		pursue_pos = target_stats->pos;

	// if we just started wandering, set the first waypoint
	if (e->stats.wander && e->stats.waypoints.empty()) {
		FPoint waypoint = getWanderPoint();
		e->stats.waypoints.push(waypoint);
		e->stats.waypoint_timer.reset(Timer::BEGIN);
	}

	// if we're not in combat, pursue the next waypoint
	if (!(e->stats.in_combat || e->stats.waypoints.empty())) {
		FPoint waypoint = e->stats.waypoints.front();
		pursue_pos.x = waypoint.x;
		pursue_pos.y = waypoint.y;
	}

	if(e->stats.effects.fear) fleeing = true;

	// If we have a successful chance_flee roll, try to move to a safe distance
	if (
			e->stats.in_combat &&
			e->stats.cur_state == StatBlock::ENEMY_STANCE &&
			!move_to_safe_dist && target_dist < e->stats.flee_range &&
			target_dist >= e->stats.melee_range &&
			Math::percentChance(e->stats.chance_flee) &&
			e->stats.flee_cooldown_timer.isEnd()
		)
	{
		move_to_safe_dist = true;
	}

	if (move_to_safe_dist) fleeing = true;

	if (fleeing) {
		FPoint target_pos = pursue_pos;

		std::vector<int> flee_dirs;

		int middle_dir = Utils::calcDirection(target_pos.x, target_pos.y, e->stats.pos.x, e->stats.pos.y);
		for (int i = -2; i <= 2; ++i) {
			int test_dir = Utils::rotateDirection(middle_dir, i);

			FPoint test_pos = Utils::calcVector(e->stats.pos, test_dir, 1);
			if (mapr->collider.isValidPosition(test_pos.x, test_pos.y, e->stats.movement_type, MapCollision::COLLIDE_NORMAL)) {
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
			int index = Math::randBetween(0, static_cast<int>(flee_dirs.size())-1);
			pursue_pos = Utils::calcVector(e->stats.pos, flee_dirs[index], 1);

			if (e->stats.flee_timer.isEnd()) {
				e->stats.flee_timer.reset(Timer::BEGIN);
			}
		}
	}
}

/**
 * Begin using a power if idle, based on behavior % chances.
 * Activate a ready power, if the attack animation has followed through
 */
void BehaviorStandard::checkPower() {

	// stunned enemies can't act
	if (e->stats.effects.stun || e->stats.effects.fear || fleeing) return;

	// currently all enemy power use happens during combat
	if (!e->stats.in_combat) return;

	// if the enemy is on global cooldown it cannot act
	if (!e->stats.cooldown.isEnd()) return;

	// Note there are two stages to activating a power.
	// First is the enemy choosing to use a power based on behavioral chance
	// Second is the power actually firing off once the related animation reaches the active frame.
	// The second stage occurs in updateState()

	// pick a power from the available powers for this creature
	if (e->stats.cur_state == StatBlock::ENEMY_STANCE || e->stats.cur_state == StatBlock::ENEMY_MOVE) {
		StatBlock::AIPower* ai_power = NULL;

		// check half dead power use
		if (e->stats.half_dead_power && e->stats.hp <= e->stats.get(Stats::HP_MAX)/2) {
			ai_power = e->stats.getAIPower(StatBlock::AI_POWER_HALF_DEAD);
		}
		// check ranged power use
		else if (target_dist > e->stats.melee_range) {
			ai_power = e->stats.getAIPower(StatBlock::AI_POWER_RANGED);
		}
		// check melee power use
		else {
			ai_power = e->stats.getAIPower(StatBlock::AI_POWER_MELEE);
		}

		if (ai_power != NULL) {
			const Power& pwr = powers->powers[ai_power->id];
			if (!los && (pwr.requires_los || pwr.requires_los_default)) {
				ai_power = NULL;
			}
			if (ai_power != NULL) {
				e->stats.cur_state = StatBlock::ENEMY_POWER;
				e->stats.activated_power = ai_power;
			}
		}
	}

	if (e->stats.cur_state != StatBlock::ENEMY_POWER && e->stats.activated_power) {
		e->stats.activated_power = NULL;
	}
}

/**
 * Check state changes related to movement
 */
void BehaviorStandard::checkMove() {

	// dying enemies can't move
	if (e->stats.cur_state == StatBlock::ENEMY_DEAD || e->stats.cur_state == StatBlock::ENEMY_CRITDEAD) return;

	// stunned enemies can't act
	if (e->stats.effects.stun) return;

	// handle not being in combat and (not patrolling waypoints or waiting at waypoint)
	if (!e->stats.hero_ally && !e->stats.in_combat && (e->stats.waypoints.empty() || !e->stats.waypoint_timer.isEnd())) {

		if (e->stats.cur_state == StatBlock::ENEMY_MOVE) {
			e->stats.cur_state = StatBlock::ENEMY_STANCE;
		}

		// currently enemies only move while in combat or patrolling
		return;
	}

	float real_speed = e->stats.speed * speedMultiplyer[e->stats.direction] * e->stats.effects.speed / 100;

	unsigned turn_ticks = turn_timer.getCurrent();
	turn_timer.setDuration(e->stats.turn_delay);

	// If an enemy's turn_delay is too long compared to their speed, they will be unable to follow a path properly.
	// So here, we get how many frames it takes to traverse a single tile and then compare it to the turn delay time.
	// We then cap the turn delay the time at the number of frames we calculated for tile traversal.
	// There may be other solutions to this problem, such as having the enemy pause when they reach a path point,
	// but I was unable to get anything else working as cleanly/bug-free as this.
	int max_turn_ticks = static_cast<int>(1.f / real_speed);
	if (e->stats.turn_delay > max_turn_ticks) {
		turn_timer.setDuration(max_turn_ticks);
	}
	turn_timer.setCurrent(turn_ticks);

	// clear current space to allow correct movement
	mapr->collider.unblock(e->stats.pos.x, e->stats.pos.y);

	// update direction
	if (e->stats.facing) {
		turn_timer.tick();
		if (turn_timer.isEnd()) {

			// if blocked, face in pathfinder direction instead
			if (!mapr->collider.lineOfMovement(e->stats.pos.x, e->stats.pos.y, pursue_pos.x, pursue_pos.y, e->stats.movement_type)) {

				// if a path is returned, target first waypoint

				bool recalculate_path = false;

				//if theres no path, it needs to be calculated
				if(path.empty())
					recalculate_path = true;

				//if the target moved more than 1 tile away, recalculate
				if(Utils::calcDist(FPoint(Point(prev_target)), FPoint(Point(pursue_pos))) > 1.f)
					recalculate_path = true;

				//if a collision ocurred then recalculate
				if(collided)
					recalculate_path = true;

				//add a 5% chance to recalculate on every frame. This prevents reclaulating lots of entities in the same frame
				chance_calc_path += 5;

				if(Math::percentChance(chance_calc_path))
					recalculate_path = true;

				//dont recalculate if we were blocked and no path was found last time
				//this makes sure that pathfinding calculation is not spammed when the target is unreachable and the entity is as close as its going to get
				if(!path_found && collided && !Math::percentChance(chance_calc_path))
					recalculate_path = false;
				else//reset the collision flag only if we dont want the cooldown in place
					collided = false;

				prev_target = pursue_pos;

				// target first waypoint
				if(recalculate_path) {
					chance_calc_path = -100;
					path.clear();
					path_found = mapr->collider.computePath(e->stats.pos, pursue_pos, path, e->stats.movement_type, MapCollision::DEFAULT_PATH_LIMIT);
				}

				if(!path.empty()) {
					pursue_pos = path.back();

					//if distance to node is lower than a tile size, the node is going to be passed and can be removed
					if(Utils::calcDist(e->stats.pos, pursue_pos) <= 1.f)
						path.pop_back();
				}
			}
			else {
				path.clear();
			}

			if (e->stats.charge_speed == 0.0f) {
				e->stats.direction = Utils::calcDirection(e->stats.pos.x, e->stats.pos.y, pursue_pos.x, pursue_pos.y);
			}
			turn_timer.reset(Timer::BEGIN);
		}
	}

	e->stats.flee_timer.tick();
	e->stats.flee_cooldown_timer.tick();

	// try to start moving
	if (e->stats.cur_state == StatBlock::ENEMY_STANCE) {
		checkMoveStateStance();
	}

	// already moving
	else if (e->stats.cur_state == StatBlock::ENEMY_MOVE) {
		checkMoveStateMove();
	}

	// if patrolling waypoints and has reached a waypoint, cycle to the next one
	if (!e->stats.waypoints.empty()) {
		// if the patroller is close to the waypoint
		FPoint waypoint = e->stats.waypoints.front();
		float waypoint_dist = Utils::calcDist(waypoint, e->stats.pos);

		FPoint saved_pos = e->stats.pos;
		e->move();
		float new_dist = Utils::calcDist(waypoint, e->stats.pos);
		e->stats.pos = saved_pos;

		if (waypoint_dist <= real_speed || (waypoint_dist <= 0.5f && new_dist > waypoint_dist)) {
			e->stats.pos = waypoint;
			turn_timer.reset(Timer::END);
			e->stats.waypoints.pop();
			// pick a new random point if we're wandering
			if (e->stats.wander) {
				waypoint = getWanderPoint();
			}
			e->stats.waypoints.push(waypoint);
			e->stats.waypoint_timer.reset(Timer::BEGIN);
		}
	}

	// re-block current space to allow correct movement
	mapr->collider.block(e->stats.pos.x, e->stats.pos.y, e->stats.hero_ally);

}

void BehaviorStandard::checkMoveStateStance() {

	// If the enemy is capable of fleeing and is at a safe distance, have it hold its position instead of moving
	if (target_dist >= e->stats.flee_range && e->stats.chance_flee > 0 && e->stats.waypoints.empty()) return;

	// try to move to the target if we're either:
	// 1. too far away and chance_pursue roll succeeds
	// 2. within range, but lack line-of-sight (required to attack)
	bool should_move_to_target = (target_dist > e->stats.melee_range && Math::percentChance(e->stats.chance_pursue)) || (target_dist <= e->stats.melee_range && !los);

	if (should_move_to_target || fleeing) {

		if (e->move()) {
			e->stats.cur_state = StatBlock::ENEMY_MOVE;
		}
		else {
			collided = true;
			unsigned char prev_direction = e->stats.direction;

			// hit an obstacle, try the next best angle
			e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
			if (e->move()) {
				e->stats.cur_state = StatBlock::ENEMY_MOVE;
			}
			else
				e->stats.direction = prev_direction;
		}
	}
}

void BehaviorStandard::checkMoveStateMove() {
	bool can_attack = true;

	if (!e->stats.cooldown.isEnd()) {
		can_attack = false;
	}
	else {
		can_attack = false;
		for (size_t i = 0; i < e->stats.powers_ai.size(); ++i) {
			if (e->stats.powers_ai[i].cooldown.isEnd()) {
				can_attack = true;
				break;
			}
		}
	}
	// in order to prevent infinite fleeing, we re-roll our chance to flee after a certain duration
	bool stop_fleeing = can_attack && fleeing && e->stats.flee_timer.isEnd() && !Math::percentChance(e->stats.chance_flee);

	if (!stop_fleeing && e->stats.flee_timer.isEnd()) {
		// if the roll to continue fleeing succeeds, but the flee duration has expired, we don't want to reset the duration to the full amount
		// instead, we scehdule the next re-roll to happen on the next frame
		// this will continue until a roll fails, returning to the stance state
		e->stats.flee_timer.setCurrent(1);
	}

	// close enough to the hero or is at a safe distance
	if (pc->stats.alive && ((target_dist < e->stats.melee_range && !fleeing) || (move_to_safe_dist && target_dist >= e->stats.flee_range) || stop_fleeing)) {
		if (stop_fleeing) {
			e->stats.flee_cooldown_timer.reset(Timer::BEGIN);
		}
		e->stats.cur_state = StatBlock::ENEMY_STANCE;
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
			e->stats.cur_state = StatBlock::ENEMY_STANCE;
			e->stats.direction = prev_direction;
		}
	}
}


/**
 * Perform miscellaneous state-based actions.
 * 1) Set animations and sound effects
 * 2) Return to the default state (Stance) when actions are complete
 */
void BehaviorStandard::updateState() {

	// stunned enemies can't act
	if (e->stats.effects.stun) return;

	int power_id;
	int power_state;

	// continue current animations
	e->activeAnimation->advanceFrame();

	switch (e->stats.cur_state) {

		case StatBlock::ENEMY_STANCE:

			e->setAnimation("stance");
			break;

		case StatBlock::ENEMY_MOVE:

			e->setAnimation("run");
			break;

		case StatBlock::ENEMY_POWER:

			if (e->stats.activated_power == NULL) {
				e->stats.cur_state = StatBlock::ENEMY_STANCE;
				break;
			}

			power_id = powers->checkReplaceByEffect(e->stats.activated_power->id, &e->stats);
			power_state = powers->powers[power_id].new_state;
			e->stats.prevent_interrupt = powers->powers[power_id].prevent_interrupt;

			// animation based on power type
			if (power_state == Power::STATE_INSTANT)
				e->instant_power = true;
			else if (power_state == Power::STATE_ATTACK)
				e->setAnimation(powers->powers[power_id].attack_anim);

			// sound effect based on power type
			if (e->activeAnimation->isFirstFrame()) {
				if (powers->powers[power_id].pre_power > 0 && Math::percentChance(powers->powers[power_id].pre_power_chance)) {
					powers->activate(powers->powers[power_id].pre_power, &e->stats, pursue_pos);
				}

				float attack_speed = (e->stats.effects.getAttackSpeed(powers->powers[power_id].attack_anim) * powers->powers[power_id].attack_speed) / 100.0f;
				e->activeAnimation->setSpeed(attack_speed);
				e->playAttackSound(powers->powers[power_id].attack_anim);

				if (powers->powers[power_id].state_duration > 0)
					e->stats.state_timer.setDuration(powers->powers[power_id].state_duration);

				if (powers->powers[power_id].charge_speed != 0.0f)
					e->stats.charge_speed = powers->powers[power_id].charge_speed;
			}

			// Activate Power:
			// if we're at the active frame of a power animation,
			// activate the power and set the local and global cooldowns
			if ((e->activeAnimation->isActiveFrame() || e->instant_power) && !e->stats.hold_state) {
				powers->activate(power_id, &e->stats, pursue_pos);

				// set cooldown for all ai powers with the same power id
				for (size_t i = 0; i < e->stats.powers_ai.size(); ++i) {
					if (e->stats.activated_power->id == e->stats.powers_ai[i].id) {
						e->stats.powers_ai[i].cooldown.setDuration(powers->powers[power_id].cooldown);
					}
				}

				if (e->stats.activated_power->type == StatBlock::AI_POWER_HALF_DEAD) {
					e->stats.half_dead_power = false;
				}

				if (!e->stats.state_timer.isEnd())
					e->stats.hold_state = true;
			}

			// animation is finished
			if ((e->activeAnimation->isLastFrame() && e->stats.state_timer.isEnd()) ||
			    (power_state == Power::STATE_ATTACK && e->activeAnimation->getName() != powers->powers[power_id].attack_anim) ||
			    e->instant_power)
			{
				if (!e->instant_power)
					e->stats.cooldown.reset(Timer::BEGIN);
				else
					e->instant_power = false;

				e->stats.activated_power = NULL;
				e->stats.cur_state = StatBlock::ENEMY_STANCE;
				e->stats.prevent_interrupt = false;
			}
			break;

		case StatBlock::ENEMY_SPAWN:

			e->setAnimation("spawn");
			//the second check is needed in case the entity does not have a spawn animation
			if (e->activeAnimation->isLastFrame() || e->activeAnimation->getName() != "spawn") {
				e->stats.cur_state = StatBlock::ENEMY_STANCE;
			}
			break;

		case StatBlock::ENEMY_BLOCK:

			e->setAnimation("block");
			break;

		case StatBlock::ENEMY_HIT:

			e->setAnimation("hit");
			if (e->activeAnimation->isFirstFrame()) {
				e->stats.effects.triggered_hit = true;
			}
			if (e->activeAnimation->isLastFrame() || e->activeAnimation->getName() != "hit")
				e->stats.cur_state = StatBlock::ENEMY_STANCE;
			break;

		case StatBlock::ENEMY_DEAD:
			if (e->stats.effects.triggered_death) break;

			e->setAnimation("die");
			if (e->activeAnimation->isFirstFrame()) {
				e->playSound(Entity::SOUND_DIE);
				e->stats.corpse_timer.setDuration(eset->misc.corpse_timeout);
			}
			if (e->activeAnimation->isSecondLastFrame()) {
				StatBlock::AIPower* ai_power = e->stats.getAIPower(StatBlock::AI_POWER_DEATH);
				if (ai_power != NULL)
					powers->activate(ai_power->id, &e->stats, e->stats.pos);

				e->stats.effects.clearEffects();
			}
			if (e->activeAnimation->isLastFrame() || e->activeAnimation->getName() != "die") {
				// puts renderable under object layer
				e->stats.corpse = true;

				//allow free movement over the corpse
				mapr->collider.unblock(e->stats.pos.x, e->stats.pos.y);

				// remove corpses that land on blocked tiles, such as water or pits
				if (!mapr->collider.isValidPosition(e->stats.pos.x, e->stats.pos.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_NORMAL)) {
					e->stats.corpse_timer.reset(Timer::END);
				}

				// prevent "jumping" when rendering
				e->stats.pos.align();
			}

			break;

		case StatBlock::ENEMY_CRITDEAD:

			e->setAnimation("critdie");
			if (e->activeAnimation->isFirstFrame()) {
				e->playSound(Entity::SOUND_CRITDIE);
				e->stats.corpse_timer.setDuration(eset->misc.corpse_timeout);
			}
			if (e->activeAnimation->isSecondLastFrame()) {
				StatBlock::AIPower* ai_power = e->stats.getAIPower(StatBlock::AI_POWER_DEATH);
				if (ai_power != NULL)
					powers->activate(ai_power->id, &e->stats, e->stats.pos);

				e->stats.effects.clearEffects();
			}
			if (e->activeAnimation->isLastFrame() || e->activeAnimation->getName() != "critdie") {
				// puts renderable under object layer
				e->stats.corpse = true;

				//allow free movement over the corpse
				mapr->collider.unblock(e->stats.pos.x, e->stats.pos.y);

				// prevent "jumping" when rendering
				e->stats.pos.align();
			}

			break;

		default:
			break;
	}

	if (e->stats.state_timer.isEnd() && e->stats.hold_state)
		e->stats.hold_state = false;

	if (e->stats.cur_state != StatBlock::ENEMY_POWER && e->stats.charge_speed != 0.0f)
		e->stats.charge_speed = 0.0f;
}

FPoint BehaviorStandard::getWanderPoint() {
	FPoint waypoint;
	waypoint.x = static_cast<float>(e->stats.wander_area.x) + static_cast<float>(rand() % (e->stats.wander_area.w)) + 0.5f;
	waypoint.y = static_cast<float>(e->stats.wander_area.y) + static_cast<float>(rand() % (e->stats.wander_area.h)) + 0.5f;

	if (mapr->collider.isValidPosition(waypoint.x, waypoint.y, e->stats.movement_type, mapr->collider.getCollideType(e->stats.hero)) &&
	    mapr->collider.lineOfMovement(e->stats.pos.x, e->stats.pos.y, waypoint.x, waypoint.y, e->stats.movement_type))
	{
		return waypoint;
	}
	else {
		// didn't get a valid waypoint, so keep our current position
		return e->stats.pos;
	}
}
