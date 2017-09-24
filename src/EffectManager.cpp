/*
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

/**
 * class EffectManager
 */

#include "Animation.h"
#include "AnimationSet.h"
#include "EffectManager.h"
#include "Settings.h"

EffectManager::EffectManager()
	: bonus(std::vector<int>(STAT_COUNT + DAMAGE_TYPES_COUNT, 0))
	, bonus_resist(std::vector<int>(ELEMENTS.size(), 0))
	, bonus_primary(std::vector<int>(PRIMARY_STATS.size(), 0))
	, triggered_others(false)
	, triggered_block(false)
	, triggered_hit(false)
	, triggered_halfdeath(false)
	, triggered_joincombat(false)
	, triggered_death(false)
	, refresh_stats(false) {
	clearStatus();
}

EffectManager::~EffectManager() {
	for (size_t i=0; i<effect_list.size(); i++) {
		removeAnimation(i);
	}
}

EffectManager& EffectManager::operator= (const EffectManager &emSource) {
	effect_list.resize(emSource.effect_list.size());

	for (unsigned i=0; i<effect_list.size(); i++) {
		effect_list[i].id = emSource.effect_list[i].id;
		effect_list[i].name = emSource.effect_list[i].name;
		effect_list[i].icon = emSource.effect_list[i].icon;
		effect_list[i].ticks = emSource.effect_list[i].ticks;
		effect_list[i].duration = emSource.effect_list[i].duration;
		effect_list[i].type = emSource.effect_list[i].type;
		effect_list[i].magnitude = emSource.effect_list[i].magnitude;
		effect_list[i].magnitude_max = emSource.effect_list[i].magnitude_max;
		effect_list[i].item = emSource.effect_list[i].item;
		effect_list[i].trigger = emSource.effect_list[i].trigger;
		effect_list[i].render_above = emSource.effect_list[i].render_above;
		effect_list[i].passive_id = emSource.effect_list[i].passive_id;
		effect_list[i].source_type = emSource.effect_list[i].source_type;
		effect_list[i].group_stack = emSource.effect_list[i].group_stack;
		effect_list[i].color_mod = emSource.effect_list[i].color_mod;
		effect_list[i].alpha_mod = emSource.effect_list[i].alpha_mod;
		effect_list[i].attack_speed_anim = emSource.effect_list[i].attack_speed_anim;

		if (emSource.effect_list[i].animation_name != "") {
			effect_list[i].animation_name = emSource.effect_list[i].animation_name;
			anim->increaseCount(effect_list[i].animation_name);
			effect_list[i].animation = loadAnimation(effect_list[i].animation_name);
		}
	}
	damage = emSource.damage;
	damage_percent = emSource.damage_percent;
	hpot = emSource.hpot;
	hpot_percent = emSource.hpot_percent;
	mpot = emSource.mpot;
	mpot_percent = emSource.mpot_percent;
	speed = emSource.speed;
	immunity_damage = emSource.immunity_damage;
	immunity_slow = emSource.immunity_slow;
	immunity_stun = emSource.immunity_stun;
	immunity_hp_steal = emSource.immunity_hp_steal;
	immunity_mp_steal = emSource.immunity_mp_steal;
	immunity_knockback = emSource.immunity_knockback;
	immunity_damage_reflect = emSource.immunity_damage_reflect;
	immunity_stat_debuff = emSource.immunity_stat_debuff;
	stun = emSource.stun;
	revive = emSource.revive;
	convert = emSource.convert;
	death_sentence = emSource.death_sentence;
	fear = emSource.fear;
	knockback_speed = emSource.knockback_speed;

	for (size_t i=0; i<emSource.bonus.size(); i++) {
		bonus[i] = emSource.bonus[i];
	}
	for (size_t i=0; i<emSource.bonus_resist.size(); i++) {
		bonus_resist[i] = emSource.bonus_resist[i];
	}
	for (size_t i=0; i<emSource.bonus_primary.size(); i++) {
		bonus_primary[i] = emSource.bonus_primary[i];
	}

	triggered_others = emSource.triggered_others;
	triggered_block = emSource.triggered_block;
	triggered_hit = emSource.triggered_hit;
	triggered_halfdeath = emSource.triggered_halfdeath;
	triggered_joincombat = emSource.triggered_joincombat;
	triggered_death = emSource.triggered_death;
	refresh_stats = emSource.refresh_stats;

	return *this;
}

