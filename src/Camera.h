/*
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
 * class Camera
 *
 * Controls the viewport that moves around the map, usually focused on the player
 *
 */

#ifndef CAMERA_H
#define CAMERA_H

#include "CommonIncludes.h"
#include "Utils.h"

class Camera {
public:
	Camera();
	~Camera();

	void logic();
	void setTarget(const FPoint& _target);
	void warpTo(const FPoint& _target);

	FPoint pos;
	FPoint shake;
	Timer shake_timer;

private:
	FPoint target;
	FPoint prev_cam_target;

	float prev_cam_dx;
	float prev_cam_dy;

	float cam_threshold;
	int shake_strength;
};

#endif
