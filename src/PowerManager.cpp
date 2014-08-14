/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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
 * class PowerManager
 */

#include "PowerManager.h"
#include "Animation.h"
#include "AnimationSet.h"
#include "AnimationManager.h"
#include "FileParser.h"
#include "Hazard.h"
#include "SharedResources.h"
#include "Settings.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "MapCollision.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <cmath>
#include <climits>
using namespace std;


/**
 * PowerManager constructor
 */
PowerManager::PowerManager()
	: collider(NULL)
	, log_msg("")
	, used_items()
	, used_equipped_items() {
	loadEffects();
	loadPowers();
}

void PowerManager::loadEffects() {
	FileParser infile;

	// @CLASS Effects|Description of powers/effects.txt
	if (!infile.open("powers/effects.txt"))
		return;

	std::string input_name = "";
	bool skippingEntry = false;

	while (infile.next()) {
		// name needs to be the first component of each power.  That is how we write
		// data to the correct power.
		if (infile.key == "name") {
			// @ATTR name|string|Uniq identifier for the effect definition.
			input_name = infile.val;
			skippingEntry = input_name == "";
			if (skippingEntry)
				logError("PowerManager: Effect without a name, skipping\n");
			continue;
		}
		if (skippingEntry)
			continue;

		if (infile.key == "type") {
			// @ATTR type|string|Defines the type of effect
			effects[input_name].type = infile.val;
		}
		else if (infile.key == "icon") {
			// @ATTR icon|integer|The icon to visually represent the effect in the status area
			effects[input_name].icon = toInt(infile.val);
		}
		else if (infile.key == "animation") {
			// @ATTR animation|string|The filename of effect animation.
			effects[input_name].animation = infile.val;
		}
		else if (infile.key == "additive") {
			// @ATTR additive|bool|Effect is cumulative
			effects[input_name].additive = toBool(infile.val);
		}
		else if (infile.key == "render_above") {
			// @ATTR render_above|bool|Effect is rendered above
			effects[input_name].render_above = toBool(infile.val);
		}
	}
	infile.close();
}

