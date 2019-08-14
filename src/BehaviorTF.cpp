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

Leo P:
Behavior logic using a TensorFlow Model through TensorFlowInterface
*/

#include "Avatar.h"
#include "BehaviorTF.h"
#include "CommonIncludes.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "MapRenderer.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "UtilsMath.h"
#include "TensorFlowInterface.h"
// For rendering ai preds
#include "SharedResources.h"
#include "CombatText.h"

#include <cmath>
#include <chrono>
#include <thread>

std::array<float, 5> POS_OFFETS_X {{0.0f, 0.0f, 1.0f, 0.0f, -1.0f}};
std::array<float, 5> POS_OFFETS_Y {{0.0f, 1.0f, 0.0f, -1.0f, 0.0f}};

const bool RENDER_PREDS = true;
const float JOIN_COMBAT_DIST = 1.42f;

BehaviorTF::BehaviorTF(Enemy *_e): BehaviorStandard(_e) {
	// tip_buf = new TooltipData();
	// tip = new WidgetTooltip();

	// Initialize graph for ai
	// Each entity will communicate with their own instance of
	tf_model = new TensorFlowInterface();

	// TODO(LEO) remove
	// test predict method
	std::array<float, TENSOR_IN_LENGTH> test_data{};
	float* test_out = tf_model->predict(test_data);
	logInfo("BehaviorTF: Test predict tf_model with output '%f'.", (*test_out));

}

BehaviorTF::~BehaviorTF() {
}

/**
 * One frame of logic for this behavior
 */
void BehaviorTF::logic() {

	// skip all logic if the enemy is dead and no longer animating
	if (e->stats.corpse) {
		if (e->stats.corpse_ticks > 0)
			e->stats.corpse_ticks--;
		return;
	}

	// standard behavior
	doUpkeep();
	// model behavior
	decideDirection();
	// standard behavior
	checkPower();
	checkMove(); // TODO(Leo): waypoint_pause necessary?
	updateState();

	fleeing = false;
}

/**
 * Use TF Model to determine which direction to take a step
 * For now, combat is deterministically determined by 'in_combat'
 * if within JOIN_COMBAT_DIST of player.
 * Borrows standard logic from BehaviorStandard::findTarget
 */
void BehaviorTF::decideDirection() {

	// dying enemies don't decide anything
	if (e->stats.cur_state == ENEMY_DEAD || e->stats.cur_state == ENEMY_CRITDEAD) return;

	bool is_pred_successful = false;
	std::array<int, NUM_ACTIONS> pred_ints;

	// reduce frequency of prediction
	if(e->stats.turn_ticks >= e->stats.turn_delay) {

		std::array<float, NUM_ACTIONS> preds;
		float max_pred=0.0f, min_pred=0.0f;

		for ( int action_int = 0; action_int < NUM_ACTIONS; action_int++ )
		{
			 ACTION action = static_cast<ACTION>(action_int);
			 std::array<float, TENSOR_IN_LENGTH> game_data = getGameStateData(action);
			 float * pred = tf_model->predict(game_data);
			 preds[action_int] = *pred;
			 max_pred = std::max(preds[action_int], max_pred);
			 min_pred = std::min(preds[action_int], min_pred);
			 //int pred_int = int( (*pred) * 10000 );
			 //pred_ints[action_int] = pred_int;
			 //max_pred_int = std::max(max_pred_int, pred_int);
			 //min_pred_int = std::min(min_pred_int, pred_int);
		}

		is_pred_successful = max_pred > min_pred;
		//pred_ints = normalizePredictions(preds, min_pred, max_pred);
		if(is_pred_successful) {
			for ( int action_int = 0; action_int < NUM_ACTIONS; action_int++) {
				pred_ints[action_int] = int( (preds[action_int] - min_pred) * 10000 );
			}
		}

		// render pred above entity
		if(RENDER_PREDS && percentChance(25) && is_pred_successful) {
			CombatText *combat_text = comb;
			int displaytype = COMBAT_MESSAGE_MISS;
			
			for ( unsigned long i=0; i<=preds.size(); i++) {
				if ( preds[i] == max_pred ) {
					displaytype = COMBAT_MESSAGE_TAKEDMG;
				}
				FPoint pos = e->stats.pos;
				pos.x += POS_OFFETS_X[i];
				pos.y += POS_OFFETS_Y[i];
				combat_text->addInt(pred_ints[i], pos, displaytype);
			}
		}
	}

	// stunned enemies can't act
	if (e->stats.effects.stun) return;

	decideCombat();

	// if we're not in combat, clear and set waypoint based on TF prediction
	// TODO(LP): is there a timing thing here between clearing and reseting waypoints that causes jitter?
	if (!e->stats.in_combat && is_pred_successful) {
		std::queue<FPoint>().swap(e->stats.waypoints);

		ACTION decided_action = chooseAction(pred_ints);
		FPoint decided_pos = moveAction(decided_action, e->stats.pos);
		e->stats.waypoints.push(decided_pos);

		pursue_pos.x = decided_pos.x;
		pursue_pos.y = decided_pos.y;

		// e->stats.waypoint_pause_ticks = 0;
	}

	// check line-of-sight
	if (target_dist < e->stats.threat_range && pc->stats.alive)
		los = mapr->collider.line_of_sight(e->stats.pos.x, e->stats.pos.y, pc->stats.pos.x, pc->stats.pos.y);
	else
		los = false;

	decideFlee();
	// hijack flee_range
	e->stats.flee_range = JOIN_COMBAT_DIST;
}

