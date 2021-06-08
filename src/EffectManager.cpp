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
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "EffectManager.h"
#include "EngineSettings.h"
#include "Hazard.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Stats.h"

EffectDef::EffectDef()
	: id("")
	, type(Effect::NONE)
	, name("")
	, icon(-1)
	, animation("")
	, can_stack(true)
	, max_stacks(-1)
	, group_stack(false)
	, render_above(false)
	, color_mod(255, 255, 255)
	, alpha_mod(255)
	, attack_speed_anim("") {
}

Effect::Effect()
	: id("")
	, name("")
	, icon(-1)
	, timer()
	, type(Effect::NONE)
	, magnitude(0)
	, magnitude_max(0)
	, animation_name("")
	, animation(NULL)
	, item(false)
	, trigger(-1)
	, render_above(false)
	, passive_id(0)
	, source_type(Power::SOURCE_TYPE_HERO)
	, group_stack(false)
	, color_mod(Color(255,255,255).encodeRGBA())
	, alpha_mod(255)
	, attack_speed_anim("") {
}

Effect::Effect(const Effect& other) {
	animation = NULL;
	*this = other;
}

Effect& Effect::operator=(const Effect& other) {
	if (this == &other)
		return *this;

	unloadAnimation();
	animation_name = other.animation_name;
	loadAnimation(animation_name);
	if (animation && other.animation)
		animation->syncTo(other.animation);

	id = other.id;
	name = other.name;
	icon = other.icon;
	timer = other.timer;
	type = other.type;
	magnitude = other.magnitude;
	magnitude_max = other.magnitude_max;
	item = other.item;
	trigger = other.trigger;
	render_above = other.render_above;
	passive_id = other.passive_id;
	source_type = other.source_type;
	group_stack = other.group_stack;
	color_mod = other.color_mod;
	alpha_mod = other.alpha_mod;
	attack_speed_anim = other.attack_speed_anim;

	return *this;
}

Effect::~Effect() {
	unloadAnimation();
}

void Effect::loadAnimation(const std::string &s) {
	if (!s.empty()) {
		animation_name = s;
		anim->increaseCount(animation_name);
		AnimationSet *animationSet = anim->getAnimationSet(animation_name);
		animation = animationSet->getAnimation("");
	}
}

void Effect::unloadAnimation() {
	if (animation) {
		if (!animation_name.empty())
			anim->decreaseCount(animation_name);
		delete animation;
		animation = NULL;
	}
}

int Effect::getTypeFromString(const std::string& type_str) {
	if (type_str.empty()) return Effect::NONE;

	if (type_str == "damage") return Effect::DAMAGE;
	else if (type_str == "damage_percent") return Effect::DAMAGE_PERCENT;
	else if (type_str == "hpot") return Effect::HPOT;
	else if (type_str == "hpot_percent") return Effect::HPOT_PERCENT;
	else if (type_str == "mpot") return Effect::MPOT;
	else if (type_str == "mpot_percent") return Effect::MPOT_PERCENT;
	else if (type_str == "speed") return Effect::SPEED;
	else if (type_str == "attack_speed") return Effect::ATTACK_SPEED;
	else if (type_str == "immunity") return Effect::IMMUNITY;
	else if (type_str == "immunity_damage") return Effect::IMMUNITY_DAMAGE;
	else if (type_str == "immunity_slow") return Effect::IMMUNITY_SLOW;
	else if (type_str == "immunity_stun") return Effect::IMMUNITY_STUN;
	else if (type_str == "immunity_hp_steal") return Effect::IMMUNITY_HP_STEAL;
	else if (type_str == "immunity_mp_steal") return Effect::IMMUNITY_MP_STEAL;
	else if (type_str == "immunity_knockback") return Effect::IMMUNITY_KNOCKBACK;
	else if (type_str == "immunity_damage_reflect") return Effect::IMMUNITY_DAMAGE_REFLECT;
	else if (type_str == "immunity_stat_debuff") return Effect::IMMUNITY_STAT_DEBUFF;
	else if (type_str == "stun") return Effect::STUN;
	else if (type_str == "revive") return Effect::REVIVE;
	else if (type_str == "convert") return Effect::CONVERT;
	else if (type_str == "fear") return Effect::FEAR;
	else if (type_str == "death_sentence") return Effect::DEATH_SENTENCE;
	else if (type_str == "shield") return Effect::SHIELD;
	else if (type_str == "heal") return Effect::HEAL;
	else if (type_str == "knockback") return Effect::KNOCKBACK;
	else {
		for (int i=0; i<Stats::COUNT; ++i) {
			if (type_str == Stats::KEY[i]) {
				return Effect::TYPE_COUNT + i;
			}
		}

		for (size_t i=0; i<eset->damage_types.list.size(); ++i) {
			if (type_str == eset->damage_types.list[i].min) {
				return Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(i*2);
			}
			else if (type_str == eset->damage_types.list[i].max) {
				return Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(i*2) + 1;
			}
		}

		for (size_t i=0; i<eset->elements.list.size(); ++i) {
			if (type_str == eset->elements.list[i].id + "_resist") {
				return Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count + i);
			}
		}

		for (size_t i=0; i<eset->primary_stats.list.size(); ++i) {
			if (type_str == eset->primary_stats.list[i].id) {
				return Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) + static_cast<int>(eset->elements.list.size() + i);
			}
		}
	}

	Utils::logError("EffectManager: '%s' is not a valid effect type.", type_str.c_str());
	return Effect::NONE;
}

