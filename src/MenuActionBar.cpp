/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
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
 * class MenuActionBar
 *
 * Handles the config, display, and usage of the 0-9 hotkeys, mouse buttons, and menu calls
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "SharedResources.h"
#include "Settings.h"
#include "StatBlock.h"
#include "TooltipData.h"
#include "UtilsParsing.h"
#include "WidgetSlot.h"
#include "WidgetLabel.h"
#include "SharedGameResources.h"

#include <climits>

using namespace std;

MenuActionBar::MenuActionBar(Avatar *_hero)
	: emptyslot(NULL)
	, disabled(NULL)
	, attention(NULL)
	, hero(_hero)
	, updated(false) {
	hero = _hero;

	src.x = 0;
	src.y = 0;
	src.w = ICON_SIZE;
	src.h = ICON_SIZE;
	drag_prev_slot = -1;
	last_mouse.x = 0;
	last_mouse.y = 0;

	clear();

	for (unsigned int i=0; i<16; i++) {
		labels[i] = new WidgetLabel();
	}

	tablist = TabList(HORIZONTAL, ACTIONBAR_BACK, ACTIONBAR_FORWARD, ACTIONBAR);

	for (unsigned int i=0; i<12; i++) {
		slots[i] = new WidgetSlot(-1, ACTIONBAR);
		slots[i]->continuous = true;
		tablist.add(slots[i]);
	}
	for (unsigned int i=0; i<4; i++) {
		menus[i] = new WidgetSlot(-1, ACTIONBAR);
		tablist.add(menus[i]);
	}

	// Read data from config file
	FileParser infile;

	// @CLASS MenuActionBar|Description of menus/actionbar.txt
	if (infile.open("menus/actionbar.txt")) {
		while (infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// numberArea
			// @ATTR slot1|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 1.
			if (infile.key == "slot1") slots[0]->pos = toRect(infile.val);
			// @ATTR slot2|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 2.
			else if (infile.key == "slot2") slots[1]->pos = toRect(infile.val);
			// @ATTR slot3|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 3.
			else if (infile.key == "slot3") slots[2]->pos = toRect(infile.val);
			// @ATTR slot4|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 4.
			else if (infile.key == "slot4") slots[3]->pos = toRect(infile.val);
			// @ATTR slot5|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 5.
			else if (infile.key == "slot5") slots[4]->pos = toRect(infile.val);
			// @ATTR slot6|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 6.
			else if (infile.key == "slot6") slots[5]->pos = toRect(infile.val);
			// @ATTR slot7|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 7.
			else if (infile.key == "slot7") slots[6]->pos = toRect(infile.val);
			// @ATTR slot8|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 8.
			else if (infile.key == "slot8") slots[7]->pos = toRect(infile.val);
			// @ATTR slot9|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 9.
			else if (infile.key == "slot9") slots[8]->pos = toRect(infile.val);
			// @ATTR slot10|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 10.
			else if (infile.key == "slot10") slots[9]->pos = toRect(infile.val);

			// mouseArea
			// @ATTR slot_M1|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 11.
			else if (infile.key == "slot_M1") slots[10]->pos = toRect(infile.val);
			// @ATTR slot_M2|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for slot 12.
			else if (infile.key == "slot_M2") slots[11]->pos = toRect(infile.val);

			// menuArea
			// @ATTR char_menu|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for the Character menu button.
			else if (infile.key == "char_menu") menus[MENU_CHARACTER]->pos = toRect(infile.val);
			// @ATTR inv_menu|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for the Inventory menu button.
			else if (infile.key == "inv_menu") menus[MENU_INVENTORY]->pos = toRect(infile.val);
			// @ATTR powers_menu|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for the Powers menu button.
			else if (infile.key == "powers_menu") menus[MENU_POWERS]->pos = toRect(infile.val);
			// @ATTR log_menu|x (integer), y (integer), w (integer), h (integer)|Position and dimensions for the Log menu button.
			else if (infile.key == "log_menu") menus[MENU_LOG]->pos = toRect(infile.val);

			// @ATTR numberArea|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the area containing slots 1-10.
			else if (infile.key == "numberArea") numberArea = toRect(infile.val);
			// @ATTR mouseArea|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the area containing slots M1 and M2.
			else if (infile.key == "mouseArea") mouseArea = toRect(infile.val);
			// @ATTR menuArea|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the area containing the menu buttons.
			else if (infile.key == "menuArea") menuArea = toRect(infile.val);

			else infile.error("MenuActionBar: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	loadGraphics();

	align();
	alignElements();
}

void MenuActionBar::alignElements() {
	for (unsigned int i=0; i<12; i++) {
		slots[i]->pos.x += window_area.x;
		slots[i]->pos.y += window_area.y;
	}
	for (unsigned int i=0; i<4; i++) {
		menus[i]->pos.x += window_area.x;
		menus[i]->pos.y += window_area.y;
	}

	// screen areas occupied by the three main sections
	numberArea.x += window_area.x;
	mouseArea.x += window_area.x;
	menuArea.x += window_area.x;
	numberArea.y = mouseArea.y = menuArea.y = window_area.y;

	// set keybinding labels
	for (unsigned int i=0; i<10; i++) {
		if (inpt->binding[i+6] < 8)
			labels[i]->set(slots[i]->pos.x+slots[i]->pos.w, slots[i]->pos.y+slots[i]->pos.h-font->getFontHeight(), JUSTIFY_RIGHT, VALIGN_TOP, inpt->mouse_button[inpt->binding[i+6]-1], font->getColor("menu_normal"));
		else
			labels[i]->set(slots[i]->pos.x+slots[i]->pos.w, slots[i]->pos.y+slots[i]->pos.h-font->getFontHeight(), JUSTIFY_RIGHT, VALIGN_TOP, inpt->getKeyName(inpt->binding[i+6]), font->getColor("menu_normal"));
	}
	for (unsigned int i=0; i<2; i++) {
		if (inpt->binding[i+20] < 8)
			labels[i+10]->set(slots[i+10]->pos.x+slots[i+10]->pos.w, slots[i+10]->pos.y+slots[i+10]->pos.h-font->getFontHeight(), JUSTIFY_RIGHT, VALIGN_TOP, inpt->mouse_button[inpt->binding[i+20]-1], font->getColor("menu_normal"));
		else
			labels[i+10]->set(slots[i+10]->pos.x+slots[i+10]->pos.w, slots[i+10]->pos.y+slots[i+10]->pos.h-font->getFontHeight(), JUSTIFY_RIGHT, VALIGN_TOP, inpt->getKeyName(inpt->binding[i+20]), font->getColor("menu_normal"));
	}
	for (unsigned int i=0; i<4; i++) {
		if (inpt->binding[i+16] < 8)
			labels[i+12]->set(menus[i]->pos.x+menus[i]->pos.w, menus[i]->pos.y+menus[i]->pos.h-font->getFontHeight(), JUSTIFY_RIGHT, VALIGN_TOP, inpt->mouse_button[inpt->binding[i+16]-1], font->getColor("menu_normal"));
		else
			labels[i+12]->set(menus[i]->pos.x+menus[i]->pos.w, menus[i]->pos.y+menus[i]->pos.h-font->getFontHeight(), JUSTIFY_RIGHT, VALIGN_TOP, inpt->getKeyName(inpt->binding[i+16]), font->getColor("menu_normal"));
	}
}

void MenuActionBar::clear() {
	// clear action bar
	for (int i=0; i<12; i++) {
		hotkeys[i] = 0;
		hotkeys_temp[i] = 0;
		hotkeys_mod[i] = 0;
		slot_item_count[i] = -1;
		slot_enabled[i] = true;
		locked[i] = false;
	}

	// clear menu notifications
	for (int i=0; i<4; i++)
		requires_attention[i] = false;

}

void MenuActionBar::loadGraphics() {
	Image *graphics;

	setBackground("images/menus/actionbar_trim.png");

	graphics = render_device->loadImage("images/menus/slot_empty.png");
	if (graphics) {
		emptyslot = graphics->createSprite();
		emptyslot->setClip(0,0,ICON_SIZE,ICON_SIZE);
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/disabled.png");
	if (graphics) {
		disabled = graphics->createSprite();
		disabled->setClip(0,0,ICON_SIZE,ICON_SIZE);
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/attention_glow.png");
	if (graphics) {
		attention = graphics->createSprite();
		graphics->unref();
	}
}

// Renders the "needs attention" icon over the appropriate log menu
void MenuActionBar::renderAttention(int menu_id) {
	Rect dest;

	// x-value is 12 hotkeys and 4 empty slots over
	dest.x = window_area.x + (menu_id * ICON_SIZE) + ICON_SIZE*15;
	dest.y = window_area.y+3;
	dest.w = dest.h = ICON_SIZE;
	if (attention) {
		attention->setDest(dest);
		render_device->render(attention);
	}

	// put an asterisk on this icon if in colorblind mode
	if (COLORBLIND) {
		WidgetLabel label;
		label.set(dest.x + 2, dest.y + 2, JUSTIFY_LEFT, VALIGN_TOP, "*", font->getColor("menu_normal"));
		label.render();
	}
}

void MenuActionBar::logic() {
	if (NO_MOUSE) {
		tablist.logic();
	}
}



void MenuActionBar::render() {

	Menu::render();

	// draw hotkeyed icons
	for (int i=0; i<12; i++) {
		if (hotkeys[i] != 0) {
			const Power &power = powers->getPower(hotkeys_mod[i]);
			slot_enabled[i] = (hero->hero_cooldown[hotkeys_mod[i]] == 0)
							  && (slot_item_count[i] != 0)
							  && !hero->stats.effects.stun
							  && hero->stats.alive
							  && hero->stats.canUsePower(power, hotkeys_mod[i]); //see if the slot should be greyed out
			unsigned icon_offset = 0;/* !slot_enabled[i] ? ICON_DISABLED_OFFSET :
								   (hero->activated_powerslot == i ? ICON_HIGHLIGHT_OFFSET : 0); */
			slots[i]->setIcon(power.icon + icon_offset);
			if (slot_item_count[i] > -1) {
				slots[i]->setAmount(slot_item_count[i]);
			}
			else {
				slots[i]->setAmount(0,0);
			}
			slots[i]->render();
		}
		else {
			Rect dest;
			dest.x = slots[i]->pos.x;
			dest.y = slots[i]->pos.y;
			dest.h = dest.w = ICON_SIZE;
			if (emptyslot) {
				emptyslot->setDest(dest);
				render_device->render(emptyslot);
			}
			slots[i]->renderSelection();
		}
	}

	for (int i=0; i<4; i++)
		menus[i]->render();

	renderCooldowns();

	// render log attention notifications
	for (int i=0; i<4; i++)
		if (requires_attention[i])
			renderAttention(i);

	// draw hotkey labels
	if (SHOW_HOTKEYS) {
		for (int i=0; i<16; i++) {
			labels[i]->render();
		}
	}

}

/**
 * Display a notification for any power on cooldown
 * Also displays disabled powers
 */
void MenuActionBar::renderCooldowns() {

	Rect item_src;
	item_src.x = item_src.y = 0;
	item_src.w = item_src.h = ICON_SIZE;

	for (int i=0; i<12; i++) {
		if (!slot_enabled[i]) {

			// Wipe from bottom to top
			if (hero->hero_cooldown[hotkeys_mod[i]] && powers->powers[hotkeys_mod[i]].cooldown) {
				item_src.h = (ICON_SIZE * hero->hero_cooldown[hotkeys_mod[i]]) / powers->powers[hotkeys_mod[i]].cooldown;
			}

			if (disabled) {
				disabled->setClip(item_src);
				disabled->setDest(slots[i]->pos);
				render_device->render(disabled);
			}
			slots[i]->renderSelection();
		}
	}
}

/**
 * On mouseover, show tooltip for buttons
 */
TooltipData MenuActionBar::checkTooltip(Point mouse) {
	TooltipData tip;

	if (isWithin(menus[MENU_CHARACTER]->pos, mouse)) {
		if (COLORBLIND && requires_attention[MENU_CHARACTER])
			tip.addText(msg->get("Character") + " (*)");
		else
			tip.addText(msg->get("Character"));
		return tip;
	}
	if (isWithin(menus[MENU_INVENTORY]->pos, mouse)) {
		if (COLORBLIND && requires_attention[MENU_INVENTORY])
			tip.addText(msg->get("Inventory") + " (*)");
		else
			tip.addText(msg->get("Inventory"));
		return tip;
	}
	if (isWithin(menus[MENU_POWERS]->pos, mouse)) {
		if (COLORBLIND && requires_attention[MENU_POWERS])
			tip.addText(msg->get("Powers") + " (*)");
		else
			tip.addText(msg->get("Powers"));
		return tip;
	}
	if (isWithin(menus[MENU_LOG]->pos, mouse)) {
		if (COLORBLIND && requires_attention[MENU_LOG])
			tip.addText(msg->get("Log") + " (*)");
		else
			tip.addText(msg->get("Log"));
		return tip;
	}
	for (int i=0; i<12; i++) {
		if (hotkeys_mod[i] != 0) {
			if (isWithin(slots[i]->pos, mouse)) {
				tip.addText(powers->powers[hotkeys_mod[i]].name);
			}
		}
	}

	return tip;
}

/**
 * After dragging a power or item onto the action bar, set as new hotkey
 */
void MenuActionBar::drop(Point mouse, int power_index, bool rearranging) {
	for (int i=0; i<12; i++) {
		if (isWithin(slots[i]->pos, mouse)) {
			if (rearranging) {
				if ((locked[i] && !locked[drag_prev_slot]) || (!locked[i] && locked[drag_prev_slot])) {
					locked[i] = !locked[i];
					locked[drag_prev_slot] = !locked[drag_prev_slot];
				}
				hotkeys[drag_prev_slot] = hotkeys[i];
			}
			else if (locked[i]) return;
			hotkeys[i] = power_index;
			updated = true;
			return;
		}
	}
}

/**
 * Return the power to the last clicked on slot
 */
void MenuActionBar::actionReturn(int power_index) {
	drop(last_mouse, power_index, 0);
}

/**
 * CTRL-click a hotkey to clear it
 */
void MenuActionBar::remove(Point mouse) {
	for (int i=0; i<12; i++) {
		if (isWithin(slots[i]->pos, mouse)) {
			if (locked[i]) return;
			hotkeys[i] = 0;
			updated = true;
			return;
		}
	}
}

/**
 * If pressing an action key (keyboard or mouseclick) and the power is enabled,
 * return that power's ID.
 */
int MenuActionBar::checkAction() {
	int current = tablist.getCurrent();

	// check click and hotkey actions
	if ((inpt->pressing[BAR_1] || (inpt->pressing[ACTIONBAR_USE] && current == 0) || (!NO_MOUSE && slots[0]->checkClick() == ACTIVATED)) && slot_enabled[0]) return hotkeys_mod[0];
	if ((inpt->pressing[BAR_2] || (inpt->pressing[ACTIONBAR_USE] && current == 1) || (!NO_MOUSE && slots[1]->checkClick() == ACTIVATED)) && slot_enabled[1]) return hotkeys_mod[1];
	if ((inpt->pressing[BAR_3] || (inpt->pressing[ACTIONBAR_USE] && current == 2) || (!NO_MOUSE && slots[2]->checkClick() == ACTIVATED)) && slot_enabled[2]) return hotkeys_mod[2];
	if ((inpt->pressing[BAR_4] || (inpt->pressing[ACTIONBAR_USE] && current == 3) || (!NO_MOUSE && slots[3]->checkClick() == ACTIVATED)) && slot_enabled[3]) return hotkeys_mod[3];
	if ((inpt->pressing[BAR_5] || (inpt->pressing[ACTIONBAR_USE] && current == 4) || (!NO_MOUSE && slots[4]->checkClick() == ACTIVATED)) && slot_enabled[4]) return hotkeys_mod[4];
	if ((inpt->pressing[BAR_6] || (inpt->pressing[ACTIONBAR_USE] && current == 5) || (!NO_MOUSE && slots[5]->checkClick() == ACTIVATED)) && slot_enabled[5]) return hotkeys_mod[5];
	if ((inpt->pressing[BAR_7] || (inpt->pressing[ACTIONBAR_USE] && current == 6) || (!NO_MOUSE && slots[6]->checkClick() == ACTIVATED)) && slot_enabled[6]) return hotkeys_mod[6];
	if ((inpt->pressing[BAR_8] || (inpt->pressing[ACTIONBAR_USE] && current == 7) || (!NO_MOUSE && slots[7]->checkClick() == ACTIVATED)) && slot_enabled[7]) return hotkeys_mod[7];
	if ((inpt->pressing[BAR_9] || (inpt->pressing[ACTIONBAR_USE] && current == 8) || (!NO_MOUSE && slots[8]->checkClick() == ACTIVATED)) && slot_enabled[8]) return hotkeys_mod[8];
	if ((inpt->pressing[BAR_0] || (inpt->pressing[ACTIONBAR_USE] && current == 9) || (!NO_MOUSE && slots[9]->checkClick() == ACTIVATED)) && slot_enabled[9]) return hotkeys_mod[9];
	if ((inpt->pressing[MAIN1] || (inpt->pressing[ACTIONBAR_USE] && current == 10) || (!NO_MOUSE && slots[10]->checkClick() == ACTIVATED)) && slot_enabled[10] && !inpt->lock[MAIN1]) return hotkeys_mod[10];
	if ((inpt->pressing[MAIN2] || (inpt->pressing[ACTIONBAR_USE] && current == 11) || (!NO_MOUSE && slots[11]->checkClick() == ACTIVATED)) && slot_enabled[11] && !inpt->lock[MAIN2]) return hotkeys_mod[11];
	return 0;
}

/**
 * If clicking while a menu is open, assume the player wants to rearrange the action bar
 */
int MenuActionBar::checkDrag(Point mouse) {
	int power_index;

	for (int i=0; i<12; i++) {
		if (isWithin(slots[i]->pos, mouse)) {
			drag_prev_slot = i;
			power_index = hotkeys[i];
			hotkeys[i] = 0;
			last_mouse = mouse;
			updated = true;
			return power_index;
		}
	}

	return 0;
}

/**
 * if clicking a menu, act as if the player pressed that menu's hotkey
 */
void MenuActionBar::checkMenu(bool &menu_c, bool &menu_i, bool &menu_p, bool &menu_l) {
	if (menus[MENU_CHARACTER]->checkClick()) {
		menu_c = true;
	}
	else if (menus[MENU_INVENTORY]->checkClick()) {
		menu_i = true;
	}
	else if (menus[MENU_POWERS]->checkClick()) {
		menu_p = true;
	}
	else if (menus[MENU_LOG]->checkClick()) {
		menu_l = true;
	}
}

/**
 * Set all hotkeys at once e.g. when loading a game
 */
void MenuActionBar::set(int power_id[12]) {
	for (int i=0; i<12; i++) {
		hotkeys[i] = power_id[i];
	}
	updated = true;
}

void MenuActionBar::resetSlots() {
	for (int i=0; i<12; i++) {
		slots[i]->checked = false;
		slots[i]->pressed = false;
	}
}

MenuActionBar::~MenuActionBar() {

	if (emptyslot)
		delete emptyslot;
	if (disabled)
		delete disabled;
	if (attention)
		delete attention;

	for (unsigned i = 0; i < 16; i++)
		delete labels[i];

	for (unsigned i = 0; i < 12; i++)
		delete slots[i];

	for (unsigned int i=0; i<4; i++)
		delete menus[i];
}
