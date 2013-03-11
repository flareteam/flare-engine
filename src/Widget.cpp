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

#include "Widget.h"

Widget::Widget()
	: render_to_alpha(false)
	, in_focus(false)
	, focusable(false)
	, pos(SDL_Rect())
{
	pos.x = pos.y = pos.w = pos.h = 0;
}

Widget::~Widget()
{}

void Widget::activate()
{}

TabList::TabList()
	: widgets()
	, current(-1)
{}

TabList::~TabList()
{}

void TabList::add(Widget* widget) {
	if( widget == NULL )
		return;

	widgets.push_back(widget);
}

void TabList::remove(Widget* widget) {
	std::vector<Widget*>::iterator find = std::find(
		widgets.begin(),
		widgets.end(),
		widget
	);

	if(find != widgets.end())
		widgets.erase(find);
}

Widget* TabList::getNext() {
	if(widgets.size() == 0)
		return NULL;

	if(current >= 0 && current < widgets.size())
		widgets.at(current)->in_focus = false;

	++current;

	if(current >= widgets.size())
		current = 0;

	widgets.at(current)->in_focus = true;
	return widgets.at(current);
}

Widget* TabList::getPrev() {
	if(widgets.size() == 0)
		return NULL;

	if(current >= 0 && current < widgets.size())
		widgets.at(current)->in_focus = false;

	--current;

	if(current <= -1)
		current = widgets.size()-1;

	widgets.at(current)->in_focus = true;
	return widgets.at(current);
}

void TabList::activate() {
	if(current >= 0 && current < widgets.size()) {
		widgets.at(current)->activate();
	}
}

void TabList::defocus() {
	if(current >= 0 && current < widgets.size())
		widgets.at(current)->in_focus = false;

	current = -1;
}