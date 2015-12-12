/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk

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
 * class StatBlock
 *
 * Character stats and calculations
 */

#ifndef STAT_BLOCK_H
#define STAT_BLOCK_H

#include "CommonIncludes.h"
#include "EffectManager.h"
#include "MapCollision.h"
#include "Stats.h"
#include "Utils.h"

class Power;
class FileParser;

typedef enum {
	AI_POWER_MELEE = 0,
	AI_POWER_RANGED = 1,
	AI_POWER_BEACON = 2,
	AI_POWER_HIT = 3,
	AI_POWER_DEATH = 4,
	AI_POWER_HALF_DEAD = 5,
	AI_POWER_JOIN_COMBAT = 6,
	AI_POWER_DEBUFF = 7
} AI_POWER;

// active states
const int ENEMY_STANCE = 0;
const int ENEMY_MOVE = 1;
const int ENEMY_CHARGE = 2;
const int ENEMY_MELEE_PHYS = 3;
const int ENEMY_MELEE_MENT = 4;
const int ENEMY_RANGED_PHYS = 5;
const int ENEMY_RANGED_MENT = 6;
const int ENEMY_SPAWN = 7;
// interrupt states
const int ENEMY_BLOCK = 9;
const int ENEMY_HIT = 10;
const int ENEMY_DEAD = 11;
const int ENEMY_CRITDEAD = 12;
const int ENEMY_HALF_DEAD = 13;
const int ENEMY_JOIN_COMBAT = 14;

// final shared states
const int ENEMY_POWER = 15; // enemy performing a power. anim/sfx based on power

// combat styles; enemy only
const int COMBAT_DEFAULT = 0;
const int COMBAT_AGGRESSIVE = 1;
const int COMBAT_PASSIVE = 2;

class AIPower {
public:
	AI_POWER type;
	int id;
	int chance;
	int ticks;

	AIPower()
		: type(AI_POWER_MELEE)
		, id(0)
		, chance(0)
		, ticks(0)
	{}
};

class StatBlock {
private:
	bool loadCoreStat(FileParser *infile);
	bool loadSfxStat(FileParser *infile);
	void loadHeroStats();
	bool statsLoaded;

public:
	StatBlock();
	~StatBlock();

	void load(const std::string& filename);
	void takeDamage(int dmg);
	void recalc();
	void applyEffects();
	void calcBase();
	void logic();
	void removeSummons();
	void removeFromSummons();
	bool summonLimitReached(int power_id) const;
	void setWanderArea(int r);
	void loadHeroSFX();
	std::string getShortClass();
	std::string getLongClass();
	void addXP(int amount);
	AIPower* getAIPower(AI_POWER ai_type);

	bool alive;
	bool corpse; // creature is dead and done animating
	int corpse_ticks;
	bool hero; // else, enemy or other
	bool hero_ally;
	bool humanoid; // true for human, sceleton...; false for wyvern, snake...
	bool permadeath;
	bool transformed;
	bool refresh_stats;
	bool converted;
	bool summoned;
	int summoned_power_index;
	bool encountered; // enemy only

	MOVEMENTTYPE movement_type;
	bool flying;
	bool intangible;
	bool facing; // does this creature turn to face the hero

	std::vector<std::string> categories;

	std::string name;

	int level;
	unsigned long xp;
	std::vector<unsigned long> xp_table;
	bool level_up;
	bool check_title;
	int stat_points_per_level;
	int power_points_per_level;

	// base stats ("attributes")
	int offense_character;
	int defense_character;
	int physical_character;
	int mental_character;

	// combat stats
	std::vector<int> starting; // default level 1 values per stat. Read from file and never changes at runtime.
	std::vector<int> base; // values before any active effects are applied
	std::vector<int> current; // values after all active effects are applied
	std::vector<int> per_level; // value increases each level after level 1
	std::vector<int> per_physical;
	std::vector<int> per_mental;
	std::vector<int> per_offense;
	std::vector<int> per_defense;

	int get(STAT stat) {
		return current[stat];
	}

	// additional values to base stats, given by items
	int offense_additional;
	int defense_additional;
	int physical_additional;
	int mental_additional;

