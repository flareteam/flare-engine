/*
Copyright © 2018 Justin Jacobs

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
 * class WidgetHorizontalList
 */

#include "EngineSettings.h"
#include "FontEngine.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "TooltipManager.h"
#include "WidgetButton.h"
#include "WidgetHorizontalList.h"

const std::string WidgetHorizontalList::DEFAULT_FILE_LEFT = "images/menus/buttons/left.png";
const std::string WidgetHorizontalList::DEFAULT_FILE_RIGHT = "images/menus/buttons/right.png";

WidgetHorizontalList::WidgetHorizontalList()
	: Widget()
	, button_left(new WidgetButton(DEFAULT_FILE_LEFT))
	, button_right(new WidgetButton(DEFAULT_FILE_RIGHT))
	, cursor(0)
	, changed_without_mouse(false)
	, action_triggered(false)
	, activated(false)
	, enabled(true)
	, has_action(false)
	, max_visible_actions(2)
{
	// we want the dimensions of a regular button, so load a temporary one here
	WidgetButton *temp = new WidgetButton(WidgetButton::DEFAULT_FILE);
	if (temp) {
		action_button_size.x = temp->pos.w;
		action_button_size.y = temp->pos.h;
		delete temp;
	}

	refresh();
}

// void WidgetHorizontalList::activate() {
// }

void WidgetHorizontalList::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);
	refresh();
}

bool WidgetHorizontalList::checkClick() {
	return checkClickAt(inpt->mouse.x,inpt->mouse.y);
}

bool WidgetHorizontalList::checkClickAt(int x, int y) {
	// enable_tablist_nav = enabled;

	Point mouse(x,y);

	checkTooltip(mouse);

	if (button_left->checkClickAt(mouse.x, mouse.y)) {
		scrollLeft();
		return true;
	}
	else if (button_right->checkClickAt(mouse.x, mouse.y)) {
		scrollRight();
		return true;
	}
	else if (changed_without_mouse) {
		// getNext() or getPrev() was used to change the slider, so treat it as a "click"
		changed_without_mouse = false;
		return true;
	}
	else if (has_action && activated) {
		activated = false;
		list_items[cursor].button->activate();
	}
	else if (has_action) {
		size_t start,end;
		getVisibleButtonRange(start, end);
		for (size_t i = start; i < end; ++i) {
			if (list_items[i].button->checkClickAt(mouse.x, mouse.y)) {
				cursor = static_cast<unsigned>(i);
				action_triggered = true;
				return true;
			}
		}
	}

	return false;
}

bool WidgetHorizontalList::checkAction() {
	if (action_triggered) {
		action_triggered = false;
		return true;
	}
	return false;
}

void WidgetHorizontalList::activate() {
	activated = true;
}

