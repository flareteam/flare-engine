/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2012-2015 Justin Jacobs

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

#include "Animation.h"
#include "Avatar.h"
#include "BehaviorAlly.h"
#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "Enemy.h"
#include "EntityBehavior.h"
#include "LootManager.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsMath.h"

#include <math.h>
#include <cassert>

Enemy::Enemy() : Entity() {
	eb = NULL;
}

Enemy::Enemy(const Enemy& e)
	: Entity(e)
{
	eb = new EntityBehavior(this); // Putting a 'this' into the init list will make MSVS complain, hence it's in the body of the ctor
}

Enemy& Enemy::operator=(const Enemy& e) {
	if (this == &e)
		return *this;

	Entity::operator=(e);
	eb = new EntityBehavior(this);

	return *this;
}

/**
 * logic()
 * Handle a single frame.  This includes:
 * - move the enemy based on AI % chances
 * - calculate the next frame of animation
 */
void Enemy::logic() {

	eb->logic();

	//need to check whether the enemy was converted here
	//cant do it in behaviour because the behaviour object would be replaced by this
	if(stats.effects.convert != stats.converted) {
		stats.converted = !stats.converted;
		stats.hero_ally = !stats.hero_ally;
		if (stats.convert_status != 0) {
			camp->setStatus(stats.convert_status);
		}
	}

	return;
}

/**
 * getRender()
 * Map objects need to be drawn in Z order, so we allow a parent object (GameEngine)
 * to collect all mobile sprites each frame.
 */
Renderable Enemy::getRender() {
	Renderable r = activeAnimation->getCurrentFrame(stats.direction);
	r.map_pos.x = stats.pos.x;
	r.map_pos.y = stats.pos.y;
	if (stats.hp > 0) {
		if (stats.hero_ally)
			r.type = Renderable::TYPE_ALLY;
		else if (stats.in_combat)
			r.type = Renderable::TYPE_ENEMY;
	}
	return r;
}

Enemy::~Enemy() {
	delete eb;
}

