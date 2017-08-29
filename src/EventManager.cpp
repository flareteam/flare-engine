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

#include "EnemyManager.h"
#include "EventManager.h"
#include "FileParser.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"

/**
 * Class: Event
 */
Event::Event()
	: type("")
	, activate_type(-1)
	, components(std::vector<Event_Component>())
	, location(Rect())
	, hotspot(Rect())
	, cooldown(0)
	, cooldown_ticks(0)
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
Event_Component* Event::getComponent(const EVENT_COMPONENT_TYPE &_type) {
	std::vector<Event_Component>::iterator it;
	for (it = components.begin(); it != components.end(); ++it)
		if (it->type == _type)
			return &(*it);
	return NULL;
}

void Event::deleteAllComponents(const EVENT_COMPONENT_TYPE &_type) {
	std::vector<Event_Component>::iterator it;
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
			evnt->activate_type = EVENT_ON_TRIGGER;
		}
		else if (infile.val == "on_mapexit") {
			// no need to set keep_after_trigger to false correctly, it's ignored anyway
			evnt->activate_type = EVENT_ON_MAPEXIT;
		}
		else if (infile.val == "on_leave") {
			evnt->activate_type = EVENT_ON_LEAVE;
		}
		else if (infile.val == "on_load") {
			evnt->activate_type = EVENT_ON_LOAD;
			evnt->keep_after_trigger = false;
		}
		else if (infile.val == "on_clear") {
			evnt->activate_type = EVENT_ON_CLEAR;
			evnt->keep_after_trigger = false;
		}
		else if (infile.val == "static") {
			evnt->activate_type = EVENT_STATIC;
		}
		else {
			infile.error("EventManager: Event activation type '%s' unknown, change to \"on_trigger\" to suppress this warning.", infile.val.c_str());
		}
	}
	else if (infile.key == "location") {
		// @ATTR event.location|rectangle|Defines the location area for the event.
		evnt->location.x = popFirstInt(infile.val);
		evnt->location.y = popFirstInt(infile.val);
		evnt->location.w = popFirstInt(infile.val);
		evnt->location.h = popFirstInt(infile.val);

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
			evnt->hotspot.x = popFirstInt(infile.val);
			evnt->hotspot.y = popFirstInt(infile.val);
			evnt->hotspot.w = popFirstInt(infile.val);
			evnt->hotspot.h = popFirstInt(infile.val);
		}

		evnt->center.x = static_cast<float>(evnt->hotspot.x) + static_cast<float>(evnt->hotspot.w)/2;
		evnt->center.y = static_cast<float>(evnt->hotspot.y) + static_cast<float>(evnt->hotspot.h)/2;
	}
	else if (infile.key == "cooldown") {
		// @ATTR event.cooldown|duration|Duration for event cooldown in 'ms' or 's'.
		evnt->cooldown = parse_duration(infile.val);
	}
	else if (infile.key == "reachable_from") {
		// @ATTR event.reachable_from|rectangle|If the hero is inside this rectangle, they can activate the event.
		evnt->reachable_from.x = popFirstInt(infile.val);
		evnt->reachable_from.y = popFirstInt(infile.val);
		evnt->reachable_from.w = popFirstInt(infile.val);
		evnt->reachable_from.h = popFirstInt(infile.val);
	}
	else {
		loadEventComponent(infile, evnt, NULL);
	}
}

void EventManager::loadEventComponent(FileParser &infile, Event* evnt, Event_Component* ec) {
	if (!loadEventComponentString(infile.key, infile.val, evnt, ec)) {
		infile.error("EventManager: '%s' is not a valid key.", infile.key.c_str());
	}
}

