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

#include "Settings.h"
#include "WidgetScrollBox.h"

WidgetScrollBox::WidgetScrollBox(int width, int height)
	: contents(NULL) {
	pos.x = pos.y = 0;
	pos.w = width;
	pos.h = height;
	cursor = 0;
	bg.r = bg.g = bg.b = 0;
	bg.a = 255;
	currentChild = -1;
	scrollbar = new WidgetScrollBar();
	update = true;
	render_to_alpha = false;
	transparent = true;
	line_height = 20;
	resize(width, height);
	tablist = TabList(VERTICAL);

	scroll_type = VERTICAL;
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
	scroll(line_height);
}

void WidgetScrollBox::scrollUp() {
	scroll(-line_height);
}

Point WidgetScrollBox::input_assist(const Point& mouse) {
	Point new_mouse;
	if (isWithinRect(pos,mouse)) {
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
		tablist.logic();
	}
}

void WidgetScrollBox::logic(int x, int y) {
	Point mouse(x, y);

	if (isWithinRect(pos,mouse)) {
		inpt->lock_scroll = true;
		if (inpt->scroll_up) scrollUp();
		if (inpt->scroll_down) scrollDown();
	}
	else {
		inpt->lock_scroll = false;
	}

	// check ScrollBar clicks
	if (contents && contents->getGraphicsHeight() > pos.h && scrollbar) {
		switch (scrollbar->checkClick(mouse.x,mouse.y)) {
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

	if (contents && !transparent) {
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

		if (contents && !transparent) {
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
		contents->setClip(src);
		contents->setDest(dest);
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

	if (in_focus) {
		Point topLeft;
		Point bottomRight;

		topLeft.x = dest.x + local_frame.x - local_offset.x;
		topLeft.y = dest.y + local_frame.y - local_offset.y;
		bottomRight.x = topLeft.x + dest.w;
		bottomRight.y = topLeft.y + dest.h;
		Color color = Color(255,248,220,255);

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
			render_device->drawRectangle(topLeft, bottomRight, color);
		}
	}
}

bool WidgetScrollBox::getNext() {
	if (children.empty()) {
		scrollDown();
		return true;
	}

	if (currentChild != -1)
		children[currentChild]->in_focus = false;
	currentChild+=1;
	currentChild = (static_cast<unsigned>(currentChild) == children.size()) ? 0 : currentChild;

	if (children[currentChild]->pos.y > (cursor + pos.h) ||
			(children[currentChild]->pos.y + children[currentChild]->pos.h) > (cursor + pos.h)) {
		scrollTo(children[currentChild]->pos.y+children[currentChild]->pos.h-pos.h);
	}
	if (children[currentChild]->pos.y < cursor ||
			(children[currentChild]->pos.y + children[currentChild]->pos.h) < cursor) {
		scrollTo(children[currentChild]->pos.y);
	}
	children[currentChild]->in_focus = true;
	return true;
}

bool WidgetScrollBox::getPrev() {
	if (children.empty()) {
		scrollUp();
		return true;
	}

	if (currentChild != -1)
		children[currentChild]->in_focus = false;
	currentChild-=1;
	currentChild = (currentChild < 0) ? static_cast<int>(children.size()) - 1 : currentChild;

	if (children[currentChild]->pos.y > (cursor + pos.h) ||
			(children[currentChild]->pos.y + children[currentChild]->pos.h) > (cursor + pos.h)) {
		scrollTo(children[currentChild]->pos.y+children[currentChild]->pos.h-pos.h);
	}
	if (children[currentChild]->pos.y < cursor ||
			(children[currentChild]->pos.y + children[currentChild]->pos.h) < cursor) {
		scrollTo(children[currentChild]->pos.y);
	}
	children[currentChild]->in_focus = true;
	return true;
}

void WidgetScrollBox::activate() {
	if (currentChild != -1)
		children[currentChild]->activate();
}
