/*
Copyright © 2012 Clint Bellanger
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


#include "MinionBehavior.h"

#include "Minion.h"
#include "MapRenderer.h"
#include "UtilsMath.h"
#include "PowerManager.h"
#include "EnemyManager.h"

const unsigned short MINIMUM_FOLLOW_DISTANCE = 200;
const unsigned short MAXIMUM_FOLLOW_DISTANCE = 2000;

MinionBehavior::MinionBehavior(Minion *_e)
{
    speedArtificiallyIncreased = false;
    minion = _e;
    //ctor
}

MinionBehavior::~MinionBehavior()
{
    //dtor
}

void MinionBehavior::logic()
{
    // skip all logic if the minion is dead and no longer animating
	if (minion->stats.corpse) {
		if (minion->stats.corpse_ticks > 0)
			minion->stats.corpse_ticks--;
		return;
	}

    doUpkeep();
	findTarget();
	checkPower();
	checkMove();
    updateState();
}

/**
 * Various upkeep on stats
 * TODO: some of these actions could be moved to StatBlock::logic()
 */
void MinionBehavior::doUpkeep() {

	minion->stats.logic();

	if (minion->stats.effects.forced_move) {
		minion->move();
	}

	if (minion->stats.wander_ticks > 0)
		minion->stats.wander_ticks--;

	if (minion->stats.wander_pause_ticks > 0)
		minion->stats.wander_pause_ticks--;

	// check for revive
	if (minion->stats.hp <= 0 && minion->stats.effects.revive) {
		minion->stats.hp = minion->stats.maxhp;
		minion->stats.alive = true;
		minion->stats.corpse = false;
		minion->newState(MINION_STANCE);
	}

	// check for bleeding to death
	if (minion->stats.hp <= 0 && !(minion->stats.cur_state == MINION_DEAD || minion->stats.cur_state == MINION_CRITDEAD)) {

		minion->stats.effects.triggered_death = true;
		minion->newState(MINION_DEAD);
	}

	// check for teleport powers
	if (minion->stats.teleportation) {

		minion->stats.pos.x = minion->stats.teleport_destination.x;
		minion->stats.pos.y = minion->stats.teleport_destination.y;

		minion->stats.teleportation = false;
	}
}

/**
 * Locate the player and set various targeting info
 */
void MinionBehavior::findTarget() {

	// stunned minions can't act
	if (minion->stats.effects.stun) return;

	// check distance and line of sight between minion and hero
	if (minion->stats.hero_alive)
        dist = minion->getDistance(minion->stats.hero_pos);
	else
		dist = 0;

    //if the minion gets too far, transport it to the player pos
    if(dist > MAXIMUM_FOLLOW_DISTANCE && !minion->stats.in_combat)
    {
        minion->stats.pos.x = minion->stats.hero_pos.x;
        minion->stats.pos.y = minion->stats.hero_pos.y;
        dist = 0;
    }

    bool enemiesInCombat = false;
    //enter combat because enemy is targeting the player or a summon
    std::vector<Enemy*>* enemies = &(minion->minionManager->enemyManager->enemies);
    for (unsigned int i=0; i < (*enemies).size(); i++) {
        if(((*enemies)[i])->stats.in_combat)
        {
            Enemy* e = (*enemies)[i];

            //now work out the distance to the enemy and compare it to the distance to the current targer (we want to target the closest enemy)
            if(enemiesInCombat){
                int enemyDist = minion->getDistance(e->stats.pos);
                int currentTargetDist = minion->getDistance(minion->pursue_pos);
                if(enemyDist < currentTargetDist){
                    minion->pursue_pos.x = e->stats.pos.x;
                    minion->pursue_pos.y = e->stats.pos.y;
                }
            }
            else
            {//minion is not already chasig another enemy so chase this one
                minion->pursue_pos.x = e->stats.pos.x;
                minion->pursue_pos.y = e->stats.pos.y;
            }

            minion->stats.in_combat = true;
            enemiesInCombat = true;
        }
    }

    //break combat if the player gets too far or all enemies die
    if(!enemiesInCombat)
        minion->stats.in_combat = false;

    //the default target is the player
    if(!minion->stats.in_combat)
    {
        minion->pursue_pos.x = minion->stats.hero_pos.x;
        minion->pursue_pos.y = minion->stats.hero_pos.y;
    }
}


/**
 * Begin using a power if idle, based on behavior % chances.
 * Activate a ready power, if the attack animation has followed through
 */
