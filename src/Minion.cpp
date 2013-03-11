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

#include "Minion.h"

#include "Settings.h"
#include "BehaviorStandard.h"
#include "EnemyBehavior.h"
#include "assert.h"

#include "MinionBehavior.h"
#include "UtilsMath.h"
#include "Hazard.h"
#include "Avatar.h"

#include <math.h>


Minion::Minion(PowerManager *_powers, MapRenderer *_map) : Entity(_map)
{
    powers = _powers;

	stats.cur_state = MINION_STANCE;
	stats.turn_ticks = MAX_FRAMES_PER_SEC;
	//stats.patrol_ticks = 0; //no longer needed due to A*
	stats.cooldown = 0;
	stats.last_seen.x = -1;
	stats.last_seen.y = -1;
	stats.in_combat = false;
	stats.join_combat = false;

	haz = NULL;

	sfx_phys = false;
	sfx_ment = false;
	sfx_hit = false;
	sfx_die = false;
	sfx_critdie = false;
	instant_power = false;

	eb = NULL;

}


Minion::Minion(const Minion& e)
 : Entity(e)
 , haz(NULL) // do not copy hazard. This constructor is used during mapload, so no hazard should be active.
 //, eb(new BehaviorStandard(this))
 , powers(e.powers)
 , sfx_phys(e.sfx_phys)
 , sfx_ment(e.sfx_ment)
 , sfx_hit(e.sfx_hit)
 , sfx_die(e.sfx_die)
 , sfx_critdie(e.sfx_critdie)
 , instant_power(e.instant_power)
{
    assert(e.haz == NULL);

}

Minion::~Minion()
{
    //dtor
}

/**
 * The current direction leads to a wall.  Try the next best direction, if one is available.
 */
int Minion::faceNextBest(int mapx, int mapy) {
	int dx = abs(mapx - stats.pos.x);
	int dy = abs(mapy - stats.pos.y);
	switch (stats.direction) {
		case 0:
			if (dy > dx) return 7;
			else return 1;
		case 1:
			if (mapy > stats.pos.y) return 0;
			else return 2;
		case 2:
			if (dx > dy) return 1;
			else return 3;
		case 3:
			if (mapx < stats.pos.x) return 2;
			else return 4;
		case 4:
			if (dy > dx) return 3;
			else return 5;
		case 5:
			if (mapy < stats.pos.y) return 4;
			else return 6;
		case 6:
			if (dx > dy) return 5;
			else return 7;
		case 7:
			if (mapx > stats.pos.x) return 6;
			else return 0;
	}
	return 0;
}

void Minion::newState(int state) {

	stats.cur_state = state;

        switch (state) {

		case MINION_STANCE:
		    setAnimation("stance");
		    break;

        case MINION_MOVE:
			setAnimation("run");
			break;

        case MINION_SPAWN:
			setAnimation("spawn");
			break;

        case MINION_BLOCK:
			setAnimation("block");
			break;

        case MINION_HIT:
			setAnimation("hit");
			break;

        case MINION_DEAD:
			setAnimation("die");
			break;

        case MINION_CRITDEAD:
			setAnimation("critdie");
			break;

        default:
			break;

        }
}

void Minion::logic()
{
    eb->logic();
    return;
}

int Minion::getDistance(Point dest) {
	int dx = dest.x - stats.pos.x;
	int dy = dest.y - stats.pos.y;
	double step1 = (double)dx * (double)dx + (double)dy * (double)dy;
	double step2 = sqrt(step1);
	return int(step2);
}

void Minion::InstantDeath() {
    stats.effects.triggered_death = true;
    newState(MINION_DEAD);

    sfx_die = true;
    stats.corpse_ticks = CORPSE_TIMEOUT;
    stats.effects.clearEffects();
}

void Minion::CheckMinionSustained(){
    //if minion was raised by a spawn power
    if(originatedFromSpawnPower){

        int maxSummons = minionManager->pc->stats.mental_character + minionManager->pc->stats.mental_additional;

        int qtySummons = 0;
        for (unsigned int i=0; i < minionManager->minions.size(); i++) {
            if(minionManager->minions[i]->originatedFromSpawnPower && !minionManager->minions[i]->stats.corpse){
                if(minionManager->minions[i]->power_index == power_index){
                    qtySummons++;
                }
            }
        }

        //if total minions sumoned by this skill does not exceed the player mental ability
        if(qtySummons > maxSummons)
        {
            InstantDeath();
        }

    }
}

/**
 * getRender()
 * Map objects need to be drawn in Z order, so we allow a parent object (GameEngine)
 * to collect all mobile sprites each frame.
 */
Renderable Minion::getRender() {
	Renderable r = activeAnimation->getCurrentFrame(stats.direction);
	r.map_pos.x = stats.pos.x;
	r.map_pos.y = stats.pos.y;
	return r;
}

/**
 * Whenever a hazard collides with a minion, this function resolves the effect
 * Called by HazardManager
 *
 * Returns false on miss
 */
