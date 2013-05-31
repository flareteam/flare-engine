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
	STAT_NAME[STAT_DMG_MELEE_MIN] = "dmg_melee_min";
	STAT_NAME[STAT_DMG_MELEE_MAX] = "dmg_melee_max";
	STAT_NAME[STAT_DMG_RANGED_MIN] = "dmg_ranged_min";
	STAT_NAME[STAT_DMG_RANGED_MAX] = "dmg_ranged_max";
	STAT_NAME[STAT_DMG_MENT_MIN] = "dmg_ment_min";
	STAT_NAME[STAT_DMG_MENT_MAX] = "dmg_ment_max";
	STAT_NAME[STAT_ABS_MIN] = "absorb_min";
	STAT_NAME[STAT_ABS_MAX] = "absorb_max";
	STAT_NAME[STAT_HP_MAX] = "hp";
	STAT_NAME[STAT_HP_REGEN] = "hp_regen";
	STAT_NAME[STAT_HP_PERCENT] = "hp_percent";
	STAT_NAME[STAT_MP_MAX] = "mp";
	STAT_NAME[STAT_MP_REGEN] = "mp_regen";
	STAT_NAME[STAT_MP_PERCENT] = "mp_percent";
	STAT_NAME[STAT_ACCURACY] = "accuracy";
	STAT_NAME[STAT_AVOIDANCE] = "avoidance";
	STAT_NAME[STAT_CRIT] = "crit";
	STAT_NAME[STAT_XP_GAIN] = "xp_gain";
	STAT_NAME[STAT_CURRENCY_FIND] = "currency_find";
	STAT_NAME[STAT_ITEM_FIND] = "item_find";
	STAT_NAME[STAT_STEALTH] = "stealth";
	STAT_NAME[STAT_POISE] = "poise";
}

