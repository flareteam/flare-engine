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
const float speedMultiplyer[8] = { static_cast<float>(1.0/M_SQRT2), 1.0f, static_cast<float>(1.0/M_SQRT2), 1.0f, static_cast<float>(1.0/M_SQRT2), 1.0f, static_cast<float>(1.0/M_SQRT2), 1.0f};

Entity::Entity()
	: sprites(NULL)
	, sound_melee(0)
	, sound_mental(0)
	, sound_hit(0)
	, sound_die(0)
	, sound_critdie(0)
	, sound_block(0)
	, sound_levelup(0)
	, play_sfx_phys(false)
	, play_sfx_ment(false)
	, play_sfx_hit(false)
	, play_sfx_die(false)
	, play_sfx_critdie(false)
	, play_sfx_block(false)
	, activeAnimation(NULL)
	, animationSet(NULL) {
}

Entity::Entity(const Entity &e)
	: sprites(e.sprites)
	, sound_melee(e.sound_melee)
	, sound_mental(e.sound_mental)
	, sound_hit(e.sound_hit)
	, sound_die(e.sound_die)
	, sound_critdie(e.sound_critdie)
	, sound_block(e.sound_block)
	, sound_levelup(e.sound_levelup)
	, play_sfx_phys(e.play_sfx_phys)
	, play_sfx_ment(e.play_sfx_ment)
	, play_sfx_hit(e.play_sfx_hit)
	, play_sfx_die(e.play_sfx_die)
	, play_sfx_critdie(e.play_sfx_critdie)
	, play_sfx_block(e.play_sfx_block)
	, activeAnimation(new Animation(*e.activeAnimation))
	, animationSet(e.animationSet)
	, stats(StatBlock(e.stats)) {
}

void Entity::loadSounds(StatBlock *src_stats) {
	snd->unload(sound_melee);
	snd->unload(sound_mental);
	snd->unload(sound_hit);
	snd->unload(sound_die);
	snd->unload(sound_critdie);
	snd->unload(sound_block);
	snd->unload(sound_levelup);

	if (!src_stats) src_stats = &stats;

	if (src_stats->sfx_phys != "")
		sound_melee = snd->load(src_stats->sfx_phys, "Entity melee attack");
	if (src_stats->sfx_ment != "")
		sound_mental = snd->load(src_stats->sfx_ment, "Entity mental attack");
	if (src_stats->sfx_hit != "")
		sound_hit = snd->load(src_stats->sfx_hit, "Entity was hit");
	if (src_stats->sfx_die != "")
		sound_die = snd->load(src_stats->sfx_die, "Entity died");
	if (src_stats->sfx_critdie != "")
		sound_critdie = snd->load(src_stats->sfx_critdie, "Entity died from critial hit");
	if (src_stats->sfx_block != "")
		sound_block = snd->load(src_stats->sfx_block, "Entity blocked");
	if (src_stats->sfx_levelup != "")
		sound_levelup = snd->load(src_stats->sfx_levelup, "Entity leveled up");
}

void Entity::unloadSounds() {
	snd->unload(sound_melee);
	snd->unload(sound_mental);
	snd->unload(sound_hit);
	snd->unload(sound_die);
	snd->unload(sound_critdie);
	snd->unload(sound_block);
	snd->unload(sound_levelup);
}


