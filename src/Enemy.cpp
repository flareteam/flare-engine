/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller

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
 * class Enemy
 */

#include "Animation.h"
#include "BehaviorStandard.h"
#include "CampaignManager.h"
#include "EnemyBehavior.h"
#include "Enemy.h"
#include "Hazard.h"
#include "LootManager.h"
#include "MapRenderer.h"
#include "PowerManager.h"
#include "SharedResources.h"
#include "UtilsMath.h"
#include "Avatar.h"

#include <sstream>

using namespace std;


Enemy::Enemy(PowerManager *_powers, MapRenderer *_map, EnemyManager *_em) : Entity(_map) {
	powers = _powers;
	enemies = _em;

	stats.cur_state = ENEMY_STANCE;
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
	reward_xp = false;
	instant_power = false;
	summoned = false;
	kill_source_type = SOURCE_TYPE_NEUTRAL;

	eb = NULL;
}

Enemy::Enemy(const Enemy& e)
 : Entity(e)
 , type(e.type)
 , haz(NULL) // do not copy hazard. This constructor is used during mapload, so no hazard should be active.
 , eb(new BehaviorStandard(this,e.enemies))
 , powers(e.powers)
 , enemies(e.enemies)
 , sfx_phys(e.sfx_phys)
 , sfx_ment(e.sfx_ment)
 , sfx_hit(e.sfx_hit)
 , sfx_die(e.sfx_die)
 , sfx_critdie(e.sfx_critdie)
 , reward_xp(e.reward_xp)
 , instant_power(e.instant_power)
 , kill_source_type(e.kill_source_type)
{
	assert(e.haz == NULL);
}

/**
 * The current direction leads to a wall.  Try the next best direction, if one is available.
 */