bool EventManager::loadEventComponentString(std::string &key, std::string &val, Event* evnt, Event_Component* ec) {
	Event_Component *e = NULL;
	if (evnt) {
		evnt->components.push_back(Event_Component());
		e = &evnt->components.back();
	}
	else if (ec) {
		e = ec;
	}

	if (!e) return true;

	e->type = EC_NONE;

	if (key == "tooltip") {
		// @ATTR event.tooltip|string|Tooltip for event
		e->type = EC_TOOLTIP;

		e->s = msg->get(val);
	}
	else if (key == "power_path") {
		// @ATTR event.power_path|["hero", point]|Event power path
		e->type = EC_POWER_PATH;

		// x,y are src, if s=="hero" we target the hero,
		// else we'll use values in a,b as coordinates
		e->x = popFirstInt(val);
		e->y = popFirstInt(val);

		std::string dest = popFirstString(val);
		if (dest == "hero") {
			e->s = "hero";
		}
		else {
			e->a = toInt(dest);
			e->b = popFirstInt(val);
		}
	}
	else if (key == "power_damage") {
		// @ATTR event.power_damage|int, int : Min, Max|Range of power damage
		e->type = EC_POWER_DAMAGE;

		e->a = popFirstInt(val);
		e->b = popFirstInt(val);
	}
	// TODO should intermap and intramap be combined?
	else if (key == "intermap") {
		// @ATTR event.intermap|filename, int, int : Map file, X, Y|Jump to specific map at location specified.
		e->type = EC_INTERMAP;

		e->s = popFirstString(val);
		e->x = -1;
		e->y = -1;

		std::string test_x = popFirstString(val);
		if (!test_x.empty()) {
			e->x = toInt(test_x);
			e->y = popFirstInt(val);
		}
	}
	else if (key == "intermap_random") {
		// @ATTR event.intermap_random|filename|Pick a random map from a map list file and teleport to it.
		Event_Component random_ec = getRandomMapFromFile(popFirstString(val));

		e->type = EC_INTERMAP;
		e->s = random_ec.s;
		e->x = random_ec.x;
		e->y = random_ec.y;
	}
	else if (key == "intramap") {
		// @ATTR event.intramap|int, int : X, Y|Jump to specific position within current map.
		e->type = EC_INTRAMAP;

		e->x = popFirstInt(val);
		e->y = popFirstInt(val);
	}
	else if (key == "mapmod") {
		// @ATTR event.mapmod|list(predefined_string, int, int, int) : Layer, X, Y, Tile ID|Modify map tiles
		e->type = EC_MAPMOD;

		e->s = popFirstString(val);
		e->x = popFirstInt(val);
		e->y = popFirstInt(val);
		e->z = popFirstInt(val);

		// add repeating mapmods
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_MAPMOD;
				e->s = repeat_val;
				e->x = popFirstInt(val);
				e->y = popFirstInt(val);
				e->z = popFirstInt(val);

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "soundfx") {
		// @ATTR event.soundfx|filename, int, int, bool : Sound file, X, Y, loop|Filename of a sound to play. Optionally, it can be played at a specific location and/or looped.
		e->type = EC_SOUNDFX;

		e->s = popFirstString(val);
		e->x = e->y = -1;
		e->z = static_cast<int>(false);

		std::string s = popFirstString(val);
		if (s != "") e->x = toInt(s);

		s = popFirstString(val);
		if (s != "") e->y = toInt(s);

		s = popFirstString(val);
		if (s != "") e->z = static_cast<int>(toBool(s));
	}
	else if (key == "loot") {
		// @ATTR event.loot|list(loot)|Add loot to the event.
		e->type = EC_LOOT;

		loot->parseLoot(val, e, &evnt->components);
	}
	else if (key == "loot_count") {
		// @ATTR event.loot_count|int, int : Min, Max|Sets the minimum (and optionally, the maximum) amount of loot this event can drop. Overrides the global drop_max setting.
		e->type = EC_LOOT_COUNT;

		e->x = popFirstInt(val);
		e->y = popFirstInt(val);
		if (e->x != 0 || e->y != 0) {
			e->x = std::max(e->x, 1);
			e->y = std::max(e->y, e->x);
		}
	}
	else if (key == "msg") {
		// @ATTR event.msg|string|Adds a message to be displayed for the event.
		e->type = EC_MSG;

		e->s = msg->get(val);
	}
	else if (key == "shakycam") {
		// @ATTR event.shakycam|duration|Makes the camera shake for this duration in 'ms' or 's'.
		e->type = EC_SHAKYCAM;

		e->x = parse_duration(val);
	}
	else if (key == "requires_status") {
		// @ATTR event.requires_status|list(string)|Event requires list of statuses
		e->type = EC_REQUIRES_STATUS;

		e->s = popFirstString(val);

		// add repeating requires_status
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_REQUIRES_STATUS;
				e->s = repeat_val;

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "requires_not_status") {
		// @ATTR event.requires_not_status|list(string)|Event requires not list of statuses
		e->type = EC_REQUIRES_NOT_STATUS;

		e->s = popFirstString(val);

		// add repeating requires_not
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_REQUIRES_NOT_STATUS;
				e->s = repeat_val;

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "requires_level") {
		// @ATTR event.requires_level|int|Event requires hero level
		e->type = EC_REQUIRES_LEVEL;

		e->x = popFirstInt(val);
	}
	else if (key == "requires_not_level") {
		// @ATTR event.requires_not_level|int|Event requires not hero level
		e->type = EC_REQUIRES_NOT_LEVEL;

		e->x = popFirstInt(val);
	}
	else if (key == "requires_currency") {
		// @ATTR event.requires_currency|int|Event requires atleast this much currency
		e->type = EC_REQUIRES_CURRENCY;

		e->x = popFirstInt(val);
	}
	else if (key == "requires_not_currency") {
		// @ATTR event.requires_not_currency|int|Event requires no more than this much currency
		e->type = EC_REQUIRES_NOT_CURRENCY;

		e->x = popFirstInt(val);
	}
	else if (key == "requires_item") {
		// @ATTR event.requires_item|list(item_id)|Event requires specific item (not equipped)
		e->type = EC_REQUIRES_ITEM;

		e->x = popFirstInt(val);

		// add repeating requires_item
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_REQUIRES_ITEM;
				e->x = toInt(repeat_val);

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "requires_not_item") {
		// @ATTR event.requires_not_item|list(item_id)|Event requires not having a specific item (not equipped)
		e->type = EC_REQUIRES_NOT_ITEM;

		e->x = popFirstInt(val);

		// add repeating requires_not_item
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_REQUIRES_NOT_ITEM;
				e->x = toInt(repeat_val);

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "requires_class") {
		// @ATTR event.requires_class|predefined_string|Event requires this base class
		e->type = EC_REQUIRES_CLASS;

		e->s = popFirstString(val);
	}
	else if (key == "requires_not_class") {
		// @ATTR event.requires_not_class|predefined_string|Event requires not this base class
		e->type = EC_REQUIRES_NOT_CLASS;

		e->s = popFirstString(val);
	}
	else if (key == "set_status") {
		// @ATTR event.set_status|list(string)|Sets specified statuses
		e->type = EC_SET_STATUS;

		e->s = popFirstString(val);

		// add repeating set_status
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_SET_STATUS;
				e->s = repeat_val;

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "unset_status") {
		// @ATTR event.unset_status|list(string)|Unsets specified statuses
		e->type = EC_UNSET_STATUS;

		e->s = popFirstString(val);

		// add repeating unset_status
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_UNSET_STATUS;
				e->s = repeat_val;

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "remove_currency") {
		// @ATTR event.remove_currency|int|Removes specified amount of currency from hero inventory
		e->type = EC_REMOVE_CURRENCY;

		e->x = std::max(toInt(val), 0);
	}
	else if (key == "remove_item") {
		// @ATTR event.remove_item|list(item_id)|Removes specified item from hero inventory
		e->type = EC_REMOVE_ITEM;

		e->x = popFirstInt(val);

		// add repeating remove_item
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_REMOVE_ITEM;
				e->x = toInt(repeat_val);

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "reward_xp") {
		// @ATTR event.reward_xp|int|Reward hero with specified amount of experience points.
		e->type = EC_REWARD_XP;

		e->x = std::max(toInt(val), 0);
	}
	else if (key == "reward_currency") {
		// @ATTR event.reward_currency|int|Reward hero with specified amount of currency.
		e->type = EC_REWARD_CURRENCY;

		e->x = std::max(toInt(val), 0);
	}
	else if (key == "reward_item") {
		// @ATTR event.reward_item|item_id, int : Item, Quantity|Reward hero with y number of item x.
		e->type = EC_REWARD_ITEM;

		e->x = popFirstInt(val);
		e->y = std::max(popFirstInt(val), 1);
	}
	else if (key == "restore") {
		// @ATTR event.restore|["hp", "mp", "hpmp", "status", "all"]|Restore the hero's HP, MP, and/or status.
		e->type = EC_RESTORE;

		e->s = val;
	}
	else if (key == "power") {
		// @ATTR event.power|power_id|Specify power coupled with event.
		e->type = EC_POWER;

		e->x = toInt(val);
	}
	else if (key == "spawn") {
		// @ATTR event.spawn|list(predefined_string, int, int) : Enemy category, X, Y|Spawn an enemy from this category at location
		e->type = EC_SPAWN;

		e->s = popFirstString(val);
		e->x = popFirstInt(val);
		e->y = popFirstInt(val);

		// add repeating spawn
		if (evnt) {
			std::string repeat_val = popFirstString(val);
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = EC_SPAWN;

				e->s = repeat_val;
				e->x = popFirstInt(val);
				e->y = popFirstInt(val);

				repeat_val = popFirstString(val);
			}
		}
	}
	else if (key == "stash") {
		// @ATTR event.stash|bool|If true, the Stash menu if opened.
		e->type = EC_STASH;

		e->x = static_cast<int>(toBool(val));
	}
	else if (key == "npc") {
		// @ATTR event.npc|filename|Filename of an NPC to start dialog with.
		e->type = EC_NPC;

		e->s = val;
	}
	else if (key == "music") {
		// @ATTR event.music|filename|Change background music to specified file.
		e->type = EC_MUSIC;

		e->s = val;
	}
	else if (key == "cutscene") {
		// @ATTR event.cutscene|filename|Show specified cutscene by filename.
		e->type = EC_CUTSCENE;

		e->s = val;
	}
	else if (key == "repeat") {
		// @ATTR event.repeat|bool|If true, the event to be triggered again.
		e->type = EC_REPEAT;

		e->x = static_cast<int>(toBool(val));
	}
	else if (key == "save_game") {
		// @ATTR event.save_game|bool|If true, the game is saved when the event is triggered. The respawn position is set to where the player is standing.
		e->type = EC_SAVE_GAME;

		e->x = static_cast<int>(toBool(val));
	}
	else if (key == "book") {
		// @ATTR event.book|filename|Opens a book by filename.
		e->type = EC_BOOK;

		e->s = val;
	}
	else if (key == "script") {
		// @ATTR event.script|filename|Loads and executes an Event from a file.
		e->type = EC_SCRIPT;

		e->s = val;
	}
	else if (key == "chance_exec") {
		// @ATTR event.chance_exec|int|Percentage chance that this event will execute when triggered.
		e->type = EC_CHANCE_EXEC;

		e->x = popFirstInt(val);
	}
	else {
		return false;
	}

	return true;
}

