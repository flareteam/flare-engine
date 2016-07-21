/*
Copyright © 2011-2012 Thane Brimhall
		Manuel A. Fernandez Montecelo <manuel.montezelo@gmail.com>

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

#ifndef ENEMYGROUPMANAGER_H
#define ENEMYGROUPMANAGER_H

#include "CommonIncludes.h"

class Enemy_Level {
public:
	std::string type;
	int level;
	std::string rarity;

	Enemy_Level() : level(0), rarity("common") {}

};

/**
 * class EnemyGroupManager
 *
 * Loads Enemies into category lists and manages spawning randomized groups of
 * enemies.
 */
class EnemyGroupManager {
public:
	EnemyGroupManager();
	~EnemyGroupManager();

	/** To get a random enemy with the given characteristics
	 *
	 * @param category Enemy of the desired category
	 * @param minlevel Enemy of the desired level (minimum)
	 * @param maxlevel Enemy of the desired level (maximum)
	 *
	 * @return A random enemy level description if a suitable was found.
	 *         Null if none was found.
	 */
	Enemy_Level getRandomEnemy(const std::string& category, int minlevel, int maxlevel) const;

	/** To get enemies that fit in a category
	 *
	 * @param category Enemies of the desired category
	 *
	 * @return Level descriptions of enemies in the category.
	 *         Empty buffer if none was found.
	 */
	std::vector<Enemy_Level> getEnemiesInCategory(const std::string& category) const;

private:

	/** Container to store enemy data */
	std::map <std::string, std::vector<Enemy_Level> > _categories;
};

#endif
