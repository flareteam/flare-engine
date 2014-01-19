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
 * class Avatar
 *
 * Contains logic and rendering routines for the player avatar.
 */

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "CommonIncludes.h"
#include "EnemyManager.h"
#include "FileParser.h"
#include "Hazard.h"
#include "MapRenderer.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"

using namespace std;

Avatar::Avatar()
	: Entity()
	, lockAttack(false)
	, path()
	, prev_target()
	, collided(false)
	, hero_stats(NULL)
	, charmed_stats(NULL)
	, act_target()
	, attacking (false)
	, drag_walking(false)
	, respawn(false)
	, close_menus(false)
	, allow_movement(true) {

	init();

	// default hero animation data
	stats.cooldown = 4;

	// load the hero's animations from hero definition file
	anim->increaseCount("animations/hero.txt");
	animationSet = anim->getAnimationSet("animations/hero.txt");
	activeAnimation = animationSet->getAnimation();

	loadLayerDefinitions();
}

void Avatar::init() {

	// name, base, look are set by GameStateNew so don't reset it here

	// other init
	sprites = 0;
	stats.cur_state = AVATAR_STANCE;
	stats.pos.x = mapr->spawn.x;
	stats.pos.y = mapr->spawn.y;
	stats.direction = mapr->spawn_dir;
	current_power = 0;
	newLevelNotification = false;

	lockAttack = false;

	stats.hero = true;
	stats.humanoid = true;
	stats.level = 1;
	stats.xp = 0;
	stats.physical_character = 1;
	stats.mental_character = 1;
	stats.defense_character = 1;
	stats.offense_character = 1;
	stats.physical_additional = 0;
	stats.mental_additional = 0;
	stats.offense_additional = 0;
	stats.defense_additional = 0;
	stats.speed = 0.2f;
	stats.recalc();

	log_msg = "";
	respawn = false;

	stats.cooldown_ticks = 0;

	haz = NULL;

	body = -1;

	transform_triggered = false;
	setPowers = false;
	revertPowers = false;
	last_transform = "";
	untransform_power = getUntransformPower();

	hero_cooldown = vector<int>(powers->powers.size(), 0);

	for (int i=0; i<4; i++) {
		sound_steps[i] = 0;
	}

	sound_melee = 0;
	sound_mental = 0;
	sound_hit = 0;
	sound_die = 0;
	sound_block = 0;
	level_up = 0;
}

/**
 * Load avatar sprite layer definitions into vector.
 */
void Avatar::loadLayerDefinitions() {
	layer_def = vector<vector<unsigned> >(8, vector<unsigned>());
	layer_reference_order = vector<string>();

	FileParser infile;
	// @CLASS Avatar|Description of engine/hero_layers.txt
	if (infile.open("engine/hero_layers.txt")) {
		while(infile.next()) {
			infile.val = infile.val + ',';

			if (infile.key == "layer") {
				// @ATTR layer|direction (integer), string, ...]|Defines the hero avatar sprite layer
				unsigned dir = eatFirstInt(infile.val,',');
				if (dir>7) {
					fprintf(stderr, "direction must be in range [0,7]\n");
					SDL_Quit();
					exit(1);
				}
				string layer = eatFirstString(infile.val,',');
				while (layer != "") {
					// check if already in layer_reference:
					unsigned ref_pos;
					for (ref_pos = 0; ref_pos < layer_reference_order.size(); ++ref_pos)
						if (layer == layer_reference_order[ref_pos])
							break;
					if (ref_pos == layer_reference_order.size())
						layer_reference_order.push_back(layer);
					layer_def[dir].push_back(ref_pos);

					layer = eatFirstString(infile.val,',');
				}
			}
		}
		infile.close();
	}

	// There are the positions of the items relative to layer_reference_order
	// so if layer_reference_order=main,body,head,off
	// and we got a layer=3,off,body,head,main
	// then the layer_def[3] looks like (3,1,2,0)
}

void Avatar::loadGraphics(std::vector<Layer_gfx> _img_gfx) {

	for (unsigned int i=0; i<animsets.size(); i++) {
		if (animsets[i])
			anim->decreaseCount(animsets[i]->getName());
		delete anims[i];
	}
	animsets.clear();
	anims.clear();

	for (unsigned int i=0; i<_img_gfx.size(); i++) {
		if (_img_gfx[i].gfx != "") {
			string name = "animations/avatar/"+stats.gfx_base+"/"+_img_gfx[i].gfx+".txt";
			anim->increaseCount(name);
			animsets.push_back(anim->getAnimationSet(name));
			anims.push_back(animsets.back()->getAnimation(activeAnimation->getName()));
			anims.back()->syncTo(activeAnimation);
		}
		else {
			animsets.push_back(NULL);
			anims.push_back(NULL);
		}
	}
	anim->cleanUp();
}

