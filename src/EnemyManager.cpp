/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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

#include "EnemyManager.h"

#include "AnimationManager.h"
#include "AnimationSet.h"
#include "Animation.h"
#include "SharedResources.h"
#include "EnemyBehavior.h"
#include "BehaviorStandard.h"
#include "BehaviorAlly.h"
#include "SharedGameResources.h"

#include <limits>

EnemyManager::EnemyManager()
	: enemies()
	, hero_stealth(0)
	, player_blocked(false)
	, player_blocked_ticks(0) {
	handleNewMap();
}

void EnemyManager::loadAnimations(Enemy *e) {
	anim->increaseCount(e->stats.animations);
	e->animationSet = anim->getAnimationSet(e->stats.animations);
	e->activeAnimation = e->animationSet->getAnimation();
}

Enemy *EnemyManager::getEnemyPrototype(const std::string& type_id) {
	for (size_t i = 0; i < prototypes.size(); i++) {
		if (prototypes[i].type == type_id) {
			anim->increaseCount(prototypes[i].stats.animations);
			return new Enemy(prototypes[i]);
		}
	}

	Enemy e = Enemy();

	e.eb = new BehaviorStandard(&e);
	e.stats.load(type_id);
	e.type = type_id;

	if (e.stats.animations == "")
		logError("EnemyManager: No animation file specified for entity: %s", type_id.c_str());

	loadAnimations(&e);
	e.loadSounds();

	prototypes.push_back(e);

	return new Enemy(prototypes.back());
}

/**
 * When loading a new map, we eliminate existing enemies and load the new ones.
 * The map will have loaded Entity blocks into an array; retrieve the Enemies and init them
 */
void EnemyManager::handleNewMap () {

	Map_Enemy me;
	std::queue<Enemy *> allies;

	// delete existing enemies
	for (unsigned int i=0; i < enemies.size(); i++) {
		anim->decreaseCount(enemies[i]->animationSet->getName());
		if(enemies[i]->stats.hero_ally && !enemies[i]->stats.corpse && enemies[i]->stats.cur_state != ENEMY_DEAD && enemies[i]->stats.cur_state != ENEMY_CRITDEAD && enemies[i]->stats.speed > 0.0f)
			allies.push(enemies[i]);
		else {
			enemies[i]->unloadSounds();
			delete enemies[i];
		}
	}
	enemies.clear();

	prototypes.clear();

	// load new enemies
	while (!mapr->enemies.empty()) {
		me = mapr->enemies.front();
		mapr->enemies.pop();

		if (me.type.empty()) {
			logError("EnemyManager: Enemy(%f, %f) doesn't have type attribute set, skipping", me.pos.x, me.pos.y);
			continue;
		}


		bool status_reqs_met = true;
		//if the status requirements arent met, dont load the enemy
		for(unsigned int i = 0; i < me.requires_status.size(); i++) {
			if (!camp->checkStatus(me.requires_status[i]))
				status_reqs_met = false;
		}
		for(unsigned int i = 0; i < me.requires_not_status.size(); i++) {
			if (camp->checkStatus(me.requires_not_status[i]))
				status_reqs_met = false;
		}
		if(!status_reqs_met)
			continue;


		Enemy *e = getEnemyPrototype(me.type);

		e->stats.waypoints = me.waypoints;
		e->stats.pos.x = me.pos.x;
		e->stats.pos.y = me.pos.y;
		e->stats.direction = me.direction;
		e->stats.wander = me.wander_radius > 0;
		e->stats.setWanderArea(me.wander_radius);

		enemies.push_back(e);

		mapr->collider.block(me.pos.x, me.pos.y, false);
	}

	FPoint spawn_pos = mapr->collider.get_random_neighbor(floor(pc->stats.pos), 1, false);
	while (!allies.empty()) {

		Enemy *e = allies.front();
		allies.pop();

		//dont need the result of this. its only called to handle animation and sound
		Enemy* temp = getEnemyPrototype(e->type);
		delete temp;

		e->stats.pos = spawn_pos;
		e->stats.direction = pc->stats.direction;

		enemies.push_back(e);

		mapr->collider.block(e->stats.pos.x, e->stats.pos.y, true);
	}

	anim->cleanUp();
}

