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

#define STAT_COUNT 24

enum STAT {
	STAT_HP_MAX = 0,
	STAT_HP_REGEN = 1,
	STAT_MP_MAX = 2,
	STAT_MP_REGEN = 3,
	STAT_ACCURACY = 4,
	STAT_AVOIDANCE = 5,
	STAT_DMG_MELEE_MIN = 6,
	STAT_DMG_MELEE_MAX = 7,
	STAT_DMG_RANGED_MIN = 8,
	STAT_DMG_RANGED_MAX = 9,
	STAT_DMG_MENT_MIN = 10,
	STAT_DMG_MENT_MAX = 11,
	STAT_ABS_MIN = 12,
	STAT_ABS_MAX = 13,
	STAT_CRIT = 14,
	STAT_XP_GAIN = 15,
	STAT_CURRENCY_FIND = 16,
	STAT_ITEM_FIND = 17,
	STAT_STEALTH = 18,
	STAT_POISE = 19,
	STAT_REFLECT = 20,
	STAT_RETURN_DAMAGE = 21,
	// since STAT_HP_PERCENT & STAT_MP_PERCENT aren't displayed in MenuCharacter, new stats should be added here.
	// Otherwise, their values won't update in MenuCharacter
	STAT_HP_PERCENT = 22,
	STAT_MP_PERCENT = 23
};

extern std::string STAT_KEY[STAT_COUNT];
extern std::string STAT_NAME[STAT_COUNT];
extern std::string STAT_DESC[STAT_COUNT];
extern bool STAT_PERCENT[STAT_COUNT];
void setStatNames();

#endif
