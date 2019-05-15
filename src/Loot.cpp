/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013-2016 Justin Jacobs

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
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "Loot.h"
#include "SharedResources.h"
#include "WidgetTooltip.h"

Loot::Loot()
	: gfx("")
	, animation(NULL)
	, wtip(new WidgetTooltip())
	, tip_visible(false)
	, dropped_by_hero(false)
	, on_ground(false)
	, sound_played(false) {
	tip.clear();
}

Loot::Loot(const Loot &other)
	: gfx("")
	, animation(NULL)
	, wtip(new WidgetTooltip())
{
	*this = other;
}

// The assignment operator mainly used in internal vector managing,
// e.g. in vector::erase()
Loot& Loot::operator= (const Loot &other) {
	if (this == &other)
		return *this;

	if (gfx != "")
		anim->decreaseCount(gfx);
	delete animation;
	animation = NULL;

	loadAnimation(other.gfx);
	if (animation)
		animation->syncTo(other.animation);

	stack.item = other.stack.item;
	stack.quantity = other.stack.quantity;
	pos.x = other.pos.x;
	pos.y = other.pos.y;
	tip = other.tip;
	wtip = new WidgetTooltip();
	tip_visible = other.tip_visible;
	dropped_by_hero = other.dropped_by_hero;
	on_ground = other.on_ground;
	sound_played = other.sound_played;

	return *this;
}

void Loot::loadAnimation(const std::string& _gfx) {
	gfx = _gfx;
	if (gfx != "") {
		anim->increaseCount(gfx);
		AnimationSet *as = anim->getAnimationSet(gfx);
		animation = as->getAnimation("");
	}
	else {
		animation = 0;
	}
}

bool Loot::isFlying() {
	return animation && !animation->isLastFrame();
}

Loot::~Loot() {
	if (gfx != "")
		anim->decreaseCount(gfx);
	delete animation;
	delete wtip;
}

