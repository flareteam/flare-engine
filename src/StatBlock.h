/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
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
 * class StatBlock
 *
 * Character stats and calculations
 */

#ifndef STAT_BLOCK_H
#define STAT_BLOCK_H

#include "CommonIncludes.h"
#include "EffectManager.h"
#include "EventManager.h"
#include "Stats.h"
#include "Utils.h"

class FileParser;

class StatBlock {
private:
	bool loadCoreStat(FileParser *infile);
	bool loadSfxStat(FileParser *infile);
	bool isNPCStat(FileParser *infile);
	void loadHeroStats();
	bool checkRequiredSpawns(int req_amount) const;
	bool statsLoaded;

public:
	enum {
		AI_POWER_MELEE = 0,
		AI_POWER_RANGED = 1,
		AI_POWER_BEACON = 2,
		AI_POWER_HIT = 3,
		AI_POWER_DEATH = 4,
		AI_POWER_HALF_DEAD = 5,
		AI_POWER_JOIN_COMBAT = 6,
		AI_POWER_DEBUFF = 7,
		AI_POWER_PASSIVE_POST = 8
	};

	enum EntityState {
		ENTITY_STANCE = 0,
		ENTITY_MOVE = 1,
		ENTITY_POWER = 2,
		ENTITY_SPAWN = 3,
		ENTITY_BLOCK = 4,
		ENTITY_HIT = 5,
		ENTITY_DEAD = 6,
		ENTITY_CRITDEAD = 7
	};

	enum CombatStyle {
		COMBAT_DEFAULT = 0,
		COMBAT_AGGRESSIVE = 1,
		COMBAT_PASSIVE = 2
	};

	class AIPower {
	public:
		int type;
		PowerID id;
		int chance;
		Timer cooldown;

		AIPower()
			: type(AI_POWER_MELEE)
			, id(0)
			, chance(0)
			, cooldown()
		{}
	};

	static const bool CAN_USE_PASSIVE = true;
	static const bool TAKE_DMG_CRIT = true;

	static const float DIRECTION_DELTA_X[8];
	static const float DIRECTION_DELTA_Y[8];
	static const float SPEED_MULTIPLIER[8];

	static size_t getFullStatCount();

	StatBlock();
	~StatBlock();

	void load(const std::string& filename);
	void takeDamage(float dmg, bool crit, int source_type);
	void recalc();
	void applyEffects();
	void calcBase();
	void logic();
	void removeSummons();
	void removeFromSummons();
	bool summonLimitReached(PowerID power_id) const;
	void setWanderArea(int r);
	void loadHeroSFX();
	std::string getShortClass();
	std::string getLongClass();
	void addXP(int amount); // TODO this should be unsigned long?
	AIPower* getAIPower(int ai_type);
	int getPowerCooldown(PowerID power_id);
	void setPowerCooldown(PowerID power_id, int power_cooldown);

	bool loadRenderLayerStat(FileParser *infile);
	bool loadAnimationSlotStat(FileParser *infile);

	bool alive;
	bool corpse; // creature is dead and done animating
	Timer corpse_timer;
	bool hero; // else, enemy or other
	bool hero_ally;
	bool enemy_ally;
	bool npc;
	bool humanoid; // true for human, sceleton...; false for wyvern, snake...
	bool lifeform;
	bool permadeath;
	bool transformed;
	bool refresh_stats;
	bool converted;
	bool summoned;
	PowerID summoned_power_index;
	bool encountered; // enemy only
	StatBlock* target_corpse;
	StatBlock* target_nearest;
	StatBlock* target_nearest_corpse;
	float target_nearest_dist;
	float target_nearest_corpse_dist;
	PowerID block_power;

	int movement_type;
	bool facing; // does this creature turn to face the hero

	std::vector<std::string> categories;

	std::string name;

	int level;
	unsigned long xp;
	XPScalingTableID xp_scaling_table;
	bool level_up;
	bool check_title;
	int stat_points_per_level;
	int power_points_per_level;

	// base stats ("attributes")
	std::vector<int> primary;
	std::vector<int> primary_starting;

	// combat stats
	std::vector<float> starting; // default level 1 values per stat. Read from file and never changes at runtime.
	std::vector<float> base; // values before any active effects are applied
	std::vector<float> current; // values after all active effects are applied
	std::vector<float> per_level; // value increases each level after level 1
	std::vector< std::vector<float> > per_primary;