void PowerManager::loadPowers() {
	FileParser infile;

	// @CLASS Powers|Description of powers/powers.txt
	if (!infile.open("powers/powers.txt"))
		return;

	int input_id = 0;
	bool skippingEntry = false;

	while (infile.next()) {
		// id needs to be the first component of each power.  That is how we write
		// data to the correct power.
		if (infile.key == "id") {
			// @ATTR id|integer|Uniq identifier for the power definition.
			input_id = toInt(infile.val);
			skippingEntry = input_id < 1;
			if (skippingEntry)
				logError("PowerManager: Power index out of bounds 1-%d, skipping\n", INT_MAX);
			if (static_cast<int>(powers.size()) < input_id + 1)
				powers.resize(input_id + 1);
			continue;
		}
		if (skippingEntry)
			continue;

		if (infile.key == "type") {
			// @ATTR type|[fixed:missile:repeater:spawn:transform]|Defines the type of power definiton
			if (infile.val == "fixed") powers[input_id].type = POWTYPE_FIXED;
			else if (infile.val == "missile") powers[input_id].type = POWTYPE_MISSILE;
			else if (infile.val == "repeater") powers[input_id].type = POWTYPE_REPEATER;
			else if (infile.val == "spawn") powers[input_id].type = POWTYPE_SPAWN;
			else if (infile.val == "transform") powers[input_id].type = POWTYPE_TRANSFORM;
			else logError("PowerManager: Unknown type %s\n", infile.val.c_str());
		}
		else if (infile.key == "name")
			// @ATTR name|string|The name of the power
			powers[input_id].name = msg->get(infile.val);
		else if (infile.key == "description")
			// @ATTR description|string|Description of the power
			powers[input_id].description = msg->get(infile.val);
		else if (infile.key == "icon")
			// @ATTR icon|string|The icon to visually represent the power eg. in skill tree or action bar.
			powers[input_id].icon = toInt(infile.val);
		else if (infile.key == "new_state") {
			// @ATTR new_state|string|When power is used, hero or enemy will change to this state. Must be one of the states [block, instant, user defined]
			if (infile.val == "block") powers[input_id].new_state = POWSTATE_BLOCK;
			else if (infile.val == "instant") powers[input_id].new_state = POWSTATE_INSTANT;
			else {
				powers[input_id].new_state = POWSTATE_ATTACK;
				powers[input_id].attack_anim = infile.val;
			}
		}
		else if (infile.key == "face")
			// @ATTR face|bool|Power will make hero or enemy to face the target location.
			powers[input_id].face = toBool(infile.val);
		else if (infile.key == "source_type") {
			// @ATTR source_type|[hero:neutral:enemy]|Determines which entities the power can effect.
			if (infile.val == "hero") powers[input_id].source_type = SOURCE_TYPE_HERO;
			else if (infile.val == "neutral") powers[input_id].source_type = SOURCE_TYPE_NEUTRAL;
			else if (infile.val == "enemy") powers[input_id].source_type = SOURCE_TYPE_ENEMY;
			else logError("PowerManager: Unknown source_type %s\n", infile.val.c_str());
		}
		else if (infile.key == "beacon")
			// @ATTR beacon|bool|True if enemy is calling its allies.
			powers[input_id].beacon = toBool(infile.val);
		else if (infile.key == "count")
			// @ATTR count|integer|The count of hazards/effect or spawns to be created by this power.
			powers[input_id].count = toInt(infile.val);
		else if (infile.key == "passive")
			// @ATTR passive|bool|If power is unlocked when the hero or enemy spawns it will be automatically activated.
			powers[input_id].passive = toBool(infile.val);
		else if (infile.key == "passive_trigger") {
			// @ATTR passive_trigger|[on_block:on_hit:on_halfdeath:on_joincombat:on_death]|This will only activate a passive power under a certain condition.
			if (infile.val == "on_block") powers[input_id].passive_trigger = TRIGGER_BLOCK;
			else if (infile.val == "on_hit") powers[input_id].passive_trigger = TRIGGER_HIT;
			else if (infile.val == "on_halfdeath") powers[input_id].passive_trigger = TRIGGER_HALFDEATH;
			else if (infile.val == "on_joincombat") powers[input_id].passive_trigger = TRIGGER_JOINCOMBAT;
			else if (infile.val == "on_death") powers[input_id].passive_trigger = TRIGGER_DEATH;
			else logError("PowerManager: Unknown passive trigger %s\n", infile.val.c_str());
		}
		else if (infile.key == "meta_power") {
			// @ATTR meta_power|boolean|If true, this power can not be used on it's own. Instead, it should be replaced via an item with a replace_power entry.
			powers[input_id].meta_power = toBool(infile.val);
		}
		// power requirements
		else if (infile.key == "requires_flags") {
			// @ATTR requires_flags|flag (string), ...|A comma separated list of equip flags that are required to use this power. See engine/equip_flags.txt
			std::string flag = popFirstString(infile.val);

			while (flag != "") {
				powers[input_id].requires_flags.insert(flag);
				flag = popFirstString(infile.val);
			}
		}
		else if (infile.key == "requires_mp")
			// @ATTR requires_mp|integer|Restrict power usage to a specified MP level.
			powers[input_id].requires_mp = toInt(infile.val);
		else if (infile.key == "requires_hp")
			// @ATTR requires_hp|integer|Restrict power usage to a specified HP level.
			powers[input_id].requires_hp = toInt(infile.val);
		else if (infile.key == "sacrifice")
			// @ATTR sacrifice|bool|If the power has requires_hp, allow it to kill the caster.
			powers[input_id].sacrifice = toBool(infile.val);
		else if (infile.key == "requires_los")
			// @ATTR requires_los|bool|Requires a line-of-sight to target.
			powers[input_id].requires_los = toBool(infile.val);
		else if (infile.key == "requires_empty_target")
			// @ATTR requires_empty_target|bool|The power can only be cast when target tile is empty.
			powers[input_id].requires_empty_target = toBool(infile.val);
		else if (infile.key == "requires_item")
			// @ATTR requires_item|item_id|Requires a specific item in inventory.
			powers[input_id].requires_item = toInt(infile.val);
		else if (infile.key == "requires_equipped_item")
			// @ATTR requires_equipped_item|item_id|Requires a specific item to be equipped on hero.
			powers[input_id].requires_equipped_item = toInt(infile.val);
		else if (infile.key == "requires_targeting")
			// @ATTR requires_targeting|bool|Power is only used when targeting using click-to-target.
			powers[input_id].requires_targeting = toBool(infile.val);
		else if (infile.key == "cooldown")
			// @ATTR cooldown|duration|Specify the duration for cooldown of the power.
			powers[input_id].cooldown = parse_duration(infile.val);
		// animation info
		else if (infile.key == "animation")
			// @ATTR animation|string|The filename of the power animation.
			powers[input_id].animation_name = infile.val;
		else if (infile.key == "soundfx")
			// @ATTR soundfx|string|Sound effect to play when use of power.
			powers[input_id].sfx_index = loadSFX(infile.val);
		else if (infile.key == "directional")
			// @ATTR directional|bool|The animation sprite sheet contains 8 directions, one per row.
			powers[input_id].directional = toBool(infile.val);
		else if (infile.key == "visual_random")
			// @ATTR visual_random|integer|The animation sprite sheet contains rows of random options
			powers[input_id].visual_random = toInt(infile.val);
		else if (infile.key == "visual_option")
			// @ATTR visual_option|integer|The animation sprite sheet containers rows of similar effects, use a specific option.
			powers[input_id].visual_option = toInt(infile.val);
		else if (infile.key == "aim_assist")
			// @ATTR aim_assist|bool|Power is aim assisted.
			powers[input_id].aim_assist = toBool(infile.val);
		else if (infile.key == "speed")
			// @ATTR speed|integer|The speed of missile hazard, the unit is defined as map units per frame.
			powers[input_id].speed = toFloat(infile.val) / MAX_FRAMES_PER_SEC;
		else if (infile.key == "lifespan")
			// @ATTR lifespan|duration|How long the hazard/animation lasts.
			powers[input_id].lifespan = parse_duration(infile.val);
		else if (infile.key == "floor")
			// @ATTR floor|bool|The hazard is drawn between the background and the object layer.
			powers[input_id].floor = toBool(infile.val);
		else if (infile.key == "complete_animation")
			// @ATTR complete_animation|bool|For hazards; Play the entire animation, even if the hazard has hit a target.
			powers[input_id].complete_animation = toBool(infile.val);
		// hazard traits
		else if (infile.key == "use_hazard")
			// @ATTR use_hazard|bool|Power uses hazard.
			powers[input_id].use_hazard = toBool(infile.val);
		else if (infile.key == "no_attack")
			// @ATTR no_attack|bool|Hazard won't affect other entities.
			powers[input_id].no_attack = toBool(infile.val);
		else if (infile.key == "radius")
			// @ATTR radius|integer|Radius in pixels
			powers[input_id].radius = toFloat(infile.val);
		else if (infile.key == "base_damage") {
			// @ATTR base_damage|[melee:ranged:ment]|
			if (infile.val == "none")        powers[input_id].base_damage = BASE_DAMAGE_NONE;
			else if (infile.val == "melee")  powers[input_id].base_damage = BASE_DAMAGE_MELEE;
			else if (infile.val == "ranged") powers[input_id].base_damage = BASE_DAMAGE_RANGED;
			else if (infile.val == "ment")   powers[input_id].base_damage = BASE_DAMAGE_MENT;
			else logError("PowerManager: Unknown base_damage %s\n", infile.val.c_str());
		}
		else if (infile.key == "starting_pos") {
			// @ATTR starting_pos|[source, target, melee]|Start position for hazard
			if (infile.val == "source")      powers[input_id].starting_pos = STARTING_POS_SOURCE;
			else if (infile.val == "target") powers[input_id].starting_pos = STARTING_POS_TARGET;
			else if (infile.val == "melee")  powers[input_id].starting_pos = STARTING_POS_MELEE;
			else logError("PowerManager: Unknown starting_pos %s\n", infile.val.c_str());
		}
		else if (infile.key == "multitarget")
			// @ATTR multitarget|bool|Allows a hazard power to hit more than one entity.
			powers[input_id].multitarget = toBool(infile.val);
		else if (infile.key == "trait_armor_penetration")
			// @ATTR trait_armor_penetration|bool|Ignores the target's Absorbtion stat
			powers[input_id].trait_armor_penetration = toBool(infile.val);
		else if (infile.key == "trait_avoidance_ignore")
			// @ATTR trait_avoidance_ignore|bool|Ignores the target's Avoidance stat
			powers[input_id].trait_avoidance_ignore = toBool(infile.val);
		else if (infile.key == "trait_crits_impaired")
			// @ATTR trait_crits_impaired|integer|Increases critical hit percentage for slowed/immobile targets
			powers[input_id].trait_crits_impaired = toInt(infile.val);
		else if (infile.key == "trait_elemental") {
			// @ATTR trait_elemental|string|Damage done is elemental. See engine/elements.txt
			for (unsigned int i=0; i<ELEMENTS.size(); i++) {
				if (infile.val == ELEMENTS[i].name) powers[input_id].trait_elemental = i;
			}
		}
		else if (infile.key == "target_range")
			// @ATTR target_range|float||The distance from the caster that the power can be activated
			powers[input_id].target_range = toFloat(infile.nextValue());
		//steal effects
		else if (infile.key == "hp_steal")
			// @ATTR hp_steal|integer|Percentage of damage to steal into HP
			powers[input_id].hp_steal = toInt(infile.val);
		else if (infile.key == "mp_steal")
			// @ATTR mp_steal|integer|Percentage of damage to steal into MP
			powers[input_id].mp_steal = toInt(infile.val);
		//missile modifiers
		else if (infile.key == "missile_angle")
			// @ATTR missile_angle|integer|Angle of missile
			powers[input_id].missile_angle = toInt(infile.val);
		else if (infile.key == "angle_variance")
			// @ATTR angle_variance|integer|Percentage of variance added to missile angle
			powers[input_id].angle_variance = toInt(infile.val);
		else if (infile.key == "speed_variance")
			// @ATTR speed_variance|integer|Percentage of variance added to missile speed
			powers[input_id].speed_variance = toFloat(infile.val);
		//repeater modifiers
		else if (infile.key == "delay")
			// @ATTR delay|duration|Delay between repeats
			powers[input_id].delay = parse_duration(infile.val);
		// buff/debuff durations
		else if (infile.key == "transform_duration")
			// @ATTR transform_duration|duration|Duration for transform
			powers[input_id].transform_duration = parse_duration(infile.val);
		else if (infile.key == "manual_untransform")
			// @ATTR manual_untransform|bool|Force manual untranform
			powers[input_id].manual_untransform = toBool(infile.val);
		else if (infile.key == "keep_equipment")
			// @ATTR keep_equipment|bool|Keep  equipment while transformed
			powers[input_id].keep_equipment = toBool(infile.val);
		// buffs
		else if (infile.key == "buff")
			// @ATTR buff|bool|Power is cast upon the caster.
			powers[input_id].buff= toBool(infile.val);
		else if (infile.key == "buff_teleport")
			// @ATTR buff_teleport|bool|Power is a teleportation power.
			powers[input_id].buff_teleport = toBool(infile.val);
		else if (infile.key == "buff_party")
			// @ATTR buff_part|bool|Power is cast upon party members
			powers[input_id].buff_party = toBool(infile.val);
		else if (infile.key == "buff_party_power_id")
			// @ATTR buff_part_power_id|integer|Buffs a power id for all party members
			powers[input_id].buff_party_power_id = toInt(infile.val);
		else if (infile.key == "post_effect") {
			// @ATTR post_effect|[effect_id, magnitude (integer), duration (duration)]|Post effect.
			PostEffect pe;
			pe.id = popFirstString(infile.val);
			pe.magnitude = popFirstInt(infile.val);
			pe.duration = parse_duration(popFirstString(infile.val));
			powers[input_id].post_effects.push_back(pe);
		}
		// pre and post power effects
		else if (infile.key == "post_power")
			// @ATTR post_power|power_id|Trigger a power if the hazard did damage.
			powers[input_id].post_power = toInt(infile.val);
		else if (infile.key == "wall_power")
			// @ATTR wall_power|power_id|Trigger a power if the hazard hit a wall.
			powers[input_id].wall_power = toInt(infile.val);
		// spawn info
		else if (infile.key == "spawn_type")
			// @ATTR spawn_type|string|Type of spawn.
			powers[input_id].spawn_type = infile.val;
		else if (infile.key == "target_neighbor")
			// @ATTR target_neighbor|int|Target is changed to an adjacent tile within a radius.
			powers[input_id].target_neighbor = toInt(infile.val);
		else if (infile.key == "spawn_limit") {
			// @ATTR spawn_limit|[fixed:stat:unlimited],stat[physical:mental:offense:defens]|
			std::string mode = popFirstString(infile.val);
			if (mode == "fixed") powers[input_id].spawn_limit_mode = SPAWN_LIMIT_MODE_FIXED;
			else if (mode == "stat") powers[input_id].spawn_limit_mode = SPAWN_LIMIT_MODE_STAT;
			else if (mode == "unlimited") powers[input_id].spawn_limit_mode = SPAWN_LIMIT_MODE_UNLIMITED;
			else logError("PowerManager: Unknown spawn_limit_mode %s\n", mode.c_str());

			if(powers[input_id].spawn_limit_mode != SPAWN_LIMIT_MODE_UNLIMITED) {
				powers[input_id].spawn_limit_qty = popFirstInt(infile.val);

				if(powers[input_id].spawn_limit_mode == SPAWN_LIMIT_MODE_STAT) {
					powers[input_id].spawn_limit_every = popFirstInt(infile.val);

					std::string stat = popFirstString(infile.val);
					if (stat == "physical") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_PHYSICAL;
					else if (stat == "mental") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_MENTAL;
					else if (stat == "offense") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_OFFENSE;
					else if (stat == "defense") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_DEFENSE;
					else logError("PowerManager: Unknown spawn_limit_stat %s\n", stat.c_str());
				}
			}
		}
		else if (infile.key == "spawn_level") {
			// @ATTR spawn_level|[default:fixed:stat:level],stat[physical:mental:offense:defens]|
			std::string mode = popFirstString(infile.val);
			if (mode == "default") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_DEFAULT;
			else if (mode == "fixed") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_FIXED;
			else if (mode == "stat") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_STAT;
			else if (mode == "level") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_LEVEL;
			else logError("PowerManager: Unknown spawn_level_mode %s\n", mode.c_str());

			if(powers[input_id].spawn_level_mode != SPAWN_LEVEL_MODE_DEFAULT) {
				powers[input_id].spawn_level_qty = popFirstInt(infile.val);

				if(powers[input_id].spawn_level_mode != SPAWN_LEVEL_MODE_FIXED) {
					powers[input_id].spawn_level_every = popFirstInt(infile.val);

					if(powers[input_id].spawn_level_mode == SPAWN_LEVEL_MODE_STAT) {
						std::string stat = popFirstString(infile.val);
						if (stat == "physical") powers[input_id].spawn_level_stat = SPAWN_LEVEL_STAT_PHYSICAL;
						else if (stat == "mental") powers[input_id].spawn_level_stat = SPAWN_LEVEL_STAT_MENTAL;
						else if (stat == "offense") powers[input_id].spawn_level_stat = SPAWN_LEVEL_STAT_OFFENSE;
						else if (stat == "defense") powers[input_id].spawn_level_stat = SPAWN_LEVEL_STAT_DEFENSE;
						else logError("PowerManager: Unknown spawn_level_stat %s\n", stat.c_str());
					}
				}
			}
		}
		else if (infile.key == "target_party")
			// @ATTR target_party|bool|Hazard will only affect party members.
			powers[input_id].target_party = toBool(infile.val);
		else if (infile.key == "target_categories") {
			// @ATTR target_categories|string,...|Hazard will only affect enemies in these categories.
			string cat;
			while ((cat = infile.nextValue()) != "") {
				powers[input_id].target_categories.push_back(cat);
			}
		}
		else if (infile.key == "modifier_accuracy") {
			// @ATTR modifier_accuracy|[multiply:add:absolute], integer|Changes this power's accuracy.
			std::string mode = popFirstString(infile.val);
			if(mode == "multiply") powers[input_id].mod_accuracy_mode = STAT_MODIFIER_MODE_MULTIPLY;
			else if(mode == "add") powers[input_id].mod_accuracy_mode = STAT_MODIFIER_MODE_ADD;
			else if(mode == "absolute") powers[input_id].mod_accuracy_mode = STAT_MODIFIER_MODE_ABSOLUTE;
			else logError("PowerManager: Unknown stat_modifier_mode %s\n", mode.c_str());

			powers[input_id].mod_accuracy_value = popFirstInt(infile.val);
		}
		else if (infile.key == "modifier_damage") {
			// @ATTR modifier_damage|[multiply:add:absolute], integer|Changes this power's damage.
			std::string mode = popFirstString(infile.val);
			if(mode == "multiply") powers[input_id].mod_damage_mode = STAT_MODIFIER_MODE_MULTIPLY;
			else if(mode == "add") powers[input_id].mod_damage_mode = STAT_MODIFIER_MODE_ADD;
			else if(mode == "absolute") powers[input_id].mod_damage_mode = STAT_MODIFIER_MODE_ABSOLUTE;
			else logError("PowerManager: Unknown stat_modifier_mode %s\n", mode.c_str());

			powers[input_id].mod_damage_value_min = popFirstInt(infile.val);
			powers[input_id].mod_damage_value_max = popFirstInt(infile.val);
		}
		else if (infile.key == "modifier_critical") {
			// @ATTR modifier_critical|[multiply:add:absolute], integer|Changes the chance that this power will land a critical hit.
			std::string mode = popFirstString(infile.val);
			if(mode == "multiply") powers[input_id].mod_crit_mode = STAT_MODIFIER_MODE_MULTIPLY;
			else if(mode == "add") powers[input_id].mod_crit_mode = STAT_MODIFIER_MODE_ADD;
			else if(mode == "absolute") powers[input_id].mod_crit_mode = STAT_MODIFIER_MODE_ABSOLUTE;
			else logError("PowerManager: Unknown stat_modifier_mode %s\n", mode.c_str());

			powers[input_id].mod_crit_value = popFirstInt(infile.val);
		}
		else
			logError("PowerManager: Ignoring unknown key %s set to %s in file %s\n",
					infile.key.c_str(), infile.val.c_str(), infile.getFileName().c_str());
	}
	infile.close();
}

