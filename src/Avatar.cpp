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
#include "Entity.h"
#include "EntityManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "InputState.h"
#include "MapRenderer.h"
#include "MenuActionBar.h"
#include "MenuExit.h"
#include "MenuGameOver.h"
#include "MenuInventory.h"
#include "MenuManager.h"
#include "MessageEngine.h"
#include "MenuMiniMap.h"
#include "ModManager.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

Avatar::Avatar()
	: Entity()
	, mm_key(settings->mouse_move_swap ? Input::MAIN2 : Input::MAIN1)
	, mm_is_distant(false)
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
	, teleport_camera_lock(false)
	, feet_index(-1)
{
	power_cooldown_timers.resize(powers->powers.size(), NULL);
	power_cast_timers.resize(powers->powers.size(), NULL);

	for (size_t i = 0; i < powers->powers.size(); ++i) {
		if (powers->isValid(i)) {
			power_cooldown_ids.push_back(i);
			power_cooldown_timers[i] = new Timer();
			power_cast_timers[i] = new Timer();
		}
	}

	init();

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
	stats.cur_state = StatBlock::ENTITY_STANCE;
	if (mapr->hero_pos_enabled) {
		stats.pos.x = mapr->hero_pos.x;
		stats.pos.y = mapr->hero_pos.y;
	}
	current_power = 0;
	current_power_original = 0;
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
	for (size_t i = 0; i < powers->powers.size(); ++i) {
		if (!powers->isValid(i))
			continue;

		if (untransform_power == 0 && powers->powers[i]->required_items.empty() && powers->powers[i]->spawn_type == "untransform") {
			untransform_power = i;
		}

		if (power_cooldown_timers[i])
			*(power_cooldown_timers[i]) = Timer();
		if (power_cast_timers[i])
			*(power_cast_timers[i]) = Timer();
	}

	stats.animations = "animations/hero.txt";
}

void Avatar::handleNewMap() {
	cursor_enemy = NULL;
	lock_enemy = NULL;
	playing_lowhp = false;

	stats.target_corpse = NULL;
	stats.target_nearest = NULL;
	stats.target_nearest_corpse = NULL;
}

/**
 * Load avatar sprite layer definitions into vector.
 */