bool Effect::typeIsStat(int t) {
	return t >= Effect::TYPE_COUNT && t < Effect::TYPE_COUNT + Stats::COUNT;
}

bool Effect::typeIsDmgMin(int t) {
	return t >= Effect::TYPE_COUNT + Stats::COUNT && t < Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) && (t - Stats::COUNT - Effect::TYPE_COUNT) % 2 == 0;
}

bool Effect::typeIsDmgMax(int t) {
	return t >= Effect::TYPE_COUNT + Stats::COUNT && t < Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) && (t - Stats::COUNT - Effect::TYPE_COUNT) % 2 == 1;
}

bool Effect::typeIsResist(int t) {
	return t >= Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) && t < Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) + static_cast<int>(eset->elements.list.size());
}

bool Effect::typeIsPrimary(int t) {
	return t >= Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) + static_cast<int>(eset->elements.list.size()) && t < Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) + static_cast<int>(eset->elements.list.size()) + static_cast<int>(eset->primary_stats.list.size());
}

int Effect::getStatFromType(int t) {
	return t - Effect::TYPE_COUNT;
}

size_t Effect::getDmgFromType(int t) {
	return static_cast<size_t>(t - Effect::TYPE_COUNT - Stats::COUNT);
}

size_t Effect::getResistFromType(int t) {
	return static_cast<size_t>(t - Effect::TYPE_COUNT - Stats::COUNT) - eset->damage_types.count;
}

size_t Effect::getPrimaryFromType(int t) {
	return static_cast<size_t>(t - Effect::TYPE_COUNT - Stats::COUNT) - eset->damage_types.count - eset->elements.list.size();
}