void Avatar::loadSounds(const string& type_id) {
	// unload any sounds that are common between creatures and the hero
	snd->unload(sound_melee);
	snd->unload(sound_mental);
	snd->unload(sound_hit);
	snd->unload(sound_die);

	if (type_id != "none") {
		sound_melee = snd->load("soundfx/enemies/" + type_id + "_phys.ogg", "Avatar melee attack");
		sound_mental = snd->load("soundfx/enemies/" + type_id + "_ment.ogg", "Avatar mental attack");
		sound_hit = snd->load("soundfx/enemies/" + type_id + "_hit.ogg", "Avatar was hit");
		sound_die = snd->load("soundfx/enemies/" + type_id + "_die.ogg", "Avatar death");
	}
	else {
		sound_melee = snd->load("soundfx/melee_attack.ogg", "Avatar melee attack");
		sound_mental = 0; // hero does not have this sound
		sound_hit = snd->load("soundfx/" + stats.gfx_base + "_hit.ogg", "Avatar was hit");
		sound_die = snd->load("soundfx/" + stats.gfx_base + "_die.ogg", "Avatar death");
	}

	sound_block = snd->load("soundfx/powers/block.ogg", "Avatar blocking");
	level_up = snd->load("soundfx/level_up.ogg", "Avatar leveling up");
}

/**
 * Walking/running steps sound depends on worn armor
 */
void Avatar::loadStepFX(const string& stepname) {

	string filename = stats.sfx_step;
	if (stepname != "") {
		filename = stepname;
	}

	// clear previous sounds
	for (int i=0; i<4; i++) {
		snd->unload(sound_steps[i]);
	}

	// A literal "NULL" means we don't want to load any new sounds
	// This is used when transforming, since creatures don't have step sound effects
	if (stepname == "NULL") return;

	// load new sounds
	sound_steps[0] = snd->load("soundfx/steps/step_" + filename + "1.ogg", "Avatar loading foot steps");
	sound_steps[1] = snd->load("soundfx/steps/step_" + filename + "2.ogg", "Avatar loading foot steps");
	sound_steps[2] = snd->load("soundfx/steps/step_" + filename + "3.ogg", "Avatar loading foot steps");
	sound_steps[3] = snd->load("soundfx/steps/step_" + filename + "4.ogg", "Avatar loading foot steps");
}


bool Avatar::pressing_move() {
	if (!allow_movement) {
		return false;
	}
	else if (MOUSE_MOVE) {
		return inpt->pressing[MAIN1];
	}
	else {
		return (inpt->pressing[UP] && !inpt->lock[UP]) ||
			   (inpt->pressing[DOWN] && !inpt->lock[DOWN]) ||
			   (inpt->pressing[LEFT] && !inpt->lock[LEFT]) ||
			   (inpt->pressing[RIGHT] && !inpt->lock[RIGHT]);
	}
}

void Avatar::set_direction() {
	// handle direction changes
	if (MOUSE_MOVE) {
		FPoint target = screen_to_map(inpt->mouse.x, inpt->mouse.y, stats.pos.x, stats.pos.y);
		stats.direction = calcDirection(stats.pos, target);
	}
	else {
		if (inpt->pressing[UP] && !inpt->lock[UP] && inpt->pressing[LEFT] && !inpt->lock[LEFT]) stats.direction = 1;
		else if (inpt->pressing[UP] && !inpt->lock[UP] && inpt->pressing[RIGHT] && !inpt->lock[RIGHT]) stats.direction = 3;
		else if (inpt->pressing[DOWN] && !inpt->lock[DOWN] && inpt->pressing[RIGHT] && !inpt->lock[RIGHT]) stats.direction = 5;
		else if (inpt->pressing[DOWN] && !inpt->lock[DOWN] && inpt->pressing[LEFT] && !inpt->lock[LEFT]) stats.direction = 7;
		else if (inpt->pressing[LEFT] && !inpt->lock[LEFT]) stats.direction = 0;
		else if (inpt->pressing[UP] && !inpt->lock[UP]) stats.direction = 2;
		else if (inpt->pressing[RIGHT] && !inpt->lock[RIGHT]) stats.direction = 4;
		else if (inpt->pressing[DOWN] && !inpt->lock[DOWN]) stats.direction = 6;
		// Adjust for ORTHO tilesets
		if (TILESET_ORIENTATION == TILESET_ORTHOGONAL &&
				((inpt->pressing[UP] && !inpt->lock[UP]) || (inpt->pressing[DOWN] && !inpt->lock[UP]) ||
				 (inpt->pressing[LEFT] && !inpt->lock[LEFT]) || (inpt->pressing[RIGHT] && !inpt->lock[RIGHT])))
			stats.direction = stats.direction == 7 ? 0 : stats.direction + 1;
	}
}

