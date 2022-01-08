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
	, button_action(new WidgetButton(WidgetButton::DEFAULT_FILE))
	, cursor(0)
	, changed_without_mouse(false)
	, action_triggered(false)
	, activated(false)
	, enabled(true)
	, has_action(false)
{
	scroll_type = SCROLL_HORIZONTAL;
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
	else if (has_action && button_action->checkClickAt(mouse.x, mouse.y)) {
		action_triggered = true;
		return true;
	}
	else if (has_action && activated) {
		activated = false;
		button_action->activate();
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

	button_left->render();
	button_right->render();

	if (has_action) {
		button_action->local_frame = local_frame;
		button_action->local_offset = local_offset;
		button_action->render();
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
			render_device->drawRectangle(topLeft, bottomRight, eset->widgets.selection_rect_color);
		}
	}
	else if (in_focus && has_action && enabled) {
		button_action->in_focus = true;
	}
	else {
		button_action->in_focus = false;
	}
}

void WidgetHorizontalList::refresh() {
	int content_width = eset->widgets.horizontal_list_text_width;
	bool is_enabled = !isEmpty() && enabled;

	button_left->enabled = is_enabled;
	button_right->enabled = is_enabled;
	button_action->enabled = is_enabled;

	button_left->setPos(pos.x, pos.y);

	if (has_action) {
		content_width = button_action->pos.w;

		button_action->setPos(pos.x + button_left->pos.w, pos.y + (button_left->pos.h / 2) - (button_action->pos.h / 2));

		button_action->setLabel(getValue());
		if (cursor < list_items.size()) {
			button_action->tooltip = list_items[cursor].tooltip;
		}

		pos.h = std::max(button_left->pos.h, button_action->pos.h);
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

	list_items.push_back(hli);
}

void WidgetHorizontalList::clear() {
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

WidgetHorizontalList::~WidgetHorizontalList() {
	delete button_left;
	delete button_right;
	delete button_action;
}

