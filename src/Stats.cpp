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

#include "Stats.h"

std::string STAT_NAME[STAT_COUNT];

// these names aren't visible in-game
// but they are used for parsing config files like engine/stats.txt
void setStatNames() {
	// @CLASS Stats|Description of the base stats, aka $STATNAME

	// @TYPE dmg_melee_min|Minimum melee damage
	STAT_NAME[STAT_DMG_MELEE_MIN] = "dmg_melee_min";
	// @TYPE dmg_melee_max|Maximum melee damage
	STAT_NAME[STAT_DMG_MELEE_MAX] = "dmg_melee_max";
	// @TYPE dmg_ranged_min|Minimum ranged damage
	STAT_NAME[STAT_DMG_RANGED_MIN] = "dmg_ranged_min";
	// @TYPE dmg_ranged_max|Maximum ranged damage
	STAT_NAME[STAT_DMG_RANGED_MAX] = "dmg_ranged_max";
	// @TYPE dmg_ment_min|Minimum mental damage
	STAT_NAME[STAT_DMG_MENT_MIN] = "dmg_ment_min";
	// @TYPE dmg_ment_max|Maximum mental damage
	STAT_NAME[STAT_DMG_MENT_MAX] = "dmg_ment_max";
	// @TYPE absorb_min|Minimum damage absorption
	STAT_NAME[STAT_ABS_MIN] = "absorb_min";
	// @TYPE absorb_max|Maximum damage absorption
	STAT_NAME[STAT_ABS_MAX] = "absorb_max";
	// @TYPE hp|Hit points
	STAT_NAME[STAT_HP_MAX] = "hp";
	// @TYPE hp_regen|HP restored per minute
	STAT_NAME[STAT_HP_REGEN] = "hp_regen";
	// @TYPE hp_percent|Base HP altered by percentage
	STAT_NAME[STAT_HP_PERCENT] = "hp_percent";
	// @TYPE mp|Magic points
	STAT_NAME[STAT_MP_MAX] = "mp";
	// @TYPE mp_regen|MP restored per minute
	STAT_NAME[STAT_MP_REGEN] = "mp_regen";
	// @TYPE mp_percent|Base MP altered by percentage
	STAT_NAME[STAT_MP_PERCENT] = "mp_percent";
	// @TYPE accuracy|Accuracy %. Higher values mean less likely to miss.
	STAT_NAME[STAT_ACCURACY] = "accuracy";
	// @TYPE avoidance|Avoidance %. Higher values means more likely to not get hit.
	STAT_NAME[STAT_AVOIDANCE] = "avoidance";
	// @TYPE crit|Critical hit chance %
	STAT_NAME[STAT_CRIT] = "crit";
	// @TYPE xp_gain|Percentage boost to the amount of experience points gained per kill.
	STAT_NAME[STAT_XP_GAIN] = "xp_gain";
	// @TYPE currency_find|Percentage boost to the amount of gold dropped per loot event.
	STAT_NAME[STAT_CURRENCY_FIND] = "currency_find";
	// @TYPE item_find|Increases the chance of finding items in loot.
	STAT_NAME[STAT_ITEM_FIND] = "item_find";
	// @TYPE stealth|Decrease the distance required to alert enemies by %
	STAT_NAME[STAT_STEALTH] = "stealth";
	// @TYPE poise|Reduced % chance of entering "hit" animation when damaged
	STAT_NAME[STAT_POISE] = "poise";
	// @TYPE reflect_chance|Percentage chance to reflect missiles
	STAT_NAME[STAT_REFLECT] = "reflect_chance";
}