void Avatar::handlePower(int actionbar_power) {
	if (actionbar_power != 0 && stats.cooldown_ticks == 0) {
		const Power &power = powers->getPower(actionbar_power);
		FPoint target;
		if (MOUSE_AIM) {
			if (power.aim_assist)
				target = screen_to_map(inpt->mouse.x,  inpt->mouse.y + AIM_ASSIST, stats.pos.x, stats.pos.y);
			else
				target = screen_to_map(inpt->mouse.x,  inpt->mouse.y, stats.pos.x, stats.pos.y);
		}
		else {
			FPoint ftarget = calcVector(stats.pos, stats.direction, stats.melee_range);
			target.x = ftarget.x;
			target.y = ftarget.y;
		}

		// check requirements
		if (!stats.canUsePower(power, actionbar_power))
			return;
		if (power.requires_los && !mapr->collider.line_of_sight(stats.pos.x, stats.pos.y, target.x, target.y))
			return;
		if (power.requires_empty_target && !mapr->collider.is_empty(target.x, target.y))
			return;
		if (hero_cooldown[actionbar_power] > 0)
			return;
		if (!powers->hasValidTarget(actionbar_power,&stats,target))
			return;

		hero_cooldown[actionbar_power] = power.cooldown; //set the cooldown timer
		current_power = actionbar_power;
		act_target = target;

		// is this a power that requires changing direction?
		if (power.face) {
			stats.direction = calcDirection(stats.pos, target);
		}

		attack_anim = power.attack_anim;

		switch (power.new_state) {
			case POWSTATE_ATTACK:	// handle attack powers
				stats.cur_state = AVATAR_ATTACK;
				break;

			case POWSTATE_BLOCK:	// handle blocking
				stats.cur_state = AVATAR_BLOCK;
				stats.effects.triggered_block = true;
				break;

			case POWSTATE_INSTANT:	// handle instant powers
				powers->activate(current_power, &stats, target);
				break;
		}
	}
}

/**
 * logic()
 * Handle a single frame.  This includes:
 * - move the avatar based on buttons pressed
 * - calculate the next frame of animation
 * - calculate camera position based on avatar position
 *
 * @param actionbar_power The actionbar power activated.  0 means no power.
 * @param restrictPowerUse rather or not to allow power usage on mouse1
 */
