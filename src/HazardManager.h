/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2015 Justin Jacobs

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
 * class HazardManager
 *
 * Holds the collection of hazards (active attacks, spells, etc) and handles group operations
 */

#ifndef HAZARD_MANAGER_H
#define HAZARD_MANAGER_H

#include "CommonIncludes.h"
#include "Utils.h"

class Avatar;
class Enemy;
class Hazard;

class HazardManager {
private:
	void hitEntity(size_t index, const bool hit);

public:
	HazardManager();
	~HazardManager();
	void logic();
	void checkNewHazards();
	void handleNewMap();
	void addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	std::vector<Hazard*> h;
	Enemy* last_enemy;
};

#endif
