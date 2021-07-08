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

#ifndef SHAREDGAMEOBJECTS_H
#define SHAREDGAMEOBJECTS_H

class Avatar;
class CampaignManager;
class EnemyGroupManager;
class EntityManager;
class HazardManager;
class ItemManager;
class LootManager;
class MapRenderer;
class MenuActionBar;
class MenuManager;
class MenuPowers;
class NPCManager;
class PowerManager;
class FogOfWar;

/* These objects are created in the GameStatePlay constructor and deleted in the GameStatePlay destructor
*  so can be accessed safely anywhere in between. The objects must not be changed by any other class.
*/
extern Avatar *pc;
extern CampaignManager *camp;
extern EnemyGroupManager *enemyg;
extern EntityManager *entitym;
extern HazardManager *hazards;
extern ItemManager *items;
extern LootManager *loot;
extern MapRenderer *mapr;
extern MenuActionBar *menu_act;
extern MenuManager *menu;
extern MenuPowers *menu_powers;
extern NPCManager *npcs;
extern PowerManager *powers;
extern FogOfWar *fow;

#endif // SHAREDGAMEOBJECTS_H
