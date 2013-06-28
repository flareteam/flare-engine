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
 * class StatBlock
 *
 * Character stats and calculations
 */

#include "StatBlock.h"
#include "FileParser.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"
#include "MapCollision.h"
#include "MenuPowers.h"
#include "UtilsMath.h"
#include <limits>

using namespace std;

StatBlock::StatBlock()
	: statsLoaded(false)
	, alive(true)
	, corpse(false)
	, corpse_ticks(0)
	, hero(false)
	, hero_ally(false)
	, humanoid(false)
	, permadeath(false)
	, transformed(false)
	, refresh_stats(false)
	, converted(false)
	, summoned(false)
	, summoned_power_index(0)
	, movement_type(MOVEMENT_NORMAL)
	, flying(false)
	, intangible(false)
	, facing(true)
	, name("")
	, sfx_prefix("")
	, level(0)
	, xp(0)
	//int xp_table[MAX_CHARACTER_LEVEL+1];
	, level_up(false)
	, check_title(false)
	, stat_points_per_level(1)
	, power_points_per_level(1)
	, offense_character(0)
	, defense_character(0)
	, physical_character(0)
	, mental_character(0)
	, starting()
	, base()
	, current()
	, per_level()
	, per_physical()
	, per_mental()
	, per_offense()
	, per_defense()
	, offense_additional(0)
	, defense_additional(0)
	, physical_additional(0)
	, mental_additional(0)
	, character_class("")
	, hp(0)
	, hp_ticker(0)
	, mp(0)
	, mp_ticker(0)
	, dmg_melee_min_default(1)
	, dmg_melee_max_default(4)
	, dmg_ment_min_default(1)
	, dmg_ment_max_default(4)
	, dmg_ranged_min_default(1)
	, dmg_ranged_max_default(4)
	, absorb_min_default(0)
	, absorb_max_default(0)
	, speed_default(14)
	, dspeed_default(10)
	, dmg_melee_min_add(0)
	, dmg_melee_max_add(0)
	, dmg_ment_min_add(0)
	, dmg_ment_max_add(0)
	, dmg_ranged_min_add(0)
	, dmg_ranged_max_add(0)
	, absorb_min_add(0)
	, absorb_max_add(0)
	, speed(14)
	, dspeed(10)
	, transform_duration(0)
	, transform_duration_total(0)
	, manual_untransform(false)
	, transform_with_equipment(false)
	, effects()
	, pos()
	, forced_speed()
	, direction(0)
	, hero_cooldown(POWER_COUNT, 0) // hero only
	, cooldown_hit(0)
	, cooldown_hit_ticks(0)
	, cur_state(0)
	, waypoints()		// enemy only
	, waypoint_pause(0)				// enemy only
	, waypoint_pause_ticks(0)		// enemy only
	, wander(false)					// enemy only
	, wander_area()		// enemy only
	, wander_ticks(0)				// enemy only
	, wander_pause_ticks(0)			// enemy only
	, chance_pursue(0)
	, chance_flee(0) // enemy only
	, powers_list()	// hero only
	, powers_list_items()	// hero only
	, powers_passive()
	, power_chance(POWERSLOT_COUNT, 0)		// enemy only
	, power_index(POWERSLOT_COUNT, 0)		// both
	, power_cooldown(POWERSLOT_COUNT, 0)	// enemy only
	, power_ticks(POWERSLOT_COUNT, 0)		// enemy only
	, melee_range(64) //both
	, threat_range(0)  // enemy
	, passive_attacker(false)//enemy
	, hero_stealth(0)
	, last_seen(-1, -1)  // no effects to gameplay?
	, turn_delay(0)
	, turn_ticks(0)
	, in_combat(false)  //enemy only
	, join_combat(false)
	, cooldown_ticks(0)
	, cooldown(0)
	, activated_powerslot(0)// enemy only
	, on_half_dead_casted(false) // enemy only
	, suppress_hp(false)
	, teleportation(false)
	, teleport_destination()
	, melee_weapon_power(0)
	, mental_weapon_power(0)
	, ranged_weapon_power(0)
	, currency(0)
	, death_penalty(false)
	, defeat_status("")			// enemy only
	, quest_loot_requires("")	// enemy only
	, quest_loot_not("")		// enemy only
	, quest_loot_id(0)			// enemy only
	, first_defeat_loot(0)		// enemy only
	, gfx_base("male")
	, gfx_head("head_short")
	, gfx_portrait("male01")
	, transform_type("")
	, animations("")
	, sfx_step("cloth")
	, prev_maxhp(0)
	, prev_maxmp(0)
	, pres_hp(0)
	, pres_mp(0)
    , summons()
	, summoner(NULL) {
	max_spendable_stat_points = 0;
	max_points_per_stat = 0;

	// xp table
	// default to MAX_INT
	for (int i=0; i<MAX_CHARACTER_LEVEL; i++) {
		xp_table[i] = std::numeric_limits<int>::max();
	}

	vulnerable = std::vector<int>(ELEMENTS.size(), 100);
	vulnerable_base = std::vector<int>(ELEMENTS.size(), 100);

	activated_powerslot = 0;
	on_half_dead_casted = false;
}

