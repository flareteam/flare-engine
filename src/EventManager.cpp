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

#include "Avatar.h"
#include "CampaignManager.h"
#include "EnemyManager.h"
#include "EngineSettings.h"
#include "EventManager.h"
#include "FileParser.h"
#include "LootManager.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "MenuInventory.h"
#include "MenuPowers.h"
#include "MenuManager.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

/**
 * Class: Event
 */
Event::Event()
	: type("")
	, activate_type(-1)
	, components(std::vector<EventComponent>())
	, location(Rect())
	, hotspot(Rect())
	, cooldown()
	, delay()
	, keep_after_trigger(true)
	, center(FPoint(-1, -1))
	, reachable_from(Rect()) {
}

Event::~Event() {
}

/**
 * returns a pointer to the event component within the components list
 * no need to free the pointer by caller
 * NULL will be returned if no such event is found
 */
EventComponent* Event::getComponent(const int _type) {
	std::vector<EventComponent>::iterator it;
	for (it = components.begin(); it != components.end(); ++it)
		if (it->type == _type)
			return &(*it);
	return NULL;
}

void Event::deleteAllComponents(const int _type) {
	std::vector<EventComponent>::iterator it;
	for (it = components.begin(); it != components.end(); ++it)
		if (it->type == _type)
			it = components.erase(it);
}


/**
 * Class: EventManager
 */
EventManager::EventManager() {
}

EventManager::~EventManager() {
}

void EventManager::loadEvent(FileParser &infile, Event* evnt) {
	if (!evnt) return;
	// @CLASS EventManager|Description of events in maps/ and npcs/

	if (infile.key == "type") {
		// @ATTR event.type|string|(IGNORED BY ENGINE) The "type" field, as used by Tiled and other mapping tools.
		evnt->type = infile.val;
	}
	else if (infile.key == "activate") {
		// @ATTR event.activate|["on_trigger", "on_load", "on_leave", "on_mapexit", "on_clear", "static"]|Set the state in which the event will be activated (map events only).
		if (infile.val == "on_trigger") {
			evnt->activate_type = Event::ACTIVATE_ON_TRIGGER;
		}
		else if (infile.val == "on_mapexit") {
			// no need to set keep_after_trigger to false correctly, it's ignored anyway
			evnt->activate_type = Event::ACTIVATE_ON_MAPEXIT;
		}
		else if (infile.val == "on_leave") {
			evnt->activate_type = Event::ACTIVATE_ON_LEAVE;
		}
		else if (infile.val == "on_load") {
			evnt->activate_type = Event::ACTIVATE_ON_LOAD;
			evnt->keep_after_trigger = false;
		}
		else if (infile.val == "on_clear") {
			evnt->activate_type = Event::ACTIVATE_ON_CLEAR;
			evnt->keep_after_trigger = false;
		}
		else if (infile.val == "static") {
			evnt->activate_type = Event::ACTIVATE_STATIC;
		}
		else {
			infile.error("EventManager: Event activation type '%s' unknown, change to \"on_trigger\" to suppress this warning.", infile.val.c_str());
		}
	}
	else if (infile.key == "location") {
		// @ATTR event.location|rectangle|Defines the location area for the event.
		evnt->location.x = Parse::popFirstInt(infile.val);
		evnt->location.y = Parse::popFirstInt(infile.val);
		evnt->location.w = Parse::popFirstInt(infile.val);
		evnt->location.h = Parse::popFirstInt(infile.val);

		if (evnt->center.x == -1 && evnt->center.y == -1) {
			evnt->center.x = static_cast<float>(evnt->location.x) + static_cast<float>(evnt->location.w)/2;
			evnt->center.y = static_cast<float>(evnt->location.y) + static_cast<float>(evnt->location.h)/2;
		}
	}
	else if (infile.key == "hotspot") {
		//  @ATTR event.hotspot|["location", rectangle]|Event uses location as hotspot or defined by rect.
		if (infile.val == "location") {
			evnt->hotspot.x = evnt->location.x;
			evnt->hotspot.y = evnt->location.y;
			evnt->hotspot.w = evnt->location.w;
			evnt->hotspot.h = evnt->location.h;
		}
		else {
			evnt->hotspot.x = Parse::popFirstInt(infile.val);
			evnt->hotspot.y = Parse::popFirstInt(infile.val);
			evnt->hotspot.w = Parse::popFirstInt(infile.val);
			evnt->hotspot.h = Parse::popFirstInt(infile.val);
		}

		evnt->center.x = static_cast<float>(evnt->hotspot.x) + static_cast<float>(evnt->hotspot.w)/2;
		evnt->center.y = static_cast<float>(evnt->hotspot.y) + static_cast<float>(evnt->hotspot.h)/2;
	}
	else if (infile.key == "cooldown") {
		// @ATTR event.cooldown|duration|Duration for event cooldown in 'ms' or 's'.
		evnt->cooldown.setDuration(Parse::toDuration(infile.val));
		evnt->cooldown.reset(Timer::END);
	}
	else if (infile.key == "delay") {
		// @ATTR event.delay|duration|Event will execute after a specified duration.
		evnt->delay.setDuration(Parse::toDuration(infile.val));
		evnt->delay.reset(Timer::END);
	}
	else if (infile.key == "reachable_from") {
		// @ATTR event.reachable_from|rectangle|If the hero is inside this rectangle, they can activate the event.
		evnt->reachable_from.x = Parse::popFirstInt(infile.val);
		evnt->reachable_from.y = Parse::popFirstInt(infile.val);
		evnt->reachable_from.w = Parse::popFirstInt(infile.val);
		evnt->reachable_from.h = Parse::popFirstInt(infile.val);
	}
	else {
		loadEventComponent(infile, evnt, NULL);
	}
}