/**
 * Load the specified sound effect for this power
 *
 * @param filename The .ogg file containing the sound for this power, assumed to be in soundfx/powers/
 * @return The sfx[] array index for this mix chunk, or -1 upon load failure
 */
int PowerManager::loadSFX(const string& filename) {

	SoundManager::SoundID sid = snd->load(filename, "PowerManager sfx");
	vector<SoundManager::SoundID>::iterator it = std::find(sfx.begin(), sfx.end(), sid);
	if (it == sfx.end()) {
		sfx.push_back(sid);
		return sfx.size() - 1;
	}

	return it - sfx.begin();
}


/**
 * Set new collision object
 */
void PowerManager::handleNewMap(MapCollision *_collider) {
	collider = _collider;
}

/**
 * Keep two points within a certain range
 */
FPoint PowerManager::limitRange(float range, FPoint src, FPoint target) {
	if (range > 0) {
		if (src.x+range < target.x)
			target.x = src.x+range;
		if (src.x-range > target.x)
			target.x = src.x-range;
		if (src.y+range < target.y)
			target.y = src.y+range;
		if (src.y-range > target.y)
			target.y = src.y-range;
	}

	return target;
}

/**
 * Check if the target is valid (not an empty area or a wall)
 */
bool PowerManager::hasValidTarget(int power_index, StatBlock *src_stats, FPoint target) {

	if (!collider) return false;

	target = limitRange(powers[power_index].target_range,src_stats->pos,target);

	if (!collider->is_empty(target.x, target.y) || collider->is_wall(target.x,target.y)) {
		if (powers[power_index].buff_teleport) {
			return false;
		}
	}

	return true;
}

