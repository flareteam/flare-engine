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

#include "FileParser.h"
#include "StatBlock.h"
#include "Utils.h"
#include "UtilsParsing.h"
#include "XPScaling.h"

XPScalingTable::XPScalingTable()
	: absolute_level_min(0)
	, absolute_level_max(0)
	, relative_level_min(0)
	, relative_level_max(0)
{
}

XPScalingTable::~XPScalingTable() {
}

XPScaling::XPScaling() {
}

XPScaling::~XPScaling() {
}

XPScalingTableID XPScaling::load(const std::string& filename) {
	XPScalingTableID table_id = Utils::hashString(filename);
	if (table_id == 0)
		return table_id;

	if (tables.find(table_id) == tables.end())
		tables[table_id] = XPScalingTable();
	else
		return table_id;

	XPScalingTable& table = tables[table_id];

	FileParser infile;
	// @CLASS XPScaling|Description of enemies/xp_scaling/
	if (infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.section == "absolute") {
				if (infile.new_section) {
					table.absolute.clear();
				}

				// @ATTR absolute.level|int, float : Level, Multiplier|The multiplier that will be applied to the rewarded XP when an enemy is at a specific level. If the enemy's level is outside the defined scaling levels, the min or max level is used (whichever is closer).
				if (infile.key == "level") {
					int level = Parse::popFirstInt(infile.val);
					float multiplier = Parse::popFirstFloat(infile.val);

					if (table.absolute.empty()) {
						table.absolute_level_min = table.absolute_level_max = level;
					}
					else {
						if (level < table.absolute_level_min)
							table.absolute_level_min = level;
						if (level > table.absolute_level_max)
							table.absolute_level_max = level;
					}
					table.absolute[level] = multiplier;
				}
				else {
					infile.error("XPScaling: '%s' is not a valid key.", infile.key.c_str());
				}
			}
			else if (infile.section == "relative") {
				if (infile.new_section) {
					table.relative.clear();
				}

				// @ATTR relative.level|int, float : Level, Multiplier|The multiplier that will be applied to the rewarded XP when an enemy's level is X levels apart from the player's level. If the enemy's level is outside the defined scaling levels, the min or max level is used (whichever is closer).
				if (infile.key == "level") {
					int level = Parse::popFirstInt(infile.val);
					float multiplier = Parse::popFirstFloat(infile.val);

					if (table.relative.empty()) {
						table.relative_level_min = table.relative_level_max = level;
					}
					else {
						if (level < table.relative_level_min)
							table.relative_level_min = level;
						if (level > table.relative_level_max)
							table.relative_level_max = level;
					}
					table.relative[level] = multiplier;
				}
				else {
					infile.error("XPScaling: '%s' is not a valid key.", infile.key.c_str());
				}
			}
		}

		infile.close();
	}

	// ensure there are no gaps in the level tables
	for (int i = table.absolute_level_max - 1; i > table.absolute_level_min; --i) {
		std::map<int, float>::iterator it = table.absolute.find(i);
		if (it == table.absolute.end()) {
			table.absolute[i] = table.absolute[i+1];
		}
	}
	for (int i = table.relative_level_max - 1; i > table.relative_level_min; --i) {
		std::map<int, float>::iterator it = table.relative.find(i);
		if (it == table.relative.end()) {
			table.relative[i] = table.relative[i+1];
		}
	}

	return table_id;
}

float XPScaling::getMultiplier(StatBlock* enemy_stats, StatBlock* player_stats) {
	float multiplier = 1;

	if (!enemy_stats || !player_stats || enemy_stats->xp_scaling_table == 0)
		return multiplier;

	std::map<XPScalingTableID, XPScalingTable>::iterator table_it = tables.find(enemy_stats->xp_scaling_table);
	if (table_it != tables.end()) {
		XPScalingTable& table = table_it->second;

		if (!table.absolute.empty()) {
			int level = enemy_stats->level;

			std::map<int, float>::iterator absolute_it = table.absolute.find(level);
			if (absolute_it != table.absolute.end()) {
				multiplier *= absolute_it->second;
			}
			else {
				if (level < table.absolute_level_min)
					multiplier *= table.absolute[table.absolute_level_min];
				else if (level > table.absolute_level_max)
					multiplier *= table.absolute[table.absolute_level_max];
			}
		}

		if (!table.relative.empty()) {
			int level = enemy_stats->level - player_stats->level;

			std::map<int, float>::iterator relative_it = table.relative.find(level);
			if (relative_it != table.relative.end()) {
				multiplier *= relative_it->second;
			}
			else {
				if (level < table.relative_level_min)
					multiplier *= table.relative[table.relative_level_min];
				else if (level > table.relative_level_max)
					multiplier *= table.relative[table.relative_level_max];
			}
		}
	}

	return multiplier;
}
