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
	, level(0)
	, xp(0)
	, level_up(false)
	, check_title(false)
	, stat_points_per_level(1)
	, power_points_per_level(1)
	, offense_character(0)
	, defense_character(0)
	, physical_character(0)
	, mental_character(0)
	, starting(std::vector<int>(STAT_COUNT,0))
	, base(std::vector<int>(STAT_COUNT,0))
	, current(std::vector<int>(STAT_COUNT,0))
	, per_level(std::vector<int>(STAT_COUNT,0))
	, per_physical(std::vector<int>(STAT_COUNT,0))
	, per_mental(std::vector<int>(STAT_COUNT,0))
	, per_offense(std::vector<int>(STAT_COUNT,0))
	, per_defense(std::vector<int>(STAT_COUNT,0))
	, offense_additional(0)
	, defense_additional(0)
	, physical_additional(0)
	, mental_additional(0)
	, character_class("")
	, character_subclass("")
	, hp(0)
	, hp_ticker(0)
	, mp(0)
	, mp_ticker(0)
	, speed_default(0.1f)
	, dmg_melee_min_add(0)
	, dmg_melee_max_add(0)
	, dmg_ment_min_add(0)
	, dmg_ment_max_add(0)
	, dmg_ranged_min_add(0)
	, dmg_ranged_max_add(0)
	, absorb_min_add(0)
	, absorb_max_add(0)
	, speed(0.1f)
	, vulnerable(ELEMENTS.size(), 100)
	, vulnerable_base(ELEMENTS.size(), 100)
	, transform_duration(0)
	, transform_duration_total(0)
	, manual_untransform(false)
	, transform_with_equipment(false)
	, untransform_on_hit(false)
	, effects()
	, pos()
	, direction(0)
	, cooldown_hit(0)
	, cooldown_hit_ticks(0)
	, cur_state(0)
	, waypoints()		// enemy only
	, waypoint_pause(MAX_FRAMES_PER_SEC)	// enemy only
	, waypoint_pause_ticks(0)		// enemy only
	, wander(false)					// enemy only
	, wander_area()		// enemy only
	, chance_pursue(0)
	, chance_flee(0) // enemy only
	, powers_list()	// hero only
	, powers_list_items()	// hero only
	, powers_passive()
	, power_chance(POWERSLOT_COUNT, 0)		// enemy only
	, power_index(POWERSLOT_COUNT, 0)		// both
	, power_cooldown(POWERSLOT_COUNT, 0)	// enemy only
	, power_ticks(POWERSLOT_COUNT, 0)		// enemy only
	, melee_range(1.0f) //both
	, threat_range(0)  // enemy
	, passive_attacker(false)//enemy
	, hero_stealth(0)
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
	, currency(0)
	, death_penalty(false)
	, defeat_status("")			// enemy only
	, convert_status("")		// enemy only
	, quest_loot_requires_status("")	// enemy only
	, quest_loot_requires_not_status("")		// enemy only
	, quest_loot_id(0)			// enemy only
	, first_defeat_loot(0)		// enemy only
	, gfx_base("male")
	, gfx_head("head_short")
	, gfx_portrait("")
	, transform_type("")
	, animations("")
	, sfx_step("")
	, sfx_phys("")
	, sfx_ment("")
	, sfx_hit("")
	, sfx_die("")
	, sfx_critdie("")
	, sfx_block("")
	, sfx_levelup("")
	, max_spendable_stat_points(0)
	, max_points_per_stat(0)
	, prev_maxhp(0)
	, prev_maxmp(0)
	, pres_hp(0)
	, pres_mp(0)
	, summons()
	, summoner(NULL)
	, attacking(false) {
}

