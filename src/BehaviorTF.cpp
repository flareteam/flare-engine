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
#include "Settings.h"
#include "SharedGameResources.h"
#include "UtilsMath.h"
#include "TensorFlowInterface.h"

#include <cmath>
#include <chrono>
#include <thread>

//TODO(LP): remove test tf
#include "SharedResources.h"
#include "CombatText.h"
std::array<float, 5> POS_OFFETS_X {{0.0f, 0.0f, 1.0f, 0.0f, -1.0f}};
std::array<float, 5> POS_OFFETS_Y {{0.0f, 1.0f, 0.0f, -1.0f, 0.0f}};

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

void BehaviorTF::logic() {
	/*
		For first pass, compute probabilities of each move direction, and render on top of entity.
	*/

	// use turn_ticks to render only after every enemy turn
	if( e->stats.turn_ticks == 0) {
		std::array<int, 5> pred_ints;
		int max_pred_int = 0;

		for ( int action_int = 0; action_int <= 4; action_int++ )
		{
			 ACTION action = static_cast<ACTION>(action_int);
			 std::array<float, TENSOR_IN_LENGTH> game_data = getGameStateData(action);
			 float * pred = tf_model->predict(game_data);
			 int pred_int = int( (*pred) * 10000 );
			 pred_ints[action_int] = pred_int;
			 max_pred_int = std::max(max_pred_int, pred_int);
		}

		// render pred above entity
		CombatText *combat_text = comb;

		for ( unsigned long i=0; i<=pred_ints.size(); i++) {
		  int displaytype = COMBAT_MESSAGE_MISS;
		  if ( pred_ints[i] == max_pred_int ) {
			  displaytype = COMBAT_MESSAGE_TAKEDMG;
		  }
		  FPoint pos = e->stats.pos;
		  pos.x += POS_OFFETS_X[i];
		  pos.y += POS_OFFETS_Y[i];
		  combat_text->addInt(pred_ints[i], pos, displaytype);
	  }
  }
}

// TODO(Leo): can move to UTILS_MATH_H
std::array<float, 2> const BehaviorTF::distEntities(float x1, float y1, float x2, float y2) const {
/*
	Return x and y distance between two positions.
*/
	std::array<float, 2> dist = {{x2 - x1, y2 - y1}};
	return dist;
}

int BehaviorTF::flatPosition(float x, float y, int vision_dim) {
	// Return position in flattened vector from 2d map
	int a = int( floor(y) * vision_dim + floor(x) );
	int b = vision_dim * vision_dim;
	return std::max( int( std::min(a , b) ), 0);
}

int BehaviorTF::flatRelativePosition(float x1, float y1, float x2, float y2, int vision_dim) {
/*
	Return index in flattened vision_dim x vision_dim grid centered at (dx1,dy1) of
	relative distance between (dx1,dy1) and (dx2, dy2)
*/
	std::array<float, 2> dist = distEntities(x1, y1, x2, y2);
	return flatPosition(vision_dim/2.0f + dist[0], vision_dim/2.0f + dist[1], vision_dim);
}

std::array<float, RELATIVE_OVERLAY_LENGTH> BehaviorTF::featureToRelativeOverlay(FEATURE feature, float x, float y) {
/*
	Return a flattened grid that contains values of feature_name in relative position on
	a VISION_DIM x VISION_DIM grid around (x, y).
*/
	std::array<float, RELATIVE_OVERLAY_LENGTH> overlay{};

	for (unsigned int i=0; i < enemym->enemies.size(); i++) {

		int flat_pos = flatRelativePosition(x, y,
			enemym->enemies[i]->stats.pos.x, enemym->enemies[i]->stats.pos.y,
			VISION_DIM
		);

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

std::array<float, TENSOR_IN_LENGTH> BehaviorTF::getGameStateData(ACTION action) {
	/*
		Convert information in SharedGameResources to data array relative to entity e.

		Return: state data correspoding to current state + action (can be NONE)

		Data array consists of:
		  entity features ['stats.hp', 'stats.mp', 'stats.pos.x', 'stats.pos.y']
 			pc featrues ['stats.hp', 'stats.mp']
			distance (dx, dy) from entity to pc
			10 x 10 grid flattened of other entity hp
	*/

	std::array<float, TENSOR_IN_LENGTH> data;

	// resolve actions
	float e_x = e->stats.pos.x;
	float e_y = e->stats.pos.y;

	switch (action) {
		case ACTION::MOVE_NORTH: e_y += 1.0f; break;
		case ACTION::MOVE_SOUTH: e_y -= 1.0f; break;
		case ACTION::MOVE_EAST : e_x -= 1.0f; break;
		case ACTION::MOVE_WEST : e_x += 1.0f; break;
		case ACTION::NONE      : break;
	}

	// entity features ['stats.hp', 'stats.mp', 'stats.pos.x', 'stats.pos.y']
	data[0] = e->stats.hp;
	data[1] = e->stats.mp;
	data[2] = e_x;
	data[3] = e_y;

	// pc features ['stats.hp', 'stats.mp']
	data[4] = pc->stats.hp;
	data[5] = pc->stats.mp;

	// printf("BehaviorTF: e hp %.2f\n", data[0]);

	// distance (dx, dy) from entity to pc
	std::array<float, 2> dist = distEntities(e_x, e_y, pc->stats.pos.x, pc->stats.pos.y);
	std::copy(dist.begin(), dist.end(), &data[6]);
	// data.insert(data.end(), dist.begin(), dist.end());

	// printf("BehaviorTF: (dx,dy) = %.2f , %.2f\n", data[6], data[7]);

	// 10 x 10 grid flattened of other entity hp
	std::array<float, RELATIVE_OVERLAY_LENGTH> allies_hp_overlay = featureToRelativeOverlay(FEATURE::HP, e_x, e_y);
	std::copy(allies_hp_overlay.begin(), allies_hp_overlay.end(), &data[8]);
	// data.insert(data.end(), allies_hp_overlay.begin(), allies_hp_overlay.end());

	// printf("BehaviorTF: allies_hp_overlay[0] = %.2f = %.2f\n", allies_hp_overlay[0], data[8]);

	// printf("BehaviorTF: data len = %lu\n", data.size());
	return data;
}
