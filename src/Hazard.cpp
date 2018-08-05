/*
Copyright © 2011-2012 Clint Bellanger
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
#include "PowerManager.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <cmath>

Hazard::Hazard(MapCollision *_collider)
	: active(true)
	, remove_now(false)
	, hit_wall(false)
	, relative_pos(false)
	, sfx_hit_played(false)
	, dmg_min(0)
	, dmg_max(0)
	, crit_chance(0)
	, accuracy(0)
	, source_type(0)
	, base_speed(0)
	, lifespan(1)
	, animationKind(0)
	, delay_frames(0)
	, angle(0)
	, src_stats(NULL)
	, power(NULL)
	, power_index(0)
	, parent(NULL)
	, collider(_collider)
	, activeAnimation(NULL)
	, animation_name("")
{
}

Hazard::Hazard(const Hazard& other) {
	activeAnimation = NULL;
	*this = other;
}

Hazard& Hazard::operator=(const Hazard& other) {
	if (this == &other)
		return *this;

	active = other.active;
	remove_now = other.remove_now;
	hit_wall = other.hit_wall;
	relative_pos = other.relative_pos;
	sfx_hit_played = other.sfx_hit_played;

	dmg_min = other.dmg_min;
	dmg_max = other.dmg_max;
	crit_chance = other.crit_chance;
	accuracy = other.accuracy;
	source_type = other.source_type;
	base_speed = other.base_speed;
	lifespan = other.lifespan;
	animationKind = other.animationKind;
	delay_frames = other.delay_frames;
	angle = other.angle;

	src_stats = other.src_stats;
	power = other.power;
	power_index = other.power_index;

	pos = other.pos;
	speed = other.speed;
	pos_offset = other.pos_offset;

	parent = other.parent;
	children = other.children;

	if (!other.animation_name.empty()) {
		animation_name = other.animation_name;
		loadAnimation(animation_name);
	}

	collider = other.collider;
	entitiesCollided = other.entitiesCollided;

	return (*this);
}

Hazard::~Hazard() {
	if (!parent && !children.empty()) {
		// make the next child the parent for the existing children
		Hazard* new_parent = children[0];
		new_parent->parent = NULL;

		for (size_t i = 1; i < children.size(); ++i) {
			children[i]->parent = new_parent;
			new_parent->children.push_back(children[i]);
		}

		for (size_t i = 0; i < entitiesCollided.size(); ++i) {
			new_parent->addEntity(entitiesCollided[i]);
		}
	}
	else if (parent) {
		// remove this hazard from the parent's list of children
		for (size_t i = 0; i < parent->children.size(); ++i) {
			if (parent->children[i] == this) {
				parent->children.erase(parent->children.begin() + i);
				break;
			}
		}
	}

	if (!animation_name.empty()) {
		anim->decreaseCount(animation_name);
	}

	if (activeAnimation) {
		delete activeAnimation;
	}

	anim->cleanUp();
}

void Hazard::logic() {

	// if the hazard is on delay, take no action
	if (delay_frames > 0) {
		delay_frames--;
		return;
	}

	// handle tickers
	if (lifespan > 0) lifespan--;

	if (power->expire_with_caster && !src_stats->alive)
		lifespan = 0;

	if (activeAnimation)
		activeAnimation->advanceFrame();

	// handle movement
	bool check_collide = false;
	if (!(speed.x == 0 && speed.y == 0)) {
		pos.x += speed.x;
		pos.y += speed.y;
		check_collide = true;
	}
	else if (!(pos_offset.x == 0 && pos_offset.y == 0)) {
		pos.x = src_stats->pos.x - pos_offset.x;
		pos.y = src_stats->pos.y - pos_offset.y;
		check_collide = true;
	}
	else if (relative_pos) {
		pos.x = src_stats->pos.x;
		pos.y = src_stats->pos.y;
	}

	if (check_collide) {
		// very simplified collider, could skim around corners
		// or even pass through thin walls if speed > tilesize
		if (!collider->isValidPosition(pos.x, pos.y, power->movement_type, MapCollision::COLLIDE_NO_ENTITY)) {

			hit_wall = true;

			if (power->wall_reflect) {
				this->reflect();
			}
			else {
				lifespan = 0;
				if (collider->isOutsideMap(pos.x, pos.y))
					remove_now = true;
		    }
		}
	}
}

void Hazard::reflect() {
  if (!collider->isWall(pos.x - speed.x, pos.y)) {
    speed.x *= -1;
	pos.x += speed.x;
  }
  else if (!collider->isWall(pos.x, pos.y - speed.y)) {
    speed.y *= -1;
	pos.y += speed.y;
  }
  else {
    speed.x *= -1;
	speed.y *= -1;
	pos.x += speed.x;
	pos.y += speed.y;
  }

  if (power->directional)
	animationKind = Utils::calcDirection(pos.x, pos.y, pos.x + speed.x, pos.y + speed.y);
}

void Hazard::loadAnimation(const std::string &s) {
	if (!animation_name.empty()) {
		anim->decreaseCount(animation_name);
	}
	if (activeAnimation) {
		delete activeAnimation;
	}
	activeAnimation = NULL;
	animation_name = s;
	if (animation_name != "") {
		anim->increaseCount(animation_name);
		AnimationSet *animationSet = anim->getAnimationSet(animation_name);
		activeAnimation = animationSet->getAnimation("");
	}

	anim->cleanUp();
}

bool Hazard::isDangerousNow() {
	return active && (delay_frames == 0) &&
		   ( (activeAnimation != NULL && activeAnimation->isActiveFrame())
			 || activeAnimation == NULL);
}

bool Hazard::hasEntity(Entity *ent) {
	if (power->multihit) {
		return false;
	}

	if (parent) {
		return parent->hasEntity(ent);
	}
	else {
		for(std::vector<Entity*>::iterator it = entitiesCollided.begin(); it != entitiesCollided.end(); ++it)
			if(*it == ent) return true;
		return false;
	}
}

void Hazard::addEntity(Entity *ent) {
	if (parent) {
		parent->addEntity(ent);
	}
	else {
		entitiesCollided.push_back(ent);
	}
}

void Hazard::addRenderable(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	if (delay_frames == 0 && activeAnimation) {
		Renderable re = activeAnimation->getCurrentFrame(animationKind);
		re.map_pos.x = pos.x;
		re.map_pos.y = pos.y;
		re.prio = (power->on_floor ? 0 : 2);
		(power->on_floor ? r_dead : r).push_back(re);
	}
}

void Hazard::setAngle(const float& _angle) {
	angle = _angle;
	while (angle >= static_cast<float>(M_PI)*2) angle -= static_cast<float>(M_PI)*2;
	while (angle < 0.0) angle += static_cast<float>(M_PI)*2;

	speed.x = base_speed * cosf(angle);
	speed.y = base_speed * sinf(angle);

	if (power->directional)
		animationKind = Utils::calcDirection(pos.x, pos.y, pos.x + speed.x, pos.y + speed.y);
}