void Avatar::loadLayerDefinitions() {
	if (!stats.layer_reference_order.empty())
		return;

	Utils::logError("Avatar: Loading render layers from engine/hero_layers.txt is deprecated! Render layers should be loaded in the 'render_layers' section of engine/stats.txt.");

	FileParser infile;
	if (infile.open("engine/hero_layers.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			// hero_layers.txt doesn't have sections, but we now expect the layer key to be under one called "render_layers"
			if (infile.section.empty())
				infile.section = "render_layers";

			if (!stats.loadRenderLayerStat(&infile)) {
				infile.error("Avatar: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
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
	else if (checkMouseMoveEnabled()) {
		return inpt->pressing[mm_key] && !inpt->pressing[Input::SHIFT] && mm_is_distant;
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
	if (checkMouseMoveEnabled()) {
		if (mm_is_distant) {
			FPoint target = Utils::screenToMap(inpt->mouse.x, inpt->mouse.y, mapr->cam.pos.x, mapr->cam.pos.y);
			stats.direction = Utils::calcDirection(stats.pos.x, stats.pos.y, target.x, target.y);
		}
	}
	else {
		// movement keys take top priority for setting direction
		bool press_up = inpt->pressing[Input::UP] && !inpt->lock[Input::UP];
		bool press_down = inpt->pressing[Input::DOWN] && !inpt->lock[Input::DOWN];
		bool press_left = inpt->pressing[Input::LEFT] && !inpt->lock[Input::LEFT];
		bool press_right = inpt->pressing[Input::RIGHT] && !inpt->lock[Input::RIGHT];

		// aiming keys can set direction as well
		if (!press_up && !press_down && !press_left && !press_right) {
			press_up = inpt->pressing[Input::AIM_UP] && !inpt->lock[Input::AIM_UP];
			press_down = inpt->pressing[Input::AIM_DOWN] && !inpt->lock[Input::AIM_DOWN];
			press_left = inpt->pressing[Input::AIM_LEFT] && !inpt->lock[Input::AIM_LEFT];
			press_right = inpt->pressing[Input::AIM_RIGHT] && !inpt->lock[Input::AIM_RIGHT];
		}

		if (press_up && press_left) stats.direction = 1;
		else if (press_up && press_right) stats.direction = 3;
		else if (press_down && press_right) stats.direction = 5;
		else if (press_down && press_left) stats.direction = 7;
		else if (press_left) stats.direction = 0;
		else if (press_up) stats.direction = 2;
		else if (press_right) stats.direction = 4;
		else if (press_down) stats.direction = 6;
		// Adjust for ORTHO tilesets
		if (eset->tileset.orientation == eset->tileset.TILESET_ORTHOGONAL && (press_up || press_down || press_left || press_right))
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
 */
void Avatar::logic() {
	bool restrict_power_use = false;
	if (checkMouseMoveEnabled()) {
		if(inpt->pressing[mm_key] && !inpt->pressing[Input::SHIFT] && !menu->act->isWithinSlots(inpt->mouse) && !menu->act->isWithinMenus(inpt->mouse)) {
			restrict_power_use = true;
		}
	}

	// clear current space to allow correct movement
	mapr->collider.unblock(stats.pos.x, stats.pos.y);

	// turn on all passive powers
	if ((stats.hp > 0 || stats.effects.triggered_death) && !respawn && !transform_triggered)
		powers->activatePassives(&stats);

	if (transform_triggered)
		transform_triggered = false;

	// handle when the player stops blocking
	if (stats.effects.triggered_block && !stats.blocking) {
		stats.cur_state = StatBlock::ENTITY_STANCE;
		stats.effects.triggered_block = false;
		stats.effects.clearTriggerEffects(Power::TRIGGER_BLOCK);
		stats.refresh_stats = true;
		stats.block_power = 0;
	}

	stats.logic();

	// alert on low health
	if (isDroppedToLowHp()) {
		// show message if set
		if (isLowHpMessageEnabled()) {
			logMsg(msg->get("Your health is low!"), MSG_NORMAL);
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
		stats.level = eset->xp.getLevelFromXP(stats.xp);
		logMsg(msg->getv("Congratulations, you have reached level %d!", stats.level), MSG_NORMAL);
		if (pc->stats.stat_points_per_level > 0) {
			logMsg(msg->get("You may increase one or more attributes through the Character Menu."), MSG_NORMAL);
			newLevelNotification = true;
		}
		if (pc->stats.power_points_per_level > 0) {
			logMsg(msg->get("You may unlock one or more abilities through the Powers Menu."), MSG_NORMAL);
		}
		stats.recalc();
		snd->play(sound_levelup, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);

		// if the player managed to level up while dead (e.g. via a bleeding creature), restore to life
		if (stats.cur_state == StatBlock::ENTITY_DEAD) {
			stats.cur_state = StatBlock::ENTITY_STANCE;
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
		if (activeAnimation)
			activeAnimation->advanceFrame();

		for (size_t i = 0; i < anims.size(); ++i) {
			if (anims[i])
				anims[i]->advanceFrame();
		}
	}

	// save a valid tile position in the event that we untransform on an invalid tile
	if (stats.transformed && mapr->collider.isValidPosition(stats.pos.x, stats.pos.y, MapCollision::MOVE_NORMAL, MapCollision::ENTITY_COLLIDE_HERO)) {
		transform_pos = stats.pos;
		transform_map = mapr->getFilename();
	}

	PowerID mm_attack_id = (settings->mouse_move_swap ? menu->act->getSlotPower(MenuActionBar::SLOT_MAIN2) : menu->act->getSlotPower(MenuActionBar::SLOT_MAIN1));
	bool mm_can_use_power = true;

	if (checkMouseMoveEnabled()) {
		if (!inpt->pressing[mm_key]) {
			lock_enemy = NULL;
		}
		else {
			// prevents erratic behavior when mouse move is too close to player
			FPoint target = Utils::screenToMap(inpt->mouse.x, inpt->mouse.y, mapr->cam.pos.x, mapr->cam.pos.y);
			if (stats.cur_state == StatBlock::ENTITY_MOVE) {
				mm_is_distant = Utils::calcDist(stats.pos, target) >= eset->misc.mouse_move_deadzone_moving;
			}
			else {
				mm_is_distant = Utils::calcDist(stats.pos, target) >= eset->misc.mouse_move_deadzone_not_moving;
			}
		}

		if (lock_enemy && lock_enemy->stats.hp <= 0) {
			lock_enemy = NULL;
		}

		if (powers->isValid(mm_attack_id)) {
			if (!stats.canUsePower(mm_attack_id, !StatBlock::CAN_USE_PASSIVE)) {
				lock_enemy = NULL;
				mm_can_use_power = false;
			}
			else if (!power_cooldown_timers[mm_attack_id]->isEnd()) {
				lock_enemy = NULL;
				mm_can_use_power = false;
			}
			else if (lock_enemy && powers->powers[mm_attack_id]->requires_los && !mapr->collider.lineOfSight(stats.pos.x, stats.pos.y, lock_enemy->stats.pos.x, lock_enemy->stats.pos.y)) {
				lock_enemy = NULL;
				mm_can_use_power = false;
			}
		}
	}

	if (teleport_camera_lock && Utils::calcDist(stats.pos, mapr->cam.pos) < 0.5f) {
		teleport_camera_lock = false;
	}

	set_dir_timer.tick();
	if (!pressing_move()) {
		set_dir_timer.reset(Timer::END);
	}

	if (!stats.effects.stun) {
		bool allowed_to_move;
		bool allowed_to_turn;

		stats.blocking = false;

		for (unsigned i=0; i<action_queue.size(); i++) {
			ActionData &action = action_queue[i];
			PowerID replaced_id = powers->checkReplaceByEffect(action.power, &stats);
			if (replaced_id == 0)
				continue;

			Power* power = powers->powers[replaced_id];

			if (power->new_state == Power::STATE_INSTANT) {
				// instant power, so no need to switch animation state
				FPoint target = action.target;
				beginPower(replaced_id, &target);
				powers->activate(replaced_id, &stats, stats.pos, target);
				power_cooldown_timers[action.power]->setDuration(power->cooldown);
				power_cooldown_timers[replaced_id]->setDuration(power->cooldown);
			}
			else if (stats.cur_state == StatBlock::ENTITY_BLOCK) {
				// special case in order to continue blocking
				if (power->type == Power::TYPE_BLOCK) {
					current_power = replaced_id;
					current_power_original = action.power;
					act_target = action.target;
					attack_anim = power->attack_anim;

					stats.cur_state = StatBlock::ENTITY_BLOCK;
					beginPower(replaced_id, &act_target);
					powers->activate(replaced_id, &stats, stats.pos, act_target);
					stats.refresh_stats = true;
				}
			}
			else if (stats.cur_state == StatBlock::ENTITY_STANCE || stats.cur_state == StatBlock::ENTITY_MOVE) {
				// this power has an animation, so prepare to switch to it
				current_power = replaced_id;
				current_power_original = action.power;
				act_target = action.target;
				attack_anim = power->attack_anim;
				resetActiveAnimation();

				if (power->new_state == Power::STATE_ATTACK) {
					stats.cur_state = StatBlock::ENTITY_POWER;
				}
				else if (power->type == Power::TYPE_BLOCK) {
					stats.cur_state = StatBlock::ENTITY_BLOCK;
					beginPower(replaced_id, &act_target);
					powers->activate(replaced_id, &stats, stats.pos, act_target);
					stats.refresh_stats = true;
				}
			}
		}

		action_queue.clear();

		switch(stats.cur_state) {
			case StatBlock::ENTITY_STANCE:

				setAnimation("stance");

				// allowed to move or use powers?
				if (checkMouseMoveEnabled()) {
					allowed_to_move = restrict_power_use && (!inpt->lock[mm_key] || drag_walking) && !lock_enemy;
					allowed_to_turn = allowed_to_move;

					if ((inpt->pressing[mm_key] && inpt->pressing[Input::SHIFT]) || lock_enemy) {
						inpt->lock[mm_key] = false;
					}
				}
				else if (!settings->mouse_aim) {
					allowed_to_move = !inpt->pressing[Input::SHIFT];
					allowed_to_turn = true;
				}
				else {
					allowed_to_move = true;
					allowed_to_turn = true;
				}

				// handle transitions to RUN
				if (allowed_to_turn)
					set_direction();

				if (pressing_move() && allowed_to_move) {
					if (move()) { // no collision
						if (checkMouseMoveEnabled() && inpt->pressing[mm_key]) {
							inpt->lock[mm_key] = true;
							drag_walking = true;
						}

						stats.cur_state = StatBlock::ENTITY_MOVE;
					}
				}

				if (checkMouseMoveEnabled() &&  settings->mouse_move_attack && cursor_enemy && !cursor_enemy->stats.hero_ally && mm_can_use_power && powers->checkCombatRange(mm_attack_id, &stats, cursor_enemy->stats.pos)) {
					stats.cur_state = StatBlock::ENTITY_STANCE;
					lock_enemy = cursor_enemy;
				}

				break;

			case StatBlock::ENTITY_MOVE:

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
					stats.cur_state = StatBlock::ENTITY_STANCE;
					break;
				}
				else if (!move()) { // collide with wall
					stats.cur_state = StatBlock::ENTITY_STANCE;
					break;
				}
				else if ((checkMouseMoveEnabled() || !settings->mouse_aim) && inpt->pressing[Input::SHIFT]) {
					// Shift should stop movement in some cases.
					// With mouse_move, it allows the player to stop moving and begin attacking.
					// With mouse_aim disabled, it allows the player to aim their attacks without having to move.
					stats.cur_state = StatBlock::ENTITY_STANCE;
					break;
				}

				if (activeAnimation->getName() != "run")
					stats.cur_state = StatBlock::ENTITY_STANCE;

				if (checkMouseMoveEnabled() && settings->mouse_move_attack && cursor_enemy && !cursor_enemy->stats.hero_ally && mm_can_use_power && powers->checkCombatRange(mm_attack_id, &stats, cursor_enemy->stats.pos)) {
					stats.cur_state = StatBlock::ENTITY_STANCE;
					lock_enemy = cursor_enemy;
				}

				break;

			case StatBlock::ENTITY_POWER:

				setAnimation(attack_anim);

				if (powers->isValid(current_power)) {
					Power* power = powers->powers[current_power];

					// if the player is attacking, show the attack cursor
					if (!power->buff && !power->buff_teleport &&
						power->type != Power::TYPE_TRANSFORM &&
						power->type != Power::TYPE_BLOCK &&
						!(power->starting_pos == Power::STARTING_POS_SOURCE && power->speed == 0)
					) {
						curs->setCursor(CursorManager::CURSOR_ATTACK);
					}

					if (activeAnimation->isFirstFrame()) {
						beginPower(current_power, &act_target);
						float attack_speed = (stats.effects.getAttackSpeed(attack_anim) * power->attack_speed) / 100.0f;
						activeAnimation->setSpeed(attack_speed);
						for (size_t i=0; i<anims.size(); ++i) {
							if (anims[i])
								anims[i]->setSpeed(attack_speed);
						}
						playAttackSound(attack_anim);
						power_cast_timers[current_power]->setDuration(activeAnimation->getDuration());
						power_cast_timers[current_power_original]->setDuration(activeAnimation->getDuration()); // for replace_by_effect
					}

					// do power
					if (activeAnimation->isActiveFrame() && !stats.hold_state) {
						// some powers check if the caster is blocking a tile
						// so we block the player tile prematurely here
						mapr->collider.block(stats.pos.x, stats.pos.y, !MapCollision::IS_ALLY);

						powers->activate(current_power, &stats, stats.pos, act_target);
						power_cooldown_timers[current_power]->setDuration(power->cooldown);
						power_cooldown_timers[current_power_original]->setDuration(power->cooldown); // for replace_by_effect

						if (!stats.state_timer.isEnd())
							stats.hold_state = true;
					}
				}

				// animation is done, switch back to normal stance
				if ((activeAnimation->isLastFrame() && stats.state_timer.isEnd()) || activeAnimation->getName() != attack_anim) {
					stats.cur_state = StatBlock::ENTITY_STANCE;
					stats.cooldown.reset(Timer::BEGIN);
					stats.prevent_interrupt = false;
					if (checkMouseMoveEnabled()) {
						drag_walking = true;
					}
				}

				if (checkMouseMoveEnabled() && lock_enemy && !powers->checkCombatRange(mm_attack_id, &stats, lock_enemy->stats.pos)) {
					lock_enemy = NULL;
				}

				break;

			case StatBlock::ENTITY_BLOCK:

				setAnimation("block");

				break;

			case StatBlock::ENTITY_HIT:

				setAnimation("hit");

				if (activeAnimation->isFirstFrame()) {
					stats.effects.triggered_hit = true;

					if (powers->isValid(stats.block_power)) {
						power_cooldown_timers[stats.block_power]->setDuration(powers->powers[stats.block_power]->cooldown);
						stats.block_power = 0;
					}
				}

				if (activeAnimation->getTimesPlayed() >= 1 || activeAnimation->getName() != "hit") {
					stats.cur_state = StatBlock::ENTITY_STANCE;
					if (checkMouseMoveEnabled()) {
						drag_walking = true;
					}
				}

				break;

			case StatBlock::ENTITY_DEAD:
				if (stats.effects.triggered_death) break;

				if (stats.transformed) {
					stats.transform_duration = 0;
					untransform();
				}

				setAnimation("die");

				if (!stats.corpse && activeAnimation->isFirstFrame() && activeAnimation->getTimesPlayed() < 1) {
					stats.effects.clearEffects();
					stats.powers_passive.clear();

					// reset power cooldowns
					std::map<size_t, Timer>::iterator pct_it;
					for (size_t i = 0; i < power_cooldown_timers.size(); ++i) {
						if (power_cooldown_timers[i])
							power_cooldown_timers[i]->reset(Timer::END);
						if (power_cast_timers[i])
							power_cast_timers[i]->reset(Timer::END);
					}

					// close menus in GameStatePlay
					close_menus = true;

					playSound(Entity::SOUND_DIE);

					logMsg(msg->get("You are defeated."), MSG_NORMAL);

					if (stats.permadeath) {
						// ignore death penalty on permadeath and instead delete the player's saved game
						stats.death_penalty = false;
						Utils::removeSaveDir(save_load->getGameSlot());
						menu->exit->disableSave();
						menu->game_over->disableSave();
					}
					else {
						// raise the death penalty flag.  This is handled in MenuInventory
						stats.death_penalty = true;
					}

					// if the player is attacking, we need to block further input
					if (inpt->pressing[Input::MAIN1])
						inpt->lock[Input::MAIN1] = true;
				}

				if (!stats.corpse && (activeAnimation->getTimesPlayed() >= 1 || activeAnimation->getName() != "die")) {
					stats.corpse = true;
					menu->game_over->visible = true;
				}

				// allow respawn with Accept if not permadeath
				if (menu->game_over->visible && menu->game_over->continue_clicked) {
					menu->game_over->close();

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
	}

	// update camera
	mapr->cam.setTarget(stats.pos);

	// check for map events
	mapr->checkEvents(stats.pos);

	// decrement all cooldowns
	for (size_t i = 0; i < power_cooldown_ids.size(); ++i) {
		PowerID power_id = power_cooldown_ids[i];
		power_cooldown_timers[power_id]->tick();
		power_cast_timers[power_id]->tick();
	}

	// make the current square solid
	mapr->collider.block(stats.pos.x, stats.pos.y, !MapCollision::IS_ALLY);

	if (stats.state_timer.isEnd() && stats.hold_state)
		stats.hold_state = false;

	if (stats.cur_state != StatBlock::ENTITY_POWER && stats.charge_speed != 0.0f)
		stats.charge_speed = 0.0f;
}

void Avatar::beginPower(PowerID replaced_id, FPoint* target) {
	if (!target)
		return;

	const Power* power = powers->powers[replaced_id];

	if (power->type == Power::TYPE_BLOCK)
		stats.blocking = true;

	// automatically target the selected enemy with melee attacks
	if (inpt->usingMouse() && power->type == Power::TYPE_FIXED && power->starting_pos == Power::STARTING_POS_MELEE && cursor_enemy) {
		*target = cursor_enemy->stats.pos;
	}

	// is this a power that requires changing direction?
	if (power->face) {
		stats.direction = Utils::calcDirection(stats.pos.x, stats.pos.y, target->x, target->y);
	}

	if (power->state_duration > 0)
		stats.state_timer.setDuration(power->state_duration);

	if (power->charge_speed != 0.0f)
		stats.charge_speed = power->charge_speed;

	stats.prevent_interrupt = power->prevent_interrupt;

	for (size_t j = 0; j < power->chain_powers.size(); ++j) {
		const ChainPower& chain_power = power->chain_powers[j];
		if (chain_power.type == ChainPower::TYPE_PRE && Math::percentChanceF(chain_power.chance)) {
			powers->activate(chain_power.id, &stats, stats.pos, *target);
		}
	}
}

void Avatar::transform() {
	// dead players can't transform
	if (stats.hp <= 0)
		return;

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
	stats.movement_type = charmed_stats->movement_type;
	stats.humanoid = charmed_stats->humanoid;
	stats.animations = charmed_stats->animations;
	stats.powers_list = charmed_stats->powers_list;
	stats.powers_passive = charmed_stats->powers_passive;
	stats.effects.clearEffects();
	stats.animations = charmed_stats->animations;
	stats.layer_reference_order = charmed_stats->layer_reference_order;
	stats.layer_def = charmed_stats->layer_def;
	stats.animation_slots = charmed_stats->animation_slots;

	anim->decreaseCount(hero_stats->animations);
	animationSet = NULL;
	loadAnimations();
	stats.cur_state = StatBlock::ENTITY_STANCE;

	// base stats
	for (int i=0; i<Stats::COUNT; ++i) {
		stats.starting[i] = std::max(stats.starting[i], charmed_stats->starting[i]);
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
	if (!mapr->collider.isValidPosition(stats.pos.x, stats.pos.y, MapCollision::MOVE_NORMAL, MapCollision::ENTITY_COLLIDE_HERO)) {
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
	stats.movement_type = hero_stats->movement_type;
	stats.humanoid = hero_stats->humanoid;
	stats.animations = hero_stats->animations;
	stats.effects = hero_stats->effects;
	stats.powers_list = hero_stats->powers_list;
	stats.powers_passive = hero_stats->powers_passive;
	stats.animations = hero_stats->animations;
	stats.layer_reference_order = hero_stats->layer_reference_order;
	stats.layer_def = hero_stats->layer_def;
	stats.animation_slots = hero_stats->animation_slots;

	anim->decreaseCount(charmed_stats->animations);
	animationSet = NULL;
	loadAnimations();
	stats.cur_state = StatBlock::ENTITY_STANCE;

	// This is a bit of a hack.
	// In order to switch to the stance animation, we can't already be in a stance animation
	setAnimation("run");

	for (int i=0; i<Stats::COUNT; ++i) {
		stats.starting[i] = hero_stats->starting[i];
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

void Avatar::logMsg(const std::string& str, int type) {
	log_msg.push(std::pair<std::string, int>(str, type));
}

// isLowHp returns true if health is below set threshold
bool Avatar::isLowHp() {
	if (stats.hp == 0)
		return false;
	float hp_one_perc = std::max(stats.get(Stats::HP_MAX), 1.f) / 100.0f;
	return stats.hp/hp_one_perc < static_cast<float>(settings->low_hp_threshold);
}

// isDroppedToLowHp returns true only if player hp just dropped below threshold
bool Avatar::isDroppedToLowHp() {
	float hp_one_perc = std::max(stats.get(Stats::HP_MAX), 1.f) / 100.0f;
	return (stats.hp/hp_one_perc < static_cast<float>(settings->low_hp_threshold)) && (prev_hp/hp_one_perc >= static_cast<float>(settings->low_hp_threshold));
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

std::string Avatar::getGfxFromType(const std::string& gfx_type) {
	feet_index = -1;
	std::string gfx;

	if (menu && menu->inv) {
		MenuItemStorage& equipment = menu->inv->inventory[MenuInventory::EQUIPMENT];

		for (int i = 0; i < equipment.getSlotNumber(); i++) {
			if (!menu->inv->isActive(i))
				continue;

			if (items->isValid(equipment[i].item) && gfx_type == equipment.slot_type[i]) {
				gfx = items->items[equipment[i].item]->gfx;
			}
			if (equipment.slot_type[i] == "feet") {
				feet_index = i;
			}
		}
	}

	// special case: if we don't have a head, use the portrait's head
	if (gfx.empty() && gfx_type == "head") {
		gfx = stats.gfx_head;
	}

	// fall back to default if it exists
	if (gfx.empty()) {
		if (Filesystem::fileExists(mods->locate("animations/avatar/" + stats.gfx_base + "/default_" + gfx_type + ".txt")))
			gfx = "default_" + gfx_type;
	}

	return gfx;
}

bool Avatar::checkMouseMoveEnabled() {
	return settings->mouse_move && !inpt->usingTouchscreen();
}

Avatar::~Avatar() {
	delete charmed_stats;
	delete hero_stats;

	unloadSounds();

	for (size_t i = 0; i < power_cooldown_timers.size(); ++i) {
		delete power_cooldown_timers[i];
		delete power_cast_timers[i];
	}

	for (unsigned i=0; i<sound_steps.size(); i++) {
		snd->unload(sound_steps[i]);
	}
}
