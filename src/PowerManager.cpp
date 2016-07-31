/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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
 * class PowerManager
 */


#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "EventManager.h"
#include "FileParser.h"
#include "Hazard.h"
#include "MapCollision.h"
#include "PowerManager.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include <cmath>
#include <climits>

/**
 * PowerManager constructor
 */
PowerManager::PowerManager()
	: collider(NULL)
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

	while (infile.next()) {
		if (infile.new_section) {
			if (infile.section == "effect") {
				// check if the previous effect and remove it if there is no identifier
				if (!effects.empty() && effects.back().id == "") {
					effects.pop_back();
				}
				effects.resize(effects.size()+1);
			}
		}

		if (effects.empty() || infile.section != "effect")
			continue;

		if (infile.key == "id") {
			// @ATTR effect.id|string|Unique identifier for the effect definition.
			effects.back().id = infile.val;
		}
		else if (infile.key == "type") {
			// @ATTR effect.type|string|Defines the type of effect
			effects.back().type = infile.val;
		}
		else if (infile.key == "icon") {
			// @ATTR effect.icon|icon_id|The icon to visually represent the effect in the status area
			effects.back().icon = toInt(infile.val);
		}
		else if (infile.key == "animation") {
			// @ATTR effect.animation|filename|The filename of effect animation.
			effects.back().animation = infile.val;
		}
		else if (infile.key == "can_stack") {
			// @ATTR effect.can_stack|bool|Allows multiple instances of this effect
			effects.back().can_stack = toBool(infile.val);
		}
		else if (infile.key == "render_above") {
			// @ATTR effect.render_above|bool|Effect is rendered above
			effects.back().render_above = toBool(infile.val);
		}
		else {
			infile.error("PowerManager: '%s' is not a valid key.", infile.key.c_str());
		}
	}
	infile.close();

	// check if the last effect and remove it if there is no identifier
	if (!effects.empty() && effects.back().id == "") {
		effects.pop_back();
	}
}

