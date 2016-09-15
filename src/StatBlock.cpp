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
#include "EnemyManager.h"
#include "UtilsMath.h"
#include <limits>

StatBlock::StatBlock()
	: statsLoaded(false)
	, alive(true)
	, corpse(false)
	, corpse_ticks(0)
	, hero(false)
	, hero_ally(false)
	, enemy_ally(false)
	, humanoid(false)
	, permadeath(false)
	, transformed(false)
	, refresh_stats(false)
	, converted(false)
	, summoned(false)
	, summoned_power_index(0)
	, encountered(false)
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
	, charge_speed(0.0f)
	, vulnerable(ELEMENTS.size(), 100)
	, vulnerable_base(ELEMENTS.size(), 100)
	, transform_duration(0)
	, transform_duration_total(0)
	, manual_untransform(false)
	, transform_with_equipment(false)
	, untransform_on_hit(false)
	, effects()
	, blocking(false) // hero only
	, pos()
	, knockback_speed()
	, knockback_srcpos()
	, knockback_destpos()
	, direction(0)
	, cooldown_hit(-1)
	, cooldown_hit_ticks(0)
	, cur_state(0)
	, state_ticks(0)
	, hold_state(false)
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
	, powers_ai() // enemy only
	, melee_range(1.0f) //both
	, threat_range(0)  // enemy
	, threat_range_far(0)  // enemy
	, flee_range(0)  // enemy
	, combat_style(COMBAT_DEFAULT)//enemy
	, hero_stealth(0)
	, turn_delay(0)
	, turn_ticks(0)
	, in_combat(false)  //enemy only
	, join_combat(false)
	, cooldown_ticks(0)
	, cooldown(0)
	, activated_power(NULL) // enemy only
	, half_dead_power(false) // enemy only
	, suppress_hp(false)
	, flee_duration(MAX_FRAMES_PER_SEC) // enemy only
	, flee_cooldown(MAX_FRAMES_PER_SEC) // enemy only
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
	, sfx_attack()
	, sfx_step("")
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

	if (infile->key == "speed") {
		// @ATTR speed|float|Movement speed
		float fvalue = toFloat(infile->val, 0);
		speed = speed_default = fvalue / MAX_FRAMES_PER_SEC;
		return true;
	}
	else if (infile->key == "cooldown") {
		// @ATTR cooldown|int|Cooldown between attacks in 'ms' or 's'.
		cooldown = parse_duration(infile->val);
		return true;
	}
	else if (infile->key == "cooldown_hit") {
		// @ATTR cooldown_hit|duration|Duration of cooldown after being hit in 'ms' or 's'.
		cooldown_hit = parse_duration(infile->val);
		return true;
	}
	else if (infile->key == "stat") {
		// @ATTR stat|string, int : Stat name, Value|The starting value for this stat.
		std::string stat = popFirstString(infile->val);
		int value = popFirstInt(infile->val);

		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (STAT_KEY[i] == stat) {
				starting[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "stat_per_level") {
		// @ATTR stat_per_level|predefined_string, int : Stat name, Value|The value for this stat added per level.
		std::string stat = popFirstString(infile->val);
		int value = popFirstInt(infile->val);

		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (STAT_KEY[i] == stat) {
				per_level[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "stat_per_physical") {
		// @ATTR stat_per_physical|predefined_string, int : Stat name, Value|The value for this stat added per Physical.
		std::string stat = popFirstString(infile->val);
		int value = popFirstInt(infile->val);

		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (STAT_KEY[i] == stat) {
				per_physical[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "stat_per_mental") {
		// @ATTR stat_per_mental|predefined_string, int : Stat name, Value|The value for this stat added per Mental.
		std::string stat = popFirstString(infile->val);
		int value = popFirstInt(infile->val);

		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (STAT_KEY[i] == stat) {
				per_mental[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "stat_per_offense") {
		// @ATTR stat_per_offense|predefined_string, int : Stat name, Value|The value for this stat added per Offense.
		std::string stat = popFirstString(infile->val);
		int value = popFirstInt(infile->val);

		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (STAT_KEY[i] == stat) {
				per_offense[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "stat_per_defense") {
		// @ATTR stat_per_defense|predefined_string, int : Stat name, Value|The value for this stat added per Defense.
		std::string stat = popFirstString(infile->val);
		int value = popFirstInt(infile->val);

		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (STAT_KEY[i] == stat) {
				per_defense[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "vulnerable") {
		// @ATTR vulnerable|predefined_string, int : Element, Value|Percentage weakness to this element.
		std::string element = popFirstString(infile->val);
		int value = popFirstInt(infile->val);

		for (unsigned int i=0; i<ELEMENTS.size(); i++) {
			if (element == ELEMENTS[i].id) {
				vulnerable[i] = vulnerable_base[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "power_filter") {
		// @ATTR power_filter|list(power_id)|Only these powers are allowed to hit this entity.
		std::string power_id = popFirstString(infile->val);
		while (!power_id.empty()) {
			power_filter.push_back(toInt(power_id));
			power_id = popFirstString(infile->val);
		}
	}

	return false;
}

/**
 * Set paths for sound effects
 */
bool StatBlock::loadSfxStat(FileParser *infile) {
	// @CLASS StatBlock: Sound effects|Description of heroes in engine/avatar/ and enemies in enemies/

	// @ATTR sfx_attack|predefined_string, filename : Animation name, Sound file|Filename of sound effect for the specified attack animation.
	if (infile->key == "sfx_attack") {
		std::string anim_name = popFirstString(infile->val);
		std::string filename = popFirstString(infile->val);

		bool found_anim_name = false;
		for (size_t i = 0; i < sfx_attack.size(); ++i) {
			if (anim_name == sfx_attack[i].first) {
				sfx_attack[i].second = filename;
				break;
			}
		}

		if (!found_anim_name) {
			sfx_attack.push_back(std::pair<std::string, std::string>(anim_name, filename));
		}
	}
	// @ATTR sfx_hit|filename|Filename of sound effect for being hit.
	else if (infile->key == "sfx_hit") sfx_hit = infile->val;
	// @ATTR sfx_die|filename|Filename of sound effect for dying.
	else if (infile->key == "sfx_die") sfx_die = infile->val;
	// @ATTR sfx_critdie|filename|Filename of sound effect for dying to a critical hit.
	else if (infile->key == "sfx_critdie") sfx_critdie = infile->val;
	// @ATTR sfx_block|filename|Filename of sound effect for blocking an incoming hit.
	else if (infile->key == "sfx_block") sfx_block = infile->val;
	// @ATTR sfx_levelup|filename|Filename of sound effect for leveling up.
	else if (infile->key == "sfx_levelup") sfx_levelup = infile->val;
	else return false;

	return true;
}

/**
 * load a statblock, typically for an enemy definition
 */
void StatBlock::load(const std::string& filename) {
	// @CLASS StatBlock: Enemies|Description of enemies in enemies/
	FileParser infile;
	if (!infile.open(filename))
		return;

	bool clear_loot = true;
	bool flee_range_defined = false;

	while (infile.next()) {
		if (infile.new_section) {
			// APPENDed file
			clear_loot = true;
		}

		int num = toInt(infile.val);
		float fnum = toFloat(infile.val);
		bool valid = loadCoreStat(&infile) || loadSfxStat(&infile);

		// @ATTR name|string|Name
		if (infile.key == "name") name = msg->get(infile.val);
		// @ATTR humanoid|bool|This creature gives human traits when transformed into, such as the ability to talk with NPCs.
		else if (infile.key == "humanoid") humanoid = toBool(infile.val);

		// @ATTR level|int|Level
		else if (infile.key == "level") level = num;

		// enemy death rewards and events
		// @ATTR xp|int|XP awarded upon death.
		else if (infile.key == "xp") xp = num;
		else if (infile.key == "loot") {
			// @ATTR loot|repeatable(loot)|Possible loot that can be dropped on death.

			// loot entries format:
			// loot=[id],[percent_chance]
			// optionally allow range:
			// loot=[id],[percent_chance],[count_min],[count_max]

			if (clear_loot) {
				loot_table.clear();
				clear_loot = false;
			}

			loot_table.push_back(Event_Component());
			loot->parseLoot(infile.val, &loot_table.back(), &loot_table);
		}
		else if (infile.key == "loot_count") {
			// @ATTR loot_count|int, int : Min, Max|Sets the minimum (and optionally, the maximum) amount of loot this creature can drop. Overrides the global drop_max setting.
			loot_count.x = popFirstInt(infile.val);
			loot_count.y = popFirstInt(infile.val);
			if (loot_count.x != 0 || loot_count.y != 0) {
				clampFloor(loot_count.x, 1);
				clampFloor(loot_count.y, loot_count.x);
			}
		}
		// @ATTR defeat_status|string|Campaign status to set upon death.
		else if (infile.key == "defeat_status") defeat_status = infile.val;
		// @ATTR convert_status|string|Campaign status to set upon being converted to a player ally.
		else if (infile.key == "convert_status") convert_status = infile.val;
		// @ATTR first_defeat_loot|item_id|Drops this item upon first death.
		else if (infile.key == "first_defeat_loot") first_defeat_loot = num;
		// @ATTR quest_loot|string, string, item_id : Required status, Required not status, Item|Drops this item when campaign status is met.
		else if (infile.key == "quest_loot") {
			quest_loot_requires_status = popFirstString(infile.val);
			quest_loot_requires_not_status = popFirstString(infile.val);
			quest_loot_id = popFirstInt(infile.val);
		}

		// behavior stats
		// @ATTR flying|bool|Creature can move over gaps/water.
		else if (infile.key == "flying") flying = toBool(infile.val);
		// @ATTR intangible|bool|Creature can move through walls.
		else if (infile.key == "intangible") intangible = toBool(infile.val);
		// @ATTR facing|bool|Creature can turn to face their target.
		else if (infile.key == "facing") facing = toBool(infile.val);

		// @ATTR waypoint_pause|duration|Duration to wait at each waypoint in 'ms' or 's'.
		else if (infile.key == "waypoint_pause") waypoint_pause = parse_duration(infile.val);

		// @ATTR turn_delay|duration|Duration it takes for this creature to turn and face their target in 'ms' or 's'.
		else if (infile.key == "turn_delay") turn_delay = parse_duration(infile.val);
		// @ATTR chance_pursue|int|Percentage change that the creature will chase their target.
		else if (infile.key == "chance_pursue") chance_pursue = num;
		// @ATTR chance_flee|int|Percentage chance that the creature will run away from their target.
		else if (infile.key == "chance_flee") chance_flee = num;

		else if (infile.key == "power") {
			// @ATTR power|["melee", "ranged", "beacon", "on_hit", "on_death", "on_half_dead", "on_join_combat", "on_debuff"], power_id, int : State, Power, Chance|A power that has a chance of being triggered in a certain state.
			AIPower ai_power;

			std::string ai_type = popFirstString(infile.val);

			ai_power.id = powers->verifyID(popFirstInt(infile.val), &infile, false);
			if (ai_power.id == 0)
				continue; // verifyID() will print our error message

			ai_power.chance = popFirstInt(infile.val);

			if (ai_type == "melee") ai_power.type = AI_POWER_MELEE;
			else if (ai_type == "ranged") ai_power.type = AI_POWER_RANGED;
			else if (ai_type == "beacon") ai_power.type = AI_POWER_BEACON;
			else if (ai_type == "on_hit") ai_power.type = AI_POWER_HIT;
			else if (ai_type == "on_death") ai_power.type = AI_POWER_DEATH;
			else if (ai_type == "on_half_dead") ai_power.type = AI_POWER_HALF_DEAD;
			else if (ai_type == "on_join_combat") ai_power.type = AI_POWER_JOIN_COMBAT;
			else if (ai_type == "on_debuff") ai_power.type = AI_POWER_DEBUFF;
			else {
				infile.error("StatBlock: '%s' is not a valid enemy power type.", ai_type.c_str());
				continue;
			}

			if (ai_power.type == AI_POWER_HALF_DEAD)
				half_dead_power = true;

			powers_ai.push_back(ai_power);
		}

		else if (infile.key == "passive_powers") {
			// @ATTR passive_powers|list(power_id)|A list of passive powers this creature has.
			powers_passive.clear();
			std::string p = popFirstString(infile.val);
			while (p != "") {
				powers_passive.push_back(toInt(p));
				p = popFirstString(infile.val);
			}
		}

		// @ATTR melee_range|float|Minimum distance from target required to use melee powers.
		else if (infile.key == "melee_range") melee_range = fnum;
		// @ATTR threat_range|float, float: Engage distance, Stop distance|The first value is the radius of the area this creature will be able to start chasing the hero. The second, optional, value is the radius at which this creature will stop pursuing their target and defaults to double the first value.
		else if (infile.key == "threat_range") {
			threat_range = toFloat(popFirstString(infile.val));

			std::string tr_far = popFirstString(infile.val);
			if (!tr_far.empty())
				threat_range_far = toFloat(tr_far);
			else
				threat_range_far = threat_range * 2;
		}
		// @ATTR flee_range|float|The radius at which this creature will start moving to a safe distance. Defaults to half of the threat_range.
		else if (infile.key == "flee_range") {
			flee_range = fnum;
			flee_range_defined = true;
		}
		// @ATTR combat_style|["default", "aggressive", "passive"]|How the creature will enter combat. Default is within range of the hero; Aggressive is always in combat; Passive must be attacked to enter combat.
		else if (infile.key == "combat_style") {
			if (infile.val == "default") combat_style = COMBAT_DEFAULT;
			else if (infile.val == "aggressive") combat_style = COMBAT_AGGRESSIVE;
			else if (infile.val == "passive") combat_style = COMBAT_PASSIVE;
			else infile.error("StatBlock: Unknown combat style '%s'", infile.val.c_str());
		}

		// @ATTR animations|filename|Filename of an animation definition.
		else if (infile.key == "animations") animations = infile.val;

		// @ATTR suppress_hp|bool|Hides the enemy HP bar for this creature.
		else if (infile.key == "suppress_hp") suppress_hp = toBool(infile.val);

		else if (infile.key == "categories") {
			// @ATTR categories|list(string)|Categories that this enemy belongs to.
			categories.clear();
			std::string cat;
			while ((cat = popFirstString(infile.val)) != "") {
				categories.push_back(cat);
			}
		}

		// @ATTR flee_duration|duration|The minimum amount of time that this creature will flee. They may flee longer than the specified time.
		else if (infile.key == "flee_duration") flee_duration = parse_duration(infile.val);
		// @ATTR flee_cooldown|duration|The amount of time this creature must wait before they can start fleeing again.
		else if (infile.key == "flee_cooldown") flee_cooldown = parse_duration(infile.val);

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

	if (!flee_range_defined)
		flee_range = threat_range / 2;

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

	if (xp >= xp_table.back())
		xp = xp_table.back();

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

	// handle party buffs
	if (enemies && powers) {
		while (!party_buffs.empty()) {
			int power_index = party_buffs.front();
			party_buffs.pop();
			Power *buff_power = &powers->powers[power_index];

			for (size_t i=0; i < enemies->enemies.size(); ++i) {
				if(enemies->enemies[i]->stats.hp > 0 &&
				   ((enemies->enemies[i]->stats.hero_ally && hero) || (enemies->enemies[i]->stats.enemy_ally && enemies->enemies[i]->stats.summoner == this)) &&
				   (buff_power->buff_party_power_id == 0 || buff_power->buff_party_power_id == enemies->enemies[i]->stats.summoned_power_index)
				) {
					powers->effect(&enemies->enemies[i]->stats, this, power_index, (hero ? SOURCE_TYPE_HERO : SOURCE_TYPE_ENEMY));
				}
			}
		}
	}

	// handle effect timers
	effects.logic();

	// apply bonuses from items/effects to base stats
	applyEffects();

	// preserve ratio on maxmp and maxhp changes
	float ratio;
	if (prev_maxhp != get(STAT_HP_MAX)) {
		ratio = static_cast<float>(pres_hp) / static_cast<float>(prev_maxhp);
		hp = static_cast<int>(ratio * static_cast<float>(get(STAT_HP_MAX)));
	}
	if (prev_maxmp != get(STAT_MP_MAX)) {
		ratio = static_cast<float>(pres_mp) / static_cast<float>(prev_maxmp);
		mp = static_cast<int>(ratio * static_cast<float>(get(STAT_MP_MAX)));
	}

	// handle cooldowns
	if (cooldown_ticks > 0) cooldown_ticks--; // global cooldown

	for (size_t i=0; i<powers_ai.size(); ++i) { // NPC/enemy powerslot cooldown
		if (powers_ai[i].ticks > 0) powers_ai[i].ticks--;
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
	if (effects.damage > 0 && hp > 0) {
		takeDamage(effects.damage);
		comb->addInt(effects.damage, pos, COMBAT_MESSAGE_TAKEDMG);
	}
	if (effects.damage_percent > 0 && hp > 0) {
		int damage = (get(STAT_HP_MAX)*effects.damage_percent)/100;
		takeDamage(damage);
		comb->addInt(damage, pos, COMBAT_MESSAGE_TAKEDMG);
	}

	if(effects.death_sentence)
		hp = 0;

	if(cooldown_hit_ticks > 0)
		cooldown_hit_ticks--;

	if (effects.stun) {
		// stun stops charge attacks
		state_ticks = 0;
		charge_speed = 0;
	}
	else if (state_ticks > 0) {
		state_ticks--;
	}

	// apply healing over time
	if (effects.hpot > 0) {
		comb->addString(msg->get("+%d HP",effects.hpot), pos, COMBAT_MESSAGE_BUFF);
		hp += effects.hpot;
		if (hp > get(STAT_HP_MAX)) hp = get(STAT_HP_MAX);
	}
	if (effects.hpot_percent > 0) {
		int hpot = (get(STAT_HP_MAX)*effects.hpot_percent)/100;
		comb->addString(msg->get("+%d HP",hpot), pos, COMBAT_MESSAGE_BUFF);
		hp += hpot;
		if (hp > get(STAT_HP_MAX)) hp = get(STAT_HP_MAX);
	}
	if (effects.mpot > 0) {
		comb->addString(msg->get("+%d MP",effects.mpot), pos, COMBAT_MESSAGE_BUFF);
		mp += effects.mpot;
		if (mp > get(STAT_MP_MAX)) mp = get(STAT_MP_MAX);
	}
	if (effects.mpot_percent > 0) {
		int mpot = (get(STAT_MP_MAX)*effects.mpot_percent)/100;
		comb->addString(msg->get("+%d MP",mpot), pos, COMBAT_MESSAGE_BUFF);
		mp += mpot;
		if (mp > get(STAT_MP_MAX)) mp = get(STAT_MP_MAX);
	}

	// set movement type
	// some creatures may shift between movement types
	if (intangible) movement_type = MOVEMENT_INTANGIBLE;
	else if (flying) movement_type = MOVEMENT_FLYING;
	else movement_type = MOVEMENT_NORMAL;

	if (hp == 0)
		removeSummons();

	if (effects.knockback_speed != 0) {
		float theta = calcTheta(knockback_srcpos.x, knockback_srcpos.y, knockback_destpos.x, knockback_destpos.y);
		knockback_speed.x = effects.knockback_speed * cosf(theta);
		knockback_speed.y = effects.knockback_speed * sinf(theta);

		mapr->collider.unblock(pos.x, pos.y);
		mapr->collider.move(pos.x, pos.y, knockback_speed.x, knockback_speed.y, movement_type, hero);
		mapr->collider.block(pos.x, pos.y, hero_ally);
	}
	else if (charge_speed != 0.0f) {
		float tmp_speed = charge_speed * speedMultiplyer[direction];
		float dx = tmp_speed * static_cast<float>(directionDeltaX[direction]);
		float dy = tmp_speed * static_cast<float>(directionDeltaY[direction]);

		mapr->collider.unblock(pos.x, pos.y);
		mapr->collider.move(pos.x, pos.y, dx, dy, movement_type, hero);
		mapr->collider.block(pos.x, pos.y, hero_ally);
	}
}

StatBlock::~StatBlock() {
	removeFromSummons();
}

bool StatBlock::canUsePower(const Power &power, int powerid) const {
	if (!alive) {
		// can't use powers when dead
		return false;
	}
	else if (transformed) {
		// needed to unlock shapeshifter powers
		return mp >= power.requires_mp;
	}
	else {
		return (
			mp >= power.requires_mp
			&& !power.passive
			&& !power.meta_power
			&& !effects.stun
			&& (power.sacrifice || hp > power.requires_hp)
			&& (menu_powers && menu_powers->meetsUsageStats(powerid))
			&& (power.type == POWTYPE_SPAWN ? !summonLimitReached(powerid) : true)
			&& !(power.spawn_type == "untransform" && !transformed)
			&& std::includes(equip_flags.begin(), equip_flags.end(), power.requires_flags.begin(), power.requires_flags.end())
			&& (!power.buff_party || (power.buff_party && enemies && enemies->checkPartyMembers()))
			&& (power.requires_item == -1 || (power.requires_item > 0 && items->requirementsMet(this, power.requires_item)))
		);
	}

}

void StatBlock::loadHeroStats() {
	// set the default global cooldown
	cooldown = parse_duration("66ms");

	// Redefine numbers from config file if present
	FileParser infile;
	// @CLASS StatBlock: Hero stats|Description of engine/stats.txt
	if (infile.open("engine/stats.txt")) {
		while (infile.next()) {
			int value = toInt(infile.val);

			bool valid = loadCoreStat(&infile);

			if (infile.key == "max_points_per_stat") {
				// @ATTR max_points_per_stat|int|Maximum points for each primary stat.
				max_points_per_stat = value;
			}
			else if (infile.key == "sfx_step") {
				// @ATTR sfx_step|string|An id for a set of step sound effects. See items/step_sounds.txt.
				sfx_step = infile.val;
			}
			else if (infile.key == "stat_points_per_level") {
				// @ATTR stat_points_per_level|int|The amount of stat points awarded each level.
				stat_points_per_level = value;
			}
			else if (infile.key == "power_points_per_level") {
				// @ATTR power_points_per_level|int|The amount of power points awarded each level.
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
			if (infile.key == "level") {
				// @ATTR level|int, int : Level, XP|The amount of XP required for this level.
				unsigned lvl_id = popFirstInt(infile.val);
				unsigned long lvl_xp = toUnsignedLong(popFirstString(infile.val));

				if (lvl_id > xp_table.size())
					xp_table.resize(lvl_id);

				xp_table[lvl_id - 1] = lvl_xp;
			}
		}
		infile.close();
	}

	if (xp_table.empty()) {
		logError("StatBlock: No XP table defined.");
		xp_table.push_back(0);
	}

	max_spendable_stat_points = static_cast<int>(xp_table.size()) * stat_points_per_level;
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

/**
 * Recursivly kill all summoned creatures
 */
void StatBlock::removeSummons() {
	for (std::vector<StatBlock*>::iterator it = summons.begin(); it != summons.end(); ++it) {
		(*it)->hp = 0;
		(*it)->effects.triggered_death = true;
		(*it)->effects.clearEffects();
		if (!(*it)->hero && !(*it)->corpse) {
			(*it)->cur_state = ENEMY_DEAD;
			(*it)->corpse_ticks = CORPSE_TIMEOUT;
		}
		(*it)->removeSummons();
		(*it)->summoner = NULL;
	}

	summons.clear();
}

void StatBlock::removeFromSummons() {

	if(summoner != NULL && !summoner->summons.empty()) {
		std::vector<StatBlock*>::iterator parent_ref = std::find(summoner->summons.begin(), summoner->summons.end(), this);

		if(parent_ref != summoner->summons.end())
			summoner->summons.erase(parent_ref);

		summoner = NULL;
	}

	removeSummons();
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
std::string StatBlock::getShortClass() {
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

void StatBlock::addXP(int amount) {
	xp += amount;

	if (xp >= xp_table.back())
		xp = xp_table.back();
}

AIPower* StatBlock::getAIPower(AI_POWER ai_type) {
	std::vector<size_t> possible_ids;
	int chance = rand() % 100;

	for (size_t i=0; i<powers_ai.size(); ++i) {
		if (ai_type != powers_ai[i].type)
			continue;

		if (chance > powers_ai[i].chance)
			continue;

		if (powers_ai[i].ticks > 0)
			continue;

		if (powers->powers[powers_ai[i].id].type == POWTYPE_SPAWN) {
			if (summonLimitReached(powers_ai[i].id))
				continue;
		}

		int live_summon_count = 0;
		for (size_t j=0; j<summons.size(); ++j) {
			if (summons[j]->hp > 0) {
				++live_summon_count;
			}
		}
		if (powers->powers[powers_ai[i].id].requires_spawns > 0) {
			if (live_summon_count < powers->powers[powers_ai[i].id].requires_spawns)
				continue;
		}

		possible_ids.push_back(i);
	}

	if (!possible_ids.empty()) {
		size_t index = static_cast<size_t>(rand()) % possible_ids.size();
		return &powers_ai[possible_ids[index]];
	}

	return NULL;
}