/**
 * Apply basic power info to a new hazard.
 *
 * This can be called several times to combine powers.
 * Typically done when a base power can be modified by equipment
 * (e.g. ammo type affects the traits of powers that shoot)
 *
 * @param power_index The activated power ID
 * @param src_stats The StatBlock of the power activator
 * @param target Aim position in map coordinates
 * @param haz A newly-initialized hazard
 */
void PowerManager::initHazard(int power_index, StatBlock *src_stats, FPoint target, Hazard *haz) {

	//the hazard holds the statblock of its source
	haz->src_stats = src_stats;

	haz->power_index = power_index;

	if (powers[power_index].source_type == -1) {
		if (src_stats->hero) haz->source_type = SOURCE_TYPE_HERO;
		else if (src_stats->hero_ally) haz->source_type = SOURCE_TYPE_ALLY;
		else haz->source_type = SOURCE_TYPE_ENEMY;
	}
	else {
		haz->source_type = powers[power_index].source_type;
	}

	haz->target_party = powers[power_index].target_party;

	// Hazard attributes based on power source
	haz->crit_chance = src_stats->get(STAT_CRIT);
	haz->accuracy = src_stats->get(STAT_ACCURACY);

	// If the hazard's damage isn't default (0), we are applying an item-based power mod.
	// We don't allow equipment power mods to alter damage (mainly to preserve the base power's multiplier).
	if (haz->dmg_max == 0) {

		// base damage is by equipped item
		if (powers[power_index].base_damage == BASE_DAMAGE_MELEE) {
			haz->dmg_min = src_stats->get(STAT_DMG_MELEE_MIN);
			haz->dmg_max = src_stats->get(STAT_DMG_MELEE_MAX);
		}
		else if (powers[power_index].base_damage == BASE_DAMAGE_RANGED) {
			haz->dmg_min = src_stats->get(STAT_DMG_RANGED_MIN);
			haz->dmg_max = src_stats->get(STAT_DMG_RANGED_MAX);
		}
		else if (powers[power_index].base_damage == BASE_DAMAGE_MENT) {
			haz->dmg_min = src_stats->get(STAT_DMG_MENT_MIN);
			haz->dmg_max = src_stats->get(STAT_DMG_MENT_MAX);
		}
	}

	// Only apply stats from powers that are not defaults
	// If we do this, we can init with multiple power layers
	// (e.g. base spell plus weapon type)

	if (powers[power_index].animation_name != "")
		haz->loadAnimation(powers[power_index].animation_name);
	if (powers[power_index].lifespan != 0)
		haz->lifespan = powers[power_index].lifespan;
	if (powers[power_index].directional)
		haz->animationKind = calcDirection(src_stats->pos.x, src_stats->pos.y, target.x, target.y);
	else if (powers[power_index].visual_random)
		haz->animationKind = rand() % powers[power_index].visual_random;
	else if (powers[power_index].visual_option)
		haz->animationKind = powers[power_index].visual_option;

	haz->on_floor = powers[power_index].floor;
	haz->base_speed = powers[power_index].speed;
	haz->complete_animation = powers[power_index].complete_animation;

	// combat traits
	if (powers[power_index].radius != 0) {
		haz->radius = powers[power_index].radius;
	}
	if (powers[power_index].trait_elemental != -1) {
		haz->trait_elemental = powers[power_index].trait_elemental;
	}

	if (powers[power_index].no_attack) {
		haz->active = false;
	}

	// note: it may look like this line would be more efficient:
	// haz->multitarget = powers[power_index].multitarget
	// but as mentioned above, only apply traits that are not the default!
	// If haz->multitarget is already true, don't reset it to false.
	// otherwise a base power with multitarget will lose multitarget from a power mod
	// e.g. Piercing Shot in flare-game has multitarget
	// but the generic Arrow missile or Sling Stone missile does not.
	if (powers[power_index].multitarget) {
		haz->multitarget = true;
	}
	if (powers[power_index].trait_armor_penetration) {
		haz->trait_armor_penetration = true;
	}
	haz->trait_crits_impaired += powers[power_index].trait_crits_impaired;

	if (powers[power_index].beacon) {
		haz->beacon = true;
	}

	// status effect durations
	// steal effects
	haz->hp_steal += powers[power_index].hp_steal;
	haz->mp_steal += powers[power_index].mp_steal;

	// hazard starting position
	if (powers[power_index].starting_pos == STARTING_POS_SOURCE) {
		haz->pos = src_stats->pos;
	}
	else if (powers[power_index].starting_pos == STARTING_POS_TARGET) {
		haz->pos = limitRange(powers[power_index].target_range,src_stats->pos,target);
	}
	else if (powers[power_index].starting_pos == STARTING_POS_MELEE) {
		haz->pos = calcVector(src_stats->pos, src_stats->direction, src_stats->melee_range);
	}
	if (powers[power_index].target_neighbor > 0) {
		haz->pos = collider->get_random_neighbor(floor(src_stats->pos), powers[power_index].target_neighbor, true);
	}

	// pre/post power effects
	if (powers[power_index].post_power != 0) {
		haz->post_power = powers[power_index].post_power;
	}
	if (powers[power_index].wall_power != 0) {
		haz->wall_power = powers[power_index].wall_power;
	}
}

