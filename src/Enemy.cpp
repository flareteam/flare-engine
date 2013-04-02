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
#include "BehaviorAlly.h"
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


Enemy::Enemy(PowerManager *_powers, MapRenderer *_map, EnemyManager *_em) : Entity(_powers, _map) {
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

	reward_xp = false;
	instant_power = false;
	summoned = false;
	summoned_power_index = 0;
	kill_source_type = SOURCE_TYPE_NEUTRAL;
	eb = NULL;
}

Enemy::Enemy(const Enemy& e)
 : Entity(e)
 , type(e.type)
 , haz(NULL) // do not copy hazard. This constructor is used during mapload, so no hazard should be active.
 , eb(new BehaviorStandard(this,e.enemies))
 , enemies(e.enemies)
 , reward_xp(e.reward_xp)
 , instant_power(e.instant_power)
 , summoned(e.summoned)
 , summoned_power_index(e.summoned_power_index)
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

	//need to check whether the enemy was converted here
	//cant do it in behaviour because the behaviour object would be replaced by this
	if(stats.effects.convert != stats.converted){
        delete eb;
        eb = stats.hero_ally ? new BehaviorStandard(this, enemies) : new BehaviorAlly(this, enemies);
        stats.converted = !stats.converted;
        stats.hero_ally = !stats.hero_ally;
    }

	return;
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

    stats.hp = 0;
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
                stat_val = enemies->pc->stats.get_physical();
                break;
            case SPAWN_LIMIT_STAT_MENTAL:
                stat_val = enemies->pc->stats.get_mental();
                break;
            case SPAWN_LIMIT_STAT_OFFENSE:
                stat_val = enemies->pc->stats.get_offense();
                break;
            case SPAWN_LIMIT_STAT_DEFENSE:
                stat_val = enemies->pc->stats.get_defense();
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
               && enemies->enemies[i]->summoned_power_index == summoned_power_index
               && enemies->enemies[i]->stats.cur_state != ENEMY_SPAWN
               && enemies->enemies[i]->stats.cur_state != ENEMY_DEAD
               && enemies->enemies[i]->stats.cur_state != ENEMY_CRITDEAD){
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