bool Minion::takeHit(const Hazard &h) {
	if (stats.cur_state != MINION_DEAD && stats.cur_state != MINION_CRITDEAD) {

		// prepare the combat text
		CombatText *combat_text = comb;

		// if it's a miss, do nothing
		int avoidance = stats.avoidance;
		clampCeil(avoidance, MAX_AVOIDANCE);
		if (percentChance(avoidance - h.accuracy - 25)) {
			combat_text->addMessage(msg->get("miss"), stats.pos, COMBAT_MESSAGE_MISS);
			return false;
		}

		// calculate base damage
		int dmg = randBetween(h.dmg_min, h.dmg_max);

		// apply elemental resistance
		if (h.trait_elemental >= 0 && unsigned(h.trait_elemental) < stats.vulnerable.size()) {
			unsigned i = h.trait_elemental;
			int vulnerable = stats.vulnerable[i];
			if (stats.vulnerable[i] > MAX_RESIST && stats.vulnerable[i] < 100)
				vulnerable = MAX_RESIST;
			dmg = (dmg * vulnerable) / 100;
		}

		if (!h.trait_armor_penetration) { // armor penetration ignores all absorption
			// substract absorption from armor
			int absorption = randBetween(stats.absorb_min, stats.absorb_max);
			if (absorption > 0) {
				if ((dmg*100)/absorption > MAX_ABSORB)
					absorption = (absorption * MAX_ABSORB) / 100;
				if (absorption == 0) absorption = 1;
			}
			dmg = dmg - absorption;
			if (dmg <= 0) {
				dmg = 0;
				if (h.trait_elemental < 0) {
					if (MAX_ABSORB < 100) dmg = 1;
				} else {
					if (MAX_RESIST < 100) dmg = 1;
				}
			}
		}

		// check for crits
		int true_crit_chance = h.crit_chance;
		if (stats.effects.stun || stats.effects.speed < 100)
			true_crit_chance += h.trait_crits_impaired;

		bool crit = percentChance(true_crit_chance);
		if (crit) {
			dmg = dmg + h.dmg_max;

			// show crit damage
			combat_text->addMessage(dmg, stats.pos, COMBAT_MESSAGE_CRIT);
		}
		else {
			// show normal damage
			combat_text->addMessage(dmg, stats.pos, COMBAT_MESSAGE_GIVEDMG);
		}

		// apply damage
		stats.takeDamage(dmg);

		// damage always breaks stun
		if (dmg > 0) stats.effects.removeEffectType("stun");

		// after effects
		if (stats.hp > 0) {

			if (h.mod_power > 0) powers->effect(&stats, h.mod_power);
			powers->effect(&stats, h.power_index);

			if (stats.effects.forced_move) {
				float theta = powers->calcTheta(h.src_stats->pos.x, h.src_stats->pos.y, stats.pos.x, stats.pos.y);
				stats.forced_speed.x = static_cast<int>(ceil(stats.effects.forced_speed * cos(theta)));
				stats.forced_speed.y = static_cast<int>(ceil(stats.effects.forced_speed * sin(theta)));
			}

		}

		if (h.hp_steal != 0) {
			int heal_amt = (dmg * h.hp_steal) / 100;
			if (heal_amt == 0 && dmg > 0) heal_amt = 1;
			combat_text->addMessage(msg->get("+%d HP",heal_amt), h.src_stats->pos, COMBAT_MESSAGE_BUFF);
			h.src_stats->hp += heal_amt;
			clampCeil(h.src_stats->hp, h.src_stats->maxhp);
		}
		if (h.mp_steal != 0) {
			int heal_amt = (dmg * h.mp_steal) / 100;
			if (heal_amt == 0 && dmg > 0) heal_amt = 1;
			combat_text->addMessage(msg->get("+%d MP",heal_amt), h.src_stats->pos, COMBAT_MESSAGE_BUFF);
			h.src_stats->mp += heal_amt;
			clampCeil(h.src_stats->mp, h.src_stats->maxmp);
		}

		// post effect power
		if (h.post_power > 0 && dmg > 0) {
			powers->activate(h.post_power, h.src_stats, stats.pos);
		}

		// interrupted to new state
		if (dmg > 0) {

			if (stats.hp <= 0 && crit) {
				stats.effects.triggered_death = true;
				newState(MINION_CRITDEAD);

                sfx_die = true;
				stats.corpse_ticks = CORPSE_TIMEOUT;
				stats.effects.clearEffects();
			}
			else if (stats.hp <= 0) {
				stats.effects.triggered_death = true;
				newState(MINION_DEAD);

                sfx_die = true;
				stats.corpse_ticks = CORPSE_TIMEOUT;
				stats.effects.clearEffects();
			}
			// don't go through a hit animation if stunned
			else if (!stats.effects.stun && !percentChance(stats.poise)) {
				newState(MINION_HIT);
				sfx_hit = true;
				// roll to see if the enemy's ON_HIT power is casted
				if (percentChance(stats.power_chance[ON_HIT])) {
					powers->activate(stats.power_index[ON_HIT], &stats, stats.pos);
				}
			}
			// just play the hit sound
			else {
				sfx_hit = true;
			}

		}

		return true;
	}
	return false;
}

