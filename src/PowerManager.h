/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
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
 * class PowerManager
 *
 * Special code for handling spells, special powers, item effects, etc.
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "FileParser.h"
#include "LootManager.h"
#include "MapRenderer.h"
#include "Map.h"

#include <cassert>

class AnimationSet;
class Hazard;

const int POWTYPE_FIXED = 0;
const int POWTYPE_MISSILE = 1;
const int POWTYPE_REPEATER = 2;
const int POWTYPE_SPAWN = 3;
const int POWTYPE_TRANSFORM = 4;
const int POWTYPE_EFFECT = 5;
const int POWTYPE_BLOCK = 6;

const int POWSTATE_INSTANT = 1;
const int POWSTATE_ATTACK = 2;

const int BASE_DAMAGE_NONE = 0;
const int BASE_DAMAGE_MELEE = 1;
const int BASE_DAMAGE_RANGED = 2;
const int BASE_DAMAGE_MENT = 3;

// when casting a spell/power, the hazard starting position is
// either the source (the avatar or enemy), the target (mouse click position),
// or melee range in the direction that the source is facing
const int STARTING_POS_SOURCE = 0;
const int STARTING_POS_TARGET = 1;
const int STARTING_POS_MELEE = 2;

const int TRIGGER_BLOCK = 0;
const int TRIGGER_HIT = 1;
const int TRIGGER_HALFDEATH = 2;
const int TRIGGER_JOINCOMBAT = 3;
const int TRIGGER_DEATH = 4;

const int SPAWN_LIMIT_MODE_FIXED = 0;
const int SPAWN_LIMIT_MODE_STAT = 1;
const int SPAWN_LIMIT_MODE_UNLIMITED = 2;

const int SPAWN_LIMIT_STAT_PHYSICAL = 0;
const int SPAWN_LIMIT_STAT_MENTAL = 1;
const int SPAWN_LIMIT_STAT_OFFENSE = 2;
const int SPAWN_LIMIT_STAT_DEFENSE = 3;

const int SPAWN_LEVEL_MODE_DEFAULT = 0;
const int SPAWN_LEVEL_MODE_FIXED = 1;
const int SPAWN_LEVEL_MODE_STAT = 2;
const int SPAWN_LEVEL_MODE_LEVEL = 3;

const int SPAWN_LEVEL_STAT_PHYSICAL = 0;
const int SPAWN_LEVEL_STAT_MENTAL = 1;
const int SPAWN_LEVEL_STAT_OFFENSE = 2;
const int SPAWN_LEVEL_STAT_DEFENSE = 3;

const int STAT_MODIFIER_MODE_MULTIPLY = 0;
const int STAT_MODIFIER_MODE_ADD = 1;
const int STAT_MODIFIER_MODE_ABSOLUTE = 2;

class PostEffect {
public:
	std::string id;
	int magnitude;
	int duration;

	PostEffect()
		: id("")
		, magnitude(0)
		, duration(0) {
	}
};

class ActionData {
public:
	int power;
	unsigned hotkey;
	bool instant_item;
	FPoint target;

	ActionData()
		: power(0)
		, hotkey(0)
		, instant_item(false)
		, target(FPoint()) {
	}
};

class Power {
public:
	// base info
	int type; // what kind of activate() this is
	std::string name;
	std::string description;
	int icon; // just the number.  The caller menu will have access to the surface.
	int new_state; // when using this power the user (avatar/enemy) starts a new state
	int state_duration; // can be used to extend the length of a state animation by pausing on the last frame
	std::string attack_anim; // name of the animation to play when using this power, if it is not block
	bool face; // does the user turn to face the mouse cursor when using this power?
	int source_type; //hero, neutral, or enemy
	bool beacon; //true if it's just an ememy calling its allies
	int count; // number of hazards/effects or spawns created
	bool passive; // if unlocked when the user spawns, automatically cast it
	int passive_trigger; // only activate passive powers under certain conditions (block, hit, death, etc)
	bool meta_power; // this power can't be used on its own and must be replaced via equipment

	// power requirements
	std::set<std::string> requires_flags; // checked against equip_flags granted from items
	int requires_mp;
	int requires_hp;
	bool sacrifice;
	bool requires_los; // line of sight
	bool requires_empty_target; // target square must be empty
	int requires_item;
	int requires_item_quantity;
	int requires_equipped_item;
	int requires_equipped_item_quantity;
	bool consumable;
	bool requires_targeting; // power only makes sense when using click-to-target
	int requires_spawns;
	int cooldown; // milliseconds before you can use the power again

	// animation info
	std::string animation_name;
	int sfx_index;
	unsigned long sfx_hit;
	bool sfx_hit_enable;
	bool directional; // sprite sheet contains options for 8 directions, one per row
	int visual_random; // sprite sheet contains rows of random options
	int visual_option; // sprite sheet contains rows of similar effects.  use a specific option
	bool aim_assist;
	float speed; // for missile hazards, tiles per frame
	int lifespan; // how long the hazard/animation lasts
	bool floor; // the hazard is drawn between the background and object layers
	bool complete_animation;
	float charge_speed;

