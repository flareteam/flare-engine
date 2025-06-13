/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
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
 * class PowerManager
 *
 * Special code for handling spells, special powers, item effects, etc.
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "Map.h"
#include "MapCollision.h"
#include "Utils.h"

class Animation;
class AnimationSet;
class EffectDef;
class Hazard;

class PostEffect {
public:
	std::string id;
	float magnitude;
	int duration;
	float chance;
	bool target_src;
	bool is_multiplier;

	PostEffect()
		: id("")
		, magnitude(0)
		, duration(0)
		, chance(100)
		, target_src(false)
		, is_multiplier(false) {
	}
};

class PowerReplaceByEffect {
public:
	PowerID power_id;
	int count;
	std::string effect_id;

	PowerReplaceByEffect()
		: power_id(0)
		, count(0)
		, effect_id()
	{}
};

class PowerRequiredItem {
public:
	ItemID id;
	int quantity;
	bool equipped;

	PowerRequiredItem()
		: id(0)
		, quantity(0)
		, equipped(false)
	{}
};

class ChainPower {
public:
	enum {
		TYPE_PRE = 0,
		TYPE_POST,
		TYPE_WALL,
	};

	PowerID id;
	uint8_t type;
	float chance;

	ChainPower()
		: id(0)
		, type(0)
		, chance(100)
	{}
};

class Power {
public:
	class ResourceState {
	public:
		int state;
		float value;
		ResourceState()
			: state(RESOURCESTATE_IGNORE)
			, value(0)
		{}
		~ResourceState() {}
	};

	enum {
		RESOURCESTATE_ANY = 0,
		RESOURCESTATE_ALL = 1,
		RESOURCESTATE_ANY_HPMP = 2,
	};

	enum {
		RESOURCESTATE_IGNORE = 0,
		RESOURCESTATE_PERCENT = 1,
		RESOURCESTATE_NOT_PERCENT = 2,
		RESOURCESTATE_PERCENT_EXACT = 3
	};

	enum {
		TYPE_FIXED = 0,
		TYPE_MISSILE = 1,
		TYPE_REPEATER = 2,
		TYPE_SPAWN = 3,
		TYPE_TRANSFORM = 4,
		TYPE_EFFECT = 5,
		TYPE_BLOCK = 6
	};

	enum {
		STATE_INSTANT = 1,
		STATE_ATTACK = 2
	};

	enum {
		STARTING_POS_SOURCE = 0,
		STARTING_POS_TARGET = 1,
		STARTING_POS_MELEE = 2
	};

	enum {
		TRIGGER_BLOCK = 0,
		TRIGGER_HIT = 1,
		TRIGGER_HALFDEATH = 2,
		TRIGGER_JOINCOMBAT = 3,
		TRIGGER_DEATH = 4,
		TRIGGER_ACTIVE_POWER = 5,
	};

	enum {
		SPAWN_LIMIT_MODE_FIXED = 0,
		SPAWN_LIMIT_MODE_STAT = 1,
		SPAWN_LIMIT_MODE_UNLIMITED = 2
	};

	enum {
		STAT_MODIFIER_MODE_MULTIPLY = 0,
		STAT_MODIFIER_MODE_ADD = 1,
		STAT_MODIFIER_MODE_ABSOLUTE = 2
	};

	enum {
		SOURCE_TYPE_HERO = 0,
		SOURCE_TYPE_NEUTRAL = 1,
		SOURCE_TYPE_ENEMY = 2,
		SOURCE_TYPE_ALLY = 3
	};

	enum {
		SCRIPT_TRIGGER_CAST = 0,
		SCRIPT_TRIGGER_HIT = 1,
		SCRIPT_TRIGGER_WALL = 2
	};

	bool prevent_interrupt; // prevents hits from interrupting the casting state
	bool face; // does the user turn to face the mouse cursor when using this power?
	bool beacon; //true if it's just an ememy calling its allies
	bool passive; // if unlocked when the user spawns, automatically cast it
	bool meta_power; // this power can't be used on its own and must be replaced via equipment
	bool no_actionbar; // prevents this power from being placed on the actionbar
	bool sacrifice;
	bool requires_los; // line of sight
	bool requires_los_default;
	bool requires_empty_target; // target square must be empty
	bool consumable;
	bool requires_targeting; // power only makes sense when using click-to-target
	bool sfx_hit_enable;
	bool directional; // sprite sheet contains options for 8 directions, one per row
	bool aim_assist;
	bool on_floor; // the hazard is drawn between the background and object layers
	bool complete_animation;
	bool use_hazard;
	bool no_attack;
	bool no_aggro;
	bool relative_pos;
	bool multitarget;
	bool multihit;
	bool expire_with_caster;
	bool ignore_zero_damage;
	bool lock_target_to_direction;
	bool target_party;
	bool trait_armor_penetration;
	bool trait_avoidance_ignore;
	bool manual_untransform; // true binds to the power another recurrence power
	bool keep_equipment;
	bool untransform_on_hit;
	bool buff;
	bool buff_teleport;
	bool buff_party;
	bool wall_reflect;
	bool target_movement_normal;
	bool target_movement_flying;
	bool target_movement_intangible;
	bool walls_block_aoe;
	bool requires_corpse;
	bool remove_corpse;
	bool post_hazards_skip_target;
	bool can_trigger_passives;

	uint8_t spawn_limit_mode;

	unsigned short visual_random; // sprite sheet contains rows of random options
	unsigned short visual_option; // sprite sheet contains rows of similar effects.  use a specific option