bool sortLoot(const EnemyLoot &a, const EnemyLoot &b) {
	return a.chance < b.chance;
}

bool StatBlock::loadCoreStat(FileParser *infile){

    int value = 0;
    if (isInt(infile->val)) value = toInt(infile->val);

    if (infile->key == "speed") {
        speed = speed_default = value;
        return true;
    }
    else if (infile->key == "dspeed") {
        dspeed = dspeed_default = value;
        return true;
    }
    else if (infile->key == "categories") {
        string cat;
        while ((cat = infile->nextValue()) != "") {
            categories.push_back(cat);
        }
        return true;
    }
    else {
        for (unsigned i=0; i<STAT_COUNT; i++) {
            if (infile->key == STAT_NAME[i]) {starting[i] = value;return true;}
            else if (infile->key == STAT_NAME[i] + "_per_level") {per_level[i] = value;return true;}
            else if (infile->key == STAT_NAME[i] + "_per_physical") {per_physical[i] = value;return true;}
            else if (infile->key == STAT_NAME[i] + "_per_mental") {per_mental[i] = value;return true;}
            else if (infile->key == STAT_NAME[i] + "_per_offense") {per_offense[i] = value;return true;}
            else if (infile->key == STAT_NAME[i] + "_per_defense") {per_defense[i] = value;return true;}
        }

        for (unsigned int i=0; i<ELEMENTS.size(); i++) {
			if (infile->key == "vulnerable_" + ELEMENTS[i].name) {
				vulnerable[i] = vulnerable_base[i] = value;
				return true;
			}
		}
    }

    return false;
}

/**
 * load a statblock, typically for an enemy definition
 */
