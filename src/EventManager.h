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
		NPC_ID = 46,
		NPC_HOTSPOT = 47,
		NPC_DIALOG_THEM = 48,
		NPC_DIALOG_YOU = 49,
		NPC_VOICE = 50,
		NPC_DIALOG_TOPIC = 51,
		NPC_DIALOG_GROUP = 52,
		NPC_DIALOG_ID = 53,
		NPC_DIALOG_RESPONSE = 54,
		NPC_DIALOG_RESPONSE_ONLY = 55,
		NPC_ALLOW_MOVEMENT = 56,
		NPC_PORTRAIT_THEM = 57,
		NPC_PORTRAIT_YOU = 58,
		QUEST_TEXT = 59,
		WAS_INSIDE_EVENT_AREA = 60,
		NPC_TAKE_A_PARTY = 61
	};

	int type;
	std::string s;
	StatusID status;
	int x;
	int y;
	int z;
	int a;
	int b;
	int c;
	float f;
	size_t id;

	EventComponent()
		: type(NONE)
		, s("")
		, status(0)
		, x(0)
		, y(0)
		, z(0)
		, a(0)
		, b(0)
		, c(0)
		, f(0)
		, id(0) {
	}
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
	static void loadEvent(FileParser &infile, Event* evnt);
	static void loadEventComponent(FileParser &infile, Event* evnt, EventComponent* ec);
	static bool loadEventComponentString(std::string &key, std::string &val, Event* evnt, EventComponent* ec);

	static bool executeEvent(Event &e);
	static bool executeDelayedEvent(Event &e);
	static bool isActive(const Event &e);
	static void executeScript(const std::string& filename, float x, float y);

private:
	static const bool SKIP_DELAY = true;
	static bool executeEventInternal(Event &e, bool skip_delay);
	static EventComponent getRandomMapFromFile(const std::string& fname);

};


#endif