/**
 * Choose an action with weight proportional to model predictions.
 */
ACTION BehaviorTF::chooseAction(std::array<int, NUM_ACTIONS> pred_ints) {
	int sumPreds = std::accumulate(pred_ints.begin(), pred_ints.end(), 0);
	int choice = randBetween(0,sumPreds);
	int cumSum = 0;

	for ( int i = 0; i < NUM_ACTIONS; i++ )
	{
		cumSum += pred_ints[i];
		if(choice <= cumSum) {
			return static_cast<ACTION>(i);
		}
	}

	// return default action if somehow the loop fails to return
	return ACTION::NONE;
}

/**
 * For now, combat is deterministically determined.
 * Borrows standard logic from BehaviorStandard::findTarget
 */
void BehaviorTF::decideCombat() {
	// check distance and line of sight between enemy and hero
	if (pc->stats.alive)
		hero_dist = calcDist(e->stats.pos, pc->stats.pos);
	else
		hero_dist = 0;

	// check entering combat (because the player got too close)
	if (e->stats.alive && !e->stats.in_combat && hero_dist <= JOIN_COMBAT_DIST) {
		e->stats.join_combat = true;
	}

	// check entering combat (because the player hit the enemy)
	if (e->stats.join_combat) {
		e->stats.in_combat = true;

		AIPower* ai_power = e->stats.getAIPower(AI_POWER_BEACON);
		if (ai_power != NULL) {
			powers->activate(ai_power->id, &e->stats, e->stats.pos); //emit beacon
		}

		ai_power = e->stats.getAIPower(AI_POWER_JOIN_COMBAT);
		if (ai_power != NULL) {
			e->stats.cur_state = ENEMY_POWER;
			e->stats.activated_power = ai_power;
		}

		e->stats.join_combat = false;
	}

	// check exiting combat (player died or got too far away)
	if (e->stats.in_combat && hero_dist > JOIN_COMBAT_DIST && !e->stats.join_combat) {
		e->stats.in_combat = false;
	}

	// check exiting combat (player or enemy died)
	if (!e->stats.alive || !pc->stats.alive) {
		e->stats.in_combat = false;
	}

	// by default, the enemy pursues the hero directly
	pursue_pos.x = pc->stats.pos.x;
	pursue_pos.y = pc->stats.pos.y;
	target_dist = hero_dist;

}

/**
 * For now, fleeing is deterministically determined.
 * Borrows standard logic from BehaviorStandard::findTarget
 */
