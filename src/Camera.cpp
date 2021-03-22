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

#include "Camera.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsMath.h"

Camera::Camera()
	: pos()
	, shake()
	, target()
	, prev_cam_target()
	, prev_cam_dx(0)
	, prev_cam_dy(0)
	, cam_threshold(eset->misc.camera_speed / 50.f)
	, shake_strength(8)
{
}

Camera::~Camera() {
}

void Camera::logic() {
	// gradulally move camera towards target

	float cam_delta = Utils::calcDist(pos, target);
	float cam_dx = (Utils::calcDist(FPoint(pos.x, target.y), target)) / eset->misc.camera_speed;
	float cam_dy = (Utils::calcDist(FPoint(target.x, pos.y), target)) / eset->misc.camera_speed;

	if (prev_cam_target.x == target.x && prev_cam_target.y == target.y) {
		// target hasn't changed

		if (cam_delta == 0 || cam_delta >= cam_threshold) {
			// camera is stationary or moving fast enough, so store the deltas
			prev_cam_dx = cam_dx;
			prev_cam_dy = cam_dy;
		}
		else if (cam_delta < cam_threshold) {
			if (cam_dx < prev_cam_dx || cam_dy < prev_cam_dy) {
				// maintain camera speed
				cam_dx = prev_cam_dx;
				cam_dy = prev_cam_dy;
			}
			else {
				// camera didn't get a chance to speed up, so set the minimum speed
				float b = fabsf(pos.x - target.x);
				float alpha = acosf(b / cam_delta);

				float fast_dx = cam_threshold * cosf(alpha);
				float fast_dy = cam_threshold * sinf(alpha);

				prev_cam_dx = fast_dx / eset->misc.camera_speed;
				prev_cam_dy = fast_dy / eset->misc.camera_speed;
			}
		}
	}
	else {
		// target changed, reset
		prev_cam_target = target;
		prev_cam_dx = 0;
		prev_cam_dy = 0;
	}

	// camera movement might overshoot its target, so compensate for that here
	if (pos.x < target.x) {
		pos.x += cam_dx;
		if (pos.x > target.x)
			pos.x = target.x;
	}
	else if (pos.x > target.x) {
		pos.x -= cam_dx;
		if (pos.x < target.x)
			pos.x = target.x;
	}
	if (pos.y < target.y) {
		pos.y += cam_dy;
		if (pos.y > target.y)
			pos.y = target.y;
	}
	else if (pos.y > target.y) {
		pos.y -= cam_dy;
		if (pos.y < target.y)
			pos.y = target.y;
	}

	// handle camera shaking timer
	shake_timer.tick();

	if (shake_timer.isEnd()) {
		shake.x = pos.x;
		shake.y = pos.y;
	}
	else {
		shake.x = pos.x + static_cast<float>((rand() % (shake_strength * 2)) - shake_strength) * 0.0078125f;
		shake.y = pos.y + static_cast<float>((rand() % (shake_strength * 2)) - shake_strength) * 0.0078125f;
	}
}

void Camera::setTarget(const FPoint& _target) {
	target = _target;
}

void Camera::warpTo(const FPoint& _target) {
	pos = shake = target = prev_cam_target = _target;
	shake_timer.reset(Timer::END);
	prev_cam_dx = 0;
	prev_cam_dy = 0;
}

