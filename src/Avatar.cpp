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
 * class Avatar
 *
 * Contains logic and rendering routines for the player avatar.
 */

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "Avatar.h"
#include "CommonIncludes.h"
#include "CursorManager.h"
#include "EnemyGroupManager.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "InputState.h"
#include "MapRenderer.h"
#include "MenuActionBar.h"
#include "MenuExit.h"
#include "MenuManager.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Utils.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

Avatar::Avatar()
	: Entity()
	, attack_cursor(false)
	, mm_key(settings->mouse_move_swap ? Input::MAIN2 : Input::MAIN1)
	, hero_stats(NULL)
	, charmed_stats(NULL)
	, act_target()
	, drag_walking(false)
	, respawn(false)
	, close_menus(false)
	, allow_movement(true)
	, cursor_enemy(NULL)
	, lock_enemy(NULL)
	, time_played(0)
	, questlog_dismissed(false)
	, using_main1(false)
	, using_main2(false)
	, prev_hp(0)
	, playing_lowhp(false)
	, teleport_camera_lock(false) {

	init();

	// load the hero's animations from hero definition file
	anim->increaseCount("animations/hero.txt");
	animationSet = anim->getAnimationSet("animations/hero.txt");
	activeAnimation = animationSet->getAnimation("");

	// set cooldown_hit to duration of hit animation if undefined
	if (!stats.cooldown_hit_enabled) {
		Animation *hit_anim = animationSet->getAnimation("hit");
		if (hit_anim) {
			stats.cooldown_hit.setDuration(hit_anim->getDuration());
			delete hit_anim;
		}
		else {
			stats.cooldown_hit.setDuration(0);
		}
	}

	loadLayerDefinitions();

	// load foot-step definitions
	// @CLASS Avatar: Step sounds|Description of items/step_sounds.txt
	FileParser infile;
	if (infile.open("items/step_sounds.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		while (infile.next()) {
			if (infile.key == "id") {
				// @ATTR id|string|An identifier name for a set of step sounds.
				step_def.push_back(Step_sfx());
				step_def.back().id = infile.val;
			}

			if (step_def.empty()) continue;

			if (infile.key == "step") {
				// @ATTR step|filename|Filename of a step sound effect.
				step_def.back().steps.push_back(infile.val);
			}
		}
		infile.close();
	}

	loadStepFX(stats.sfx_step);
}

void Avatar::init() {

	// name, base, look are set by GameStateNew so don't reset it here

	// other init
	sprites = 0;
	stats.cur_state = StatBlock::AVATAR_STANCE;
	if (mapr->hero_pos_enabled) {
		stats.pos.x = mapr->hero_pos.x;
		stats.pos.y = mapr->hero_pos.y;
	}
	current_power = 0;
	newLevelNotification = false;

	stats.hero = true;
	stats.humanoid = true;
	stats.level = 1;
	stats.xp = 0;
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		stats.primary[i] = stats.primary_starting[i] = 1;
		stats.primary_additional[i] = 0;
	}
	stats.speed = 0.2f;
	stats.recalc();

	while (!log_msg.empty()) {
		log_msg.pop();
	}
	respawn = false;

	stats.cooldown.reset(Timer::END);

	body = -1;

	transform_triggered = false;
	setPowers = false;
	revertPowers = false;
	last_transform = "";

	// Find untransform power index to use for manual untransfrom ability
	untransform_power = 0;
	for (unsigned id=0; id<powers->powers.size(); id++) {
		if (powers->powers[id].spawn_type == "untransform" && powers->powers[id].required_items.empty()) {
			untransform_power = id;
			break;
		}
	}

	power_cooldown_timers.clear();
	power_cooldown_timers.resize(powers->powers.size());
	power_cast_timers.clear();
	power_cast_timers.resize(powers->powers.size());

}

void Avatar::handleNewMap() {
	cursor_enemy = NULL;
	lock_enemy = NULL;
	playing_lowhp = false;
}

/**
 * Load avatar sprite layer definitions into vector.
 */