/**
 * A particular event has been triggered.
 * Process all of this events components.
 *
 * @param The triggered event
 * @return Returns true if the event shall not be run again.
 */
bool EventManager::executeEvent(Event &ev) {
	// skip executing events that are on cooldown
	if (ev.cooldown_ticks > 0) return false;

	// set cooldown
	ev.cooldown_ticks = ev.cooldown;

	// if chance_exec roll fails, don't execute the event
	// we respect the value of "repeat", even if the event doesn't execute
	Event_Component *ec_chance_exec = ev.getComponent(EC_CHANCE_EXEC);
	if (ec_chance_exec && !percentChance(ec_chance_exec->x)) {
		Event_Component *ec_repeat = ev.getComponent(EC_REPEAT);
		if (ec_repeat) {
			ev.keep_after_trigger = ec_repeat->x == 0 ? false : true;
		}
		return !ev.keep_after_trigger;
	}

	Event_Component *ec;

	for (unsigned i = 0; i < ev.components.size(); ++i) {
		ec = &ev.components[i];

		if (ec->type == EC_SET_STATUS) {
			camp->setStatus(ec->s);
		}
		else if (ec->type == EC_UNSET_STATUS) {
			camp->unsetStatus(ec->s);
		}
		else if (ec->type == EC_INTERMAP) {

			if (fileExists(mods->locate(ec->s))) {
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
				pc->logMsg(msg->get("Unknown destination"), false);
			}
		}
		else if (ec->type == EC_INTRAMAP) {
			mapr->teleportation = true;
			mapr->teleport_mapname = "";
			mapr->teleport_destination.x = static_cast<float>(ec->x) + 0.5f;
			mapr->teleport_destination.y = static_cast<float>(ec->y) + 0.5f;
		}
		else if (ec->type == EC_MAPMOD) {
			if (ec->s == "collision") {
				if (ec->x >= 0 && ec->x < mapr->w && ec->y >= 0 && ec->y < mapr->h) {
					mapr->collider.colmap[ec->x][ec->y] = static_cast<unsigned short>(ec->z);
					mapr->map_change = true;
				}
				else
					logError("EventManager: Mapmod at position (%d, %d) is out of bounds 0-255.", ec->x, ec->y);
			}
			else {
				size_t index = static_cast<size_t>(distance(mapr->layernames.begin(), find(mapr->layernames.begin(), mapr->layernames.end(), ec->s)));
				if (!mapr->isValidTile(ec->z))
					logError("EventManager: Mapmod at position (%d, %d) contains invalid tile id (%d).", ec->x, ec->y, ec->z);
				else if (index >= mapr->layers.size())
					logError("EventManager: Mapmod at position (%d, %d) is on an invalid layer.", ec->x, ec->y);
				else if (ec->x >= 0 && ec->x < mapr->w && ec->y >= 0 && ec->y < mapr->h)
					mapr->layers[index][ec->x][ec->y] = static_cast<unsigned short>(ec->z);
				else
					logError("EventManager: Mapmod at position (%d, %d) is out of bounds 0-255.", ec->x, ec->y);
			}
		}
		else if (ec->type == EC_SOUNDFX) {
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

			if (ev.activate_type == EVENT_ON_LOAD || ec->z != 0)
				loop = true;

			SoundManager::SoundID sid = snd->load(ec->s, "MapRenderer background soundfx");

			snd->play(sid, GLOBAL_VIRTUAL_CHANNEL, pos, loop);
			mapr->sids.push_back(sid);
		}
		else if (ec->type == EC_LOOT) {
			Event_Component *ec_lootcount = ev.getComponent(EC_LOOT_COUNT);
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
		else if (ec->type == EC_MSG) {
			pc->logMsg(ec->s, false);
		}
		else if (ec->type == EC_SHAKYCAM) {
			mapr->shaky_cam_ticks = ec->x;
		}
		else if (ec->type == EC_REMOVE_CURRENCY) {
			camp->removeCurrency(ec->x);
		}
		else if (ec->type == EC_REMOVE_ITEM) {
			camp->removeItem(ec->x);
		}
		else if (ec->type == EC_REWARD_XP) {
			camp->rewardXP(ec->x, true);
		}
		else if (ec->type == EC_REWARD_CURRENCY) {
			camp->rewardCurrency(ec->x);
		}
		else if (ec->type == EC_REWARD_ITEM) {
			ItemStack istack;
			istack.item = ec->x;
			istack.quantity = ec->y;
			camp->rewardItem(istack);
		}
		else if (ec->type == EC_RESTORE) {
			camp->restoreHPMP(ec->s);
		}
		else if (ec->type == EC_SPAWN) {
			Point spawn_pos;
			spawn_pos.x = ec->x;
			spawn_pos.y = ec->y;
			enemies->spawn(ec->s, spawn_pos);
		}
		else if (ec->type == EC_POWER) {
			Event_Component *ec_path = ev.getComponent(EC_POWER_PATH);
			FPoint target;

			if (ec_path) {
				// targets hero option
				if (ec_path->s == "hero") {
					target.x = mapr->cam.x;
					target.y = mapr->cam.y;
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
		else if (ec->type == EC_STASH) {
			mapr->stash = ec->x == 0 ? false : true;
			if (mapr->stash) {
				mapr->stash_pos.x = static_cast<float>(ev.location.x) + 0.5f;
				mapr->stash_pos.y = static_cast<float>(ev.location.y) + 0.5f;
			}
		}
		else if (ec->type == EC_NPC) {
			mapr->event_npc = ec->s;
		}
		else if (ec->type == EC_MUSIC) {
			mapr->music_filename = ec->s;
			mapr->loadMusic();
		}
		else if (ec->type == EC_CUTSCENE) {
			mapr->cutscene = true;
			mapr->cutscene_file = ec->s;
		}
		else if (ec->type == EC_REPEAT) {
			ev.keep_after_trigger = ec->x == 0 ? false : true;
		}
		else if (ec->type == EC_SAVE_GAME) {
			mapr->save_game = ec->x == 0 ? false : true;
		}
		else if (ec->type == EC_NPC_ID) {
			mapr->npc_id = ec->x;
		}
		else if (ec->type == EC_BOOK) {
			mapr->show_book = ec->s;
		}
		else if (ec->type == EC_SCRIPT) {
			if (ev.center.x != -1 && ev.center.y != -1)
				executeScript(ec->s, ev.center.x, ev.center.y);
			else
				executeScript(ec->s, pc->stats.pos.x, pc->stats.pos.y);
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

	if (script_file.open(filename)) {
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
			Event_Component *ec_power = script_evnt.front().getComponent(EC_POWER);
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

Event_Component EventManager::getRandomMapFromFile(const std::string& fname) {
	FileParser infile;
	std::vector<Event_Component> ec_list;

	// @CLASS EventManager: Random Map List|Description of maps/random/lists/
	if (infile.open(fname)) {
		while (infile.next()) {
			// @ATTR map|filename, int, int : Map file, X, Y|Adds a map and optional spawn position to the random list of maps to teleport to.
			if (infile.key == "map") {
				Event_Component ec;
				ec.s = popFirstString(infile.val);
				if (ec.s != mapr->getFilename()) {
					ec.x = -1;
					ec.y = -1;

					std::string test_x = popFirstString(infile.val);
					if (!test_x.empty()) {
						ec.x = toInt(test_x);
						ec.y = popFirstInt(infile.val);
					}

					ec_list.push_back(ec);
				}
			}
		}

		infile.close();
	}

	if (ec_list.empty())
		return Event_Component();
	else
		return ec_list[rand() % ec_list.size()];
}
