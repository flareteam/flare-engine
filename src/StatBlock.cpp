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

#include "Avatar.h"
#include "CampaignManager.h"
#include "CombatText.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "Hazard.h"
#include "LootManager.h"
#include "MapCollision.h"
#include "MapRenderer.h"
#include "MenuPowers.h"
#include "MessageEngine.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <limits>

StatBlock::StatBlock()
	: statsLoaded(false)
	, alive(true)
	, corpse(false)
	, corpse_timer()
	, hero(false)
	, hero_ally(false)
	, enemy_ally(false)
	, humanoid(false)
	, lifeform(true)
	, permadeath(false)
	, transformed(false)
	, refresh_stats(false)
	, converted(false)
	, summoned(false)
	, summoned_power_index(0)
	, encountered(false)
	, target_corpse(NULL) // hero only
	, target_nearest(NULL) // hero only
	, target_nearest_corpse(NULL) // hero only
	, target_nearest_dist(0) // hero only
	, target_nearest_corpse_dist(0) // hero only
	, block_power(0)
	, movement_type(MapCollision::MOVE_NORMAL)
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
	, starting(Stats::COUNT + eset->damage_types.count, 0)
	, base(Stats::COUNT + eset->damage_types.count, 0)
	, current(Stats::COUNT + eset->damage_types.count, 0)
	, per_level(Stats::COUNT + eset->damage_types.count, 0)
	, character_class("")
	, character_subclass("")
	, hp(0)
	, hp_ticker(0)
	, mp(0)
	, mp_ticker(0)
	, speed_default(0.1f)
	, dmg_min_add(eset->damage_types.list.size(), 0)
	, dmg_max_add(eset->damage_types.list.size(), 0)
	, absorb_min_add(0)
	, absorb_max_add(0)
	, speed(0.1f)
	, charge_speed(0.0f)
	, vulnerable(eset->elements.list.size(), 100)
	, vulnerable_base(eset->elements.list.size(), 100)
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
	, cooldown_hit()
	, cooldown_hit_enabled(false)
	, cur_state(0)
	, state_timer()
	, hold_state(false)
	, prevent_interrupt(false)
	, waypoints()		// enemy only
	, waypoint_timer(settings->max_frames_per_sec)	// enemy only
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
	, in_combat(false)  //enemy only
	, join_combat(false)
	, cooldown()
	, activated_power(NULL) // enemy only
	, half_dead_power(false) // enemy only
	, suppress_hp(false)
	, flee_timer(settings->max_frames_per_sec) // enemy only
	, flee_cooldown_timer(settings->max_frames_per_sec) // enemy only
	, perfect_accuracy(false)
	, teleportation(false)
	, teleport_destination()
	, currency(0)
	, death_penalty(false)
	, defeat_status(0)			// enemy only
	, convert_status(0)		// enemy only
	, quest_loot_requires_status(0)	// enemy only
	, quest_loot_requires_not_status(0)		// enemy only
	, quest_loot_id(0)			// enemy only
	, first_defeat_loot(0)		// enemy only
	, gfx_base("male")
	, gfx_head("head_short")
	, gfx_portrait("")
	, transform_type("")
	, animations("")
	, sfx_attack()
	, sfx_step("")
	, sfx_hit()
	, sfx_die()
	, sfx_critdie()
	, sfx_block()
	, sfx_levelup("")
	, sfx_lowhp("")
	, sfx_lowhp_loop(false)
	, max_spendable_stat_points(0)
	, max_points_per_stat(0)
	, prev_maxhp(0)
	, prev_maxmp(0)
	, prev_hp(0)
	, prev_mp(0)
	, summons()
	, summoner(NULL)
	, bleed_source_type(-1)
{
	primary.resize(eset->primary_stats.list.size(), 0);
	primary_starting.resize(eset->primary_stats.list.size(), 0);
	primary_additional.resize(eset->primary_stats.list.size(), 0);
	per_primary.resize(eset->primary_stats.list.size());

	for (size_t i = 0; i < per_primary.size(); ++i) {
		per_primary[i].resize(Stats::COUNT + eset->damage_types.count, 0);
	}
}

