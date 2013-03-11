/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Ryan Dansie

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

#ifndef MINION_H
#define MINION_H

#include "Entity.h"
#include "MinionManager.h"

#include <string>

class MinionBehavior;

enum MinionState{
    // active states
    MINION_STANCE = 0,
    MINION_MOVE = 1,
    MINION_CHARGE = 2,
    MINION_MELEE_PHYS = 3,
    MINION_MELEE_MENT = 4,
    MINION_RANGED_PHYS = 5,
    MINION_RANGED_MENT = 6,
    MINION_SPAWN = 7,
    // interrupt states
    MINION_BLOCK = 9,
    MINION_HIT = 10,
    MINION_DEAD = 11,
    MINION_CRITDEAD = 12,
    MINION_HALF_DEAD = 13,
    MINION_JOIN_COMBAT = 14,

    // final shared states
    MINION_POWER = 15 // minion performing a power. anim/sfx based on power
};

class Minion : public Entity
{
    public:
        Minion(PowerManager *_powers, MapRenderer *_map);
        Minion(const Minion& e);
        ~Minion();
        void logic();
        virtual Renderable getRender();
        int faceNextBest(int mapx, int mapy);
        void newState(int state);
        int getDistance(Point dest);
        bool takeHit(const Hazard &h);
        void InstantDeath();
        void CheckMinionSustained();

        Hazard *haz;
        PowerManager *powers;
        MinionBehavior *eb;
        std::string type;
        int power_index;
        MinionManager *minionManager;

        Point pursue_pos;

        // sound effects flags
	bool sfx_phys;
	bool sfx_ment;

	bool sfx_hit;
	bool sfx_die;
	bool sfx_critdie;

	// other flags
	bool instant_power;
	bool originatedFromSpawnPower;

};

#endif // MINION_H
