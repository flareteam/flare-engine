/*
Copyright © 2013 Stefan Beller
Copyright © 2015 Justin Jacobs

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

#include "Avatar.h"
#include "CampaignManager.h"
#include "EnemyGroupManager.h"
#include "HazardManager.h"
#include "LootManager.h"
#include "MenuActionBar.h"
#include "MenuPowers.h"
#include "PowerManager.h"
#include "SharedGameResources.h"

Avatar *pc = NULL;
MenuManager *menu = NULL;
CampaignManager *camp = NULL;
EnemyGroupManager *enemyg = NULL;
EnemyManager *enemym = NULL;
HazardManager *hazards = NULL;
ItemManager *items = NULL;
LootManager *loot = NULL;
MapRenderer *mapr = NULL;
MenuActionBar *menu_act= NULL;
MenuPowers *menu_powers = NULL;
PowerManager *powers = NULL;