bool StatBlock::loadCoreStat(FileParser *infile) {
	// @CLASS StatBlock: Core stats|Description of engine/stats.txt and enemies in enemies/

	if (infile->key == "speed") {
		// @ATTR speed|float|Movement speed
		float fvalue = Parse::toFloat(infile->val, 0);
		speed = speed_default = fvalue / settings->max_frames_per_sec;
		return true;
	}
	else if (infile->key == "cooldown") {
		// @ATTR cooldown|int|Cooldown between attacks in 'ms' or 's'.
		cooldown.setDuration(Parse::toDuration(infile->val));
		return true;
	}
	else if (infile->key == "cooldown_hit") {
		// @ATTR cooldown_hit|duration|Duration of cooldown after being hit in 'ms' or 's'.
		cooldown_hit.setDuration(Parse::toDuration(infile->val));
		cooldown_hit_enabled = true;
		return true;
	}
	else if (infile->key == "stat") {
		// @ATTR stat|string, int : Stat name, Value|The starting value for this stat.
		std::string stat = Parse::popFirstString(infile->val);
		int value = Parse::popFirstInt(infile->val);

		for (int i=0; i<Stats::COUNT; ++i) {
			if (Stats::KEY[i] == stat) {
				starting[i] = value;
				return true;
			}
		}

		for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
			if (eset->damage_types.list[i].min == stat) {
				starting[Stats::COUNT + (i*2)] = value;
				return true;
			}
			else if (eset->damage_types.list[i].max == stat) {
				starting[Stats::COUNT + (i*2) + 1] = value;
				return true;
			}
		}
	}
	else if (infile->key == "stat_per_level") {
		// @ATTR stat_per_level|predefined_string, int : Stat name, Value|The value for this stat added per level.
		std::string stat = Parse::popFirstString(infile->val);
		int value = Parse::popFirstInt(infile->val);

		for (int i=0; i<Stats::COUNT; i++) {
			if (Stats::KEY[i] == stat) {
				per_level[i] = value;
				return true;
			}
		}

		for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
			if (eset->damage_types.list[i].min == stat) {
				per_level[Stats::COUNT + (i*2)] = value;
				return true;
			}
			else if (eset->damage_types.list[i].max == stat) {
				per_level[Stats::COUNT + (i*2) + 1] = value;
				return true;
			}
		}
	}
	else if (infile->key == "stat_per_primary") {
		// @ATTR stat_per_primary|predefined_string, predefined_string, int : Primary Stat, Stat name, Value|The value for this stat added for every point allocated to this primary stat.
		std::string prim_stat = Parse::popFirstString(infile->val);
		size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);
		if (prim_stat_index == eset->primary_stats.list.size()) {
			infile->error("StatBlock: '%s' is not a valid primary stat.", prim_stat.c_str());
			return true;
		}

		std::string stat = Parse::popFirstString(infile->val);
		int value = Parse::popFirstInt(infile->val);

		for (int i=0; i<Stats::COUNT; i++) {
			if (Stats::KEY[i] == stat) {
				per_primary[prim_stat_index][i] = value;
				return true;
			}
		}

		for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
			if (eset->damage_types.list[i].min == stat) {
				per_primary[prim_stat_index][Stats::COUNT + (i*2)] = value;
				return true;
			}
			else if (eset->damage_types.list[i].max == stat) {
				per_primary[prim_stat_index][Stats::COUNT + (i*2) + 1] = value;
				return true;
			}
		}
	}
	else if (infile->key == "vulnerable") {
		// @ATTR vulnerable|predefined_string, int : Element, Value|Percentage weakness to this element.
		std::string element = Parse::popFirstString(infile->val);
		int value = Parse::popFirstInt(infile->val);

		for (unsigned int i=0; i<eset->elements.list.size(); i++) {
			if (element == eset->elements.list[i].id) {
				vulnerable[i] = vulnerable_base[i] = value;
				return true;
			}
		}
	}
	else if (infile->key == "power_filter") {
		// @ATTR power_filter|list(power_id)|Only these powers are allowed to hit this entity.
		std::string power_id = Parse::popFirstString(infile->val);
		while (!power_id.empty()) {
			power_filter.push_back(Parse::toInt(power_id));
			power_id = Parse::popFirstString(infile->val);
		}
		return true;
	}

	return false;
}

/**
 * Set paths for sound effects
 */