	float get(Stats::STAT stat) const {
		if (stat == Stats::ABS_MAX)
			return std::max(current[stat], current[Stats::ABS_MIN]);
		else
			return current[stat];
	}
	float getDamageMin(size_t dmg_type) const;
	float getDamageMax(size_t dmg_type) const;
	float getDamageResist(size_t dmg_type) const;
	float getResourceStat(size_t resource_index, size_t field_offset) const;

	// additional values to base stats, given by items
	std::vector<int> primary_additional;

	// getter for full base stats (character + additional)
	int get_primary(size_t index) const {
		return primary[index] + primary_additional[index];
	}

	// Base class picked when starting a new game. Defaults to "Adventurer".
	std::string character_class;
	// Class derived from certain properties defined in engine/titles.txt
	std::string character_subclass;

	// physical stats
	float hp;

	// mental stats
	float mp;

	std::vector<float> resource_stats;

	float speed_default;

	// additional damage and absorb granted from items
	std::vector<FMinMax> item_base_dmg;
	FMinMax item_base_abs;

	float speed;
	float charge_speed;

	std::set<std::string> equip_flags;

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

	Timer cooldown_hit;
	bool cooldown_hit_enabled;

	// state
	int cur_state;
	Timer state_timer;
	bool hold_state;
	bool prevent_interrupt;

	// waypoint patrolling
	std::queue<FPoint> waypoints;
	Timer waypoint_timer;

	// wandering area
	bool wander;
	Rect wander_area;

	// enemy behavioral stats
	float chance_pursue;
	float chance_flee;

	std::vector<PowerID> powers_list;
	std::vector<PowerID> powers_list_items;
	std::vector<PowerID> powers_passive;
	std::vector<AIPower> powers_ai;

	bool canUsePower(PowerID powerid, bool allow_passive) const;

	float melee_range;
	float threat_range;
	float threat_range_far;
	float flee_range;
	int combat_style; // determines how the creature enters combat
	float hero_stealth;
	int turn_delay;
	bool in_combat;
	bool join_combat;
	Timer cooldown; // global cooldown
	AIPower* activated_power;
	bool half_dead_power;
	bool suppress_hp; // hide an enemy HP bar
	Timer flee_timer;
	Timer flee_cooldown_timer;
	bool perfect_accuracy; // prevents misses & overhits; used for Event powers

	std::vector<EventComponent> loot_table;
	Point loot_count;

	// for the teleport spell
	bool teleportation;
	FPoint teleport_destination;

	// for purchasing tracking
	int currency;

	// marked for death
	bool death_penalty;

	// Campaign event interaction
	StatusID defeat_status;
	StatusID convert_status;
	StatusID quest_loot_requires_status;
	StatusID quest_loot_requires_not_status;
	ItemID quest_loot_id;
	ItemID first_defeat_loot;

	// player look options
	std::string gfx_base; // folder in /images/avatar
	std::string gfx_head; // png in /images/avatar/[base]
	std::string gfx_portrait; // png in /images/portraits
	std::string transform_type;

	std::string animations;

	// default sounds
	std::vector<std::pair<std::string, std::vector<std::string> > > sfx_attack;
	std::string sfx_step;
	std::vector<std::string> sfx_hit;
	std::vector<std::string> sfx_die;
	std::vector<std::string> sfx_critdie;
	std::vector<std::string> sfx_block;
	std::string sfx_levelup;
	std::string sfx_lowhp;
	bool sfx_lowhp_loop;

	// formula numbers
	int max_spendable_stat_points;
	int max_points_per_stat;

	// preserve state before calcs
	float prev_maxhp;
	float prev_maxmp;
	float prev_hp;
	float prev_mp;

	std::vector<float> prev_max_resource_stats;
	std::vector<float> prev_resource_stats;

	// links to summoned creatures and the entity which summoned this
	std::vector<StatBlock*> summons;
	StatBlock* summoner;
	std::queue<PowerID> party_buffs;

	std::vector<PowerID> power_filter;

	std::vector<EventComponent> invincible_requirements;

	bool abort_npc_interact;

	std::vector<std::string> layer_reference_order;
	std::vector<std::vector<unsigned> > layer_def;

	std::map<std::string, std::string> animation_slots;

	bool critdie_enabled;
};

#endif

