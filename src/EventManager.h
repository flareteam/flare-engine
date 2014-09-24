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

/*
 * class EventManager
 */

#pragma once
#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "FileParser.h"
#include "Utils.h"
#include "StatBlock.h"

class Event {
public:
	std::string type;
	std::vector<Event_Component> components;
	Rect location;
	Rect hotspot;
	int cooldown; // events that run multiple times pause this long in frames
	int cooldown_ticks;
	StatBlock *stats;
	bool keep_after_trigger; // if this event has been triggered once, should this event be kept? If so, this event can be triggered multiple times.
	FPoint center;
	Rect reachable_from;

	Event();
	~Event();

	Event_Component* getComponent(const std::string &_type);
	void deleteAllComponents(const std::string &_type);
};

class EventManager {
public:
	EventManager();
	~EventManager();
	static void loadEvent(FileParser &infile, Event* evnt);
	static void loadEventComponent(FileParser &infile, Event* evnt, Event_Component* ec);

	static bool executeEvent(Event &e);
	static bool isActive(const Event &e);
};


#endif
