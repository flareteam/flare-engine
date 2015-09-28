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

#include "Avatar.h"
#include "CampaignManager.h"
#include "EnemyGroupManager.h"
#include "LootManager.h"
#include "MenuActionBar.h"
#include "MenuPowers.h"
#include "PowerManager.h"

extern MenuActionBar *menu_act;
extern MenuPowers *menu_powers;

/* These objects are created in the GameStatePlay constructor and deleted in the GameStatePlay destructor
*  so can be accessed safely anywhere in between. The objects must not be changed by any other class.
*/
extern Avatar *pc;
extern MenuManager *menu;
extern CampaignManager *camp;
extern EnemyGroupManager *enemyg;
extern EnemyManager *enemies;
extern ItemManager *items;
extern LootManager *loot;
extern MapRenderer *mapr;
extern PowerManager *powers;

extern int CurrentGameSlot;

#endif // SHAREDGAMEOBJECTS_H
