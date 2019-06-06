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
	int magnitude;
	int duration;
	int chance;
	bool target_src;

	PostEffect()
		: id("")
		, magnitude(0)
		, duration(0)
		, chance(100)
		, target_src(false) {
	}
};

class PowerReplaceByEffect {
public:
	int power_id;
	int count;
	std::string effect_id;
};

class PowerRequiredItem {
public:
	int id;
	int quantity;
	bool equipped;

	PowerRequiredItem()
		: id(0)
		, quantity(0)
		, equipped(false)
	{}
};

class Power {
public:
	enum {
		HPMPSTATE_ANY = 0,
		HPMPSTATE_ALL = 1
	};
	enum {
		HPMPSTATE_IGNORE = 0,
		HPMPSTATE_PERCENT = 1,
		HPMPSTATE_NOT_PERCENT = 2
	};
	class HPMPState {
	public:
		int mode;
		int hp_state;
		int mp_state;
		int hp;
		int mp;
		HPMPState()
			: mode(HPMPSTATE_ANY)
			, hp_state(HPMPSTATE_IGNORE)
			, mp_state(HPMPSTATE_IGNORE)
			, hp(-1)
			, mp(-1)
		{}
		~HPMPState() {}
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
		TRIGGER_DEATH = 4
	};

	enum {
		SPAWN_LIMIT_MODE_FIXED = 0,
		SPAWN_LIMIT_MODE_STAT = 1,
		SPAWN_LIMIT_MODE_UNLIMITED = 2
	};

	enum {
		SPAWN_LEVEL_MODE_DEFAULT = 0,
		SPAWN_LEVEL_MODE_FIXED = 1,
		SPAWN_LEVEL_MODE_STAT = 2,
		SPAWN_LEVEL_MODE_LEVEL = 3
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

	// base info
	bool is_empty;
	int type; // what kind of activate() this is
	std::string name;
	std::string description;
	int icon; // just the number.  The caller menu will have access to the surface.
	int new_state; // when using this power the user (avatar/enemy) starts a new state
	int state_duration; // can be used to extend the length of a state animation by pausing on the last frame
	bool prevent_interrupt; // prevents hits from interrupting the casting state
	std::string attack_anim; // name of the animation to play when using this power, if it is not block
	bool face; // does the user turn to face the mouse cursor when using this power?
	int source_type; //hero, neutral, or enemy
	bool beacon; //true if it's just an ememy calling its allies
	int count; // number of hazards/effects or spawns created
	bool passive; // if unlocked when the user spawns, automatically cast it
	int passive_trigger; // only activate passive powers under certain conditions (block, hit, death, etc)
	bool meta_power; // this power can't be used on its own and must be replaced via equipment
	bool no_actionbar; // prevents this power from being placed on the actionbar

	// power requirements
	std::set<std::string> requires_flags; // checked against equip_flags granted from items
	int requires_mp;
	int requires_hp;
	bool sacrifice;
	bool requires_los; // line of sight
	bool requires_los_default;
	bool requires_empty_target; // target square must be empty
	std::vector<PowerRequiredItem> required_items;
	bool consumable;
	bool requires_targeting; // power only makes sense when using click-to-target
	int requires_spawns;
	int cooldown; // milliseconds before you can use the power again
	HPMPState requires_max_hpmp;

	// animation info
	std::string animation_name;
	int sfx_index;
	SoundID sfx_hit;
	bool sfx_hit_enable;
	bool directional; // sprite sheet contains options for 8 directions, one per row
	int visual_random; // sprite sheet contains rows of random options
	int visual_option; // sprite sheet contains rows of similar effects.  use a specific option
	bool aim_assist;
	float speed; // for missile hazards, tiles per frame
	int lifespan; // how long the hazard/animation lasts
	bool on_floor; // the hazard is drawn between the background and object layers
	bool complete_animation;
	float charge_speed;
	float attack_speed;

	// hazard traits
	bool use_hazard;
	bool no_attack;
	bool no_aggro;
	float radius;
	size_t base_damage;
	int starting_pos; // enum. (source, target, or melee)
	bool relative_pos;
	bool multitarget;
	bool multihit;
	bool expire_with_caster;
	bool ignore_zero_damage;
	bool lock_target_to_direction;
	int movement_type;
	float target_range;
	bool target_party;
	std::vector<std::string> target_categories;
	float combat_range;

	int mod_accuracy_mode;
	int mod_accuracy_value;

