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

#ifndef MAP_SAVER_H
#define MAP_SAVER_H

#include "EventManager.h"
#include "Map.h"

class MapSaver {
public:
	MapSaver(Map* _map);
	~MapSaver();

	bool saveMap(const std::string& tileset_definitions);
	bool saveMap(const std::string& file, const std::string& tileset_definitions);

private:
	void writeHeader(std::ofstream& map_file);
	void writeTilesets(std::ofstream& map_file, const std::string& tileset_definitions);
	void writeLayers(std::ofstream& map_file);
	void writeEnemies(std::ofstream& map_file);
	void writeNPCs(std::ofstream& map_file);
	void writeEvents(std::ofstream& map_file);
	void writeEventComponents(std::ofstream& map_file, int eventID);

	Map* map;
	std::string dest_file;

	std::string EVENT_COMPONENT_NAME[EventComponent::EVENT_COMPONENT_COUNT];

};

#endif //MAP_SAVER_H