void PowerManager::loadPowers() {
	FileParser infile;

	// @CLASS Powers|Description of powers/powers.txt
	if (!infile.open("powers/powers.txt"))
		return;

	bool clear_post_effects = true;

	int input_id = 0;
	bool skippingEntry = false;

	while (infile.next()) {
		// id needs to be the first component of each power.  That is how we write
		// data to the correct power.
		if (infile.key == "id") {
			// @ATTR power.id|power_id|Uniq identifier for the power definition.
			input_id = toInt(infile.val);
			skippingEntry = input_id < 1;
			if (skippingEntry)
				infile.error("PowerManager: Power index out of bounds 1-%d, skipping power.", INT_MAX);
			if (static_cast<int>(powers.size()) < input_id + 1)
				powers.resize(input_id + 1);

			clear_post_effects = true;

			continue;
		}
		if (skippingEntry)
			continue;

		if (infile.key == "type") {
			// @ATTR power.type|["fixed", "missile", "repeater", "spawn", "transform", "block"]|Defines the type of power definiton
			if (infile.val == "fixed") powers[input_id].type = POWTYPE_FIXED;
			else if (infile.val == "missile") powers[input_id].type = POWTYPE_MISSILE;
			else if (infile.val == "repeater") powers[input_id].type = POWTYPE_REPEATER;
			else if (infile.val == "spawn") powers[input_id].type = POWTYPE_SPAWN;
			else if (infile.val == "transform") powers[input_id].type = POWTYPE_TRANSFORM;
			else if (infile.val == "block") powers[input_id].type = POWTYPE_BLOCK;
			else infile.error("PowerManager: Unknown type '%s'", infile.val.c_str());
		}
		else if (infile.key == "name")
			// @ATTR power.name|string|The name of the power
			powers[input_id].name = msg->get(infile.val);
		else if (infile.key == "description")
			// @ATTR power.description|string|Description of the power
			powers[input_id].description = msg->get(infile.val);
		else if (infile.key == "icon")
			// @ATTR power.icon|icon_id|The icon to visually represent the power eg. in skill tree or action bar.
			powers[input_id].icon = toInt(infile.val);
		else if (infile.key == "new_state") {
			// @ATTR power.new_state|predefined_string|When power is used, hero or enemy will change to this state. Must be one of the states ["instant", user defined]
			if (infile.val == "instant") powers[input_id].new_state = POWSTATE_INSTANT;
			else {
				powers[input_id].new_state = POWSTATE_ATTACK;
				powers[input_id].attack_anim = infile.val;
			}
		}
		else if (infile.key == "state_duration") {
			// @ATTR power.state_duration|duration|Sets the length of time the caster is in their state animation. A time longer than the animation length will cause the animation to pause on the last frame. Times shorter than the state animation length will have no effect.
			powers[input_id].state_duration = parse_duration(infile.val);
		}
		else if (infile.key == "face")
			// @ATTR power.face|bool|Power will make hero or enemy to face the target location.
			powers[input_id].face = toBool(infile.val);
		else if (infile.key == "source_type") {
			// @ATTR power.source_type|["hero", "neutral", "enemy"]|Determines which entities the power can effect.
			if (infile.val == "hero") powers[input_id].source_type = SOURCE_TYPE_HERO;
			else if (infile.val == "neutral") powers[input_id].source_type = SOURCE_TYPE_NEUTRAL;
			else if (infile.val == "enemy") powers[input_id].source_type = SOURCE_TYPE_ENEMY;
			else infile.error("PowerManager: Unknown source_type '%s'", infile.val.c_str());
		}
		else if (infile.key == "beacon")
			// @ATTR power.beacon|bool|True if enemy is calling its allies.
			powers[input_id].beacon = toBool(infile.val);
		else if (infile.key == "count")
			// @ATTR power.count|int|The count of hazards/effect or spawns to be created by this power.
			powers[input_id].count = toInt(infile.val);
		else if (infile.key == "passive")
			// @ATTR power.passive|bool|If power is unlocked when the hero or enemy spawns it will be automatically activated.
			powers[input_id].passive = toBool(infile.val);
		else if (infile.key == "passive_trigger") {
			// @ATTR power.passive_trigger|["on_block", "on_hit", "on_halfdeath", "on_joincombat", "on_death"]|This will only activate a passive power under a certain condition.
			if (infile.val == "on_block") powers[input_id].passive_trigger = TRIGGER_BLOCK;
			else if (infile.val == "on_hit") powers[input_id].passive_trigger = TRIGGER_HIT;
			else if (infile.val == "on_halfdeath") powers[input_id].passive_trigger = TRIGGER_HALFDEATH;
			else if (infile.val == "on_joincombat") powers[input_id].passive_trigger = TRIGGER_JOINCOMBAT;
			else if (infile.val == "on_death") powers[input_id].passive_trigger = TRIGGER_DEATH;
			else infile.error("PowerManager: Unknown passive trigger '%s'", infile.val.c_str());
		}
		else if (infile.key == "meta_power") {
			// @ATTR power.meta_power|bool|If true, this power can not be used on it's own. Instead, it should be replaced via an item with a replace_power entry.
			powers[input_id].meta_power = toBool(infile.val);
		}
		// power requirements
		else if (infile.key == "requires_flags") {
			// @ATTR power.requires_flags|list(predefined_string)|A comma separated list of equip flags that are required to use this power. See engine/equip_flags.txt
			powers[input_id].requires_flags.clear();
			std::string flag = popFirstString(infile.val);

			while (flag != "") {
				powers[input_id].requires_flags.insert(flag);
				flag = popFirstString(infile.val);
			}
		}
		else if (infile.key == "requires_mp")
			// @ATTR power.requires_mp|int|Restrict power usage to a specified MP level.
			powers[input_id].requires_mp = toInt(infile.val);
		else if (infile.key == "requires_hp")
			// @ATTR power.requires_hp|int|Restrict power usage to a specified HP level.
			powers[input_id].requires_hp = toInt(infile.val);
		else if (infile.key == "sacrifice")
			// @ATTR power.sacrifice|bool|If the power has requires_hp, allow it to kill the caster.
			powers[input_id].sacrifice = toBool(infile.val);
		else if (infile.key == "requires_los")
			// @ATTR power.requires_los|bool|Requires a line-of-sight to target.
			powers[input_id].requires_los = toBool(infile.val);
		else if (infile.key == "requires_empty_target")
			// @ATTR power.requires_empty_target|bool|The power can only be cast when target tile is empty.
			powers[input_id].requires_empty_target = toBool(infile.val);
		else if (infile.key == "requires_item") {
			// @ATTR power.requires_item|item_id, int : Item, Quantity|Requires a specific item of a specific quantity in inventory.
			powers[input_id].requires_item = popFirstInt(infile.val);
			powers[input_id].requires_item_quantity = toInt(popFirstString(infile.val), 1);
		}
		else if (infile.key == "requires_equipped_item") {
			// @ATTR power.requires_equipped_item|item_id, int : Item, Quantity|Requires a specific item of a specific quantity to be equipped on hero.
			powers[input_id].requires_equipped_item = popFirstInt(infile.val);
			powers[input_id].requires_equipped_item_quantity = popFirstInt(infile.val);

			// a maximum of 1 equipped item can be consumed at a time
			if (powers[input_id].requires_equipped_item_quantity > 1) {
				infile.error("PowerManager: Only 1 equipped item can be consumed at a time.");
				clampCeil(powers[input_id].requires_equipped_item_quantity, 1);
			}
		}
		else if (infile.key == "requires_targeting")
			// @ATTR power.requires_targeting|bool|Power is only used when targeting using click-to-target.
			powers[input_id].requires_targeting = toBool(infile.val);
		else if (infile.key == "requires_spawns")
			// @ATTR power.requires_spawns|int|The caster must have at least this many summoned creatures to use this power.
			powers[input_id].requires_spawns = toInt(infile.val);
		else if (infile.key == "cooldown")
			// @ATTR power.cooldown|duration|Specify the duration for cooldown of the power in 'ms' or 's'.
			powers[input_id].cooldown = parse_duration(infile.val);
		// animation info
		else if (infile.key == "animation")
			// @ATTR power.animation|filename|The filename of the power animation.
			powers[input_id].animation_name = infile.val;
		else if (infile.key == "soundfx")
			// @ATTR power.soundfx|filename|Filename of a sound effect to play when the power is used.
			powers[input_id].sfx_index = loadSFX(infile.val);
		else if (infile.key == "soundfx_hit") {
			// @ATTR power.soundfx_hit|filename|Filename of a sound effect to play when the power's hazard hits a valid target.
			int sfx_id = loadSFX(infile.val);
			if (sfx_id != -1) {
				powers[input_id].sfx_hit = sfx[sfx_id];
				powers[input_id].sfx_hit_enable = true;
			}
		}
		else if (infile.key == "directional")
			// @ATTR power.directional|bool|The animation sprite sheet contains 8 directions, one per row.
			powers[input_id].directional = toBool(infile.val);
		else if (infile.key == "visual_random")
			// @ATTR power.visual_random|int|The animation sprite sheet contains rows of random options
			powers[input_id].visual_random = toInt(infile.val);
		else if (infile.key == "visual_option")
			// @ATTR power.visual_option|int|The animation sprite sheet containers rows of similar effects, use a specific option.
			powers[input_id].visual_option = toInt(infile.val);
		else if (infile.key == "aim_assist")
			// @ATTR power.aim_assist|bool|Power is aim assisted.
			powers[input_id].aim_assist = toBool(infile.val);
		else if (infile.key == "speed")
			// @ATTR power.speed|float|The speed of missile hazard, the unit is defined as map units per frame.
			powers[input_id].speed = toFloat(infile.val) / MAX_FRAMES_PER_SEC;
		else if (infile.key == "lifespan")
			// @ATTR power.lifespan|duration|How long the hazard/animation lasts in 'ms' or 's'.
			powers[input_id].lifespan = parse_duration(infile.val);
		else if (infile.key == "floor")
			// @ATTR power.floor|bool|The hazard is drawn between the background and the object layer.
			powers[input_id].floor = toBool(infile.val);
		else if (infile.key == "complete_animation")
			// @ATTR power.complete_animation|bool|For hazards; Play the entire animation, even if the hazard has hit a target.
			powers[input_id].complete_animation = toBool(infile.val);
		else if (infile.key == "charge_speed")
			// @ATTR power.charge_speed|float|Moves the caster at this speed in the direction they are facing until the state animation is finished.
			powers[input_id].charge_speed = toFloat(infile.val) / MAX_FRAMES_PER_SEC;
		// hazard traits
		else if (infile.key == "use_hazard")
			// @ATTR power.use_hazard|bool|Power uses hazard.
			powers[input_id].use_hazard = toBool(infile.val);
		else if (infile.key == "no_attack")
			// @ATTR power.no_attack|bool|Hazard won't affect other entities.
			powers[input_id].no_attack = toBool(infile.val);
		else if (infile.key == "radius")
			// @ATTR power.radius|float|Radius in pixels
			powers[input_id].radius = toFloat(infile.val);
		else if (infile.key == "base_damage") {
			// @ATTR power.base_damage|["melee", "ranged", "ment"]|Determines which of the three primary damage stats will be used to calculate damage.
			if (infile.val == "none")        powers[input_id].base_damage = BASE_DAMAGE_NONE;
			else if (infile.val == "melee")  powers[input_id].base_damage = BASE_DAMAGE_MELEE;
			else if (infile.val == "ranged") powers[input_id].base_damage = BASE_DAMAGE_RANGED;
			else if (infile.val == "ment")   powers[input_id].base_damage = BASE_DAMAGE_MENT;
			else infile.error("PowerManager: Unknown base_damage '%s'", infile.val.c_str());
		}
		else if (infile.key == "starting_pos") {
			// @ATTR power.starting_pos|["source", "target", "melee"]|Start position for hazard
			if (infile.val == "source")      powers[input_id].starting_pos = STARTING_POS_SOURCE;
			else if (infile.val == "target") powers[input_id].starting_pos = STARTING_POS_TARGET;
			else if (infile.val == "melee")  powers[input_id].starting_pos = STARTING_POS_MELEE;
			else infile.error("PowerManager: Unknown starting_pos '%s'", infile.val.c_str());
		}
		else if (infile.key == "relative_pos") {
			// @ATTR power.relative_pos|bool|Hazard will move relative to the caster's position.
			powers[input_id].relative_pos = toBool(infile.val);
		}
		else if (infile.key == "multitarget")
			// @ATTR power.multitarget|bool|Allows a hazard power to hit more than one entity.
			powers[input_id].multitarget = toBool(infile.val);
		else if (infile.key == "multihit")
			// @ATTR power.multihit|bool|Allows a hazard power to hit the same entity more than once.
			powers[input_id].multihit = toBool(infile.val);
		else if (infile.key == "expire_with_caster")
			// @ATTR power.expire_with_caster|bool|If true, hazard will disappear when the caster dies.
			powers[input_id].expire_with_caster = toBool(infile.val);
		else if (infile.key == "trait_armor_penetration")
			// @ATTR power.trait_armor_penetration|bool|Ignores the target's Absorbtion stat
			powers[input_id].trait_armor_penetration = toBool(infile.val);
		else if (infile.key == "trait_avoidance_ignore")
			// @ATTR power.trait_avoidance_ignore|bool|Ignores the target's Avoidance stat
			powers[input_id].trait_avoidance_ignore = toBool(infile.val);
		else if (infile.key == "trait_crits_impaired")
			// @ATTR power.trait_crits_impaired|int|Increases critical hit percentage for slowed/immobile targets
			powers[input_id].trait_crits_impaired = toInt(infile.val);
		else if (infile.key == "trait_elemental") {
			// @ATTR power.trait_elemental|predefined_string|Damage done is elemental. See engine/elements.txt
			for (unsigned int i=0; i<ELEMENTS.size(); i++) {
				if (infile.val == ELEMENTS[i].id) powers[input_id].trait_elemental = i;
			}
		}
		else if (infile.key == "target_range")
			// @ATTR power.target_range|float|The distance from the caster that the power can be activated
			powers[input_id].target_range = toFloat(popFirstString(infile.val));
		//steal effects
		else if (infile.key == "hp_steal")
			// @ATTR power.hp_steal|int|Percentage of damage to steal into HP
			powers[input_id].hp_steal = toInt(infile.val);
		else if (infile.key == "mp_steal")
			// @ATTR power.mp_steal|int|Percentage of damage to steal into MP
			powers[input_id].mp_steal = toInt(infile.val);
		//missile modifiers
		else if (infile.key == "missile_angle")
			// @ATTR power.missile_angle|int|Angle of missile
			powers[input_id].missile_angle = toInt(infile.val);
		else if (infile.key == "angle_variance")
			// @ATTR power.angle_variance|int|Percentage of variance added to missile angle
			powers[input_id].angle_variance = toInt(infile.val);
		else if (infile.key == "speed_variance")
			// @ATTR power.speed_variance|float|Percentage of variance added to missile speed
			powers[input_id].speed_variance = toFloat(infile.val);
		//repeater modifiers
		else if (infile.key == "delay")
			// @ATTR power.delay|duration|Delay between repeats in 'ms' or 's'.
			powers[input_id].delay = parse_duration(infile.val);
		// buff/debuff durations
		else if (infile.key == "transform_duration")
			// @ATTR power.transform_duration|duration|Duration for transform in 'ms' or 's'.
			powers[input_id].transform_duration = parse_duration(infile.val);
		else if (infile.key == "manual_untransform")
			// @ATTR power.manual_untransform|bool|Force manual untranform
			powers[input_id].manual_untransform = toBool(infile.val);
		else if (infile.key == "keep_equipment")
			// @ATTR power.keep_equipment|bool|Keep equipment while transformed
			powers[input_id].keep_equipment = toBool(infile.val);
		else if (infile.key == "untransform_on_hit")
			// @ATTR power.untransform_on_hit|bool|Force untransform when the player is hit
			powers[input_id].untransform_on_hit = toBool(infile.val);
		// buffs
		else if (infile.key == "buff")
			// @ATTR power.buff|bool|Power is cast upon the caster.
			powers[input_id].buff= toBool(infile.val);
		else if (infile.key == "buff_teleport")
			// @ATTR power.buff_teleport|bool|Power is a teleportation power.
			powers[input_id].buff_teleport = toBool(infile.val);
		else if (infile.key == "buff_party")
			// @ATTR power.buff_party|bool|Power is cast upon party members
			powers[input_id].buff_party = toBool(infile.val);
		else if (infile.key == "buff_party_power_id")
			// @ATTR power.buff_party_power_id|power_id|Buffs a power id for all party members
			powers[input_id].buff_party_power_id = toInt(infile.val);
		else if (infile.key == "post_effect") {
			// @ATTR power.post_effect|predefined_string, int, duration : Effect ID, Magnitude, Duration|Post effect. Duration is in 'ms' or 's'.
			if (clear_post_effects) {
				powers[input_id].post_effects.clear();
				clear_post_effects = false;
			}
			PostEffect pe;
			pe.id = popFirstString(infile.val);
			if (!isValidEffect(pe.id)) {
				infile.error("PowerManager: Unknown effect '%s'", pe.id.c_str());
			}
			else {
				pe.magnitude = popFirstInt(infile.val);
				pe.duration = parse_duration(popFirstString(infile.val));
				powers[input_id].post_effects.push_back(pe);
			}
		}
		// pre and post power effects
		else if (infile.key == "post_power")
			// @ATTR power.post_power|power_id|Trigger a power if the hazard did damage.
			powers[input_id].post_power = toInt(infile.val);
		else if (infile.key == "wall_power")
			// @ATTR power.wall_power|power_id|Trigger a power if the hazard hit a wall.
			powers[input_id].wall_power = toInt(infile.val);
		// spawn info
		else if (infile.key == "spawn_type")
			// @ATTR power.spawn_type|predefined_string|For non-transform powers, an enemy is spawned from this category. For transform powers, the caster will transform into a creature from this category.
			powers[input_id].spawn_type = infile.val;
		else if (infile.key == "target_neighbor")
			// @ATTR power.target_neighbor|int|Target is changed to an adjacent tile within a radius.
			powers[input_id].target_neighbor = toInt(infile.val);
		else if (infile.key == "spawn_limit") {
			// @ATTR power.spawn_limit|["fixed", "stat", "unlimited"], [int, predefined_string] : Mode, Value|The maximum number of creatures that can be spawned and alive from this power. "fixed" takes an integer. "stat" takes a primary stat as a string (e.g. "physical").
			std::string mode = popFirstString(infile.val);
			if (mode == "fixed") powers[input_id].spawn_limit_mode = SPAWN_LIMIT_MODE_FIXED;
			else if (mode == "stat") powers[input_id].spawn_limit_mode = SPAWN_LIMIT_MODE_STAT;
			else if (mode == "unlimited") powers[input_id].spawn_limit_mode = SPAWN_LIMIT_MODE_UNLIMITED;
			else infile.error("PowerManager: Unknown spawn_limit_mode '%s'", mode.c_str());

			if(powers[input_id].spawn_limit_mode != SPAWN_LIMIT_MODE_UNLIMITED) {
				powers[input_id].spawn_limit_qty = popFirstInt(infile.val);

				if(powers[input_id].spawn_limit_mode == SPAWN_LIMIT_MODE_STAT) {
					powers[input_id].spawn_limit_every = popFirstInt(infile.val);

					std::string stat = popFirstString(infile.val);
					if (stat == "physical") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_PHYSICAL;
					else if (stat == "mental") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_MENTAL;
					else if (stat == "offense") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_OFFENSE;
					else if (stat == "defense") powers[input_id].spawn_limit_stat = SPAWN_LIMIT_STAT_DEFENSE;
					else infile.error("PowerManager: Unknown spawn_limit_stat '%s'", stat.c_str());
				}
			}
		}
		else if (infile.key == "spawn_level") {
			// @ATTR power.spawn_limit|["default", "fixed", "stat", "level"], [int, predefined_string] : Mode, Value|The level of spawned creatures. "fixed" and "level" take an integer. "stat" takes a primary stat as a string (e.g. "physical").
			std::string mode = popFirstString(infile.val);
			if (mode == "default") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_DEFAULT;
			else if (mode == "fixed") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_FIXED;
			else if (mode == "stat") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_STAT;
			else if (mode == "level") powers[input_id].spawn_level_mode = SPAWN_LEVEL_MODE_LEVEL;
			else infile.error("PowerManager: Unknown spawn_level_mode '%s'", mode.c_str());

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
						else infile.error("PowerManager: Unknown spawn_level_stat '%s'", stat.c_str());
					}
				}
			}
		}
		else if (infile.key == "target_party")
			// @ATTR power.target_party|bool|Hazard will only affect party members.
			powers[input_id].target_party = toBool(infile.val);
		else if (infile.key == "target_categories") {
			// @ATTR power.target_categories|list(predefined_string)|Hazard will only affect enemies in these categories.
			powers[input_id].target_categories.clear();
			std::string cat;
			while ((cat = popFirstString(infile.val)) != "") {
				powers[input_id].target_categories.push_back(cat);
			}
		}
		else if (infile.key == "modifier_accuracy") {
			// @ATTR power.modifier_accuracy|["multiply", "add", "absolute"], int : Mode, Value|Changes this power's accuracy.
			std::string mode = popFirstString(infile.val);
			if(mode == "multiply") powers[input_id].mod_accuracy_mode = STAT_MODIFIER_MODE_MULTIPLY;
			else if(mode == "add") powers[input_id].mod_accuracy_mode = STAT_MODIFIER_MODE_ADD;
			else if(mode == "absolute") powers[input_id].mod_accuracy_mode = STAT_MODIFIER_MODE_ABSOLUTE;
			else infile.error("PowerManager: Unknown stat_modifier_mode '%s'", mode.c_str());

			powers[input_id].mod_accuracy_value = popFirstInt(infile.val);
		}
		else if (infile.key == "modifier_damage") {
			// @ATTR power.modifier_damage|["multiply", "add", "absolute"], int, int : Mode, Min, Max|Changes this power's damage. The "Max" value is ignored, except in the case of "absolute" modifiers.
			std::string mode = popFirstString(infile.val);
			if(mode == "multiply") powers[input_id].mod_damage_mode = STAT_MODIFIER_MODE_MULTIPLY;
			else if(mode == "add") powers[input_id].mod_damage_mode = STAT_MODIFIER_MODE_ADD;
			else if(mode == "absolute") powers[input_id].mod_damage_mode = STAT_MODIFIER_MODE_ABSOLUTE;
			else infile.error("PowerManager: Unknown stat_modifier_mode '%s'", mode.c_str());

			powers[input_id].mod_damage_value_min = popFirstInt(infile.val);
			powers[input_id].mod_damage_value_max = popFirstInt(infile.val);
		}
		else if (infile.key == "modifier_critical") {
			// @ATTR power.modifier_critical|["multiply", "add", "absolute"], int : Mode, Value|Changes the chance that this power will land a critical hit.
			std::string mode = popFirstString(infile.val);
			if(mode == "multiply") powers[input_id].mod_crit_mode = STAT_MODIFIER_MODE_MULTIPLY;
			else if(mode == "add") powers[input_id].mod_crit_mode = STAT_MODIFIER_MODE_ADD;
			else if(mode == "absolute") powers[input_id].mod_crit_mode = STAT_MODIFIER_MODE_ABSOLUTE;
			else infile.error("PowerManager: Unknown stat_modifier_mode '%s'", mode.c_str());

			powers[input_id].mod_crit_value = popFirstInt(infile.val);
		}
		else if (infile.key == "target_movement_normal") {
			// @ATTR power.target_movement_normal|bool|Power can affect entities with normal movement (aka walking on ground)
			powers[input_id].target_movement_normal = toBool(infile.val);
		}
		else if (infile.key == "target_movement_flying") {
			// @ATTR power.target_movement_flying|bool|Power can affect flying entities
			powers[input_id].target_movement_flying = toBool(infile.val);
		}
		else if (infile.key == "target_movement_intangible") {
			// @ATTR power.target_movement_intangible|bool|Power can affect intangible entities
			powers[input_id].target_movement_intangible = toBool(infile.val);
		}
		else if (infile.key == "walls_block_aoe") {
			// @ATTR power.walls_block_aoe|bool|When true, prevents hazard aoe from hitting targets that are behind walls/pits.
			powers[input_id].walls_block_aoe = toBool(infile.val);
		}
		else if (infile.key == "script") {
			// @ATTR power.script|["on_cast", "on_hit", "on_wall"], filename : Trigger, Filename|Loads and executes a script file when the trigger is activated.
			std::string trigger = popFirstString(infile.val);
			if (trigger == "on_cast") powers[input_id].script_trigger = SCRIPT_TRIGGER_CAST;
			else if (trigger == "on_hit") powers[input_id].script_trigger = SCRIPT_TRIGGER_HIT;
			else if (trigger == "on_wall") powers[input_id].script_trigger = SCRIPT_TRIGGER_WALL;
			else infile.error("PowerManager: Unknown script trigger '%s'", trigger.c_str());

			powers[input_id].script = popFirstString(infile.val);
		}

		else infile.error("PowerManager: '%s' is not a valid key", infile.key.c_str());
	}
	infile.close();

	// verify wall/post power ids
	for (size_t i=0; i<powers.size(); ++i) {
		powers[i].wall_power = verifyID(powers[i].wall_power);
		powers[i].post_power = verifyID(powers[i].post_power);
	}
}

