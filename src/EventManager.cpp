/*
Copyright © 2013 Igor Paliychuk

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

#include "EventManager.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"

using namespace std;

EventManager::EventManager() {

}

EventManager::~EventManager() {

}

void EventManager::loadEvent(FileParser &infile, Event* evnt) {
	if (!evnt) return;
	// @CLASS EventManager|Description of events in maps/ and npcs/

	if (infile.key == "type") {
		// @ATTR event.type|[on_trigger:on_mapexit:on_leave:on_load:on_clear]|Type of map event.
		std::string type = infile.val;
		evnt->type = type;

		if      (type == "on_trigger");
		else if (type == "on_mapexit"); // no need to set keep_after_trigger to false correctly, it's ignored anyway
		else if (type == "on_leave");
		else if (type == "on_load") {
			evnt->keep_after_trigger = false;
		}
		else if (type == "on_clear") {
			evnt->keep_after_trigger = false;
		}
		else {
			infile.error("EventManager: Event type '%s' unknown, change to \"on_trigger\" to suppress this warning.", type.c_str());
		}
	}
	else if (infile.key == "location") {
		// @ATTR event.location|[x,y,w,h]|Defines the location area for the event.
		evnt->location.x = toInt(infile.nextValue());
		evnt->location.y = toInt(infile.nextValue());
		evnt->location.w = toInt(infile.nextValue());
		evnt->location.h = toInt(infile.nextValue());

		if (evnt->center.x == -1 && evnt->center.y == -1) {
			evnt->center.x = evnt->location.x + (float)evnt->location.w/2;
			evnt->center.y = evnt->location.y + (float)evnt->location.h/2;
		}
	}
	else if (infile.key == "hotspot") {
		//  @ATTR event.hotspot|[ [x, y, w, h] : location ]|Event uses location as hotspot or defined by rect.
		if (infile.val == "location") {
			evnt->hotspot.x = evnt->location.x;
			evnt->hotspot.y = evnt->location.y;
			evnt->hotspot.w = evnt->location.w;
			evnt->hotspot.h = evnt->location.h;
		}
		else {
			evnt->hotspot.x = toInt(infile.nextValue());
			evnt->hotspot.y = toInt(infile.nextValue());
			evnt->hotspot.w = toInt(infile.nextValue());
			evnt->hotspot.h = toInt(infile.nextValue());
		}

		evnt->center.x = evnt->hotspot.x + (float)evnt->hotspot.w/2;
		evnt->center.y = evnt->hotspot.y + (float)evnt->hotspot.h/2;
	}
	else if (infile.key == "cooldown") {
		// @ATTR event.cooldown|duration|Duration for event cooldown.
		evnt->cooldown = parse_duration(infile.val);
	}
	else if (infile.key == "reachable_from") {
		// @ATTR event.reachable_from|[x,y,w,h]|If the hero is inside this rectangle, they can activate the event.
		evnt->reachable_from.x = toInt(infile.nextValue());
		evnt->reachable_from.y = toInt(infile.nextValue());
		evnt->reachable_from.w = toInt(infile.nextValue());
		evnt->reachable_from.h = toInt(infile.nextValue());
	}
	else {
		loadEventComponent(infile, evnt, NULL);
	}
}

void EventManager::loadEventComponent(FileParser &infile, Event* evnt, Event_Component* ec) {
	Event_Component *e = NULL;
	if (evnt) {
		evnt->components.push_back(Event_Component());
		e = &evnt->components.back();
	}
	else if (ec) {
		e = ec;
	}

	if (!e) return;

	e->type = infile.key;

	if (infile.key == "tooltip") {
		// @ATTR event.tooltip|string|Tooltip for event
		e->s = msg->get(infile.val);
	}
	else if (infile.key == "power_path") {
		// @ATTR event.power_path|[hero:[x,y]]|Event power path
		// x,y are src, if s=="hero" we target the hero,
		// else we'll use values in a,b as coordinates
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());

		std::string dest = infile.nextValue();
		if (dest == "hero") {
			e->s = "hero";
		}
		else {
			e->a = toInt(dest);
			e->b = toInt(infile.nextValue());
		}
	}
	else if (infile.key == "power_damage") {
		// @ATTR event.power_damage|min(integer), max(integer)|Range of power damage
		e->a = toInt(infile.nextValue());
		e->b = toInt(infile.nextValue());
	}
	else if (infile.key == "intermap") {
		// @ATTR event.intermap|[map(string),x(integer),y(integer)]|Jump to specific map at location specified.
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
	}
	else if (infile.key == "intramap") {
		// @ATTR event.intramap|[x(integer),y(integer)]|Jump to specific position within current map.
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
	}
	else if (infile.key == "mapmod") {
		// @ATTR event.mapmod|[string,int,int,int],..|Modify map tiles
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
		e->z = toInt(infile.nextValue());

		// add repeating mapmods
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;
				e->s = repeat_val;
				e->x = toInt(infile.nextValue());
				e->y = toInt(infile.nextValue());
				e->z = toInt(infile.nextValue());

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "soundfx") {
		// @ATTR event.soundfx|[soundfile(string),x(integer),y(integer)]|Play a sound at optional location
		e->s = infile.nextValue();
		e->x = e->y = -1;

		std::string s = infile.nextValue();
		if (s != "") e->x = toInt(s);

		s = infile.nextValue();
		if (s != "") e->y = toInt(s);

	}
	else if (infile.key == "loot") {
		// @ATTR event.loot|[string,drop_chance([fixed:chance(integer)]),quantity_min(integer),quantity_max(integer)],...|Add loot to the event
		loot->parseLoot(infile, e, &evnt->components);
	}
	else if (infile.key == "msg") {
		// @ATTR event.msg|string|Adds a message to be displayed for the event.
		e->s = msg->get(infile.val);
	}
	else if (infile.key == "shakycam") {
		// @ATTR event.shakycam|duration|Makes the camera shake for this duration.
		e->x = parse_duration(infile.val);
	}
	else if (infile.key == "requires_status") {
		// @ATTR event.requires_status|string,...|Event requires list of statuses
		e->s = infile.nextValue();

		// add repeating requires_status
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;
				e->s = repeat_val;

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR event.requires_not|string,...|Event requires not list of statuses
		e->s = infile.nextValue();

		// add repeating requires_not
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;
				e->s = repeat_val;

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "requires_level") {
		// @ATTR event.requires_level|integer|Event requires hero level
		e->x = toInt(infile.nextValue());
	}
	else if (infile.key == "requires_not_level") {
		// @ATTR event.requires_not_level|integer|Event requires not hero level
		e->x = toInt(infile.nextValue());
	}
	else if (infile.key == "requires_currency") {
		// @ATTR event.requires_currency|integer|Event requires atleast this much currency
		e->x = toInt(infile.nextValue());
	}
	else if (infile.key == "requires_item") {
		// @ATTR event.requires_item|integer,...|Event requires specific item
		e->x = toInt(infile.nextValue());

		// add repeating requires_item
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;
				e->x = toInt(repeat_val);

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "set_status") {
		// @ATTR event.set_status|string,...|Sets specified statuses
		e->s = infile.nextValue();

		// add repeating set_status
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;
				e->s = repeat_val;

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "unset_status") {
		// @ATTR event.unset_status|string,...|Unsets specified statuses
		e->s = infile.nextValue();

		// add repeating unset_status
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;
				e->s = repeat_val;

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "remove_currency") {
		// @ATTR event.remove_currency|integer|Removes specified amount of currency from hero inventory
		e->x = toInt(infile.val);
		clampFloor(e->x, 0);
	}
	else if (infile.key == "remove_item") {
		// @ATTR event.remove_item|integer,...|Removes specified item from hero inventory
		e->x = toInt(infile.nextValue());

		// add repeating remove_item
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;
				e->x = toInt(repeat_val);

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "reward_xp") {
		// @ATTR event.reward_xp|integer|Reward hero with specified amount of experience points.
		e->x = toInt(infile.val);
		clampFloor(e->x, 0);
	}
	else if (infile.key == "reward_currency") {
		// @ATTR event.reward_currency|integer|Reward hero with specified amount of currency.
		e->x = toInt(infile.val);
		clampFloor(e->x, 0);
	}
	else if (infile.key == "reward_item") {
		// @ATTR event.reward_item|x(integer),y(integer)|Reward hero with y number of item x.
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.val);
		clampFloor(e->y, 0);
	}
	else if (infile.key == "restore") {
		// @ATTR event.restore|string|Restore the hero's HP, MP, and/or status.
		e->s = infile.val;
	}
	else if (infile.key == "power") {
		// @ATTR event.power|power_id|Specify power coupled with event.
		e->x = toInt(infile.val);
	}
	else if (infile.key == "spawn") {
		// @ATTR event.spawn|[string,x(integer),y(integer)], ...|Spawn specified enemies at location
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());

		// add repeating spawn
		if (evnt) {
			std::string repeat_val = infile.nextValue();
			while (repeat_val != "") {
				evnt->components.push_back(Event_Component());
				e = &evnt->components.back();
				e->type = infile.key;

				e->s = repeat_val;
				e->x = toInt(infile.nextValue());
				e->y = toInt(infile.nextValue());

				repeat_val = infile.nextValue();
			}
		}
	}
	else if (infile.key == "stash") {
		// @ATTR event.stash|boolean|Opens the Stash menu.
		e->s = infile.val;
	}
	else if (infile.key == "npc") {
		// @ATTR event.npc|string|Filename of an NPC to start dialog with.
		e->s = infile.val;
	}
	else if (infile.key == "music") {
		// @ATTR event.music|string|Change background music to specified file.
		e->s = infile.val;
	}
	else if (infile.key == "cutscene") {
		// @ATTR event.cutscene|string|Show specified cutscene.
		e->s = infile.val;
	}
	else if (infile.key == "repeat") {
		// @ATTR event.repeat|boolean|Allows the event to be triggered again.
		e->s = infile.val;
	}
	else if (infile.key == "save_game") {
		// @ATTR event.save_game|boolean|Saves the game when the event is triggered. The respawn position is set to where the player is standing.
		e->s = infile.val;
	}
	else {
		infile.error("EventManager: '%s' is not a valid key.", infile.key.c_str());
	}
}

/**
 * A particular event has been triggered.
 * Process all of this events components.
 *
 * @param The triggered event
 * @return Returns true if the event shall not be run again.
 */