void EffectManager::clearStatus() {
	damage = 0;
	damage_percent = 0;
	hpot = 0;
	hpot_percent = 0;
	mpot = 0;
	mpot_percent = 0;
	speed = 100;
	immunity_damage = false;
	immunity_slow = false;
	immunity_stun = false;
	immunity_hp_steal = false;
	immunity_mp_steal = false;
	immunity_knockback = false;
	immunity_damage_reflect = false;
	immunity_stat_debuff = false;
	stun = false;
	revive = false;
	convert = false;
	death_sentence = false;
	fear = false;
	knockback_speed = 0;

	for (unsigned i=0; i<STAT_COUNT + DAMAGE_TYPES_COUNT; i++) {
		bonus[i] = 0;
	}

	for (unsigned i=0; i<bonus_resist.size(); i++) {
		bonus_resist[i] = 0;
	}

	for (unsigned i=0; i<bonus_primary.size(); i++) {
		bonus_primary[i] = 0;
	}
}

void EffectManager::logic() {
	clearStatus();

	for (unsigned i=0; i<effect_list.size(); i++) {
		// @CLASS EffectManager|Description of "type" in powers/effects.txt
		// expire timed effects and total up magnitudes of active effects
		if (effect_list[i].duration >= 0) {
			if (effect_list[i].duration > 0) {
				if (effect_list[i].ticks > 0) effect_list[i].ticks--;
				if (effect_list[i].ticks == 0) {
					//death sentence is only applied at the end of the timer
					// @TYPE death_sentence|Causes sudden death at the end of the effect duration.
					if (effect_list[i].type == EFFECT_DEATH_SENTENCE) death_sentence = true;
					removeEffect(i);
					i--;
					continue;
				}
			}

			// @TYPE damage|Damage per second
			if (effect_list[i].type == EFFECT_DAMAGE && effect_list[i].ticks % MAX_FRAMES_PER_SEC == 1) damage += effect_list[i].magnitude;
			// @TYPE damage_percent|Damage per second (percentage of max HP)
			else if (effect_list[i].type == EFFECT_DAMAGE_PERCENT && effect_list[i].ticks % MAX_FRAMES_PER_SEC == 1) damage_percent += effect_list[i].magnitude;
			// @TYPE hpot|HP restored per second
			else if (effect_list[i].type == EFFECT_HPOT && effect_list[i].ticks % MAX_FRAMES_PER_SEC == 1) hpot += effect_list[i].magnitude;
			// @TYPE hpot_percent|HP restored per second (percentage of max HP)
			else if (effect_list[i].type == EFFECT_HPOT_PERCENT && effect_list[i].ticks % MAX_FRAMES_PER_SEC == 1) hpot_percent += effect_list[i].magnitude;
			// @TYPE mpot|MP restored per second
			else if (effect_list[i].type == EFFECT_MPOT && effect_list[i].ticks % MAX_FRAMES_PER_SEC == 1) mpot += effect_list[i].magnitude;
			// @TYPE mpot_percent|MP restored per second (percentage of max MP)
			else if (effect_list[i].type == EFFECT_MPOT_PERCENT && effect_list[i].ticks % MAX_FRAMES_PER_SEC == 1) mpot_percent += effect_list[i].magnitude;
			// @TYPE speed|Changes movement speed. A magnitude of 100 is 100% speed (aka normal speed).
			else if (effect_list[i].type == EFFECT_SPEED) speed = (static_cast<float>(effect_list[i].magnitude) * speed) / 100.f;
			// @TYPE attack_speed|Changes attack speed. A magnitude of 100 is 100% speed (aka normal speed).
			// attack speed is calculated when getAttackSpeed() is called

			// @TYPE immunity|Applies all immunity effects. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY) {
				immunity_damage = true;
				immunity_slow = true;
				immunity_stun = true;
				immunity_hp_steal = true;
				immunity_mp_steal = true;
				immunity_knockback = true;
				immunity_damage_reflect = true;
				immunity_stat_debuff = true;
			}
			// @TYPE immunity_damage|Removes and prevents damage over time. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_DAMAGE) immunity_damage = true;
			// @TYPE immunity_slow|Removes and prevents slow effects. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_SLOW) immunity_slow = true;
			// @TYPE immunity_stun|Removes and prevents stun effects. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_STUN) immunity_stun = true;
			// @TYPE immunity_hp_steal|Prevents HP stealing. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_HP_STEAL) immunity_hp_steal = true;
			// @TYPE immunity_mp_steal|Prevents MP stealing. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_MP_STEAL) immunity_mp_steal = true;
			// @TYPE immunity_knockback|Removes and prevents knockback effects. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_KNOCKBACK) immunity_knockback = true;
			// @TYPE immunity_damage_reflect|Prevents damage reflection. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_DAMAGE_REFLECT) immunity_damage_reflect = true;
			// @TYPE immunity_stat_debuff|Prevents stat value altering effects that have a magnitude less than 0. Magnitude is ignored.
			else if (effect_list[i].type == EFFECT_IMMUNITY_STAT_DEBUFF) immunity_stat_debuff = true;

			// @TYPE stun|Can't move or attack. Being attacked breaks stun.
			else if (effect_list[i].type == EFFECT_STUN) stun = true;
			// @TYPE revive|Revives the player. Typically attached to a power that triggers when the player dies.
			else if (effect_list[i].type == EFFECT_REVIVE) revive = true;
			// @TYPE convert|Causes an enemy or an ally to switch allegiance
			else if (effect_list[i].type == EFFECT_CONVERT) convert = true;
			// @TYPE fear|Causes enemies to run away
			else if (effect_list[i].type == EFFECT_FEAR) fear = true;
			// @TYPE knockback|Pushes the target away from the source caster. Speed is the given value divided by the framerate cap.
			else if (effect_list[i].type == EFFECT_KNOCKBACK) knockback_speed = static_cast<float>(effect_list[i].magnitude)/static_cast<float>(MAX_FRAMES_PER_SEC);

			// @TYPE ${STATNAME}|Increases ${STATNAME}, where ${STATNAME} is any of the base stats. Examples: hp, avoidance, xp_gain
			// @TYPE ${DAMAGE_TYPE}|Increases a damage min or max, where ${DAMAGE_TYPE} is any 'min' or 'max' value found in engine/damage_types.txt. Example: dmg_melee_min
			else if (effect_list[i].type >= EFFECT_COUNT && effect_list[i].type < EFFECT_COUNT + STAT_COUNT + static_cast<int>(DAMAGE_TYPES_COUNT)) {
				bonus[effect_list[i].type - EFFECT_COUNT] += effect_list[i].magnitude;
			}
			// else if (effect_list[i].type >= EFFECT_COUNT + STAT_COUNT && effect_list[i].type < EFFECT_COUNT + STAT_COUNT + static_cast<int>(DAMAGE_TYPES_COUNT)) {
			// 	bonus[effect_list[i].type - EFFECT_COUNT] += effect_list[i].magnitude;
			// }
			// @TYPE ${ELEMENT}_resist|Increase Resistance % to ${ELEMENT}, where ${ELEMENT} is any found in engine/elements.txt. Example: fire_resist
			else if (effect_list[i].type >= EFFECT_COUNT + STAT_COUNT + static_cast<int>(DAMAGE_TYPES_COUNT) && effect_list[i].type < EFFECT_COUNT + STAT_COUNT + static_cast<int>(DAMAGE_TYPES_COUNT) + static_cast<int>(ELEMENTS.size())) {
				bonus_resist[effect_list[i].type - EFFECT_COUNT - STAT_COUNT - DAMAGE_TYPES_COUNT] += effect_list[i].magnitude;
			}
			// @TYPE ${PRIMARYSTAT}|Increases ${PRIMARYSTAT}, where ${PRIMARYSTAT} is any of the primary stats defined in engine/primary_stats.txt. Example: physical
			else if (effect_list[i].type >= EFFECT_COUNT) {
				bonus_primary[effect_list[i].type - EFFECT_COUNT - STAT_COUNT - DAMAGE_TYPES_COUNT - ELEMENTS.size()] += effect_list[i].magnitude;
			}
		}
		// expire shield effects
		if (effect_list[i].magnitude_max > 0 && effect_list[i].magnitude == 0) {
			// @TYPE shield|Create a damage absorbing barrier based on Mental damage stat. Duration is ignored.
			if (effect_list[i].type == EFFECT_SHIELD) {
				removeEffect(i);
				i--;
				continue;
			}
		}
		// expire effects based on animations
		if ((effect_list[i].animation && effect_list[i].animation->isLastFrame()) || !effect_list[i].animation) {
			// @TYPE heal|Restore HP based on Mental damage stat.
			if (effect_list[i].type == EFFECT_HEAL) {
				removeEffect(i);
				i--;
				continue;
			}
		}

		// animate
		if (effect_list[i].animation) {
			if (!effect_list[i].animation->isCompleted())
				effect_list[i].animation->advanceFrame();
		}
	}
}

