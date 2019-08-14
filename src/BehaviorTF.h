/*
Copyright © 2013 Ryan Dansie

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


#ifndef BEHAVIORTF_H
#define BEHAVIORTF_H

#include "BehaviorStandard.h"
#include "TensorFlowInterface.h"

enum class ACTION {
	NONE       = 0,
	MOVE_NORTH = 1,
	MOVE_EAST  = 2,
	MOVE_SOUTH = 3,
	MOVE_WEST  = 4
};
const int NUM_ACTIONS = 5;
enum FEATURE { HP = 0 };

// entity featurizes game state in a VISION_DIM x VISION_DIM grid around them
const int VISION_DIM = 10;
const int RELATIVE_OVERLAY_LENGTH = 100; // VISION_DIM x VISION_DIM

class TensorFlowInterface;

class BehaviorTF : public BehaviorStandard {
public:
	explicit BehaviorTF(Enemy *_e);
	virtual ~BehaviorTF();

	void logic();
protected:
private:
	void decideDirection();
	void decideCombat();
	void decideFlee();

	ACTION chooseAction(std::array<int, NUM_ACTIONS> pred_ints);
	FPoint moveAction(ACTION action, FPoint start_pos);

	// TODO(Leo): can move to UTILS_MATH_H
	std::array<float, 2> const distEntities(FPoint pos1, FPoint pos2) const;
	int flatPosition(float x, float y, int vision_dim);
	int flatRelativePosition(FPoint pos1, FPoint pos2, int vision_dim);
	float normalCDF(float x);

	std::array<float, NUM_ACTIONS> featurizeAction(ACTION action);
	std::array<float, RELATIVE_OVERLAY_LENGTH> featureToRelativeOverlay(FEATURE feature, FPoint entity_pos);
	std::array<float, TENSOR_IN_LENGTH> getGameStateData(ACTION action = ACTION::NONE);

	std::array<int, NUM_ACTIONS> normalizePredictions(std::array<float, NUM_ACTIONS> preds, float min_pred, float max_pred);

	TensorFlowInterface* tf_model;

};

#endif // BEHAVIORTF_H