void Avatar::logic(int actionbar_power, bool restrictPowerUse) {

	// hazards are processed after Avatar and Enemy[]
	// so process and clear sound effects from previous frames
	// check sound effects
	if (AUDIO) {
		if (sfx_phys)
			snd->play(sound_melee, GLOBAL_VIRTUAL_CHANNEL, stats.pos, false);
		if (sfx_ment)
			snd->play(sound_mental, GLOBAL_VIRTUAL_CHANNEL, stats.pos, false);
		if (sfx_hit)
			snd->play(sound_hit, GLOBAL_VIRTUAL_CHANNEL, stats.pos, false);
		if (sfx_die)
			snd->play(sound_die, GLOBAL_VIRTUAL_CHANNEL, stats.pos, false);
		if (sfx_critdie)
			snd->play(sound_die, GLOBAL_VIRTUAL_CHANNEL, stats.pos, false);
		if(sfx_block)
			snd->play(sound_block, GLOBAL_VIRTUAL_CHANNEL, stats.pos, false);

		// clear sound flags
		sfx_hit = false;
		sfx_phys = false;
		sfx_ment = false;
		sfx_die = false;
		sfx_critdie = false;
		sfx_block = false;
	}

	// clear current space to allow correct movement
	mapr->collider.unblock(stats.pos.x, stats.pos.y);

	// turn on all passive powers
	if ((stats.hp > 0 || stats.effects.triggered_death) && !respawn && !transform_triggered) powers->activatePassives(&stats);
	if (transform_triggered) transform_triggered = false;

	int stepfx;
	stats.logic();
	if (stats.effects.forced_move) {
		move();

		// calc new cam position from player position
		// cam is focused at player position
		mapr->cam.x = stats.pos.x;
		mapr->cam.y = stats.pos.y;

		mapr->collider.block(stats.pos.x, stats.pos.y, false);
		return;
	}
	if (stats.effects.stun) {

		mapr->collider.block(stats.pos.x, stats.pos.y, false);
		return;
	}


	bool allowed_to_move;
	bool allowed_to_use_power;

	// check for revive
	if (stats.hp <= 0 && stats.effects.revive) {
		stats.hp = stats.get(STAT_HP_MAX);
		stats.alive = true;
		stats.corpse = false;
		stats.cur_state = AVATAR_STANCE;
	}

	// check level up
	if (stats.level < (int)stats.xp_table.size() && stats.xp >= stats.xp_table[stats.level]) {
		stats.level_up = true;
		stats.level++;
		stringstream ss;
		ss << msg->get("Congratulations, you have reached level %d!", stats.level);
		if (stats.level < stats.max_spendable_stat_points) {
			ss << " " << msg->get("You may increase one attribute through the Character Menu.");
			newLevelNotification = true;
		}
		log_msg = ss.str();
		stats.recalc();
		snd->play(level_up);

		// if the player managed to level up while dead (e.g. via a bleeding creature), restore to life
		if (stats.cur_state == AVATAR_DEAD) {
			stats.cur_state = AVATAR_STANCE;
		}
	}

	// check for bleeding spurt
	if (stats.effects.damage > 0 && stats.hp > 0) {
		comb->addMessage(stats.effects.damage, stats.pos, COMBAT_MESSAGE_TAKEDMG);
	}

	// check for bleeding to death
	if (stats.hp == 0 && !(stats.cur_state == AVATAR_DEAD)) {
		stats.effects.triggered_death = true;
		stats.cur_state = AVATAR_DEAD;
	}

	// assist mouse movement
	if (!inpt->pressing[MAIN1]) {
		drag_walking = false;
		attacking = false;
	}
	else {
		if(!inpt->lock[MAIN1]) {
			attacking = true;
		}
	}

	// handle animation
	activeAnimation->advanceFrame();
	for (unsigned i=0; i < anims.size(); i++)
		if (anims[i] != NULL)
			anims[i]->advanceFrame();

	// handle transformation
	if (stats.transform_type != "" && stats.transform_type != "untransform" && stats.transformed == false) transform();
	if (stats.transform_type != "" && stats.transform_duration == 0) untransform();

	switch(stats.cur_state) {
		case AVATAR_STANCE:

			setAnimation("stance");

			// allowed to move or use powers?
			if (MOUSE_MOVE) {
				allowed_to_move = restrictPowerUse && (!inpt->lock[MAIN1] || drag_walking) && !lockAttack;
				allowed_to_use_power = !allowed_to_move;
			}
			else {
				allowed_to_move = true;
				allowed_to_use_power = true;
			}

			// handle transitions to RUN
			if (allowed_to_move)
				set_direction();

			if (pressing_move() && allowed_to_move) {
				if (MOUSE_MOVE && inpt->pressing[MAIN1]) {
					inpt->lock[MAIN1] = true;
					drag_walking = true;
				}

				if (move()) { // no collision
					stats.cur_state = AVATAR_RUN;
				}
				else {
					collided = true;
				}

			}

			if (MOUSE_MOVE && !inpt->pressing[MAIN1]) {
				inpt->lock[MAIN1] = false;
				lockAttack = false;
			}

			// handle power usage
			if (allowed_to_use_power)
				handlePower(actionbar_power);
			break;

		case AVATAR_RUN:

			setAnimation("run");

			stepfx = rand() % 4;

			if (activeAnimation->isFirstFrame() || activeAnimation->isActiveFrame())
				snd->play(sound_steps[stepfx]);

			// allowed to move or use powers?
			if (MOUSE_MOVE) {
				allowed_to_use_power = !(restrictPowerUse && !inpt->lock[MAIN1]);
			}
			else {
				allowed_to_use_power = true;
			}

			// handle direction changes
			set_direction();

			// handle transition to STANCE
			if (!pressing_move()) {
				stats.cur_state = AVATAR_STANCE;
				break;
			}
			else if (!move()) { // collide with wall
				collided = true;
				stats.cur_state = AVATAR_STANCE;
				break;
			}

			// handle power usage
			if (allowed_to_use_power)
				handlePower(actionbar_power);

			if (activeAnimation->getName() != "run")
				stats.cur_state = AVATAR_STANCE;

			break;

		case AVATAR_ATTACK:

			setAnimation(attack_anim);

			if (MOUSE_MOVE) lockAttack = true;

			if (activeAnimation->isFirstFrame() && attack_anim == "swing")
				snd->play(sound_melee);

			if (activeAnimation->isFirstFrame() && attack_anim == "cast")
				snd->play(sound_mental);

			// do power
			if (activeAnimation->isActiveFrame()) {
				powers->activate(current_power, &stats, act_target);
			}

			if (activeAnimation->getTimesPlayed() >= 1 || activeAnimation->getName() != attack_anim) {
				stats.cur_state = AVATAR_STANCE;
				stats.cooldown_ticks += stats.cooldown;
			}
			break;

		case AVATAR_BLOCK:

			setAnimation("block");

			if (powers->powers[actionbar_power].new_state != POWSTATE_BLOCK || activeAnimation->getName() != "block") {
				stats.cur_state = AVATAR_STANCE;
				stats.effects.triggered_block = false;
				stats.effects.clearTriggerEffects(TRIGGER_BLOCK);
			}
			break;

		case AVATAR_HIT:

			setAnimation("hit");

			if (activeAnimation->isFirstFrame()) {
				stats.effects.triggered_hit = true;
			}

			if (activeAnimation->getTimesPlayed() >= 1 || activeAnimation->getName() != "hit") {
				stats.cur_state = AVATAR_STANCE;
			}

			break;

		case AVATAR_DEAD:
			if (stats.effects.triggered_death) break;

			if (stats.transformed) {
				stats.transform_duration = 0;
				untransform();
			}

			setAnimation("die");

			if (!stats.corpse && activeAnimation->isFirstFrame() && activeAnimation->getTimesPlayed() < 1) {
				stats.effects.clearEffects();

				// raise the death penalty flag.  Another module will read this and reset.
				stats.death_penalty = true;

				// close menus in GameStatePlay
				close_menus = true;

				snd->play(sound_die);

				if (stats.permadeath) {
					log_msg = msg->get("You are defeated. Game over! Press Enter to exit to Title.");
				}
				else {
					log_msg = msg->get("You are defeated. Press Enter to continue.");
				}

				//once the player dies, kill off any remaining summons
				for (unsigned int i=0; i < enemies->enemies.size(); i++) {
					if(!enemies->enemies[i]->stats.corpse && enemies->enemies[i]->stats.hero_ally)
						enemies->enemies[i]->InstantDeath();
				}
			}

			if (activeAnimation->getTimesPlayed() >= 1 || activeAnimation->getName() != "die") {
				stats.corpse = true;
			}

			// allow respawn with Accept if not permadeath
			if (inpt->pressing[ACCEPT]) {
				inpt->lock[ACCEPT] = true;
				mapr->teleportation = true;
				mapr->teleport_mapname = mapr->respawn_map;
				if (stats.permadeath) {
					// set these positions so it doesn't flash before jumping to Title
					mapr->teleport_destination.x = stats.pos.x;
					mapr->teleport_destination.y = stats.pos.y;
				}
				else {
					respawn = true;

					// set teleportation variables.  GameEngine acts on these.
					mapr->teleport_destination.x = mapr->respawn_point.x;
					mapr->teleport_destination.y = mapr->respawn_point.y;
				}
			}

			break;

		default:
			break;
	}

	// calc new cam position from player position
	// cam is focused at player position
	mapr->cam.x = stats.pos.x;
	mapr->cam.y = stats.pos.y;

	// check for map events
	mapr->checkEvents(stats.pos);

	// decrement all cooldowns
	for (unsigned i = 0; i < hero_cooldown.size(); i++) {
		hero_cooldown[i]--;
		if (hero_cooldown[i] < 0) hero_cooldown[i] = 0;
	}

	// make the current square solid
	mapr->collider.block(stats.pos.x, stats.pos.y, false);
}

