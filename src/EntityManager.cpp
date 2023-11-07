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

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "Avatar.h"
#include "CampaignManager.h"
#include "EnemyGroupManager.h"
#include "Entity.h"
#include "EntityBehavior.h"
#include "EntityManager.h"
#include "EngineSettings.h"
#include "EntityBehavior.h"
#include "EventManager.h"
#include "FogOfWar.h"
#include "Hazard.h"
#include "MapRenderer.h"
#include "MenuActionBar.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"

#include <limits>

EntityManager::EntityManager()
	: entities()
	, hero_stealth(0)
	, player_blocked(false)
	, player_blocked_timer(settings->max_frames_per_sec / 6) {
	handleNewMap();
}

Entity *EntityManager::getEntityPrototype(const std::string& type_id) {
	Entity* e = new Entity(prototypes.at(loadEntityPrototype(type_id)));
	return e;
}

size_t EntityManager::loadEntityPrototype(const std::string& type_id) {
	for (size_t i = 0; i < prototypes.size(); i++) {
		if (prototypes[i].type_filename == type_id) {
			return i;
		}
	}

	Entity e = Entity();

	e.stats.load(type_id);
	e.type_filename = type_id;

	if (e.stats.animations == "")
		Utils::logError("EntityManager: No animation file specified for entity: %s", type_id.c_str());

	e.loadAnimations();
	e.loadSounds();

	// set cooldown_hit to duration of hit animation if undefined
	if (!e.stats.cooldown_hit_enabled && e.animationSet) {
		Animation *hit_anim = e.animationSet->getAnimation("hit");
		if (hit_anim) {
			e.stats.cooldown_hit.setDuration(hit_anim->getDuration());
			delete hit_anim;
		}
		else {
			e.stats.cooldown_hit.setDuration(0);
		}
	}

	prototypes.push_back(e);
	size_t prototype = prototypes.size() - 1;

	for (size_t i = 0; i < e.stats.powers_ai.size(); i++) {
		PowerID power_index = e.stats.powers_ai[i].id;
		if (powers->isValid(power_index)) {
			const std::string& spawn_type = powers->powers[power_index]->spawn_type;
			if (!spawn_type.empty() && spawn_type != "untransform") {
				std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(spawn_type);
				for (size_t j = 0; j < spawn_enemies.size(); j++) {
					loadEntityPrototype(spawn_enemies[j].type);
				}
			}
		}
	}

	return prototype;
}

/**
 * When loading a new map, we eliminate existing entities and load the new ones.
 * The map will have loaded Entity blocks into an array; retrieve the entities and init them
 */