void BehaviorTF::decideFlee() {
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

std::array<int, NUM_ACTIONS> normalizePredictions(std::array<float, NUM_ACTIONS> preds, float min_pred, float max_pred) {
	/*
		Normalize tensorflow prediction and cast to positive integers to allow random draw of action
	*/
	std::array<int, NUM_ACTIONS> pred_ints;
	if(max_pred > min_pred) {
		for ( int i = 0; i < NUM_ACTIONS; i++) {
			pred_ints[i] = int( (preds[i] + min_pred) * 10000 );
		}
	}
	return pred_ints;
}

float BehaviorTF::normalCDF(float x) {
	/*
		Approximation to normal cdf
	*/

	// TODO(LP): Implement
	return x;
}

// TODO(Leo): can move to UTILS_MATH_H
std::array<float, 2> const BehaviorTF::distEntities(FPoint pos1, FPoint pos2) const {
/*
	Return x and y distance between two positions.
*/
	std::array<float, 2> dist = {{pos2.x - pos1.x, pos2.y - pos1.y}};
	return dist;
}

int BehaviorTF::flatPosition(float x, float y, int vision_dim) {
	// Return position in flattened vector from 2d map
	int a = int( floor(y) * vision_dim + floor(x) );
	int b = vision_dim * vision_dim - 1;
	return std::max( int( std::min(a , b) ), 0);
}

int BehaviorTF::flatRelativePosition(FPoint pos1, FPoint pos2, int vision_dim) {
/*
	Return index in flattened vision_dim x vision_dim grid centered at (dx1,dy1) of
	relative distance between (dx1,dy1) and (dx2, dy2)
*/
	std::array<float, 2> dist = distEntities(pos1, pos2);
	return flatPosition(vision_dim/2.0f + dist[0], vision_dim/2.0f + dist[1], vision_dim);
}

std::array<float, RELATIVE_OVERLAY_LENGTH> BehaviorTF::featureToRelativeOverlay(FEATURE feature, FPoint entity_pos) {
/*
	Return a flattened grid that contains values of feature_name in relative position on
	a VISION_DIM x VISION_DIM grid around entity_pos.
*/
	std::array<float, RELATIVE_OVERLAY_LENGTH> overlay{};

	for (unsigned int i=0; i < enemym->enemies.size(); i++) {

		int flat_pos = flatRelativePosition(entity_pos, enemym->enemies[i]->stats.pos, VISION_DIM);

		switch (feature) {
			case FEATURE::HP : {
				overlay[flat_pos] = enemym->enemies[i]->stats.hp;
				break;
			}
			default:
				logError("BehaviorTF: Feature %i not recognized for relative overlay.", static_cast<int>(feature));
		}
	}

	return overlay;
}

FPoint BehaviorTF::moveAction(ACTION action, FPoint start_pos) {
	/*
	  Resolve move actions.
	 */
	FPoint end_pos = start_pos;
	switch (action) {
		case ACTION::MOVE_NORTH: end_pos.y += 1.0f; break;
		case ACTION::MOVE_SOUTH: end_pos.y -= 1.0f; break;
		case ACTION::MOVE_EAST : end_pos.x -= 1.0f; break;
		case ACTION::MOVE_WEST : end_pos.x += 1.0f; break;
		case ACTION::NONE      : break;
	}
	return end_pos;
}

std::array<float, NUM_ACTIONS> BehaviorTF::featurizeAction(ACTION action) {
	/*
	  One hot encoding of actions.
	 */
	std::array<float, NUM_ACTIONS> action_encoding{};
	int index = static_cast<int>(action);
	action_encoding[index] = 1.0f;
	return action_encoding;
}

std::array<float, TENSOR_IN_LENGTH> BehaviorTF::getGameStateData(ACTION action) {
	/*
		Convert information in SharedGameResources to data array relative to entity e.

		Return: state data correspoding to current state + action (can be NONE)

		Data array consists of:
		  entity features ['stats.hp', 'stats.mp', 'stats.pos.x', 'stats.pos.y']
 			pc featrues ['stats.hp', 'stats.mp']
			distance (dx, dy) from entity to pc
			10 x 10 grid flattened of other entity hp
			one hot encoding of action
	*/

	std::array<float, TENSOR_IN_LENGTH> data;

	// resolve actions
	// FPoint new_pos = moveAction(action, e->stats.pos);
	// mdp model takes action as a feature instead
	FPoint new_pos = moveAction(ACTION::NONE, e->stats.pos);

	// entity features ['stats.hp', 'stats.mp', 'stats.pos.x', 'stats.pos.y']
	data[0] = e->stats.hp;
	data[1] = e->stats.mp;
	data[2] = new_pos.x;
	data[3] = new_pos.y;

	// pc features ['stats.hp', 'stats.mp']
	data[4] = pc->stats.hp;
	data[5] = pc->stats.mp;

	// printf("BehaviorTF: e hp %.2f\n", data[0]);
	// distance (dx, dy) from entity to pc
	std::array<float, 2> dist = distEntities(new_pos, pc->stats.pos);
	std::copy(dist.begin(), dist.end(), &data[6]);
	// data.insert(data.end(), dist.begin(), dist.end());
	// printf("BehaviorTF: (dx,dy) = %.2f , %.2f\n", data[6], data[7]);

	// 10 x 10 grid flattened of other entity hp
	std::array<float, RELATIVE_OVERLAY_LENGTH> allies_hp_overlay = featureToRelativeOverlay(FEATURE::HP, new_pos);
	std::copy(allies_hp_overlay.begin(), allies_hp_overlay.end(), &data[8]);
	// data.insert(data.end(), allies_hp_overlay.begin(), allies_hp_overlay.end());
	// printf("BehaviorTF: allies_hp_overlay[0] = %.2f = %.2f\n", allies_hp_overlay[0], data[8]);
	// printf("BehaviorTF: data len = %lu\n", data.size());

	std::array<float, NUM_ACTIONS> action_encoding = featurizeAction(action);
	std::copy(action_encoding.begin(), action_encoding.end(), &data[108]);

	return data;
}
