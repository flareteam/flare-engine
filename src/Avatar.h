/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Henrik Andersson

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
 * class Avatar
 *
 * Contains logic and rendering routines for the player avatar.
 */

#pragma once
#ifndef AVATAR_H
#define AVATAR_H

#include "CommonIncludes.h"
#include "Entity.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Utils.h"

using namespace std;

class Entity;
class Hazard;
class StatBlock;

/**
 * Avatar State enum
 */
enum AvatarState {
	AVATAR_STANCE = 0,
	AVATAR_RUN = 1,
	AVATAR_BLOCK = 2,
	AVATAR_HIT = 3,
	AVATAR_DEAD = 4,
	AVATAR_ATTACK = 5,
};

class Layer_gfx {
public:
	std::string gfx;
	std::string type;
	Layer_gfx()
		: gfx("")
		, type("") {
	}
};

class Step_sfx {
public:
	std::string id;
	std::vector<std::string> steps;
};

class Avatar : public Entity {
private:
	bool lockAttack;

	std::vector<Step_sfx> step_def;

	std::vector<SoundManager::SoundID> sound_steps;

	void setAnimation(std::string name);
	std::vector<AnimationSet*> animsets; // hold the animations for all equipped items in the right order of drawing.
	std::vector<Animation*> anims; // hold the animations for all equipped items in the right order of drawing.

	short body;

	bool transform_triggered;
	std::string last_transform;
	int getUntransformPower();

	//variables for patfinding
	vector<FPoint> path;
	FPoint prev_target;

	void handlePower(int actionbar_power);

	// visible power target
	FPoint target_pos;
	bool target_visible;
	Animation *target_anim;
	AnimationSet *target_animset;

	bool lock_cursor; // keeps the attacking cursor while holding down the power key/button

public:
	Avatar();
	~Avatar();

	void init();
	void loadLayerDefinitions();
	std::vector<std::string> layer_reference_order;
	std::vector<std::vector<unsigned> > layer_def;
	void loadGraphics(std::vector<Layer_gfx> _img_gfx);
	void loadStepFX(const std::string& stepname);

	void logic(int actionbar_power, bool restrictPowerUse);
	bool pressing_move();
	void set_direction();
	std::string log_msg;

	std::string attack_anim;

	// transformation handling
	void transform();
	void untransform();
	bool setPowers;
	bool revertPowers;
	int untransform_power;
	StatBlock *hero_stats;
	StatBlock *charmed_stats;

	virtual void resetActiveAnimation();
	virtual Renderable getRender() {
		return Renderable();
	}
	void addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	// vars
	Hazard *haz;
	int current_power;
	FPoint act_target;
	bool drag_walking;
	bool newLevelNotification;
	bool respawn;
	bool close_menus;
	bool allow_movement;
	std::vector<int> hero_cooldown;
};

#endif