void Avatar::transform() {
	// calling a transform power locks the actionbar, so we unlock it here
	inpt->unlockActionBar();

	transform_triggered = true;
	stats.transformed = true;
	setPowers = true;

	delete charmed_stats;
	charmed_stats = new StatBlock();
	charmed_stats->load("enemies/" + stats.transform_type + ".txt");

	// temporary save hero stats
	delete hero_stats;

	hero_stats = new StatBlock();
	*hero_stats = stats;

	// replace some hero stats
	stats.speed = charmed_stats->speed;
	stats.flying = charmed_stats->flying;
	stats.humanoid = charmed_stats->humanoid;
	stats.animations = charmed_stats->animations;
	stats.powers_list = charmed_stats->powers_list;
	stats.powers_passive = charmed_stats->powers_passive;
	stats.effects.clearEffects();

	anim->decreaseCount("animations/hero.txt");
	anim->increaseCount(charmed_stats->animations);
	animationSet = anim->getAnimationSet(charmed_stats->animations);
	delete activeAnimation;
	activeAnimation = animationSet->getAnimation();
	stats.cur_state = AVATAR_STANCE;

	// damage
	clampFloor(stats.starting[STAT_DMG_MELEE_MIN], charmed_stats->starting[STAT_DMG_MELEE_MIN]);
	clampFloor(stats.starting[STAT_DMG_MELEE_MAX], charmed_stats->starting[STAT_DMG_MELEE_MAX]);

	clampFloor(stats.starting[STAT_DMG_MENT_MIN], charmed_stats->starting[STAT_DMG_MENT_MIN]);
	clampFloor(stats.starting[STAT_DMG_MENT_MAX], charmed_stats->starting[STAT_DMG_MENT_MAX]);

	clampFloor(stats.starting[STAT_DMG_RANGED_MIN], charmed_stats->starting[STAT_DMG_RANGED_MIN]);
	clampFloor(stats.starting[STAT_DMG_RANGED_MAX], charmed_stats->starting[STAT_DMG_RANGED_MAX]);

	// dexterity
	clampFloor(stats.starting[STAT_ABS_MIN], charmed_stats->starting[STAT_ABS_MIN]);
	clampFloor(stats.starting[STAT_ABS_MAX], charmed_stats->starting[STAT_ABS_MAX]);

	clampFloor(stats.starting[STAT_AVOIDANCE], charmed_stats->starting[STAT_AVOIDANCE]);

	clampFloor(stats.starting[STAT_ACCURACY], charmed_stats->starting[STAT_ACCURACY]);

	clampFloor(stats.starting[STAT_CRIT], charmed_stats->starting[STAT_CRIT]);

	// resistances
	for (unsigned int i=0; i<stats.vulnerable.size(); i++)
		clampCeil(stats.vulnerable[i], charmed_stats->vulnerable[i]);

	loadSounds(charmed_stats->sfx_prefix);
	loadStepFX("NULL");

	stats.applyEffects();
}