bool StatBlock::loadCoreStat(FileParser *infile) {
	// @CLASS StatBlock: Core stats|Description of engine/stats.txt and enemies in enemies/

	int value = toInt(infile->val, 0);
	float fvalue = toFloat(infile->val, 0);

	if (infile->key == "speed") {
		// @ATTR speed|float|Movement speed
		speed = speed_default = fvalue / MAX_FRAMES_PER_SEC;
		return true;
	}
	else {
		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (infile->key == STAT_NAME[i]) {
				// @ATTR $STATNAME|integer|The starting value for this stat.
				starting[i] = value;
				return true;
			}
			else if (infile->key == STAT_NAME[i] + "_per_level") {
				// @ATTR $STATNAME_per_level|integer|The value for this stat added per level.
				per_level[i] = value;
				return true;
			}
			else if (infile->key == STAT_NAME[i] + "_per_physical") {
				// @ATTR $STATNAME_per_physical|integer|The value for this stat added per Physical.
				per_physical[i] = value;
				return true;
			}
			else if (infile->key == STAT_NAME[i] + "_per_mental") {
				// @ATTR $STATNAME_per_mental|integer|The value for this stat added per Mental.
				per_mental[i] = value;
				return true;
			}
			else if (infile->key == STAT_NAME[i] + "_per_offense") {
				// @ATTR $STATNAME_per_offense|integer|The value for this stat added per Offense.
				per_offense[i] = value;
				return true;
			}
			else if (infile->key == STAT_NAME[i] + "_per_defense") {
				// @ATTR $STATNAME_per_defense|integer|The value for this stat added per Defense.
				per_defense[i] = value;
				return true;
			}
		}

		for (unsigned int i=0; i<ELEMENTS.size(); i++) {
			if (infile->key == "vulnerable_" + ELEMENTS[i].name) {
				// @ATTR vulnerable_$ELEMENT|integer|Percentage weakness to this element.
				vulnerable[i] = vulnerable_base[i] = value;
				return true;
			}
		}
	}

	return false;
}

/**
 * Set paths for sound effects
 */
bool StatBlock::loadSfxStat(FileParser *infile) {
	// @CLASS StatBlock: Sound effects|Description of heroes in engine/avatar/ and enemies in enemies/

	// @ATTR sfx_phys|string|Filename of sound effect for physical attack.
	if (infile->key == "sfx_phys") sfx_phys = infile->val;
	// @ATTR sfx_ment|string|Filename of sound effect for mental attack.
	else if (infile->key == "sfx_ment") sfx_ment = infile->val;
	// @ATTR sfx_hit|string|Filename of sound effect for being hit.
	else if (infile->key == "sfx_hit") sfx_hit = infile->val;
	// @ATTR sfx_die|string|Filename of sound effect for dying.
	else if (infile->key == "sfx_die") sfx_die = infile->val;
	// @ATTR sfx_critdie|string|Filename of sound effect for dying to a critical hit.
	else if (infile->key == "sfx_critdie") sfx_critdie = infile->val;
	// @ATTR sfx_block|string|Filename of sound effect for blocking an incoming hit.
	else if (infile->key == "sfx_block") sfx_block = infile->val;
	// @ATTR sfx_levelup|string|Filename of sound effect for leveling up.
	else if (infile->key == "sfx_levelup") sfx_levelup = infile->val;
	else return false;

	return true;
}

/**
 * load a statblock, typically for an enemy definition
 */