	int mod_crit_mode;
	int mod_crit_value;

	int mod_damage_mode;
	int mod_damage_value_min;
	int mod_damage_value_max;//only used if mode is absolute

	//steal effects (in %, eg. hp_steal=50 turns 50% damage done into HP regain.)
	int hp_steal;
	int mp_steal;

	//missile traits
	int missile_angle;
	int angle_variance;
	float speed_variance;

	//repeater traits
	int delay;

	int trait_elemental; // enum. of elements
	bool trait_armor_penetration;
	int trait_crits_impaired; // crit bonus vs. movement impaired enemies (slowed, immobilized, stunned)
	bool trait_avoidance_ignore;

	int transform_duration;
	bool manual_untransform; // true binds to the power another recurrence power
	bool keep_equipment;
	bool untransform_on_hit;

	// special effects
	bool buff;
	bool buff_teleport;
	bool buff_party;
	int buff_party_power_id;

	std::vector<PostEffect> post_effects;

	int pre_power;
	int pre_power_chance;
	int post_power;
	int post_power_chance;
	int wall_power;
	int wall_power_chance;
	bool wall_reflect;

	// spawn info
	std::string spawn_type;
	int target_neighbor;
	int spawn_limit_mode;
	int spawn_limit_qty;
	int spawn_limit_every;
	size_t spawn_limit_stat;

	int spawn_level_mode;
	int spawn_level_qty;
	int spawn_level_every;
	size_t spawn_level_stat;

	// targeting by movement type
	bool target_movement_normal;
	bool target_movement_flying;
	bool target_movement_intangible;

	bool walls_block_aoe;

	int script_trigger;
	std::string script;

	std::vector< std::pair<std::string, int> > remove_effects;

	std::vector<PowerReplaceByEffect> replace_by_effect;

	bool requires_corpse;
	bool remove_corpse;

	float target_nearest;

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

	void initHazard(int powernum, StatBlock *src_stats, const FPoint& target, Hazard *haz);
	void buff(int power_index, StatBlock *src_stats, const FPoint& target);
	void playSound(int power_index);

	bool fixed(int powernum, StatBlock *src_stats, const FPoint& target);
	bool missile(int powernum, StatBlock *src_stats, const FPoint& target);
	bool repeater(int powernum, StatBlock *src_stats, const FPoint& target);
	bool spawn(int powernum, StatBlock *src_stats, const FPoint& target);
	bool transform(int powernum, StatBlock *src_stats, const FPoint& target);
	bool block(int power_index, StatBlock *src_stats);

	void payPowerCost(int power_index, StatBlock *src_stats);

	void activatePassiveByTrigger(int power_id, StatBlock *src_stats, bool& triggered_others);
	void activatePassivePostPowers(StatBlock *src_stats);

	void setCombatRange(size_t power_index);

	std::vector<Animation*> power_animations;
	std::vector<Animation*> effect_animations;

public:
	static const bool ALLOW_ZERO_ID = true;

	explicit PowerManager();
	~PowerManager();

	void handleNewMap(MapCollision *_collider);
	bool activate(int power_index, StatBlock *src_stats, const FPoint& target);
	bool canUsePower(unsigned id) const;
	bool hasValidTarget(int power_index, StatBlock *src_stats, const FPoint& target);
	bool effect(StatBlock *target_stats, StatBlock *caster_stats, int power_index, int source_type);
	void activatePassives(StatBlock *src_stats);
	void activateSinglePassive(StatBlock *src_stats, int id);
	int verifyID(int power_id, FileParser* infile, bool allow_zero);
	bool checkNearestTargeting(const Power &pow, const StatBlock *src_stats, bool check_corpses);
	bool checkRequiredItems(const Power &pow, const StatBlock *src_stats);
	bool checkRequiredMaxHPMP(const Power &pow, const StatBlock *src_stats);
	bool checkCombatRange(int power_index, StatBlock *src_stats, FPoint target);
	int checkReplaceByEffect(int power_index, StatBlock *src_stats);

	EffectDef* getEffectDef(const std::string& id);

	std::vector<EffectDef> effects;
	std::vector<Power> powers;
	std::queue<Hazard *> hazards; // output; read by HazardManager
	std::queue<Map_Enemy> map_enemies; // output; read by PowerManager

	// shared sounds for power special effects
	std::vector<SoundID> sfx;

	std::vector<int> used_items;
	std::vector<int> used_equipped_items;
};

#endif