bool EventManager::executeEvent(Event &ev) {
	if(&ev == NULL) return false;

	// skip executing events that are on cooldown
	if (ev.cooldown_ticks > 0) return false;

	// set cooldown
	ev.cooldown_ticks = ev.cooldown;

	Event_Component *ec;

	for (unsigned i = 0; i < ev.components.size(); ++i) {
		ec = &ev.components[i];

		if (ec->type == "set_status") {
			camp->setStatus(ec->s);
		}
		else if (ec->type == "unset_status") {
			camp->unsetStatus(ec->s);
		}
		else if (ec->type == "intermap") {

			if (fileExists(mods->locate(ec->s))) {
				mapr->teleportation = true;
				mapr->teleport_mapname = ec->s;
				mapr->teleport_destination.x = ec->x + 0.5f;
				mapr->teleport_destination.y = ec->y + 0.5f;
			}
			else {
				ev.keep_after_trigger = false;
				mapr->log_msg = msg->get("Unknown destination");
			}
		}
		else if (ec->type == "intramap") {
			mapr->teleportation = true;
			mapr->teleport_mapname = "";
			mapr->teleport_destination.x = ec->x + 0.5f;
			mapr->teleport_destination.y = ec->y + 0.5f;
		}
		else if (ec->type == "mapmod") {
			if (ec->s == "collision") {
				if (ec->x >= 0 && ec->x < 256 && ec->y >= 0 && ec->y < 256)
					mapr->collider.colmap[ec->x][ec->y] = ec->z;
				else
					logError("EventManager: Mapmod at position (%d, %d) is out of bounds 0-255.\n", ec->x, ec->y);
			}
			else {
				int index = distance(mapr->layernames.begin(), find(mapr->layernames.begin(), mapr->layernames.end(), ec->s));
				if (ec->x >= 0 && ec->x < 256 && ec->y >= 0 && ec->y < 256)
					mapr->layers[index][ec->x][ec->y] = ec->z;
				else
					logError("EventManager: Mapmod at position (%d, %d) is out of bounds 0-255.\n", ec->x, ec->y);
			}
			mapr->map_change = true;
		}
		else if (ec->type == "soundfx") {
			FPoint pos(0,0);
			bool loop = false;

			if (ec->x != -1 && ec->y != -1) {
				if (ec->x != 0 && ec->y != 0) {
					pos.x = ec->x + 0.5f;
					pos.y = ec->y + 0.5f;
				}
			}
			else if (ev.location.x != 0 && ev.location.y != 0) {
				pos.x = ev.location.x + 0.5f;
				pos.y = ev.location.y + 0.5f;
			}

			if (ev.type == "on_load")
				loop = true;

			SoundManager::SoundID sid = snd->load(ec->s, "MapRenderer background soundfx");

			snd->play(sid, GLOBAL_VIRTUAL_CHANNEL, pos, loop);
			mapr->sids.push_back(sid);
		}
		else if (ec->type == "loot") {
			ec->x = ev.hotspot.x;
			ec->y = ev.hotspot.y;
			mapr->loot.push_back(*ec);
		}
		else if (ec->type == "msg") {
			mapr->log_msg = ec->s;
		}
		else if (ec->type == "shakycam") {
			mapr->shaky_cam_ticks = ec->x;
		}
		else if (ec->type == "remove_currency") {
			camp->removeCurrency(ec->x);
		}
		else if (ec->type == "remove_item") {
			camp->removeItem(ec->x);
		}
		else if (ec->type == "reward_xp") {
			camp->rewardXP(ec->x, true);
		}
		else if (ec->type == "reward_currency") {
			camp->rewardCurrency(ec->x);
		}
		else if (ec->type == "reward_item") {
			ItemStack istack;
			istack.item = ec->x;
			istack.quantity = ec->y;
			camp->rewardItem(istack);
		}
		else if (ec->type == "restore") {
			camp->restoreHPMP(ec->s);
		}
		else if (ec->type == "spawn") {
			Point spawn_pos;
			spawn_pos.x = ec->x;
			spawn_pos.y = ec->y;
			powers->spawn(ec->s, spawn_pos);
		}
		else if (ec->type == "power") {

			int power_index = ec->x;

			Event_Component *ec_path = ev.getComponent("power_path");
			if (ev.stats == NULL) {
				ev.stats = new StatBlock();

				ev.stats->current[STAT_ACCURACY] = 1000; //always hits its target

				// if a power path was specified, place the source position there
				if (ec_path) {
					ev.stats->pos.x = ec_path->x + 0.5f;
					ev.stats->pos.y = ec_path->y + 0.5f;
				}
				// otherwise the source position is the event position
				else {
					ev.stats->pos.x = ev.location.x + 0.5f;
					ev.stats->pos.y = ev.location.y + 0.5f;
				}

				Event_Component *ec_damage = ev.getComponent("power_damage");
				if (ec_damage) {
					ev.stats->current[STAT_DMG_MELEE_MIN] = ev.stats->current[STAT_DMG_RANGED_MIN] = ev.stats->current[STAT_DMG_MENT_MIN] = ec_damage->a;
					ev.stats->current[STAT_DMG_MELEE_MAX] = ev.stats->current[STAT_DMG_RANGED_MAX] = ev.stats->current[STAT_DMG_MENT_MAX] = ec_damage->b;
				}
			}

			FPoint target;

			if (ec_path) {
				// targets hero option
				if (ec_path->s == "hero") {
					target.x = mapr->cam.x;
					target.y = mapr->cam.y;
				}
				// targets fixed path option
				else {
					target.x = ec_path->a + 0.5f;
					target.y = ec_path->b + 0.5f;
				}
			}
			// no path specified, targets self location
			else {
				target.x = ev.stats->pos.x;
				target.y = ev.stats->pos.y;
			}

			powers->activate(power_index, ev.stats, target);
		}
		else if (ec->type == "stash") {
			mapr->stash = toBool(ec->s);
			if (mapr->stash) {
				mapr->stash_pos.x = ev.location.x + 0.5f;
				mapr->stash_pos.y = ev.location.y + 0.5f;
			}
		}
		else if (ec->type == "npc") {
			mapr->event_npc = ec->s;
		}
		else if (ec->type == "music") {
			mapr->music_filename = ec->s;
			mapr->loadMusic();
		}
		else if (ec->type == "cutscene") {
			mapr->cutscene = true;
			mapr->cutscene_file = ec->s;
		}
		else if (ec->type == "repeat") {
			ev.keep_after_trigger = toBool(ec->s);
		}
		else if (ec->type == "save_game") {
			mapr->save_game = toBool(ec->s);
		}
	}
	return !ev.keep_after_trigger;
}


bool EventManager::isActive(const Event &e) {
	for (unsigned i=0; i < e.components.size(); i++) {
		if (e.components[i].type == "requires_not_status") {
			if (camp->checkStatus(e.components[i].s)) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_status") {
			if (!camp->checkStatus(e.components[i].s)) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_currency") {
			if (!camp->checkCurrency(e.components[i].x)) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_item") {
			if (!camp->checkItem(e.components[i].x)) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_level") {
			if (camp->hero->level < e.components[i].x) {
				return false;
			}
		}
		else if (e.components[i].type == "requires_not_level") {
			if (camp->hero->level >= e.components[i].x) {
				return false;
			}
		}
	}
	return true;
}

