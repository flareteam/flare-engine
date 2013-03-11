/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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


#include "MinionManager.h"

#include "AnimationManager.h"
#include "AnimationSet.h"
#include "Animation.h"
#include "SharedResources.h"
#include "MinionBehavior.h"

#include <iostream>
#include <algorithm>

using namespace std;

MinionManager::MinionManager(PowerManager *_powers, MapRenderer *_map)
	: map(_map)
	, powers(_powers)
	, minions(vector<Minion*>())
	, hero_alive(true)
	, hero_stealth(0)
{

}

void MinionManager::loadSounds(const string& type_id) {

	// first check to make sure the sfx isn't already loaded
	if (find(sfx_prefixes.begin(), sfx_prefixes.end(), type_id) != sfx_prefixes.end())
		return;

	if (type_id != "none") {
		sound_phys.push_back(snd->load("soundfx/enemies/" + type_id + "_phys.ogg", "MinionManager physical attack sound"));
		sound_ment.push_back(snd->load("soundfx/enemies/" + type_id + "_ment.ogg", "MinionManager mental attack sound"));
		sound_hit.push_back(snd->load("soundfx/enemies/" + type_id + "_hit.ogg", "MinionManager physical hit sound"));
		sound_die.push_back(snd->load("soundfx/enemies/" + type_id + "_die.ogg", "MinionManager die sound"));
		sound_critdie.push_back(snd->load("soundfx/enemies/" + type_id + "_critdie.ogg", "MinionManager critdeath sound"));
	} else {
		sound_phys.push_back(0);
		sound_ment.push_back(0);
		sound_hit.push_back(0);
		sound_die.push_back(0);
		sound_critdie.push_back(0);
	}

	sfx_prefixes.push_back(type_id);
}

void MinionManager::loadAnimations(Minion *e) {
	string animationsname = "animations/enemies/"+e->stats.animations + ".txt";
	anim->increaseCount(animationsname);
	e->animationSet = anim->getAnimationSet(animationsname);
	e->activeAnimation = e->animationSet->getAnimation();
}

/**
 * Powers can cause new enemies to spawn
 * Check PowerManager for any new queued enemies
 */
void MinionManager::handleSpawn() {

	while (!powers->minions.empty()) {
		Minion* new_minion = powers->minions.front();
		powers->minions.pop();

        new_minion->stats.minion = true;
        new_minion->minionManager = this;
		new_minion->map = map;
		new_minion->originatedFromSpawnPower = true;

		// factory
		new_minion->eb = new MinionBehavior(new_minion);

		new_minion->stats.corpse = false;

		new_minion->stats.load("enemies/" + new_minion->type + ".txt");
		if (new_minion->stats.animations != "") {
			// load the animation file if specified
			string animationname = "animations/enemies/"+new_minion->stats.animations + ".txt";
			anim->increaseCount(animationname);
			new_minion->animationSet = anim->getAnimationSet(animationname);
			if (new_minion->animationSet)
            {
				new_minion->activeAnimation = new_minion->animationSet->getAnimation();
				new_minion->setAnimation("spawn");
            }
			else
				cout << "Warning: animations file could not be loaded for minion " << new_minion->type << endl;
		}
		else {
			cout << "Warning: no animation file specified for minion: " << new_minion->type << endl;
		}
		loadSounds(new_minion->stats.sfx_prefix);

		// special animation state for spawning enemies
		new_minion->stats.cur_state = MINION_SPAWN;
		minions.push_back(new_minion);

	}

}

/**
 * perform logic() for all enemies
 */
void MinionManager::logic() {

	handleSpawn();

	for (unsigned int i=0; i < minions.size(); i++) {

		// hazards are processed after Avatar and Minion[]
		// so process and clear sound effects from previous frames
		// check sound effects
		if (AUDIO) {
			vector<string>::iterator found = find (sfx_prefixes.begin(), sfx_prefixes.end(), minions[i]->stats.sfx_prefix);
			unsigned pref_id = distance(sfx_prefixes.begin(), found);

			if (pref_id >= sfx_prefixes.size()) {
				cerr << "ERROR: Minion sfx_prefix doesn't match registered prefixes (Minion: '"
					 << minions[i]->stats.name << "', sfx_prefix: '"
					 << minions[i]->stats.sfx_prefix << "')" << endl;
			} else {
				if (minions[i]->sfx_phys) snd->play(sound_phys[pref_id]);
				if (minions[i]->sfx_ment) snd->play(sound_ment[pref_id]);
				if (minions[i]->sfx_hit) snd->play(sound_hit[pref_id]);
				if (minions[i]->sfx_die) snd->play(sound_die[pref_id]);
				if (minions[i]->sfx_critdie) snd->play(sound_critdie[pref_id]);
			}

			// clear sound flags
			minions[i]->sfx_hit = false;
			minions[i]->sfx_phys = false;
			minions[i]->sfx_ment = false;
			minions[i]->sfx_die = false;
			minions[i]->sfx_critdie = false;
		}

        // new actions this round
        minions[i]->stats.hero_pos = hero_pos;
        minions[i]->stats.hero_alive = hero_alive;
        minions[i]->stats.hero_stealth = hero_stealth;
        minions[i]->logic();

	}

}

void MinionManager::RemoveCorpses()
{

	for (unsigned int i=0; i < minions.size(); i++) {
        if(minions[i]->stats.corpse)
            anim->decreaseCount(minions[i]->animationSet->getName());
	}

    minions.erase(std::remove_if( minions.begin(), minions.end(), RemoveCorpsePredicate ), minions.end() );

}

bool MinionManager::RemoveCorpsePredicate( const Minion* m ) {
    return m->stats.corpse;
}

Minion* MinionManager::minionFocus(Point mouse, Point cam, bool alive_only) {
	Point p;
	SDL_Rect r;
	for(unsigned int i = 0; i < minions.size(); i++) {
		if(alive_only && (minions[i]->stats.cur_state == MINION_DEAD || minions[i]->stats.cur_state == MINION_CRITDEAD)) {
			continue;
		}
		p = map_to_screen(minions[i]->stats.pos.x, minions[i]->stats.pos.y, cam.x, cam.y);

		r.w = minions[i]->getRender().src.w;
		r.h = minions[i]->getRender().src.h;
		r.x = p.x - minions[i]->getRender().offset.x;
		r.y = p.y - minions[i]->getRender().offset.y;

		if (isWithin(r, mouse)) {
			Minion *Minion = minions[i];
			return Minion;
		}
	}
	return NULL;
}

/**
 * addRenders()
 * Map objects need to be drawn in Z order, so we allow a parent object (GameEngine)
 * to collect all mobile sprites each frame.
 */
void MinionManager::addRenders(vector<Renderable> &r, vector<Renderable> &r_dead) {
	vector<Minion*>::iterator it;
	for (it = minions.begin(); it != minions.end(); ++it) {
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

MinionManager::~MinionManager() {
	for (unsigned int i=0; i < minions.size(); i++) {
		anim->decreaseCount(minions[i]->animationSet->getName());
		delete minions[i];
	}

	for (unsigned i=0; i<sound_phys.size(); i++) {
		snd->unload(sound_phys[i]);
		snd->unload(sound_ment[i]);
		snd->unload(sound_hit[i]);
		snd->unload(sound_die[i]);
		snd->unload(sound_critdie[i]);
	}
}