void EffectManager::addEffect(EffectDef &effect, int duration, int magnitude, bool item, int trigger, int passive_id, int source_type) {
	int effect_type = getType(effect.type);
	refresh_stats = true;

	// if we're already immune, don't add negative effects
	if (immunity_damage && (effect_type == EFFECT_DAMAGE || effect_type == EFFECT_DAMAGE_PERCENT))
		return;
	else if (immunity_slow && effect_type == EFFECT_SPEED && magnitude < 100)
		return;
	else if (immunity_stun && effect_type == EFFECT_STUN)
		return;
	else if (immunity_knockback && effect_type == EFFECT_KNOCKBACK)
		return;
	else if (immunity_stat_debuff && effect_type > EFFECT_COUNT && magnitude < 0)
		return;

	// only allow one knockback effect at a time
	if (effect_type == EFFECT_KNOCKBACK && knockback_speed != 0)
		return;

	if (effect_type == EFFECT_ATTACK_SPEED && magnitude < 100) {
		logInfo("EffectManager: Attack speeds less than 100 are unsupported.");
		return;
	}

	bool insert_effect = false;
	int stacks_applied = 0;
	size_t insert_pos;

	for (size_t i=effect_list.size(); i>0; i--) {
		if (effect_list[i-1].id == effect.id) {
			if (trigger > -1 && effect_list[i-1].trigger == trigger)
				return; // trigger effects can only be cast once per trigger

			if (!effect.can_stack) {
				removeEffect(i-1);
			}
			else{
				if(effect_type == EFFECT_SHIELD && effect.group_stack){
					effect_list[i-1].magnitude += magnitude;

					if(effect.max_stacks == -1
						|| (magnitude != 0 && effect_list[i-1].magnitude_max/magnitude < effect.max_stacks)){
						effect_list[i-1].magnitude_max += magnitude;
					}

					if(effect_list[i-1].magnitude > effect_list[i-1].magnitude_max){
						effect_list[i-1].magnitude = effect_list[i-1].magnitude_max;
					}

					return;
				}

				 if (insert_effect == false) {
					// to keep matching effects together, they are inserted after the most recent matching effect
					// otherwise, they are added to the end of the effect list
					insert_effect = true;
					insert_pos = i;
				}

				stacks_applied++;
			}
		}
		// if we're adding an immunity effect, remove all negative effects
		if (effect_type == EFFECT_IMMUNITY)
			clearNegativeEffects();
		else if (effect_type == EFFECT_IMMUNITY_DAMAGE)
			clearNegativeEffects(EFFECT_IMMUNITY_DAMAGE);
		else if (effect_type == EFFECT_IMMUNITY_SLOW)
			clearNegativeEffects(EFFECT_IMMUNITY_SLOW);
		else if (effect_type == EFFECT_IMMUNITY_STUN)
			clearNegativeEffects(EFFECT_IMMUNITY_STUN);
		else if (effect_type == EFFECT_IMMUNITY_KNOCKBACK)
			clearNegativeEffects(EFFECT_IMMUNITY_KNOCKBACK);
	}

	Effect e;

	e.id = effect.id;
	e.name = effect.name;
	e.icon = effect.icon;
	e.type = effect_type;
	e.render_above = effect.render_above;
	e.group_stack = effect.group_stack;
	e.color_mod = effect.color_mod;
	e.alpha_mod = effect.alpha_mod;
	e.attack_speed_anim = effect.attack_speed_anim;

	if (effect.animation != "") {
		anim->increaseCount(effect.animation);
		e.animation = loadAnimation(effect.animation);
		e.animation_name = effect.animation;
	}

	e.ticks = e.duration = duration;
	e.magnitude = e.magnitude_max = magnitude;
	e.item = item;
	e.trigger = trigger;
	e.passive_id = passive_id;
	e.source_type = source_type;

	if(effect.max_stacks != -1 && stacks_applied >= effect.max_stacks){
		//Remove the oldest effect of the type
		removeEffect(insert_pos-stacks_applied);

		//All elemnts have shiftef to left
		insert_pos--;
	}

	if (insert_effect)
		effect_list.insert(effect_list.begin() + insert_pos, e);
	else
		effect_list.push_back(e);
}

