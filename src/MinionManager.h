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

#ifndef MINIONMANAGER_H
#define MINIONMANAGER_H

#include "Settings.h"
#include "MapRenderer.h"
#include "Minion.h"
#include "Utils.h"
#include "PowerManager.h"
#include "CampaignManager.h"

class Minion;

class MinionManager
{
private:

    MapRenderer *map;
	PowerManager *powers;

	void loadSounds(const std::string& type_id);
	void loadAnimations(Minion *e);

	std::vector<std::string> sfx_prefixes;
	std::vector<SoundManager::SoundID> sound_phys;
	std::vector<SoundManager::SoundID> sound_ment;
	std::vector<SoundManager::SoundID> sound_hit;
	std::vector<SoundManager::SoundID> sound_die;
	std::vector<SoundManager::SoundID> sound_critdie;

	std::vector<std::string> anim_prefixes;
	std::vector<std::vector<Animation*> > anim_entities;


	std::vector<Minion> prototypes;

	public:
    MinionManager(PowerManager *_powers, MapRenderer *_map);
	~MinionManager();
	void handleSpawn();
	void logic();
	void addRenders(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);
	Minion *minionFocus(Point mouse, Point cam, bool alive_only);
	void RemoveCorpses();
	static bool RemoveCorpsePredicate(const Minion* m );

    EnemyManager* enemyManager;

	// vars
	std::vector<Minion*> minions;
	Point hero_pos;
	bool hero_alive;
	int hero_stealth;
	Avatar *pc;
};

#endif // MINIONMANAGER_H
