/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
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

#ifndef AVATAR_H
#define AVATAR_H

#include "CommonIncludes.h"
#include "Entity.h"
#include "Utils.h"

class Entity;
class StatBlock;

class ActionData {
public:
	PowerID power;
	unsigned hotkey;
	bool instant_item;
	bool activated_from_inventory;
	FPoint target;

	ActionData()
		: power(0)
		, hotkey(0)
		, instant_item(false)
		, activated_from_inventory(false)
		, target(FPoint()) {
	}
};

class Avatar : public Entity {
private:
	class Step_sfx {
	public:
		std::string id;
		std::vector<std::string> steps;
	};

	void loadLayerDefinitions();
	bool pressing_move();
	void set_direction();
	void transform();
	void untransform();
	void beginPower(PowerID power_id, FPoint* target);


	std::vector<Step_sfx> step_def;

	std::vector<SoundID> sound_steps;

	short body;

	bool transform_triggered;
	std::string last_transform;


	int mm_key; // mouse movement key
	bool mm_is_distant;

	Timer set_dir_timer;

	bool isDroppedToLowHp();

	std::vector<PowerID> power_cooldown_ids;

public:
	enum {
		MSG_NORMAL = 0,
		MSG_UNIQUE = 1
	};

	Avatar();
	~Avatar();

	void init();
	void handleNewMap();
	void loadGraphics(std::vector<Entity::Layer_gfx> _img_gfx);
	void loadStepFX(const std::string& stepname);

	void logic();

	void checkTransform();

	void logMsg(const std::string& str, int type);

	bool isLowHp();
	bool isLowHpMessageEnabled();
	bool isLowHpSoundEnabled();
	bool isLowHpCursorEnabled();

	std::string getGfxFromType(const std::string& gfx_type);

	std::queue<std::pair<std::string, int> > log_msg;

	std::string attack_anim;
	bool setPowers;
	bool revertPowers;
	PowerID untransform_power;
	StatBlock *hero_stats;
	StatBlock *charmed_stats;
	FPoint transform_pos;
	std::string transform_map;

	// vars
	PowerID current_power;
	PowerID current_power_original;
	FPoint act_target;
	bool drag_walking;
	bool newLevelNotification;
	bool respawn;
	bool close_menus;
	bool allow_movement;
	std::vector<Timer*> power_cooldown_timers;
	std::vector<Timer*> power_cast_timers;
	Entity* cursor_enemy; // enemy selected with the mouse cursor
	Entity* lock_enemy;
	unsigned long time_played;
	bool questlog_dismissed;
	bool using_main1;
	bool using_main2;
	float prev_hp;
	bool playing_lowhp;
	bool teleport_camera_lock;
	int feet_index;

	std::vector<ActionData> action_queue;
};

#endif

