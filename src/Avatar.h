/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Henrik Andersson
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
 * class Avatar
 *
 * Contains logic and rendering routines for the player avatar.
 */

#ifndef AVATAR_H
#define AVATAR_H

#include "CommonIncludes.h"
#include "Entity.h"
#include "PowerManager.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Utils.h"

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
	void loadLayerDefinitions();
	bool pressing_move();
	void set_direction();
	void transform();
	void untransform();
	void setAnimation(std::string name);

	bool lockAttack;

	std::vector<Step_sfx> step_def;

	std::vector<SoundManager::SoundID> sound_steps;

	std::vector<AnimationSet*> animsets; // hold the animations for all equipped items in the right order of drawing.
	std::vector<Animation*> anims; // hold the animations for all equipped items in the right order of drawing.

	short body;

	bool transform_triggered;
	std::string last_transform;

	bool lock_cursor; // keeps the attacking cursor while holding down the power key/button

public:
	Avatar();
	~Avatar();

	void init();
	void loadGraphics(std::vector<Layer_gfx> _img_gfx);
	void loadStepFX(const std::string& stepname);

	void logic(std::vector<ActionData> &action_queue, bool restrict_power_use, bool npc);

	// transformation handling
	bool isTransforming() {
		return transform_triggered;
	}
	void checkTransform();

	void addRenders(std::vector<Renderable> &r);

	virtual void resetActiveAnimation();
	virtual Renderable getRender() {
		return Renderable();
	}

	std::vector<std::string> layer_reference_order;
	std::vector<std::vector<unsigned> > layer_def;

	std::string log_msg;

	std::string attack_anim;
	bool setPowers;
	bool revertPowers;
	int untransform_power;
	StatBlock *hero_stats;
	StatBlock *charmed_stats;
	FPoint transform_pos;
	std::string transform_map;

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
	std::vector<int> power_cast_ticks;
	std::vector<int> power_cast_duration;
	FPoint enemy_pos; // positon of the highlighted enemy
};

#endif