bool PowerManager::isValidEffect(const std::string& type) {
	if (type == "speed")
		return true;

	if (type == "physical")
		return true;
	if (type == "mental")
		return true;
	if (type == "offense")
		return true;
	if (type == "defense")
		return true;

	for (int i=0; i<STAT_COUNT; ++i) {
		if (type == STAT_KEY[i])
			return true;
	}

	for (unsigned i=0; i<ELEMENTS.size(); ++i) {
		if (type == ELEMENTS[i].id + "_resist")
			return true;
	}

	if (getEffectDef(type) != NULL)
		return true;

	return false;
}

/**
 * Load the specified sound effect for this power
 *
 * @param filename The .ogg file containing the sound for this power, assumed to be in soundfx/powers/
 * @return The sfx[] array index for this mix chunk, or -1 upon load failure
 */
int PowerManager::loadSFX(const std::string& filename) {

	SoundManager::SoundID sid = snd->load(filename, "PowerManager sfx");
	std::vector<SoundManager::SoundID>::iterator it = std::find(sfx.begin(), sfx.end(), sid);
	if (it == sfx.end()) {
		sfx.push_back(sid);
		return static_cast<int>(sfx.size()) - 1;
	}

	return static_cast<int>(it - sfx.begin());
}


/**
 * Set new collision object
 */
