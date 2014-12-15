/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Justin Jacobs
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

#include "SharedResources.h"
#include "WidgetLabel.h"
#include "WidgetListBox.h"
#include "WidgetScrollBar.h"
#include "WidgetTooltip.h"

WidgetListBox::WidgetListBox(int height, const std::string& _fileName)
	: Widget()
	, fileName(_fileName)
	, listboxs(NULL)
	, cursor(0)
	, has_scroll_bar(false)
	, any_selected(false)
	, vlabels(std::vector<WidgetLabel>(height,WidgetLabel()))
	, rows(std::vector<Rect>(height,Rect()))
	, tip(new WidgetTooltip())
	, scrollbar(new WidgetScrollBar())
	, color_normal(font->getColor("widget_normal"))
	, color_disabled(font->getColor("widget_disabled"))
	, pos_scroll()
	, pressed(false)
	, multi_select(false)
	, can_deselect(true)
	, can_select(true)
	, scrollbar_offset(0) {

	// load ListBox images
	Image *graphics;
	graphics = render_device->loadImage(fileName, "Couldn't load image", true);
	if (graphics) {
		listboxs = graphics->createSprite();
		pos.w = listboxs->getGraphicsWidth();
		pos.h = (listboxs->getGraphicsHeight() / 3); // height of one item
		graphics->unref();
	}
}

bool WidgetListBox::checkClick() {
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}

/**
 * Sets and releases the "pressed" visual state of the ListBox
 * If press and release, activate (return true)
 */
bool WidgetListBox::checkClick(int x, int y) {

	Point mouse(x, y);

	refresh();

	// check scroll wheel
	Rect scroll_area;
	scroll_area.x = rows[0].x;
	scroll_area.y = rows[0].y;
	scroll_area.w = rows[0].w;
	scroll_area.h = rows[0].h * rows.size();

	if (isWithin(scroll_area,mouse)) {
		inpt->lock_scroll = true;
		if (inpt->scroll_up) scrollUp();
		if (inpt->scroll_down) scrollDown();
	}
	else {
		inpt->lock_scroll = false;
	}

	// check ScrollBar clicks
	if (has_scroll_bar) {
		switch (scrollbar->checkClick(mouse.x,mouse.y)) {
			case 1:
				scrollUp();
				break;
			case 2:
				scrollDown();
				break;
			case 3:
				cursor = scrollbar->getValue();
				refresh();
				break;
			default:
				break;
		}
	}

	// main ListBox already in use, new click not allowed
	if (inpt->lock[MAIN1]) return false;

	// main click released, so the ListBox state goes back to unpressed
	if (pressed && !inpt->lock[MAIN1] && can_select) {
		pressed = false;

		for(unsigned i=0; i<rows.size(); i++) {
			if (i<values.size()) {
				if (isWithin(rows[i], mouse) && values[i+cursor] != "") {
					// deselect other options if multi-select is disabled
					if (!multi_select) {
						for (unsigned j=0; j<values.size(); j++) {
							if (j!=i+cursor)
								selected[j] = false;
						}
					}
					// activate upon release
					if (selected[i+cursor]) {
						if (can_deselect) selected[i+cursor] = false;
					}
					else {
						selected[i+cursor] = true;
					}
					refresh();
					return true;
				}
			}
		}
	}

	pressed = false;

	// detect new click
	if (inpt->pressing[MAIN1]) {
		for (unsigned i=0; i<rows.size(); i++) {
			if (isWithin(rows[i], mouse)) {

				inpt->lock[MAIN1] = true;
				pressed = true;

			}
		}
	}
	return false;

}

/**
 * If mousing-over an item with a tooltip, return that tooltip data.
 *
 * @param mouse The x,y screen coordinates of the mouse cursor
 */
TooltipData WidgetListBox::checkTooltip(Point mouse) {
	TooltipData _tip;

	for(unsigned i=0; i<rows.size(); i++) {
		if (i<values.size()) {
			if (isWithin(rows[i], mouse) && tooltips[i+cursor] != "") {
				_tip.addText(tooltips[i+cursor]);
				break;
			}
		}
	}

	return _tip;
}

/**
 * Add a new value (with tooltip) to the list
 */
void WidgetListBox::append(std::string value, std::string tooltip) {
	values.push_back(value);
	tooltips.push_back(tooltip);
	selected.push_back(false);
	refresh();
}

/**
 * Remove a value from the list
 */
void WidgetListBox::remove(int index) {
	values.erase(values.begin()+index);
	tooltips.erase(tooltips.begin()+index);
	selected.erase(selected.begin()+index);
	scrollUp();
	refresh();
}

/*
 * Clear the list
 */
void WidgetListBox::clear() {
	values.clear();
	tooltips.clear();
	selected.clear();
	refresh();
}

/*
 * Move an item up on the list
 */