void Avatar::loadLayerDefinitions() {
	layer_def = std::vector<std::vector<unsigned> >(8, std::vector<unsigned>());
	layer_reference_order = std::vector<std::string>();

	FileParser infile;
	// @CLASS Avatar: Hero layers|Description of engine/hero_layers.txt
	if (infile.open("engine/hero_layers.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (infile.key == "layer") {
				// @ATTR layer|direction, list(string) : Direction, Layer name(s)|Defines the hero avatar sprite layer
				unsigned dir = Parse::toDirection(Parse::popFirstString(infile.val));
				if (dir>7) {
					infile.error("Avatar: Hero layer direction must be in range [0,7]");
					Utils::logErrorDialog("Avatar: Hero layer direction must be in range [0,7]");
					mods->resetModConfig();
					Utils::Exit(1);
				}
				std::string layer = Parse::popFirstString(infile.val);
				while (layer != "") {
					// check if already in layer_reference:
					unsigned ref_pos;
					for (ref_pos = 0; ref_pos < layer_reference_order.size(); ++ref_pos)
						if (layer == layer_reference_order[ref_pos])
							break;
					if (ref_pos == layer_reference_order.size())
						layer_reference_order.push_back(layer);
					layer_def[dir].push_back(ref_pos);

					layer = Parse::popFirstString(infile.val);
				}
			}
			else {
				infile.error("Avatar: '%s' is not a valid key.", infile.key.c_str());
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
			std::string name = "animations/avatar/"+stats.gfx_base+"/"+_img_gfx[i].gfx+".txt";
			anim->increaseCount(name);
			animsets.push_back(anim->getAnimationSet(name));
			animsets.back()->setParent(animationSet);
			anims.push_back(animsets.back()->getAnimation(activeAnimation->getName()));
			setAnimation("stance");
			if(!anims.back()->syncTo(activeAnimation)) {
				Utils::logError("Avatar: Error syncing animation in '%s' to 'animations/hero.txt'.", animsets.back()->getName().c_str());
			}
		}
		else {
			animsets.push_back(NULL);
			anims.push_back(NULL);
		}
	}
	anim->cleanUp();
}

/**
 * Walking/running steps sound depends on worn armor
 */
void Avatar::loadStepFX(const std::string& stepname) {
	std::string filename = stats.sfx_step;
	if (stepname != "") {
		filename = stepname;
	}

	// clear previous sounds
	for (unsigned i=0; i<sound_steps.size(); i++) {
		snd->unload(sound_steps[i]);
	}
	sound_steps.clear();

	if (filename == "") return;

	// A literal "NULL" means we don't want to load any new sounds
	// This is used when transforming, since creatures don't have step sound effects
	if (stepname == "NULL") return;

	// load new sounds
	for (unsigned i=0; i<step_def.size(); i++) {
		if (step_def[i].id == filename) {
			sound_steps.resize(step_def[i].steps.size());
			for (unsigned j=0; j<sound_steps.size(); j++) {
				sound_steps[j] = snd->load(step_def[i].steps[j], "Avatar loading foot steps");
			}
			return;
		}
	}

	// Could not find step sound fx
	Utils::logError("Avatar: Could not find footstep sounds for '%s'.", filename.c_str());
}


bool Avatar::pressing_move() {
	if (!allow_movement || teleport_camera_lock) {
		return false;
	}
	else if (stats.effects.knockback_speed != 0) {
		return false;
	}
	else if (settings->mouse_move) {
		return inpt->pressing[mm_key] && !inpt->pressing[Input::SHIFT];
	}
	else {
		return (inpt->pressing[Input::UP] && !inpt->lock[Input::UP]) ||
			   (inpt->pressing[Input::DOWN] && !inpt->lock[Input::DOWN]) ||
			   (inpt->pressing[Input::LEFT] && !inpt->lock[Input::LEFT]) ||
			   (inpt->pressing[Input::RIGHT] && !inpt->lock[Input::RIGHT]);
	}
}

void Avatar::set_direction() {
	if (teleport_camera_lock || !set_dir_timer.isEnd())
		return;

	int old_dir = stats.direction;

	// handle direction changes
	if (settings->mouse_move) {
		FPoint target = Utils::screenToMap(inpt->mouse.x, inpt->mouse.y, mapr->cam.x, mapr->cam.y);
		stats.direction = Utils::calcDirection(stats.pos.x, stats.pos.y, target.x, target.y);
	}
	else {
		if (inpt->pressing[Input::UP] && !inpt->lock[Input::UP] && inpt->pressing[Input::LEFT] && !inpt->lock[Input::LEFT]) stats.direction = 1;
		else if (inpt->pressing[Input::UP] && !inpt->lock[Input::UP] && inpt->pressing[Input::RIGHT] && !inpt->lock[Input::RIGHT]) stats.direction = 3;
		else if (inpt->pressing[Input::DOWN] && !inpt->lock[Input::DOWN] && inpt->pressing[Input::RIGHT] && !inpt->lock[Input::RIGHT]) stats.direction = 5;
		else if (inpt->pressing[Input::DOWN] && !inpt->lock[Input::DOWN] && inpt->pressing[Input::LEFT] && !inpt->lock[Input::LEFT]) stats.direction = 7;
		else if (inpt->pressing[Input::LEFT] && !inpt->lock[Input::LEFT]) stats.direction = 0;
		else if (inpt->pressing[Input::UP] && !inpt->lock[Input::UP]) stats.direction = 2;
		else if (inpt->pressing[Input::RIGHT] && !inpt->lock[Input::RIGHT]) stats.direction = 4;
		else if (inpt->pressing[Input::DOWN] && !inpt->lock[Input::DOWN]) stats.direction = 6;
		// Adjust for ORTHO tilesets
		if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL &&
				((inpt->pressing[Input::UP] && !inpt->lock[Input::UP]) || (inpt->pressing[Input::DOWN] && !inpt->lock[Input::UP]) ||
				 (inpt->pressing[Input::LEFT] && !inpt->lock[Input::LEFT]) || (inpt->pressing[Input::RIGHT] && !inpt->lock[Input::RIGHT])))
			stats.direction = static_cast<unsigned char>((stats.direction == 7) ? 0 : stats.direction + 1);
	}

	// give direction changing a 100ms cooldown
	// this allows the player to quickly change direction on their own without becoming overly "jittery"
	// the cooldown can be ended by releasing the move button, but the cooldown is so fast that it doesn't matter much (maybe a speed run tactic?)
	if (stats.direction != old_dir)
		set_dir_timer.setDuration(settings->max_frames_per_sec / 10);
}

