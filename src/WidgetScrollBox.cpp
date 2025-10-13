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
#include "Settings.h"
#include "WidgetScrollBox.h"

WidgetScrollBox::WidgetScrollBox(int width, int height)
	: contents(NULL)
	, update(true)
	, show_focus_when_scrollbar_disabled(true)
	, bg(0,0,0,0)
	, tablist()
	, cursor(0)
	, cursor_target(0)
	, scrollbar(new WidgetScrollBar(WidgetScrollBar::DEFAULT_FILE))
{
	pos.x = pos.y = 0;
	pos.w = width;
	pos.h = height;
	currentChild = -1;
	scroll_type = SCROLL_VERTICAL;

	resize(width, height);
	tablist.setScrollType(SCROLL_TWO_DIRECTIONS);
	tablist.is_inner_tablist = true;
}

WidgetScrollBox::~WidgetScrollBox() {
	if (contents) delete contents;
	delete scrollbar;
}

void WidgetScrollBox::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);

	if (contents && scrollbar) {
		scrollbar->refresh(pos.x + pos.w, pos.y, pos.h, static_cast<int>(cursor), contents_size.y - pos.h);
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
	cursor_target += static_cast<float>(amount);
	if (cursor_target < 0) {
		cursor_target = 0;
	}
	else if (contents && cursor_target > static_cast<float>(contents_size.y - pos.h)) {
		cursor_target = static_cast<float>(contents_size.y - pos.h);
	}
	refresh();
}

void WidgetScrollBox::scrollTo(int amount) {
	cursor = static_cast<float>(amount);
	if (cursor < 0) {
		cursor = 0;
	}
	else if (contents && cursor > static_cast<float>(contents_size.y - pos.h)) {
		cursor = static_cast<float>(contents_size.y - pos.h);
	}
	cursor_target = cursor;
	refresh();
}

void WidgetScrollBox::scrollToSmooth(int amount) {
	cursor_target = static_cast<float>(amount);
	if (cursor_target < 0) {
		cursor_target = 0;
	}
	else if (contents && cursor_target > static_cast<float>(contents_size.y - pos.h)) {
		cursor_target = static_cast<float>(contents_size.y - pos.h);
	}
	refresh();
}

void WidgetScrollBox::scrollDown() {
	int amount = pos.h / SCROLL_SPEED_COARSE_MOD;
	scroll(amount);
}

void WidgetScrollBox::scrollUp() {
	int amount = pos.h / SCROLL_SPEED_COARSE_MOD;
	scroll(-amount);
}

void WidgetScrollBox::scrollToTop() {
	scrollTo(0);
}

