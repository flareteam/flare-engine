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
#include "SharedResources.h"
#include "StatBlock.h"
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
	, pos_offset()
	, relative_pos(false)
	, base_speed(0)
	, angle(0)
	, base_lifespan(1)
	, lifespan(1)
	, radius(0)
	, power_index(0)
	, movement_type(MOVEMENT_FLYING)
	, animationKind(0)
	, on_floor(false)
	, delay_frames(0)
	, complete_animation(false)
	, multitarget(false)
	, active(true)
	, multihit(false)
	, expire_with_caster(false)
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
	, post_power_chance(100)
	, wall_power(0)
	, wall_power_chance(100)
	, wall_reflect(false)
	, target_movement_normal(true)
	, target_movement_flying(true)
	, target_movement_intangible(true)
	, walls_block_aoe(false)
	, sfx_hit(0)
	, sfx_hit_enable(false)
	, sfx_hit_played(false)
	, parent(NULL)
	, script_trigger(-1)
	, script("") {
}

Hazard::Hazard(const Hazard& other) {
	collider = other.collider;
	entitiesCollided = other.entitiesCollided;

	if (!other.animation_name.empty()) {
		animation_name = other.animation_name;
		loadAnimation(animation_name);
	}

	src_stats = other.src_stats;

	dmg_min = other.dmg_min;
	dmg_max = other.dmg_max;
	crit_chance = other.crit_chance;
	accuracy = other.accuracy;
	source_type = other.source_type;
	target_party = other.target_party;

	pos = other.pos;
	speed = other.speed;
	pos_offset = other.pos_offset;
	relative_pos = other.relative_pos;
	base_speed = other.base_speed;
	angle = other.angle;
	base_lifespan = other.base_lifespan;
	lifespan = other.lifespan;
	radius = other.radius;
	power_index = other.power_index;
	movement_type = other.movement_type;

	animationKind = other.animationKind;

	on_floor = other.on_floor;
	delay_frames = other.delay_frames;
	complete_animation = other.complete_animation;

	multitarget = other.multitarget;
	active = other.active;

	multihit = other.multihit;
	expire_with_caster = other.expire_with_caster;
	remove_now = other.remove_now;
	hit_wall = other.hit_wall;

	hp_steal = other.hp_steal;
	mp_steal = other.mp_steal;

	trait_armor_penetration = other.trait_armor_penetration;
	trait_crits_impaired = other.trait_crits_impaired;
	trait_elemental = other.trait_elemental;
	beacon = other.beacon;
	missile = other.missile;
	directional = other.directional;

	post_power = other.post_power;
	post_power_chance = other.post_power_chance;
	wall_power = other.wall_power;
	wall_power_chance = other.wall_power_chance;

	wall_reflect = other.wall_reflect;

	target_movement_normal = other.target_movement_normal;
	target_movement_flying = other.target_movement_flying;
	target_movement_intangible = other.target_movement_intangible;

	walls_block_aoe = other.walls_block_aoe;

	sfx_hit = other.sfx_hit;
	sfx_hit_enable = sfx_hit_enable;
	sfx_hit_played = sfx_hit_played;

	parent = other.parent;
	children = other.children;

	script_trigger = other.script_trigger;
	script = other.script;
}