/**
 * Any attack-based effects are handled by hazards.
 * Self-enhancements (buffs) are handled by this function.
 */
void PowerManager::buff(int power_index, StatBlock *src_stats, FPoint target) {

	// teleport to the target location
	if (powers[power_index].buff_teleport) {
		target = limitRange(powers[power_index].target_range,src_stats->pos,target);
		if (powers[power_index].target_neighbor > 0) {
			FPoint new_target = collider->get_random_neighbor(floor(target), powers[power_index].target_neighbor);
			if (floor(new_target.x) == floor(target.x) && floor(new_target.y) == floor(target.y)) {
				src_stats->teleportation = false;
			}
			else {
				src_stats->teleportation = true;
				src_stats->teleport_destination.x = new_target.x;
				src_stats->teleport_destination.y = new_target.y;
			}
		}
		else {
			src_stats->teleportation = true;
			src_stats->teleport_destination.x = target.x;
			src_stats->teleport_destination.y = target.y;
		}
	}

	// handle all other effects
	if (powers[power_index].buff || (powers[power_index].buff_party && src_stats->hero_ally)) {
		int source_type = src_stats->hero ? SOURCE_TYPE_HERO : (src_stats->hero_ally ? SOURCE_TYPE_ALLY : SOURCE_TYPE_ENEMY);
		effect(src_stats, src_stats, power_index, source_type);
	}

	if (powers[power_index].buff_party && !powers[power_index].passive) {
		party_buffs.push(power_index);
	}


	// activate any post powers here if the power doesn't use a hazard
	// otherwise the post power will chain off the hazard itself
	if (!powers[power_index].use_hazard) {
		activate(powers[power_index].post_power, src_stats, src_stats->pos);
	}
}