	int type; // what kind of activate() this is
	int icon; // just the number.  The caller menu will have access to the surface.
	int new_state; // when using this power the user (avatar/enemy) starts a new state
	int state_duration; // can be used to extend the length of a state animation by pausing on the last frame
	int source_type; //hero, neutral, or enemy
	int count; // number of hazards/effects or spawns created
	int passive_trigger; // only activate passive powers under certain conditions (block, hit, death, etc)
	int requires_spawns;
	int cooldown; // milliseconds before you can use the power again
	int requires_hpmp_state_mode;
	int requires_resource_stat_state_mode;
	int sfx_index;
	int lifespan; // how long the hazard/animation lasts
	int starting_pos; // enum. (source, target, or melee)
	int movement_type;
	int mod_accuracy_mode;
	int mod_crit_mode;
	int mod_damage_mode;
	int delay;
	int transform_duration;
	int target_neighbor;
	int script_trigger;

	float requires_mp;
	float requires_hp;
	float speed; // for missile hazards, tiles per frame
	float charge_speed;
	float attack_speed;
	float radius;
	float target_range;
	float combat_range;
	float mod_accuracy_value;
	float mod_crit_value;
	float mod_damage_value_min;
	float mod_damage_value_max; //only used if mode is absolute
	float hp_steal;
	float mp_steal;
	float missile_angle;
	float angle_variance;
	float speed_variance;
	float spawn_limit_count;
	float spawn_limit_ratio;
	float trait_crits_impaired; // crit bonus vs. movement impaired enemies (slowed, immobilized, stunned)
	float target_nearest;

	size_t base_damage; // index of eset->damage_types.list
	size_t converted_damage; // index of eset->damage_types.list
	size_t spawn_limit_stat;

	SoundID sfx_hit;
	PowerID buff_party_power_id;

	ResourceState requires_hp_state;
	ResourceState requires_mp_state;

	SpawnLevel spawn_level;

	std::string name;
	std::string description;
	std::string attack_anim; // name of the animation to play when using this power, if it is not block
	std::string animation_name;
	std::string spawn_type;
	std::string script;

	std::vector<float> requires_resource_stat;
	std::vector<PowerRequiredItem> required_items;
	std::vector<ResourceState> requires_resource_stat_state;
	std::vector<std::string> target_categories;
	std::vector<float> resource_steal;
	std::vector<PostEffect> post_effects;
	std::vector<ChainPower> chain_powers;
	std::vector< std::pair<std::string, int> > remove_effects;
	std::vector<PowerReplaceByEffect> replace_by_effect;
	std::vector<size_t> disable_equip_slots;
	std::set<std::string> requires_flags; // checked against equip_flags granted from items

	Power();
	~Power() {
	}
};

class PowerManager {
private:

	MapCollision *collider;

	void loadEffects();
	void loadPowers();

	bool isValidEffect(const std::string& type);
	int loadSFX(const std::string& filename);

	void initHazard(PowerID power_index, StatBlock *src_stats, const FPoint& origin, const FPoint& target, Hazard *haz);
	void buff(PowerID power_index, StatBlock *src_stats, const FPoint& origin, const FPoint& target);
	void playSound(PowerID power_index);

	bool fixed(PowerID power_index, StatBlock *src_stats, const FPoint& origin, const FPoint& target);
	bool missile(PowerID power_index, StatBlock *src_stats, const FPoint& origin, const FPoint& target);
	bool repeater(PowerID power_index, StatBlock *src_stats, const FPoint& origin, const FPoint& target);
	bool spawn(PowerID power_index, StatBlock *src_stats, const FPoint& origin, const FPoint& target);
	bool transform(PowerID power_index, StatBlock *src_stats, const FPoint& target);
	bool block(PowerID power_index, StatBlock *src_stats);

	void payPowerCost(PowerID power_index, StatBlock *src_stats);

	bool activatePassiveByTrigger(PowerID power_id, StatBlock *src_stats, bool& triggered_others);
	void activatePassivePostPowers(StatBlock *src_stats);

	std::vector<Animation*> power_animations;
	std::vector<Animation*> effect_animations;

public:
	static const bool ALLOW_ZERO_ID = true;

	explicit PowerManager();
	~PowerManager();
	bool isValid(PowerID power_id);

	void handleNewMap(MapCollision *_collider);
	bool activate(PowerID power_index, StatBlock *src_stats, const FPoint& origin, const FPoint& target);
	bool canUsePower(PowerID id) const;
	bool hasValidTarget(PowerID power_index, StatBlock *src_stats, const FPoint& target);
	bool effect(StatBlock *target_stats, StatBlock *caster_stats, PowerID power_index, int source_type);
	void activatePassives(StatBlock *src_stats);
	void activateSinglePassive(StatBlock *src_stats, PowerID id);
	PowerID verifyID(PowerID power_id, FileParser* infile, bool allow_zero);
	bool checkNearestTargeting(const Power* pow, const StatBlock *src_stats, bool check_corpses);
	bool checkRequiredItems(const Power* pow, const StatBlock *src_stats);
	bool checkRequiredResourceState(const Power* pow, const StatBlock *src_stats);
	bool checkCombatRange(PowerID power_index, StatBlock *src_stats, FPoint target);
	bool checkPowerCost(const Power* pow, const StatBlock *src_stats);
	PowerID checkReplaceByEffect(PowerID power_index, StatBlock *src_stats);

	EffectDef* getEffectDef(const std::string& id);

	std::vector<EffectDef> effects;
	std::vector<Power*> powers;

	std::queue<Hazard *> hazards; // output; read by HazardManager
	std::queue<Map_Enemy> map_enemies; // output; read by PowerManager

	// shared sounds for power special effects
	std::vector<SoundID> sfx;

	std::vector<ItemID> used_items;
	std::vector<ItemID> used_equipped_items;
};

#endif