void EventManager::loadEventComponent(FileParser &infile, Event* evnt, EventComponent* ec) {
	if (!loadEventComponentString(infile.key, infile.val, evnt, ec)) {
		infile.error("EventManager: '%s' is not a valid key.", infile.key.c_str());
	}
}

bool EventManager::loadEventComponentString(std::string &key, std::string &val, Event* evnt, EventComponent* ec) {
	EventComponent *e = NULL;
	if (evnt) {
		evnt->components.push_back(EventComponent());
		e = &evnt->components.back();
	}
	else if (ec) {
		e = ec;
	}

	if (!e) return true;

	e->type = EventComponent::NONE;

	if (key == "tooltip") {
		// @ATTR event.tooltip|string|Tooltip for event
		e->type = EventComponent::TOOLTIP;

		e->s = msg->get(val);
	}
	else if (key == "power_path") {
		// @ATTR event.power_path|int, int, ["hero", point] : Source X, Source Y, Destination|Path that an event power will take.
		e->type = EventComponent::POWER_PATH;

		// x,y are src, if s=="hero" we target the hero,
		// else we'll use values in a,b as coordinates
		e->x = Parse::popFirstInt(val);
		e->y = Parse::popFirstInt(val);

		std::string dest = Parse::popFirstString(val);
		if (dest == "hero") {
			e->s = "hero";
		}
		else {
			e->a = Parse::toInt(dest);
			e->b = Parse::popFirstInt(val);
		}
	}
	else if (key == "power_damage") {
		// @ATTR event.power_damage|int, int : Min, Max|Range of power damage
		e->type = EventComponent::POWER_DAMAGE;

		e->a = Parse::popFirstInt(val);
		e->b = Parse::popFirstInt(val);
	}
	else if (key == "intermap") {
		// @ATTR event.intermap|filename, int, int : Map file, X, Y|Jump to specific map at location specified.
		e->type = EventComponent::INTERMAP;

		e->s = Parse::popFirstString(val);
		e->x = -1;
		e->y = -1;

		std::string test_x = Parse::popFirstString(val);
		if (!test_x.empty()) {
			e->x = Parse::toInt(test_x);
			e->y = Parse::popFirstInt(val);
		}
	}
	else if (key == "intermap_random") {
		// @ATTR event.intermap_random|filename|Pick a random map from a map list file and teleport to it.
		e->type = EventComponent::INTERMAP;

		e->s = Parse::popFirstString(val);
		e->z = 1; // flag that tells an intermap event that it contains a map list
	}
	else if (key == "intramap") {
		// @ATTR event.intramap|int, int : X, Y|Jump to specific position within current map.
		e->type = EventComponent::INTRAMAP;

		e->x = Parse::popFirstInt(val);
		e->y = Parse::popFirstInt(val);
	}
	else if (key == "mapmod") {
		// @ATTR event.mapmod|list(predefined_string, int, int, int) : Layer, X, Y, Tile ID|Modify map tiles
		e->type = EventComponent::MAPMOD;

		e->s = Parse::popFirstString(val);
		e->x = Parse::popFirstInt(val);
		e->y = Parse::popFirstInt(val);
		e->z = Parse::popFirstInt(val);

		// add repeating mapmods
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::MAPMOD;
				e->s = repeat_val;
				e->x = Parse::popFirstInt(val);
				e->y = Parse::popFirstInt(val);
				e->z = Parse::popFirstInt(val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "soundfx") {
		// @ATTR event.soundfx|filename, int, int, bool : Sound file, X, Y, loop|Filename of a sound to play. Optionally, it can be played at a specific location and/or looped.
		e->type = EventComponent::SOUNDFX;

		e->s = Parse::popFirstString(val);
		e->x = e->y = -1;
		e->z = static_cast<int>(false);

		std::string s = Parse::popFirstString(val);
		if (s != "") e->x = Parse::toInt(s);

		s = Parse::popFirstString(val);
		if (s != "") e->y = Parse::toInt(s);

		s = Parse::popFirstString(val);
		if (s != "") e->z = static_cast<int>(Parse::toBool(s));
	}
	else if (key == "loot") {
		// @ATTR event.loot|list(loot)|Add loot to the event.
		e->type = EventComponent::LOOT;

		loot->parseLoot(val, e, &evnt->components);
	}
	else if (key == "loot_count") {
		// @ATTR event.loot_count|int, int : Min, Max|Sets the minimum (and optionally, the maximum) amount of loot this event can drop. Overrides the global drop_max setting.
		e->type = EventComponent::LOOT_COUNT;

		e->x = Parse::popFirstInt(val);
		e->y = Parse::popFirstInt(val);
		if (e->x != 0 || e->y != 0) {
			e->x = std::max(e->x, 1);
			e->y = std::max(e->y, e->x);
		}
	}
	else if (key == "msg") {
		// @ATTR event.msg|string|Adds a message to be displayed for the event.
		e->type = EventComponent::MSG;

		e->s = msg->get(val);
	}
	else if (key == "shakycam") {
		// @ATTR event.shakycam|duration|Makes the camera shake for this duration in 'ms' or 's'.
		e->type = EventComponent::SHAKYCAM;

		e->x = Parse::toDuration(val);
	}
	else if (key == "requires_status") {
		// @ATTR event.requires_status|list(string)|Event requires list of statuses
		e->type = EventComponent::REQUIRES_STATUS;

		e->status = camp->registerStatus(Parse::popFirstString(val));

		// add repeating requires_status
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::REQUIRES_STATUS;
				e->status = camp->registerStatus(repeat_val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "requires_not_status") {
		// @ATTR event.requires_not_status|list(string)|Event requires not list of statuses
		e->type = EventComponent::REQUIRES_NOT_STATUS;

		e->status = camp->registerStatus(Parse::popFirstString(val));

		// add repeating requires_not
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::REQUIRES_NOT_STATUS;
				e->status = camp->registerStatus(repeat_val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "requires_level") {
		// @ATTR event.requires_level|int|Event requires hero level
		e->type = EventComponent::REQUIRES_LEVEL;

		e->x = Parse::popFirstInt(val);
	}
	else if (key == "requires_not_level") {
		// @ATTR event.requires_not_level|int|Event requires not hero level
		e->type = EventComponent::REQUIRES_NOT_LEVEL;

		e->x = Parse::popFirstInt(val);
	}
	else if (key == "requires_currency") {
		// @ATTR event.requires_currency|int|Event requires atleast this much currency
		e->type = EventComponent::REQUIRES_CURRENCY;

		e->x = Parse::popFirstInt(val);
	}
	else if (key == "requires_not_currency") {
		// @ATTR event.requires_not_currency|int|Event requires no more than this much currency
		e->type = EventComponent::REQUIRES_NOT_CURRENCY;

		e->x = Parse::popFirstInt(val);
	}
	else if (key == "requires_item") {
		// @ATTR event.requires_item|list(item_id)|Event requires specific item (not equipped)
		e->type = EventComponent::REQUIRES_ITEM;

		e->x = Parse::popFirstInt(val);

		// add repeating requires_item
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::REQUIRES_ITEM;
				e->x = Parse::toInt(repeat_val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "requires_not_item") {
		// @ATTR event.requires_not_item|list(item_id)|Event requires not having a specific item (not equipped)
		e->type = EventComponent::REQUIRES_NOT_ITEM;

		e->x = Parse::popFirstInt(val);

		// add repeating requires_not_item
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::REQUIRES_NOT_ITEM;
				e->x = Parse::toInt(repeat_val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "requires_class") {
		// @ATTR event.requires_class|predefined_string|Event requires this base class
		e->type = EventComponent::REQUIRES_CLASS;

		e->s = Parse::popFirstString(val);
	}
	else if (key == "requires_not_class") {
		// @ATTR event.requires_not_class|predefined_string|Event requires not this base class
		e->type = EventComponent::REQUIRES_NOT_CLASS;

		e->s = Parse::popFirstString(val);
	}
	else if (key == "set_status") {
		// @ATTR event.set_status|list(string)|Sets specified statuses
		e->type = EventComponent::SET_STATUS;

		e->status = camp->registerStatus(Parse::popFirstString(val));

		// add repeating set_status
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::SET_STATUS;
				e->status = camp->registerStatus(repeat_val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "unset_status") {
		// @ATTR event.unset_status|list(string)|Unsets specified statuses
		e->type = EventComponent::UNSET_STATUS;

		e->status = camp->registerStatus(Parse::popFirstString(val));

		// add repeating unset_status
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::UNSET_STATUS;
				e->status = camp->registerStatus(repeat_val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "remove_currency") {
		// @ATTR event.remove_currency|int|Removes specified amount of currency from hero inventory
		e->type = EventComponent::REMOVE_CURRENCY;

		e->x = std::max(Parse::toInt(val), 0);
	}
	else if (key == "remove_item") {
		// @ATTR event.remove_item|list(item_id)|Removes specified item from hero inventory
		e->type = EventComponent::REMOVE_ITEM;

		e->x = Parse::popFirstInt(val);

		// add repeating remove_item
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::REMOVE_ITEM;
				e->x = Parse::toInt(repeat_val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "reward_xp") {
		// @ATTR event.reward_xp|int|Reward hero with specified amount of experience points.
		e->type = EventComponent::REWARD_XP;

		e->x = std::max(Parse::toInt(val), 0);
	}
	else if (key == "reward_currency") {
		// @ATTR event.reward_currency|int|Reward hero with specified amount of currency.
		e->type = EventComponent::REWARD_CURRENCY;

		e->x = std::max(Parse::toInt(val), 0);
	}
	else if (key == "reward_item") {
		// @ATTR event.reward_item|item_id, int : Item, Quantity|Reward hero with y number of item x.
		e->type = EventComponent::REWARD_ITEM;

		e->x = Parse::popFirstInt(val);
		e->y = std::max(Parse::popFirstInt(val), 1);
	}
	else if (key == "reward_loot") {
		// @ATTR event.reward_loot|list(loot)|Reward hero with random loot.
		e->type = EventComponent::REWARD_LOOT;

		e->s = val;
	}
	else if (key == "reward_loot_count") {
		// @ATTR event.reward_loot_count|int, int : Min, Max|Sets the minimum (and optionally, the maximum) amount of loot that reward_loot can give the hero. Defaults to 1.
		e->type = EventComponent::REWARD_LOOT_COUNT;

		e->x = std::max(Parse::popFirstInt(val), 1);
		e->y = std::max(Parse::popFirstInt(val), e->x);
	}
	else if (key == "restore") {
		// @ATTR event.restore|["hp", "mp", "hpmp", "status", "all"]|Restore the hero's HP, MP, and/or status.
		e->type = EventComponent::RESTORE;

		e->s = val;
	}
	else if (key == "power") {
		// @ATTR event.power|power_id|Specify power coupled with event.
		e->type = EventComponent::POWER;

		e->x = Parse::toInt(val);
	}
	else if (key == "spawn") {
		// @ATTR event.spawn|list(predefined_string, int, int) : Enemy category, X, Y|Spawn an enemy from this category at location
		e->type = EventComponent::SPAWN;

		e->s = Parse::popFirstString(val);
		e->x = Parse::popFirstInt(val);
		e->y = Parse::popFirstInt(val);

		// add repeating spawn
		if (evnt) {
			std::string repeat_val = Parse::popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(EventComponent());
				e = &evnt->components.back();
				e->type = EventComponent::SPAWN;

				e->s = repeat_val;
				e->x = Parse::popFirstInt(val);
				e->y = Parse::popFirstInt(val);

				repeat_val = Parse::popFirstString(val);
			}
		}
	}
	else if (key == "stash") {
		// @ATTR event.stash|bool|If true, the Stash menu if opened.
		e->type = EventComponent::STASH;

		e->x = static_cast<int>(Parse::toBool(val));
	}
	else if (key == "npc") {
		// @ATTR event.npc|filename|Filename of an NPC to start dialog with.
		e->type = EventComponent::NPC;

		e->s = val;
	}
	else if (key == "music") {
		// @ATTR event.music|filename|Change background music to specified file.
		e->type = EventComponent::MUSIC;

		e->s = val;
	}
	else if (key == "cutscene") {
		// @ATTR event.cutscene|filename|Show specified cutscene by filename.
		e->type = EventComponent::CUTSCENE;

		e->s = val;
	}
	else if (key == "repeat") {
		// @ATTR event.repeat|bool|If true, the event to be triggered again.
		e->type = EventComponent::REPEAT;

		e->x = static_cast<int>(Parse::toBool(val));
	}
	else if (key == "save_game") {
		// @ATTR event.save_game|bool|If true, the game is saved when the event is triggered. The respawn position is set to where the player is standing.
		e->type = EventComponent::SAVE_GAME;

		e->x = static_cast<int>(Parse::toBool(val));
	}
	else if (key == "book") {
		// @ATTR event.book|filename|Opens a book by filename.
		e->type = EventComponent::BOOK;

		e->s = val;
	}
	else if (key == "script") {
		// @ATTR event.script|filename|Loads and executes an Event from a file.
		e->type = EventComponent::SCRIPT;

		e->s = val;
	}
	else if (key == "chance_exec") {
		// @ATTR event.chance_exec|int|Percentage chance that this event will execute when triggered.
		e->type = EventComponent::CHANCE_EXEC;

		e->x = Parse::popFirstInt(val);
	}
	else if (key == "respec") {
		// @ATTR event.respec|["xp", "stats", "powers"], bool : Respec mode, Ignore class defaults|Resets various aspects of the character's progression. Resetting "xp" also resets "stats". Resetting "stats" also resets "powers".
		e->type = EventComponent::RESPEC;

		std::string mode = Parse::popFirstString(val);
		std::string use_engine_defaults = Parse::popFirstString(val);

		if (mode == "xp") {
			e->x = 3;
		}
		else if (mode == "stats") {
			e->x = 2;
		}
		else if (mode == "powers") {
			e->x = 1;
		}

		if (!use_engine_defaults.empty())
			e->y = static_cast<int>(Parse::toBool(use_engine_defaults));
	}
	else {
		return false;
	}

	return true;
}

bool EventManager::executeEvent(Event &e) {
	return executeEventInternal(e, !SKIP_DELAY);
}

bool EventManager::executeDelayedEvent(Event &e) {
	return executeEventInternal(e, SKIP_DELAY);
}

/**
 * A particular event has been triggered.
 * Process all of this events components.
 *
 * @param The triggered event
 * @param Delay ignore flag
 * @return Returns true if the event shall not be run again.
 */
bool EventManager::executeEventInternal(Event &ev, bool skip_delay) {
	// skip executing events that are on cooldown
	if (!ev.delay.isEnd() || !ev.cooldown.isEnd()) return false;

	// need to know this for early returns
	EventComponent *ec_repeat = ev.getComponent(EventComponent::REPEAT);
	if (ec_repeat) {
		ev.keep_after_trigger = ec_repeat->x == 0 ? false : true;
	}

	// Delay event execution
	// When an event is delayed, we create a copy and push it to mapr->delayed_events.
	// The original starts both the cooldown and delay timers.
	// The delay will finish, followed by the cooldown, which gives the correct timing for repeating events.
	// The copy only starts the delay timer. The cooldown is not needed because the copy never repeats.
	if (ev.delay.getDuration() > 0 && !skip_delay) {
		ev.delay.reset(Timer::BEGIN);
		mapr->delayed_events.push_back(ev);
		ev.cooldown.reset(Timer::BEGIN);

		return !ev.keep_after_trigger;
	}

	// set cooldown
	ev.cooldown.reset(Timer::BEGIN);

	// if chance_exec roll fails, don't execute the event
	// we respect the value of "repeat", even if the event doesn't execute
	EventComponent *ec_chance_exec = ev.getComponent(EventComponent::CHANCE_EXEC);
	if (ec_chance_exec && !Math::percentChance(ec_chance_exec->x)) {
		return !ev.keep_after_trigger;
	}

	EventComponent *ec;

	for (unsigned i = 0; i < ev.components.size(); ++i) {
		ec = &ev.components[i];

		if (ec->type == EventComponent::SET_STATUS) {
			camp->setStatus(ec->status);
		}
		else if (ec->type == EventComponent::UNSET_STATUS) {
			camp->unsetStatus(ec->status);
		}
		else if (ec->type == EventComponent::INTERMAP) {
			if (ec->z == 1) {
				// this is intermap_random
				std::string map_list = ec->s;
				EventComponent random_ec = getRandomMapFromFile(map_list);

				ec->s = random_ec.s;
				ec->x = random_ec.x;
				ec->y = random_ec.y;
			}

			if (Filesystem::fileExists(mods->locate(ec->s))) {
				mapr->teleportation = true;
				mapr->teleport_mapname = ec->s;

				if (ec->x == -1 && ec->y == -1) {
					// the teleport destination will be set to the map's hero_pos once the map is loaded
					mapr->teleport_destination.x = -1;
					mapr->teleport_destination.y = -1;
				}
				else {
					mapr->teleport_destination.x = static_cast<float>(ec->x) + 0.5f;
					mapr->teleport_destination.y = static_cast<float>(ec->y) + 0.5f;
				}
			}
			else {
				ev.keep_after_trigger = false;
				pc->logMsg(msg->get("Unknown destination"), Avatar::MSG_UNIQUE);
			}
		}
		else if (ec->type == EventComponent::INTRAMAP) {
			mapr->teleportation = true;
			mapr->teleport_mapname = "";
			mapr->teleport_destination.x = static_cast<float>(ec->x) + 0.5f;
			mapr->teleport_destination.y = static_cast<float>(ec->y) + 0.5f;
		}
		else if (ec->type == EventComponent::MAPMOD) {
			if (ec->s == "collision") {
				if (ec->x >= 0 && ec->x < mapr->w && ec->y >= 0 && ec->y < mapr->h) {
					mapr->collider.colmap[ec->x][ec->y] = static_cast<unsigned short>(ec->z);
					mapr->map_change = true;
				}
				else
					Utils::logError("EventManager: Mapmod at position (%d, %d) is out of bounds 0-255.", ec->x, ec->y);
			}
			else {
				size_t index = static_cast<size_t>(distance(mapr->layernames.begin(), find(mapr->layernames.begin(), mapr->layernames.end(), ec->s)));
				if (!mapr->isValidTile(ec->z))
					Utils::logError("EventManager: Mapmod at position (%d, %d) contains invalid tile id (%d).", ec->x, ec->y, ec->z);
				else if (index >= mapr->layers.size())
					Utils::logError("EventManager: Mapmod at position (%d, %d) is on an invalid layer.", ec->x, ec->y);
				else if (ec->x >= 0 && ec->x < mapr->w && ec->y >= 0 && ec->y < mapr->h)
					mapr->layers[index][ec->x][ec->y] = static_cast<unsigned short>(ec->z);
				else
					Utils::logError("EventManager: Mapmod at position (%d, %d) is out of bounds 0-255.", ec->x, ec->y);
			}
		}
		else if (ec->type == EventComponent::SOUNDFX) {
			FPoint pos(0,0);
			bool loop = false;

			if (ec->x != -1 && ec->y != -1) {
				if (ec->x != 0 && ec->y != 0) {
					pos.x = static_cast<float>(ec->x) + 0.5f;
					pos.y = static_cast<float>(ec->y) + 0.5f;
				}
			}
			else if (ev.location.x != 0 && ev.location.y != 0) {
				pos.x = static_cast<float>(ev.location.x) + 0.5f;
				pos.y = static_cast<float>(ev.location.y) + 0.5f;
			}

			if (ev.activate_type == Event::ACTIVATE_ON_LOAD || ec->z != 0)
				loop = true;

			SoundID sid = snd->load(ec->s, "MapRenderer background soundfx");

			snd->play(sid, snd->DEFAULT_CHANNEL, pos, loop);
			mapr->sids.push_back(sid);
		}
		else if (ec->type == EventComponent::LOOT) {
			EventComponent *ec_lootcount = ev.getComponent(EventComponent::LOOT_COUNT);
			if (ec_lootcount) {
				mapr->loot_count.x = ec_lootcount->x;
				mapr->loot_count.y = ec_lootcount->y;
			}
			else {
				mapr->loot_count.x = 0;
				mapr->loot_count.y = 0;
			}

			ec->x = ev.hotspot.x;
			ec->y = ev.hotspot.y;
			mapr->loot.push_back(*ec);
		}
		else if (ec->type == EventComponent::MSG) {
			pc->logMsg(ec->s, Avatar::MSG_UNIQUE);
		}
		else if (ec->type == EventComponent::SHAKYCAM) {
			mapr->shaky_cam_timer.setDuration(ec->x);
		}
		else if (ec->type == EventComponent::REMOVE_CURRENCY) {
			camp->removeCurrency(ec->x);
		}
		else if (ec->type == EventComponent::REMOVE_ITEM) {
			camp->removeItem(ec->x);
		}
		else if (ec->type == EventComponent::REWARD_XP) {
			camp->rewardXP(ec->x, CampaignManager::XP_SHOW_MSG);
		}
		else if (ec->type == EventComponent::REWARD_CURRENCY) {
			camp->rewardCurrency(ec->x);
		}
		else if (ec->type == EventComponent::REWARD_ITEM) {
			ItemStack istack;
			istack.item = ec->x;
			istack.quantity = ec->y;
			camp->rewardItem(istack);
		}
		else if (ec->type == EventComponent::REWARD_LOOT) {
			std::vector<EventComponent> random_table;
			Point random_table_count(1,1);

			EventComponent *ec_lootcount = ev.getComponent(EventComponent::REWARD_LOOT_COUNT);
			if (ec_lootcount) {
				random_table_count.x = ec_lootcount->x;
				random_table_count.y = ec_lootcount->y;
			}

			random_table.push_back(EventComponent());
			loot->parseLoot(ec->s, &random_table.back(), &random_table);

			unsigned rand_count = Math::randBetween(random_table_count.x, random_table_count.y);
			std::vector<ItemStack> rand_itemstacks;
			for (unsigned j = 0; j < rand_count; ++j) {
				loot->checkLoot(random_table, NULL, &rand_itemstacks);
			}
			for (size_t j = 0; j < rand_itemstacks.size(); ++j) {
				if (rand_itemstacks[j].item == eset->misc.currency_id)
					camp->rewardCurrency(rand_itemstacks[j].quantity);
				else
					camp->rewardItem(rand_itemstacks[j]);
			}
		}
		else if (ec->type == EventComponent::RESTORE) {
			camp->restoreHPMP(ec->s);
		}
		else if (ec->type == EventComponent::SPAWN) {
			Point spawn_pos;
			spawn_pos.x = ec->x;
			spawn_pos.y = ec->y;
			enemym->spawn(ec->s, spawn_pos);
		}
		else if (ec->type == EventComponent::POWER) {
			EventComponent *ec_path = ev.getComponent(EventComponent::POWER_PATH);
			FPoint target;

			if (ec_path) {
				// targets hero option
				if (ec_path->s == "hero") {
					target.x = pc->stats.pos.x;
					target.y = pc->stats.pos.y;
				}
				// targets fixed path option
				else {
					target.x = static_cast<float>(ec_path->a) + 0.5f;
					target.y = static_cast<float>(ec_path->b) + 0.5f;
				}
			}
			// no path specified, targets self location
			else {
				target.x = static_cast<float>(ev.location.x) + 0.5f;
				target.y = static_cast<float>(ev.location.y) + 0.5f;
			}

			// ec->x is power id
			// ec->y is statblock index
			mapr->activatePower(ec->x, ec->y, target);
		}
		else if (ec->type == EventComponent::STASH) {
			mapr->stash = ec->x == 0 ? false : true;
			if (mapr->stash) {
				mapr->stash_pos.x = static_cast<float>(ev.location.x) + 0.5f;
				mapr->stash_pos.y = static_cast<float>(ev.location.y) + 0.5f;
			}
		}
		else if (ec->type == EventComponent::NPC) {
			mapr->event_npc = ec->s;
		}
		else if (ec->type == EventComponent::MUSIC) {
			mapr->music_filename = ec->s;
			mapr->loadMusic();
		}
		else if (ec->type == EventComponent::CUTSCENE) {
			mapr->cutscene = true;
			mapr->cutscene_file = ec->s;
		}
		else if (ec->type == EventComponent::REPEAT) {
			ev.keep_after_trigger = ec->x == 0 ? false : true;
		}
		else if (ec->type == EventComponent::SAVE_GAME) {
			mapr->save_game = ec->x == 0 ? false : true;
		}
		else if (ec->type == EventComponent::NPC_ID) {
			mapr->npc_id = ec->x;
		}
		else if (ec->type == EventComponent::BOOK) {
			mapr->show_book = ec->s;
		}
		else if (ec->type == EventComponent::SCRIPT) {
			if (ev.center.x != -1 && ev.center.y != -1)
				executeScript(ec->s, ev.center.x, ev.center.y);
			else
				executeScript(ec->s, pc->stats.pos.x, pc->stats.pos.y);
		}
		else if (ec->type == EventComponent::RESPEC) {
			bool use_engine_defaults = static_cast<bool>(ec->y);
			EngineSettings::HeroClasses::HeroClass* pc_class;
			pc_class = eset->hero_classes.getByName(pc->stats.character_class);

			if (ec->x == 3) {
				// xp
				pc->stats.level = 1;
				pc->stats.xp = 0;
			}
			if (ec->x >= 2) {
				// stats
				for (size_t j = 0; j < eset->primary_stats.list.size(); ++j) {
					pc->stats.primary[j] = 1;
					pc->stats.primary_additional[j] = 0;

					if (pc_class && !use_engine_defaults) {
						pc->stats.primary[j] += pc_class->primary[j];
						pc->stats.primary_starting[j] = pc->stats.primary[j];
					}
				}

				pc->stats.recalc();
				menu->inv->applyEquipment();
				pc->stats.logic();
			}
			if (ec->x >= 1) {
				// powers
				pc->stats.powers_list.clear();
				pc->stats.powers_passive.clear();
				pc->stats.effects.clearEffects();
				menu_powers->resetToBasePowers();
				if (pc_class && !use_engine_defaults) {
					for (size_t j=0; j < pc_class->powers.size(); j++) {
						pc->stats.powers_list.push_back(pc_class->powers[j]);
					}
				}
				menu_powers->setUnlockedPowers();

				menu_act->clear();
				if (pc_class && !use_engine_defaults) {
					menu->act->set(pc_class->hotkeys);
				}
				menu->pow->newPowerNotification = false;

				pc->respawn = true; // re-applies equipment, also revives the player
				pc->stats.refresh_stats = true;
			}
		}
	}
	return !ev.keep_after_trigger;
}


bool EventManager::isActive(const Event &e) {
	for (size_t i=0; i < e.components.size(); i++) {
		if (!camp->checkAllRequirements(e.components[i]))
			return false;
	}
	return true;
}

void EventManager::executeScript(const std::string& filename, float x, float y) {
	FileParser script_file;
	std::queue<Event> script_evnt;

	if (script_file.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (script_file.next()) {
			if (script_file.new_section && script_file.section == "event") {
				Event tmp_evnt;
				tmp_evnt.location.x = tmp_evnt.hotspot.x = static_cast<int>(x);
				tmp_evnt.location.y = tmp_evnt.hotspot.y = static_cast<int>(y);
				tmp_evnt.location.w = tmp_evnt.hotspot.w = 1;
				tmp_evnt.location.h = tmp_evnt.hotspot.h = 1;
				tmp_evnt.center.x = static_cast<float>(tmp_evnt.location.x) + 0.5f;
				tmp_evnt.center.y = static_cast<float>(tmp_evnt.location.y) + 0.5f;

				script_evnt.push(tmp_evnt);
			}

			if (script_evnt.empty())
				continue;

			if (script_file.key == "script" && script_file.val == filename) {
				script_file.error("EventManager: Calling a script from within itself is not allowed.");
				continue;
			}

			loadEventComponent(script_file, &script_evnt.back(), NULL);
		}
		script_file.close();

		while (!script_evnt.empty()) {
			// create StatBlocks if we need them
			EventComponent *ec_power = script_evnt.front().getComponent(EventComponent::POWER);
			if (ec_power) {
				ec_power->y = mapr->addEventStatBlock(script_evnt.front());
			}

			if (isActive(script_evnt.front())) {
				executeEvent(script_evnt.front());
			}
			script_evnt.pop();
		}
	}
}

EventComponent EventManager::getRandomMapFromFile(const std::string& fname) {
	// map pool is the same, so pick the next one in the "playlist"
	if (fname == mapr->intermap_random_filename && !mapr->intermap_random_queue.empty()) {
		EventComponent ec = mapr->intermap_random_queue.front();
		mapr->intermap_random_queue.pop();
		return ec;
	}

	// starting a new map pool, so clear the queue
	while (!mapr->intermap_random_queue.empty()) {
		mapr->intermap_random_queue.pop();
	}

	FileParser infile;
	std::vector<EventComponent> ec_list;

	// @CLASS EventManager: Random Map List|Description of maps/random/lists/
	if (infile.open(fname, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR map|filename, int, int : Map file, X, Y|Adds a map and optional spawn position to the random list of maps to teleport to.
			if (infile.key == "map") {
				EventComponent ec;
				ec.s = Parse::popFirstString(infile.val);
				if (ec_list.empty() || ec.s != mapr->getFilename()) {
					ec.x = -1;
					ec.y = -1;

					std::string test_x = Parse::popFirstString(infile.val);
					if (!test_x.empty()) {
						ec.x = Parse::toInt(test_x);
						ec.y = Parse::popFirstInt(infile.val);
					}

					ec_list.push_back(ec);
				}
			}
		}

		infile.close();
	}

	if (ec_list.empty()) {
		mapr->intermap_random_filename = "";
		return EventComponent();
	}
	else {
		mapr->intermap_random_filename = fname;

		while (!ec_list.empty()) {
			size_t index = rand() % ec_list.size();
			mapr->intermap_random_queue.push(ec_list[index]);
			ec_list.erase(ec_list.begin() + index);
		}

		EventComponent ec = mapr->intermap_random_queue.front();
		mapr->intermap_random_queue.pop();
		return ec;
	}
}
