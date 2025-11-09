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
#include "Entity.h"
#include "EntityManager.h"
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
			for (size_t j = 0; j < h[i-1]->power->chain_powers.size(); ++j) {
				ChainPower& chain_power = h[i-1]->power->chain_powers[j];
				if (chain_power.type == ChainPower::TYPE_EXPIRE && Math::percentChanceF(chain_power.chance)) {
					powers->activate(chain_power.id, h[i-1]->src_stats, h[i-1]->pos, h[i-1]->pos);

					if (powers->powers[chain_power.id]->directional) {
						powers->hazards.back()->direction = h[i-1]->direction;
					}
				}
			}

			delete h[i-1];
			h.erase(h.begin()+(i-1));
		}
	}

	checkNewHazards();

	// handle single-frame transforms
	for (size_t i=h.size(); i>0; i--) {
		size_t hindex = i-1;
		Hazard* hazard = h[hindex];

		hazard->logic();

		// remove all hazards that need to die immediately (e.g. exit the map)
		if (hazard->remove_now) {
			delete hazard;
			h.erase(h.begin()+(hindex));
			continue;
		}


		// if a moving hazard hits a wall, check for an after-effect
		if (hazard->hit_wall) {
			if (hazard->power->script_trigger == Power::SCRIPT_TRIGGER_WALL) {
				eventm->executeScript(hazard->power->script, hazard->pos.x, hazard->pos.y);
			}

			for (size_t j = 0; j < hazard->power->chain_powers.size(); ++j) {
				ChainPower& chain_power = hazard->power->chain_powers[j];
				if (chain_power.type == ChainPower::TYPE_WALL && Math::percentChanceF(chain_power.chance)) {
					powers->activate(chain_power.id, hazard->src_stats, hazard->pos, hazard->pos);

					if (powers->powers[chain_power.id]->directional) {
						powers->hazards.back()->direction = hazard->direction;
					}
				}
			}

			// clear wall hit
			hazard->hit_wall = false;
		}

		// handle collisions
		if (hazard->isDangerousNow()) {

			// process hazards that can hurt enemies & allies
			for (size_t eindex = 0; eindex < entitym->entities.size(); eindex++) {
				Entity *e = entitym->entities[eindex];

				// hero/ally powers can only hit allies if target_party is true
				if ((hazard->source_type == Power::SOURCE_TYPE_HERO || hazard->source_type == Power::SOURCE_TYPE_ALLY) && e->stats.hero_ally && !hazard->power->target_party) {
					continue;
				}

				// enemy hazard can't hurt other enemies
				if (hazard->source_type == Power::SOURCE_TYPE_ENEMY && !e->stats.hero_ally) {
					continue;
				}

				// only check living enemies
				if (e->stats.hp > 0 && hazard->active) {
					if (Utils::isWithinRadius(hazard->pos, hazard->power->radius, e->stats.pos)) {
						if (!hazard->hasEntity(e)) {
							// hit!
							hazard->addEntity(e);
							hitEntity(hindex, e->takeHit(*hazard));
							if (!hazard->power->beacon) {
								last_enemy = e;
							}
						}
					}
				}

			}

			// process hazards that can hurt the hero
			if (hazard->source_type != Power::SOURCE_TYPE_HERO && hazard->source_type != Power::SOURCE_TYPE_ALLY) { //enemy or neutral sources
				if (pc->stats.hp > 0 && hazard->active) {
					if (Utils::isWithinRadius(hazard->pos, hazard->power->radius, pc->stats.pos)) {
						if (!hazard->hasEntity(pc)) {
							// hit!
							hazard->addEntity(pc);
							hitEntity(hindex, pc->takeHit(*hazard));
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
		eventm->executeScript(h[index]->power->script, h[index]->pos.x, h[index]->pos.y);
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
