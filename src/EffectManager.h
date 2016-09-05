/*
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
 * class EffectManager
 *
 * Holds the collection of hazards (active attacks, spells, etc) and handles group operations
 */

#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H

#include "CommonIncludes.h"
#include "Hazard.h"
#include "SharedResources.h"
#include "Stats.h"
#include "Utils.h"

class Animation;
class Hazard;

#define EFFECT_COUNT 28

enum EFFECT_TYPE {
	EFFECT_NONE = 0,
	EFFECT_DAMAGE = 1,
	EFFECT_DAMAGE_PERCENT = 2,
	EFFECT_HPOT = 3,
	EFFECT_HPOT_PERCENT = 4,
	EFFECT_MPOT = 5,
	EFFECT_MPOT_PERCENT = 6,
	EFFECT_SPEED = 7,
	EFFECT_IMMUNITY = 8,
	EFFECT_IMMUNITY_DAMAGE = 9,
	EFFECT_IMMUNITY_SLOW = 10,
	EFFECT_IMMUNITY_STUN = 11,
	EFFECT_IMMUNITY_HP_STEAL = 12,
	EFFECT_IMMUNITY_MP_STEAL = 13,
	EFFECT_IMMUNITY_KNOCKBACK = 14,
	EFFECT_IMMUNITY_DAMAGE_REFLECT = 15,
	EFFECT_STUN = 16,
	EFFECT_REVIVE = 17,
	EFFECT_CONVERT = 18,
	EFFECT_FEAR = 19,
	EFFECT_OFFENSE = 20,
	EFFECT_DEFENSE = 21,
	EFFECT_PHYSICAL = 22,
	EFFECT_MENTAL = 23,
	EFFECT_DEATH_SENTENCE = 24,
	EFFECT_SHIELD = 25,
	EFFECT_HEAL = 26,
	EFFECT_KNOCKBACK = 27
};

class Effect {
public:
	std::string id;
	std::string name;
	int icon;
	int ticks;
	int duration;
	int type;
	int magnitude;
	int magnitude_max;
	std::string animation_name;
	Animation* animation;
	bool item;
	int trigger;
	bool render_above;
	int passive_id;
	int source_type;

	Effect()
		: id("")
		, name("")
		, icon(-1)
		, ticks(0)
		, duration(-1)
		, type(EFFECT_NONE)
		, magnitude(0)
		, magnitude_max(0)
		, animation_name("")
		, animation(NULL)
		, item(false)
		, trigger(-1)
		, render_above(false)
		, passive_id(0)
		, source_type(SOURCE_TYPE_HERO) {
	}

	~Effect() {
	}

};

class EffectManager {
private:
	Animation* loadAnimation(const std::string &s);
	void removeEffect(size_t id);
	void removeAnimation(size_t id);
	void clearStatus();
	int getType(const std::string& type);

public:
	EffectManager();
	~EffectManager();
	EffectManager& operator= (const EffectManager &emSource);
	void logic();
	void addEffect(EffectDef &effect, int duration, int magnitude, bool item, int trigger, int passive_id, int source_type);
	void removeEffectType(const int type);
	void removeEffectPassive(int id);
	void clearEffects();
	void clearNegativeEffects(int type = -1);
	void clearItemEffects();
	void clearTriggerEffects(int trigger);
	int damageShields(int dmg);
	bool isDebuffed();

	std::vector<Effect> effect_list;

	int damage;
	int damage_percent;
	int hpot;
	int hpot_percent;
	int mpot;
	int mpot_percent;
	float speed;
	bool immunity_damage;
	bool immunity_slow;
	bool immunity_stun;
	bool immunity_hp_steal;
	bool immunity_mp_steal;
	bool immunity_knockback;
	bool immunity_damage_reflect;
	bool stun;
	bool revive;
	bool convert;
	bool death_sentence;
	bool fear;
	float knockback_speed;

	int bonus_offense;
	int bonus_defense;
	int bonus_physical;
	int bonus_mental;
	std::vector<int> bonus;
	std::vector<int> bonus_resist;

	bool triggered_others;
	bool triggered_block;
	bool triggered_hit;
	bool triggered_halfdeath;
	bool triggered_joincombat;
	bool triggered_death;
};

#endif