	// getters for full base stats (character + additional)
	int get_offense()  const {
		return offense_character + offense_additional;
	}
	int get_defense()  const {
		return defense_character + defense_additional;
	}
	int get_physical() const {
		return physical_character + physical_additional;
	}
	int get_mental()   const {
		return mental_character + mental_additional;
	}

	// derived stats ("disciplines")
	int physoff() {
		return get_physical() + get_offense();
	}
	int physdef() {
		return get_physical() + get_defense();
	}
	int mentoff() {
		return get_mental() + get_offense();
	}
	int mentdef() {
		return get_mental() + get_defense();
	}
	int physment() {
		return get_physical() + get_mental();
	}
	int offdef() {
		return get_offense() + get_defense();
	}

	// Base class picked when starting a new game. Defaults to "Adventurer".
	std::string character_class;
	// Class derived from certain properties defined in engine/titles.txt
	std::string character_subclass;

	// physical stats
	int hp;
	int hp_ticker;

	// mental stats
	int mp;
	int mp_ticker;

	float speed_default;

	// addition damage and absorb granted from items
	int dmg_melee_min_add;
	int dmg_melee_max_add;
	int dmg_ment_min_add;
	int dmg_ment_max_add;
	int dmg_ranged_min_add;
	int dmg_ranged_max_add;
	int absorb_min_add;
	int absorb_max_add;

	float speed;

	std::set<std::string> equip_flags;
	std::vector<int> vulnerable;
	std::vector<int> vulnerable_base;

	// buff and debuff stats
	int transform_duration;
	int transform_duration_total;
	bool manual_untransform;
	bool transform_with_equipment;
	bool untransform_on_hit;
	EffectManager effects;
	bool blocking;

	FPoint pos;
	FPoint knockback_speed;
	FPoint knockback_srcpos;
	FPoint knockback_destpos;
	unsigned char direction;

	int cooldown_hit;
	int cooldown_hit_ticks;

	// state
	int cur_state;
	int state_ticks;
	bool hold_state;

	// waypoint patrolling
	std::queue<FPoint> waypoints;
	int waypoint_pause;
	int waypoint_pause_ticks;

	// wandering area
	bool wander;
	Rect wander_area;

	// enemy behavioral stats
	int chance_pursue;
	int chance_flee;

	std::vector<int> powers_list;
	std::vector<int> powers_list_items;
	std::vector<int> powers_passive;
	std::vector<AIPower> powers_ai;

	bool canUsePower(const Power &power, unsigned powerid) const;

	float melee_range;
	float threat_range;
	int combat_style; // determines how the creature enters combat
	int hero_stealth;
	int turn_delay;
	int turn_ticks;
	bool in_combat;
	bool join_combat;
	int cooldown_ticks;
	int cooldown; // min. # of frames between abilities
	AIPower* activated_power;
	bool half_dead_power;
	bool suppress_hp; // hide an enemy HP bar

	std::vector<Event_Component> loot_table;
	Point loot_count;

	// for the teleport spell
	bool teleportation;
	FPoint teleport_destination;

	// for purchasing tracking
	int currency;

	// marked for death
	bool death_penalty;

	// Campaign event interaction
	std::string defeat_status;
	std::string convert_status;
	std::string quest_loot_requires_status;
	std::string quest_loot_requires_not_status;
	int quest_loot_id;
	int first_defeat_loot;

	// player look options
	std::string gfx_base; // folder in /images/avatar
	std::string gfx_head; // png in /images/avatar/[base]
	std::string gfx_portrait; // png in /images/portraits
	std::string transform_type;

	std::string animations;

	// default sounds
	std::string sfx_step;
	std::string sfx_phys;
	std::string sfx_ment;
	std::string sfx_hit;
	std::string sfx_die;
	std::string sfx_critdie;
	std::string sfx_block;
	std::string sfx_levelup;

	// formula numbers
	int max_spendable_stat_points;
	int max_points_per_stat;

	// preserve state before calcs
	int prev_maxhp;
	int prev_maxmp;
	int pres_hp;
	int pres_mp;

	// links to summoned creatures and the entity which summoned this
	std::vector<StatBlock*> summons;
	StatBlock* summoner;

	bool attacking;
};

#endif