	// hazard traits
	bool use_hazard;
	bool no_attack;
	float radius;
	int base_damage; // enum.  damage is powered by melee, ranged, mental weapon
	int starting_pos; // enum. (source, target, or melee)
	bool relative_pos;
	bool multitarget;
	bool multihit;
	float target_range;
	bool target_party;
	std::vector<std::string> target_categories;

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

	int post_power;
	int wall_power;

	// spawn info
	std::string spawn_type;
	int target_neighbor;
	int spawn_limit_mode;
	int spawn_limit_qty;
	int spawn_limit_every;
	int spawn_limit_stat;

	int spawn_level_mode;
	int spawn_level_qty;
	int spawn_level_every;
	int spawn_level_stat;

	// targeting by movement type
	bool target_movement_normal;
	bool target_movement_flying;
	bool target_movement_intangible;

	bool walls_block_aoe;

	// loot
	std::vector<Event_Component> loot;

	Power()
		: type(-1)
		, name("")
		, description("")
		, icon(-1)
		, new_state(-1)
		, state_duration(0)
		, attack_anim("")
		, face(false)
		, source_type(-1)
		, beacon(false)
		, count(1)
		, passive(false)
		, passive_trigger(-1)
		, meta_power(false)

		, requires_mp(0)
		, requires_hp(0)
		, sacrifice(false)
		, requires_los(false)
		, requires_empty_target(false)
		, requires_item(-1)
		, requires_item_quantity(0)
		, requires_equipped_item(-1)
		, requires_equipped_item_quantity(0)
		, consumable(false)
		, requires_targeting(false)
		, requires_spawns(0)
		, cooldown(0)

		, animation_name("")
		, sfx_index(-1)
		, sfx_hit(0)
		, directional(false)
		, visual_random(0)
		, visual_option(0)
		, aim_assist(false)
		, speed(0)
		, lifespan(0)
		, floor(false)
		, complete_animation(false)
		, charge_speed(0.0f)

		, use_hazard(false)
		, no_attack(false)
		, radius(0)
		, base_damage(BASE_DAMAGE_NONE)
		, starting_pos(STARTING_POS_SOURCE)
		, relative_pos(false)
		, multitarget(false)
		, multihit(false)
		, target_range(0)
		, target_party(false)
		, mod_accuracy_mode(-1)
		, mod_accuracy_value(100)
		, mod_crit_mode(-1)
		, mod_crit_value(100)
		, mod_damage_mode(-1)
		, mod_damage_value_min(100)
		, mod_damage_value_max(0)

		, hp_steal(0)
		, mp_steal(0)

		, missile_angle(0)
		, angle_variance(0)
		, speed_variance(0)

		, delay(0)

		, trait_elemental(-1)
		, trait_armor_penetration(false)
		, trait_crits_impaired(0)
		, trait_avoidance_ignore(false)

		, transform_duration(0)
		, manual_untransform(false)
		, keep_equipment(false)
		, untransform_on_hit(false)

		, buff(false)
		, buff_teleport(false)
		, buff_party(false)
		, buff_party_power_id(0)

		, post_power(0)
		, wall_power(0)

		, spawn_type("")
		, target_neighbor(0)
		, spawn_limit_mode(SPAWN_LIMIT_MODE_UNLIMITED)
		, spawn_limit_qty(1)
		, spawn_limit_every(1)
		, spawn_limit_stat(SPAWN_LIMIT_STAT_MENTAL)
		, spawn_level_mode(SPAWN_LEVEL_MODE_DEFAULT)
		, spawn_level_qty(0)
		, spawn_level_every(0)
		, spawn_level_stat(SPAWN_LEVEL_STAT_MENTAL)

		, target_movement_normal(true)
		, target_movement_flying(true)
		, target_movement_intangible(true)

		, walls_block_aoe(false) {
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

public:
	PowerManager(LootManager *_lootm);
	~PowerManager();

	LootManager *lootm;
	std::string log_msg;

	void handleNewMap(MapCollision *_collider);
	bool activate(int power_index, StatBlock *src_stats, const FPoint& target);
	const Power &getPower(unsigned id) 	{
		assert(id < powers.size());
		return powers[id];
	}
	bool canUsePower(unsigned id) const;
	bool hasValidTarget(int power_index, StatBlock *src_stats, const FPoint& target);
	bool spawn(const std::string& enemy_type, const Point& target);
	bool effect(StatBlock *src_stats, StatBlock *caster_stats, int power_index, int source_type);
	void activatePassives(StatBlock *src_stats);
	void activateSinglePassive(StatBlock *src_stats, int id);
	int verifyID(int power_id, FileParser* infile = NULL, bool allow_zero = true);

	EffectDef* getEffectDef(const std::string& id);

	std::vector<EffectDef> effects;
	std::vector<Power> powers;
	std::queue<Hazard *> hazards; // output; read by HazardManager
	std::queue<Map_Enemy> enemies; // output; read by PowerManager

	// shared sounds for power special effects
	std::vector<SoundManager::SoundID> sfx;

	std::vector<int> used_items;
	std::vector<int> used_equipped_items;

	std::vector<Event_Component> loot;
};

#endif
