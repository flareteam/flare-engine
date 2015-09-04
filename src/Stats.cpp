/*
Copyright © 2013 Justin Jacobs

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

#include "Settings.h"
#include "SharedResources.h"
#include "Stats.h"

std::string STAT_KEY[STAT_COUNT];
std::string STAT_NAME[STAT_COUNT];
std::string STAT_DESC[STAT_COUNT];
bool STAT_PERCENT[STAT_COUNT];

// STAT_KEY values aren't visible in-game, but they are used for parsing config files like engine/stats.txt
// STAT_NAME values are the translated strings visible in the Character menu and item tooltips
// STAT_DESC values are the translated descriptions of stats visible in Character menu tooltips
// STAT_PERCENT is used to determine if we should treat the value as a percentage when displaying it (i.e. use %)
void setStatNames() {
	// @CLASS Stats|Description of the base stats, aka $STATNAME

	// @TYPE hp|Hit points
	STAT_KEY[STAT_HP_MAX] = "hp";
	STAT_NAME[STAT_HP_MAX] = msg->get("Max HP");
	STAT_DESC[STAT_HP_MAX] = "";
	STAT_PERCENT[STAT_HP_MAX] = false;
	// @TYPE hp_regen|HP restored per minute
	STAT_KEY[STAT_HP_REGEN] = "hp_regen";
	STAT_NAME[STAT_HP_REGEN] = msg->get("HP Regen");
	STAT_DESC[STAT_HP_REGEN] = msg->get("Ticks of HP regen per minute.");
	STAT_PERCENT[STAT_HP_REGEN] = false;
	// @TYPE mp|Magic points
	STAT_KEY[STAT_MP_MAX] = "mp";
	STAT_NAME[STAT_MP_MAX] = msg->get("Max MP");
	STAT_DESC[STAT_MP_MAX] = "";
	STAT_PERCENT[STAT_MP_MAX] = false;
	// @TYPE mp_regen|MP restored per minute
	STAT_KEY[STAT_MP_REGEN] = "mp_regen";
	STAT_NAME[STAT_MP_REGEN] = msg->get("MP Regen");
	STAT_DESC[STAT_MP_REGEN] = msg->get("Ticks of MP regen per minute.");
	STAT_PERCENT[STAT_MP_REGEN] = false;
	// @TYPE accuracy|Accuracy %. Higher values mean less likely to miss.
	STAT_KEY[STAT_ACCURACY] = "accuracy";
	STAT_NAME[STAT_ACCURACY] = msg->get("Accuracy");
	STAT_DESC[STAT_ACCURACY] = "";
	STAT_PERCENT[STAT_ACCURACY] = true;
	// @TYPE avoidance|Avoidance %. Higher values means more likely to not get hit.
	STAT_KEY[STAT_AVOIDANCE] = "avoidance";
	STAT_NAME[STAT_AVOIDANCE] = msg->get("Avoidance");
	STAT_DESC[STAT_AVOIDANCE] = "";
	STAT_PERCENT[STAT_AVOIDANCE] = true;
	// @TYPE dmg_melee_min|Minimum melee damage
	STAT_KEY[STAT_DMG_MELEE_MIN] = "dmg_melee_min";
	STAT_NAME[STAT_DMG_MELEE_MIN] = msg->get("Melee Damage Min");
	STAT_DESC[STAT_DMG_MELEE_MIN] = "";
	STAT_PERCENT[STAT_DMG_MELEE_MIN] = false;
	// @TYPE dmg_melee_max|Maximum melee damage
	STAT_KEY[STAT_DMG_MELEE_MAX] = "dmg_melee_max";
	STAT_NAME[STAT_DMG_MELEE_MAX] = msg->get("Melee Damage Max");
	STAT_DESC[STAT_DMG_MELEE_MAX] = "";
	STAT_PERCENT[STAT_DMG_MELEE_MAX] = false;
	// @TYPE dmg_ranged_min|Minimum ranged damage
	STAT_KEY[STAT_DMG_RANGED_MIN] = "dmg_ranged_min";
	STAT_NAME[STAT_DMG_RANGED_MIN] = msg->get("Ranged Damage Min");
	STAT_DESC[STAT_DMG_RANGED_MIN] = "";
	STAT_PERCENT[STAT_DMG_RANGED_MIN] = false;
	// @TYPE dmg_ranged_max|Maximum ranged damage
	STAT_KEY[STAT_DMG_RANGED_MAX] = "dmg_ranged_max";
	STAT_NAME[STAT_DMG_RANGED_MAX] = msg->get("Ranged Damage Max");
	STAT_DESC[STAT_DMG_RANGED_MAX] = "";
	STAT_PERCENT[STAT_DMG_RANGED_MAX] = false;
	// @TYPE dmg_ment_min|Minimum mental damage
	STAT_KEY[STAT_DMG_MENT_MIN] = "dmg_ment_min";
	STAT_NAME[STAT_DMG_MENT_MIN] = msg->get("Mental Damage Min");
	STAT_DESC[STAT_DMG_MENT_MIN] = "";
	STAT_PERCENT[STAT_DMG_MENT_MIN] = false;
	// @TYPE dmg_ment_max|Maximum mental damage
	STAT_KEY[STAT_DMG_MENT_MAX] = "dmg_ment_max";
	STAT_NAME[STAT_DMG_MENT_MAX] = msg->get("Mental Damage Max");
	STAT_DESC[STAT_DMG_MENT_MAX] = "";
	STAT_PERCENT[STAT_DMG_MENT_MAX] = false;
	// @TYPE absorb_min|Minimum damage absorption
	STAT_KEY[STAT_ABS_MIN] = "absorb_min";
	STAT_NAME[STAT_ABS_MIN] = msg->get("Absorb Min");
	STAT_DESC[STAT_ABS_MIN] = "";
	STAT_PERCENT[STAT_ABS_MIN] = false;
	// @TYPE absorb_max|Maximum damage absorption
	STAT_KEY[STAT_ABS_MAX] = "absorb_max";
	STAT_NAME[STAT_ABS_MAX] = msg->get("Absorb Max");
	STAT_DESC[STAT_ABS_MAX] = "";
	STAT_PERCENT[STAT_ABS_MAX] = false;
	// @TYPE crit|Critical hit chance %
	STAT_KEY[STAT_CRIT] = "crit";
	STAT_NAME[STAT_CRIT] = msg->get("Critical Hit Chance");
	STAT_DESC[STAT_CRIT] = "";
	STAT_PERCENT[STAT_CRIT] = true;
	// @TYPE xp_gain|Percentage boost to the amount of experience points gained per kill.
	STAT_KEY[STAT_XP_GAIN] = "xp_gain";
	STAT_NAME[STAT_XP_GAIN] = msg->get("Bonus XP");
	STAT_DESC[STAT_XP_GAIN] = msg->get("Increases the XP gained per kill.");
	STAT_PERCENT[STAT_XP_GAIN] = true;
	// @TYPE currency_find|Percentage boost to the amount of gold dropped per loot event.
	STAT_KEY[STAT_CURRENCY_FIND] = "currency_find";
	STAT_NAME[STAT_CURRENCY_FIND] = msg->get("Bonus %s", CURRENCY);
	STAT_DESC[STAT_CURRENCY_FIND] = msg->get("Increases the %s found per drop.", CURRENCY);
	STAT_PERCENT[STAT_CURRENCY_FIND] = true;
	// @TYPE item_find|Increases the chance of finding items in loot.
	STAT_KEY[STAT_ITEM_FIND] = "item_find";
	STAT_NAME[STAT_ITEM_FIND] = msg->get("Item Find Chance");
	STAT_DESC[STAT_ITEM_FIND] = msg->get("Increases the chance that an enemy will drop an item.");
	STAT_PERCENT[STAT_ITEM_FIND] = true;
	// @TYPE stealth|Decrease the distance required to alert enemies by %
	STAT_KEY[STAT_STEALTH] = "stealth";
	STAT_NAME[STAT_STEALTH] = msg->get("Stealth");
	STAT_DESC[STAT_STEALTH] = msg->get("Increases your ability to move undetected.");
	STAT_PERCENT[STAT_STEALTH] = true;
	// @TYPE poise|Reduced % chance of entering "hit" animation when damaged
	STAT_KEY[STAT_POISE] = "poise";
	STAT_NAME[STAT_POISE] = msg->get("Poise");
	STAT_DESC[STAT_POISE] = msg->get("Reduces your chance of stumbling when hit.");
	STAT_PERCENT[STAT_POISE] = true;
	// @TYPE reflect_chance|Percentage chance to reflect missiles
	STAT_KEY[STAT_REFLECT] = "reflect_chance";
	STAT_NAME[STAT_REFLECT] = msg->get("Reflect Chance");
	STAT_DESC[STAT_REFLECT] = msg->get("Increases your chance of reflecting missiles back at enemies.");
	STAT_PERCENT[STAT_REFLECT] = true;
	// @TYPE hp_percent|Base HP altered by percentage
	STAT_KEY[STAT_HP_PERCENT] = "hp_percent";
	STAT_NAME[STAT_HP_PERCENT] = msg->get("Base HP");
	STAT_DESC[STAT_HP_PERCENT] = "";
	STAT_PERCENT[STAT_HP_PERCENT] = true;
	// @TYPE mp_percent|Base MP altered by percentage
	STAT_KEY[STAT_MP_PERCENT] = "mp_percent";
	STAT_NAME[STAT_MP_PERCENT] = msg->get("Base MP");
	STAT_DESC[STAT_MP_PERCENT] = "";
	STAT_PERCENT[STAT_MP_PERCENT] = true;
}