int Enemy::faceNextBest(int mapx, int mapy) {
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

/**
 * Calculate distance between the enemy and the hero
 */
int Enemy::getDistance(Point dest) {
	int dx = dest.x - stats.pos.x;
	int dy = dest.y - stats.pos.y;
	double step1 = (double)dx * (double)dx + (double)dy * (double)dy;
	double step2 = sqrt(step1);
	return int(step2);
}

void Enemy::newState(int state) {

	stats.cur_state = state;
}

/**
 * logic()
 * Handle a single frame.  This includes:
 * - move the enemy based on AI % chances
 * - calculate the next frame of animation
 */
void Enemy::logic() {

	eb->logic();
	return;
}

/**
 * Whenever a hazard collides with an enemy, this function resolves the effect
 * Called by HazardManager
 *
 * Returns false on miss
 */
bool Enemy::takeHit(const Hazard &h) {
	if (stats.cur_state != ENEMY_DEAD && stats.cur_state != ENEMY_CRITDEAD)
	{
		if (!stats.in_combat) {
			stats.join_combat = true;
			stats.in_combat = true;
			stats.last_seen.x = stats.hero_pos.x;
			stats.last_seen.y = stats.hero_pos.y;
			powers->activate(stats.power_index[BEACON], &stats, stats.pos); //emit beacon
		}

		// exit if it was a beacon (to prevent stats.targeted from being set)
		if (powers->powers[h.power_index].beacon) return false;

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
			map->shaky_cam_ticks = MAX_FRAMES_PER_SEC/2;

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

			if (h.mod_power > 0) powers->effect(&stats, h.mod_power,h.source_type);
			powers->effect(&stats, h.power_index,h.source_type);

			if (stats.effects.forced_move) {
				float theta = powers->calcTheta(stats.hero_pos.x, stats.hero_pos.y, stats.pos.x, stats.pos.y);
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
				doRewards(h.source_type);
				stats.effects.triggered_death = true;
				stats.cur_state = ENEMY_CRITDEAD;
				map->collider.unblock(stats.pos.x,stats.pos.y);

			}
			else if (stats.hp <= 0) {
				doRewards(h.source_type);
				stats.effects.triggered_death = true;
				stats.cur_state = ENEMY_DEAD;
				map->collider.unblock(stats.pos.x,stats.pos.y);

			}
			// don't go through a hit animation if stunned
			else if (!stats.effects.stun && !percentChance(stats.poise)) {
				sfx_hit = true;

				if(stats.cooldown_hit_ticks == 0) {
					stats.cur_state = ENEMY_HIT;
					stats.cooldown_hit_ticks = stats.cooldown_hit;
				}
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

/**
 * Upon enemy death, handle rewards (currency, xp, loot)
 */
void Enemy::doRewards(int source_type) {

    if(stats.hero_ally)
        return;

	reward_xp = true;
	kill_source_type = source_type;

	// some creatures create special loot if we're on a quest
	if (stats.quest_loot_requires != "") {

		// the loot manager will check quest_loot_id
		// if set (not zero), the loot manager will 100% generate that loot.
		if (!(map->camp->checkStatus(stats.quest_loot_requires) && !map->camp->checkStatus(stats.quest_loot_not))) {
			stats.quest_loot_id = 0;
		}
	}

	// some creatures drop special loot the first time they are defeated
	// this must be done in conjunction with defeat status
	if (stats.first_defeat_loot > 0) {
		if (!map->camp->checkStatus(stats.defeat_status)) {
			stats.quest_loot_id = stats.first_defeat_loot;
		}
	}

	// defeating some creatures (e.g. bosses) affects the story
	if (stats.defeat_status != "") {
		map->camp->setStatus(stats.defeat_status);
	}

	LootManager::getInstance()->addEnemyLoot(this);
}

void Enemy::InstantDeath() {
    stats.effects.triggered_death = true;
    stats.cur_state = ENEMY_DEAD;

    sfx_die = true;
    stats.corpse_ticks = CORPSE_TIMEOUT;
    stats.effects.clearEffects();
}

void Enemy::CheckSummonSustained(){
    //if minion was raised by a spawn power
    if(summoned && stats.hero_ally){

        Power *spawn_power = &powers->powers[summoned_power_index];

        int max_summons = 0;

        if(spawn_power->spawn_limit_mode == SPAWN_LIMIT_MODE_FIXED)
            max_summons = spawn_power->spawn_limit_qty;
        else if(spawn_power->spawn_limit_mode == SPAWN_LIMIT_MODE_STAT)
        {
            int stat_val = 1;
            switch(spawn_power->spawn_limit_stat){
            case SPAWN_LIMIT_STAT_PHYSICAL:
                stat_val = enemies->pc->stats.physical_character + enemies->pc->stats.physical_additional;
                break;
            case SPAWN_LIMIT_STAT_MENTAL:
                stat_val = enemies->pc->stats.mental_character + enemies->pc->stats.mental_additional;
                break;
            case SPAWN_LIMIT_STAT_OFFENSE:
                stat_val = enemies->pc->stats.offense_character + enemies->pc->stats.offense_additional;
                break;
            case SPAWN_LIMIT_STAT_DEFENSE:
                stat_val = enemies->pc->stats.defense_character + enemies->pc->stats.defense_additional;
                break;
            }
            max_summons = (stat_val / (spawn_power->spawn_limit_every == 0 ? 1 : spawn_power->spawn_limit_every)) * spawn_power->spawn_limit_qty;
        }
        else
            return;//unlimited or unknown mode

        //if the power is available, there should be at least 1 allowed summon
        if(max_summons < 1) max_summons = 1;

        int qty_summons = 0;
        for (unsigned int i=0; i < enemies->enemies.size(); i++) {
            if(enemies->enemies[i]->stats.hero_ally && enemies->enemies[i]->summoned
               && !enemies->enemies[i]->stats.corpse
               && enemies->enemies[i]->summoned_power_index == summoned_power_index){
                    qty_summons++;
            }
        }

        //if total minions sumoned by this skill does not exceed the player mental ability
        if(qty_summons > max_summons)
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
Renderable Enemy::getRender() {
	Renderable r = activeAnimation->getCurrentFrame(stats.direction);
	r.map_pos.x = stats.pos.x;
	r.map_pos.y = stats.pos.y;
	return r;
}

Enemy::~Enemy() {
	delete haz;
	delete eb;
}