void MinionBehavior::checkPower() {

    // stunned minions can't act
	if (minion->stats.effects.stun) return;

	// currently all minion power use happens during combat
	if (!minion->stats.in_combat) return;

	// if the minion is on global cooldown it cannot act
	if (minion->stats.cooldown_ticks > 0) return;

	// Note there are two stages to activating a power.
	// First is the minion choosing to use a power based on behavioral chance
	// Second is the power actually firing off once the related animation reaches the active frame.
	// (these are separate so that interruptions can take place)

    int currentTargetDist = minion->getDistance(minion->pursue_pos);

    // check line-of-sight
	if (currentTargetDist < minion->stats.threat_range && minion->stats.hero_alive)
		los = minion->map->collider.line_of_sight(minion->stats.pos.x, minion->stats.pos.y, minion->pursue_pos.x, minion->pursue_pos.y);
	else
		los = false;

	// Begin Power Animation:
	// standard minions can begin a power-use animation if they're standing around or moving voluntarily.
	if (los && (minion->stats.cur_state == MINION_STANCE || minion->stats.cur_state == MINION_MOVE)) {

		// check half dead power use
		if (!minion->stats.on_half_dead_casted && minion->stats.hp <= minion->stats.maxhp/2) {
			if (percentChance(minion->stats.power_chance[ON_HALF_DEAD])) {
				minion->newState(MINION_POWER);
				minion->stats.activated_powerslot = ON_HALF_DEAD;
				return;
			}
		}

		// check ranged power use
		if (currentTargetDist > minion->stats.melee_range) {

			if (percentChance(minion->stats.power_chance[RANGED_PHYS]) && minion->stats.power_ticks[RANGED_PHYS] == 0) {
				minion->newState(MINION_POWER);
				minion->stats.activated_powerslot = RANGED_PHYS;
				return;
			}
			if (percentChance(minion->stats.power_chance[RANGED_MENT]) && minion->stats.power_ticks[RANGED_MENT] == 0) {
				minion->newState(MINION_POWER);
				minion->stats.activated_powerslot = RANGED_MENT;
				return;
			}

		}
		else { // check melee power use

			if (percentChance(minion->stats.power_chance[MELEE_PHYS]) && minion->stats.power_ticks[MELEE_PHYS] == 0) {
				minion->newState(MINION_POWER);
				minion->stats.activated_powerslot = MELEE_PHYS;
				return;
			}
			if (percentChance(minion->stats.power_chance[MELEE_MENT]) && minion->stats.power_ticks[MELEE_MENT] == 0) {
				minion->newState(MINION_POWER);
				minion->stats.activated_powerslot = MELEE_MENT;
				return;
			}
		}
	}

	// Activate Power:
	// minion has started the animation to use a power. Activate the power on the Active animation frame
	if (minion->stats.cur_state == MINION_POWER) {

		// if we're at the active frame of a power animation,
		// activate the power and set the local and global cooldowns
		if (minion->activeAnimation->isActiveFrame() || minion->instant_power) {
			minion->instant_power = false;

			int power_slot =  minion->stats.activated_powerslot;
			int power_id = minion->stats.power_index[minion->stats.activated_powerslot];

			minion->powers->activate(power_id, &minion->stats, minion->pursue_pos);

			minion->stats.power_ticks[power_slot] = minion->stats.power_cooldown[power_slot];
			minion->stats.cooldown_ticks = minion->stats.cooldown;

			if (minion->stats.activated_powerslot == ON_HALF_DEAD) {
				minion->stats.on_half_dead_casted = true;
			}
		}
	}

}

/**
 * Check state changes related to movement
 */
