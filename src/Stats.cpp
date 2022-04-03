/*
Copyright © 2013-2015 Justin Jacobs

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

#include "EngineSettings.h"
#include "MessageEngine.h"
#include "SharedResources.h"
#include "Stats.h"

namespace Stats {
	std::string KEY[COUNT];
	std::string NAME[COUNT];
	std::string DESC[COUNT];
	bool PERCENT[COUNT];

	// KEY values aren't visible in-game, but they are used for parsing config files like engine/stats.txt
	// NAME values are the translated strings visible in the Character menu and item tooltips
	// DESC values are the translated descriptions of stats visible in Character menu tooltips
	// PERCENT is used to determine if we should treat the value as a percentage when displaying it (i.e. use %)
	void init() {
		// @CLASS Stats|Description of the base stats which may be used wherever a stat_id is required.

		// @TYPE hp|Hit points
		KEY[HP_MAX] = "hp";
		NAME[HP_MAX] = msg->get("Max HP");
		DESC[HP_MAX] = msg->get("Total amount of HP.");
		PERCENT[HP_MAX] = false;

		// @TYPE hp_regen|HP restored per minute
		KEY[HP_REGEN] = "hp_regen";
		NAME[HP_REGEN] = msg->get("HP Regen");
		DESC[HP_REGEN] = msg->get("Ticks of HP regen per minute.");
		PERCENT[HP_REGEN] = false;

		// @TYPE mp|Magic points
		KEY[MP_MAX] = "mp";
		NAME[MP_MAX] = msg->get("Max MP");
		DESC[MP_MAX] = msg->get("Total amount of MP.");
		PERCENT[MP_MAX] = false;

		// @TYPE mp_regen|MP restored per minute
		KEY[MP_REGEN] = "mp_regen";
		NAME[MP_REGEN] = msg->get("MP Regen");
		DESC[MP_REGEN] = msg->get("Ticks of MP regen per minute.");
		PERCENT[MP_REGEN] = false;

		// @TYPE accuracy|Accuracy %. Higher values mean less likely to miss.
		KEY[ACCURACY] = "accuracy";
		NAME[ACCURACY] = msg->get("Accuracy");
		DESC[ACCURACY] = msg->get("Accuracy rating. The enemy's Avoidance rating is subtracted from this value to calculate your likeliness to land a direct hit.");
		PERCENT[ACCURACY] = true;

		// @TYPE avoidance|Avoidance %. Higher values means more likely to not get hit.
		KEY[AVOIDANCE] = "avoidance";
		NAME[AVOIDANCE] = msg->get("Avoidance");
		DESC[AVOIDANCE] = msg->get("Avoidance rating. This value is subtracted from the enemy's Accuracy rating to calculate their likeliness to land a direct hit.");
		PERCENT[AVOIDANCE] = true;

		// @TYPE absorb_min|Minimum damage absorption
		KEY[ABS_MIN] = "absorb_min";
		NAME[ABS_MIN] = msg->get("Absorb Min");
		DESC[ABS_MIN] = msg->get("Reduces the amount of damage taken.");
		PERCENT[ABS_MIN] = false;

		// @TYPE absorb_max|Maximum damage absorption
		KEY[ABS_MAX] = "absorb_max";
		NAME[ABS_MAX] = msg->get("Absorb Max");
		DESC[ABS_MAX] = msg->get("Reduces the amount of damage taken.");
		PERCENT[ABS_MAX] = false;

		// @TYPE crit|Critical hit chance %
		KEY[CRIT] = "crit";
		NAME[CRIT] = msg->get("Critical Hit Chance");
		DESC[CRIT] = msg->get("Chance for an attack to do extra damage.");
		PERCENT[CRIT] = true;

		// @TYPE xp_gain|Percentage boost to the amount of experience points gained per kill.
		KEY[XP_GAIN] = "xp_gain";
		NAME[XP_GAIN] = msg->get("Bonus XP");
		DESC[XP_GAIN] = msg->get("Increases the XP gained per kill.");
		PERCENT[XP_GAIN] = true;

		// @TYPE currency_find|Percentage boost to the amount of gold dropped per loot event.
		KEY[CURRENCY_FIND] = "currency_find";
		NAME[CURRENCY_FIND] = msg->get("Bonus %s", eset->loot.currency);
		DESC[CURRENCY_FIND] = msg->get("Increases the %s found per drop.", eset->loot.currency);
		PERCENT[CURRENCY_FIND] = true;

		// @TYPE item_find|Increases the chance of finding items in loot.
		KEY[ITEM_FIND] = "item_find";
		NAME[ITEM_FIND] = msg->get("Item Find Chance");
		DESC[ITEM_FIND] = msg->get("Increases the chance that an enemy will drop an item.");
		PERCENT[ITEM_FIND] = true;

		// @TYPE stealth|Decrease the distance required to alert enemies by %
		KEY[STEALTH] = "stealth";
		NAME[STEALTH] = msg->get("Stealth");
		DESC[STEALTH] = msg->get("Increases your ability to move undetected.");
		PERCENT[STEALTH] = true;

		// @TYPE poise|Reduced % chance of entering "hit" animation when damaged
		KEY[POISE] = "poise";
		NAME[POISE] = msg->get("Poise");
		DESC[POISE] = msg->get("Reduces your chance of stumbling when hit.");
		PERCENT[POISE] = true;

		// @TYPE reflect_chance|Percentage chance to reflect missiles
		KEY[REFLECT] = "reflect_chance";
		NAME[REFLECT] = msg->get("Missile Reflect Chance");
		DESC[REFLECT] = msg->get("Increases your chance of reflecting missiles back at enemies.");
		PERCENT[REFLECT] = true;

		// @TYPE return_damage|Deals a percentage of the damage taken back to the attacker
		KEY[RETURN_DAMAGE] = "return_damage";
		NAME[RETURN_DAMAGE] = msg->get("Damage Reflection");
		DESC[RETURN_DAMAGE] = msg->get("Deals a percentage of damage taken back to the attacker.");
		PERCENT[RETURN_DAMAGE] = true;

		// @TYPE hp_steal|Percentage of HP stolen when damaging a target
		KEY[HP_STEAL] = "hp_steal";
		NAME[HP_STEAL] = msg->get("HP Steal");
		DESC[HP_STEAL] = msg->get("Percentage of HP stolen per hit.");
		PERCENT[HP_STEAL] = true;

		// @TYPE mp_steal|Percentage of MP stolen when damaging a target
		KEY[MP_STEAL] = "mp_steal";
		NAME[MP_STEAL] = msg->get("MP Steal");
		DESC[MP_STEAL] = msg->get("Percentage of MP stolen per hit.");
		PERCENT[MP_STEAL] = true;

		// @TYPE resist_damage_over_time|Percentage chance that damage-over-time effects will be negated
		KEY[RESIST_DAMAGE_OVER_TIME] = "resist_damage_over_time";
		NAME[RESIST_DAMAGE_OVER_TIME] = msg->get("Resistance to damage-over-time");
		DESC[RESIST_DAMAGE_OVER_TIME] = msg->get("Percentage chance that damage-over-time effects will be negated.");
		PERCENT[RESIST_DAMAGE_OVER_TIME] = true;

		// @TYPE resist_slow|Percentage chance that slow effects will be negated
		KEY[RESIST_SLOW] = "resist_slow";
		NAME[RESIST_SLOW] = msg->get("Resistance to slow");
		DESC[RESIST_SLOW] = msg->get("Percentage chance that slow effects will be negated.");
		PERCENT[RESIST_SLOW] = true;

		// @TYPE resist_stun|Percentage chance that stun effects will be negated
		KEY[RESIST_STUN] = "resist_stun";
		NAME[RESIST_STUN] = msg->get("Resistance to stun");
		DESC[RESIST_STUN] = msg->get("Percentage chance that stun effects will be negated.");
		PERCENT[RESIST_STUN] = true;

		// @TYPE resist_knockback|Percentage chance that knockback effects will be negated
		KEY[RESIST_KNOCKBACK] = "resist_knockback";
		NAME[RESIST_KNOCKBACK] = msg->get("Resistance to knockback");
		DESC[RESIST_KNOCKBACK] = msg->get("Percentage chance that knockback effects will be negated.");
		PERCENT[RESIST_KNOCKBACK] = true;

		// @TYPE resist_stat_debuff|Percentage chance that stat debuff effects will be negated
		KEY[RESIST_STAT_DEBUFF] = "resist_stat_debuff";
		NAME[RESIST_STAT_DEBUFF] = msg->get("Resistance to stat debuffs");
		DESC[RESIST_STAT_DEBUFF] = msg->get("Percentage chance that stat debuff effects will be negated.");
		PERCENT[RESIST_STAT_DEBUFF] = true;

		// @TYPE resist_damage_reflect|Percentage chance that damage reflection will be negated
		KEY[RESIST_DAMAGE_REFLECT] = "resist_damage_reflect";
		NAME[RESIST_DAMAGE_REFLECT] = msg->get("Resistance to damage reflection");
		DESC[RESIST_DAMAGE_REFLECT] = msg->get("Percentage chance that damage reflection will be negated.");
		PERCENT[RESIST_DAMAGE_REFLECT] = true;

		// @TYPE resist_hp_steal|Percentage chance that HP steal will be negated
		KEY[RESIST_HP_STEAL] = "resist_hp_steal";
		NAME[RESIST_HP_STEAL] = msg->get("Resistance to HP steal");
		DESC[RESIST_HP_STEAL] = msg->get("Percentage chance that HP steal will be negated.");
		PERCENT[RESIST_HP_STEAL] = true;

		// @TYPE resist_mp_steal|Percentage chance that MP steal will be negated
		KEY[RESIST_MP_STEAL] = "resist_mp_steal";
		NAME[RESIST_MP_STEAL] = msg->get("Resistance to MP steal");
		DESC[RESIST_MP_STEAL] = msg->get("Percentage chance that MP steal will be negated.");
		PERCENT[RESIST_MP_STEAL] = true;

		// @TYPE hp_percent|Base HP altered by percentage
		KEY[HP_PERCENT] = "hp_percent";
		NAME[HP_PERCENT] = msg->get("Base HP");
		DESC[HP_PERCENT] = "";
		PERCENT[HP_PERCENT] = true;

		// @TYPE mp_percent|Base MP altered by percentage
		KEY[MP_PERCENT] = "mp_percent";
		NAME[MP_PERCENT] = msg->get("Base MP");
		DESC[MP_PERCENT] = "";
		PERCENT[MP_PERCENT] = true;
	}
}