void StatBlock::load(const string& filename) {
	// @CLASS StatBlock: Enemies|Description of enemies in enemies/
	FileParser infile;
	if (!infile.open(filename))
		return;

	while (infile.next()) {
		int num = toInt(infile.val);
		float fnum = toFloat(infile.val);
		bool valid = loadCoreStat(&infile) || loadSfxStat(&infile);

		// @ATTR name|string|Name
		if (infile.key == "name") name = msg->get(infile.val);
		// @ATTR humanoid|boolean|This creature gives human traits when transformed into, such as the ability to talk with NPCs.
		else if (infile.key == "humanoid") humanoid = toBool(infile.val);

		// @ATTR level|integer|Level
		else if (infile.key == "level") level = num;

		// enemy death rewards and events
		// @ATTR xp|integer|XP awarded upon death.
		else if (infile.key == "xp") xp = num;
		else if (infile.key == "loot") {
			// @ATTR loot|[currency:item (integer)], chance (integer), min (integer), max (integer)|Possible loot that can be dropped on death.

			// loot entries format:
			// loot=[id],[percent_chance]
			// optionally allow range:
			// loot=[id],[percent_chance],[count_min],[count_max]

			loot_table.push_back(Event_Component());
			loot->parseLoot(infile, &loot_table.back(), &loot_table);
		}
		// @ATTR defeat_status|string|Campaign status to set upon death.
		else if (infile.key == "defeat_status") defeat_status = infile.val;
		// @ATTR convert_status|string|Campaign status to set upon being converted to a player ally.
		else if (infile.key == "convert_status") convert_status = infile.val;
		// @ATTR first_defeat_loot|integer|Drops this item upon first death.
		else if (infile.key == "first_defeat_loot") first_defeat_loot = num;
		// @ATTR quest_loot|[requires status (string), requires not status (string), item (integer)|Drops this item when campaign status is met.
		else if (infile.key == "quest_loot") {
			quest_loot_requires_status = infile.nextValue();
			quest_loot_requires_not_status = infile.nextValue();
			quest_loot_id = toInt(infile.nextValue());
		}
		// combat stats
		// @ATTR cooldown|integer|Cooldown between attacks.
		else if (infile.key == "cooldown") cooldown = parse_duration(infile.val);

		// behavior stats
		// @ATTR flying|boolean|Creature can move over gaps/water.
		else if (infile.key == "flying") flying = toBool(infile.val);
		// @ATTR intangible|boolean|Creature can move through walls.
		else if (infile.key == "intangible") intangible = toBool(infile.val);
		// @ATTR facing|boolean|Creature can turn to face their target.
		else if (infile.key == "facing") facing = toBool(infile.val);

		// @ATTR waypoint_pause|duration|Duration to wait at each waypoint.
		else if (infile.key == "waypoint_pause") waypoint_pause = parse_duration(infile.val);

		// @ATTR turn_delay|duration|Duration it takes for this creature to turn and face their target.
		else if (infile.key == "turn_delay") turn_delay = parse_duration(infile.val);
		// @ATTR chance_pursue|integer|Percentage change that the creature will chase their target.
		else if (infile.key == "chance_pursue") chance_pursue = num;
		// @ATTR chance_flee|integer|Percentage chance that the creature will run away from their target.
		else if (infile.key == "chance_flee") chance_flee = num;

		// @ATTR chance_melee_phys|integer|Percentage chance that the creature will use their physical melee power.
		else if (infile.key == "chance_melee_phys") power_chance[MELEE_PHYS] = num;
		// @ATTR chance_melee_ment|integer|Percentage chance that the creature will use their mental melee power.
		else if (infile.key == "chance_melee_ment") power_chance[MELEE_MENT] = num;
		// @ATTR chance_ranged_phys|integer|Percentage chance that the creature will use their physical ranged power.
		else if (infile.key == "chance_ranged_phys") power_chance[RANGED_PHYS] = num;
		// @ATTR chance_ranged_ment|integer|Percentage chance that the creature will use their mental ranged power.
		else if (infile.key == "chance_ranged_ment") power_chance[RANGED_MENT] = num;
		// @ATTR power_melee_phys|integer|Power index for the physical melee power.
		else if (infile.key == "power_melee_phys") power_index[MELEE_PHYS] = num;
		// @ATTR power_melee_ment|integer|Power index for the mental melee power.
		else if (infile.key == "power_melee_ment") power_index[MELEE_MENT] = num;
		// @ATTR power_ranged_phys|integer|Power index for the physical ranged power.
		else if (infile.key == "power_ranged_phys") power_index[RANGED_PHYS] = num;
		// @ATTR power_ranged_ment|integer|Power index for the mental ranged power.
		else if (infile.key == "power_ranged_ment") power_index[RANGED_MENT] = num;
		// @ATTR power_beacon|integer|Power index of a "beacon" power used to aggro nearby creatures.
		else if (infile.key == "power_beacon") power_index[BEACON] = num;
		// @ATTR cooldown_melee_phys|duration|Cooldown after using the physical melee power.
		else if (infile.key == "cooldown_melee_phys") power_cooldown[MELEE_PHYS] = parse_duration(infile.val);
		// @ATTR cooldown_melee_ment|duration|Cooldown after using the mental melee power.
		else if (infile.key == "cooldown_melee_ment") power_cooldown[MELEE_MENT] = parse_duration(infile.val);
		// @ATTR cooldown_ranged_phys|duration|Cooldown after using the physical ranged power.
		else if (infile.key == "cooldown_ranged_phys") power_cooldown[RANGED_PHYS] = parse_duration(infile.val);
		// @ATTR cooldown_ranged_ment|duration|Cooldown after using the mental ranged power.
		else if (infile.key == "cooldown_ranged_ment") power_cooldown[RANGED_MENT] = parse_duration(infile.val);
		// @ATTR power_on_hit|integer|Power index that is triggered when hit.
		else if (infile.key == "power_on_hit") power_index[ON_HIT] = num;
		// @ATTR power_on_death|integer|Power index that is triggered when dead.
		else if (infile.key == "power_on_death") power_index[ON_DEATH] = num;
		// @ATTR power_on_half_dead|integer|Power index that is triggered when at half health.
		else if (infile.key == "power_on_half_dead") power_index[ON_HALF_DEAD] = num;
		// @ATTR power_on_debuff|integer|Power index that is triggered when under a negative status effect.
		else if (infile.key == "power_on_debuff") power_index[ON_DEBUFF] = num;
		// @ATTR power_on_join_combat|integer|Power index that is triggered when initiating combat.
		else if (infile.key == "power_on_join_combat") power_index[ON_JOIN_COMBAT] = num;
		// @ATTR chance_on_hit|integer|Percentage chance that power_on_hit will be triggered.
		else if (infile.key == "chance_on_hit") power_chance[ON_HIT] = num;
		// @ATTR chance_on_death|integer|Percentage chance that power_on_death will be triggered.
		else if (infile.key == "chance_on_death") power_chance[ON_DEATH] = num;
		// @ATTR chance_on_half_dead|integer|Percentage chance that power_on_half_dead will be triggered.
		else if (infile.key == "chance_on_half_dead") power_chance[ON_HALF_DEAD] = num;
		// @ATTR chance_on_debuff|integer|Percentage chance that power_on_debuff will be triggered.
		else if (infile.key == "chance_on_debuff") power_chance[ON_DEBUFF] = num;
		// @ATTR chance_on_join_combat|integer|Percentage chance that power_on_join_combat will be triggered.
		else if (infile.key == "chance_on_join_combat") power_chance[ON_JOIN_COMBAT] = num;
		// @ATTR cooldown_hit|duration|Duration of cooldown after being hit.
		else if (infile.key == "cooldown_hit") cooldown_hit = parse_duration(infile.val);

		else if (infile.key == "passive_powers") {
			// @ATTR passive_powers|power (integer), ...|A list of passive powers this creature has.
			std::string p = infile.nextValue();
			while (p != "") {
				powers_passive.push_back(toInt(p));
				p = infile.nextValue();
			}
		}

		// @ATTR melee_range|float|Minimum distance from target required to use melee powers.
		else if (infile.key == "melee_range") melee_range = fnum;
		// @ATTR threat_range|float|Radius of the area this creature will be able to start chasing the hero.
		else if (infile.key == "threat_range") threat_range = fnum;
		// @ATTR passive_attacker|boolean|Won't initiate combat until attacked.
		else if (infile.key == "passive_attacker") passive_attacker = toBool(infile.val);

		// @ATTR animations|string|Filename of an animation definition.
		else if (infile.key == "animations") animations = infile.val;

		// @ATTR supress_hp|boolean|Hides the enemy HP bar for this creature.
		else if (infile.key == "suppress_hp") suppress_hp = toBool(infile.val);

		else if (infile.key == "categories") {
			// @ATTR categories|category (string), ...|Categories that this enemy belongs to.
			string cat;
			while ((cat = infile.nextValue()) != "") {
				categories.push_back(cat);
			}
		}

		// this is only used for EnemyGroupManager
		// we check for them here so that we don't get an error saying they are invalid
		else if (infile.key == "rarity") ; // but do nothing

		else if (!valid) {
			infile.error("StatBlock: '%s' is not a valid key.", infile.key.c_str());
		}
	}
	infile.close();

	hp = starting[STAT_HP_MAX];
	mp = starting[STAT_MP_MAX];

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
	for (unsigned i=0; i<xp_table.size(); i++) {
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
	clampFloor(base[STAT_DMG_MELEE_MIN], 0);
	clampFloor(base[STAT_DMG_MELEE_MAX], base[STAT_DMG_MELEE_MIN]);
	clampFloor(base[STAT_DMG_RANGED_MIN], 0);
	clampFloor(base[STAT_DMG_RANGED_MAX], base[STAT_DMG_RANGED_MIN]);
	clampFloor(base[STAT_DMG_MENT_MIN], 0);
	clampFloor(base[STAT_DMG_MENT_MAX], base[STAT_DMG_MENT_MIN]);
	clampFloor(base[STAT_ABS_MIN], 0);
	clampFloor(base[STAT_ABS_MAX], base[STAT_ABS_MIN]);
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
	if (!menu_powers) return false;

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
			   && (power.type == POWTYPE_SPAWN ? !summonLimitReached(powerid) : true)
			   && !power.meta_power
			   && (power.requires_item == -1 || (power.requires_item > 0 && items->requirementsMet(this, power.requires_item)));
	}

}