/**
 * Powers can cause new enemies to spawn
 * Check PowerManager for any new queued enemies
 */
void EnemyManager::handleSpawn() {

	Map_Enemy espawn;

	while (!powers->enemies.empty()) {
		espawn = powers->enemies.front();
		powers->enemies.pop();

		Enemy *e = new Enemy();
		// factory
		if(espawn.hero_ally)
			e->eb = new BehaviorAlly(e);
		else
			e->eb = new BehaviorStandard(e);

		e->stats.hero_ally = espawn.hero_ally;
		e->stats.summoned = true;
		e->stats.summoned_power_index = espawn.summon_power_index;

		if(espawn.summoner != NULL) {
			e->stats.summoner = espawn.summoner;
			espawn.summoner->summons.push_back(&(e->stats));
		}

		e->stats.direction = espawn.direction;

		Enemy_Level el = enemyg->getRandomEnemy(espawn.type, 0, 0);
		e->type = el.type;

		if (el.type != "") {
			e->stats.load(el.type);
		}
		else {
			logError("EnemyManager: Could not spawn creature type '%s'", espawn.type.c_str());
			delete e;
			return;
		}

		if (e->stats.animations != "") {
			// load the animation file if specified
			anim->increaseCount(e->stats.animations);
			e->animationSet = anim->getAnimationSet(e->stats.animations);
			if (e->animationSet)
				e->activeAnimation = e->animationSet->getAnimation();
			else
				logError("EnemyManager: Animations file could not be loaded for %s", espawn.type.c_str());
		}
		else {
			logError("EnemyManager: No animation file specified for entity: %s", espawn.type.c_str());
		}
		e->loadSounds();

		//Set level
		if(e->stats.summoned_power_index != 0) {
			if(powers->powers[e->stats.summoned_power_index].spawn_level_mode == SPAWN_LEVEL_MODE_FIXED)
				e->stats.level = powers->powers[e->stats.summoned_power_index].spawn_level_qty;

			if(powers->powers[e->stats.summoned_power_index].spawn_level_mode == SPAWN_LEVEL_MODE_LEVEL) {
				if(e->stats.summoner != NULL && powers->powers[e->stats.summoned_power_index].spawn_level_every != 0) {
					e->stats.level = powers->powers[e->stats.summoned_power_index].spawn_level_qty
									 * (e->stats.summoner->level / powers->powers[e->stats.summoned_power_index].spawn_level_every);
				}
			}

			if(powers->powers[e->stats.summoned_power_index].spawn_level_mode == SPAWN_LEVEL_MODE_STAT) {
				if(e->stats.summoner != NULL && powers->powers[e->stats.summoned_power_index].spawn_level_every != 0) {
					int stat_val = 0;
					if(powers->powers[e->stats.summoned_power_index].spawn_level_stat == SPAWN_LEVEL_STAT_DEFENSE)
						stat_val = e->stats.summoner->get_defense();
					if(powers->powers[e->stats.summoned_power_index].spawn_level_stat == SPAWN_LEVEL_STAT_OFFENSE)
						stat_val = e->stats.summoner->get_offense();
					if(powers->powers[e->stats.summoned_power_index].spawn_level_stat == SPAWN_LEVEL_STAT_MENTAL)
						stat_val = e->stats.summoner->get_mental();
					if(powers->powers[e->stats.summoned_power_index].spawn_level_stat == SPAWN_LEVEL_STAT_PHYSICAL)
						stat_val = e->stats.summoner->get_physical();

					e->stats.level = powers->powers[e->stats.summoned_power_index].spawn_level_qty
									 * (stat_val / powers->powers[e->stats.summoned_power_index].spawn_level_every);
				}
			}

			if(e->stats.level < 1) e->stats.level = 1;

			e->stats.applyEffects();
		}

		if (mapr->collider.is_valid_position(espawn.pos.x + 0.5f, espawn.pos.y + 0.5f, e->stats.movement_type, false) || !e->stats.hero_ally) {
			e->stats.pos.x = espawn.pos.x + 0.5f;
			e->stats.pos.y = espawn.pos.y + 0.5f;
		}
		else {
			e->stats.pos.x = pc->stats.pos.x;
			e->stats.pos.y = pc->stats.pos.y;
		}
		// special animation state for spawning enemies
		e->stats.cur_state = ENEMY_SPAWN;

		//now apply post effects to the spawned enemy
		if(e->stats.summoned_power_index > 0)
			powers->effect(&e->stats, (espawn.summoner != NULL ? espawn.summoner : &e->stats), e->stats.summoned_power_index, e->stats.hero_ally ? SOURCE_TYPE_HERO : SOURCE_TYPE_ENEMY);

		//apply party passives
		//synchronise tha party passives in the pc stat block with the passives in the allies stat blocks
		//at the time the summon is spawned, it takes the passives available at that time. if the passives change later, the changes wont affect summons retrospectively. could be exploited with equipment switching
		for (unsigned i=0; i< pc->stats.powers_passive.size(); i++) {
			if (powers->powers[pc->stats.powers_passive[i]].passive && powers->powers[pc->stats.powers_passive[i]].buff_party && e->stats.hero_ally
					&& (powers->powers[pc->stats.powers_passive[i]].buff_party_power_id == 0 || powers->powers[pc->stats.powers_passive[i]].buff_party_power_id == e->stats.summoned_power_index)) {

				e->stats.powers_passive.push_back(pc->stats.powers_passive[i]);
			}
		}

		for (unsigned i=0; i<pc->stats.powers_list_items.size(); i++) {
			if (powers->powers[pc->stats.powers_list_items[i]].passive && powers->powers[pc->stats.powers_list_items[i]].buff_party && e->stats.hero_ally
					&& (powers->powers[pc->stats.powers_passive[i]].buff_party_power_id == 0 || powers->powers[pc->stats.powers_passive[i]].buff_party_power_id == e->stats.summoned_power_index)) {

				e->stats.powers_passive.push_back(pc->stats.powers_list_items[i]);
			}
		}

		enemies.push_back(e);

		mapr->collider.block(espawn.pos.x, espawn.pos.y, e->stats.hero_ally);
	}
}

