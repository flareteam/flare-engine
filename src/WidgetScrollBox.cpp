/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2016 Justin Jacobs
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson

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
 * class WidgetScrollBox
 */

#include "EngineSettings.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "WidgetScrollBox.h"

WidgetScrollBox::WidgetScrollBox(int width, int height)
	: contents(NULL) {
	pos.x = pos.y = 0;
	pos.w = width;
	pos.h = height;
	cursor = 0;
	bg.r = bg.g = bg.b = 0;
	bg.a = 0;
	currentChild = -1;
	scrollbar = new WidgetScrollBar(WidgetScrollBar::DEFAULT_FILE);
	update = true;
	resize(width, height);
	tablist = TabList();
	tablist.setScrollType(SCROLL_TWO_DIRECTIONS);

	scroll_type = SCROLL_VERTICAL;
}

WidgetScrollBox::~WidgetScrollBox() {
	if (contents) delete contents;
	delete scrollbar;
}

void WidgetScrollBox::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);

	if (contents && scrollbar) {
		scrollbar->refresh(pos.x+pos.w, pos.y, pos.h-scrollbar->pos_down.h, cursor,
						   contents->getGraphicsHeight()-pos.h);
	}
}

void WidgetScrollBox::addChildWidget(Widget* child) {

	std::vector<Widget*>::iterator find = std::find(
			children.begin(),
			children.end(),
			child);

	if (find == children.end()) {
		children.push_back(child);
		tablist.add(child);
		child->local_frame = pos;
	}

}

void WidgetScrollBox::clearChildWidgets() {
	currentChild = -1;
	children.clear();
	tablist.clear();
}

void WidgetScrollBox::scroll(int amount) {
	cursor += amount;
	if (cursor < 0) {
		cursor = 0;
	}
	else if (contents && cursor > contents->getGraphicsHeight() - pos.h) {
		cursor = contents->getGraphicsHeight() - pos.h;
	}
	refresh();
}

void WidgetScrollBox::scrollTo(int amount) {
	cursor = amount;
	if (cursor < 0) {
		cursor = 0;
	}
	else if (contents && cursor > contents->getGraphicsHeight() - pos.h) {
		cursor = contents->getGraphicsHeight() - pos.h;
	}
	refresh();
}

void WidgetScrollBox::scrollDown() {
	int amount = pos.h / 4;
	scroll(amount);
}

void WidgetScrollBox::scrollUp() {
	int amount = pos.h / 4;
	scroll(-amount);
}

void WidgetScrollBox::scrollToTop() {
	scrollTo(0);
}

Point WidgetScrollBox::input_assist(const Point& mouse) {
	Point new_mouse;
	if (Utils::isWithinRect(pos,mouse)) {
		new_mouse.x = mouse.x-pos.x;
		new_mouse.y = mouse.y-pos.y+cursor;
	}
	else {
		new_mouse.x = -1;
		new_mouse.y = -1;
	}
	return new_mouse;
}

void WidgetScrollBox::logic() {
	logic(inpt->mouse.x,inpt->mouse.y);
	if (in_focus) {
		if (currentChild == -1 && !children.empty())
			getNext();
		tablist.logic();
	}
	else {
		// TODO don't run every frame
		tablist.defocus();
		currentChild = -1;
	}
}

void WidgetScrollBox::logic(int x, int y) {
	Point mouse(x, y);

	if (Utils::isWithinRect(pos,mouse)) {
		inpt->lock_scroll = true;
		if (inpt->scroll_up) scrollUp();
		if (inpt->scroll_down) scrollDown();
	}
	else {
		inpt->lock_scroll = false;
	}

	// check ScrollBar clicks
	if (contents && contents->getGraphicsHeight() > pos.h && scrollbar) {
		switch (scrollbar->checkClickAt(mouse.x,mouse.y)) {
			case 1:
				scrollUp();
				break;
			case 2:
				scrollDown();
				break;
			case 3:
				cursor = scrollbar->getValue();
				break;
			default:
				break;
		}
	}
}

