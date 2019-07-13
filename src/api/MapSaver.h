/*
Copyright © 2015 Igor Paliychuk

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

/*
 * class MapSaver
 */

#ifdef FLARE_MAP_SAVER

#ifndef MAP_SAVER_H
#define MAP_SAVER_H

#include "Map.h"

class MapSaver {
public:
	MapSaver(Map* _map);
	~MapSaver();

	bool saveMap(std::string tileset_definitions);
	bool saveMap(std::string file, std::string tileset_definitions);

private:
	void writeHeader(std::ofstream& map_file);
	void writeTilesets(std::ofstream& map_file, std::string tileset_definitions);
	void writeLayers(std::ofstream& map_file);
	void writeEnemies(std::ofstream& map_file);
	void writeNPCs(std::ofstream& map_file);
	void writeEvents(std::ofstream& map_file);
	void writeEventComponents(std::ofstream& map_file, int eventID);

	Map* map;
	std::string dest_file;

	static const int EC_COUNT = 43;

	std::string EVENT_COMPONENT_NAME[EC_COUNT];

	class EC {
	public:
		enum {
			NONE = 0,
			TOOLTIP = 1,
			POWER_PATH = 2,
			POWER_DAMAGE = 3,
			INTERMAP = 4,
			INTRAMAP = 5,
			MAPMOD = 6,
			SOUNDFX = 7,
			LOOT = 8,
			LOOT_COUNT = 9,
			MSG = 10,
			SHAKYCAM = 11,
			REQUIRES_STATUS = 12,
			REQUIRES_NOT_STATUS = 13,
			REQUIRES_LEVEL = 14,
			REQUIRES_NOT_LEVEL = 15,
			REQUIRES_CURRENCY = 16,
			REQUIRES_NOT_CURRENCY = 17,
			REQUIRES_ITEM = 18,
			REQUIRES_NOT_ITEM = 19,
			REQUIRES_CLASS = 20,
			REQUIRES_NOT_CLASS = 21,
			SET_STATUS = 22,
			UNSET_STATUS = 23,
			REMOVE_CURRENCY = 24,
			REMOVE_ITEM = 25,
			REWARD_XP = 26,
			REWARD_CURRENCY = 27,
			REWARD_ITEM = 28,
			REWARD_LOOT = 29,
			REWARD_LOOT_COUNT = 30,
			RESTORE = 31,
			POWER = 32,
			SPAWN = 33,
			STASH = 34,
			NPC = 35,
			MUSIC = 36,
			CUTSCENE = 37,
			REPEAT = 38,
			SAVE_GAME = 39,
			BOOK = 40,
			SCRIPT = 41,
			CHANCE_EXEC = 42,
			RESPEC = 43,
		};

		int type;
		std::string s;
		StatusID status;
		int x;
		int y;
		int z;
		int a;
		int b;
		int c;
		float f;

		EC()
			: type(NONE)
			, s("")
			, status(0)
			, x(0)
			, y(0)
			, z(0)
			, a(0)
			, b(0)
			, c(0)
			, f(0) {
		}
	};

};

#endif //MAP_SAVER

#endif //FLARE_MAP_SAVER