void Entity::move_from_offending_tile() {

	// If we got stuck on a tile, which we're not allowed to be on, move away
	// This is just a workaround as we cannot reproduce being stuck easily nor find the
	// errornous code, check https://github.com/clintbellanger/flare-engine/issues/1058

	// As this method should do nothing while regular gameplay, but only in case of bugs
	// we don't need to care about nice graphical effects, so we may just jump out of the
	// offending tile. The idea is simple: We can only be stuck on a tile by accident,
	// so we got here somehow. We'll try to push this entity to the nearest valid place
	while (!mapr->collider.is_valid_position(stats.pos.x, stats.pos.y, stats.movement_type, stats.hero)) {
		logError("Entity: %s got stuck on an invalid tile. Please report this bug, if you're able to reproduce it!",
				stats.hero ? "The hero" : "An entity");

		float pushx = 0;
		float pushy = 0;

		if (mapr->collider.is_valid_position(stats.pos.x + 1, stats.pos.y, stats.movement_type, stats.hero))
			pushx += 0.1f * (2 - (static_cast<float>(static_cast<int>(stats.pos.x + 1)) + 0.5f - stats.pos.x));

		if (mapr->collider.is_valid_position(stats.pos.x - 1, stats.pos.y, stats.movement_type, stats.hero))
			pushx -= 0.1f * (2 - (stats.pos.x - (static_cast<float>(static_cast<int>(stats.pos.x - 1)) + 0.5f)));

		if (mapr->collider.is_valid_position(stats.pos.x, stats.pos.y + 1, stats.movement_type, stats.hero))
			pushy += 0.1f * (2 - (static_cast<float>(static_cast<int>(stats.pos.y + 1)) + 0.5f - stats.pos.y));

		if (mapr->collider.is_valid_position(stats.pos.x, stats.pos.y- 1, stats.movement_type, stats.hero))
			pushy -= 0.1f * (2 - (stats.pos.y - (static_cast<float>(static_cast<int>(stats.pos.y - 1)) + 0.5f)));

		stats.pos.x += pushx;
		stats.pos.y += pushy;

		// we don't move, but we're still stuck on an invalid tile,
		// the final life saver before being crushed by an invalid tile:
		// just blink away. This will seriously irritate the player, but there
		// is probably no other easy way to repair the game
		if (pushx == 0 && pushy == 0) {
			stats.pos.x = static_cast<float>(randBetween(1, mapr->w-1)) + 0.5f;
			stats.pos.y = static_cast<float>(randBetween(1, mapr->h-1)) + 0.5f;
			logError("Entity: %s got stuck on an invalid tile. Please report this bug, if you're able to reproduce it!",
					stats.hero ? "The hero" : "An entity");
		}
	}
}

/**
 * move()
 * Apply speed to the direction faced.
 *
 * @return Returns false if wall collision, otherwise true.
 */
bool Entity::move() {

	move_from_offending_tile();

	if (stats.effects.knockback_speed != 0)
		return false;

	if (stats.effects.stun || stats.effects.speed == 0) return false;

	float speed = stats.speed * speedMultiplyer[stats.direction] * stats.effects.speed / 100;
	float dx = speed * static_cast<float>(directionDeltaX[stats.direction]);
	float dy = speed * static_cast<float>(directionDeltaY[stats.direction]);

	bool full_move = mapr->collider.move(stats.pos.x, stats.pos.y, dx, dy, stats.movement_type, stats.hero);

	return full_move;
}

/**
 * Whenever a hazard collides with an entity, this function resolves the effect
 * Called by HazardManager
 *
 * Returns false on miss
 */
