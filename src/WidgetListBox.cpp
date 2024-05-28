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
 * class WidgetListBox
 */

#include "EngineSettings.h"
#include "FontEngine.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "TooltipManager.h"
#include "WidgetLabel.h"
#include "WidgetListBox.h"
#include "WidgetScrollBar.h"

const std::string WidgetListBox::DEFAULT_FILE = "images/menus/buttons/listbox_default.png";

WidgetListBox::WidgetListBox(int height, const std::string& _fileName)
	: Widget()
	, fileName(_fileName)
	, listboxs(NULL)
	, cursor(0)
	, has_scroll_bar(false)
	, any_selected(false)
	, show_tooltip_for_selected(false)
	, rows(std::vector<Rect>(height,Rect()))
	, scrollbar(new WidgetScrollBar(WidgetScrollBar::DEFAULT_FILE))
	, pos_scroll()
	, pressed(false)
	, multi_select(false)
	, can_deselect(true)
	, can_select(true)
	, scrollbar_offset(0)
	, disable_text_trim(false) {

	// load ListBox images
	Image *graphics = NULL;
	if (fileName != DEFAULT_FILE) {
		graphics = render_device->loadImage(fileName, RenderDevice::ERROR_NORMAL);
	}
	if (!graphics) {
		graphics = render_device->loadImage(DEFAULT_FILE, RenderDevice::ERROR_EXIT);
	}
	if (graphics) {
		listboxs = graphics->createSprite();
		pos.w = listboxs->getGraphicsWidth();
		pos.h = (listboxs->getGraphicsHeight() / 3); // height of one item
		graphics->unref();
	}

	scroll_type = SCROLL_VERTICAL;
}

bool WidgetListBox::checkClick() {
	return checkClickAt(inpt->mouse.x,inpt->mouse.y);
}

void WidgetListBox::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);
	refresh();
}

/**
 * Sets and releases the "pressed" visual state of the ListBox
 * If press and release, activate (return true)
 */
bool WidgetListBox::checkClickAt(int x, int y) {

	Point mouse(x, y);

	refresh();

	if (inpt->usingMouse()) {
		checkTooltip(mouse);
	}
	else if (!inpt->usingMouse() && show_tooltip_for_selected) {
		int sel = getSelected();
		if (sel != -1) {
			int row = sel-cursor;
			Point tip_pos;
			tip_pos.x = rows[row].x + rows[row].w/2;
			tip_pos.y = rows[row].y + rows[row].h/2;
			checkTooltip(tip_pos);
		}
	}

	// check scroll wheel
	Rect scroll_area;
	scroll_area.x = rows[0].x;
	scroll_area.y = rows[0].y;
	scroll_area.w = rows[0].w;
	scroll_area.h = rows[0].h * static_cast<int>(rows.size());

	if (Utils::isWithinRect(scroll_area,mouse)) {
		inpt->lock_scroll = true;
		if (inpt->scroll_up) scrollUp();
		if (inpt->scroll_down) scrollDown();
	}
	else {
		inpt->lock_scroll = false;
	}

	// check ScrollBar clicks
	if (has_scroll_bar) {
		switch (scrollbar->checkClickAt(mouse.x,mouse.y)) {
			case WidgetScrollBar::CLICK_UP:
				scrollUp();
				break;
			case WidgetScrollBar::CLICK_DOWN:
				scrollDown();
				break;
			case WidgetScrollBar::CLICK_KNOB:
				cursor = scrollbar->getValue();
				refresh();
				break;
			default:
				break;
		}
	}

	if (inpt->usingMouse()) {
		if (can_select && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
			for (size_t i=0; i<rows.size(); i++) {
				if (Utils::isWithinRect(rows[i], mouse) && i+cursor < items.size() && !items[i+cursor].value.empty()) {
					inpt->lock[Input::MAIN1] = true;
					// deselect other options if multi-select is disabled
					if (!multi_select) {
						for (unsigned j=0; j<items.size(); j++) {
							if (j != i+cursor)
								items[j].selected = false;
						}
					}
					// toggle selection
					if (items[i+cursor].selected) {
						if (can_deselect)
							items[i+cursor].selected = false;
					}
					else {
						items[i+cursor].selected = true;
					}
					refresh();
					return true;
				}
			}
		}
	}

	return false;

}

void WidgetListBox::checkTooltip(const Point& mouse) {
	TooltipData tip_data;
	for (size_t i=0; i<rows.size(); i++) {
		if (Utils::isWithinRect(rows[i], mouse) && i+cursor < items.size() && !items[i+cursor].tooltip.empty()) {
			tip_data.addText(items[i+cursor].tooltip);
			break;
		}
	}

	if (!tip_data.isEmpty()) {
		Point new_mouse(mouse.x + local_frame.x - local_offset.x, mouse.y + local_frame.y - local_offset.y);
		tooltipm->push(tip_data, new_mouse, TooltipData::STYLE_FLOAT);
	}
}