Hazard& Hazard::operator=(const Hazard& other) {
	collider = other.collider;
	entitiesCollided = other.entitiesCollided;

	if (!other.animation_name.empty()) {
		animation_name = other.animation_name;
		loadAnimation(animation_name);
	}

	src_stats = other.src_stats;

	dmg_min = other.dmg_min;
	dmg_max = other.dmg_max;
	crit_chance = other.crit_chance;
	accuracy = other.accuracy;
	source_type = other.source_type;
	target_party = other.target_party;

	pos = other.pos;
	speed = other.speed;
	pos_offset = other.pos_offset;
	relative_pos = other.relative_pos;
	base_speed = other.base_speed;
	angle = other.angle;
	base_lifespan = other.base_lifespan;
	lifespan = other.lifespan;
	radius = other.radius;
	power_index = other.power_index;
	movement_type = other.movement_type;

	animationKind = other.animationKind;

	on_floor = other.on_floor;
	delay_frames = other.delay_frames;
	complete_animation = other.complete_animation;

	multitarget = other.multitarget;
	active = other.active;

	multihit = other.multihit;
	expire_with_caster = other.expire_with_caster;
	remove_now = other.remove_now;
	hit_wall = other.hit_wall;

	hp_steal = other.hp_steal;
	mp_steal = other.mp_steal;

	trait_armor_penetration = other.trait_armor_penetration;
	trait_crits_impaired = other.trait_crits_impaired;
	trait_elemental = other.trait_elemental;
	beacon = other.beacon;
	missile = other.missile;
	directional = other.directional;

	post_power = other.post_power;
	post_power_chance = other.post_power_chance;
	wall_power = other.wall_power;
	wall_power_chance = other.wall_power_chance;

	wall_reflect = other.wall_reflect;

	target_movement_normal = other.target_movement_normal;
	target_movement_flying = other.target_movement_flying;
	target_movement_intangible = other.target_movement_intangible;

	walls_block_aoe = other.walls_block_aoe;

	sfx_hit = other.sfx_hit;
	sfx_hit_enable = sfx_hit_enable;
	sfx_hit_played = sfx_hit_played;

	parent = other.parent;
	children = other.children;

	script_trigger = other.script_trigger;
	script = other.script;

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

	if (expire_with_caster && !src_stats->alive)
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
		if (!collider->is_valid_position(pos.x, pos.y, movement_type, false, false)) {

			hit_wall = true;

			if (wall_reflect) {
				this->reflect();
			}
			else {
				lifespan = 0;
				if (collider->is_outside_map(int(pos.x), int(pos.y)))
					remove_now = true;
		    }
		}
	}
}

void Hazard::reflect() {
  if (!collider->is_wall(pos.x - speed.x, pos.y)) {
    speed.x *= -1;
	pos.x += speed.x;
  }
  else if (!collider->is_wall(pos.x, pos.y - speed.y)) {
    speed.y *= -1;
	pos.y += speed.y;
  }
  else {
    speed.x *= -1;
	speed.y *= -1;
	pos.x += speed.x;
	pos.y += speed.y;
  }

  if (directional)
	animationKind = calcDirection(pos.x, pos.y, pos.x + speed.x, pos.y + speed.y);
}

void Hazard::loadAnimation(const std::string &s) {
	if (activeAnimation) {
		anim->decreaseCount(animation_name);
		delete activeAnimation;
		activeAnimation = 0;
	}
	animation_name = s;
	if (animation_name != "") {
		anim->increaseCount(animation_name);
		AnimationSet *animationSet = anim->getAnimationSet(animation_name);
		activeAnimation = animationSet->getAnimation("");
	}
}

bool Hazard::isDangerousNow() {
	return active && (delay_frames == 0) &&
		   ( (activeAnimation != NULL && activeAnimation->isActiveFrame())
			 || activeAnimation == NULL);
}

bool Hazard::hasEntity(Entity *ent) {
	if (multihit) {
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
		re.prio = (on_floor ? 0 : 2);
		(on_floor ? r_dead : r).push_back(re);
	}
}

void Hazard::setAngle(const float& _angle) {
	angle = _angle;
	while (angle >= static_cast<float>(M_PI)*2) angle -= static_cast<float>(M_PI)*2;
	while (angle < 0.0) angle += static_cast<float>(M_PI)*2;

	speed.x = base_speed * cosf(angle);
	speed.y = base_speed * sinf(angle);

	if (directional)
		animationKind = calcDirection(pos.x, pos.y, pos.x + speed.x, pos.y + speed.y);
}
