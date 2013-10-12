/*
Copyright © 2011-2012 Clint Bellanger and kitano
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

/**
 * class Entity
 *
 * An Entity represents any character in the game - the player, allies, enemies
 * This base class handles logic common to all of these child classes
 */

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "CommonIncludes.h"
#include "Entity.h"
#include "SharedResources.h"
#include "UtilsMath.h"
#include "SharedGameResources.h"

#include <math.h>

#ifdef _MSC_VER
#define M_SQRT2 sqrt(2.0)
#endif

const int directionDeltaX[8] =   {-1, -1, -1,  0,  1,  1,  1,  0};
const int directionDeltaY[8] =   { 1,  0, -1, -1, -1,  0,  1,  1};
const float speedMultiplyer[8] = { (float)(1.0/M_SQRT2), 1.0f, (float)(1.0/M_SQRT2), 1.0f, (float)(1.0/M_SQRT2), 1.0f, (float)(1.0/M_SQRT2), 1.0f};

using namespace std;

Entity::Entity()
	: sprites(NULL)
	, sfx_phys(false)
	, sfx_ment(false)
	, sfx_hit(false)
	, sfx_die(false)
	, sfx_critdie(false)
	, sfx_block(false)
	, activeAnimation(NULL)
	, animationSet(NULL) {
}

Entity::Entity(const Entity &e)
	: sprites(e.sprites)
	, sfx_phys(e.sfx_phys)
	, sfx_ment(e.sfx_ment)
	, sfx_hit(e.sfx_hit)
	, sfx_die(e.sfx_die)
	, sfx_critdie(e.sfx_critdie)
	, sfx_block(e.sfx_block)
	, activeAnimation(new Animation(*e.activeAnimation))
	, animationSet(e.animationSet)
	, stats(StatBlock(e.stats)) {
}

/**
 * move()
 * Apply speed to the direction faced.
 *
 * @return Returns false if wall collision, otherwise true.
 */
bool Entity::move() {

	if (stats.effects.forced_move)
		return mapr->collider.move(stats.pos.x, stats.pos.y, stats.forced_speed.x, stats.forced_speed.y, stats.movement_type, stats.hero);

	if (stats.effects.speed == 0) return false;

	float speed = stats.speed * speedMultiplyer[stats.direction] * stats.effects.speed / 100;
	float dx = speed * directionDeltaX[stats.direction];
	float dy = speed * directionDeltaY[stats.direction];

	bool full_move = mapr->collider.move(stats.pos.x, stats.pos.y, dx, dy, stats.movement_type, stats.hero);

	return full_move;
}

/**
 * Whenever a hazard collides with an entity, this function resolves the effect
 * Called by HazardManager
 *
 * Returns false on miss
 */