void EffectManager::removeEffect(size_t id) {
	removeAnimation(id);
	effect_list.erase(effect_list.begin()+id);
	refresh_stats = true;
}

void EffectManager::removeAnimation(size_t id) {
	if (effect_list[id].animation && effect_list[id].animation_name != "") {
		anim->decreaseCount(effect_list[id].animation_name);
		delete effect_list[id].animation;
		effect_list[id].animation = NULL;
		effect_list[id].animation_name = "";
	}
}

void EffectManager::removeEffectType(const int type) {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].type == type) removeEffect(i-1);
	}
}

void EffectManager::removeEffectPassive(int id) {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].passive_id == id) removeEffect(i-1);
	}
}

void EffectManager::removeEffectID(const std::vector< std::pair<std::string, int> >& remove_effects) {
	for (size_t i = 0; i < remove_effects.size(); i++) {
		int count = remove_effects[i].second;
		bool remove_all = (count == 0 ? true : false);

		for (size_t j = effect_list.size(); j > 0; j--) {
			if (!remove_all && count <= 0)
				break;

			if (effect_list[j-1].id == remove_effects[i].first) {
				removeEffect(j-1);
				count--;
			}
		}
	}
}

void EffectManager::clearEffects() {
	for (size_t i=effect_list.size(); i > 0; i--) {
		removeEffect(i-1);
	}

	clearStatus();

	// clear triggers
	triggered_others = triggered_block = triggered_hit = triggered_halfdeath = triggered_joincombat = triggered_death = false;
}