void MinionBehavior::checkMove() {

	// dying minions can't move
	if (minion->stats.cur_state == MINION_DEAD || minion->stats.cur_state == MINION_CRITDEAD) return;

	// stunned minions can't act
	if (minion->stats.effects.stun) return;


	// update direction
	if (minion->stats.facing) {
		if (++minion->stats.turn_ticks > minion->stats.turn_delay) {

			// if blocked, face in pathfinder direction instead
			if (!minion->map->collider.line_of_movement(minion->stats.pos.x, minion->stats.pos.y, minion->pursue_pos.x, minion->pursue_pos.y, minion->stats.movement_type)) {

				// if a path is returned, target first waypoint
				std::vector<Point> path;

				if ( minion->map->collider.compute_path(minion->stats.pos, minion->pursue_pos, path, minion->stats.movement_type) ) {
					minion->pursue_pos = path.back();
				}
			}

			minion->stats.direction = minion->face(minion->pursue_pos.x, minion->pursue_pos.y);
			minion->stats.turn_ticks = 0;
		}
	}
	int prev_direction = minion->stats.direction;


	// try to start moving
	if (minion->stats.cur_state == MINION_STANCE) {

        int currentTargetDist = minion->getDistance(minion->pursue_pos);

        if(minion->stats.in_combat && currentTargetDist > minion->stats.melee_range)
            minion->newState(MINION_MOVE);

        if(!minion->stats.in_combat && dist > MINIMUM_FOLLOW_DISTANCE)
        {
            if (minion->move()) {
                minion->newState(MINION_MOVE);
            }
            else {
                // hit an obstacle, try the next best angle
                minion->stats.direction = minion->faceNextBest(minion->pursue_pos.x, minion->pursue_pos.y);
                if (minion->move()) {
                    minion->newState(MINION_MOVE);
                }
                else minion->stats.direction = prev_direction;
            }
        }
	}

	// already moving
	else if (minion->stats.cur_state == MINION_MOVE) {

    //if close enough to hero, stop miving
        if(dist < MINIMUM_FOLLOW_DISTANCE && !minion->stats.in_combat)
        {
            minion->newState(MINION_STANCE);
        }

		// try to continue moving
		else if (!minion->move()) {

			// hit an obstacle.  Try the next best angle
			minion->stats.direction = minion->faceNextBest(minion->pursue_pos.x, minion->pursue_pos.y);
			if (!minion->move()) {
				//minion->newState(MINION_STANCE);
				minion->stats.direction = prev_direction;
			}
		}
	}


}


/**
 * Perform miscellaneous state-based actions.
 * 1) Set animations and sound effects
 * 2) Return to the default state (Stance) when actions are complete
 */
void MinionBehavior::updateState() {

	// stunned enemies can't act
	if (minion->stats.effects.stun) return;

	int power_id;
	int power_state;

	// continue current animations
	minion->activeAnimation->advanceFrame();

	switch (minion->stats.cur_state) {

		case MINION_POWER:

			power_id = minion->stats.power_index[minion->stats.activated_powerslot];
			power_state = minion->powers->powers[power_id].new_state;

			// animation based on power type
			if (power_state == POWSTATE_SWING) minion->setAnimation("melee");
			else if (power_state == POWSTATE_SHOOT) minion->setAnimation("ranged");
			else if (power_state == POWSTATE_CAST) minion->setAnimation("ment");
			else if (power_state == POWSTATE_INSTANT) minion->instant_power = true;

			// sound effect based on power type
			if (minion->activeAnimation->isFirstFrame()) {
				if (power_state == POWSTATE_SWING) minion->sfx_phys = true;
				else if (power_state == POWSTATE_SHOOT) minion->sfx_phys = true;
				else if (power_state == POWSTATE_CAST) minion->sfx_ment = true;
			}

			if (minion->activeAnimation->isLastFrame()) minion->newState(MINION_STANCE);
			break;

		case MINION_SPAWN:

			if (minion->activeAnimation->isLastFrame()) {
                //now check whether the summon can be sustained by the player
                minion->newState(MINION_STANCE);
                minion->CheckMinionSustained();
            }
			break;

		case MINION_HIT:

			if (minion->activeAnimation->isFirstFrame()) {
				minion->stats.effects.triggered_hit = true;
			}
			if (minion->activeAnimation->isLastFrame()) minion->newState(MINION_STANCE);
			break;

		case MINION_DEAD:
			if (minion->stats.effects.triggered_death) break;

			if (minion->activeAnimation->isSecondLastFrame()) {
				if (percentChance(minion->stats.power_chance[ON_DEATH]))
					minion->powers->activate(minion->stats.power_index[ON_DEATH], &minion->stats, minion->stats.pos);
			}
			if (minion->activeAnimation->isLastFrame()) minion->stats.corpse = true; // puts renderable under object layer

			break;

		case MINION_CRITDEAD:

			if (minion->activeAnimation->isFirstFrame()) {
				minion->sfx_critdie = true;
				minion->stats.corpse_ticks = CORPSE_TIMEOUT;
				minion->stats.effects.clearEffects();
			}
			if (minion->activeAnimation->isSecondLastFrame()) {
				if (percentChance(minion->stats.power_chance[ON_DEATH]))
					minion->powers->activate(minion->stats.power_index[ON_DEATH], &minion->stats, minion->stats.pos);
			}
			if (minion->activeAnimation->isLastFrame()) minion->stats.corpse = true; // puts renderable under object layer

			break;

		default:
			break;
	}

	// activate all passive powers
	if (minion->stats.hp > 0 || minion->stats.effects.triggered_death) minion->powers->activatePassives(&minion->stats);

}