void Avatar::untransform() {
	// calling a transform power locks the actionbar, so we unlock it here
	inpt->unlockActionBar();

	// Only allow untransform when on a valid tile
	if (!mapr->collider.is_valid_position(stats.pos.x,stats.pos.y,MOVEMENT_NORMAL, true)) return;

	stats.transformed = false;
	transform_triggered = true;
	stats.transform_type = "";
	revertPowers = true;
	stats.effects.clearEffects();

	// revert some hero stats to last saved
	stats.speed = hero_stats->speed;
	stats.flying = hero_stats->flying;
	stats.humanoid = hero_stats->humanoid;
	stats.animations = hero_stats->animations;
	stats.effects = hero_stats->effects;
	stats.powers_list = hero_stats->powers_list;
	stats.powers_passive = hero_stats->powers_passive;

	anim->increaseCount("animations/hero.txt");
	anim->decreaseCount(charmed_stats->animations);
	animationSet = anim->getAnimationSet("animations/hero.txt");
	delete activeAnimation;
	activeAnimation = animationSet->getAnimation();
	stats.cur_state = AVATAR_STANCE;

	// This is a bit of a hack.
	// In order to switch to the stance animation, we can't already be in a stance animation
	setAnimation("run");

	stats.starting[STAT_DMG_MELEE_MIN] = hero_stats->starting[STAT_DMG_MELEE_MIN];
	stats.starting[STAT_DMG_MELEE_MAX] = hero_stats->starting[STAT_DMG_MELEE_MAX];
	stats.starting[STAT_DMG_MENT_MIN] = hero_stats->starting[STAT_DMG_MENT_MIN];
	stats.starting[STAT_DMG_MENT_MAX] = hero_stats->starting[STAT_DMG_MENT_MAX];
	stats.starting[STAT_DMG_RANGED_MIN] = hero_stats->starting[STAT_DMG_RANGED_MIN];
	stats.starting[STAT_DMG_RANGED_MAX] = hero_stats->starting[STAT_DMG_RANGED_MAX];

	stats.starting[STAT_ABS_MIN] = hero_stats->starting[STAT_ABS_MIN];
	stats.starting[STAT_ABS_MAX] = hero_stats->starting[STAT_ABS_MAX];
	stats.starting[STAT_AVOIDANCE] = hero_stats->starting[STAT_AVOIDANCE];
	stats.starting[STAT_ACCURACY] = hero_stats->starting[STAT_ACCURACY];
	stats.starting[STAT_CRIT] = hero_stats->starting[STAT_CRIT];

	for (unsigned int i=0; i<stats.vulnerable.size(); i++) {
		stats.vulnerable[i] = hero_stats->vulnerable[i];
	}

	loadSounds();
	loadStepFX(stats.sfx_step);

	delete charmed_stats;
	delete hero_stats;
	charmed_stats = NULL;
	hero_stats = NULL;

	stats.applyEffects();
}