bool StatBlock::loadSfxStat(FileParser *infile) {
	// @CLASS StatBlock: Sound effects|Description of heroes in engine/avatar/ and enemies in enemies/

	if (infile->new_section) {
		sfx_attack.clear();
		sfx_hit.clear();
		sfx_die.clear();
		sfx_critdie.clear();
		sfx_block.clear();
	}

	if (infile->key == "sfx_attack") {
		// @ATTR sfx_attack|repeatable(predefined_string, filename) : Animation name, Sound file|Filename of sound effect for the specified attack animation.
		std::string anim_name = Parse::popFirstString(infile->val);
		std::string filename = Parse::popFirstString(infile->val);

		size_t found_index = sfx_attack.size();
		for (size_t i = 0; i < sfx_attack.size(); ++i) {
			if (anim_name == sfx_attack[i].first) {
				found_index = i;
				break;
			}
		}

		if (found_index == sfx_attack.size()) {
			sfx_attack.push_back(std::pair<std::string, std::vector<std::string> >());
			sfx_attack.back().first = anim_name;
			sfx_attack.back().second.push_back(filename);
		}
		else {
			if (std::find(sfx_attack[found_index].second.begin(), sfx_attack[found_index].second.end(), filename) == sfx_attack[found_index].second.end()) {
				sfx_attack[found_index].second.push_back(filename);
			}
		}
	}
	else if (infile->key == "sfx_hit") {
		// @ATTR sfx_hit|repeatable(filename)|Filename of sound effect for being hit.
		if (std::find(sfx_hit.begin(), sfx_hit.end(), infile->val) == sfx_hit.end()) {
			sfx_hit.push_back(infile->val);
		}
	}
	else if (infile->key == "sfx_die") {
		// @ATTR sfx_die|repeatable(filename)|Filename of sound effect for dying.
		if (std::find(sfx_die.begin(), sfx_die.end(), infile->val) == sfx_die.end()) {
			sfx_die.push_back(infile->val);
		}
	}
	else if (infile->key == "sfx_critdie") {
		// @ATTR sfx_critdie|repeatable(filename)|Filename of sound effect for dying to a critical hit.
		if (std::find(sfx_critdie.begin(), sfx_critdie.end(), infile->val) == sfx_critdie.end()) {
			sfx_critdie.push_back(infile->val);
		}
	}
	else if (infile->key == "sfx_block") {
		// @ATTR sfx_block|repeatable(filename)|Filename of sound effect for blocking an incoming hit.
		if (std::find(sfx_block.begin(), sfx_block.end(), infile->val) == sfx_block.end()) {
			sfx_block.push_back(infile->val);
		}
	}
	else if (infile->key == "sfx_levelup") {
		// @ATTR sfx_levelup|filename|Filename of sound effect for leveling up.
		sfx_levelup = infile->val;
	}
	else if (infile->key == "sfx_lowhp") {
		// @ATTR sfx_lowhp|filename, bool: Sound file, loop|Filename of sound effect for low health warning. Optionally, it can be looped.
		sfx_lowhp = Parse::popFirstString(infile->val);
		if (infile->val != "") sfx_lowhp_loop = Parse::toBool(infile->val);
	}
	else {
		return false;
	}

	return true;
}

/**
 * load a statblock, typically for an enemy definition
 */