/**
 * logic()
 * Handle a single frame.  This includes:
 * - move the avatar based on buttons pressed
 * - calculate the next frame of animation
 * - calculate camera position based on avatar position
 *
 * @param action The actionbar power activated and the target.  action.power == 0 means no power.
 * @param restrict_power_use Whether or not to allow power usage on mouse1
 * @param npc True if the player is talking to an NPC. Can limit ability to move/attack in certain conditions
 */
void Avatar::logic(std::vector<ActionData> &action_queue, bool restrict_power_use) {
	// clear current space to allow correct movement
	mapr->collider.unblock(stats.pos.x, stats.pos.y);

	// turn on all passive powers
	if ((stats.hp > 0 || stats.effects.triggered_death) && !respawn && !transform_triggered)
		powers->activatePassives(&stats);

	if (transform_triggered)
		transform_triggered = false;

	// handle when the player stops blocking
	if (stats.effects.triggered_block && !stats.blocking) {
		stats.cur_state = StatBlock::AVATAR_STANCE;
		stats.effects.triggered_block = false;
		stats.effects.clearTriggerEffects(Power::TRIGGER_BLOCK);
		stats.refresh_stats = true;
	}

	stats.logic();

	// alert on low health
	if (isDroppedToLowHp()) {
		// show message if set
		if (isLowHpMessageEnabled()) {
			std::stringstream ss;
			ss << msg->get("Your health is low!");
			logMsg(ss.str(), MSG_NORMAL);
		}
		// play a sound if set in settings
		if (isLowHpSoundEnabled() && !playing_lowhp) {
			// if looping, then do not cleanup
			snd->play(sound_lowhp, "lowhp", snd->NO_POS, stats.sfx_lowhp_loop, !stats.sfx_lowhp_loop);
			playing_lowhp = true;
		}
	}
	// if looping, stop sounds when HP recovered above threshold
	if (isLowHpSoundEnabled() && !isLowHp() && playing_lowhp && stats.sfx_lowhp_loop) {
		snd->pauseChannel("lowhp");
		playing_lowhp = false;
	}
	else if (isLowHpSoundEnabled() && isLowHp() && !playing_lowhp && stats.sfx_lowhp_loop) {
		snd->play(sound_lowhp, "lowhp", snd->NO_POS, stats.sfx_lowhp_loop, !stats.sfx_lowhp_loop);
		playing_lowhp = true;
	}
	else if (!isLowHpSoundEnabled() && playing_lowhp) {
		snd->pauseChannel("lowhp");
		playing_lowhp = false;
	}

	// we can not use stats.prev_hp here
	prev_hp = stats.hp;

	// check level up
	if (stats.level < eset->xp.getMaxLevel() && stats.xp >= eset->xp.getLevelXP(stats.level + 1)) {
		stats.level_up = true;
		stats.level++;
		std::stringstream ss;
		ss << msg->get("Congratulations, you have reached level %d!", stats.level);
		if (pc->stats.stat_points_per_level > 0) {
			ss << " " << msg->get("You may increase one or more attributes through the Character Menu.");
			newLevelNotification = true;
		}
		if (pc->stats.power_points_per_level > 0) {
			ss << " " << msg->get("You may unlock one or more abilities through the Powers Menu.");
		}
		logMsg(ss.str(), MSG_NORMAL);
		stats.recalc();
		snd->play(sound_levelup, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);

		// if the player managed to level up while dead (e.g. via a bleeding creature), restore to life
		if (stats.cur_state == StatBlock::AVATAR_DEAD) {
			stats.cur_state = StatBlock::AVATAR_STANCE;
		}
	}

	// assist mouse movement
	mm_key = settings->mouse_move_swap ? Input::MAIN2 : Input::MAIN1;
	if (!inpt->pressing[mm_key]) {
		drag_walking = false;
	}

	// block some interactions when attacking/moving
	using_main1 = inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1];
	using_main2 = inpt->pressing[Input::MAIN2] && !inpt->lock[Input::MAIN2];

	// handle animation
	if (!stats.effects.stun) {
		activeAnimation->advanceFrame();
		for (unsigned i=0; i < anims.size(); i++) {
			if (anims[i] != NULL)
				anims[i]->advanceFrame();
		}
	}

	// save a valid tile position in the event that we untransform on an invalid tile
	if (stats.transformed && mapr->collider.isValidPosition(stats.pos.x, stats.pos.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_HERO)) {
		transform_pos = stats.pos;
		transform_map = mapr->getFilename();
	}

	int mm_attack_id = (settings->mouse_move_swap ? menu->act->getSlotPower(MenuActionBar::SLOT_MAIN2) : menu->act->getSlotPower(MenuActionBar::SLOT_MAIN1));
	bool mm_can_use_power = true;

	if (settings->mouse_move) {
		if (!inpt->pressing[mm_key]) {
			lock_enemy = NULL;
		}
		if (lock_enemy && lock_enemy->stats.hp <= 0) {
			lock_enemy = NULL;
		}
		if (mm_attack_id > 0) {
			if (!stats.canUsePower(mm_attack_id, !StatBlock::CAN_USE_PASSIVE)) {
				lock_enemy = NULL;
				mm_can_use_power = false;
			}
			else if (!power_cooldown_timers[mm_attack_id].isEnd()) {
				lock_enemy = NULL;
				mm_can_use_power = false;
			}
			else if (lock_enemy && powers->powers[mm_attack_id].requires_los && !mapr->collider.lineOfSight(stats.pos.x, stats.pos.y, lock_enemy->stats.pos.x, lock_enemy->stats.pos.y)) {
				lock_enemy = NULL;
				mm_can_use_power = false;
			}
		}
	}

	if (teleport_camera_lock && Utils::calcDist(stats.pos, mapr->cam) < 0.5f) {
		teleport_camera_lock = false;
	}

	set_dir_timer.tick();
	if (!pressing_move()) {
		set_dir_timer.reset(Timer::END);
	}

	if (!stats.effects.stun) {
		bool allowed_to_move;
		bool allowed_to_use_power = true;

		switch(stats.cur_state) {
			case StatBlock::AVATAR_STANCE:

				setAnimation("stance");

				// allowed to move or use powers?
				if (settings->mouse_move) {
					allowed_to_move = restrict_power_use && (!inpt->lock[mm_key] || drag_walking) && !lock_enemy;
					allowed_to_use_power = true;

					if ((inpt->pressing[mm_key] && inpt->pressing[Input::SHIFT]) || lock_enemy) {
						inpt->lock[mm_key] = false;
					}
				}
				else {
					allowed_to_move = true;
					allowed_to_use_power = true;
				}

				// handle transitions to RUN
				if (allowed_to_move)
					set_direction();

				if (pressing_move() && allowed_to_move) {
					if (move()) { // no collision
						if (settings->mouse_move && inpt->pressing[mm_key]) {
							inpt->lock[mm_key] = true;
							drag_walking = true;
						}

						stats.cur_state = StatBlock::AVATAR_RUN;
					}
				}

				if (settings->mouse_move &&  settings->mouse_move_attack && cursor_enemy && mm_can_use_power && powers->checkCombatRange(mm_attack_id, &stats, cursor_enemy->stats.pos)) {
					stats.cur_state = StatBlock::AVATAR_STANCE;
					lock_enemy = cursor_enemy;
				}

				break;

			case StatBlock::AVATAR_RUN:

				setAnimation("run");

				if (!sound_steps.empty()) {
					int stepfx = rand() % static_cast<int>(sound_steps.size());

					if (activeAnimation->isFirstFrame() || activeAnimation->isActiveFrame())
						snd->play(sound_steps[stepfx], snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
				}

				// handle direction changes
				set_direction();

				// handle transition to STANCE
				if (!pressing_move()) {
					stats.cur_state = StatBlock::AVATAR_STANCE;
					break;
				}
				else if (!move()) { // collide with wall
					stats.cur_state = StatBlock::AVATAR_STANCE;
					break;
				}
				else if (settings->mouse_move && inpt->pressing[Input::SHIFT]) {
					// when moving with the mouse, pressing Shift should stop movement and begin attacking
					stats.cur_state = StatBlock::AVATAR_STANCE;
					break;
				}

				if (activeAnimation->getName() != "run")
					stats.cur_state = StatBlock::AVATAR_STANCE;

				if (settings->mouse_move && settings->mouse_move_attack && cursor_enemy && mm_can_use_power && powers->checkCombatRange(mm_attack_id, &stats, cursor_enemy->stats.pos)) {
					stats.cur_state = StatBlock::AVATAR_STANCE;
					lock_enemy = cursor_enemy;
				}

				break;

			case StatBlock::AVATAR_ATTACK:

				setAnimation(attack_anim);

				if (attack_cursor) {
					curs->setCursor(CursorManager::CURSOR_ATTACK);
				}

				if (activeAnimation->isFirstFrame()) {
					float attack_speed = (stats.effects.getAttackSpeed(attack_anim) * powers->powers[current_power].attack_speed) / 100.0f;
					activeAnimation->setSpeed(attack_speed);
					playAttackSound(attack_anim);
					power_cast_timers[current_power].setDuration(activeAnimation->getDuration());
				}

				// do power
				if (activeAnimation->isActiveFrame() && !stats.hold_state) {
					// some powers check if the caster is blocking a tile
					// so we block the player tile prematurely here
					mapr->collider.block(stats.pos.x, stats.pos.y, !MapCollision::IS_ALLY);

					powers->activate(current_power, &stats, act_target);
					power_cooldown_timers[current_power].setDuration(powers->powers[current_power].cooldown);

					if (!stats.state_timer.isEnd())
						stats.hold_state = true;
				}

				// animation is done, switch back to normal stance
				if ((activeAnimation->isLastFrame() && stats.state_timer.isEnd()) || activeAnimation->getName() != attack_anim) {
					stats.cur_state = StatBlock::AVATAR_STANCE;
					stats.cooldown.reset(Timer::BEGIN);
					allowed_to_use_power = false;
					stats.prevent_interrupt = false;
					if (settings->mouse_move) {
						drag_walking = true;
					}
				}

				if (settings->mouse_move && lock_enemy && !powers->checkCombatRange(mm_attack_id, &stats, lock_enemy->stats.pos)) {
					lock_enemy = NULL;
				}

				break;

			case StatBlock::AVATAR_BLOCK:

				setAnimation("block");

				stats.blocking = false;

				break;

			case StatBlock::AVATAR_HIT:

				setAnimation("hit");

				if (activeAnimation->isFirstFrame()) {
					stats.effects.triggered_hit = true;

					if (stats.block_power != 0) {
						power_cooldown_timers[stats.block_power].setDuration(powers->powers[stats.block_power].cooldown);
						stats.block_power = 0;
					}
				}

				if (activeAnimation->getTimesPlayed() >= 1 || activeAnimation->getName() != "hit") {
					stats.cur_state = StatBlock::AVATAR_STANCE;
					if (settings->mouse_move) {
						drag_walking = true;
					}
				}

				break;

			case StatBlock::AVATAR_DEAD:
				allowed_to_use_power = false;

				if (stats.effects.triggered_death) break;

				if (stats.transformed) {
					stats.transform_duration = 0;
					untransform();
				}

				setAnimation("die");

				if (!stats.corpse && activeAnimation->isFirstFrame() && activeAnimation->getTimesPlayed() < 1) {
					stats.effects.clearEffects();

					// reset power cooldowns
					for (size_t i = 0; i < power_cooldown_timers.size(); i++) {
						power_cooldown_timers[i].reset(Timer::END);
						power_cast_timers[i].reset(Timer::END);
					}

					// close menus in GameStatePlay
					close_menus = true;

					playSound(Entity::SOUND_DIE);

					if (stats.permadeath) {
						// ignore death penalty on permadeath and instead delete the player's saved game
						stats.death_penalty = false;
						Utils::removeSaveDir(save_load->getGameSlot());
						menu->exit->disableSave();

						logMsg(Utils::substituteVarsInString(msg->get("You are defeated. Game over! ${INPUT_CONTINUE} to exit to Title."), this), MSG_NORMAL);
					}
					else {
						// raise the death penalty flag.  This is handled in MenuInventory
						stats.death_penalty = true;

						logMsg(Utils::substituteVarsInString(msg->get("You are defeated. ${INPUT_CONTINUE} to continue."), this), MSG_NORMAL);
					}

					// if the player is attacking, we need to block further input
					if (inpt->pressing[Input::MAIN1])
						inpt->lock[Input::MAIN1] = true;
				}

				if (activeAnimation->getTimesPlayed() >= 1 || activeAnimation->getName() != "die") {
					stats.corpse = true;
				}

				// allow respawn with Accept if not permadeath
				if ((inpt->pressing[Input::ACCEPT] || (settings->touchscreen && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1])) && stats.corpse) {
					if (inpt->pressing[Input::ACCEPT]) inpt->lock[Input::ACCEPT] = true;
					if (settings->touchscreen && inpt->pressing[Input::MAIN1]) inpt->lock[Input::MAIN1] = true;
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

		// handle power usage
		if (allowed_to_use_power) {
			bool blocking = false;

			for (unsigned i=0; i<action_queue.size(); i++) {
				ActionData &action = action_queue[i];
				int power_id = powers->checkReplaceByEffect(action.power, &stats);
				const Power &power = powers->powers[power_id];

				if (power.type == Power::TYPE_BLOCK)
					blocking = true;

				if (power_id != 0 && (stats.cooldown.isEnd() || action.instant_item)) {
					FPoint target = action.target;

					// check requirements
					if ((stats.cur_state == StatBlock::AVATAR_ATTACK || stats.cur_state == StatBlock::AVATAR_HIT) && !action.instant_item)
						continue;
					if (!stats.canUsePower(power_id, !StatBlock::CAN_USE_PASSIVE))
						continue;
					if (power.requires_los && !mapr->collider.lineOfSight(stats.pos.x, stats.pos.y, target.x, target.y))
						continue;
					if (power.requires_empty_target && !mapr->collider.isEmpty(target.x, target.y))
						continue;
					if (!power_cooldown_timers[power_id].isEnd())
						continue;
					if (!powers->hasValidTarget(power_id, &stats, target))
						continue;

					// automatically target the selected enemy with melee attacks
					if (inpt->usingMouse() && power.type == Power::TYPE_FIXED && power.starting_pos == Power::STARTING_POS_MELEE && cursor_enemy) {
						target = cursor_enemy->stats.pos;
					}

					// is this a power that requires changing direction?
					if (power.face) {
						stats.direction = Utils::calcDirection(stats.pos.x, stats.pos.y, target.x, target.y);
					}

					if (power.new_state != Power::STATE_INSTANT) {
						current_power = power_id;
						act_target = target;
						attack_anim = power.attack_anim;
					}

					if (power.state_duration > 0)
						stats.state_timer.setDuration(power.state_duration);

					if (power.charge_speed != 0.0f)
						stats.charge_speed = power.charge_speed;

					stats.prevent_interrupt = power.prevent_interrupt;

					if (power.pre_power > 0 && Math::percentChance(power.pre_power_chance)) {
						powers->activate(power.pre_power, &stats, target);
					}

					switch (power.new_state) {
						case Power::STATE_ATTACK:	// handle attack powers
							stats.cur_state = StatBlock::AVATAR_ATTACK;
							break;

						case Power::STATE_INSTANT:	// handle instant powers
							powers->activate(power_id, &stats, target);
							power_cooldown_timers[power_id].setDuration(power.cooldown);
							break;

						default:
							if (power.type == Power::TYPE_BLOCK) {
								stats.cur_state = StatBlock::AVATAR_BLOCK;
								powers->activate(power_id, &stats, target);
								stats.refresh_stats = true;
							}
							break;
					}

					// if the player is attacking, show the attack cursor
					attack_cursor = (
						stats.cur_state == StatBlock::AVATAR_ATTACK &&
						!power.buff && !power.buff_teleport &&
						power.type != Power::TYPE_TRANSFORM &&
						power.type != Power::TYPE_BLOCK &&
						!(power.starting_pos == Power::STARTING_POS_SOURCE && power.speed == 0)
					);

				}
			}

			stats.blocking = blocking;
		}

	}

	// calc new cam position from player position
	// cam is focused at player position
	float cam_dx = (Utils::calcDist(FPoint(mapr->cam.x, stats.pos.y), stats.pos)) / eset->misc.camera_speed;
	float cam_dy = (Utils::calcDist(FPoint(stats.pos.x, mapr->cam.y), stats.pos)) / eset->misc.camera_speed;

	if (mapr->cam.x < stats.pos.x) {
		mapr->cam.x += cam_dx;
		if (mapr->cam.x > stats.pos.x)
			mapr->cam.x = stats.pos.x;
	}
	else if (mapr->cam.x > stats.pos.x) {
		mapr->cam.x -= cam_dx;
		if (mapr->cam.x < stats.pos.x)
			mapr->cam.x = stats.pos.x;
	}
	if (mapr->cam.y < stats.pos.y) {
		mapr->cam.y += cam_dy;
		if (mapr->cam.y > stats.pos.y)
			mapr->cam.y = stats.pos.y;
	}
	else if (mapr->cam.y > stats.pos.y) {
		mapr->cam.y -= cam_dy;
		if (mapr->cam.y < stats.pos.y)
			mapr->cam.y = stats.pos.y;
	}

	// check for map events
	mapr->checkEvents(stats.pos);

	// decrement all cooldowns
	for (unsigned i = 0; i < power_cooldown_timers.size(); i++) {
		power_cooldown_timers[i].tick();
		power_cast_timers[i].tick();
	}

	// make the current square solid
	mapr->collider.block(stats.pos.x, stats.pos.y, !MapCollision::IS_ALLY);

	if (stats.state_timer.isEnd() && stats.hold_state)
		stats.hold_state = false;

	if (stats.cur_state != StatBlock::AVATAR_ATTACK && stats.charge_speed != 0.0f)
		stats.charge_speed = 0.0f;
}

void Avatar::transform() {
	// calling a transform power locks the actionbar, so we unlock it here
	inpt->unlockActionBar();

	delete charmed_stats;
	charmed_stats = NULL;

	Enemy_Level el = enemyg->getRandomEnemy(stats.transform_type, 0, 0);

	if (el.type != "") {
		charmed_stats = new StatBlock();
		charmed_stats->load(el.type);
	}
	else {
		Utils::logError("Avatar: Could not transform into creature type '%s'", stats.transform_type.c_str());
		stats.transform_type = "";
		return;
	}

	transform_triggered = true;
	stats.transformed = true;
	setPowers = true;

	// temporary save hero stats
	delete hero_stats;

	hero_stats = new StatBlock();
	*hero_stats = stats;

	// do not allow two copies of the summons list
	hero_stats->summons.clear();

	// replace some hero stats
	stats.speed = charmed_stats->speed;
	stats.flying = charmed_stats->flying;
	stats.intangible = charmed_stats->intangible;
	stats.humanoid = charmed_stats->humanoid;
	stats.animations = charmed_stats->animations;
	stats.powers_list = charmed_stats->powers_list;
	stats.powers_passive = charmed_stats->powers_passive;
	stats.effects.clearEffects();

	anim->decreaseCount("animations/hero.txt");
	anim->increaseCount(charmed_stats->animations);
	animationSet = anim->getAnimationSet(charmed_stats->animations);
	delete activeAnimation;
	activeAnimation = animationSet->getAnimation("");
	stats.cur_state = StatBlock::AVATAR_STANCE;

	// base stats
	for (int i=0; i<Stats::COUNT; ++i) {
		stats.starting[i] = std::max(stats.starting[i], charmed_stats->starting[i]);
	}

	// resistances
	for (unsigned int i=0; i<stats.vulnerable.size(); i++) {
		stats.vulnerable[i] = std::min(stats.vulnerable[i], charmed_stats->vulnerable[i]);
	}

	loadSoundsFromStatBlock(charmed_stats);
	loadStepFX("NULL");

	stats.applyEffects();

	transform_pos = stats.pos;
	transform_map = mapr->getFilename();
}

void Avatar::untransform() {
	// calling a transform power locks the actionbar, so we unlock it here
	inpt->unlockActionBar();

	// For timed transformations, move the player to the last valid tile when untransforming
	mapr->collider.unblock(stats.pos.x, stats.pos.y);
	if (!mapr->collider.isValidPosition(stats.pos.x, stats.pos.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_HERO)) {
		logMsg(msg->get("Transformation expired. You have been moved back to a safe place."), MSG_NORMAL);
		if (transform_map != mapr->getFilename()) {
			mapr->teleportation = true;
			mapr->teleport_mapname = transform_map;
			mapr->teleport_destination.x = floorf(transform_pos.x) + 0.5f;
			mapr->teleport_destination.y = floorf(transform_pos.y) + 0.5f;
			transform_map = "";
		}
		else {
			stats.pos.x = floorf(transform_pos.x) + 0.5f;
			stats.pos.y = floorf(transform_pos.y) + 0.5f;
		}
	}
	mapr->collider.block(stats.pos.x, stats.pos.y, !MapCollision::IS_ALLY);

	stats.transformed = false;
	transform_triggered = true;
	stats.transform_type = "";
	revertPowers = true;
	stats.effects.clearEffects();

	// revert some hero stats to last saved
	stats.speed = hero_stats->speed;
	stats.flying = hero_stats->flying;
	stats.intangible = hero_stats->intangible;
	stats.humanoid = hero_stats->humanoid;
	stats.animations = hero_stats->animations;
	stats.effects = hero_stats->effects;
	stats.powers_list = hero_stats->powers_list;
	stats.powers_passive = hero_stats->powers_passive;

	anim->increaseCount("animations/hero.txt");
	anim->decreaseCount(charmed_stats->animations);
	animationSet = anim->getAnimationSet("animations/hero.txt");
	delete activeAnimation;
	activeAnimation = animationSet->getAnimation("");
	stats.cur_state = StatBlock::AVATAR_STANCE;

	// This is a bit of a hack.
	// In order to switch to the stance animation, we can't already be in a stance animation
	setAnimation("run");

	for (int i=0; i<Stats::COUNT; ++i) {
		stats.starting[i] = hero_stats->starting[i];
	}

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
	stats.untransform_on_hit = false;
}

void Avatar::checkTransform() {
	// handle transformation
	if (stats.transform_type != "" && stats.transform_type != "untransform" && stats.transformed == false)
		transform();
	if (stats.transform_type != "" && stats.transform_duration == 0)
		untransform();
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

void Avatar::resetActiveAnimation() {
	activeAnimation->reset(); // shield stutter
	for (unsigned i=0; i < animsets.size(); i++)
		if (anims[i])
			anims[i]->reset();
}

void Avatar::addRenders(std::vector<Renderable> &r) {
	if (!stats.transformed) {
		for (unsigned i = 0; i < layer_def[stats.direction].size(); ++i) {
			unsigned index = layer_def[stats.direction][i];
			if (anims[index]) {
				Renderable ren = anims[index]->getCurrentFrame(stats.direction);
				ren.map_pos = stats.pos;
				ren.prio = i+1;
				stats.effects.getCurrentColor(ren.color_mod);
				stats.effects.getCurrentAlpha(ren.alpha_mod);
				if (stats.hp > 0) {
					ren.type = Renderable::TYPE_HERO;
				}
				r.push_back(ren);
			}
		}
	}
	else {
		Renderable ren = activeAnimation->getCurrentFrame(stats.direction);
		ren.map_pos = stats.pos;
		stats.effects.getCurrentColor(ren.color_mod);
		stats.effects.getCurrentAlpha(ren.alpha_mod);
		if (stats.hp > 0) {
			ren.type = Renderable::TYPE_HERO;
		}
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

void Avatar::logMsg(const std::string& str, int type) {
	log_msg.push(std::pair<std::string, int>(str, type));
}

// isLowHp returns true if health is below set threshold
bool Avatar::isLowHp() {
	if (stats.hp == 0)
		return false;
	float hp_one_perc = static_cast<float>(std::max(stats.get(Stats::HP_MAX), 1)) / 100.0f;
	return static_cast<float>(stats.hp)/hp_one_perc < static_cast<float>(settings->low_hp_threshold);
}

// isDroppedToLowHp returns true only if player hp just dropped below threshold
bool Avatar::isDroppedToLowHp() {
	float hp_one_perc = static_cast<float>(std::max(stats.get(Stats::HP_MAX), 1)) / 100.0f;
	return static_cast<float>(stats.hp)/hp_one_perc < static_cast<float>(settings->low_hp_threshold) &&
		static_cast<float>(prev_hp)/hp_one_perc >= static_cast<float>(settings->low_hp_threshold);
}

bool Avatar::isLowHpMessageEnabled() {
	return settings->low_hp_warning_type == settings->LHP_WARN_TEXT ||
		settings->low_hp_warning_type == settings->LHP_WARN_TEXT_CURSOR ||
		settings->low_hp_warning_type == settings->LHP_WARN_TEXT_SOUND ||
		settings->low_hp_warning_type == settings->LHP_WARN_ALL;
}

bool Avatar::isLowHpSoundEnabled() {
	return settings->low_hp_warning_type == settings->LHP_WARN_SOUND ||
		settings->low_hp_warning_type == settings->LHP_WARN_TEXT_SOUND ||
		settings->low_hp_warning_type == settings->LHP_WARN_CURSOR_SOUND ||
		settings->low_hp_warning_type == settings->LHP_WARN_ALL;
}

bool Avatar::isLowHpCursorEnabled() {
	return settings->low_hp_warning_type == settings->LHP_WARN_CURSOR ||
		settings->low_hp_warning_type == settings->LHP_WARN_TEXT_CURSOR ||
		settings->low_hp_warning_type == settings->LHP_WARN_CURSOR_SOUND ||
		settings->low_hp_warning_type == settings->LHP_WARN_ALL;
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

	unloadSounds();

	for (unsigned i=0; i<sound_steps.size(); i++)
		snd->unload(sound_steps[i]);
}