void Avatar::setAnimation(std::string name) {
	if (name == activeAnimation->getName())
		return;

	Entity::setAnimation(name);
	for (unsigned i=0; i < animsets.size(); i++) {
		delete anims[i];
		if (animsets[i])
			anims[i] = animsets[i]->getAnimation(name);
		else
			anims[i] = 0;
	}
}

/**
 * Find untransform power index to use for manual untransfrom ability
 */
int Avatar::getUntransformPower() {
	for (unsigned id=0; id<powers->powers.size(); id++) {
		if (powers->powers[id].spawn_type == "untransform" && powers->powers[id].requires_item == -1)
			return id;
	}
	return 0;
}

void Avatar::resetActiveAnimation() {
	activeAnimation->reset(); // shield stutter
	for (unsigned i=0; i < animsets.size(); i++)
		if (anims[i])
			anims[i]->reset();
}

void Avatar::addRenders(vector<Renderable> &r) {
	if (!stats.transformed) {
		for (unsigned i = 0; i < layer_def[stats.direction].size(); ++i) {
			unsigned index = layer_def[stats.direction][i];
			if (anims[index]) {
				Renderable ren = anims[index]->getCurrentFrame(stats.direction);
				ren.map_pos = stats.pos;
				ren.prio = i+1;
				r.push_back(ren);
			}
		}
	}
	else {
		Renderable ren = activeAnimation->getCurrentFrame(stats.direction);
		ren.map_pos = stats.pos;
		r.push_back(ren);
	}
	// add effects
	for (unsigned i = 0; i < stats.effects.effect_list.size(); ++i) {
		if (stats.effects.effect_list[i].animation && !stats.effects.effect_list[i].animation->isCompleted()) {
			Renderable ren = stats.effects.effect_list[i].animation->getCurrentFrame(0);
			ren.map_pos = stats.pos;
			if (stats.effects.effect_list[i].render_above) ren.prio = layer_def[stats.direction].size()+1;
			else ren.prio = 0;
			r.push_back(ren);
		}
	}
}

Avatar::~Avatar() {

	if (stats.transformed && charmed_stats && charmed_stats->animations != "") {
		anim->decreaseCount(charmed_stats->animations);
	}
	else {
		anim->decreaseCount("animations/hero.txt");
	}

	for (unsigned int i=0; i<animsets.size(); i++) {
		if (animsets[i])
			anim->decreaseCount(animsets[i]->getName());
		delete anims[i];
	}
	anim->cleanUp();

	delete charmed_stats;
	delete hero_stats;

	snd->unload(sound_melee);
	snd->unload(sound_mental);
	snd->unload(sound_hit);
	snd->unload(sound_die);
	snd->unload(sound_block);

	for (int i = 0; i < 4; i++)
		snd->unload(sound_steps[i]);

	snd->unload(level_up);

	delete haz;
}
