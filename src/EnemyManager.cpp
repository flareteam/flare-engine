/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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
	e->activeAnimation = e->animationSet->getAnimation("");
}

Enemy *EnemyManager::getEnemyPrototype(const std::string& type_id) {
	Enemy* e = new Enemy(prototypes.at(loadEnemyPrototype(type_id)));
	anim->increaseCount(e->stats.animations);
	return e;
}

size_t EnemyManager::loadEnemyPrototype(const std::string& type_id) {
	for (size_t i = 0; i < prototypes.size(); i++) {
		if (prototypes[i].type == type_id) {
			return i;
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

	// set cooldown_hit to duration of hit animation if undefined
	if (e.stats.cooldown_hit == -1) {
		Animation *hit_anim = e.animationSet->getAnimation("hit");
		if (hit_anim) {
			e.stats.cooldown_hit = hit_anim->getDuration();
			delete hit_anim;
		}
		else {
			e.stats.cooldown_hit = 0;
		}
	}

	prototypes.push_back(e);
	size_t prototype = prototypes.size() - 1;

	for (size_t i = 0; i < e.stats.powers_ai.size(); i++) {
		int power_index = e.stats.powers_ai[i].id;
		const std::string& spawn_type = powers->powers[power_index].spawn_type;
		if (power_index != 0 && spawn_type != "" && spawn_type != "untransform") {
			std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(spawn_type);
			for (size_t j = 0; j < spawn_enemies.size(); j++) {
				loadEnemyPrototype(spawn_enemies[j].type);
			}
		}
	}

	return prototype;
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


	for (unsigned int i=0; i < prototypes.size(); i++) {
		anim->decreaseCount(prototypes[i].animationSet->getName());
		prototypes[i].unloadSounds();
	}
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
		e->stats.direction = static_cast<unsigned char>(me.direction);
		e->stats.wander = me.wander_radius > 0;
		e->stats.setWanderArea(me.wander_radius);

		enemies.push_back(e);

		mapr->collider.block(me.pos.x, me.pos.y, false);
	}

	FPoint spawn_pos = mapr->collider.get_random_neighbor(FPointToPoint(pc->stats.pos), 1, false);
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

	// load enemies that can be spawn by avatar's powers
	for (size_t i = 0; i < pc->stats.powers_list.size(); i++) {
		int power_index = pc->stats.powers_list[i];
		const std::string& spawn_type = powers->powers[power_index].spawn_type;
		if (spawn_type != "" && spawn_type != "untransform") {
			std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(spawn_type);
			for (size_t j = 0; j < spawn_enemies.size(); j++) {
				loadEnemyPrototype(spawn_enemies[j].type);
			}
		}
	}

	// load enemies that can be spawn by powers in the action bar
	if (menu_act != NULL) {
		for (size_t i = 0; i < menu_act->hotkeys.size(); i++) {
			int power_index = menu_act->hotkeys[i];
			const std::string& spawn_type = powers->powers[power_index].spawn_type;
			if (power_index != 0 && spawn_type != "" && spawn_type != "untransform") {
				std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(spawn_type);
				for (size_t j = 0; j < spawn_enemies.size(); j++) {
					loadEnemyPrototype(spawn_enemies[j].type);
				}
			}
		}
	}

	// load enemies that can be spawn by map events
	for (size_t i = 0; i < mapr->events.size(); i++) {
		for (size_t j = 0; j < mapr->events[i].components.size(); j++) {
			if (mapr->events[i].components[j].type == EC_SPAWN) {
				std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(mapr->events[i].components[j].s);
				for (size_t k = 0; k < spawn_enemies.size(); k++) {
					loadEnemyPrototype(spawn_enemies[k].type);
				}
			}
		}
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
		e->stats.enemy_ally = espawn.enemy_ally;
		e->stats.summoned = true;
		e->stats.summoned_power_index = espawn.summon_power_index;

		if(espawn.summoner != NULL) {
			e->stats.summoner = espawn.summoner;
			espawn.summoner->summons.push_back(&(e->stats));
		}

		e->stats.direction = static_cast<unsigned char>(espawn.direction);

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
				e->activeAnimation = e->animationSet->getAnimation("");
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
					for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
						if (powers->powers[e->stats.summoned_power_index].spawn_level_stat == i) {
							stat_val = e->stats.summoner->get_primary(i);
							break;
						}
					}

					e->stats.level = powers->powers[e->stats.summoned_power_index].spawn_level_qty
									 * (stat_val / powers->powers[e->stats.summoned_power_index].spawn_level_every);
				}
			}

			if(e->stats.level < 1) e->stats.level = 1;

			e->stats.applyEffects();

			// TODO move this to StatBlock?
			e->stats.hp = e->stats.get(STAT_HP_MAX);
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
			int pwr = pc->stats.powers_passive[i];
			if (powers->powers[pwr].passive && powers->powers[pwr].buff_party && (e->stats.hero_ally || e->stats.enemy_ally)
					&& (powers->powers[pwr].buff_party_power_id == 0 || powers->powers[pwr].buff_party_power_id == e->stats.summoned_power_index)) {

				e->stats.powers_passive.push_back(pwr);
			}
		}

		for (unsigned i=0; i<pc->stats.powers_list_items.size(); i++) {
			int pwr = pc->stats.powers_list_items[i];
			if (powers->powers[pwr].passive && powers->powers[pwr].buff_party && (e->stats.hero_ally || e->stats.enemy_ally)
					&& (powers->powers[pwr].buff_party_power_id == 0 || powers->powers[pwr].buff_party_power_id == e->stats.summoned_power_index)) {

				e->stats.powers_passive.push_back(pwr);
			}
		}

		enemies.push_back(e);

		mapr->collider.block(espawn.pos.x, espawn.pos.y, e->stats.hero_ally);
	}
}

bool EnemyManager::checkPartyMembers() {
	for (unsigned int i=0; i < enemies.size(); i++) {
		if(enemies[i]->stats.hero_ally && enemies[i]->stats.hp > 0) {
			return true;
		}
	}
	return false;
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

	std::vector<Enemy*>::iterator it;
	for (it = enemies.begin(); it != enemies.end(); ++it) {
		// new actions this round
		(*it)->stats.hero_stealth = hero_stealth;
		(*it)->logic();
	}
}

Enemy* EnemyManager::enemyFocus(const Point& mouse, const FPoint& cam, bool alive_only) {
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

		if (isWithinRect(r, mouse)) {
			Enemy *enemy = enemies[i];
			return enemy;
		}
	}
	return NULL;
}

Enemy* EnemyManager::getNearestEnemy(const FPoint& pos) {
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
				xp_multiplier = static_cast<float>(PARTY_EXP_PERCENTAGE) / 100.0f;

			camp->rewardXP(static_cast<int>((static_cast<float>(enemies[i]->stats.xp) * xp_multiplier)), false);
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

void EnemyManager::spawn(const std::string& enemy_type, const Point& target) {
	Map_Enemy espawn;

	espawn.type = enemy_type;
	espawn.pos = target;

	// quick spawns start facing a random direction
	espawn.direction = rand() % 8;

	powers->enemies.push(espawn);
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
		if (!dead || (*it)->stats.corpse_ticks > 0) {
			Renderable re = (*it)->getRender();
			re.prio = 1;
			re.color_mod = (*it)->stats.effects.getCurrentColor();
			re.alpha_mod = (*it)->stats.effects.getCurrentAlpha();

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
	for (unsigned int i=0; i < prototypes.size(); i++) {
		anim->decreaseCount(prototypes[i].animationSet->getName());
		prototypes[i].unloadSounds();
	}
}
