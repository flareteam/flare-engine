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

#ifndef MAP_SAVER
#define MAP_SAVER

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

	std::string EVENT_COMPONENT_NAME[39];
};

#endif //MAP_SAVER