void StatBlock::load(const string& filename) {
	FileParser infile;
	if (!infile.open(filename))
		return;

	int num = 0;
	string loot_token;

	while (infile.next()) {
		if (isInt(infile.val)) num = toInt(infile.val);
		bool valid = loadCoreStat(&infile);

		if (infile.key == "name") name = msg->get(infile.val);
		else if (infile.key == "humanoid") humanoid = toBool(infile.val);
		else if (infile.key == "sfx_prefix") sfx_prefix = infile.val;

		else if (infile.key == "level") level = num;

		// enemy death rewards and events
		else if (infile.key == "xp") xp = num;
		else if (infile.key == "loot") {

			// loot entries format:
			// loot=[id],[percent_chance]
			// optionally allow range:
			// loot=[id],[percent_chance],[count_min],[count_max]

			EnemyLoot el;
			std::string loot_id = infile.nextValue();

			// id 0 means currency. The keyword "currency" can also be used.
			if (loot_id == "currency")
				el.id = 0;
			else
				el.id = toInt(loot_id);
			el.chance = toInt(infile.nextValue());

			// check for optional range.
			loot_token = infile.nextValue();
			if (loot_token != "") {
				el.count_min = toInt(loot_token);
				el.count_max = el.count_min;
			}
			loot_token = infile.nextValue();
			if (loot_token != "") {
				el.count_max = toInt(loot_token);
			}

			loot.push_back(el);
		}
		else if (infile.key == "defeat_status") defeat_status = infile.val;
		else if (infile.key == "first_defeat_loot") first_defeat_loot = num;
		else if (infile.key == "quest_loot") {
			quest_loot_requires = infile.nextValue();
			quest_loot_not = infile.nextValue();
			quest_loot_id = toInt(infile.nextValue());
		}
		// combat stats
		else if (infile.key == "cooldown") cooldown = parse_duration(infile.val);

		// behavior stats
		else if (infile.key == "flying") flying = toBool(infile.val);
		else if (infile.key == "intangible") intangible = toBool(infile.val);
		else if (infile.key == "facing") facing = toBool(infile.val);

		else if (infile.key == "waypoint_pause") waypoint_pause = num;

		else if (infile.key == "turn_delay") turn_delay = num;
		else if (infile.key == "chance_pursue") chance_pursue = num;
		else if (infile.key == "chance_flee") chance_flee = num;

		else if (infile.key == "chance_melee_phys") power_chance[MELEE_PHYS] = num;
		else if (infile.key == "chance_melee_ment") power_chance[MELEE_MENT] = num;
		else if (infile.key == "chance_ranged_phys") power_chance[RANGED_PHYS] = num;
		else if (infile.key == "chance_ranged_ment") power_chance[RANGED_MENT] = num;
		else if (infile.key == "power_melee_phys") power_index[MELEE_PHYS] = num;
		else if (infile.key == "power_melee_ment") power_index[MELEE_MENT] = num;
		else if (infile.key == "power_ranged_phys") power_index[RANGED_PHYS] = num;
		else if (infile.key == "power_ranged_ment") power_index[RANGED_MENT] = num;
		else if (infile.key == "power_beacon") power_index[BEACON] = num;
		else if (infile.key == "cooldown_melee_phys") power_cooldown[MELEE_PHYS] = parse_duration(infile.val);
		else if (infile.key == "cooldown_melee_ment") power_cooldown[MELEE_MENT] = parse_duration(infile.val);
		else if (infile.key == "cooldown_ranged_phys") power_cooldown[RANGED_PHYS] = parse_duration(infile.val);
		else if (infile.key == "cooldown_ranged_ment") power_cooldown[RANGED_MENT] = parse_duration(infile.val);
		else if (infile.key == "power_on_hit") power_index[ON_HIT] = num;
		else if (infile.key == "power_on_death") power_index[ON_DEATH] = num;
		else if (infile.key == "power_on_half_dead") power_index[ON_HALF_DEAD] = num;
		else if (infile.key == "power_on_debuff") power_index[ON_DEBUFF] = num;
		else if (infile.key == "power_on_join_combat") power_index[ON_JOIN_COMBAT] = num;
		else if (infile.key == "chance_on_hit") power_chance[ON_HIT] = num;
		else if (infile.key == "chance_on_death") power_chance[ON_DEATH] = num;
		else if (infile.key == "chance_on_half_dead") power_chance[ON_HALF_DEAD] = num;
		else if (infile.key == "chance_on_debuff") power_chance[ON_DEBUFF] = num;
		else if (infile.key == "chance_on_join_combat") power_chance[ON_JOIN_COMBAT] = num;
		else if (infile.key == "cooldown_hit") cooldown_hit = num;

		else if (infile.key == "passive_powers") {
			std::string p = infile.nextValue();
			while (p != "") {
				powers_passive.push_back(toInt(p));
				p = infile.nextValue();
			}
		}

		else if (infile.key == "melee_range") melee_range = num;
		else if (infile.key == "threat_range") threat_range = num;
		else if (infile.key == "passive_attacker") passive_attacker = toBool(infile.val);

		// animation stats
		else if (infile.key == "melee_weapon_power") melee_weapon_power = num;
		else if (infile.key == "mental_weapon_power") mental_weapon_power = num;
		else if (infile.key == "ranged_weapon_power") ranged_weapon_power = num;

		else if (infile.key == "animations") animations = infile.val;

		// hide enemy HP bar
		else if (infile.key == "suppress_hp") suppress_hp = toBool(infile.val);
		// this is only used for EnemyGroupManager
		// we check for them here so that we don't get an error saying they are invalid
		else if (infile.key == "rarity") valid = true;

		else if (!valid) {
			fprintf(stderr, "%s=%s not a valid StatBlock parameter\n", infile.key.c_str(), infile.val.c_str());
		}
	}
	infile.close();

	hp = starting[STAT_HP_MAX];
	mp = starting[STAT_MP_MAX];

	// sort loot table
	std::sort(loot.begin(), loot.end(), sortLoot);

	applyEffects();
}

