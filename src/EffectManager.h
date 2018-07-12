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
#include "Utils.h"

class Animation;
class Hazard;

class Effect {
public:
	static const int TYPE_COUNT = 26;
	enum {
		NONE = 0,
		DAMAGE = 1,
		DAMAGE_PERCENT = 2,
		HPOT = 3,
		HPOT_PERCENT = 4,
		MPOT = 5,
		MPOT_PERCENT = 6,
		SPEED = 7,
		ATTACK_SPEED = 8,
		IMMUNITY = 9,
		IMMUNITY_DAMAGE = 10,
		IMMUNITY_SLOW = 11,
		IMMUNITY_STUN = 12,
		IMMUNITY_HP_STEAL = 13,
		IMMUNITY_MP_STEAL = 14,
		IMMUNITY_KNOCKBACK = 15,
		IMMUNITY_DAMAGE_REFLECT = 16,
		IMMUNITY_STAT_DEBUFF = 17,
		STUN = 18,
		REVIVE = 19,
		CONVERT = 20,
		FEAR = 21,
		DEATH_SENTENCE = 22,
		SHIELD = 23,
		HEAL = 24,
		KNOCKBACK = 25
	};

	Effect();
	Effect(const Effect& other);
	Effect& operator=(const Effect& other);
	~Effect();

	void loadAnimation(const std::string &s);
	void unloadAnimation();

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
	size_t passive_id;
	int source_type;
	bool group_stack;
	Color color_mod;
	uint8_t alpha_mod;
	std::string attack_speed_anim;
};

class EffectDef {
public:
	EffectDef();

	std::string id;
	std::string type;
	std::string name;
	int icon;
	std::string animation;
	bool can_stack;
	int max_stacks;
	bool group_stack;
	bool render_above;
	Color color_mod;
	uint8_t alpha_mod;
	std::string attack_speed_anim;
};

class EffectManager {
private:
	void removeEffect(size_t id);
	void clearStatus();
	int getType(const std::string& type);
	void addEffectInternal(EffectDef &effect, int duration, int magnitude, int source_type, bool item, size_t power_id);

public:
	EffectManager();
	~EffectManager();
	void logic();
	void addEffect(EffectDef &effect, int duration, int magnitude, int source_type, size_t power_id);
	void addItemEffect(EffectDef &effect, int duration, int magnitude);
	void removeEffectType(const int type);
	void removeEffectPassive(size_t id);
	void removeEffectID(const std::vector< std::pair<std::string, int> >& remove_effects);
	void clearEffects();
	void clearNegativeEffects(int type = -1);
	void clearItemEffects();
	void clearTriggerEffects(int trigger);
	int damageShields(int dmg);
	bool isDebuffed();
	void getCurrentColor(Color& color_mod);
	void getCurrentAlpha(uint8_t& alpha_mod);
	bool hasEffect(const std::string& id, int req_count);
	float getAttackSpeed(const std::string& anim_name);

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
	bool immunity_stat_debuff;
	bool stun;
	bool revive;
	bool convert;
	bool death_sentence;
	bool fear;
	float knockback_speed;

	std::vector<int> bonus;
	std::vector<int> bonus_resist;
	std::vector<int> bonus_primary;

	bool triggered_others;
	bool triggered_block;
	bool triggered_hit;
	bool triggered_halfdeath;
	bool triggered_joincombat;
	bool triggered_death;

	bool refresh_stats;

	static const int NO_POWER = 0;
};

#endif