/**
 * Add a new value (with tooltip) to the list
 */
void WidgetListBox::append(const std::string& value, const std::string& tooltip) {
	items.resize(items.size()+1);
	items.back().value = value;
	items.back().tooltip = tooltip;
	refresh();
}

/**
 * Set a value (with tooltip) at a specific index
 */
void WidgetListBox::set(unsigned index, const std::string& value, const std::string& tooltip) {
	if (index >= items.size()) {
		append(value, tooltip);
		return;
	}

	items[index].value = value;
	items[index].tooltip = tooltip;
	refresh();
}

/**
 * Remove a value from the list
 */
void WidgetListBox::remove(int index) {
	items.erase(items.begin()+index);
	scrollUp();
	refresh();
}

/*
 * Clear the list
 */
void WidgetListBox::clear() {
	items.clear();
	refresh();
}

/*
 * Move an item up on the list
 */
void WidgetListBox::shiftUp() {
	any_selected = false;
	if (!items.empty() && !items[0].selected) {
		for (unsigned i=1; i < items.size(); i++) {
			if (items[i].selected) {
				any_selected = true;
				ListBoxItem tmp_item = items[i];

				items[i] = items[i-1];
				items[i-1] = tmp_item;
			}
		}
		if (any_selected) {
			scrollUp();
		}
	}
}

/*
 * Move an item down on the list
 */
void WidgetListBox::shiftDown() {
	any_selected = false;
	if (!items.empty() && !items[items.size()-1].selected) {
		for (int i=static_cast<int>(items.size())-2; i >= 0; i--) {
			if (items[i].selected) {
				any_selected = true;
				ListBoxItem tmp_item = items[i];

				items[i] = items[i+1];
				items[i+1] = tmp_item;
			}
		}
		if (any_selected) {
			scrollDown();
		}
	}
}

int WidgetListBox::getSelected() {
	// return the first selected value
	for (unsigned i=0; i<items.size(); i++) {
		if (items[i].selected) return i;
	}
	return -1; // nothing is selected
}

std::string WidgetListBox::getValue() {
	for (unsigned i=0; i<items.size(); i++) {
		if (items[i].selected) return items[i].value;
	}
	return "";
}

/*
 * Get the item name at a specific index
 */
std::string WidgetListBox::getValue(int index) {
	if (items.empty())
		return "";

	return items[index].value;
}

/*
 * Get the item tooltip at a specific index
 */
std::string WidgetListBox::getTooltip(int index) {
	if (items.empty())
		return "";

	return items[index].tooltip;
}

/*
 * Get the amount of ListBox items
 */
int WidgetListBox::getSize() {
	return static_cast<int>(items.size());
}

/*
 * Shift the viewing area up
 */
void WidgetListBox::scrollUp() {
	if (cursor > 0)
		cursor -= 1;
	refresh();
}

/*
 * Shift the viewing area down
 */
void WidgetListBox::scrollDown() {
	if (cursor+rows.size() < items.size())
		cursor += 1;
	refresh();
}

