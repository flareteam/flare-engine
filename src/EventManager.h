/*
Copyright © 2013 Igor Paliychuk
Copyright © 2013-2016 Justin Jacobs

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

/*
 * class EventManager
 */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "CommonIncludes.h"
#include "Utils.h"

class FileParser;

class EventComponent {
public:
	enum {
		NONE = 0,
		TOOLTIP = 1,
		POWER_PATH = 2,
		POWER_DAMAGE = 3,
		INTERMAP = 4,
		INTRAMAP = 5,
		MAPMOD = 6,
		SOUNDFX = 7,
		LOOT = 8,
		LOOT_COUNT = 9,
		MSG = 10,
		SHAKYCAM = 11,
		REQUIRES_STATUS = 12,
		REQUIRES_NOT_STATUS = 13,
		REQUIRES_LEVEL = 14,
		REQUIRES_NOT_LEVEL = 15,
		REQUIRES_CURRENCY = 16,
		REQUIRES_NOT_CURRENCY = 17,
		REQUIRES_ITEM = 18,
		REQUIRES_NOT_ITEM = 19,
		REQUIRES_CLASS = 20,
		REQUIRES_NOT_CLASS = 21,
		SET_STATUS = 22,
		UNSET_STATUS = 23,
		REMOVE_CURRENCY = 24,
		REMOVE_ITEM = 25,
		REWARD_XP = 26,
		REWARD_CURRENCY = 27,
		REWARD_ITEM = 28,
		REWARD_LOOT = 29,
		REWARD_LOOT_COUNT = 30,
		RESTORE = 31,
		POWER = 32,
		SPAWN = 33,
		STASH = 34,
		NPC = 35,
		MUSIC = 36,
		CUTSCENE = 37,
		REPEAT = 38,
		SAVE_GAME = 39,
		BOOK = 40,
		SCRIPT = 41,
		CHANCE_EXEC = 42,
		RESPEC = 43,
		SHOW_ON_MINIMAP = 44,
		PARALLAX_LAYERS = 45,
		RANDOM_STATUS = 46,
		NPC_ID = 47,
		NPC_HOTSPOT = 48,
		NPC_DIALOG_THEM = 49,
		NPC_DIALOG_YOU = 50,
		NPC_VOICE = 51,
		NPC_DIALOG_TOPIC = 52,
		NPC_DIALOG_GROUP = 53,
		NPC_DIALOG_ID = 54,
		NPC_DIALOG_RESPONSE = 55,
		NPC_DIALOG_RESPONSE_ONLY = 56,
		NPC_ALLOW_MOVEMENT = 57,
		NPC_PORTRAIT_THEM = 58,
		NPC_PORTRAIT_YOU = 59,
		QUEST_TEXT = 60,
		WAS_INSIDE_EVENT_AREA = 61,
		NPC_TAKE_A_PARTY = 62
	};

	enum {
		RANDOM_STATUS_MODE_APPEND = 1,
		RANDOM_STATUS_MODE_CLEAR = 2,
		RANDOM_STATUS_MODE_ROLL = 3,
		RANDOM_STATUS_MODE_SET = 4,
		RANDOM_STATUS_MODE_UNSET = 5,
	};

	static const size_t DATA_COUNT = 6;

	union ECData {
		int Int;
		float Float;
		bool Bool;
	};

	int type;
	std::string s;
	StatusID status;
	size_t id;
	ECData data[DATA_COUNT];

	EventComponent();
};

class Event {
public:
	enum {
		ACTIVATE_ON_TRIGGER = 0,
		ACTIVATE_ON_INTERACT = 1,
		ACTIVATE_ON_MAPEXIT = 2,
		ACTIVATE_ON_LEAVE = 3,
		ACTIVATE_ON_LOAD = 4,
		ACTIVATE_ON_CLEAR = 5,
		ACTIVATE_STATIC = 6
	};

	std::string type;
	int activate_type;
	std::vector<EventComponent> components;
	Rect location;
	Rect hotspot;
	Timer cooldown; // events that run multiple times pause this long in frames
	Timer delay;
	bool keep_after_trigger; // if this event has been triggered once, should this event be kept? If so, this event can be triggered multiple times.
	FPoint center;
	Rect reachable_from;

	Event();
	~Event();

	EventComponent* getComponent(const int _type);
	void deleteAllComponents(const int _type);
};

class EventManager {
public:
	EventManager();
	~EventManager();
	void loadEvent(FileParser &infile, Event* evnt);
	void loadEventComponent(FileParser &infile, Event* evnt, EventComponent* ec);
	bool loadEventComponentString(std::string &key, std::string &val, Event* evnt, EventComponent* ec);

	bool executeEvent(Event &e);
	bool executeDelayedEvent(Event &e);
	bool isActive(const Event &e);
	void executeScript(const std::string& filename, float x, float y);

private:
	static const bool SKIP_DELAY = true;
	bool executeEventInternal(Event &e, bool skip_delay);
	EventComponent getRandomMapFromFile(const std::string& fname);

};


#endif