void StatBlock::load(const std::string& filename) {
	// @CLASS StatBlock: Enemies|Description of enemies in enemies/
	FileParser infile;
	if (!infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return;

	bool clear_loot = true;
	bool flee_range_defined = false;

	while (infile.next()) {
		if (infile.new_section) {
			// APPENDed file
			clear_loot = true;
		}

		int num = Parse::toInt(infile.val);
		float fnum = Parse::toFloat(infile.val);
		bool valid = loadCoreStat(&infile) || loadSfxStat(&infile);

		// @ATTR name|string|Name
		if (infile.key == "name") name = msg->get(infile.val);
		// @ATTR humanoid|bool|This creature gives human traits when transformed into, such as the ability to talk with NPCs.
		else if (infile.key == "humanoid") humanoid = Parse::toBool(infile.val);
		// @ATTR lifeform|bool|Determines whether or not this entity is referred to as a living thing, such as displaying "Dead" vs "Destroyed" when their HP is 0.
		else if (infile.key == "lifeform") lifeform = Parse::toBool(infile.val);

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

			loot_table.push_back(EventComponent());
			loot->parseLoot(infile.val, &loot_table.back(), &loot_table);
		}
		else if (infile.key == "loot_count") {
			// @ATTR loot_count|int, int : Min, Max|Sets the minimum (and optionally, the maximum) amount of loot this creature can drop. Overrides the global drop_max setting.
			loot_count.x = Parse::popFirstInt(infile.val);
			loot_count.y = Parse::popFirstInt(infile.val);
			if (loot_count.x != 0 || loot_count.y != 0) {
				loot_count.x = std::max(loot_count.x, 1);
				loot_count.y = std::max(loot_count.y, loot_count.x);
			}
		}
		// @ATTR defeat_status|string|Campaign status to set upon death.
		else if (infile.key == "defeat_status") defeat_status = camp->registerStatus(infile.val);
		// @ATTR convert_status|string|Campaign status to set upon being converted to a player ally.
		else if (infile.key == "convert_status") convert_status = camp->registerStatus(infile.val);
		// @ATTR first_defeat_loot|item_id|Drops this item upon first death.
		else if (infile.key == "first_defeat_loot") first_defeat_loot = num;
		// @ATTR quest_loot|string, string, item_id : Required status, Required not status, Item|Drops this item when campaign status is met.
		else if (infile.key == "quest_loot") {
			quest_loot_requires_status = camp->registerStatus(Parse::popFirstString(infile.val));
			quest_loot_requires_not_status = camp->registerStatus(Parse::popFirstString(infile.val));
			quest_loot_id = Parse::popFirstInt(infile.val);
		}

		// behavior stats
		// @ATTR flying|bool|Creature can move over gaps/water.
		else if (infile.key == "flying") flying = Parse::toBool(infile.val);
		// @ATTR intangible|bool|Creature can move through walls.
		else if (infile.key == "intangible") intangible = Parse::toBool(infile.val);
		// @ATTR facing|bool|Creature can turn to face their target.
		else if (infile.key == "facing") facing = Parse::toBool(infile.val);

		// @ATTR waypoint_pause|duration|Duration to wait at each waypoint in 'ms' or 's'.
		else if (infile.key == "waypoint_pause") waypoint_timer.setDuration(Parse::toDuration(infile.val));

		// @ATTR turn_delay|duration|Duration it takes for this creature to turn and face their target in 'ms' or 's'.
		else if (infile.key == "turn_delay") turn_delay = Parse::toDuration(infile.val);
		// @ATTR chance_pursue|int|Percentage change that the creature will chase their target.
		else if (infile.key == "chance_pursue") chance_pursue = num;
		// @ATTR chance_flee|int|Percentage chance that the creature will run away from their target.
		else if (infile.key == "chance_flee") chance_flee = num;

		else if (infile.key == "power") {
			// @ATTR power|["melee", "ranged", "beacon", "on_hit", "on_death", "on_half_dead", "on_join_combat", "on_debuff"], power_id, int : State, Power, Chance|A power that has a chance of being triggered in a certain state.
			AIPower ai_power;

			std::string ai_type = Parse::popFirstString(infile.val);

			ai_power.id = powers->verifyID(Parse::popFirstInt(infile.val), &infile, !PowerManager::ALLOW_ZERO_ID);
			if (ai_power.id == 0)
				continue; // verifyID() will print our error message

			ai_power.chance = Parse::popFirstInt(infile.val);

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
			std::string p = Parse::popFirstString(infile.val);
			while (p != "") {
				powers_passive.push_back(Parse::toInt(p));
				p = Parse::popFirstString(infile.val);

				// if a passive power has a post power, add it to the AI power list so we can track its cooldown
				int post_power = powers->powers[powers_passive.back()].post_power;
				if (post_power > 0) {
					AIPower passive_post_power;
					passive_post_power.type = AI_POWER_PASSIVE_POST;
					passive_post_power.id = post_power;
					passive_post_power.chance = 0; // post_power chance is used instead
					powers_ai.push_back(passive_post_power);
				}
			}
		}

		// @ATTR melee_range|float|Minimum distance from target required to use melee powers.
		else if (infile.key == "melee_range") melee_range = fnum;
		// @ATTR threat_range|float, float: Engage distance, Stop distance|The first value is the radius of the area this creature will be able to start chasing the hero. The second, optional, value is the radius at which this creature will stop pursuing their target and defaults to double the first value.
		else if (infile.key == "threat_range") {
			threat_range = Parse::toFloat(Parse::popFirstString(infile.val));

			std::string tr_far = Parse::popFirstString(infile.val);
			if (!tr_far.empty())
				threat_range_far = Parse::toFloat(tr_far);
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
		else if (infile.key == "suppress_hp") suppress_hp = Parse::toBool(infile.val);

		else if (infile.key == "categories") {
			// @ATTR categories|list(string)|Categories that this enemy belongs to.
			categories.clear();
			std::string cat;
			while ((cat = Parse::popFirstString(infile.val)) != "") {
				categories.push_back(cat);
			}
		}

		// @ATTR flee_duration|duration|The minimum amount of time that this creature will flee. They may flee longer than the specified time.
		else if (infile.key == "flee_duration") flee_timer.setDuration(Parse::toDuration(infile.val));
		// @ATTR flee_cooldown|duration|The amount of time this creature must wait before they can start fleeing again.
		else if (infile.key == "flee_cooldown") flee_cooldown_timer.setDuration(Parse::toDuration(infile.val));

		// this is only used for EnemyGroupManager
		// we check for them here so that we don't get an error saying they are invalid
		else if (infile.key == "rarity") ; // but do nothing

		else if (!valid) {
			infile.error("StatBlock: '%s' is not a valid key.", infile.key.c_str());
		}
	}
	infile.close();

	hp = starting[Stats::HP_MAX];
	mp = starting[Stats::MP_MAX];

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

	if (hero) {
		if (!statsLoaded) loadHeroStats();

		refresh_stats = true;

		unsigned long xp_max = eset->xp.getLevelXP(eset->xp.getMaxLevel());
		xp = std::min(xp, xp_max);

		level = eset->xp.getLevelFromXP(xp);
		if (level != 0)
			check_title = true;
	}

	if (level < 1)
		level = 1;

	applyEffects();

	hp = get(Stats::HP_MAX);
	mp = get(Stats::MP_MAX);
}

/**
 * Base damage and absorb is 0
 * Plus an optional bonus_per_[base stat]
 */
void StatBlock::calcBase() {
	// bonuses are skipped for the default level 1 of a stat
	int lev0 = std::max(level - 1, 0);

	for (size_t i = 0; i < Stats::COUNT + eset->damage_types.count; ++i) {
		base[i] = starting[i];
		base[i] += lev0 * per_level[i];
		for (size_t j = 0; j < per_primary.size(); ++j) {
			base[i] += std::max(get_primary(j) - 1, 0) * per_primary[j][i];
		}
	}

	// add damage from equipment and increase to minimum amounts
	for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
		base[Stats::COUNT + (i*2)] += dmg_min_add[i];
		base[Stats::COUNT + (i*2) + 1] += dmg_max_add[i];
		base[Stats::COUNT + (i*2)] = std::max(base[Stats::COUNT + (i*2)], 0);
		base[Stats::COUNT + (i*2) + 1] = std::max(base[Stats::COUNT + (i*2) + 1], base[Stats::COUNT + (i*2)]);
	}

	// add absorb from equipment and increase to minimum amounts
	base[Stats::ABS_MIN] += absorb_min_add;
	base[Stats::ABS_MAX] += absorb_max_add;
	base[Stats::ABS_MIN] = std::max(base[Stats::ABS_MIN], 0);
	base[Stats::ABS_MAX] = std::max(base[Stats::ABS_MAX], base[Stats::ABS_MIN]);
}