EffectManager::EffectManager()
	: bonus(std::vector<int>(Stats::COUNT + eset->damage_types.count, 0))
	, bonus_resist(std::vector<int>(eset->elements.list.size(), 0))
	, bonus_primary(std::vector<int>(eset->primary_stats.list.size(), 0))
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

	for (unsigned i=0; i<Stats::COUNT + eset->damage_types.count; i++) {
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

	for (size_t i=0; i<effect_list.size(); ++i) {
		Effect& ei = effect_list[i];

		// @CLASS EffectManager|Description of "type" in powers/effects.txt
		// expire timed effects and total up magnitudes of active effects
		if (ei.timer.getDuration() > 0) {
			if (ei.timer.isEnd()) {
				//death sentence is only applied at the end of the timer
				// @TYPE death_sentence|Causes sudden death at the end of the effect duration.
				if (ei.type == Effect::DEATH_SENTENCE) death_sentence = true;
				removeEffect(i);
				i--;
				continue;
			}
		}

		// @TYPE damage|Damage per second
		if (ei.type == Effect::DAMAGE && ei.timer.isWholeSecond()) damage += ei.magnitude;
		// @TYPE damage_percent|Damage per second (percentage of max HP)
		else if (ei.type == Effect::DAMAGE_PERCENT && ei.timer.isWholeSecond()) damage_percent += ei.magnitude;
		// @TYPE hpot|HP restored per second
		else if (ei.type == Effect::HPOT && ei.timer.isWholeSecond()) hpot += ei.magnitude;
		// @TYPE hpot_percent|HP restored per second (percentage of max HP)
		else if (ei.type == Effect::HPOT_PERCENT && ei.timer.isWholeSecond()) hpot_percent += ei.magnitude;
		// @TYPE mpot|MP restored per second
		else if (ei.type == Effect::MPOT && ei.timer.isWholeSecond()) mpot += ei.magnitude;
		// @TYPE mpot_percent|MP restored per second (percentage of max MP)
		else if (ei.type == Effect::MPOT_PERCENT && ei.timer.isWholeSecond()) mpot_percent += ei.magnitude;
		// @TYPE speed|Changes movement speed. A magnitude of 100 is 100% speed (aka normal speed).
		else if (ei.type == Effect::SPEED) speed = (static_cast<float>(ei.magnitude) * speed) / 100.f;
		// @TYPE attack_speed|Changes attack speed. A magnitude of 100 is 100% speed (aka normal speed).
		// attack speed is calculated when getAttackSpeed() is called

		// @TYPE immunity|Applies all immunity effects. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY) {
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
		else if (ei.type == Effect::IMMUNITY_DAMAGE) immunity_damage = true;
		// @TYPE immunity_slow|Removes and prevents slow effects. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY_SLOW) immunity_slow = true;
		// @TYPE immunity_stun|Removes and prevents stun effects. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY_STUN) immunity_stun = true;
		// @TYPE immunity_hp_steal|Prevents HP stealing. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY_HP_STEAL) immunity_hp_steal = true;
		// @TYPE immunity_mp_steal|Prevents MP stealing. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY_MP_STEAL) immunity_mp_steal = true;
		// @TYPE immunity_knockback|Removes and prevents knockback effects. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY_KNOCKBACK) immunity_knockback = true;
		// @TYPE immunity_damage_reflect|Prevents damage reflection. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY_DAMAGE_REFLECT) immunity_damage_reflect = true;
		// @TYPE immunity_stat_debuff|Prevents stat value altering effects that have a magnitude less than 0. Magnitude is ignored.
		else if (ei.type == Effect::IMMUNITY_STAT_DEBUFF) immunity_stat_debuff = true;

		// @TYPE stun|Can't move or attack. Being attacked breaks stun.
		else if (ei.type == Effect::STUN) stun = true;
		// @TYPE revive|Revives the player. Typically attached to a power that triggers when the player dies.
		else if (ei.type == Effect::REVIVE) revive = true;
		// @TYPE convert|Causes an enemy or an ally to switch allegiance
		else if (ei.type == Effect::CONVERT) convert = true;
		// @TYPE fear|Causes enemies to run away
		else if (ei.type == Effect::FEAR) fear = true;
		// @TYPE knockback|Pushes the target away from the source caster. Speed is the given value divided by the framerate cap.
		else if (ei.type == Effect::KNOCKBACK) knockback_speed = static_cast<float>(ei.magnitude)/static_cast<float>(settings->max_frames_per_sec);

		// @TYPE ${STATNAME}|Increases ${STATNAME}, where ${STATNAME} is any of the base stats. Examples: hp, avoidance, xp_gain
		// @TYPE ${DAMAGE_TYPE}|Increases a damage min or max, where ${DAMAGE_TYPE} is any 'min' or 'max' value found in engine/damage_types.txt. Example: dmg_melee_min
		else if (ei.type >= Effect::TYPE_COUNT && ei.type < Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count)) {
			bonus[ei.type - Effect::TYPE_COUNT] += ei.magnitude;
		}
		// @TYPE ${ELEMENT}_resist|Increase Resistance % to ${ELEMENT}, where ${ELEMENT} is any found in engine/elements.txt. Example: fire_resist
		else if (ei.type >= Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) && ei.type < Effect::TYPE_COUNT + Stats::COUNT + static_cast<int>(eset->damage_types.count) + static_cast<int>(eset->elements.list.size())) {
			bonus_resist[ei.type - Effect::TYPE_COUNT - Stats::COUNT - eset->damage_types.count] += ei.magnitude;
		}
		// @TYPE ${PRIMARYSTAT}|Increases ${PRIMARYSTAT}, where ${PRIMARYSTAT} is any of the primary stats defined in engine/primary_stats.txt. Example: physical
		else if (ei.type >= Effect::TYPE_COUNT) {
			bonus_primary[ei.type - Effect::TYPE_COUNT - Stats::COUNT - eset->damage_types.count - eset->elements.list.size()] += ei.magnitude;
		}

		ei.timer.tick();

		// expire shield effects
		if (ei.magnitude_max > 0 && ei.magnitude == 0) {
			// @TYPE shield|Create a damage absorbing barrier based on Mental damage stat. Duration is ignored.
			if (ei.type == Effect::SHIELD) {
				removeEffect(i);
				i--;
				continue;
			}
		}
		// expire effects based on animations
		if ((ei.animation && ei.animation->isLastFrame()) || !ei.animation) {
			// @TYPE heal|Restore HP based on Mental damage stat.
			if (ei.type == Effect::HEAL) {
				removeEffect(i);
				i--;
				continue;
			}
		}

		// animate
		if (ei.animation) {
			if (!ei.animation->isCompleted())
				ei.animation->advanceFrame();
		}
	}
}

