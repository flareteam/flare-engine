/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2012-2015 Justin Jacobs

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

/**
 * class HazardManager
 *
 * Holds the collection of hazards (active attacks, spells, etc) and handles group operations
 */

#include "Avatar.h"
#include "Animation.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "EventManager.h"
#include "Hazard.h"
#include "HazardManager.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "UtilsMath.h"

HazardManager::HazardManager()
	: last_enemy(NULL)
{
}

void HazardManager::logic() {

	// remove all hazards with lifespan 0.  Most hazards still display their last frame.
	for (size_t i=h.size(); i>0; i--) {
		if (h[i-1]->lifespan == 0) {
			delete h[i-1];
			h.erase(h.begin()+(i-1));
		}
	}

	checkNewHazards();

	// handle single-frame transforms
	for (size_t i=h.size(); i>0; i--) {
		h[i-1]->logic();

		// remove all hazards that need to die immediately (e.g. exit the map)
		if (h[i-1]->remove_now) {
			delete h[i-1];
			h.erase(h.begin()+(i-1));
			continue;
		}


		// if a moving hazard hits a wall, check for an after-effect
		if (h[i-1]->hit_wall) {
			if (h[i-1]->power->script_trigger == Power::SCRIPT_TRIGGER_WALL) {
				EventManager::executeScript(h[i-1]->power->script, h[i-1]->pos.x, h[i-1]->pos.y);
			}

			if (h[i-1]->power->wall_power > 0 && Math::percentChance(h[i-1]->power->wall_power_chance)) {
				powers->activate(h[i-1]->power->wall_power, h[i-1]->src_stats, h[i-1]->pos);

				if (powers->powers[h[i-1]->power->wall_power].directional) {
					powers->hazards.back()->animationKind = h[i-1]->animationKind;
				}
			}

			// clear wall hit
			h[i-1]->hit_wall = false;
		}

	}

	// handle collisions
	for (size_t i=0; i<h.size(); i++) {
		if (h[i]->isDangerousNow()) {

			// process hazards that can hurt enemies
			if (h[i]->source_type != Power::SOURCE_TYPE_ENEMY) { //hero or neutral sources
				for (unsigned int eindex = 0; eindex < enemym->enemies.size(); eindex++) {

					// only check living enemies
					if (enemym->enemies[eindex]->stats.hp > 0 && h[i]->active && (enemym->enemies[eindex]->stats.hero_ally == h[i]->power->target_party)) {
						if (Utils::isWithinRadius(h[i]->pos, h[i]->power->radius, enemym->enemies[eindex]->stats.pos)) {
							if (!h[i]->hasEntity(enemym->enemies[eindex])) {
								// hit!
								h[i]->addEntity(enemym->enemies[eindex]);
								hitEntity(i, enemym->enemies[eindex]->takeHit(*h[i]));
								if (!h[i]->power->beacon) {
									last_enemy = enemym->enemies[eindex];
								}
							}
						}
					}

				}
			}

			// process hazards that can hurt the hero
			if (h[i]->source_type != Power::SOURCE_TYPE_HERO && h[i]->source_type != Power::SOURCE_TYPE_ALLY) { //enemy or neutral sources
				if (pc->stats.hp > 0 && h[i]->active) {
					if (Utils::isWithinRadius(h[i]->pos, h[i]->power->radius, pc->stats.pos)) {
						if (!h[i]->hasEntity(pc)) {
							// hit!
							h[i]->addEntity(pc);
							hitEntity(i, pc->takeHit(*h[i]));
						}
					}
				}

				//now process allies
				for (unsigned int eindex = 0; eindex < enemym->enemies.size(); eindex++) {
					// only check living allies
					if (enemym->enemies[eindex]->stats.hp > 0 && h[i]->active && enemym->enemies[eindex]->stats.hero_ally) {
						if (Utils::isWithinRadius(h[i]->pos, h[i]->power->radius, enemym->enemies[eindex]->stats.pos)) {
							if (!h[i]->hasEntity(enemym->enemies[eindex])) {
								// hit!
								h[i]->addEntity(enemym->enemies[eindex]);
								hitEntity(i, enemym->enemies[eindex]->takeHit(*h[i]));
							}
						}
					}
				}

			}

		}
	}
}

void HazardManager::hitEntity(size_t index, const bool hit) {
	if (!hit) return;

	if (!h[index]->power->multitarget) {
		h[index]->active = false;
		if (!h[index]->power->complete_animation) h[index]->lifespan = 0;
	}
	if (h[index]->power->sfx_hit_enable && !h[index]->sfx_hit_played) {
		snd->play(h[index]->power->sfx_hit, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
		h[index]->sfx_hit_played = true;
	}

	if (h[index]->power->script_trigger == Power::SCRIPT_TRIGGER_HIT) {
		EventManager::executeScript(h[index]->power->script, h[index]->pos.x, h[index]->pos.y);
	}
}

/**
 * Look for hazards generated this frame
 */
void HazardManager::checkNewHazards() {

	// check PowerManager for hazards
	while (!powers->hazards.empty()) {
		Hazard *new_haz = powers->hazards.front();
		powers->hazards.pop();

		h.push_back(new_haz);
	}
}

/**
 * Reset all hazards and get new collision object
 */
void HazardManager::handleNewMap() {
	for (unsigned int i = 0; i < h.size(); i++) {
		delete h[i];
	}
	h.clear();
	last_enemy = NULL;
}

/**
 * addRenders()
 * Map objects need to be drawn in Z order, so we allow a parent object (GameEngine)
 * to collect all mobile sprites each frame.
 */
void HazardManager::addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead) {
	for (unsigned int i=0; i<h.size(); i++) {
		h[i]->addRenderable(r, r_dead);
	}
}

HazardManager::~HazardManager() {
	for (unsigned int i = 0; i < h.size(); i++)
		delete h[i];
	// h.clear(); not needed in destructor
	last_enemy = NULL;
}