void WidgetListBox::shiftUp() {
	any_selected = false;
	if (!selected[0]) {
		for (unsigned i=1; i < values.size(); i++) {
			if (selected[i]) {
				any_selected = true;
				bool tmp_selected = selected[i];
				std::string tmp_value = values[i];
				std::string tmp_tooltip = tooltips[i];

				selected[i] = selected[i-1];
				values[i] = values[i-1];
				tooltips[i] = tooltips[i-1];

				selected[i-1] = tmp_selected;
				values[i-1] = tmp_value;
				tooltips[i-1] = tmp_tooltip;
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
	if (!selected[values.size()-1]) {
		for (int i=values.size()-2; i >= 0; i--) {
			if (selected[i]) {
				any_selected = true;
				bool tmp_selected = selected[i];
				std::string tmp_value = values[i];
				std::string tmp_tooltip = tooltips[i];

				selected[i] = selected[i+1];
				values[i] = values[i+1];
				tooltips[i] = tooltips[i+1];

				selected[i+1] = tmp_selected;
				values[i+1] = tmp_value;
				tooltips[i+1] = tmp_tooltip;
			}
		}
		if (any_selected) {
			scrollDown();
		}
	}
}

int WidgetListBox::getSelected() {
	// return the first selected value
	for (unsigned i=0; i<values.size(); i++) {
		if (selected[i]) return i;
	}
	return -1; // nothing is selected
}

std::string WidgetListBox::getValue() {
	for (unsigned i=0; i<values.size(); i++) {
		if (selected[i]) return values[i];
	}
	return "";
}

/*
 * Get the item name at a specific index
 */
std::string WidgetListBox::getValue(int index) {
	return values[index];
}

/*
 * Get the item tooltip at a specific index
 */
std::string WidgetListBox::getTooltip(int index) {
	return tooltips[index];
}

/*
 * Get the amount of ListBox items
 */
int WidgetListBox::getSize() {
	return values.size();
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
	if (cursor+rows.size() < values.size())
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
			listboxs->setClip(src);
			listboxs->setDest(rows[i]);
			render_device->render(listboxs);
		}

		if (i<values.size()) {
			vlabels[i].local_frame = local_frame;
			vlabels[i].local_offset = local_offset;
			vlabels[i].render();
		}
	}

	if (in_focus) {
		Point topLeft;
		Point bottomRight;
		Uint32 color;

		topLeft.x = rows[0].x + local_frame.x - local_offset.x;
		topLeft.y = rows[0].y + local_frame.y - local_offset.y;
		bottomRight.x = rows[rows.size() - 1].x + rows[0].w + local_frame.x - local_offset.x;
		bottomRight.y = rows[rows.size() - 1].y + rows[0].h + local_frame.y - local_offset.y;
		color = render_device->MapRGB(255,248,220);

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

	std::string temp;
	int right_margin = 0;

	// Update the scrollbar
	if (values.size() > rows.size()) {
		has_scroll_bar = true;
		pos_scroll.x = pos.x+pos.w-scrollbar->pos_up.w-scrollbar_offset;
		pos_scroll.y = pos.y+scrollbar_offset;
		pos_scroll.w = scrollbar->pos_up.w;
		pos_scroll.h = (pos.h*rows.size())-scrollbar->pos_down.h-(scrollbar_offset*2);
		scrollbar->refresh(pos_scroll.x, pos_scroll.y, pos_scroll.h, cursor, values.size()-rows.size());
		right_margin = scrollbar->pos_knob.w + 8;
	}
	else {
		has_scroll_bar = false;
		right_margin = 8;
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

		int font_x = rows[i].x + 8;
		int font_y = rows[i].y + (rows[i].h/2);

		if (i+cursor < values.size()) {
			// gets the maxiumum value length that can fit in the listbox
			// maybe there is a better way to do this?
			unsigned int max_length = (unsigned int)(pos.w-right_margin)/font->calc_width("X");
			if (font->calc_width(values[i+cursor]) > pos.w-right_margin) {
				temp = values[i+cursor].substr(0,max_length);
				temp.append("...");
			}
			else {
				temp = values[i+cursor];
			}
		}

		if(i+cursor < values.size() && selected[i+cursor]) {
			vlabels[i].set(font_x, font_y, JUSTIFY_LEFT, VALIGN_CENTER, temp, color_normal);
		}
		else if (i < values.size()) {
			vlabels[i].set(font_x, font_y, JUSTIFY_LEFT, VALIGN_CENTER, temp, color_disabled);
		}
	}

}

bool WidgetListBox::getNext() {
	if (values.size() < 1) return false;

	int sel = getSelected();
	if (sel != -1) selected[sel] = false;

	if(sel == (int)values.size()-1) {
		selected[0] = true;
		while (getSelected() < cursor) scrollUp();
	}
	else {
		selected[sel+1] = true;
		while (getSelected() > cursor+(int)rows.size()-1) scrollDown();
	}

	return true;
}

bool WidgetListBox::getPrev() {
	if (values.size() < 1) return false;

	int sel = getSelected();
	if (sel == -1) sel = 0;
	selected[sel] = false;

	if(sel == 0) {
		selected[values.size()-1] = true;
		while (getSelected() > cursor+(int)rows.size()-1) scrollDown();
	}
	else {
		selected[sel-1] = true;
		while (getSelected() < cursor) scrollUp();
	}

	return true;
}

WidgetListBox::~WidgetListBox() {
	if (listboxs) delete listboxs;
	if (tip) delete tip;
	if (scrollbar) delete scrollbar;
}

