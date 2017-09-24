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

#ifndef STATS_H
#define STATS_H

#include "CommonIncludes.h"

#define STAT_COUNT 20

enum STAT {
	STAT_HP_MAX = 0,
	STAT_HP_REGEN = 1,
	STAT_MP_MAX = 2,
	STAT_MP_REGEN = 3,
	STAT_ACCURACY = 4,
	STAT_AVOIDANCE = 5,
	STAT_ABS_MIN = 6,
	STAT_ABS_MAX = 7,
	STAT_CRIT = 8,
	STAT_XP_GAIN = 9,
	STAT_CURRENCY_FIND = 10,
	STAT_ITEM_FIND = 11,
	STAT_STEALTH = 12,
	STAT_POISE = 13,
	STAT_REFLECT = 14,
	STAT_RETURN_DAMAGE = 15,
	STAT_HP_STEAL = 16,
	STAT_MP_STEAL = 17,
	// STAT_HP_PERCENT & STAT_MP_PERCENT aren't displayed in MenuCharacter; new stats should be added above this comment.
	STAT_HP_PERCENT = 18,
	STAT_MP_PERCENT = 19
};

extern std::string STAT_KEY[STAT_COUNT];
extern std::string STAT_NAME[STAT_COUNT];
extern std::string STAT_DESC[STAT_COUNT];
extern bool STAT_PERCENT[STAT_COUNT];
void setStatNames();

#endif