void EntityManager::handleNewMap () {

	Map_Enemy me;
	std::queue<Entity *> allies;

	// delete existing entities
	for (unsigned int i=0; i < entities.size(); i++) {
		if (entities[i]->stats.npc)
			continue;

		if(entities[i]->stats.hero_ally && !entities[i]->stats.corpse && entities[i]->stats.cur_state != StatBlock::ENTITY_DEAD && entities[i]->stats.cur_state != StatBlock::ENTITY_CRITDEAD && entities[i]->stats.speed > 0.0f)
			allies.push(entities[i]);
		else {
			entities[i]->unloadSounds();
			delete entities[i];
		}
	}
	entities.clear();


	for (unsigned int i=0; i < prototypes.size(); i++) {
		prototypes[i].unloadSounds();
	}
	prototypes.clear();

	// load new entities
	while (!mapr->enemies.empty()) {
		me = mapr->enemies.front();
		mapr->enemies.pop();

		if (me.type.empty()) {
			Utils::logError("EntityManager: Entity(%f, %f) doesn't have type attribute set, skipping", me.pos.x, me.pos.y);
			continue;
		}

		if (!camp->checkRequirementsInVector(me.requirements))
			continue;

		Entity *e = getEntityPrototype(me.type);

		e->stats.waypoints = me.waypoints;
		e->stats.pos.x = me.pos.x;
		e->stats.pos.y = me.pos.y;
		e->stats.direction = static_cast<unsigned char>(me.direction);
		e->stats.wander = me.wander_radius > 0;
		e->stats.setWanderArea(me.wander_radius);
		e->stats.invincible_requirements = me.invincible_requirements;

		// Set level
		if (me.spawn_level.mode != SpawnLevel::MODE_DEFAULT) {
			if (me.spawn_level.mode == SpawnLevel::MODE_FIXED) {
				e->stats.level = static_cast<int>(me.spawn_level.count);
			}
			else if (pc != NULL && me.spawn_level.ratio != 0) {
				StatBlock* ratio_stats = &(pc->stats);

				if (me.spawn_level.mode == SpawnLevel::MODE_LEVEL) {
					e->stats.level = static_cast<int>(me.spawn_level.count * (static_cast<float>(ratio_stats->level) / me.spawn_level.ratio));
				}
				else if (me.spawn_level.mode == SpawnLevel::MODE_STAT) {
					int stat_val = 0;
					for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
						if (me.spawn_level.stat == i) {
							stat_val = ratio_stats->get_primary(i);
							break;
						}
					}

					e->stats.level = static_cast<int>(me.spawn_level.count * (static_cast<float>(stat_val) / me.spawn_level.ratio));
				}
			}

			// apply Effects and set HP to max HP
			e->stats.recalc();
		}

		entities.push_back(e);

		mapr->collider.block(me.pos.x, me.pos.y, !MapCollision::IS_ALLY);
	}

	// TODO support spawning flying enemies over pits?
	FPoint spawn_pos = mapr->collider.getRandomNeighbor(Point(pc->stats.pos), 1, MapCollision::MOVE_NORMAL, MapCollision::ENTITY_COLLIDE_ALL);
	while (!allies.empty()) {

		Entity *e = allies.front();
		allies.pop();

		//dont need the result of this. its only called to handle animation and sound
		Entity* temp = getEntityPrototype(e->type_filename);
		delete temp;

		e->stats.pos = spawn_pos;
		e->stats.direction = pc->stats.direction;

		entities.push_back(e);

		mapr->collider.block(e->stats.pos.x, e->stats.pos.y, MapCollision::IS_ALLY);
	}

	// load entities that can be spawn by avatar's powers
	for (size_t i = 0; i < pc->stats.powers_list.size(); i++) {
		PowerID power_index = pc->stats.powers_list[i];
		if (powers->isValid(power_index)) {
			const std::string& spawn_type = powers->powers[power_index]->spawn_type;
			if (!spawn_type.empty() && spawn_type != "untransform") {
				std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(spawn_type);
				for (size_t j = 0; j < spawn_enemies.size(); j++) {
					loadEntityPrototype(spawn_enemies[j].type);
				}
			}
		}
	}

	// load entities that can be spawn by powers in the action bar
	if (menu_act != NULL) {
		for (size_t i = 0; i < menu_act->hotkeys.size(); i++) {
			PowerID power_index = menu_act->hotkeys[i];
			if (power_index != 0) {
				const std::string& spawn_type = powers->powers[power_index]->spawn_type;
				if (!spawn_type.empty() && spawn_type != "untransform") {
					std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(spawn_type);
					for (size_t j = 0; j < spawn_enemies.size(); j++) {
						loadEntityPrototype(spawn_enemies[j].type);
					}
				}
			}
		}
	}

	// load entities that can be spawn by map events
	for (size_t i = 0; i < mapr->events.size(); i++) {
		for (size_t j = 0; j < mapr->events[i].components.size(); j++) {
			if (mapr->events[i].components[j].type == EventComponent::SPAWN) {
				std::vector<Enemy_Level> spawn_enemies = enemyg->getEnemiesInCategory(mapr->events[i].components[j].s);
				for (size_t k = 0; k < spawn_enemies.size(); k++) {
					loadEntityPrototype(spawn_enemies[k].type);
				}
			}
		}
	}

	anim->cleanUp();
}

/**
 * Powers can cause new entities to spawn
 * Check PowerManager for any new queued entities
 */
