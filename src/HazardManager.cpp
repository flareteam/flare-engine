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
#include "EnemyManager.h"
#include "Hazard.h"
#include "HazardManager.h"
#include "SharedGameResources.h"
#include "SharedResources.h"

HazardManager::HazardManager()
	: last_enemy(NULL)
{
	Color mark_color(255, 0, 0, 255);

	int marker_size = TILE_H_HALF+1;
	dev_marker.image = render_device->createImage(marker_size, marker_size);
	for (int i = 0; i < marker_size; ++i) {
		dev_marker.image->drawPixel(i, (marker_size-1)/2, mark_color);
		dev_marker.image->drawPixel((marker_size-1)/2, i, mark_color);
	}
	dev_marker.src.x = 0;
	dev_marker.src.y = 0;
	dev_marker.src.w = marker_size;
	dev_marker.src.h = marker_size;
	dev_marker.offset.x = (marker_size-1)/2;
	dev_marker.offset.y = (marker_size-1)/2;
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
		if (h[i-1]->hit_wall && h[i-1]->wall_power > 0) {
			powers->activate(h[i-1]->wall_power, h[i-1]->src_stats, h[i-1]->pos);
			if (powers->powers[h[i-1]->wall_power].directional) powers->hazards.back()->animationKind = h[i-1]->animationKind;
		}

	}

	bool hit;

	// handle collisions
	for (size_t i=0; i<h.size(); i++) {
		if (h[i]->isDangerousNow()) {

			// process hazards that can hurt enemies
			if (h[i]->source_type != SOURCE_TYPE_ENEMY) { //hero or neutral sources
				for (unsigned int eindex = 0; eindex < enemies->enemies.size(); eindex++) {

					// only check living enemies
					if (enemies->enemies[eindex]->stats.hp > 0 && h[i]->active && (enemies->enemies[eindex]->stats.hero_ally == h[i]->target_party)) {
						if (isWithin(h[i]->pos, h[i]->radius, enemies->enemies[eindex]->stats.pos)) {
							if (!h[i]->hasEntity(enemies->enemies[eindex])) {
								h[i]->addEntity(enemies->enemies[eindex]);
								if (!h[i]->beacon) last_enemy = enemies->enemies[eindex];
								// hit!
								hit = enemies->enemies[eindex]->takeHit(*h[i]);
								hitEntity(i, hit);
							}
						}
					}

				}
			}

			// process hazards that can hurt the hero
			if (h[i]->source_type != SOURCE_TYPE_HERO && h[i]->source_type != SOURCE_TYPE_ALLY) { //enemy or neutral sources
				if (pc->stats.hp > 0 && h[i]->active) {
					if (isWithin(h[i]->pos, h[i]->radius, pc->stats.pos)) {
						if (!h[i]->hasEntity(pc)) {
							h[i]->addEntity(pc);
							// hit!
							hit = pc->takeHit(*h[i]);
							hitEntity(i, hit);
						}
					}
				}

				//now process allies
				for (unsigned int eindex = 0; eindex < enemies->enemies.size(); eindex++) {
					// only check living allies
					if (enemies->enemies[eindex]->stats.hp > 0 && h[i]->active && enemies->enemies[eindex]->stats.hero_ally) {
						if (isWithin(h[i]->pos, h[i]->radius, enemies->enemies[eindex]->stats.pos)) {
							if (!h[i]->hasEntity(enemies->enemies[eindex])) {
								h[i]->addEntity(enemies->enemies[eindex]);
								// hit!
								hit = enemies->enemies[eindex]->takeHit(*h[i]);
								hitEntity(i, hit);
							}
						}
					}
				}

			}

		}
	}
}

void HazardManager::hitEntity(size_t index, const bool hit) {
	if (!h[index]->multitarget && hit) {
		h[index]->active = false;
		if (!h[index]->complete_animation) h[index]->lifespan = 0;
	}
	if (hit && h[index]->sfx_hit_enable && !h[index]->sfx_hit_played) {
		snd->play(h[index]->sfx_hit);
		h[index]->sfx_hit_played = true;
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

	// check hero hazards
	if (pc->haz != NULL) {
		h.push_back(pc->haz);
		pc->haz = NULL;
	}

	// check monster hazards
	for (unsigned int eindex = 0; eindex < enemies->enemies.size(); eindex++) {
		if (enemies->enemies[eindex]->haz != NULL) {
			h.push_back(enemies->enemies[eindex]->haz);
			enemies->enemies[eindex]->haz = NULL;
		}
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

		if (DEV_HUD && h[i]->delay_frames == 0) {
			dev_marker.map_pos = h[i]->pos;
			dev_marker.prio = 3;
			r.push_back(dev_marker);
		}
	}
}

HazardManager::~HazardManager() {
	for (unsigned int i = 0; i < h.size(); i++)
		delete h[i];
	// h.clear(); not needed in destructor
	last_enemy = NULL;

	dev_marker.image->unref();
}