/**
 * Reduce temphp first, then hp
 */
void StatBlock::takeDamage(int dmg) {
	hp -= effects.damageShields(dmg);
	if (hp <= 0) {
		hp = 0;
	}
}

/**
 * Recalc level and stats
 * Refill HP/MP
 * Creatures might skip these formulas.
 */
void StatBlock::recalc() {

	if (!statsLoaded) loadHeroStats();

	refresh_stats = true;

	level = 0;
	for (int i=0; i<MAX_CHARACTER_LEVEL; i++) {
		if (xp >= xp_table[i]) {
			level=i+1;
			check_title = true;
		}
	}

	applyEffects();

	hp = get(STAT_HP_MAX);
	mp = get(STAT_MP_MAX);
}

/**
 * Base damage and absorb is 0
 * Plus an optional bonus_per_[base stat]
 */
void StatBlock::calcBase() {
	// bonuses are skipped for the default level 1 of a stat
	int lev0 = level -1;
	int phys0 = get_physical() -1;
	int ment0 = get_mental() -1;
	int off0 = get_offense() -1;
	int def0 = get_defense() -1;

	clampFloor(lev0,0);
	clampFloor(phys0,0);
	clampFloor(ment0,0);
	clampFloor(off0,0);
	clampFloor(def0,0);

	for (int i=0; i<STAT_COUNT; i++) {
		base[i] = starting[i];
		base[i] += lev0 * per_level[i];
		base[i] += phys0 * per_physical[i];
		base[i] += ment0 * per_mental[i];
		base[i] += off0 * per_offense[i];
		base[i] += def0 * per_defense[i];
	}

	// add damage/absorb from equipment
	base[STAT_DMG_MELEE_MIN] += dmg_melee_min_add;
	base[STAT_DMG_MELEE_MAX] += dmg_melee_max_add;
	base[STAT_DMG_MENT_MIN] += dmg_ment_min_add;
	base[STAT_DMG_MENT_MAX] += dmg_ment_max_add;
	base[STAT_DMG_RANGED_MIN] += dmg_ranged_min_add;
	base[STAT_DMG_RANGED_MAX] += dmg_ranged_max_add;
	base[STAT_ABS_MIN] += absorb_min_add;
	base[STAT_ABS_MAX] += absorb_max_add;

	// increase damage and absorb to minimum amounts
    clampFloor(base[STAT_DMG_MELEE_MIN], dmg_melee_min_default);
    clampFloor(base[STAT_DMG_MELEE_MAX], dmg_melee_max_default);
    clampFloor(base[STAT_DMG_RANGED_MIN], dmg_ranged_min_default);
    clampFloor(base[STAT_DMG_RANGED_MAX], dmg_ranged_max_default);
    clampFloor(base[STAT_DMG_MENT_MIN], dmg_ment_min_default);
    clampFloor(base[STAT_DMG_MENT_MAX], dmg_ment_max_default);
    clampFloor(base[STAT_ABS_MIN], absorb_min_default);
    clampFloor(base[STAT_ABS_MAX], absorb_max_default);
}

/**
 * Recalc derived stats from base stats + effect bonuses
 */