void EntityManager::handleSpawn() {

	Map_Enemy espawn;

	while (!powers->map_enemies.empty()) {
		espawn = powers->map_enemies.front();
		powers->map_enemies.pop();

		mapr->collider.unblock(espawn.pos.x, espawn.pos.y);

		Entity *e = new Entity();

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
		e->type_filename = el.type;

		if (el.type != "") {
			e->stats.load(el.type);
		}
		else {
			Utils::logError("EntityManager: Could not spawn creature type '%s'", espawn.type.c_str());
			delete e;
			return;
		}

		if (e->stats.animations == "") {
			Utils::logError("EntityManager: No animation file specified for entity: %s", espawn.type.c_str());
		}
		e->loadAnimations();
		e->loadSounds();

		//Set level
		if (powers->isValid(e->stats.summoned_power_index)) {
			SpawnLevel* spawn_level = &(powers->powers[e->stats.summoned_power_index]->spawn_level);

			if (spawn_level->mode == SpawnLevel::MODE_FIXED) {
				e->stats.level = static_cast<int>(spawn_level->count);
			}
			else if (e->stats.summoner != NULL && spawn_level->ratio != 0) {
				StatBlock* ratio_stats = e->stats.summoner;

				if (spawn_level->mode == SpawnLevel::MODE_LEVEL) {
					e->stats.level = static_cast<int>(spawn_level->count * (static_cast<float>(ratio_stats->level) / spawn_level->ratio));
				}
				else if (spawn_level->mode == SpawnLevel::MODE_STAT) {
					int stat_val = 0;
					for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
						if (spawn_level->stat == i) {
							stat_val = ratio_stats->get_primary(i);
							break;
						}
					}

					e->stats.level = static_cast<int>(spawn_level->count * (static_cast<float>(stat_val) / spawn_level->ratio));
				}
			}

			// apply Effects and set HP to max HP
			e->stats.recalc();
		}

		if (mapr->collider.isValidPosition(espawn.pos.x, espawn.pos.y, e->stats.movement_type, MapCollision::ENTITY_COLLIDE_ALL) || !e->stats.hero_ally) {
			e->stats.pos.x = espawn.pos.x;
			e->stats.pos.y = espawn.pos.y;
		}
		else {
			e->stats.pos = mapr->collider.getRandomNeighbor(Point(pc->stats.pos), 1, e->stats.movement_type, MapCollision::ENTITY_COLLIDE_ALL);
		}

		// special animation state for spawning entities
		e->stats.cur_state = StatBlock::ENTITY_SPAWN;

		//now apply post effects to the spawned entity
		powers->effect(&e->stats, (espawn.summoner != NULL ? espawn.summoner : &e->stats), e->stats.summoned_power_index, e->stats.hero_ally ? Power::SOURCE_TYPE_HERO : Power::SOURCE_TYPE_ENEMY);

		//apply party passives
		//synchronise tha party passives in the pc stat block with the passives in the allies stat blocks
		//at the time the summon is spawned, it takes the passives available at that time. if the passives change later, the changes wont affect summons retrospectively. could be exploited with equipment switching
		for (unsigned i=0; i< pc->stats.powers_passive.size(); i++) {
			PowerID pwr = pc->stats.powers_passive[i];
			if (powers->isValid(pwr) && powers->powers[pwr]->passive && powers->powers[pwr]->buff_party && (e->stats.hero_ally || e->stats.enemy_ally)
					&& (powers->powers[pwr]->buff_party_power_id == 0 || powers->powers[pwr]->buff_party_power_id == e->stats.summoned_power_index)) {

				e->stats.powers_passive.push_back(pwr);
			}
		}

		for (unsigned i=0; i<pc->stats.powers_list_items.size(); i++) {
			PowerID pwr = pc->stats.powers_list_items[i];
			if (powers->isValid(pwr) && powers->powers[pwr]->passive && powers->powers[pwr]->buff_party && (e->stats.hero_ally || e->stats.enemy_ally)
					&& (powers->powers[pwr]->buff_party_power_id == 0 || powers->powers[pwr]->buff_party_power_id == e->stats.summoned_power_index)) {

				e->stats.powers_passive.push_back(pwr);
			}
		}

		entities.push_back(e);

		mapr->collider.block(e->stats.pos.x, e->stats.pos.y, e->stats.hero_ally);
	}
}

bool EntityManager::checkPartyMembers() {
	for (unsigned int i=0; i < entities.size(); i++) {
		if(entities[i]->stats.hero_ally && entities[i]->stats.hp > 0) {
			return true;
		}
	}
	return false;
}