void EffectManager::addEffect(EffectDef &effect, int duration, int magnitude, int source_type, PowerID power_id) {
	addEffectInternal(effect, duration, magnitude, source_type, false, power_id);
}

void EffectManager::addItemEffect(EffectDef &effect, int duration, int magnitude) {
	// only the hero can wear items, so use Power::SOURCE_TYPE_HERO
	addEffectInternal(effect, duration, magnitude, Power::SOURCE_TYPE_HERO, true, NO_POWER);
}

void EffectManager::addEffectInternal(EffectDef &effect, int duration, int magnitude, int source_type, bool item, PowerID power_id) {
	refresh_stats = true;

	// if we're already immune, don't add negative effects
	if (immunity_damage && (effect.type == Effect::DAMAGE || effect.type == Effect::DAMAGE_PERCENT))
		return;
	else if (immunity_slow && effect.type == Effect::SPEED && magnitude < 100)
		return;
	else if (immunity_stun && effect.type == Effect::STUN)
		return;
	else if (immunity_knockback && effect.type == Effect::KNOCKBACK)
		return;
	else if (immunity_stat_debuff && effect.type > Effect::TYPE_COUNT && magnitude < 0)
		return;

	// only allow one knockback effect at a time
	if (effect.type == Effect::KNOCKBACK && knockback_speed != 0)
		return;

	bool insert_effect = false;
	size_t insert_pos;
	int stacks_applied = 0;
	int trigger = power_id > 0 ? powers->powers[power_id].passive_trigger : -1;
	size_t passive_id = (power_id > 0 && powers->powers[power_id].passive) ? power_id : 0;

	for (size_t i=effect_list.size(); i>0; i--) {
		Effect& ei = effect_list[i-1];

		// while checking only id would be sufficient, it is a slow string compare
		// so we check the type first, which is an int compare, before taking the slow path
		if (ei.type == effect.type && ei.id == effect.id) {
			if (trigger > -1 && ei.trigger == trigger)
				return; // trigger effects can only be cast once per trigger

			if (!effect.can_stack) {
				removeEffect(i-1);
			}
			else{
				if (effect.type == Effect::SHIELD && effect.group_stack){
					ei.magnitude += magnitude;

					if (effect.max_stacks == -1
						|| (magnitude != 0 && ei.magnitude_max/magnitude < effect.max_stacks)){
						ei.magnitude_max += magnitude;
					}

					if (ei.magnitude > ei.magnitude_max){
						ei.magnitude = ei.magnitude_max;
					}

					return;
				}

				if (insert_effect == false && effect.max_stacks != -1) {
					// to keep stackable effects together, they are inserted after the most recent matching effect
					// otherwise, they are added to the end of the effect list
					insert_effect = true;
					insert_pos = i;
				}

				stacks_applied++;
			}
		}
		// if we're adding an immunity effect, remove all negative effects
		if (effect.type == Effect::IMMUNITY)
			clearNegativeEffects();
		else if (effect.type == Effect::IMMUNITY_DAMAGE)
			clearNegativeEffects(Effect::IMMUNITY_DAMAGE);
		else if (effect.type == Effect::IMMUNITY_SLOW)
			clearNegativeEffects(Effect::IMMUNITY_SLOW);
		else if (effect.type == Effect::IMMUNITY_STUN)
			clearNegativeEffects(Effect::IMMUNITY_STUN);
		else if (effect.type == Effect::IMMUNITY_KNOCKBACK)
			clearNegativeEffects(Effect::IMMUNITY_KNOCKBACK);
	}

	Effect e;

	e.id = effect.id;
	e.name = effect.name;
	e.icon = effect.icon;
	e.type = effect.type;
	e.render_above = effect.render_above;
	e.group_stack = effect.group_stack;
	e.color_mod = effect.color_mod.encodeRGBA();
	e.alpha_mod = effect.alpha_mod;
	e.attack_speed_anim = effect.attack_speed_anim;

	if (!effect.animation.empty()) {
		e.loadAnimation(effect.animation);
	}

	e.timer.setDuration(duration);
	e.magnitude = e.magnitude_max = magnitude;
	e.item = item;
	e.trigger = trigger;
	e.passive_id = passive_id;
	e.source_type = source_type;

	if (insert_effect) {
		if (effect.max_stacks != -1 && stacks_applied >= effect.max_stacks){
			//Remove the oldest effect of the type
			removeEffect(insert_pos-stacks_applied);

			//All elements have shifted to left
			insert_pos--;
		}

		effect_list.insert(effect_list.begin() + insert_pos, e);
	}
	else {
		effect_list.push_back(e);
	}
}