void WidgetHorizontalList::render() {
	button_left->local_frame = local_frame;
	button_left->local_offset = local_offset;

	button_right->local_frame = local_frame;
	button_right->local_offset = local_offset;

	if (!multipleActionsVisible()) {
		button_left->render();
		button_right->render();
	}

	if (has_action) {
		size_t start,end;
		getVisibleButtonRange(start, end);
		for (size_t i = start; i < end; ++i) {
			list_items[i].button->local_frame = local_frame;
			list_items[i].button->local_offset = local_offset;
			list_items[i].button->render();
		}
	}
	else {
		// render label
		label.local_frame = local_frame;
		label.local_offset = local_offset;
		label.render();
	}

	if (in_focus && (!has_action || !enabled)) {
		Point topLeft;
		Point bottomRight;

		topLeft.x = pos.x + local_frame.x - local_offset.x;
		topLeft.y = pos.y + local_frame.y - local_offset.y;
		bottomRight.x = topLeft.x + pos.w;
		bottomRight.y = topLeft.y + pos.h;

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
	else if (in_focus && has_action && enabled) {
		for (size_t i = 0; i < list_items.size(); ++i) {
			list_items[i].button->in_focus = (cursor == i ? true : false);
		}
	}
	else {
		if (has_action) {
			for (size_t i = 0; i < list_items.size(); ++i) {
				list_items[i].button->in_focus = false;
			}
		}
	}
}

void WidgetHorizontalList::refresh() {
	scroll_type = multipleActionsVisible() ? SCROLL_VERTICAL : SCROLL_HORIZONTAL;

	int content_width = eset->widgets.horizontal_list_text_width;
	bool is_enabled = !isEmpty() && enabled;

	button_left->enabled = is_enabled && !multipleActionsVisible();
	button_right->enabled = is_enabled && !multipleActionsVisible();

	button_left->setPos(pos.x, pos.y);

	if (has_action) {
		content_width = action_button_size.x;

		for (size_t i = 0; i < list_items.size(); ++i) {
			list_items[i].button->enabled = is_enabled;

			int y_offset = pos.y + (button_left->pos.h / 2);
			if (multipleActionsVisible()) {
				int action_count = static_cast<int>(std::min(max_visible_actions, list_items.size()));
				y_offset += (-action_button_size.y * action_count / 2) + action_button_size.y * static_cast<int>(i);
			}
			else {
				y_offset += (-action_button_size.y / 2);
			}

			list_items[i].button->setPos(pos.x + button_left->pos.w, y_offset);

			list_items[i].button->setLabel(list_items[i].value);
			if (cursor < list_items.size()) {
				list_items[i].button->tooltip = list_items[i].tooltip;
			}

			pos.h = std::max(button_left->pos.h, list_items[i].button->pos.h);
		}
	}
	else {
		label.setText(getValue());
		label.setPos(pos.x + button_left->pos.w + content_width/2, pos.y + button_left->pos.h / 2);
		label.setMaxWidth(content_width);
		label.setJustify(FontEngine::JUSTIFY_CENTER);
		label.setVAlign(LabelInfo::VALIGN_CENTER);
		label.setColor(is_enabled ? font->getColor(FontEngine::COLOR_WIDGET_NORMAL) : font->getColor(FontEngine::COLOR_WIDGET_DISABLED));

		pos.h = std::max(button_left->pos.h, label.getBounds()->h);

		tooltip_area.x = pos.x + button_left->pos.w;
		tooltip_area.y = std::min(pos.y, label.getBounds()->y);
		tooltip_area.w = content_width;
		tooltip_area.h = std::max(button_left->pos.h, label.getBounds()->h);
	}

	button_right->setPos(pos.x + button_left->pos.w + content_width, pos.y);
	pos.w = button_left->pos.w + button_right->pos.w + content_width;
}

void WidgetHorizontalList::checkTooltip(const Point& mouse) {
	// WidgetButton will handle the tooltip if the action button is enabled
	if (has_action)
		return;

	if (isEmpty())
		return;

	if (inpt->usingMouse() && Utils::isWithinRect(tooltip_area, mouse) && !list_items[cursor].tooltip.empty()) {
		TooltipData tip_data;
		tip_data.addText(list_items[cursor].tooltip);
		Point new_mouse(mouse.x + local_frame.x - local_offset.x, mouse.y + local_frame.y - local_offset.y);
		tooltipm->push(tip_data, new_mouse, TooltipData::STYLE_FLOAT);
	}
}

void WidgetHorizontalList::append(const std::string& value, const std::string& tooltip) {
	HListItem hli;
	hli.value = value;
	hli.tooltip = tooltip;

	// these get deleted when calling clear()
	hli.button = new WidgetButton(WidgetButton::DEFAULT_FILE);

	list_items.push_back(hli);
}

void WidgetHorizontalList::clear() {
	for (size_t i = 0; i < list_items.size(); ++i) {
		delete list_items[i].button;
	}
	list_items.clear();
	cursor = 0;
}

std::string WidgetHorizontalList::getValue() {
	if (cursor < getSize()) {
		return list_items[cursor].value;
	}

	return "";
}

void WidgetHorizontalList::setValue(unsigned index, const std::string& value) {
	if (index < getSize()) {
		list_items[index].value = value;
	}
}

unsigned WidgetHorizontalList::getSelected() {
	if (cursor < getSize())
		return cursor;
	else
		return getSize();
}

unsigned WidgetHorizontalList::getSize() {
	return static_cast<unsigned>(list_items.size());
}

bool WidgetHorizontalList::isEmpty() {
	return list_items.empty();
}

void WidgetHorizontalList::scrollLeft() {
	if (isEmpty())
		return;

	if (cursor == 0)
		cursor = getSize() - 1;
	else
		cursor--;

	refresh();
}

void WidgetHorizontalList::select(unsigned index) {
	if (isEmpty())
		return;

	if (index < getSize())
		cursor = index;

	refresh();
}

void WidgetHorizontalList::scrollRight() {
	if (isEmpty())
		return;

	if (cursor+1 >= getSize())
		cursor = 0;
	else
		cursor++;

	refresh();
}

bool WidgetHorizontalList::getPrev() {
	if (!isEmpty() && enabled) {
		scrollLeft();
		changed_without_mouse = true;
	}
	return true;
}

bool WidgetHorizontalList::getNext() {
	if (!isEmpty() && enabled) {
		scrollRight();
		changed_without_mouse = true;
	}
	return true;
}

bool WidgetHorizontalList::multipleActionsVisible() {
	return has_action && max_visible_actions > 1 && max_visible_actions >= list_items.size();
}

void WidgetHorizontalList::getVisibleButtonRange(size_t& start, size_t& end) {
	if (!multipleActionsVisible()) {
		start = cursor;
		end = cursor+1;
	}
	else {
		start = 0;
		end = list_items.size();
	}
}

WidgetHorizontalList::~WidgetHorizontalList() {
	clear();
	delete button_left;
	delete button_right;
}

