/*
Copyright © 2011-2012 Clint Bellanger

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
 * class Hazard
 *
 * Stand-alone object that can harm the hero or creatures
 * These are generated whenever something makes any attack
 */

#include "Animation.h"
#include "AnimationSet.h"
#include "AnimationManager.h"
#include "Hazard.h"
#include "MapCollision.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <cmath>

Hazard::Hazard(MapCollision *_collider)
	: collider(_collider)
	, activeAnimation(NULL)
	, animation_name("")
	, src_stats(NULL)
	, dmg_min(0)
	, dmg_max(0)
	, crit_chance(0)
	, accuracy(0)
	, source_type(0)
	, target_party(false)
	, pos()
	, speed()
	, base_speed(0)
	, angle(0)
	, base_lifespan(1)
	, lifespan(1)
	, radius(0)
	, power_index(0)
	, animationKind(0)
	, on_floor(false)
	, delay_frames(0)
	, complete_animation(false)
	, multitarget(false)
	, active(true)
	, remove_now(false)
	, hit_wall(false)
	, hp_steal(0)
	, mp_steal(0)
	, trait_armor_penetration(false)
	, trait_crits_impaired(0)
	, trait_elemental(-1)
	, beacon(false)
	, missile(false)
	, directional(false)
	, post_power(0)
	, wall_power(0) {
}

Hazard::~Hazard() {
	if (activeAnimation) {
		anim->decreaseCount(animation_name);
		delete activeAnimation;
	}
}

void Hazard::logic() {

	// if the hazard is on delay, take no action
	if (delay_frames > 0) {
		delay_frames--;
		return;
	}

	// handle tickers
	if (lifespan > 0) lifespan--;

	if (activeAnimation)
		activeAnimation->advanceFrame();

	// handle movement
	if (!(speed.x == 0 && speed.y == 0)) {
		pos.x += speed.x;
		pos.y += speed.y;

		// very simplified collider, could skim around corners
		// or even pass through thin walls if speed > tilesize
		if (collider->is_wall(pos.x, pos.y)) {
			lifespan = 0;
			hit_wall = true;

			if (collider->is_outside_map(int(pos.x), int(pos.y)))
				remove_now = true;
		}
	}
}

void Hazard::loadAnimation(std::string &s) {
	if (activeAnimation) {
		anim->decreaseCount(animation_name);
		delete activeAnimation;
		activeAnimation = 0;
	}
	animation_name = s;
	if (animation_name != "") {
		anim->increaseCount(animation_name);
		AnimationSet *animationSet = anim->getAnimationSet(animation_name);
		activeAnimation = animationSet->getAnimation();
	}
}

bool Hazard::isDangerousNow() {
	return active && (delay_frames == 0) &&
		   ( (activeAnimation != NULL && activeAnimation->isActiveFrame())
			 || activeAnimation == NULL);
}

bool Hazard::hasEntity(Entity *ent) {
	for(std::vector<Entity*>::iterator it = entitiesCollided.begin(); it != entitiesCollided.end(); ++it)
		if(*it == ent) return true;
	return false;
}

void Hazard::addEntity(Entity *ent) {
	entitiesCollided.push_back(ent);
}

void Hazard::addRenderable(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	if (delay_frames == 0 && activeAnimation) {
		Renderable re = activeAnimation->getCurrentFrame(animationKind);
		re.map_pos.x = pos.x;
		re.map_pos.y = pos.y;
		(on_floor ? r_dead : r).push_back(re);
	}
}

void Hazard::setAngle(const float& _angle) {
	angle = _angle;
	while (angle >= M_PI*2) angle -= M_PI*2;
	while (angle < 0.0) angle += M_PI*2;

	speed.x = base_speed * cos(angle);
	speed.y = base_speed * sin(angle);

	if (directional)
		animationKind = calcDirection(pos.x, pos.y, pos.x + speed.x, pos.y + speed.y);
}