void EffectManager::removeEffect(size_t id) {
	effect_list.erase(effect_list.begin()+id);
	refresh_stats = true;
}

void EffectManager::removeEffectType(const int type) {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].type == type) removeEffect(i-1);
	}
}

void EffectManager::removeEffectPassive(size_t id) {
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
		if ((type == -1 || type == Effect::IMMUNITY_DAMAGE) && effect_list[i-1].type == Effect::DAMAGE)
			removeEffect(i-1);
		else if ((type == -1 || type == Effect::IMMUNITY_DAMAGE) && effect_list[i-1].type == Effect::DAMAGE_PERCENT)
			removeEffect(i-1);
		else if ((type == -1 || type == Effect::IMMUNITY_SLOW) && effect_list[i-1].type == Effect::SPEED && effect_list[i-1].magnitude_max < 100)
			removeEffect(i-1);
		else if ((type == -1 || type == Effect::IMMUNITY_STUN) && effect_list[i-1].type == Effect::STUN)
			removeEffect(i-1);
		else if ((type == -1 || type == Effect::IMMUNITY_KNOCKBACK) && effect_list[i-1].type == Effect::KNOCKBACK)
			removeEffect(i-1);
		else if ((type == -1 || type == Effect::IMMUNITY_STAT_DEBUFF) && effect_list[i-1].type > Effect::TYPE_COUNT && effect_list[i-1].magnitude_max < 0)
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
		if (effect_list[i].magnitude_max > 0 && effect_list[i].type == Effect::SHIELD) {
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

bool EffectManager::isDebuffed() {
	for (size_t i=effect_list.size(); i > 0; i--) {
		if (effect_list[i-1].type == Effect::DAMAGE) return true;
		else if (effect_list[i-1].type == Effect::DAMAGE_PERCENT) return true;
		else if (effect_list[i-1].type == Effect::SPEED && effect_list[i-1].magnitude_max < 100) return true;
		else if (effect_list[i-1].type == Effect::STUN) return true;
		else if (effect_list[i-1].type == Effect::KNOCKBACK) return true;
		else if (effect_list[i-1].type > Effect::TYPE_COUNT && effect_list[i-1].magnitude_max < 0) return true;
	}
	return false;
}

void EffectManager::getCurrentColor(Color& color_mod) {
	uint32_t default_color = color_mod.encodeRGBA();
	uint32_t no_color = Color(255, 255, 255).encodeRGBA();

	for (size_t i=effect_list.size(); i > 0; i--) {
		Effect& ei = effect_list[i-1];
		if (ei.color_mod == no_color)
			continue;

		if (ei.color_mod != default_color) {
			color_mod.decodeRGBA(ei.color_mod);
			return;
		}
	}
}

void EffectManager::getCurrentAlpha(uint8_t& alpha_mod) {
	uint8_t default_alpha = alpha_mod;
	uint8_t no_alpha = 255;

	for (size_t i=effect_list.size(); i > 0; i--) {
		Effect& ei = effect_list[i-1];
		if (ei.alpha_mod == no_alpha)
			continue;

		if (ei.alpha_mod != default_alpha) {
			alpha_mod = ei.alpha_mod;
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
		if (effect_list[i].type != Effect::ATTACK_SPEED)
			continue;

		if (effect_list[i].attack_speed_anim.empty() || effect_list[i].attack_speed_anim == anim_name) {
			attack_speed = (static_cast<float>(effect_list[i].magnitude) * attack_speed) / 100.0f;
		}
	}

	return attack_speed;
}

int EffectManager::getDamageSourceType(int dmg_mode) {
	if (!(dmg_mode == Effect::DAMAGE || dmg_mode == Effect::DAMAGE_PERCENT))
		return -1;

	int source_type = Power::SOURCE_TYPE_NEUTRAL;

	for (size_t i = 0; i < effect_list.size(); ++i) {
		Effect& ei = effect_list[i];
		if (ei.type == dmg_mode) {
			// anything other than ally source type take precedence, so we can return early
			if (ei.source_type != Power::SOURCE_TYPE_ALLY)
				return ei.source_type;

			source_type = ei.source_type;
		}
	}

	return source_type;
}