/**
 * Play the sound effect for this power
 * Equipped items may have unique sounds
 */
void PowerManager::playSound(int power_index) {
	if (powers[power_index].sfx_index != -1)
		snd->play(sfx[powers[power_index].sfx_index]);
}

bool PowerManager::effect(StatBlock *src_stats, StatBlock *caster_stats, int power_index, int source_type) {
	for (unsigned i=0; i<powers[power_index].post_effects.size(); i++) {

		std::string effect_index = powers[power_index].post_effects[i].id;
		int magnitude = powers[power_index].post_effects[i].magnitude;
		int duration = powers[power_index].post_effects[i].duration;

		if (effects.find(effect_index) != effects.end()) {
			if (effects[effect_index].type == "shield") {
				// charge shield to max ment weapon damage * damage multiplier
				if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY)
					magnitude = caster_stats->get(STAT_DMG_MENT_MAX) * powers[power_index].mod_damage_value_min / 100;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ADD)
					magnitude = caster_stats->get(STAT_DMG_MENT_MAX) + powers[power_index].mod_damage_value_min;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ABSOLUTE)
					magnitude = randBetween(powers[power_index].mod_damage_value_min, powers[power_index].mod_damage_value_max);

				comb->addMessage(msg->get("+%d Shield",magnitude), src_stats->pos, COMBAT_MESSAGE_BUFF);
			}
			else if (effects[effect_index].type == "heal") {
				// heal for ment weapon damage * damage multiplier
				magnitude = randBetween(caster_stats->get(STAT_DMG_MENT_MIN), caster_stats->get(STAT_DMG_MENT_MAX));

				if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY)
					magnitude = magnitude * powers[power_index].mod_damage_value_min / 100;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ADD)
					magnitude += powers[power_index].mod_damage_value_min;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ABSOLUTE)
					magnitude = randBetween(powers[power_index].mod_damage_value_min, powers[power_index].mod_damage_value_max);

				comb->addMessage(msg->get("+%d HP",magnitude), src_stats->pos, COMBAT_MESSAGE_BUFF);
				src_stats->hp += magnitude;
				if (src_stats->hp > src_stats->get(STAT_HP_MAX)) src_stats->hp = src_stats->get(STAT_HP_MAX);
			}

			int passive_id = 0;
			if (powers[power_index].passive) passive_id = power_index;

			src_stats->effects.addEffect(effect_index, effects[effect_index].icon, duration, magnitude, effects[effect_index].type, effects[effect_index].animation, effects[effect_index].additive, false, powers[power_index].passive_trigger, effects[effect_index].render_above, passive_id, source_type);
		}

		// If there's a sound effect, play it here
		playSound(power_index);
	}

	return true;
}

/**
 * The activated power creates a static effect (not a moving hazard)
 *
 * @param power_index The activated power ID
 * @param src_stats The StatBlock of the power activator
 * @param target The mouse cursor position in map coordinates
 * return boolean true if successful
 */
bool PowerManager::fixed(int power_index, StatBlock *src_stats, FPoint target) {

	if (powers[power_index].use_hazard) {
		int delay_iterator = 0;
		for (int i=0; i < powers[power_index].count; i++) {
			Hazard *haz = new Hazard(collider);
			initHazard(power_index, src_stats, target, haz);

			// add optional delay
			haz->delay_frames = delay_iterator;
			delay_iterator += powers[power_index].delay;

			// Hazard memory is now the responsibility of HazardManager
			hazards.push(haz);
		}
	}

	buff(power_index, src_stats, target);

	// If there's a sound effect, play it here
	playSound(power_index);

	payPowerCost(power_index, src_stats);
	return true;
}

/**
 * The activated power creates a group of missile hazards (e.g. arrow, thrown knife, firebolt).
 * Each individual missile is a single animated hazard that travels from the caster position to the
 * mouse target position.
 *
 * @param power_index The activated power ID
 * @param src_stats The StatBlock of the power activator
 * @param target The mouse cursor position in map coordinates
 * return boolean true if successful
 */
