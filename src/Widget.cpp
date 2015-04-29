/*
Copyright © 2012 Stefan Beller
Copyright © 2013 Joseph Bleau

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

#include "SharedResources.h"
#include "Settings.h"
#include "Widget.h"

Widget::Widget()
	: render_to_alpha(false)
	, in_focus(false)
	, focusable(false)
	, alignment(ALIGN_TOPLEFT) {
}

Widget::~Widget() {
}

void Widget::activate() {
}

void Widget::deactivate() {
}

void Widget::defocus() {
	in_focus = false;
}

bool Widget::getNext() {
	return false;
}

bool Widget::getPrev() {
	return false;
}

void Widget::setBasePos(int x, int y, ALIGNMENT a) {
	pos_base.x = x;
	pos_base.y = y;
	alignment = a;
}

void Widget::setPos(int offset_x, int offset_y) {
	pos.x = pos_base.x + offset_x;
	pos.y = pos_base.y + offset_y;
	alignToScreenEdge(alignment, &pos);
}

TabList::TabList(ScrollType _scrolltype, int _LEFT, int _RIGHT, int _ACTIVATE)
	: widgets()
	, current(-1)
	, previous(-1)
	, locked(false)
	, scrolltype(_scrolltype)
	, MV_LEFT(_LEFT)
	, MV_RIGHT(_RIGHT)
	, ACTIVATE(_ACTIVATE) {
}

TabList::~TabList() {
}

bool TabList::isLocked() {
	return locked;
}

void TabList::lock() {
	locked = true;
	if (current_is_valid())
		widgets.at(current)->defocus();
}

void TabList::unlock() {
	locked = false;
	if (current_is_valid())
		widgets.at(current)->in_focus = true;
}

void TabList::add(Widget* widget) {
	if (widget == NULL)
		return;

	std::vector<Widget*>::iterator find = std::find(
			widgets.begin(),
			widgets.end(),
			widget
										  );
	if (find == widgets.end())
		widgets.push_back(widget);
}

void TabList::remove(Widget* widget) {
	std::vector<Widget*>::iterator find = std::find(
			widgets.begin(),
			widgets.end(),
			widget
										  );

	if (find != widgets.end())
		widgets.erase(find);
}

void TabList::clear() {
	widgets.clear();
}

void TabList::setCurrent(Widget* widget) {
	if (!widget) {
		current = -1;
		return;
	}

	for (unsigned i=0; i<widgets.size(); ++i) {
		if (widgets[i] == widget) {
			current = i;
			break;
		}
	}
}

int TabList::getCurrent() {
	return current;
}

unsigned TabList::size() {
	return widgets.size();
}

bool TabList::current_is_valid() {
	return current >= 0 && current < (int)widgets.size();
}

bool TabList::previous_is_valid() {
	return previous >= 0 && previous < (int)widgets.size();
}

Widget* TabList::getNext(bool inner) {
	if (widgets.size() == 0)
		return NULL;

	if (current_is_valid()) {
		if (inner && widgets.at(current)->getNext())
			return NULL;

		widgets.at(current)->defocus();
	}
	++current;

	if (current >= (int)widgets.size())
		current = 0;

	widgets.at(current)->in_focus = true;
	return widgets.at(current);
}

Widget* TabList::getPrev(bool inner) {
	if (widgets.size() == 0)
		return NULL;

	if (current_is_valid()) {
		if (inner && widgets.at(current)->getPrev())
			return NULL;

		widgets.at(current)->defocus();
	}

	--current;

	if (current <= -1)
		current = widgets.size()-1;

	widgets.at(current)->in_focus = true;
	return widgets.at(current);
}

void TabList::deactivatePrevious() {
	if (previous_is_valid() && previous != current) {
		widgets.at(previous)->deactivate();
	}
}

void TabList::activate() {
	if (current_is_valid()) {
		widgets.at(current)->activate();
		previous = current;
	}
}

void TabList::defocus() {
	for (unsigned i=0; i < widgets.size(); ++i) {
		widgets.at(i)->defocus();
	}

	current = -1;
}

void TabList::logic(bool allow_keyboard) {
	if (locked) return;
	if (NO_MOUSE || allow_keyboard) {
		if (scrolltype == VERTICAL || scrolltype == TWO_DIRECTIONS) {
			if (inpt->pressing[DOWN] && !inpt->lock[DOWN]) {
				inpt->lock[DOWN] = true;
				getNext();
			}
			else if (inpt->pressing[UP] && !inpt->lock[UP]) {
				inpt->lock[UP] = true;
				getPrev();
			}
		}

		if (scrolltype == HORIZONTAL || scrolltype == TWO_DIRECTIONS) {
			if (inpt->pressing[MV_LEFT] && !inpt->lock[MV_LEFT]) {
				inpt->lock[MV_LEFT] = true;
				getPrev(false);
			}
			else if (inpt->pressing[MV_RIGHT] && !inpt->lock[MV_RIGHT]) {
				inpt->lock[MV_RIGHT] = true;
				getNext(false);
			}
		}

		if (inpt->pressing[ACTIVATE] && !inpt->lock[ACTIVATE]) {
			inpt->lock[ACTIVATE] = true;
			deactivatePrevious(); //Deactivate previously activated item
			activate();	// Activate the currently infocus item
		}
	}

	// If mouse is clicked, defocus current tabindex item
	if (inpt->pressing[MAIN1] && !inpt->lock[MAIN1] && current_is_valid() && !isWithin(widgets[getCurrent()]->pos, inpt->mouse)) {
		defocus();
	}
}
