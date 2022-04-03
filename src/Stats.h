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

namespace Stats {
	enum STAT {
		HP_MAX = 0,
		HP_REGEN,
		MP_MAX,
		MP_REGEN,
		ACCURACY,
		AVOIDANCE,
		ABS_MIN,
		ABS_MAX,
		CRIT,
		XP_GAIN,
		CURRENCY_FIND,
		ITEM_FIND,
		STEALTH,
		POISE,
		REFLECT,
		RETURN_DAMAGE,
		HP_STEAL,
		MP_STEAL,
		// all the below stats are hidden by default in MenuCharacter
		RESIST_DAMAGE_OVER_TIME,
		RESIST_SLOW,
		RESIST_STUN,
		RESIST_KNOCKBACK,
		RESIST_STAT_DEBUFF,
		RESIST_DAMAGE_REFLECT,
		RESIST_HP_STEAL,
		RESIST_MP_STEAL,
		HP_PERCENT,
		MP_PERCENT,
		COUNT
	};

	extern std::string KEY[COUNT];
	extern std::string NAME[COUNT];
	extern std::string DESC[COUNT];
	extern bool PERCENT[COUNT];
	void init();
}

#endif