void StatBlock::loadHeroStats() {
	// Redefine numbers from config file if present
	FileParser infile;
	// @CLASS StatBlock: Hero stats|Description of engine/stats.txt
	if (infile.open("engine/stats.txt")) {
		while (infile.next()) {
			int value = toInt(infile.val);

			bool valid = loadCoreStat(&infile);

			if (infile.key == "max_points_per_stat") {
				// @ATTR max_points_per_stat|integer|Maximum points for each primary stat.
				max_points_per_stat = value;
			}
			else if (infile.key == "sfx_step") {
				// @ATTR sfx_step|string|An id for a set of step sound effects. See items/step_sounds.txt.
				sfx_step = infile.val;
			}
			else if (infile.key == "stat_points_per_level") {
				// @ATTR stat_points_per_level|integer|The amount of stat points awarded each level.
				stat_points_per_level = value;
			}
			else if (infile.key == "power_points_per_level") {
				// @ATTR power_points_per_level|integer|The amount of power points awarded each level.
				power_points_per_level = value;
			}
			else if (!valid) {
				infile.error("StatBlock: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	if (max_points_per_stat == 0) max_points_per_stat = max_spendable_stat_points / 4 + 1;
	statsLoaded = true;

	// load the XP table
	// @CLASS StatBlock: XP table|Description of engine/xp_table.txt
	if (infile.open("engine/xp_table.txt")) {
		while(infile.next()) {
			unsigned key = toInt(infile.key);
			if (key > 0) {
				// @ATTR $LEVEL|integer|The amount of XP required for this level.
				if (key > xp_table.size())
					xp_table.resize(key);
				xp_table[key - 1] = toInt(infile.val);
			}
		}
		infile.close();
	}
	max_spendable_stat_points = xp_table.size() * stat_points_per_level;
}

void StatBlock::loadHeroSFX() {
	// load the paths to base sound effects
	FileParser infile;
	if (infile.open("engine/avatar/"+gfx_base+".txt", true, "")) {
		while(infile.next()) {
			loadSfxStat(&infile);
		}
		infile.close();
	}
}

void StatBlock::removeFromSummons() {

	if(summoner != NULL && !summoner->summons.empty()) {
		vector<StatBlock*>::iterator parent_ref = find(summoner->summons.begin(), summoner->summons.end(), this);

		if(parent_ref != summoner->summons.end())
			summoner->summons.erase(parent_ref);

		summoner = NULL;
	}

	if (!summons.empty()) {
		for (vector<StatBlock*>::iterator it=summons.begin(); it != summons.end(); ++it)
			(*it)->summoner = NULL;

		summons.clear();
	}
}

bool StatBlock::summonLimitReached(int power_id) const {

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

void StatBlock::setWanderArea(int r) {
	wander_area.x = int(floor(pos.x)) - r;
	wander_area.y = int(floor(pos.y)) - r;
	wander_area.w = wander_area.h = (r*2) + 1;
}

/**
 * Returns the short version of the class string
 * For the sake of consistency with previous versions,
 * this means returning the generated subclass
 */
std::string StatBlock:: getShortClass() {
	if (character_subclass == "")
		return msg->get(character_class);
	else
		return msg->get(character_subclass);
}

/**
 * Returns the long version of the class string
 * It contains both the base class and the generated subclass
 */
std::string StatBlock::getLongClass() {
	if (character_subclass == "" || character_class == character_subclass)
		return msg->get(character_class);
	else
		return msg->get(character_class) + " / " + msg->get(character_subclass);
}