void WidgetScrollBox::resize(int w, int h) {

	pos.w = w;

	if (pos.h > h) h = pos.h;

	if (contents) {
		delete contents;
		contents = NULL;
	}

	Image *graphics;
	graphics = render_device->createImage(pos.w,h);
	if (graphics) {
		contents = graphics->createSprite();
		graphics->unref();
	}

	if (contents) {
		contents->getGraphics()->fillWithColor(bg);
	}

	cursor = 0;
	refresh();
}

void WidgetScrollBox::refresh() {
	if (update) {
		int h = pos.h;
		if (contents) {
			h = contents->getGraphicsHeight();
		}

		if (contents) {
			delete contents;
			contents = NULL;
		}

		Image *graphics;
		graphics = render_device->createImage(pos.w,h);
		if (graphics) {
			contents = graphics->createSprite();
			graphics->unref();
		}

		if (contents) {
			contents->getGraphics()->fillWithColor(bg);
		}
	}

	if (contents && scrollbar) {
		scrollbar->refresh(pos.x+pos.w, pos.y, pos.h-scrollbar->pos_down.h, cursor,
						   contents->getGraphicsHeight()-pos.h);
	}
}

void WidgetScrollBox::render() {
	Rect src,dest;
	dest = pos;
	src.x = 0;
	src.y = cursor;
	src.w = pos.w;
	src.h = pos.h;

	if (contents) {
		contents->local_frame = local_frame;
		contents->setOffset(local_offset);
		contents->setClipFromRect(src);
		contents->setDestFromRect(dest);
		render_device->render(contents);
	}

	for (unsigned i = 0; i < children.size(); i++) {
		children[i]->local_frame = pos;
		children[i]->local_offset.y = cursor;
		children[i]->render();
	}

	if (contents && contents->getGraphicsHeight() > pos.h && scrollbar) {
		scrollbar->local_frame = local_frame;
		scrollbar->local_offset = local_offset;
		scrollbar->render();
	}
	update = false;

	if (in_focus && children.empty()) {
		Point topLeft;
		Point bottomRight;
		Rect sb_rect = scrollbar->getBounds();

		topLeft.x = sb_rect.x + local_frame.x - local_offset.x;
		topLeft.y = sb_rect.y + local_frame.y - local_offset.y;
		bottomRight.x = topLeft.x + sb_rect.w;
		bottomRight.y = topLeft.y + sb_rect.h;

		// Only draw rectangle if it fits in local frame
		bool draw = true;
		if (local_frame.w &&
				(topLeft.x<local_frame.x || bottomRight.x>(local_frame.x+local_frame.w))) {
			draw = false;
		}
		if (local_frame.h &&
				(topLeft.y<local_frame.y || bottomRight.y>(local_frame.y+local_frame.h))) {
			draw = false;
		}
		if (draw) {
			render_device->drawRectangle(topLeft, bottomRight, eset->widgets.selection_rect_color);
		}
	}
}

bool WidgetScrollBox::getNext() {
	if (children.empty()) {
		int prev_cursor = cursor;
		int bottom = contents ? contents->getGraphicsHeight() - pos.h : 0;

		scrollDown();

		if (cursor == bottom && prev_cursor == bottom)
			return false;

		return true;
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = false;
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_DOWN);
		tablist.setCurrent(children[currentChild]);
	}
	else {
		// TODO neaten this up?
		currentChild = 0;
		tablist.setCurrent(children[currentChild]);
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_DOWN);
		tablist.setCurrent(children[currentChild]);
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_UP);
		tablist.setCurrent(children[currentChild]);
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = true;
		scrollTo(children[currentChild]->pos.y);
	}
	else {
		return false;
	}

	return true;
}

bool WidgetScrollBox::getPrev() {
	if (children.empty()) {
		int prev_cursor = cursor;

		scrollUp();

		if (cursor == 0 && prev_cursor == 0)
			return false;

		return true;
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = false;
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_UP);
		tablist.setCurrent(children[currentChild]);
	}
	else {
		currentChild = 0;
		tablist.setCurrent(children[currentChild]);
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_DOWN);
		tablist.setCurrent(children[currentChild]);
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_UP);
		tablist.setCurrent(children[currentChild]);
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = true;
		scrollTo(children[currentChild]->pos.y);
	}
	else {
		return false;
	}

	return true;
}

void WidgetScrollBox::activate() {
	if (currentChild != -1)
		children[currentChild]->activate();
}