void EnemyManager::handlePartyBuff() {
	while (!powers->party_buffs.empty()) {
		int power_index = powers->party_buffs.front();
		powers->party_buffs.pop();
		Power *buff_power = &powers->powers[power_index];

		for (unsigned int i=0; i < enemies.size(); i++) {
			if(enemies[i]->stats.hero_ally && enemies[i]->stats.hp > 0 && (buff_power->buff_party_power_id == 0 || buff_power->buff_party_power_id == enemies[i]->stats.summoned_power_index)) {
				powers->effect(&enemies[i]->stats, &pc->stats, power_index,SOURCE_TYPE_HERO);
			}
		}
	}
}

/**
 * perform logic() for all enemies
 */
void EnemyManager::logic() {

	if(player_blocked) {
		player_blocked_ticks--;
		if(player_blocked_ticks <= 0)
			player_blocked = false;
	}

	handleSpawn();

	handlePartyBuff();

	std::vector<Enemy*>::iterator it;
	for (it = enemies.begin(); it != enemies.end(); ++it) {
		// hazards are processed after Avatar and Enemy[]
		// so process and clear sound effects from previous frames
		// check sound effects
		if (AUDIO) {
			if ((*it)->play_sfx_phys)
				snd->play((*it)->sound_melee, GLOBAL_VIRTUAL_CHANNEL, (*it)->stats.pos, false);
			if ((*it)->play_sfx_ment)
				snd->play((*it)->sound_mental, GLOBAL_VIRTUAL_CHANNEL, (*it)->stats.pos, false);
			if ((*it)->play_sfx_hit)
				snd->play((*it)->sound_hit, GLOBAL_VIRTUAL_CHANNEL, (*it)->stats.pos, false);
			if ((*it)->play_sfx_die)
				snd->play((*it)->sound_die, GLOBAL_VIRTUAL_CHANNEL, (*it)->stats.pos, false);
			if ((*it)->play_sfx_critdie)
				snd->play((*it)->sound_critdie, GLOBAL_VIRTUAL_CHANNEL, (*it)->stats.pos, false);

			// clear sound flags
			(*it)->play_sfx_hit = false;
			(*it)->play_sfx_phys = false;
			(*it)->play_sfx_ment = false;
			(*it)->play_sfx_die = false;
			(*it)->play_sfx_critdie = false;
		}

		// new actions this round
		(*it)->stats.hero_stealth = hero_stealth;
		(*it)->logic();
	}
}