void WidgetListBox::render() {
	Rect src;
	src.x = 0;
	src.w = pos.w;
	src.h = pos.h;

	if (listboxs) {
		listboxs->local_frame = local_frame;
		listboxs->setOffset(local_offset);
	}

	for(unsigned i=0; i<rows.size(); i++) {
		if(i==0)
			src.y = 0;
		else if(i==rows.size()-1)
			src.y = pos.h*2;
		else
			src.y = pos.h;

		if (listboxs) {
			listboxs->setClipFromRect(src);
			listboxs->setDestFromRect(rows[i]);
			render_device->render(listboxs);
		}

		if (i+cursor < items.size()) {
			items[i+cursor].label.local_frame = local_frame;
			items[i+cursor].label.local_offset = local_offset;
			items[i+cursor].label.render();
		}
	}

	if (in_focus) {
		Point topLeft;
		Point bottomRight;

		if (!rows.empty()) {
			topLeft.x = rows[0].x + local_frame.x - local_offset.x;
			topLeft.y = rows[0].y + local_frame.y - local_offset.y;
			bottomRight.x = rows[rows.size() - 1].x + pos.w + local_frame.x - local_offset.x;
			bottomRight.y = rows[rows.size() - 1].y + pos.h + local_frame.y - local_offset.y;
		}

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

	if (has_scroll_bar) {
		scrollbar->local_frame = local_frame;
		scrollbar->local_offset = local_offset;
		scrollbar->render();
	}
}

/**
 * Create the text buffer
 * Also, toggle the scrollbar based on the size of the list
 */
void WidgetListBox::refresh() {
	int right_margin = 0;
	int padding = font->getFontHeight();

	// Update the scrollbar
	if (items.size() > rows.size()) {
		has_scroll_bar = true;

		pos_scroll.w = scrollbar->getBounds().w;
		pos_scroll.h = (pos.h * static_cast<int>(rows.size())) - (scrollbar_offset*2);

		pos_scroll.x = pos.x + pos.w - pos_scroll.w - scrollbar_offset;
		pos_scroll.y = pos.y + scrollbar_offset;

		scrollbar->refresh(pos_scroll.x, pos_scroll.y, pos_scroll.h, cursor, static_cast<int>(items.size()-rows.size()));

		right_margin = pos_scroll.w + eset->widgets.listbox_text_margin.y;
	}
	else {
		has_scroll_bar = false;
		right_margin = eset->widgets.listbox_text_margin.y;
	}

	// cache all item text
	for (size_t i = 0; i < items.size(); ++i) {
		items[i].label.setVAlign(LabelInfo::VALIGN_CENTER);
		if (disable_text_trim)
			items[i].label.setText(items[i].value);
		else
			items[i].label.setText(font->trimTextToWidth(items[i].value, pos.w-right_margin-padding, FontEngine::USE_ELLIPSIS, 0));

		items[i].label.setHidden(i < static_cast<size_t>(cursor) || i >= static_cast<size_t>(cursor) + rows.size());
	}

	// Update each row's hitbox and label
	for(unsigned i=0; i<rows.size(); i++) {
		rows[i].x = pos.x;
		rows[i].y = (pos.h*i)+pos.y;
		if (has_scroll_bar) {
			rows[i].w = pos.w - pos_scroll.w;
		}
		else {
			rows[i].w = pos.w;
		}
		rows[i].h = pos.h;


		if (i+cursor < items.size()) {
			items[i+cursor].label.setPos(rows[i].x + eset->widgets.listbox_text_margin.x, rows[i].y + (rows[i].h/2));
			if (items[i+cursor].selected) {
				items[i+cursor].label.setColor(font->getColor(FontEngine::COLOR_WIDGET_NORMAL));
			}
			else {
				items[i+cursor].label.setColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED));
			}
		}
	}

}

bool WidgetListBox::getNext() {
	if (can_select) {
		if (items.size() < 1)
			return false;

		int sel = getSelected();
		if (sel == -1)
			sel = cursor-1;

		for (size_t i = 0; i < items.size(); ++i) {
			items[i].selected = false;
		}

		if(sel == static_cast<int>(items.size())-1) {
			items[0].selected = true;
			while (getSelected() < cursor) {
				scrollUp();
			}
		}
		else {
			items[sel+1].selected = true;
			while (getSelected() > cursor + static_cast<int>(rows.size()) - 1) {
				scrollDown();
			}
		}
	}
	else {
		scrollDown();
	}

	return true;
}

bool WidgetListBox::getPrev() {
	if (can_select) {
		if (items.size() < 1)
			return false;

		int sel = getSelected();
		if (sel == -1)
			sel = cursor;

		for (size_t i = 0; i < items.size(); ++i) {
			items[i].selected = false;
		}

		if(sel == 0) {
			items[items.size()-1].selected = true;
			while (getSelected() > cursor + static_cast<int>(rows.size()) - 1) {
				scrollDown();
			}
		}
		else {
			items[sel-1].selected = true;
			while (getSelected() < cursor) {
				scrollUp();
			}
		}
	}
	else {
		scrollUp();
	}

	return true;
}

void WidgetListBox::defocus() {
	Widget::defocus();

	int sel = getSelected();
	if (!can_select && sel != -1) {
		items[sel].selected = false;
	}

}

void WidgetListBox::activate() {
	if (!inpt->usingMouse()) {
		show_tooltip_for_selected = !show_tooltip_for_selected;
	}
}

void WidgetListBox::select(int index) {
	if (items.empty())
		return;

	int sel = getSelected();
	if (!multi_select && sel != -1) {
		items[sel].selected = false;
	}
	items[index].selected = true;
}

bool WidgetListBox::isSelected(int index) {
	if (items.empty())
		return false;

	return items[index].selected;
}

/*
 * Change the number of visible rows
 */
void WidgetListBox::setHeight(int new_size) {
	if (new_size < 2)
		new_size = 2;

	rows.clear();

	rows.resize(static_cast<size_t>(new_size));

	refresh();
}

void WidgetListBox::sort() {
	std::sort(items.begin(), items.end());
}

WidgetListBox::~WidgetListBox() {
	delete listboxs;
	delete scrollbar;
}