void EffectManager::clearNegativeEffects(int type) {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if ((type == -1 || type == EFFECT_IMMUNITY_DAMAGE) && effect_list[i-1].type == EFFECT_DAMAGE)
			removeEffect(i-1);
		else if ((type == -1 || type == EFFECT_IMMUNITY_DAMAGE) && effect_list[i-1].type == EFFECT_DAMAGE_PERCENT)
			removeEffect(i-1);
		else if ((type == -1 || type == EFFECT_IMMUNITY_SLOW) && effect_list[i-1].type == EFFECT_SPEED && effect_list[i-1].magnitude_max < 100)
			removeEffect(i-1);
		else if ((type == -1 || type == EFFECT_IMMUNITY_STUN) && effect_list[i-1].type == EFFECT_STUN)
			removeEffect(i-1);
		else if ((type == -1 || type == EFFECT_IMMUNITY_KNOCKBACK) && effect_list[i-1].type == EFFECT_KNOCKBACK)
			removeEffect(i-1);
		else if ((type == -1 || type == EFFECT_IMMUNITY_STAT_DEBUFF) && effect_list[i-1].type > EFFECT_COUNT && effect_list[i-1].magnitude_max < 0)
			removeEffect(i-1);
	}
}

void EffectManager::clearItemEffects() {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].item) removeEffect(i-1);
	}
}

void EffectManager::clearTriggerEffects(int trigger) {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].trigger > -1 && effect_list[i-1].trigger == trigger) removeEffect(i-1);
	}
}

int EffectManager::damageShields(int dmg) {
	int over_dmg = dmg;

	for (unsigned i=0; i<effect_list.size(); i++) {
		if (effect_list[i].magnitude_max > 0 && effect_list[i].type == EFFECT_SHIELD) {
			effect_list[i].magnitude -= over_dmg;
			if (effect_list[i].magnitude < 0) {
				over_dmg = abs(effect_list[i].magnitude);
				effect_list[i].magnitude = 0;
			}
			else {
				return 0;
			}
		}
	}

	return over_dmg;
}

Animation* EffectManager::loadAnimation(const std::string &s) {
	if (s != "") {
		AnimationSet *animationSet = anim->getAnimationSet(s);
		return animationSet->getAnimation("");
	}
	return NULL;
}

