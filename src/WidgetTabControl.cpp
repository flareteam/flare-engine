/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2016 Justin Jacobs

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

#include "EngineSettings.h"
#include "FontEngine.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetTabControl.h"


WidgetTabControl::WidgetTabControl()
	: active_tab_surface(NULL)
	, inactive_tab_surface(NULL)
	, active_tab(0)
	, lock_main1(false)
	, dragging(false)
	, button_prev(new WidgetButton("images/menus/buttons/left.png"))
	, button_next(new WidgetButton("images/menus/buttons/right.png"))
	, show_buttons(false)
{

	loadGraphics();

	scroll_type = SCROLL_HORIZONTAL;
}

WidgetTabControl::~WidgetTabControl() {
	if (active_tab_surface)
		delete active_tab_surface;
	if (inactive_tab_surface)
		delete inactive_tab_surface;

	delete button_prev;
	delete button_next;
}

/**
 * Initialize a tab at a given index. A new tab will be allocated if it does not exist.
 */
void WidgetTabControl::setupTab(unsigned index, const std::string& title, TabList* tl) {
	if (index+1 >titles.size()) {
		titles.resize(index+1);
		tabs.resize(index+1);
		active_labels.resize(index+1);
		inactive_labels.resize(index+1);
		enabled.resize(index+1);
		tablists.resize(index+1, NULL);
	}

	titles[index] = title;
	enabled[index] = true;
	tablists[index] = tl;
}

/**
 * Returns the index of the open tab.
 *
 * For example, if the first tab is open, it will return 0.
 */
int WidgetTabControl::getActiveTab() {
	return active_tab;
}

/**
 * Sets the active tab to a given index.
 */
void WidgetTabControl::setActiveTab(unsigned tab) {
	if (tab > tabs.size())
		tab = 0;
	else if (tab == tabs.size())
		tab = static_cast<unsigned>(tabs.size()-1);

	// Set the tab. If the specified tab is not enabled, get the first enabled tab.
	bool found_tab = false;
	for (unsigned i = tab; i < tabs.size(); ++i) {
		if (enabled[i]) {
			active_tab = i;
			found_tab = true;
			break;
		}
	}
	if (!found_tab) {
		for (unsigned i = 0; i < tab; ++i) {
			if (enabled[i]) {
				active_tab = i;
				found_tab = true;
				break;
			}
		}
	}

	if (!found_tab) {
		// no enabled tabs, just return what we started with
		active_tab = tab;
	}

	for (unsigned i = 0; i < tabs.size(); ++i) {
		if (tablists[i] && i != active_tab) {
			tablists[i]->lock();
			tablists[i]->defocus();
		}
	}

	if (tablists[active_tab]) {
		tablists[active_tab]->unlock();
	}
}

/**
 * Define the position and size of the tab control area and the child tabs (including text).
 * Typically called when running window/menu align() rountines.
 *
 * @param x       X coordinate of the top-left corner of the widget.
 * @param y       Y coordinate of the top-left corner of the widget.
 * @param w       The maximum width of the area allowed for tabs. If this is exceeded, a single tab is show with navigation buttons instead.
 */
void WidgetTabControl::setMainArea(int x, int y, int w) {
	// Set tabs area.
	tabs_area.x = x;
	tabs_area.y = y;
	tabs_area.w = 0;
	tabs_area.h = getTabHeight();

	show_buttons = false;

	int x_offset = tabs_area.x;

	// update individual tabs
	for (size_t i = 0; i < tabs.size(); ++i) {
		tabs[i].y = tabs_area.y;
		tabs[i].h = tabs_area.h;

		tabs[i].x = x_offset;

		active_labels[i].setPos(tabs[i].x + eset->widgets.tab_padding.x + eset->widgets.tab_text_padding, tabs[i].y + tabs[i].h/2 + eset->widgets.tab_padding.y);
		active_labels[i].setVAlign(LabelInfo::VALIGN_CENTER);
		active_labels[i].setText(titles[i]);
		active_labels[i].setColor(font->getColor(FontEngine::COLOR_WIDGET_NORMAL));

		inactive_labels[i].setPos(tabs[i].x + eset->widgets.tab_padding.x + eset->widgets.tab_text_padding, tabs[i].y + tabs[i].h/2 + eset->widgets.tab_padding.y);
		inactive_labels[i].setVAlign(LabelInfo::VALIGN_CENTER);
		inactive_labels[i].setText(titles[i]);
		inactive_labels[i].setColor(font->getColor(FontEngine::COLOR_WIDGET_DISABLED));

		if (enabled[i]) {
			tabs[i].w = active_labels[i].getBounds()->w + (eset->widgets.tab_padding.x * 2) + (eset->widgets.tab_text_padding * 2);
			tabs_area.w += tabs[i].w;
			x_offset += tabs[i].w;
		}
	}

	if (tabs_area.w > w || show_buttons) {
		show_buttons = true;

		int between_buttons = w - button_prev->pos.w - button_next->pos.w;

		// only one tab will be shown at a time, so center all the tabs between the buttons
		for (size_t i = 0; i < tabs.size(); ++i) {
			tabs[i].x = tabs_area.x + button_prev->pos.w + ((between_buttons - tabs[i].w) / 2);
			active_labels[i].setPos(tabs[i].x + eset->widgets.tab_padding.x + eset->widgets.tab_text_padding, tabs[i].y + tabs[i].h/2 + eset->widgets.tab_padding.y);
			inactive_labels[i].setPos(tabs[i].x + eset->widgets.tab_padding.x + eset->widgets.tab_text_padding, tabs[i].y + tabs[i].h/2 + eset->widgets.tab_padding.y);
		}
	}

	if (!enabled[active_tab])
		getNext();

	int button_y_offset = tabs_area.y + ((tabs_area.h - button_prev->pos.h) / 2);
	button_prev->setPos(tabs_area.x, button_y_offset);
	button_next->setPos(tabs_area.x + w - button_next->pos.w, button_y_offset);
}