void PowerManager::handleNewMap(MapCollision *_collider) {
	collider = _collider;
}

/**
 * Check if the target is valid (not an empty area or a wall)
 */
bool PowerManager::hasValidTarget(int power_index, StatBlock *src_stats, const FPoint& target) {

	if (!collider) return false;

	FPoint limit_target = clampDistance(powers[power_index].target_range,src_stats->pos,target);

	if (!collider->is_empty(limit_target.x, limit_target.y) || collider->is_wall(limit_target.x,limit_target.y)) {
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
void PowerManager::initHazard(int power_index, StatBlock *src_stats, const FPoint& target, Hazard *haz) {

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

	// animation properties
	if (powers[power_index].animation_name != "") {
		haz->loadAnimation(powers[power_index].animation_name);
	}

	if (powers[power_index].directional) {
		haz->directional = powers[power_index].directional;
		haz->animationKind = calcDirection(src_stats->pos.x, src_stats->pos.y, target.x, target.y);
	}
	else if (powers[power_index].visual_random) {
		haz->animationKind = rand() % powers[power_index].visual_random;
	}
	else if (powers[power_index].visual_option) {
		haz->animationKind = powers[power_index].visual_option;
	}

	haz->base_lifespan = haz->lifespan = powers[power_index].lifespan;
	haz->on_floor = powers[power_index].floor;
	haz->base_speed = powers[power_index].speed;
	haz->complete_animation = powers[power_index].complete_animation;

	// combat traits
	haz->radius = powers[power_index].radius;
	haz->trait_elemental = powers[power_index].trait_elemental;
	haz->active = !powers[power_index].no_attack;

	haz->multitarget = powers[power_index].multitarget;
	haz->multihit = powers[power_index].multihit;
	haz->expire_with_caster = powers[power_index].expire_with_caster;
	haz->trait_armor_penetration = powers[power_index].trait_armor_penetration;
	haz->trait_crits_impaired += powers[power_index].trait_crits_impaired;

	haz->beacon = powers[power_index].beacon;

	// steal effects
	haz->hp_steal += powers[power_index].hp_steal;
	haz->mp_steal += powers[power_index].mp_steal;

	// hazard starting position
	if (powers[power_index].starting_pos == STARTING_POS_SOURCE) {
		haz->pos = src_stats->pos;
	}
	else if (powers[power_index].starting_pos == STARTING_POS_TARGET) {
		haz->pos = clampDistance(powers[power_index].target_range,src_stats->pos,target);
	}
	else if (powers[power_index].starting_pos == STARTING_POS_MELEE) {
		haz->pos = calcVector(src_stats->pos, src_stats->direction, src_stats->melee_range);
	}

	if (powers[power_index].target_neighbor > 0) {
		haz->pos = collider->get_random_neighbor(FPointToPoint(src_stats->pos), powers[power_index].target_neighbor, true);
	}

	if (powers[power_index].relative_pos) {
		haz->relative_pos = true;
		haz->pos_offset.x = src_stats->pos.x - haz->pos.x;
		haz->pos_offset.y = src_stats->pos.y - haz->pos.y;
	}

	// pre/post power effects
	haz->post_power = powers[power_index].post_power;
	haz->wall_power = powers[power_index].wall_power;

	// flag missile powers for reflection
	haz->missile = (powers[power_index].type == POWTYPE_MISSILE);

	// targeting by movement type
	haz->target_movement_normal = powers[power_index].target_movement_normal;
	haz->target_movement_flying = powers[power_index].target_movement_flying;
	haz->target_movement_intangible = powers[power_index].target_movement_intangible;

	haz->walls_block_aoe = powers[power_index].walls_block_aoe;

	if (powers[power_index].sfx_hit_enable) {
		haz->sfx_hit = powers[power_index].sfx_hit;
		haz->sfx_hit_enable = powers[power_index].sfx_hit_enable;
	}

	if (powers[power_index].script_trigger != -1) {
		haz->script_trigger = powers[power_index].script_trigger;
		haz->script = powers[power_index].script;
	}
}

/**
 * Any attack-based effects are handled by hazards.
 * Self-enhancements (buffs) are handled by this function.
 */
void PowerManager::buff(int power_index, StatBlock *src_stats, const FPoint& target) {

	// teleport to the target location
	if (powers[power_index].buff_teleport) {
		FPoint limit_target = clampDistance(powers[power_index].target_range,src_stats->pos,target);
		if (powers[power_index].target_neighbor > 0) {
			FPoint new_target = collider->get_random_neighbor(FPointToPoint(limit_target), powers[power_index].target_neighbor);
			if (floor(new_target.x) == floor(limit_target.x) && floor(new_target.y) == floor(limit_target.y)) {
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
			src_stats->teleport_destination.x = limit_target.x;
			src_stats->teleport_destination.y = limit_target.y;
		}
	}

	// handle all other effects
	if (powers[power_index].buff || (powers[power_index].buff_party && (src_stats->hero_ally || src_stats->enemy_ally))) {
		int source_type = src_stats->hero ? SOURCE_TYPE_HERO : (src_stats->hero_ally ? SOURCE_TYPE_ALLY : SOURCE_TYPE_ENEMY);
		effect(src_stats, src_stats, power_index, source_type);
	}

	if (powers[power_index].buff_party && !powers[power_index].passive) {
		src_stats->party_buffs.push(power_index);
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

		EffectDef effect_data;
		EffectDef* effect_ptr = getEffectDef(powers[power_index].post_effects[i].id);

		int magnitude = powers[power_index].post_effects[i].magnitude;
		int duration = powers[power_index].post_effects[i].duration;

		if (effect_ptr != NULL) {
			// effects loaded from powers/effects.txt
			effect_data = (*effect_ptr);

			if (effect_data.type == "shield") {
				// charge shield to max ment weapon damage * damage multiplier
				if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY)
					magnitude = caster_stats->get(STAT_DMG_MENT_MAX) * powers[power_index].mod_damage_value_min / 100;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ADD)
					magnitude = caster_stats->get(STAT_DMG_MENT_MAX) + powers[power_index].mod_damage_value_min;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ABSOLUTE)
					magnitude = randBetween(powers[power_index].mod_damage_value_min, powers[power_index].mod_damage_value_max);
				else
					magnitude = caster_stats->get(STAT_DMG_MENT_MAX);

				comb->addString(msg->get("+%d Shield",magnitude), src_stats->pos, COMBAT_MESSAGE_BUFF);
			}
			else if (effect_data.type == "heal") {
				// heal for ment weapon damage * damage multiplier
				magnitude = randBetween(caster_stats->get(STAT_DMG_MENT_MIN), caster_stats->get(STAT_DMG_MENT_MAX));

				if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY)
					magnitude = magnitude * powers[power_index].mod_damage_value_min / 100;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ADD)
					magnitude += powers[power_index].mod_damage_value_min;
				else if(powers[power_index].mod_damage_mode == STAT_MODIFIER_MODE_ABSOLUTE)
					magnitude = randBetween(powers[power_index].mod_damage_value_min, powers[power_index].mod_damage_value_max);

				comb->addString(msg->get("+%d HP",magnitude), src_stats->pos, COMBAT_MESSAGE_BUFF);
				src_stats->hp += magnitude;
				if (src_stats->hp > src_stats->get(STAT_HP_MAX)) src_stats->hp = src_stats->get(STAT_HP_MAX);
			}
			else if (effect_data.type == "knockback") {
				if (src_stats->speed_default == 0) {
					// enemies that can't move can't be knocked back
					continue;
				}
				src_stats->knockback_srcpos = caster_stats->pos;
				src_stats->knockback_destpos = src_stats->pos;
			}
		}
		else {
			// all other effects
			effect_data.id = effect_data.type = powers[power_index].post_effects[i].id;
		}

		int passive_id = 0;
		if (powers[power_index].passive) passive_id = power_index;

		src_stats->effects.addEffect(effect_data, duration, magnitude, false, powers[power_index].passive_trigger, passive_id, source_type);
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
bool PowerManager::fixed(int power_index, StatBlock *src_stats, const FPoint& target) {

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
bool PowerManager::missile(int power_index, StatBlock *src_stats, const FPoint& target) {
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
		initHazard(power_index, src_stats, target, haz);

		//calculate individual missile angle
		float offset_angle = ((1.0f - static_cast<float>(powers[power_index].count))/2 + static_cast<float>(i)) * (static_cast<float>(powers[power_index].missile_angle) * static_cast<float>(M_PI) / 180.0f);
		float variance = 0;
		if (powers[power_index].angle_variance != 0) {
			//random between 0 and angle_variance away
			variance = static_cast<float>(pow(-1.0f, (rand() % 2) - 1) * (rand() % powers[power_index].angle_variance) * M_PI / 180.0f);
		}
		float alpha = theta + offset_angle + variance;

		//calculate the missile velocity
		float speed_var = 0;
		if (powers[power_index].speed_variance != 0) {
			const float var = powers[power_index].speed_variance;
			speed_var = ((var * 2.0f * static_cast<float>(rand())) / static_cast<float>(RAND_MAX)) - var;
		}

		// set speed and angle
		haz->base_speed += speed_var;
		haz->setAngle(alpha);

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
bool PowerManager::repeater(int power_index, StatBlock *src_stats, const FPoint& target) {

	payPowerCost(power_index, src_stats);

	//initialize variables
	FPoint location_iterator;
	FPoint speed;
	int delay_iterator = 0;

	// calculate polar coordinates angle
	float theta = calcTheta(src_stats->pos.x, src_stats->pos.y, target.x, target.y);

	speed.x = powers[power_index].speed * cosf(theta);
	speed.y = powers[power_index].speed * sinf(theta);

	location_iterator = src_stats->pos;

	playSound(power_index);

	Hazard* parent_haz = NULL;
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

		if (i == 0 && powers[power_index].count > 1) {
			parent_haz = haz;
		}
		else if (parent_haz != NULL && i > 0) {
			haz->parent = parent_haz;
			parent_haz->children.push_back(haz);
		}

		hazards.push(haz);
	}

	return true;

}


/**
 * Spawn a creature. Does not create a hazard
 */
bool PowerManager::spawn(int power_index, StatBlock *src_stats, const FPoint& target) {
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

	// force target_neighbor if our initial target is blocked
	int target_neighbor = powers[power_index].target_neighbor;
	if (!collider->is_empty(espawn.pos.x, espawn.pos.y) && target_neighbor < 1) {
		target_neighbor = 1;
	}

	if (target_neighbor > 0) {
		espawn.pos = FPointToPoint(collider->get_random_neighbor(FPointToPoint(src_stats->pos), target_neighbor));
	}

	// can't spawn on a blocked tile
	if (!collider->is_empty(espawn.pos.x, espawn.pos.y)) {
		return false;
	}

	espawn.direction = calcDirection(src_stats->pos.x, src_stats->pos.y, target.x, target.y);
	espawn.summon_power_index = power_index;
	espawn.hero_ally = src_stats->hero || src_stats->hero_ally;
	espawn.enemy_ally = !src_stats->hero;

	for (int i=0; i < powers[power_index].count; i++) {
		enemies.push(espawn);
	}
	payPowerCost(power_index, src_stats);

	// apply any buffs
	buff(power_index, src_stats, target);

	// If there's a sound effect, play it here
	playSound(power_index);

	return true;
}

/**
 * Transform into a creature. Fully replaces entity characteristics
 */
bool PowerManager::transform(int power_index, StatBlock *src_stats, const FPoint& target) {
	// locking the actionbar prevents power usage until after the hero is transformed
	inpt->lockActionBar();

	if (src_stats->transformed && powers[power_index].spawn_type != "untransform") {
		pc->logMsg(msg->get("You are already transformed, untransform first."), true);
		return false;
	}

	// execute untransform powers
	if (powers[power_index].spawn_type == "untransform" && src_stats->transformed) {
		collider->unblock(src_stats->pos.x, src_stats->pos.y);
		if (collider->is_valid_position(src_stats->pos.x, src_stats->pos.y, MOVEMENT_NORMAL, true)) {
			src_stats->transform_duration = 0;
			src_stats->transform_type = "untransform"; // untransform() is called only if type !=""
		}
		else {
			pc->logMsg(msg->get("Could not untransform at this position."), true);
			inpt->unlockActionBar();
			collider->block(src_stats->pos.x, src_stats->pos.y, false);
			return false;
		}
		collider->block(src_stats->pos.x, src_stats->pos.y, false);
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

	// apply any buffs
	buff(power_index, src_stats, target);

	src_stats->manual_untransform = powers[power_index].manual_untransform;
	src_stats->transform_with_equipment = powers[power_index].keep_equipment;
	src_stats->untransform_on_hit = powers[power_index].untransform_on_hit;

	// If there's a sound effect, play it here
	playSound(power_index);

	payPowerCost(power_index, src_stats);

	return true;
}

/**
 * Stationary blocking with optional buffs/debuffs
 * Only the hero can block
 */
bool PowerManager::block(int power_index, StatBlock *src_stats) {
	// if the hero is blocking, we can't activate any more blocking powers
	if (src_stats->effects.triggered_block)
		return false;

	src_stats->effects.triggered_block = true;

	// apply any attached effects
	// passive_trigger MUST be "TRIGGER_BLOCK", since that is how we will later remove effects added by blocking
	powers[power_index].passive_trigger = TRIGGER_BLOCK;
	effect(src_stats, src_stats, power_index, SOURCE_TYPE_HERO);

	// If there's a sound effect, play it here
	playSound(power_index);

	payPowerCost(power_index, src_stats);

	return true;
}

/**
 * Activate is basically a switch/redirect to the appropriate function
 */
bool PowerManager::activate(int power_index, StatBlock *src_stats, const FPoint& target) {
	if (static_cast<unsigned>(power_index) >= powers.size())
		return false;

	if (src_stats->hero) {
		if (powers[power_index].requires_mp > src_stats->mp)
			return false;
	}

	if (src_stats->hp > 0 && powers[power_index].sacrifice == false && powers[power_index].requires_hp >= src_stats->hp)
		return false;

	if (powers[power_index].type == POWTYPE_BLOCK)
		return block(power_index, src_stats);

	if (powers[power_index].script_trigger == SCRIPT_TRIGGER_CAST) {
		EventManager::executeScript(powers[power_index].script, src_stats->pos.x, src_stats->pos.y);
	}

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

			// carried items
			if (powers[power_index].requires_item != -1) {
				int quantity = powers[power_index].requires_item_quantity;
				while (quantity > 0) {
					used_items.push_back(powers[power_index].requires_item);
					quantity--;
				}
			}

			// equipped item
			// if the item is to be consumed, only one may be consumed at a time
			// only allow one instance of duplicate items at a time in the used_equipped_items queue
			// this is useful for Ouroboros rings, where we have 2 equipped, but only want to remove one at a time
			if (powers[power_index].requires_equipped_item != -1 &&
					std::find(used_equipped_items.begin(), used_equipped_items.end(), powers[power_index].requires_equipped_item) == used_equipped_items.end()) {

				int quantity = powers[power_index].requires_equipped_item_quantity;
				while (quantity > 0) {
					used_equipped_items.push_back(powers[power_index].requires_equipped_item);
					quantity--;
				}
			}
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

EffectDef* PowerManager::getEffectDef(const std::string& id) {
	for (unsigned i=0; i<effects.size(); ++i) {
		if (effects[i].id == id) {
			return &effects[i];
		}
	}
	return NULL;
}

int PowerManager::verifyID(int power_id, FileParser* infile, bool allow_zero) {
	bool lower_bound = (allow_zero && power_id < 0) || (!allow_zero && power_id < 1);
	if (lower_bound || static_cast<unsigned>(power_id) >= powers.size()) {
		if (infile != NULL)
			infile->error("PowerManager: %d is not a valid power id.", power_id);
		else
			logError("PowerManager: %d is not a valid power id.", power_id);

		return 0;
	}
	return power_id;
}

PowerManager::~PowerManager() {

	for (unsigned i=0; i<sfx.size(); i++) {
		snd->unload(sfx[i]);
	}
	sfx.clear();
}

