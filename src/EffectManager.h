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
class StatBlock;

class Effect {
public:
	enum {
		NONE = 0,
		DAMAGE,
		DAMAGE_PERCENT,
		HPOT,
		HPOT_PERCENT,
		MPOT,
		MPOT_PERCENT,
		SPEED,
		ATTACK_SPEED,
		RESIST_ALL,
		STUN,
		REVIVE,
		CONVERT,
		FEAR,
		DEATH_SENTENCE,
		SHIELD,
		HEAL,
		KNOCKBACK,
		TYPE_COUNT
	};

	Effect();
	Effect(const Effect& other);
	Effect& operator=(const Effect& other);
	~Effect();

	void loadAnimation(const std::string &s);
	void unloadAnimation();

	static int getTypeFromString(const std::string& _type);
	static bool typeIsStat(int t);
	static bool typeIsDmgMin(int t);
	static bool typeIsDmgMax(int t);
	static bool typeIsResist(int t);
	static bool typeIsResourceStat(int t);
	static bool typeIsResourceEffect(int t);
	static bool typeIsPrimary(int t);
	static bool typeIsEffectResist(int t);
	static int getStatFromType(int t);
	static size_t getDmgFromType(int t);
	static size_t getResourceStatFromType(int t);
	static size_t getResourceStatSubIndexFromType(int t);
	static size_t getPrimaryFromType(int t);

	static bool isImmunityTypeString(const std::string& type_str); // handling of deprecated types

	std::string id;
	std::string name;
	int icon;
	Timer timer;
	int type;
	float magnitude;
	float magnitude_max;
	std::string animation_name;
	Animation* animation;
	bool is_from_item;
	int trigger;
	bool render_above;
	size_t passive_id;
	int source_type;
	bool group_stack;
	uint32_t color_mod;
	uint8_t alpha_mod;
	std::string attack_speed_anim;
	bool is_multiplier;
};

class EffectDef {
public:
	EffectDef();

	std::string id;
	int type;
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

	bool is_immunity_type; // handling of deprecated types
};

class EffectParams {
public:
	EffectParams();

	bool is_from_item;
	bool is_multiplier;
	int duration;
	int source_type;
	float magnitude;
	PowerID power_id;
};

class EffectManager {
private:
	void removeEffect(size_t id);
	void clearStatus();

public:
	EffectManager();
	~EffectManager();
	void logic();
	void addEffect(StatBlock* stats, EffectDef &effect, EffectParams &params);
	void removeEffectType(const int type);
	void removeEffectPassive(size_t id);
	void removeEffectID(const std::vector< std::pair<std::string, int> >& remove_effects);
	void clearEffects();
	void clearNegativeEffects(int type);
	void clearItemEffects();
	void clearTriggerEffects(int trigger);
	float damageShields(float dmg);
	bool isDebuffed();
	void getCurrentColor(Color& color_mod);
	void getCurrentAlpha(uint8_t& alpha_mod);
	bool hasEffect(const std::string& id, int req_count);
	float getAttackSpeed(const std::string& anim_name);
	int getDamageSourceType(int dmg_mode);

	std::vector<Effect> effect_list;

	// TODO rename these to *_per_second; maybe make into array?
	float damage;
	float damage_percent;
	float hpot;
	float hpot_percent;
	float mpot;
	float mpot_percent;
	std::vector<float> resource_ot;
	std::vector<float> resource_ot_percent;

	float speed;
	bool stun;
	bool revive;
	bool convert;
	bool death_sentence;
	bool fear;
	float knockback_speed;

	std::vector<float> bonus;
	std::vector<float> bonus_multiplier;
	std::vector<int> bonus_primary;

	// TODO convert to array
	bool triggered_others;
	bool triggered_block;
	bool triggered_hit;
	bool triggered_halfdeath;
	bool triggered_joincombat;
	bool triggered_death;
	bool triggered_active_power;

	bool refresh_stats;

	static const int NO_POWER = 0;
};

#endif