bool Entity::takeHit(const Hazard &h) {

	//check if this enemy should be affected by this hazard based on the category
	if(!powers->powers[h.power_index].target_categories.empty() && !stats.hero) {
		//the power has a target category requirement, so if it doesnt match, dont continue
		bool match_found = false;
		for (unsigned int i=0; i<stats.categories.size(); i++) {
			if(std::find(powers->powers[h.power_index].target_categories.begin(), powers->powers[h.power_index].target_categories.end(), stats.categories[i]) != powers->powers[h.power_index].target_categories.end()) {
				match_found = true;
			}
		}
		if(!match_found)
			return false;
	}

	//if the target is already dead, they cannot be hit
	if ((stats.cur_state == ENEMY_DEAD || stats.cur_state == ENEMY_CRITDEAD) && !stats.hero)
		return false;

	if(stats.cur_state == AVATAR_DEAD && stats.hero)
		return false;

	//if the target is an enemy and they are not already in combat, activate a beacon to draw other enemies into battle
	if (!stats.in_combat && !stats.hero && !stats.hero_ally) {
		stats.join_combat = true;
		stats.in_combat = true;
		powers->activate(stats.power_index[BEACON], &stats, stats.pos); //emit beacon
	}

	// exit if it was a beacon (to prevent stats.targeted from being set)
	if (powers->powers[h.power_index].beacon) return false;

	// prepare the combat text
	CombatText *combat_text = comb;

	// if it's a miss, do nothing
	int accuracy = h.accuracy;
	if(powers->powers[h.power_index].mod_accuracy_mode == STAT_MODIFIER_MODE_MULTIPLY)
		accuracy = accuracy * powers->powers[h.power_index].mod_accuracy_value / 100;
	else if(powers->powers[h.power_index].mod_accuracy_mode == STAT_MODIFIER_MODE_ADD)
		accuracy += powers->powers[h.power_index].mod_accuracy_value;
	else if(powers->powers[h.power_index].mod_accuracy_mode == STAT_MODIFIER_MODE_ABSOLUTE)
		accuracy = powers->powers[h.power_index].mod_accuracy_value;

	int avoidance = 0;
	if(!powers->powers[h.power_index].trait_avoidance_ignore) {
		avoidance = stats.get(STAT_AVOIDANCE);
		if (stats.effects.triggered_block) avoidance *= 2;
	}

	int true_avoidance = 100 - (accuracy + 25 - avoidance);
	//if we are using an absolute accuracy, offset the constant 25 added to the accuracy
	if(powers->powers[h.power_index].mod_accuracy_mode == STAT_MODIFIER_MODE_ABSOLUTE)
		true_avoidance += 25;
	clampFloor(true_avoidance, MIN_AVOIDANCE);
	clampCeil(true_avoidance, MAX_AVOIDANCE);

	if (percentChance(true_avoidance)) {
		combat_text->addMessage(msg->get("miss"), stats.pos, COMBAT_MESSAGE_MISS);
		return false;
	}

	// calculate base damage
	int dmg = randBetween(h.dmg_min, h.dmg_max);

	if(powers->powers[h.power_index].mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY)
		dmg = dmg * powers->powers[h.power_index].mod_damage_value_min / 100;
	else if(powers->powers[h.power_index].mod_damage_mode == STAT_MODIFIER_MODE_ADD)
		dmg += powers->powers[h.power_index].mod_damage_value_min;
	else if(powers->powers[h.power_index].mod_damage_mode == STAT_MODIFIER_MODE_ABSOLUTE)
		dmg = randBetween(powers->powers[h.power_index].mod_damage_value_min, powers->powers[h.power_index].mod_damage_value_max);

	// apply elemental resistance
	if (h.trait_elemental >= 0 && unsigned(h.trait_elemental) < stats.vulnerable.size()) {
		unsigned i = h.trait_elemental;
		int vulnerable = stats.vulnerable[i];
		clampFloor(vulnerable,MIN_RESIST);
		if (stats.vulnerable[i] < 100)
			clampCeil(vulnerable,MAX_RESIST);
		dmg = (dmg * vulnerable) / 100;
	}

	if (!h.trait_armor_penetration) { // armor penetration ignores all absorption
		// substract absorption from armor
		int absorption = randBetween(stats.get(STAT_ABS_MIN), stats.get(STAT_ABS_MAX));

		if (stats.effects.triggered_block) {
			absorption += absorption + stats.get(STAT_ABS_MAX); // blocking doubles your absorb amount
		}

		if (absorption > 0 && dmg > 0) {
			int abs = absorption;
			if ((abs*100)/dmg < MIN_BLOCK)
				absorption = (dmg * MIN_BLOCK) /100;
			if ((abs*100)/dmg > MAX_BLOCK)
				absorption = (dmg * MAX_BLOCK) /100;
			if ((abs*100)/dmg < MIN_ABSORB && !stats.effects.triggered_block)
				absorption = (dmg * MIN_ABSORB) /100;
			if ((abs*100)/dmg > MAX_ABSORB && !stats.effects.triggered_block)
				absorption = (dmg * MAX_ABSORB) /100;

			// Sometimes, the absorb limits cause absorbtion to drop to 1
			// This could be confusing to a player that has something with an absorb of 1 equipped
			// So we round absorption up in this case
			if (absorption == 0) absorption = 1;
		}

		dmg = dmg - absorption;
		if (dmg <= 0) {
			dmg = 0;
			if (h.trait_elemental < 0) {
				if (stats.effects.triggered_block && MAX_BLOCK < 100) dmg = 1;
				else if (!stats.effects.triggered_block && MAX_ABSORB < 100) dmg = 1;
			}
			else {
				if (MAX_RESIST < 100) dmg = 1;
			}
			sfx_block = true;
			if (activeAnimation->getName() == "block")
				resetActiveAnimation();
		}
	}

	// check for crits
	int true_crit_chance = h.crit_chance;

	if(powers->powers[h.power_index].mod_crit_mode == STAT_MODIFIER_MODE_MULTIPLY)
		true_crit_chance = true_crit_chance * powers->powers[h.power_index].mod_crit_value / 100;
	else if(powers->powers[h.power_index].mod_crit_mode == STAT_MODIFIER_MODE_ADD)
		true_crit_chance += powers->powers[h.power_index].mod_crit_value;
	else if(powers->powers[h.power_index].mod_crit_mode == STAT_MODIFIER_MODE_ABSOLUTE)
		true_crit_chance = powers->powers[h.power_index].mod_crit_value;

	if (stats.effects.stun || stats.effects.speed < 100)
		true_crit_chance += h.trait_crits_impaired;

	bool crit = percentChance(true_crit_chance);
	if (crit) {
		dmg = dmg + h.dmg_max;
		if(!stats.hero)
			mapr->shaky_cam_ticks = MAX_FRAMES_PER_SEC/2;
	}

	if(stats.hero)
		combat_text->addMessage(dmg, stats.pos, COMBAT_MESSAGE_TAKEDMG);
	else {
		if(crit)
			combat_text->addMessage(dmg, stats.pos, COMBAT_MESSAGE_CRIT);
		else
			combat_text->addMessage(dmg, stats.pos, COMBAT_MESSAGE_GIVEDMG);
	}

	// temporarily save the current HP for calculating HP/MP steal on final blow
	int prev_hp = stats.hp;

	// apply damage
	stats.takeDamage(dmg);

	// after effects
	if (dmg > 0) {

		// damage always breaks stun
		stats.effects.removeEffectType("stun");

		if (stats.hp > 0) {
			if (h.mod_power > 0) powers->effect(&stats, h.src_stats, h.mod_power,h.source_type);
			powers->effect(&stats, h.src_stats, h.power_index,h.source_type);
		}

		if (!stats.effects.immunity) {
			if (stats.effects.forced_move && stats.hp > 0) {
				float theta = calcTheta(h.src_stats->pos.x, h.src_stats->pos.y, stats.pos.x, stats.pos.y);
				stats.forced_speed.x = stats.effects.forced_speed * cos(theta);
				stats.forced_speed.y = stats.effects.forced_speed * sin(theta);
			}
			if (h.hp_steal != 0) {
				int steal_amt = (min(dmg, prev_hp) * h.hp_steal) / 100;
				if (steal_amt == 0) steal_amt = 1;
				combat_text->addMessage(msg->get("+%d HP",steal_amt), h.src_stats->pos, COMBAT_MESSAGE_BUFF);
				h.src_stats->hp = min(h.src_stats->hp + steal_amt, h.src_stats->get(STAT_HP_MAX));
			}
			if (h.mp_steal != 0) {
				int steal_amt = (min(dmg, prev_hp) * h.mp_steal) / 100;
				if (steal_amt == 0) steal_amt = 1;
				combat_text->addMessage(msg->get("+%d MP",steal_amt), h.src_stats->pos, COMBAT_MESSAGE_BUFF);
				h.src_stats->mp = min(h.src_stats->mp + steal_amt, h.src_stats->get(STAT_MP_MAX));
			}
		}
	}

	// post effect power
	if (h.post_power > 0 && dmg > 0) {
		powers->activate(h.post_power, h.src_stats, stats.pos);
	}

	// interrupted to new state
	if (dmg > 0) {
		bool chance_poise = percentChance(stats.get(STAT_POISE));

		if(stats.hp <= 0) {
			stats.effects.triggered_death = true;
			if(stats.hero)
				stats.cur_state = AVATAR_DEAD;
			else {
				doRewards(h.source_type);
				if (crit)
					stats.cur_state = ENEMY_CRITDEAD;
				else
					stats.cur_state = ENEMY_DEAD;
				mapr->collider.unblock(stats.pos.x,stats.pos.y);
			}
		}
		// don't go through a hit animation if stunned
		else if (!stats.effects.stun && !chance_poise) {
			sfx_hit = true;

			if(!chance_poise && stats.cooldown_hit_ticks == 0) {
				if(stats.hero)
					stats.cur_state = AVATAR_HIT;
				else
					stats.cur_state = ENEMY_HIT;
				stats.cooldown_hit_ticks = stats.cooldown_hit;
			}
			// roll to see if the enemy's ON_HIT power is casted
			if (percentChance(stats.power_chance[ON_HIT])) {
				powers->activate(stats.power_index[ON_HIT], &stats, stats.pos);
			}
		}
		// just play the hit sound
		else
			sfx_hit = true;
	}

	return true;
}

void Entity::resetActiveAnimation() {
	activeAnimation->reset();
}

/**
 * Set the entity's current animation by name
 */
bool Entity::setAnimation(const string& animationName) {

	// if the animation is already the requested one do nothing
	if (activeAnimation != NULL && activeAnimation->getName() == animationName)
		return true;

	delete activeAnimation;
	activeAnimation = animationSet->getAnimation(animationName);

	if (activeAnimation == NULL)
		fprintf(stderr, "Entity::setAnimation(%s): not found\n", animationName.c_str());

	return activeAnimation == NULL;
}

Entity::~Entity () {

	delete activeAnimation;
}