/**
 * perform logic() for all entities
 */
void EntityManager::logic() {

	if (player_blocked) {
		player_blocked_timer.tick();
		if (player_blocked_timer.isEnd())
			player_blocked = false;
	}

	handleSpawn();

	std::vector<Entity*>::iterator it;
	for (it = entities.begin(); it != entities.end(); ++it) {
		// new actions this round
		(*it)->stats.hero_stealth = hero_stealth;
		if (!(*it)->stats.npc) {
			(*it)->logic();
		}
	}
}

Entity* EntityManager::entityFocus(const Point& mouse, const FPoint& cam, bool alive_only) {
	Point p;
	Rect r;
	for(unsigned int i = 0; i < entities.size(); i++) {
		if(alive_only && (entities[i]->stats.cur_state == StatBlock::ENTITY_DEAD || entities[i]->stats.cur_state == StatBlock::ENTITY_CRITDEAD)) {
			continue;
		}

		if (Utils::isWithinRect(entities[i]->getRenderBounds(cam), mouse)) {
			Entity *entity = entities[i];
			return entity;
		}
	}
	return NULL;
}

Entity* EntityManager::getNearestEntity(const FPoint& pos, bool get_corpse, float *saved_distance, float max_range) {
	Entity* nearest = NULL;
	float best_distance = std::numeric_limits<float>::max();

	for (unsigned i=0; i<entities.size(); i++) {
		if(!get_corpse && (entities[i]->stats.cur_state == StatBlock::ENTITY_DEAD || entities[i]->stats.cur_state == StatBlock::ENTITY_CRITDEAD)) {
			continue;
		}
		if (get_corpse && !entities[i]->stats.corpse) {
			continue;
		}

		float distance = Utils::calcDist(pos, entities[i]->stats.pos);
		if (distance < best_distance) {
			best_distance = distance;
			nearest = entities[i];
		}
	}

	if (nearest && saved_distance)
		*saved_distance = best_distance;

	if (!saved_distance && best_distance > max_range)
		nearest = NULL;

	return nearest;
}

bool EntityManager::isCleared() {
	if (entities.empty()) return true;

	for (unsigned int i=0; i < entities.size(); i++) {
		if (entities[i]->stats.alive && !entities[i]->stats.hero_ally)
			return false;
	}

	return true;
}

void EntityManager::spawn(const std::string& entity_type, const Point& target) {
	Map_Enemy espawn;

	espawn.type = entity_type;
	espawn.pos = FPoint(target);
	espawn.pos.x += 0.5f;
	espawn.pos.y += 0.5f;

	// quick spawns start facing a random direction
	espawn.direction = rand() % 8;

	if (!mapr->collider.isValidPosition(espawn.pos.x, espawn.pos.y, MapCollision::MOVE_NORMAL, MapCollision::ENTITY_COLLIDE_NONE)) {
		return;
	}
	else {
		mapr->collider.block(espawn.pos.x, espawn.pos.y, !MapCollision::IS_ALLY);
	}

	powers->map_enemies.push(espawn);
}

/**
 * addRenders()
 * Map objects need to be drawn in Z order, so we allow a parent object (GameEngine)
 * to collect all mobile sprites each frame.
 */
void EntityManager::addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	std::vector<Entity*>::iterator it;
	for (it = entities.begin(); it != entities.end(); ++it) {
		if (mapr->fogofwar > FogOfWar::TYPE_MINIMAP) {
			float delta = Utils::calcDist(pc->stats.pos, (*it)->stats.pos);
			if (delta > fow->mask_radius-1.0) {
				continue;
			}
		}

		bool dead = (*it)->stats.corpse;
		if (!dead || !(*it)->stats.corpse_timer.isEnd()) {
			if (dead)
				(*it)->addRenders(r_dead);
			else
				(*it)->addRenders(r);
		}
	}
}

EntityManager::~EntityManager() {
	for (unsigned int i=0; i < entities.size(); i++) {
		if (entities[i]->stats.npc)
			continue;

		entities[i]->unloadSounds();
		delete entities[i];
	}
	for (unsigned int i=0; i < prototypes.size(); i++) {
		prototypes[i].unloadSounds();
	}
}
