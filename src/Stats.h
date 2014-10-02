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

#ifndef STATS_H
#define STATS_H

#include "CommonIncludes.h"

#define STAT_COUNT 22

enum STAT {
	STAT_DMG_MELEE_MIN = 0,
	STAT_DMG_MELEE_MAX = 1,
	STAT_DMG_RANGED_MIN = 2,
	STAT_DMG_RANGED_MAX = 3,
	STAT_DMG_MENT_MIN = 4,
	STAT_DMG_MENT_MAX = 5,
	STAT_ABS_MIN = 6,
	STAT_ABS_MAX = 7,
	STAT_HP_MAX = 8,
	STAT_HP_REGEN = 9,
	STAT_HP_PERCENT = 10,
	STAT_MP_MAX = 11,
	STAT_MP_REGEN = 12,
	STAT_MP_PERCENT = 13,
	STAT_ACCURACY = 14,
	STAT_AVOIDANCE = 15,
	STAT_CRIT = 16,
	STAT_XP_GAIN = 17,
	STAT_CURRENCY_FIND = 18,
	STAT_ITEM_FIND = 19,
	STAT_STEALTH = 20,
	STAT_POISE = 21
};

extern std::string STAT_NAME[STAT_COUNT];
void setStatNames();

#endif
