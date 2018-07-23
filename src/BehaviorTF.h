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
enum FEATURE { HP = 0 };

// entity featurizes game state in a VISION_DIM x VISION_DIM grid around them
const int VISION_DIM = 10;
const int RELATIVE_OVERLAY_LENGTH = 100; // VISION_DIM x VISION_DIM

//TODO(LEO): remove test tf
// class WidgetTooltip;
// class TooltipData;
class TensorFlowInterface;

class BehaviorTF : public BehaviorStandard {
public:
	explicit BehaviorTF(Enemy *_e);
	virtual ~BehaviorTF();

	void logic();
protected:
private:
	// TODO(Leo): can move to UTILS_MATH_H
	std::array<float, 2> const distEntities(float x1, float y1, float x2, float y2) const;
	int flatPosition(float x, float y, int vision_dim);
	int flatRelativePosition(float x1, float y1, float x2, float y2, int vision_dim);
	std::array<float, RELATIVE_OVERLAY_LENGTH> featureToRelativeOverlay(FEATURE feature, float x, float y);
	std::array<float, TENSOR_IN_LENGTH> getGameStateData(ACTION action = ACTION::NONE);

	TensorFlowInterface* tf_model;

	//TODO(LEO): remove test tf
	// WidgetTooltip *tip;
	// TooltipData *tip_buf;
};

#endif // BEHAVIORTF_H