bool PowerManager::missile(int power_index, StatBlock *src_stats, FPoint target) {
	const float pi = 3.1415926535898f;

	FPoint src;
	if (powers[power_index].starting_pos == STARTING_POS_TARGET) {
		src = target;
	}
	else {
		src = src_stats->pos;
	}

	// calculate polar coordinates angle
	float theta = calcTheta(src.x, src.y, target.x, target.y);

	int delay_iterator = 0;

	//generate hazards
	for (int i=0; i < powers[power_index].count; i++) {
		Hazard *haz = new Hazard(collider);

		//calculate individual missile angle
		float offset_angle = ((1.0f - powers[power_index].count)/2 + i) * (powers[power_index].missile_angle * pi / 180.0f);
		float variance = 0;
		if (powers[power_index].angle_variance != 0)
			variance = pow(-1.0f, (rand() % 2) - 1) * (rand() % powers[power_index].angle_variance) * pi / 180.0f; //random between 0 and angle_variance away
		float alpha = theta + offset_angle + variance;
		while (alpha >= pi+pi) alpha -= pi+pi;
		while (alpha < 0.0) alpha += pi+pi;

		initHazard(power_index, src_stats, target, haz);

		//calculate the missile velocity
		float speed_var = 0;
		if (powers[power_index].speed_variance != 0) {
			const float var = powers[power_index].speed_variance;
			speed_var = ((var * 2.0f * rand()) / RAND_MAX) - var;
		}
		haz->speed.x = (haz->base_speed + speed_var) * cos(alpha);
		haz->speed.y = (haz->base_speed + speed_var) * sin(alpha);

		// calculate direction based on trajectory, not actual target
		if (powers[power_index].directional)
			haz->animationKind = calcDirection(src.x, src.y, src.x + haz->speed.x, src.y + haz->speed.y);

		// add optional delay
		haz->delay_frames = delay_iterator;
		delay_iterator += powers[power_index].delay;

		hazards.push(haz);
	}

	payPowerCost(power_index, src_stats);

	playSound(power_index);
	return true;
}

/**
 * Repeaters are multiple hazards that spawn in a straight line
 */
bool PowerManager::repeater(int power_index, StatBlock *src_stats, FPoint target) {

	payPowerCost(power_index, src_stats);

	//initialize variables
	FPoint location_iterator;
	FPoint speed;
	int delay_iterator = 0;

	// calculate polar coordinates angle
	float theta = calcTheta(src_stats->pos.x, src_stats->pos.y, target.x, target.y);

	speed.x = powers[power_index].speed * cos(theta);
	speed.y = powers[power_index].speed * sin(theta);

	location_iterator = src_stats->pos;

	playSound(power_index);

	for (int i=0; i<powers[power_index].count; i++) {

		location_iterator.x += speed.x;
		location_iterator.y += speed.y;

		// only travels until it hits a wall
		if (collider->is_wall(location_iterator.x, location_iterator.y)) {
			break; // no more hazards
		}

		Hazard *haz = new Hazard(collider);
		initHazard(power_index, src_stats, target, haz);

		haz->pos = location_iterator;
		haz->delay_frames = delay_iterator;
		delay_iterator += powers[power_index].delay;

		hazards.push(haz);
	}

	return true;

}


/**
 * Spawn a creature. Does not create a hazard
 */
bool PowerManager::spawn(int power_index, StatBlock *src_stats, FPoint target) {

	// apply any buffs
	buff(power_index, src_stats, target);

	// If there's a sound effect, play it here
	playSound(power_index);

	Map_Enemy espawn;
	espawn.type = powers[power_index].spawn_type;
	espawn.summoner = src_stats;

	// enemy spawning position
	if (powers[power_index].starting_pos == STARTING_POS_SOURCE) {
		espawn.pos = src_stats->pos;
	}
	else if (powers[power_index].starting_pos == STARTING_POS_TARGET) {
		espawn.pos = target;
	}
	else if (powers[power_index].starting_pos == STARTING_POS_MELEE) {
		espawn.pos = calcVector(src_stats->pos, src_stats->direction, src_stats->melee_range);
	}

	if (powers[power_index].target_neighbor > 0) {
		espawn.pos = floor(collider->get_random_neighbor(floor(src_stats->pos), powers[power_index].target_neighbor));
	}

	espawn.direction = calcDirection(src_stats->pos.x, src_stats->pos.y, target.x, target.y);
	espawn.summon_power_index = power_index;
	espawn.hero_ally = src_stats->hero || src_stats->hero_ally;

	for (int i=0; i < powers[power_index].count; i++) {
		enemies.push(espawn);
	}
	payPowerCost(power_index, src_stats);

	return true;
}

/**
 * A simpler spawn routine for map events
 */
bool PowerManager::spawn(const std::string& enemy_type, Point target) {

	Map_Enemy espawn;

	espawn.type = enemy_type;
	espawn.pos = target;

	// quick spawns start facing a random direction
	espawn.direction = rand() % 8;

	enemies.push(espawn);
	return true;
}

/**
 * Transform into a creature. Fully replaces entity characteristics
 */
bool PowerManager::transform(int power_index, StatBlock *src_stats, FPoint target) {
	// locking the actionbar prevents power usage until after the hero is transformed
	inpt->lockActionBar();

	if (src_stats->transformed && powers[power_index].spawn_type != "untransform") {
		log_msg = msg->get("You are already transformed, untransform first.");
		return false;
	}

	// apply any buffs
	buff(power_index, src_stats, target);

	src_stats->manual_untransform = powers[power_index].manual_untransform;
	src_stats->transform_with_equipment = powers[power_index].keep_equipment;

	// If there's a sound effect, play it here
	playSound(power_index);

	// execute untransform powers
	if (powers[power_index].spawn_type == "untransform" && src_stats->transformed) {
		src_stats->transform_duration = 0;
		src_stats->transform_type = "untransform"; // untransform() is called only if type !=""
	}
	else {
		if (powers[power_index].transform_duration == 0) {
			// permanent transformation
			src_stats->transform_duration = -1;
		}
		else if (powers[power_index].transform_duration > 0) {
			// timed transformation
			src_stats->transform_duration = powers[power_index].transform_duration;
		}

		src_stats->transform_type = powers[power_index].spawn_type;
	}

	payPowerCost(power_index, src_stats);

	return true;
}