int EffectManager::getType(const std::string& type) {
	if (type.empty()) return EFFECT_NONE;

	if (type == "damage") return EFFECT_DAMAGE;
	else if (type == "damage_percent") return EFFECT_DAMAGE_PERCENT;
	else if (type == "hpot") return EFFECT_HPOT;
	else if (type == "hpot_percent") return EFFECT_HPOT_PERCENT;
	else if (type == "mpot") return EFFECT_MPOT;
	else if (type == "mpot_percent") return EFFECT_MPOT_PERCENT;
	else if (type == "speed") return EFFECT_SPEED;
	else if (type == "attack_speed") return EFFECT_ATTACK_SPEED;
	else if (type == "immunity") return EFFECT_IMMUNITY;
	else if (type == "immunity_damage") return EFFECT_IMMUNITY_DAMAGE;
	else if (type == "immunity_slow") return EFFECT_IMMUNITY_SLOW;
	else if (type == "immunity_stun") return EFFECT_IMMUNITY_STUN;
	else if (type == "immunity_hp_steal") return EFFECT_IMMUNITY_HP_STEAL;
	else if (type == "immunity_mp_steal") return EFFECT_IMMUNITY_MP_STEAL;
	else if (type == "immunity_knockback") return EFFECT_IMMUNITY_KNOCKBACK;
	else if (type == "immunity_damage_reflect") return EFFECT_IMMUNITY_DAMAGE_REFLECT;
	else if (type == "immunity_stat_debuff") return EFFECT_IMMUNITY_STAT_DEBUFF;
	else if (type == "stun") return EFFECT_STUN;
	else if (type == "revive") return EFFECT_REVIVE;
	else if (type == "convert") return EFFECT_CONVERT;
	else if (type == "fear") return EFFECT_FEAR;
	else if (type == "death_sentence") return EFFECT_DEATH_SENTENCE;
	else if (type == "shield") return EFFECT_SHIELD;
	else if (type == "heal") return EFFECT_HEAL;
	else if (type == "knockback") return EFFECT_KNOCKBACK;
	else {
		for (unsigned i=0; i<STAT_COUNT; i++) {
			if (type == STAT_KEY[i]) {
				return EFFECT_COUNT + i;
			}
		}

		for (unsigned i=0; i<DAMAGE_TYPES.size(); i++) {
			if (type == DAMAGE_TYPES[i].min) {
				return EFFECT_COUNT + STAT_COUNT + (i*2);
			}
			else if (type == DAMAGE_TYPES[i].max) {
				return EFFECT_COUNT + STAT_COUNT + (i*2) + 1;
			}
		}

		for (unsigned i=0; i<bonus_resist.size(); i++) {
			if (type == ELEMENTS[i].id + "_resist") {
				return EFFECT_COUNT + STAT_COUNT + static_cast<int>(DAMAGE_TYPES_COUNT) + i;
			}
		}

		for (unsigned i=0; i<bonus_primary.size(); i++) {
			if (type == PRIMARY_STATS[i].id) {
				return EFFECT_COUNT + STAT_COUNT + static_cast<int>(DAMAGE_TYPES_COUNT) + static_cast<int>(ELEMENTS.size()) + i;
			}
		}
	}

	logError("EffectManager: '%s' is not a valid effect type.", type.c_str());
	return EFFECT_NONE;
}

bool EffectManager::isDebuffed() {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].type == EFFECT_DAMAGE) return true;
		else if (effect_list[i-1].type == EFFECT_DAMAGE_PERCENT) return true;
		else if (effect_list[i-1].type == EFFECT_SPEED && effect_list[i-1].magnitude_max < 100) return true;
		else if (effect_list[i-1].type == EFFECT_STUN) return true;
		else if (effect_list[i-1].type == EFFECT_KNOCKBACK) return true;
		else if (effect_list[i-1].type > EFFECT_COUNT && effect_list[i-1].magnitude_max < 0) return true;
	}
	return false;
}

void EffectManager::getCurrentColor(Color& color_mod) {
	Color default_color = color_mod;
	Color no_color = Color(255, 255, 255);

	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].color_mod == no_color)
			continue;

		if (effect_list[i-1].color_mod != default_color) {
			color_mod = effect_list[i-1].color_mod;
			return;
		}
	}
}

void EffectManager::getCurrentAlpha(uint8_t& alpha_mod) {
	uint8_t default_alpha = alpha_mod;
	uint8_t no_alpha = 255;

	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].alpha_mod == no_alpha)
			continue;

		if (effect_list[i-1].alpha_mod != default_alpha) {
			alpha_mod = effect_list[i-1].alpha_mod;
			return;
		}
	}
}

bool EffectManager::hasEffect(const std::string& id, int req_count) {
	if (req_count <= 0)
		return false;

	int count = 0;

	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].id == id)
			count++;
	}

	return count >= req_count;
}

float EffectManager::getAttackSpeed(const std::string& anim_name) {
	float attack_speed = 100;

	for (size_t i = 0; i < effect_list.size(); ++i) {
		if (effect_list[i].type != EFFECT_ATTACK_SPEED)
			continue;

		if (effect_list[i].attack_speed_anim.empty() || (!effect_list[i].attack_speed_anim.empty() && effect_list[i].attack_speed_anim == anim_name)) {
			attack_speed = (static_cast<float>(effect_list[i].magnitude) * attack_speed) / 100.0f;
		}
	}

	return attack_speed;
}