Point WidgetScrollBox::input_assist(const Point& mouse) {
	Point new_mouse;
	if (Utils::isWithinRect(pos,mouse)) {
		new_mouse.x = mouse.x-pos.x;
		new_mouse.y = mouse.y-pos.y + static_cast<int>(cursor);
	}
	else {
		// x position is maintained for dragging of WidgetSlider knobs
		new_mouse.x = mouse.x-pos.x;
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
	else if (currentChild != -1 || tablist.getCurrent() != -1) {
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
	if (contents && contents_size.y > pos.h && scrollbar) {
		switch (scrollbar->checkClickAt(mouse.x,mouse.y)) {
			case WidgetScrollBar::CLICK_UP:
				scrollUp();
				break;
			case WidgetScrollBar::CLICK_DOWN:
				scrollDown();
				break;
			case WidgetScrollBar::CLICK_KNOB:
				cursor = cursor_target = static_cast<float>(scrollbar->getValue());
				break;
			default:
				break;
		}
	}

	if (cursor_target < cursor) {
		cursor -= (static_cast<float>(pos.h * SCROLL_SPEED_SMOOTH_MOD) + (cursor - cursor_target)) / settings->max_frames_per_sec;
		if (cursor < cursor_target)
			cursor = cursor_target;
	}
	else if (cursor_target > cursor) {
		cursor += (static_cast<float>(pos.h * SCROLL_SPEED_SMOOTH_MOD) + (cursor_target - cursor)) / settings->max_frames_per_sec;
		if (cursor > cursor_target)
			cursor = cursor_target;
	}

	// getPrev() and getNext() aren't called when pressing left and right
	int current_child = tablist.getCurrent();
	if (current_child != -1) {
		if (children[current_child]->in_focus) {
			if (children[current_child]->pos.y < static_cast<int>(cursor) || children[current_child]->pos.y > static_cast<int>(cursor) + pos.h)
				scrollToSmooth(children[current_child]->pos.y);
		}
	}
}

void WidgetScrollBox::resize(int w, int h) {

	pos.w = w;

	if (pos.h > h)
		h = pos.h;

	contents_size.x = w;
	contents_size.y = h;

	cursor = cursor_target = 0;

	update = true;
	refresh();
}

void WidgetScrollBox::refresh() {
	if (update) {
		if (contents) {
			delete contents;
			contents = NULL;
		}

		Image *graphics;
		graphics = render_device->createImage(contents_size.x, contents_size.y);
		if (graphics) {
			contents = graphics->createSprite();
			graphics->unref();
		}

		if (contents) {
			contents->getGraphics()->fillWithColor(bg);
		}
	}

	if (contents && scrollbar) {
		scrollbar->refresh(pos.x + pos.w, pos.y, pos.h, static_cast<int>(cursor_target), contents_size.y - pos.h);
	}
}

void WidgetScrollBox::render() {
	update = false;

	Rect src,dest;
	dest = pos;
	src.x = 0;
	src.y = static_cast<int>(cursor);
	src.w = pos.w;
	src.h = pos.h;

	int content_height = 0;

	// draw content buffer, minus child widgets
	if (contents) {
		content_height = contents_size.y;
		contents->local_frame = local_frame;
		contents->setOffset(local_offset);
		contents->setClipFromRect(src);
		contents->setDestFromRect(dest);
		render_device->render(contents);
	}

	// draw child widgets
	for (unsigned i = 0; i < children.size(); i++) {
		children[i]->local_frame = pos;
		children[i]->local_offset.y = static_cast<int>(cursor);
		children[i]->render();
	}

	// draw scrollbar
	if (content_height > pos.h && scrollbar) {
		scrollbar->local_frame = local_frame;
		scrollbar->local_offset = local_offset;
		scrollbar->render();
	}

	// draw focus rectangle around the scrollbar
	if (in_focus && children.empty() && (show_focus_when_scrollbar_disabled || content_height > pos.h)) {
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
			render_device->drawRectangleCorners(eset->widgets.selection_rect_corner_size, topLeft, bottomRight, eset->widgets.selection_rect_color);
		}
	}
}

bool WidgetScrollBox::getNext() {
	if (children.empty()) {
		int prev_cursor = static_cast<int>(cursor);
		int bottom = contents ? contents_size.y - pos.h : 0;

		scrollDown();

		if (static_cast<int>(cursor) == bottom && prev_cursor == bottom)
			return false;

		return true;
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = false;
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_DOWN);
		if (currentChild != -1)
			tablist.setCurrent(children[currentChild]);
	}
	else {
		if (!children[0]->enable_tablist_nav) {
			tablist.getNext(!TabList::GET_INNER, TabList::WIDGET_SELECT_AUTO);
			currentChild = tablist.getCurrent();
		}
		else {
			currentChild = 0;
			tablist.setCurrent(children[currentChild]);
		}
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = true;
		scrollToSmooth(children[currentChild]->pos.y);
	}
	else {
		return false;
	}

	return true;
}

bool WidgetScrollBox::getPrev() {
	if (children.empty()) {
		int prev_cursor = static_cast<int>(cursor);

		scrollUp();

		if (cursor == 0 && prev_cursor == 0)
			return false;

		return true;
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = false;
		currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_UP);
		if (currentChild != -1)
			tablist.setCurrent(children[currentChild]);
	}
	else {
		if (!children[0]->enable_tablist_nav) {
			currentChild = tablist.getNextRelativeIndex(TabList::WIDGET_SELECT_DOWN);
			tablist.setCurrent(children[currentChild]);
		}
		else {
			currentChild = 0;
			tablist.setCurrent(children[currentChild]);
		}
	}

	if (currentChild != -1) {
		children[currentChild]->in_focus = true;
		scrollToSmooth(children[currentChild]->pos.y);
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
