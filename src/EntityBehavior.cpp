/*
Copyright © 2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Ryan Dansie
Copyright © 2012-2021 Justin Jacobs

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
 * The behavior object is a component of Entity.
 * Make AI decisions (movement, actions) for entities.
 */

#include "Animation.h"
#include "Avatar.h"
#include "CommonIncludes.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EngineSettings.h"
#include "Entity.h"
#include "EntityBehavior.h"
#include "MapRenderer.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsMath.h"

const float EntityBehavior::ALLY_FLEE_DISTANCE = 2;
const float EntityBehavior::ALLY_FOLLOW_DISTANCE_WALK = 5.5;
const float EntityBehavior::ALLY_FOLLOW_DISTANCE_STOP = 5;
const float EntityBehavior::ALLY_TELEPORT_DISTANCE = 40;

EntityBehavior::EntityBehavior(Entity *_e)
	: e(_e)
	, path()
	, prev_target()
	, collided(false)
	, path_found(false)
	, chance_calc_path(0)
	, path_found_fails(0)
	, path_found_fail_timer()
	, target_dist(0)
	, hero_dist(0)
	, pursue_pos(-1, -1)
	, los(false)
	, fleeing(false)
	, move_to_safe_dist(false)
	, turn_timer()
	, instant_power(false)
{
	// wait when PATH_FOUND_FAIL_THRESHOLD is exceeded
	path_found_fail_timer.setDuration(settings->max_frames_per_sec * PATH_FOUND_FAIL_WAIT_SECONDS);
	path_found_fail_timer.reset(Timer::END);
}

/**
 * One frame of logic for this behavior
 */