/**
 * Recalc derived stats from base stats + effect bonuses
 */
void StatBlock::applyEffects() {

	// preserve hp/mp states
	// max HP and MP can't drop below 1
	prev_maxhp = std::max(get(Stats::HP_MAX), 1);
	prev_maxmp = std::max(get(Stats::MP_MAX), 1);
	prev_hp = hp;
	prev_mp = mp;

	// calculate primary stats
	// refresh the character menu if there has been a change
	for (size_t i = 0; i < primary.size(); ++i) {
		if (get_primary(i) != primary[i] + effects.bonus_primary[i])
			refresh_stats = true;

		primary_additional[i] = effects.bonus_primary[i];
	}

	calcBase();

	for (size_t i=0; i<Stats::COUNT + eset->damage_types.count; i++) {
		current[i] = base[i] + effects.bonus[i];
	}

	for (unsigned i=0; i<effects.bonus_resist.size(); i++) {
		vulnerable[i] = vulnerable_base[i] - effects.bonus_resist[i];
	}

	current[Stats::HP_MAX] += (current[Stats::HP_MAX] * current[Stats::HP_PERCENT]) / 100;
	current[Stats::MP_MAX] += (current[Stats::MP_MAX] * current[Stats::MP_PERCENT]) / 100;

	// max HP and MP can't drop below 1
	current[Stats::HP_MAX] = std::max(get(Stats::HP_MAX), 1);
	current[Stats::MP_MAX] = std::max(get(Stats::MP_MAX), 1);

	if (hp > get(Stats::HP_MAX)) hp = get(Stats::HP_MAX);
	if (mp > get(Stats::MP_MAX)) mp = get(Stats::MP_MAX);

	speed = speed_default;
}

