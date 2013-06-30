/*
Copyright © 2013 Stefan Beller

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

#ifndef SHAREDGAMEOBJECTS_H
#define SHAREDGAMEOBJECTS_H

#include "LootManager.h"
#include "MenuPowers.h"
#include "EnemyGroupManager.h"
#include "PowerManager.h"
#include "CampaignManager.h"
#include "Avatar.h"

extern MenuPowers *menu_powers;

/* These objects are created in the GameStatePlay constructor and deleted in the GameStatePlay destructor
*  so can be accessed safely anywhere in between. The objects must not be changed by any other class.
*/
extern LootManager *loot;
extern EnemyGroupManager *enemyg;
extern PowerManager *powers;
extern MapRenderer *mapr;
extern EnemyManager *enemies;
extern CampaignManager *camp;
extern ItemManager *items;
extern Avatar *pc;

#endif // SHAREDGAMEOBJECTS_H