void EntityBehavior::logic() {
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
void EntityBehavior::doUpkeep() {
	// activate all passive powers
	if (e->stats.hp > 0 || e->stats.effects.triggered_death)
		powers->activatePassives(&e->stats);

	e->stats.logic();

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
void EntityBehavior::findTarget() {
	// dying enemies can't target anything
	if (e->stats.cur_state == StatBlock::ENTITY_DEAD || e->stats.cur_state == StatBlock::ENTITY_CRITDEAD) return;

	// stunned enemies can't act
	if (e->stats.effects.stun) return;

	StatBlock *target_stats = NULL;
	float stealth_threat_range = (e->stats.threat_range * (100 - static_cast<float>(e->stats.hero_stealth))) / 100;
	bool is_ally = e->stats.hero_ally;

	// check distance and line of sight between enemy and hero
	// by default, the enemy pursues the hero directly
	if (pc->stats.alive) {
		target_dist = Utils::calcDist(e->stats.pos, pc->stats.pos);
		target_stats = &pc->stats;
	}
	else {
		target_dist = 0;
	}
	hero_dist = target_dist;

	// if the minion gets too far, transport it to the player pos
	if (is_ally && hero_dist > ALLY_TELEPORT_DISTANCE && !e->stats.in_combat) {
		mapr->collider.unblock(e->stats.pos.x, e->stats.pos.y);
		e->stats.pos.x = pc->stats.pos.x;
		e->stats.pos.y = pc->stats.pos.y;
		mapr->collider.block(e->stats.pos.x, e->stats.pos.y, MapCollision::IS_ALLY);
		hero_dist = 0;
	}

	bool enemies_in_combat = false;
	for (size_t i = 0; i < entitym->entities.size(); ++i) {
		Entity* entity = entitym->entities[i];
		if (!entity->stats.alive)
			continue;

		if ((!is_ally && entity->stats.hero_ally) || (is_ally && !entity->stats.hero_ally && entity->stats.in_combat)) {
			float entity_dist = Utils::calcDist(e->stats.pos, entity->stats.pos);
			if (!target_stats || (is_ally && target_stats && target_stats->hero)) {
				// pick the first available target if none is already selected
				target_stats = &entitym->entities[i]->stats;
				target_dist = entity_dist;
				e->stats.in_combat = true;
				enemies_in_combat = true;
			}
			else if (entity_dist < target_dist) {
				// pick a new target if it's closer
				target_stats = &entitym->entities[i]->stats;
				target_dist = entity_dist;
			}
		}
	}

	//break combat if the player gets too far or all enemies die
	if(!enemies_in_combat)
		e->stats.in_combat = false;

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
	if (!is_ally && &pc->stats == target_stats)
		close_to_target = target_dist < stealth_threat_range;
	else if (target_stats && &pc->stats != target_stats)
		close_to_target = target_dist < e->stats.threat_range;

	if (e->stats.alive && !e->stats.in_combat && los && close_to_target && e->stats.combat_style != StatBlock::COMBAT_PASSIVE) {
		e->stats.join_combat = true;
	}

	// if the join_combat flag wasn't set above, it could have been set if the enemy was hit by a hazard
	// we put the entity in a combat state and activate powers that trigger when entering combat
	if (e->stats.join_combat) {
		e->stats.in_combat = true;

		StatBlock::AIPower* ai_power;
		if (!is_ally) {
			ai_power = e->stats.getAIPower(StatBlock::AI_POWER_BEACON);
			if (ai_power != NULL) {
				powers->activate(ai_power->id, &e->stats, e->stats.pos); //emit beacon
			}
		}

		ai_power = e->stats.getAIPower(StatBlock::AI_POWER_JOIN_COMBAT);
		if (ai_power != NULL) {
			e->stats.cur_state = StatBlock::ENTITY_POWER;
			e->stats.activated_power = ai_power;
		}

		e->stats.join_combat = false;
	}

	// check exiting combat
	if (e->stats.combat_style != StatBlock::COMBAT_AGGRESSIVE) {
		// target got too far away
		if (target_dist > e->stats.threat_range_far && !e->stats.join_combat)
			e->stats.in_combat = false;

		// either party is dead
		if (!e->stats.alive || !pc->stats.alive)
			e->stats.in_combat = false;
	}

	// TODO default ally target to player?

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

	// if the player is blocked, all summons which the player is facing to move away for the specified frames
	// need to set the flag player_blocked so that other allies know to get out of the way as well
	// if hero is facing the summon
	if (is_ally && eset->misc.enable_ally_collision_ai) {
		if (!entitym->player_blocked && hero_dist < ALLY_FLEE_DISTANCE
				&& mapr->collider.isFacing(pc->stats.pos.x,pc->stats.pos.y,pc->stats.direction,e->stats.pos.x,e->stats.pos.y)) {
			entitym->player_blocked = true;
			entitym->player_blocked_timer.reset(Timer::BEGIN);
		}

		bool player_closer_than_target = Utils::calcDist(e->stats.pos, pursue_pos) > Utils::calcDist(e->stats.pos, pc->stats.pos);

		if (entitym->player_blocked && (!e->stats.in_combat || player_closer_than_target)
				&& mapr->collider.isFacing(pc->stats.pos.x,pc->stats.pos.y,pc->stats.direction,e->stats.pos.x,e->stats.pos.y)) {
			fleeing = true;
			pursue_pos = pc->stats.pos;
		}
	}

	if (e->stats.effects.fear) fleeing = true;

	// If we have a successful chance_flee roll, try to move to a safe distance
	if (
			e->stats.in_combat &&
			e->stats.cur_state == StatBlock::ENTITY_STANCE &&
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
void EntityBehavior::checkPower() {

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
	if (e->stats.cur_state == StatBlock::ENTITY_STANCE || e->stats.cur_state == StatBlock::ENTITY_MOVE) {
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
				e->stats.cur_state = StatBlock::ENTITY_POWER;
				e->stats.activated_power = ai_power;
			}
		}
	}

	if (e->stats.cur_state != StatBlock::ENTITY_POWER && e->stats.activated_power) {
		e->stats.activated_power = NULL;
	}
}

/**
 * Check state changes related to movement
 */
void EntityBehavior::checkMove() {

	// dying enemies can't move
	if (e->stats.cur_state == StatBlock::ENTITY_DEAD || e->stats.cur_state == StatBlock::ENTITY_CRITDEAD) return;

	// stunned enemies can't act
	if (e->stats.effects.stun) return;

	// handle not being in combat and (not patrolling waypoints or waiting at waypoint)
	if (!e->stats.hero_ally && !e->stats.in_combat && (e->stats.waypoints.empty() || !e->stats.waypoint_timer.isEnd())) {

		if (e->stats.cur_state == StatBlock::ENTITY_MOVE) {
			e->stats.cur_state = StatBlock::ENTITY_STANCE;
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

	path_found_fail_timer.tick();

	// update direction
	if (e->stats.facing) {
		turn_timer.tick();
		if (turn_timer.isEnd()) {

			// if blocked, face in pathfinder direction instead
			if (!mapr->collider.lineOfMovement(e->stats.pos.x, e->stats.pos.y, pursue_pos.x, pursue_pos.y, e->stats.movement_type)) {

				// if a path is returned, target first waypoint

				bool recalculate_path = false;

				// add a 5% chance to recalculate on every frame. This prevents reclaulating lots of entities in the same frame
				chance_calc_path += 5;

				bool calc_path_success = Math::percentChance(chance_calc_path);
				if (calc_path_success)
					recalculate_path = true;

				// if a collision ocurred then recalculate
				if (collided)
					recalculate_path = true;

				// if theres no path, it needs to be calculated
				if (!recalculate_path && path.empty())
					recalculate_path = true;

				// if the target moved more than 1 tile away, recalculate
				if (!recalculate_path && Utils::calcDist(FPoint(Point(prev_target)), FPoint(Point(pursue_pos))) > 1.f)
					recalculate_path = true;

				// dont recalculate if we were blocked and no path was found last time
				// this makes sure that pathfinding calculation is not spammed when the target is unreachable and the entity is as close as its going to get
				if (!path_found && collided && !calc_path_success) {
					recalculate_path = false;
				}
				else {
					// reset the collision flag only if we dont want the cooldown in place
					collided = false;
				}

				if (!path_found_fail_timer.isEnd()) {
					recalculate_path = false;
					chance_calc_path = -100;
				}

				prev_target = pursue_pos;

				// target first waypoint
				if (recalculate_path) {
					chance_calc_path = -100;
					path.clear();
					path_found = mapr->collider.computePath(e->stats.pos, pursue_pos, path, e->stats.movement_type, MapCollision::DEFAULT_PATH_LIMIT);

					if (!path_found) {
						path_found_fails++;
						if (path_found_fails >= PATH_FOUND_FAIL_THRESHOLD) {
							// could not find a path after several tries, so wait a little before the next attempt
							path_found_fail_timer.reset(Timer::BEGIN);
						}
					}
					else {
						path_found_fails = 0;
						path_found_fail_timer.reset(Timer::END);
					}
				}

				if (!path.empty()) {
					pursue_pos = path.back();

					// if distance to node is lower than a tile size, the node is going to be passed and can be removed
					if (Utils::calcDist(e->stats.pos, pursue_pos) <= 1.f)
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
	if (e->stats.cur_state == StatBlock::ENTITY_STANCE) {
		checkMoveStateStance();
	}

	// already moving
	else if (e->stats.cur_state == StatBlock::ENTITY_MOVE) {
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

void EntityBehavior::checkMoveStateStance() {

	// If the enemy is capable of fleeing and is at a safe distance, have it hold its position instead of moving
	if (target_dist >= e->stats.flee_range && e->stats.chance_flee > 0 && e->stats.waypoints.empty()) return;

	// try to move to the target if we're either:
	// 1. too far away and chance_pursue roll succeeds
	// 2. within range, but lack line-of-sight (required to attack)
	bool ally_targeting_hero = e->stats.hero_ally && !e->stats.in_combat && hero_dist > ALLY_FOLLOW_DISTANCE_WALK;
	bool should_move_to_target = e->stats.in_combat && ((target_dist > e->stats.melee_range && Math::percentChance(e->stats.chance_pursue)) || (target_dist <= e->stats.melee_range && !los));

	if (should_move_to_target || fleeing || ally_targeting_hero) {

		if (e->move()) {
			e->stats.cur_state = StatBlock::ENTITY_MOVE;
		}
		else {
			collided = true;
			unsigned char prev_direction = e->stats.direction;

			// hit an obstacle, try the next best angle
			e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
			if (e->move()) {
				e->stats.cur_state = StatBlock::ENTITY_MOVE;
			}
			else
				e->stats.direction = prev_direction;
		}
	}
}

void EntityBehavior::checkMoveStateMove() {
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
	bool ally_targeting_hero = e->stats.hero_ally && !e->stats.in_combat && !fleeing && hero_dist < ALLY_FOLLOW_DISTANCE_STOP;
	if (pc->stats.alive && ((target_dist < e->stats.melee_range && !fleeing) || (move_to_safe_dist && target_dist >= e->stats.flee_range) || stop_fleeing || ally_targeting_hero)) {
		if (stop_fleeing) {
			e->stats.flee_cooldown_timer.reset(Timer::BEGIN);
		}
		e->stats.cur_state = StatBlock::ENTITY_STANCE;
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
			// this prevents an ally trying to move perpendicular to a 1-tile-wide path if the player gets close to it in a certain position and gets blocked
			if (e->stats.hero_ally && entitym->player_blocked && !e->stats.in_combat) {
				e->stats.direction = pc->stats.direction;
				if (!e->move()) {
					e->stats.cur_state = StatBlock::ENTITY_STANCE;
					e->stats.direction = prev_direction;
				}
			}
			else {
				e->stats.cur_state = StatBlock::ENTITY_STANCE;
				e->stats.direction = prev_direction;
			}
		}
	}
}


/**
 * Perform miscellaneous state-based actions.
 * 1) Set animations and sound effects
 * 2) Return to the default state (Stance) when actions are complete
 */
void EntityBehavior::updateState() {

	// stunned enemies can't act
	if (e->stats.effects.stun) return;

	PowerID power_id;
	int power_state;

	// continue current animations
	e->activeAnimation->advanceFrame();

	switch (e->stats.cur_state) {

		case StatBlock::ENTITY_STANCE:

			e->setAnimation("stance");
			break;

		case StatBlock::ENTITY_MOVE:

			e->setAnimation("run");
			break;

		case StatBlock::ENTITY_POWER:

			if (e->stats.activated_power == NULL) {
				e->stats.cur_state = StatBlock::ENTITY_STANCE;
				break;
			}

			power_id = powers->checkReplaceByEffect(e->stats.activated_power->id, &e->stats);
			power_state = powers->powers[power_id].new_state;
			e->stats.prevent_interrupt = powers->powers[power_id].prevent_interrupt;

			// animation based on power type
			if (power_state == Power::STATE_INSTANT)
				instant_power = true;
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
			if ((e->activeAnimation->isActiveFrame() || instant_power) && !e->stats.hold_state) {
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
			if ((e->activeAnimation->isLastFrame() && e->stats.state_timer.isEnd()) || (power_state == Power::STATE_ATTACK && e->activeAnimation->getName() != powers->powers[power_id].attack_anim) || instant_power) {
				if (!instant_power)
					e->stats.cooldown.reset(Timer::BEGIN);
				else
					instant_power = false;

				e->stats.activated_power = NULL;
				e->stats.cur_state = StatBlock::ENTITY_STANCE;
				e->stats.prevent_interrupt = false;
			}
			break;

		case StatBlock::ENTITY_SPAWN:

			e->setAnimation("spawn");
			//the second check is needed in case the entity does not have a spawn animation
			if (e->activeAnimation->isLastFrame() || e->activeAnimation->getName() != "spawn") {
				e->stats.cur_state = StatBlock::ENTITY_STANCE;
			}
			break;

		case StatBlock::ENTITY_BLOCK:

			e->setAnimation("block");
			break;

		case StatBlock::ENTITY_HIT:

			e->setAnimation("hit");
			if (e->activeAnimation->isFirstFrame()) {
				e->stats.effects.triggered_hit = true;
			}
			if (e->activeAnimation->isLastFrame() || e->activeAnimation->getName() != "hit")
				e->stats.cur_state = StatBlock::ENTITY_STANCE;
			break;

		case StatBlock::ENTITY_DEAD:
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

		case StatBlock::ENTITY_CRITDEAD:

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

	if (e->stats.cur_state != StatBlock::ENTITY_POWER && e->stats.charge_speed != 0.0f)
		e->stats.charge_speed = 0.0f;
}

FPoint EntityBehavior::getWanderPoint() {
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
EntityBehavior::~EntityBehavior() {
}