/**
 * Process per-frame actions
 */
void StatBlock::logic() {
	alive = !(hp <= 0 && !effects.triggered_death && !effects.revive);

	// handle party buffs
	if (enemym && powers) {
		while (!party_buffs.empty()) {
			int power_index = party_buffs.front();
			party_buffs.pop();
			Power *buff_power = &powers->powers[power_index];

			for (size_t i=0; i < enemym->enemies.size(); ++i) {
				if(enemym->enemies[i]->stats.hp > 0 &&
				   ((enemym->enemies[i]->stats.hero_ally && hero) || (enemym->enemies[i]->stats.enemy_ally && enemym->enemies[i]->stats.summoner == this)) &&
				   (buff_power->buff_party_power_id == 0 || buff_power->buff_party_power_id == enemym->enemies[i]->stats.summoned_power_index)
				) {
					powers->effect(&enemym->enemies[i]->stats, this, power_index, (hero ? Power::SOURCE_TYPE_HERO : Power::SOURCE_TYPE_ENEMY));
				}
			}
		}
	}

	// handle effect timers
	effects.logic();

	// apply bonuses from items/effects to base stats
	applyEffects();

	if (hero && effects.refresh_stats) {
		refresh_stats = true;
		effects.refresh_stats = false;
	}

	// preserve ratio on maxmp and maxhp changes
	float ratio;
	if (prev_maxhp != get(Stats::HP_MAX)) {
		ratio = static_cast<float>(prev_hp) / static_cast<float>(prev_maxhp);
		hp = static_cast<int>(ratio * static_cast<float>(get(Stats::HP_MAX)));
	}
	if (prev_maxmp != get(Stats::MP_MAX)) {
		ratio = static_cast<float>(prev_mp) / static_cast<float>(prev_maxmp);
		mp = static_cast<int>(ratio * static_cast<float>(get(Stats::MP_MAX)));
	}

	// handle cooldowns
	cooldown.tick(); // global cooldown

	for (size_t i=0; i<powers_ai.size(); ++i) { // NPC/enemy powerslot cooldown
		powers_ai[i].cooldown.tick();
	}

	// HP regen
	if (get(Stats::HP_REGEN) > 0 && hp < get(Stats::HP_MAX) && hp > 0) {
		hp_ticker++;
		if (hp_ticker >= (60 * settings->max_frames_per_sec) / get(Stats::HP_REGEN)) {
			hp++;
			hp_ticker = 0;
		}
	}

	// MP regen
	if (get(Stats::MP_REGEN) > 0 && mp < get(Stats::MP_MAX) && hp > 0) {
		mp_ticker++;
		if (mp_ticker >= (60 * settings->max_frames_per_sec) / get(Stats::MP_REGEN)) {
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
		comb->addInt(effects.damage, pos, CombatText::MSG_TAKEDMG);
	}
	if (effects.damage_percent > 0 && hp > 0) {
		int damage = (get(Stats::HP_MAX)*effects.damage_percent)/100;
		takeDamage(damage);
		comb->addInt(damage, pos, CombatText::MSG_TAKEDMG);
	}

	if(effects.death_sentence)
		hp = 0;

	cooldown_hit.tick();

	if (effects.stun) {
		// stun stops charge attacks
		state_timer.reset(Timer::END);
		charge_speed = 0;
	}

	state_timer.tick();

	// apply healing over time
	if (effects.hpot > 0) {
		comb->addString(msg->get("+%d HP",effects.hpot), pos, CombatText::MSG_BUFF);
		hp += effects.hpot;
		if (hp > get(Stats::HP_MAX)) hp = get(Stats::HP_MAX);
	}
	if (effects.hpot_percent > 0) {
		int hpot = (get(Stats::HP_MAX)*effects.hpot_percent)/100;
		comb->addString(msg->get("+%d HP",hpot), pos, CombatText::MSG_BUFF);
		hp += hpot;
		if (hp > get(Stats::HP_MAX)) hp = get(Stats::HP_MAX);
	}
	if (effects.mpot > 0) {
		comb->addString(msg->get("+%d MP",effects.mpot), pos, CombatText::MSG_BUFF);
		mp += effects.mpot;
		if (mp > get(Stats::MP_MAX)) mp = get(Stats::MP_MAX);
	}
	if (effects.mpot_percent > 0) {
		int mpot = (get(Stats::MP_MAX)*effects.mpot_percent)/100;
		comb->addString(msg->get("+%d MP",mpot), pos, CombatText::MSG_BUFF);
		mp += mpot;
		if (mp > get(Stats::MP_MAX)) mp = get(Stats::MP_MAX);
	}

	// set movement type
	// some creatures may shift between movement types
	if (intangible) movement_type = MapCollision::MOVE_INTANGIBLE;
	else if (flying) movement_type = MapCollision::MOVE_FLYING;
	else movement_type = MapCollision::MOVE_NORMAL;

	if (hp == 0)
		removeSummons();

	if (effects.knockback_speed != 0) {
		float theta = Utils::calcTheta(knockback_srcpos.x, knockback_srcpos.y, knockback_destpos.x, knockback_destpos.y);
		knockback_speed.x = effects.knockback_speed * cosf(theta);
		knockback_speed.y = effects.knockback_speed * sinf(theta);

		mapr->collider.unblock(pos.x, pos.y);
		mapr->collider.move(pos.x, pos.y, knockback_speed.x, knockback_speed.y, movement_type, mapr->collider.getCollideType(hero));
		mapr->collider.block(pos.x, pos.y, hero_ally);
	}
	else if (charge_speed != 0.0f) {
		float tmp_speed = charge_speed * speedMultiplyer[direction];
		float dx = tmp_speed * static_cast<float>(directionDeltaX[direction]);
		float dy = tmp_speed * static_cast<float>(directionDeltaY[direction]);

		mapr->collider.unblock(pos.x, pos.y);
		mapr->collider.move(pos.x, pos.y, dx, dy, movement_type, mapr->collider.getCollideType(hero));
		mapr->collider.block(pos.x, pos.y, hero_ally);
	}


	// enemies heal rapidly while not in combat
	if (!in_combat && !hero_ally && !hero) {
		if (alive && pc->stats.alive) {
			hp++;
			if (hp > get(Stats::HP_MAX))
				hp = get(Stats::HP_MAX);
		}
	}

	waypoint_timer.tick();

	// check for revive
	if (hp <= 0 && effects.revive) {
		hp = get(Stats::HP_MAX);
		alive = true;
		corpse = false;
		if (hero)
			cur_state = AVATAR_STANCE;
		else
			cur_state = ENEMY_STANCE;
	}

	// check for bleeding to death
	if (hp <= 0 && !hero && cur_state != ENEMY_DEAD && cur_state != ENEMY_CRITDEAD) {
		for (size_t i = 0; i < effects.effect_list.size(); ++i) {
			if (effects.effect_list[i].type == Effect::DAMAGE || effects.effect_list[i].type == Effect::DAMAGE_PERCENT) {
				bleed_source_type = effects.effect_list[i].source_type;
				break;
			}
		}
		effects.triggered_death = true;
		cur_state = ENEMY_DEAD;
	}
	else if (hp <= 0 && hero && cur_state != AVATAR_DEAD) {
		effects.triggered_death = true;
		cur_state = AVATAR_DEAD;
	}
}

StatBlock::~StatBlock() {
	removeFromSummons();
}

bool StatBlock::canUsePower(int powerid, bool allow_passive) const {
	const Power& power = powers->powers[powerid];

	if (!alive) {
		// can't use powers when dead
		return false;
	}
	else if (!hero) {
		// AI can always use their powers
		return true;
	}
	else if (transformed) {
		// needed to unlock shapeshifter powers
		return mp >= power.requires_mp;
	}
	else {
		return (
			mp >= power.requires_mp
			&& (!power.passive || allow_passive)
			&& !power.meta_power
			&& (!effects.stun || (allow_passive && power.passive))
			&& (power.sacrifice || hp > power.requires_hp)
			&& powers->checkRequiredMaxHPMP(power, this)
			&& (!power.requires_corpse || (target_corpse && !target_corpse->corpse_timer.isEnd()) || (target_nearest_corpse && powers->checkNearestTargeting(power, this, true) && !target_nearest_corpse->corpse_timer.isEnd()))
			&& (checkRequiredSpawns(power.requires_spawns))
			&& (menu_powers && menu_powers->meetsUsageStats(powerid))
			&& (power.type == Power::TYPE_SPAWN ? !summonLimitReached(powerid) : true)
			&& !(power.spawn_type == "untransform" && !transformed)
			&& std::includes(equip_flags.begin(), equip_flags.end(), power.requires_flags.begin(), power.requires_flags.end())
			&& (!power.buff_party || (power.buff_party && enemym && enemym->checkPartyMembers()))
			&& powers->checkRequiredItems(power, this)
		);
	}

}

void StatBlock::loadHeroStats() {
	// set the default global cooldown
	cooldown.setDuration(Parse::toDuration("66ms"));

	// Redefine numbers from config file if present
	FileParser infile;
	// @CLASS StatBlock: Hero stats|Description of engine/stats.txt
	if (infile.open("engine/stats.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			int value = Parse::toInt(infile.val);

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

	max_spendable_stat_points = eset->xp.getMaxLevel() * stat_points_per_level;
}

void StatBlock::loadHeroSFX() {
	// load the paths to base sound effects
	FileParser infile;
	if (infile.open("engine/avatar/"+gfx_base+".txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
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
			(*it)->corpse_timer.reset(Timer::BEGIN);
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

	if(spawn_power->spawn_limit_mode == Power::SPAWN_LIMIT_MODE_FIXED)
		max_summons = spawn_power->spawn_limit_qty;
	else if(spawn_power->spawn_limit_mode == Power::SPAWN_LIMIT_MODE_STAT) {
		int stat_val = 1;
		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			if (spawn_power->spawn_limit_stat == i) {
				stat_val = get_primary(i);
				break;
			}
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
	wander_area.x = int(floorf(pos.x)) - r;
	wander_area.y = int(floorf(pos.y)) - r;
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

	unsigned long xp_max = eset->xp.getLevelXP(eset->xp.getMaxLevel());
	xp = std::min(xp, xp_max);
}

StatBlock::AIPower* StatBlock::getAIPower(int ai_type) {
	std::vector<size_t> possible_ids;
	int chance = rand() % 100;

	for (size_t i=0; i<powers_ai.size(); ++i) {
		if (ai_type != powers_ai[i].type)
			continue;

		if (chance > powers_ai[i].chance)
			continue;

		if (!powers_ai[i].cooldown.isEnd())
			continue;

		if (powers->powers[powers_ai[i].id].type == Power::TYPE_SPAWN) {
			if (summonLimitReached(powers_ai[i].id))
				continue;
		}

		if (!checkRequiredSpawns(powers->powers[powers_ai[i].id].requires_spawns))
			continue;

		possible_ids.push_back(i);
	}

	if (!possible_ids.empty()) {
		size_t index = static_cast<size_t>(rand()) % possible_ids.size();
		return &powers_ai[possible_ids[index]];
	}

	return NULL;
}

bool StatBlock::checkRequiredSpawns(int req_amount) const {
	if (req_amount <= 0)
		return true;

	int live_summon_count = 0;
	for (size_t j=0; j<summons.size(); ++j) {
		if (summons[j]->hp > 0) {
			++live_summon_count;
		}
	}

	if (live_summon_count < req_amount)
		return false;

	return true;
}

int StatBlock::getPowerCooldown(int power_id) {
	if (hero) {
		return pc->power_cooldown_timers[power_id].getDuration();
	}
	else {
		for (size_t i = 0; i < powers_ai.size(); ++i) {
			if (power_id == powers_ai[i].id)
				return powers_ai[i].cooldown.getDuration();
		}
	}

	return 0;
}

void StatBlock::setPowerCooldown(int power_id, int power_cooldown) {
	if (hero) {
		pc->power_cooldown_timers[power_id].setDuration(power_cooldown);
	}
	else {
		for (size_t i = 0; i < powers_ai.size(); ++i) {
			if (power_id == powers_ai[i].id) {
				powers_ai[i].cooldown.setDuration(power_cooldown);
				break;
			}
		}
	}
}