void StatBlock::applyEffects() {

	// preserve hp/mp states
	prev_maxhp = get(STAT_HP_MAX);
	prev_maxmp = get(STAT_MP_MAX);
	pres_hp = hp;
	pres_mp = mp;

    // calculate primary stats
    // refresh the character menu if there has been a change
    if (get_physical() != physical_character + effects.bonus_physical ||
            get_mental() != mental_character + effects.bonus_mental ||
            get_offense() != offense_character + effects.bonus_offense ||
            get_defense() != defense_character + effects.bonus_defense) refresh_stats = true;

    offense_additional = effects.bonus_offense;
    defense_additional = effects.bonus_defense;
    physical_additional = effects.bonus_physical;
    mental_additional = effects.bonus_mental;

	calcBase();

    for (int i=0; i<STAT_COUNT; i++) {
        current[i] = base[i] + effects.bonus[i];
    }

	for (unsigned i=0; i<effects.bonus_resist.size(); i++) {
		vulnerable[i] = vulnerable_base[i] - effects.bonus_resist[i];
	}

	current[STAT_HP_MAX] += (current[STAT_HP_MAX] * current[STAT_HP_PERCENT]) / 100;
	current[STAT_MP_MAX] += (current[STAT_MP_MAX] * current[STAT_MP_PERCENT]) / 100;

	if (hp > get(STAT_HP_MAX)) hp = get(STAT_HP_MAX);
	if (mp > get(STAT_MP_MAX)) mp = get(STAT_MP_MAX);

	speed = speed_default;
	dspeed = dspeed_default;
}

/**
 * Process per-frame actions
 */
void StatBlock::logic() {
	if (hp <= 0 && !effects.triggered_death && !effects.revive) alive = false;
	else alive = true;

	// handle effect timers
	effects.logic();

	// apply bonuses from items/effects to base stats
	applyEffects();

	// preserve ratio on maxmp and maxhp changes
	float ratio;
	if (prev_maxhp != get(STAT_HP_MAX)) {
		ratio = (float)pres_hp / (float)prev_maxhp;
		hp = (int)(ratio * get(STAT_HP_MAX));
	}
	if (prev_maxmp != get(STAT_MP_MAX)) {
		ratio = (float)pres_mp / (float)prev_maxmp;
		mp = (int)(ratio * get(STAT_MP_MAX));
	}

	// handle cooldowns
	if (cooldown_ticks > 0) cooldown_ticks--; // global cooldown

	for (int i=0; i<POWERSLOT_COUNT; i++) { // NPC/enemy powerslot cooldown
		if (power_ticks[i] > 0) power_ticks[i]--;
	}

	// HP regen
	if (get(STAT_HP_REGEN) > 0 && hp < get(STAT_HP_MAX) && hp > 0) {
		hp_ticker++;
		if (hp_ticker >= (60 * MAX_FRAMES_PER_SEC)/get(STAT_HP_REGEN)) {
			hp++;
			hp_ticker = 0;
		}
	}

	// MP regen
	if (get(STAT_MP_REGEN) > 0 && mp < get(STAT_MP_MAX) && hp > 0) {
		mp_ticker++;
		if (mp_ticker >= (60 * MAX_FRAMES_PER_SEC)/get(STAT_MP_REGEN)) {
			mp++;
			mp_ticker = 0;
		}
	}

	// handle buff/debuff durations
	if (transform_duration > 0)
		transform_duration--;

	// apply bleed
	if (effects.damage > 0) {
		takeDamage(effects.damage);
	}

	if(effects.death_sentence)
		hp = 0;

	if(cooldown_hit_ticks > 0)
		cooldown_hit_ticks--;

	// apply healing over time
	if (effects.hpot > 0) {
		comb->addMessage(msg->get("+%d HP",effects.hpot), pos, COMBAT_MESSAGE_BUFF);
		hp += effects.hpot;
		if (hp > get(STAT_HP_MAX)) hp = get(STAT_HP_MAX);
	}
	if (effects.mpot > 0) {
		comb->addMessage(msg->get("+%d MP",effects.mpot), pos, COMBAT_MESSAGE_BUFF);
		mp += effects.mpot;
		if (mp > get(STAT_MP_MAX)) mp = get(STAT_MP_MAX);
	}

	// set movement type
	// some creatures may shift between movement types
	if (intangible) movement_type = MOVEMENT_INTANGIBLE;
	else if (flying) movement_type = MOVEMENT_FLYING;
	else movement_type = MOVEMENT_NORMAL;
}

StatBlock::~StatBlock() {
    removeFromSummons();
}