/**
 * Activate is basically a switch/redirect to the appropriate function
 */
bool PowerManager::activate(int power_index, StatBlock *src_stats, FPoint target) {

	if (src_stats->hero) {
		if (powers[power_index].requires_mp > src_stats->mp)
			return false;
	}

	if (src_stats->hp > 0 && powers[power_index].sacrifice == false && powers[power_index].requires_hp >= src_stats->hp)
		return false;

	// logic for different types of powers are very different.  We allow these
	// separate functions to handle the details.
	switch(powers[power_index].type) {
		case POWTYPE_FIXED:
			return fixed(power_index, src_stats, target);
		case POWTYPE_MISSILE:
			return missile(power_index, src_stats, target);
		case POWTYPE_REPEATER:
			return repeater(power_index, src_stats, target);
		case POWTYPE_SPAWN:
			return spawn(power_index, src_stats, target);
		case POWTYPE_TRANSFORM:
			return transform(power_index, src_stats, target);
	}

	return false;
}

/**
 * pay costs, i.e. remove mana or items.
 */
void PowerManager::payPowerCost(int power_index, StatBlock *src_stats) {
	if (src_stats) {
		if (src_stats->hero) {
			src_stats->mp -= powers[power_index].requires_mp;
			if (powers[power_index].requires_item != -1)
				used_items.push_back(powers[power_index].requires_item);
			// only allow one instance of duplicate items at a time in the used_equipped_items queue
			// this is useful for Ouroboros rings, where we have 2 equipped, but only want to remove one at a time
			if (powers[power_index].requires_equipped_item != -1 &&
					find(used_equipped_items.begin(), used_equipped_items.end(), powers[power_index].requires_equipped_item) == used_equipped_items.end())
				used_equipped_items.push_back(powers[power_index].requires_equipped_item);
		}
		src_stats->hp -= powers[power_index].requires_hp;
		src_stats->hp = (src_stats->hp < 0 ? 0 : src_stats->hp);
	}
}

/**
 * Activate an entity's passive powers
 */
void PowerManager::activatePassives(StatBlock *src_stats) {
	bool triggered_others = false;
	int trigger = -1;
	// unlocked powers
	for (unsigned i=0; i<src_stats->powers_passive.size(); i++) {
		if (powers[src_stats->powers_passive[i]].passive) {
			trigger = powers[src_stats->powers_passive[i]].passive_trigger;

			if (trigger == -1) {
				if (src_stats->effects.triggered_others) continue;
				else triggered_others = true;
			}
			else if (trigger == TRIGGER_BLOCK && !src_stats->effects.triggered_block) continue;
			else if (trigger == TRIGGER_HIT && !src_stats->effects.triggered_hit) continue;
			else if (trigger == TRIGGER_HALFDEATH && !src_stats->effects.triggered_halfdeath) {
				if (src_stats->hp > src_stats->get(STAT_HP_MAX)/2) continue;
				else src_stats->effects.triggered_halfdeath = true;
			}
			else if (trigger == TRIGGER_JOINCOMBAT && !src_stats->effects.triggered_joincombat) {
				if (!src_stats->in_combat) continue;
				else src_stats->effects.triggered_joincombat = true;
			}
			else if (trigger == TRIGGER_DEATH && !src_stats->effects.triggered_death) continue;

			activate(src_stats->powers_passive[i], src_stats, src_stats->pos);
			src_stats->refresh_stats = true;
		}
	}
	// item powers
	for (unsigned i=0; i<src_stats->powers_list_items.size(); i++) {
		if (powers[src_stats->powers_list_items[i]].passive) {
			trigger = powers[src_stats->powers_list_items[i]].passive_trigger;

			if (trigger == -1) {
				if (src_stats->effects.triggered_others) continue;
				else triggered_others = true;
			}
			else if (trigger == TRIGGER_BLOCK && !src_stats->effects.triggered_block) continue;
			else if (trigger == TRIGGER_HIT && !src_stats->effects.triggered_hit) continue;
			else if (trigger == TRIGGER_HALFDEATH && !src_stats->effects.triggered_halfdeath) {
				if (src_stats->hp > src_stats->get(STAT_HP_MAX)/2) continue;
				else src_stats->effects.triggered_halfdeath = true;
			}
			else if (trigger == TRIGGER_JOINCOMBAT && !src_stats->effects.triggered_joincombat) {
				if (!src_stats->in_combat) continue;
				else src_stats->effects.triggered_joincombat = true;
			}
			else if (trigger == TRIGGER_DEATH && !src_stats->effects.triggered_death) continue;

			activate(src_stats->powers_list_items[i], src_stats, src_stats->pos);
			src_stats->refresh_stats = true;
		}
	}
	// Only trigger normal passives once
	if (triggered_others) src_stats->effects.triggered_others = true;

	// the hit/death triggers can be triggered more than once, so reset them here
	// the block trigger is handled in the Avatar class
	src_stats->effects.triggered_hit = false;
	src_stats->effects.triggered_death = false;
}

/**
 * Activate a single passive
 * this is used when unlocking powers in MenuPowers
 */
void PowerManager::activateSinglePassive(StatBlock *src_stats, int id) {
	if (!powers[id].passive) return;

	if (powers[id].passive_trigger == -1) {
		activate(id, src_stats, src_stats->pos);
		src_stats->refresh_stats = true;
		src_stats->effects.triggered_others = true;
	}
}

PowerManager::~PowerManager() {

	for (unsigned i=0; i<sfx.size(); i++) {
		snd->unload(sfx[i]);
	}
	sfx.clear();
}