bool Entity::takeHit(Hazard &h) {

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
	}

	int true_avoidance = 100 - (accuracy + 25 - avoidance);
	//if we are using an absolute accuracy, offset the constant 25 added to the accuracy
	if(powers->powers[h.power_index].mod_accuracy_mode == STAT_MODIFIER_MODE_ABSOLUTE)
		true_avoidance += 25;
	clampFloor(true_avoidance, MIN_AVOIDANCE);
	clampCeil(true_avoidance, MAX_AVOIDANCE);

	if (h.missile && percentChance(stats.get(STAT_REFLECT))) {
		// reflect the missile 180 degrees
		h.setAngle(h.angle+static_cast<float>(M_PI));

		// change hazard source to match the reflector's type
		// maybe we should change the source stats pointer to the reflector's StatBlock
		if (h.source_type == SOURCE_TYPE_HERO || h.source_type == SOURCE_TYPE_ALLY)
			h.source_type = SOURCE_TYPE_ENEMY;
		else if (h.source_type == SOURCE_TYPE_ENEMY)
			h.source_type = stats.hero ? SOURCE_TYPE_HERO : SOURCE_TYPE_ALLY;

		// reset the hazard ticks
		h.lifespan = h.base_lifespan;

		if (activeAnimation->getName() == "block") {
			play_sfx_block = true;
		}

		return false;
	}

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
		// subtract absorption from armor
		int absorption = randBetween(stats.get(STAT_ABS_MIN), stats.get(STAT_ABS_MAX));

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
			if (activeAnimation->getName() == "block") {
				play_sfx_block = true;
				resetActiveAnimation();
			}
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
		dmg *= 2;
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

	// save debuff status to check for on_debuff powers later
	bool was_debuffed = stats.effects.isDebuffed();

	// apply damage
	stats.takeDamage(dmg);

	// after effects
	if (dmg > 0) {

		// damage always breaks stun
		stats.effects.removeEffectType(EFFECT_STUN);

		if (stats.hp > 0) {
			powers->effect(&stats, h.src_stats, h.power_index,h.source_type);
		}

		if (!stats.effects.immunity) {
			if (h.hp_steal != 0) {
				int steal_amt = (std::min(dmg, prev_hp) * h.hp_steal) / 100;
				if (steal_amt == 0) steal_amt = 1;
				combat_text->addMessage(msg->get("+%d HP",steal_amt), h.src_stats->pos, COMBAT_MESSAGE_BUFF);
				h.src_stats->hp = std::min(h.src_stats->hp + steal_amt, h.src_stats->get(STAT_HP_MAX));
			}
			if (h.mp_steal != 0) {
				int steal_amt = (std::min(dmg, prev_hp) * h.mp_steal) / 100;
				if (steal_amt == 0) steal_amt = 1;
				combat_text->addMessage(msg->get("+%d MP",steal_amt), h.src_stats->pos, COMBAT_MESSAGE_BUFF);
				h.src_stats->mp = std::min(h.src_stats->mp + steal_amt, h.src_stats->get(STAT_MP_MAX));
			}
		}
	}

	// post effect power
	if (h.post_power > 0 && dmg > 0) {
		powers->activate(h.post_power, h.src_stats, stats.pos);
	}

	// loot
	if (dmg > 0 && !h.loot.empty()) {
		for (unsigned i=0; i<h.loot.size(); i++) {
			powers->loot.push_back(h.loot[i]);
			powers->loot.back().x = static_cast<int>(stats.pos.x);
			powers->loot.back().y = static_cast<int>(stats.pos.y);
		}
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

			return true;
		}

		// play hit sound effect
		play_sfx_hit = true;

		// if this hit caused a debuff, activate an on_debuff power
		if (!was_debuffed && stats.effects.isDebuffed()) {
			AIPower* ai_power = stats.getAIPower(AI_POWER_DEBUFF);
			if (ai_power != NULL) {
				stats.cur_state = ENEMY_POWER;
				stats.activated_power = ai_power;
				stats.cooldown_ticks = 0; // ignore global cooldown
				return true;
			}
		}

		// roll to see if the enemy's ON_HIT power is casted
		AIPower* ai_power = stats.getAIPower(AI_POWER_HIT);
		if (ai_power != NULL) {
			stats.cur_state = ENEMY_POWER;
			stats.activated_power = ai_power;
			stats.cooldown_ticks = 0; // ignore global cooldown
			return true;
		}

		// don't go through a hit animation if stunned or successfully poised
		// however, critical hits ignore poise
		if (!stats.effects.stun && (!chance_poise || crit)) {
			if(stats.cooldown_hit_ticks == 0) {
				if(stats.hero)
					stats.cur_state = AVATAR_HIT;
				else
					stats.cur_state = ENEMY_HIT;
				stats.cooldown_hit_ticks = stats.cooldown_hit;

				if (stats.untransform_on_hit)
					stats.transform_duration = 0;
			}
		}
	}

	return true;
}

void Entity::resetActiveAnimation() {
	activeAnimation->reset();
}

/**
 * Set the entity's current animation by name
 */
bool Entity::setAnimation(const std::string& animationName) {

	// if the animation is already the requested one do nothing
	if (activeAnimation != NULL && activeAnimation->getName() == animationName)
		return true;

	delete activeAnimation;
	activeAnimation = animationSet->getAnimation(animationName);

	if (activeAnimation == NULL)
		logError("Entity::setAnimation(%s): not found", animationName.c_str());

	return activeAnimation == NULL;
}

Entity::~Entity () {
	delete activeAnimation;
}