bool StatBlock::canUsePower(const Power &power, unsigned powerid) const {

	// needed to unlock shapeshifter powers
	if (transformed) return mp >= power.requires_mp;

	//don't use untransform power if hero is not transformed
	else if (power.spawn_type == "untransform" && !transformed) return false;
	else {
		return std::includes(equip_flags.begin(), equip_flags.end(), power.requires_flags.begin(), power.requires_flags.end())
			   && mp >= power.requires_mp
			   && (!power.sacrifice == false || hp > power.requires_hp)
			   && menu_powers->meetsUsageStats(powerid)
			   && !power.passive
			   && (power.type == POWTYPE_SPAWN ? !summonLimitReached(powerid) : true);
	}

}

void StatBlock::loadHeroStats() {
	// Redefine numbers from config file if present
	FileParser infile;
	if (!infile.open("engine/stats.txt"))
		return;

	while (infile.next()) {
		int value = toInt(infile.val);

		loadCoreStat(&infile);

		if (infile.key == "max_points_per_stat") {
			max_points_per_stat = value;
		}
		else if (infile.key == "sfx_step") {
			sfx_step = infile.val;
		}
		else if (infile.key == "stat_points_per_level") {
			stat_points_per_level = value;
		}
		else if (infile.key == "power_points_per_level") {
			power_points_per_level = value;
		}
		else if (infile.key == "cooldown_hit") {
			cooldown_hit = value;
		}
	}
	infile.close();
	if (max_points_per_stat == 0) max_points_per_stat = max_spendable_stat_points / 4 + 1;
	statsLoaded = true;

	// Load the XP table as well
	if (!infile.open("engine/xp_table.txt"))
		return;

	while(infile.next()) {
		xp_table[toInt(infile.key) - 1] = toInt(infile.val);
	}
	max_spendable_stat_points = toInt(infile.key) * stat_points_per_level;
	infile.close();
}

void StatBlock::removeFromSummons() {

    if(summoner != NULL){
        vector<StatBlock*>::iterator parent_ref = find(summoner->summons.begin(), summoner->summons.end(), this);

        if(parent_ref != summoner->summons.end())
            summoner->summons.erase(parent_ref);

        summoner = NULL;
    }

    for (vector<StatBlock*>::iterator it=summons.begin(); it != summons.end(); ++it)
        (*it)->summoner = NULL;

    summons.clear();
}

bool StatBlock::summonLimitReached(int power_id) const{

    //find the limit
    Power *spawn_power = &powers->powers[power_id];

    int max_summons = 0;

    if(spawn_power->spawn_limit_mode == SPAWN_LIMIT_MODE_FIXED)
        max_summons = spawn_power->spawn_limit_qty;
    else if(spawn_power->spawn_limit_mode == SPAWN_LIMIT_MODE_STAT) {
        int stat_val = 1;
        switch(spawn_power->spawn_limit_stat) {
            case SPAWN_LIMIT_STAT_PHYSICAL:
                stat_val = get_physical();
                break;
            case SPAWN_LIMIT_STAT_MENTAL:
                stat_val = get_mental();
                break;
            case SPAWN_LIMIT_STAT_OFFENSE:
                stat_val = get_offense();
                break;
            case SPAWN_LIMIT_STAT_DEFENSE:
                stat_val = get_defense();
                break;
        }
        max_summons = (stat_val / (spawn_power->spawn_limit_every == 0 ? 1 : spawn_power->spawn_limit_every)) * spawn_power->spawn_limit_qty;
    }
    else
        return false;//unlimited or unknown mode

    //if the power is available, there should be at least 1 allowed summon
    if(max_summons < 1) max_summons = 1;


    //find out how many there are currently
    int qty_summons = 0;

    for (unsigned int i=0; i < summons.size(); i++) {
        if(!summons[i]->corpse && summons[i]->summoned_power_index == power_id
                && summons[i]->cur_state != ENEMY_SPAWN
                && summons[i]->cur_state != ENEMY_DEAD
                && summons[i]->cur_state != ENEMY_CRITDEAD) {
            qty_summons++;
        }
    }

    return qty_summons >= max_summons;
}
