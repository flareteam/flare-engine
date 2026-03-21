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
		TOOLTIP,
		INTERMAP,
		INTERMAP_ID,
		INTRAMAP,
		MAPMOD,
		MAPMOD_TOGGLE,
		SOUNDFX,
		LOOT,
		LOOT_COUNT,
		MSG,
		SHAKYCAM,
		REQUIRES_STATUS,
		REQUIRES_NOT_STATUS,
		REQUIRES_LEVEL,
		REQUIRES_NOT_LEVEL,
		REQUIRES_CURRENCY,
		REQUIRES_NOT_CURRENCY,
		REQUIRES_ITEM,
		REQUIRES_NOT_ITEM,
		REQUIRES_CLASS,
		REQUIRES_NOT_CLASS,
		REQUIRES_TILE,
		REQUIRES_NOT_TILE,
		SET_STATUS,
		UNSET_STATUS,
		REMOVE_CURRENCY,
		REMOVE_ITEM,
		REWARD_XP,
		REWARD_CURRENCY,
		REWARD_ITEM,
		REWARD_LOOT,
		REWARD_LOOT_COUNT,
		RESTORE,
		POWER,
		POWER_PATH,
		POWER_DAMAGE,
		POWER_STATS,
		POWER_LEVEL,
		SPAWN,
		SPAWN_LEVEL,
		STASH,
		NPC,
		MUSIC,
		CUTSCENE,
		REPEAT,
		SAVE_GAME,
		BOOK,
		SCRIPT,
		CHANCE_EXEC,
		RESPEC,
		SHOW_ON_MINIMAP,
		PARALLAX_LAYERS,
		RANDOM_STATUS,
		PROCGEN_FILENAME,
		PROCGEN_LINK,
		PROCGEN_DOOR_LEVEL,
		NPC_ID,
		NPC_HOTSPOT,
		NPC_DIALOG_THEM,
		NPC_DIALOG_YOU,
		NPC_VOICE,
		NPC_DIALOG_TOPIC,
		NPC_DIALOG_GROUP,
		NPC_DIALOG_ID,
		NPC_DIALOG_RESPONSE,
		NPC_DIALOG_RESPONSE_ONLY,
		NPC_ALLOW_MOVEMENT,
		NPC_PORTRAIT_THEM,
		NPC_PORTRAIT_YOU,
		QUEST_TEXT,
		WAS_INSIDE_EVENT_AREA,
		NPC_TAKE_A_PARTY,
		EVENT_COMPONENT_COUNT,
	};

	enum {
		RANDOM_STATUS_MODE_APPEND = 1,
		RANDOM_STATUS_MODE_CLEAR = 2,
		RANDOM_STATUS_MODE_ROLL = 3,
		RANDOM_STATUS_MODE_SET = 4,
		RANDOM_STATUS_MODE_UNSET = 5,
	};

	static const size_t DATA_COUNT = 11;

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

	std::string getIntermapIDString(size_t index);

private:
	static const bool SKIP_DELAY = true;
	bool executeEventInternal(Event &e, bool skip_delay);
	EventComponent getRandomMapFromFile(const std::string& fname);
	size_t getIntermapID(const std::string& s);

	std::map< std::string, std::queue<Event> > script_cache;

	std::vector<std::string> intermap_ids;
};


#endif
