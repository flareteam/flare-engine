/*
Copyright © 2011-2012 Clint Bellanger

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

#include "WidgetTabControl.h"
#include "WidgetLabel.h"

using namespace std;

/**
 * Class constructor.
 *
 * @param amount  Amount of tabs the control will have.
 */
WidgetTabControl::WidgetTabControl(int amount)
	: active_labels(amount)
	, inactive_labels(amount) {

	// Based on given amount:
	tabsAmount = amount;
	titles = new std::string[tabsAmount];
	tabs = new SDL_Rect[tabsAmount];

	// Predefined:
	activeTab = 0;
	tabPadding.x = 8;
	tabPadding.y = 4;

	// Load needed graphics.
	loadGraphics();

	color_normal = font->getColor("widget_normal");
	color_disabled = font->getColor("widget_disabled");
}
/**
 * Class destructor.
 */
WidgetTabControl::~WidgetTabControl() {
	delete[] titles;
	delete[] tabs;
}

/**
 * Sets the title of a tab.
 *
 * @param number        Tab index. For example, 0 for the first tab.
 * @param title         Tab title.
 * @param updateHeader  Whether or not the header should be updated.
 */
void WidgetTabControl::setTabTitle(int number, const std::string& title) {
	titles[number] = title;
}

/**
 * Returns the index of the open tab.
 *
 * For example, if the first tab is open, it will return 0.
 */
int WidgetTabControl::getActiveTab() {
	return activeTab;
}

/**
 * Define the position and size of the tab control.
 *
 * @param x       X coordinate of the top-left corner of the widget.
 * @param y       Y coordinate of the top-left corner of the widget.
 * @param width   Width of the widget.
 * @param height  Height of the widget.
 */
void WidgetTabControl::setMainArea(int x, int y, int width, int height) {
	// Set tabs area.
	tabsArea.x = x;
	tabsArea.y = y;
	tabsArea.w = width;
	tabsArea.h = activeTabSurface.getGraphicsHeight();

	// Set content area.
	contentArea.x = x + 8;
	contentArea.y = y + tabsArea.h+8;
	contentArea.w = width - 16;
	contentArea.h = height - tabsArea.h;
}

/**
 * Updates the areas or the tabs.
 *
 * Use it right after you set the area and tab titles of the tab control.
 */
void WidgetTabControl::updateHeader() {
	for (int i=0; i<tabsAmount; i++) {
		tabs[i].y = tabsArea.y;
		tabs[i].h = tabsArea.h;

		if (i==0) tabs[i].x = tabsArea.x;
		else tabs[i].x = tabs[i-1].x + tabs[i-1].w;

		tabs[i].w = tabPadding.x + font->calc_width(titles[i]) + tabPadding.x;

		active_labels[i].set(
			tabs[i].x + tabPadding.x,
			tabs[i].y + tabs[i].h/2,
			JUSTIFY_LEFT,
			VALIGN_CENTER,
			titles[i],
			color_normal);

		inactive_labels[i].set(
			tabs[i].x + tabPadding.x,
			tabs[i].y + tabs[i].h/2,
			JUSTIFY_LEFT,
			VALIGN_CENTER,
			titles[i],
			color_disabled);
	}
}

/**
 * Load the graphics for the control.
 */
void WidgetTabControl::loadGraphics() {
	activeTabSurface.setGraphics(loadGraphicSurface("images/menus/tab_active.png"));
	inactiveTabSurface.setGraphics(loadGraphicSurface("images/menus/tab_inactive.png"));

	if (activeTabSurface.graphicsIsNull() || inactiveTabSurface.graphicsIsNull()) {
		SDL_Quit();
		exit(1);
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
	// If the click was in the tabs area;
	if(isWithin(tabsArea, mouse) && inpt->pressing[MAIN1]) {
		// Mark the clicked tab as activeTab.
		for (int i=0; i<tabsAmount; i++) {
			if(isWithin(tabs[i], mouse)) {
				activeTab = i;
				return;
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
	for (int i=0; i<tabsAmount; i++) {
		renderTab(i);
	}
}

/**
 * Renders the given tab on the widget header.
 */
void WidgetTabControl::renderTab(int number) {
	int i = number;
	SDL_Rect src;
	SDL_Rect dest;

	// Draw tab’s background.
	src.x = src.y = 0;
	dest.x = tabs[i].x;
	dest.y = tabs[i].y;
	src.w = tabs[i].w;
	src.h = tabs[i].h;

	if (i == activeTab) {
		activeTabSurface.setClip(src);
		activeTabSurface.setDest(dest);
		render_device->render(activeTabSurface);
	}
	else {
		inactiveTabSurface.setClip(src);
		inactiveTabSurface.setDest(dest);
		render_device->render(inactiveTabSurface);
	}

	// Draw tab’s right edge.
	src.x = activeTabSurface.getGraphicsWidth() - tabPadding.x;
	src.w = tabPadding.x;
	dest.x = tabs[i].x + tabs[i].w - tabPadding.x;

	if (i == activeTab) {
		activeTabSurface.setClip(src);
		activeTabSurface.setDest(dest);
		render_device->render(activeTabSurface);
	}
	else {
		inactiveTabSurface.setClip(src);
		inactiveTabSurface.setDest(dest);
		render_device->render(inactiveTabSurface);
	}

	// Render labels
	if (i == activeTab) {
		active_labels[i].render();
	}
	else {
		inactive_labels[i].render();
	}
}

/**
 * Returns the height in pixels of the widget.
 */
SDL_Rect WidgetTabControl::getContentArea() {
	return contentArea;
}
