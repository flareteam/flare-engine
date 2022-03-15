/*
Copyright © 2012-2022 Justin Jacobs

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
 * class XPScaling
 *
 * XP scaling based on enemy level
 */

#ifndef XPSCALING_H
#define XPSCALING_H

#include "CommonIncludes.h"
#include "Utils.h"

class StatBlock;

class XPScalingTable {
public:
	XPScalingTable();
	~XPScalingTable();

	std::map<int, float> absolute;
	std::map<int, float> relative;

	int absolute_level_min;
	int absolute_level_max;
	int relative_level_min;
	int relative_level_max;
};

class XPScaling {
public:
	XPScaling();
	~XPScaling();

	XPScalingTableID load(const std::string& filename);
	float getMultiplier(StatBlock* enemy_stats, StatBlock* player_stats);

	std::map<XPScalingTableID, XPScalingTable> tables;
};

#endif
