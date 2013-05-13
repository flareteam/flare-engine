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

#pragma once
#ifndef STATS_H
#define STATS_H

#include "CommonIncludes.h"

#define STAT_COUNT 26

enum STAT {
	STAT_PHYSICAL = 0,
	STAT_MENTAL = 1,
	STAT_OFFENSE = 2,
	STAT_DEFENSE = 3,
	STAT_DMG_MELEE_MIN = 4,
	STAT_DMG_MELEE_MAX = 5,
	STAT_DMG_RANGED_MIN = 6,
	STAT_DMG_RANGED_MAX = 7,
	STAT_DMG_MENT_MIN = 8,
	STAT_DMG_MENT_MAX = 9,
	STAT_ABS_MIN = 10,
	STAT_ABS_MAX = 11,
	STAT_HP_MAX = 12,
	STAT_HP_REGEN = 13,
	STAT_HP_PERCENT = 14,
	STAT_MP_MAX = 15,
	STAT_MP_REGEN = 16,
	STAT_MP_PERCENT = 17,
	STAT_ACCURACY = 18,
	STAT_AVOIDANCE = 19,
	STAT_CRIT = 20,
	STAT_XP_GAIN = 21,
	STAT_CURRENCY_FIND = 22,
	STAT_ITEM_FIND = 23,
	STAT_STEALTH = 24,
	STAT_POISE = 25
};

extern std::string STAT_NAME[STAT_COUNT];
void setStatNames();

#endif