/**
 * Load the graphics for the control.
 */
void WidgetTabControl::loadGraphics() {
	Image *graphics;
	graphics = render_device->loadImage("images/menus/tab_active.png", RenderDevice::ERROR_EXIT);
	if (graphics) {
		active_tab_surface = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/tab_inactive.png", RenderDevice::ERROR_EXIT);
	if (graphics) {
		inactive_tab_surface = graphics->createSprite();
		graphics->unref();
	}
}

void WidgetTabControl::logic() {
	logic(inpt->mouse.x,inpt->mouse.y);
}

/**
 * Performs one frame of logic.
 *
 * It basically checks if it was clicked on the header, and if so changes the active tab.
 */
void WidgetTabControl::logic(int x, int y) {
	Point mouse(x, y);
	if (show_buttons) {
		if (enabled[0]) {
			button_prev->enabled = (active_tab > 0);
		}
		else {
			button_prev->enabled = (active_tab > getNextEnabledTab(0));
		}

		unsigned end_tab = static_cast<unsigned>(tabs.size() - 1);
		if (enabled[end_tab]) {
			button_next->enabled = (active_tab < end_tab);
		}
		else {
			button_next->enabled = (active_tab < getPrevEnabledTab(end_tab));
		}

		if (button_prev->checkClickAt(mouse.x, mouse.y)) {
			getPrev();
		}
		else if (button_next->checkClickAt(mouse.x, mouse.y)) {
			getNext();
		}
	}
	else {
		// If the click was in the tabs area;
		if (Utils::isWithinRect(tabs_area, mouse) && (!lock_main1 || dragging)) {
			lock_main1 = false;
			dragging = false;

			if (inpt->pressing[Input::MAIN1]) {
				inpt->lock[Input::MAIN1] = true;
				dragging = true;

				// Mark the clicked tab as active_tab.
				for (unsigned i=0; i<tabs.size(); i++) {
					if(Utils::isWithinRect(tabs[i], mouse) && enabled[i]) {
						active_tab = i;
						setActiveTab(i);
						break;
						// return;
					}
				}
			}
		}
		else {
			lock_main1 = inpt->pressing[Input::MAIN1];
		}
		if (!inpt->pressing[Input::MAIN1]) {
			dragging = false;
		}
	}

	if (tablists[active_tab] && tablists[active_tab]->getCurrent() != -1) {
		if (inpt->pressing[Input::MENU_PAGE_NEXT] && !inpt->lock[Input::MENU_PAGE_NEXT] && active_tab < tabs.size()) {
			for (unsigned i = active_tab + 1; i < tabs.size(); ++i) {
				if (enabled[i] && tablists[i]) {
					inpt->lock[Input::MENU_PAGE_NEXT] = true;
					tablists[active_tab]->defocus();
					tablists[active_tab]->lock();
					tablists[i]->unlock();
					tablists[i]->getNext(!TabList::GET_INNER, TabList::WIDGET_SELECT_AUTO);
					active_tab = i;
					break;
				}
			}
		}
		else if (inpt->pressing[Input::MENU_PAGE_PREV] && !inpt->lock[Input::MENU_PAGE_PREV] && active_tab > 0) {
			for (unsigned i = active_tab; i > 0; --i) {
				if (enabled[i-1] && tablists[i-1]) {
					inpt->lock[Input::MENU_PAGE_PREV] = true;
					tablists[active_tab]->defocus();
					tablists[active_tab]->lock();
					tablists[i-1]->unlock();
					tablists[i-1]->getPrev(!TabList::GET_INNER, TabList::WIDGET_SELECT_AUTO);
					active_tab = i-1;
					break;
				}
			}
		}
	}
}

/**
 * Renders the widget.
 *
 * Remember to render then on top of it the actual content of the {@link getActiveTab() active tab}.
 */
void WidgetTabControl::render() {
	for (unsigned i=0; i<tabs.size(); i++) {
		renderTab(i);
	}

	if (show_buttons) {
		button_prev->render();
		button_next->render();
	}

	// draw selection rectangle
	if (in_focus) {
		Point topLeft;
		Point bottomRight;

		topLeft.x = tabs[active_tab].x;
		topLeft.y = tabs[active_tab].y;
		bottomRight.x = topLeft.x + tabs[active_tab].w;
		bottomRight.y = topLeft.y + tabs[active_tab].h;

		render_device->drawRectangleCorners(eset->widgets.selection_rect_corner_size, topLeft, bottomRight, eset->widgets.selection_rect_color);
	}
}

/**
 * Renders the given tab on the widget header.
 */
void WidgetTabControl::renderTab(unsigned number) {
	if (!enabled[number] || (show_buttons && number != active_tab))
		return;

	unsigned i = number;
	Rect src;
	Rect dest;

	// Draw tab’s background.
	int gfx_width = active_tab_surface->getGraphicsWidth();
	int width_to_render = tabs[i].w - eset->widgets.tab_padding.x; // don't draw the right edge yet
	int render_cursor = 0;

	src.y = 0;
	src.h = tabs[i].h;
	dest.y = tabs[i].y;

	// repeat the middle part of the image for long tabs
	// src.x and dest.x are assigned here
	while (render_cursor < width_to_render) {
		dest.x = tabs[i].x + render_cursor;
		if (render_cursor == 0) {
			// left edge + middle
			src.x = 0;
			src.w = tabs[i].w - eset->widgets.tab_padding.x;

			if (src.w > gfx_width - eset->widgets.tab_padding.x)
				src.w = gfx_width - eset->widgets.tab_padding.x;
		}
		else {
			// only middle
			src.x = eset->widgets.tab_padding.x;
			src.w = tabs[i].w - (eset->widgets.tab_padding.x * 2);

			if (src.w > gfx_width - (eset->widgets.tab_padding.x * 2))
				src.w = gfx_width - (eset->widgets.tab_padding.x * 2);
		}

		render_cursor += src.w;

		if (render_cursor > tabs[i].w)
			src.w = tabs[i].w - (render_cursor - src.w);

		if (i == active_tab) {
			active_tab_surface->setClipFromRect(src);
			active_tab_surface->setDestFromRect(dest);
			render_device->render(active_tab_surface);
		}
		else {
			inactive_tab_surface->setClipFromRect(src);
			inactive_tab_surface->setDestFromRect(dest);
			render_device->render(inactive_tab_surface);
		}
	}

	// Draw tab’s right edge.
	src.x = active_tab_surface->getGraphicsWidth() - eset->widgets.tab_padding.x;
	src.w = eset->widgets.tab_padding.x;
	dest.x = tabs[i].x + tabs[i].w - eset->widgets.tab_padding.x;

	if (i == active_tab) {
		active_tab_surface->setClipFromRect(src);
		active_tab_surface->setDestFromRect(dest);
		render_device->render(active_tab_surface);
	}
	else {
		inactive_tab_surface->setClipFromRect(src);
		inactive_tab_surface->setDestFromRect(dest);
		render_device->render(inactive_tab_surface);
	}

	// Render labels
	if (i == active_tab) {
		active_labels[i].render();
	}
	else {
		inactive_labels[i].render();
	}
}

bool WidgetTabControl::getNext() {
	setActiveTab(getNextEnabledTab(active_tab));
	return true;
}

bool WidgetTabControl::getPrev() {
	setActiveTab(getPrevEnabledTab(active_tab));
	return true;
}

unsigned WidgetTabControl::getNextEnabledTab(unsigned tab) {
	for (unsigned i = tab+1; i < tabs.size(); ++i) {
		if (enabled[i])
			return i;
	}
	return tab;
}

unsigned WidgetTabControl::getPrevEnabledTab(unsigned tab) {
	for (unsigned i = tab-1; i < tabs.size(); i--) {
		if (enabled[i])
			return i;
	}
	return tab;
}

int WidgetTabControl::getTabHeight() {
	return (active_tab_surface ? active_tab_surface->getGraphicsHeight() : 0);
}

void WidgetTabControl::setEnabled(unsigned index, bool val) {
	if (index >= enabled.size())
		return;

	enabled[index] = val;
}