Enemy* EnemyManager::enemyFocus(Point mouse, FPoint cam, bool alive_only) {
	Point p;
	Rect r;
	for(unsigned int i = 0; i < enemies.size(); i++) {
		if(alive_only && (enemies[i]->stats.cur_state == ENEMY_DEAD || enemies[i]->stats.cur_state == ENEMY_CRITDEAD)) {
			continue;
		}
		p = map_to_screen(enemies[i]->stats.pos.x, enemies[i]->stats.pos.y, cam.x, cam.y);

		r.w = enemies[i]->getRender().src.w;
		r.h = enemies[i]->getRender().src.h;
		r.x = p.x - enemies[i]->getRender().offset.x;
		r.y = p.y - enemies[i]->getRender().offset.y;

		if (isWithin(r, mouse)) {
			Enemy *enemy = enemies[i];
			return enemy;
		}
	}
	return NULL;
}

Enemy* EnemyManager::getNearestEnemy(FPoint pos) {
	Enemy* nearest = NULL;
	float best_distance = std::numeric_limits<float>::max();

	for (unsigned i=0; i<enemies.size(); i++) {
		if(enemies[i]->stats.cur_state == ENEMY_DEAD || enemies[i]->stats.cur_state == ENEMY_CRITDEAD) {
			continue;
		}

		float distance = calcDist(pos, enemies[i]->stats.pos);
		if (distance < best_distance) {
			best_distance = distance;
			nearest = enemies[i];
		}
	}

	if (best_distance > INTERACT_RANGE) nearest = NULL;

	return nearest;
}

/**
 * If an enemy has died, reward the hero with experience points
 */
void EnemyManager::checkEnemiesforXP() {
	for (unsigned int i=0; i < enemies.size(); i++) {
		if (enemies[i]->reward_xp) {
			//adjust for party exp if necessary
			float xp_multiplier = 1;
			if(enemies[i]->kill_source_type == SOURCE_TYPE_ALLY)
				xp_multiplier = (float)PARTY_EXP_PERCENTAGE / 100.0f;

			camp->rewardXP((int)(enemies[i]->stats.xp * xp_multiplier), false);
			enemies[i]->reward_xp = false; // clear flag
		}
	}
}

bool EnemyManager::isCleared() {
	if (enemies.empty()) return true;

	for (unsigned int i=0; i < enemies.size(); i++) {
		if (enemies[i]->stats.alive && !enemies[i]->stats.hero_ally)
			return false;
	}

	return true;
}

/**
 * addRenders()
 * Map objects need to be drawn in Z order, so we allow a parent object (GameEngine)
 * to collect all mobile sprites each frame.
 */
void EnemyManager::addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	std::vector<Enemy*>::iterator it;
	for (it = enemies.begin(); it != enemies.end(); ++it) {
		bool dead = (*it)->stats.corpse;
		if (!dead || (dead && (*it)->stats.corpse_ticks > 0)) {
			Renderable re = (*it)->getRender();
			re.prio = 1;

			// draw corpses below objects so that floor loot is more visible
			(dead ? r_dead : r).push_back(re);

			// add effects
			for (unsigned i = 0; i < (*it)->stats.effects.effect_list.size(); ++i) {
				if ((*it)->stats.effects.effect_list[i].animation) {
					Renderable ren = (*it)->stats.effects.effect_list[i].animation->getCurrentFrame(0);
					ren.map_pos = (*it)->stats.pos;
					if ((*it)->stats.effects.effect_list[i].render_above) ren.prio = 2;
					else ren.prio = 0;
					r.push_back(ren);
				}
			}
		}
	}
}

EnemyManager::~EnemyManager() {
	for (unsigned int i=0; i < enemies.size(); i++) {
		anim->decreaseCount(enemies[i]->animationSet->getName());
		enemies[i]->unloadSounds();
		delete enemies[i];
	}
}
