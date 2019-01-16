/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
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

/**
 * class MenuActionBar
 *
 * Handles the config, display, and usage of the 0-9 hotkeys, mouse buttons, and menu calls
 */

#include "Avatar.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "MenuInventory.h"
#include "MenuManager.h"
#include "MenuTouchControls.h"
#include "MessageEngine.h"
#include "Platform.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "StatBlock.h"
#include "TooltipManager.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"
#include "WidgetSlot.h"

#include <climits>

MenuActionBar::MenuActionBar()
	: sprite_emptyslot(NULL)
	, sprite_disabled(NULL)
	, sprite_attention(NULL)
	, sfx_unable_to_cast(0)
	, slots_count(0)
	, drag_prev_slot(-1)
	, updated(false)
	, twostep_slot(-1) {

	menu_labels.resize(MENU_COUNT);

	tablist = TabList();
	tablist.setScrollType(Widget::SCROLL_HORIZONTAL);
	tablist.setInputs(Input::ACTIONBAR_BACK, Input::ACTIONBAR_FORWARD, Input::ACTIONBAR);

	for (unsigned i=0; i<MENU_COUNT; i++) {
		menus[i] = new WidgetSlot(-1, Input::ACTIONBAR);

		// NOTE: This prevents these buttons from being clickable unless they get defined in the config file.
		// However, it doesn't prevent them from being added to the tablist, so they can still be activated there despite being invisible
		// Mods should be expected to define these slot positions. This just keeps things from getting *too* broken if they're not defined.
		menus[i]->pos.w = 0;
		menus[i]->pos.h = 0;
	}

	menu_titles[MENU_CHARACTER] = msg->get("Character");
	menu_titles[MENU_INVENTORY] = msg->get("Inventory");
	menu_titles[MENU_POWERS] = msg->get("Powers");
	menu_titles[MENU_LOG] = msg->get("Log");

	// Read data from config file
	FileParser infile;

	// @CLASS MenuActionBar|Description of menus/actionbar.txt
	if (infile.open("menus/actionbar.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR slot|repeatable(int, int, int, bool) : Index, X, Y, Locked|Index (max 10) and position for power slot. If a slot is locked, its Power can't be changed by the player.
			if (infile.key == "slot") {
				unsigned index = Parse::popFirstInt(infile.val);
				if (index == 0 || index > 10) {
					infile.error("MenuActionBar: Slot index must be in range 1-10.");
				}
				else {
					int x = Parse::popFirstInt(infile.val);
					int y = Parse::popFirstInt(infile.val);
					std::string val = Parse::popFirstString(infile.val);
					bool is_locked = (val.empty() ? false : Parse::toBool(val));
					addSlot(index-1, x, y, is_locked);
				}
			}
			// @ATTR slot_M1|point, bool : Position, Locked|Position for the primary action slot. If the slot is locked, its Power can't be changed by the player.
			else if (infile.key == "slot_M1") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				std::string val = Parse::popFirstString(infile.val);
				bool is_locked = (val.empty() ? false : Parse::toBool(val));
				addSlot(10, x, y, is_locked);
			}
			// @ATTR slot_M2|point, bool : Position Locked|Position for the secondary action slot. If the slot is locked, its Power can't be changed by the player.
			else if (infile.key == "slot_M2") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				std::string val = Parse::popFirstString(infile.val);
				bool is_locked = (val.empty() ? false : Parse::toBool(val));
				addSlot(11, x, y, is_locked);
			}

			// @ATTR char_menu|point|Position for the Character menu button.
			else if (infile.key == "char_menu") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				menus[MENU_CHARACTER]->setBasePos(x, y, Utils::ALIGN_TOPLEFT);
				menus[MENU_CHARACTER]->pos.w = menus[MENU_CHARACTER]->pos.h = eset->resolutions.icon_size;
			}
			// @ATTR inv_menu|point|Position for the Inventory menu button.
			else if (infile.key == "inv_menu") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				menus[MENU_INVENTORY]->setBasePos(x, y, Utils::ALIGN_TOPLEFT);
				menus[MENU_INVENTORY]->pos.w = menus[MENU_INVENTORY]->pos.h = eset->resolutions.icon_size;
			}
			// @ATTR powers_menu|point|Position for the Powers menu button.
			else if (infile.key == "powers_menu") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				menus[MENU_POWERS]->setBasePos(x, y, Utils::ALIGN_TOPLEFT);
				menus[MENU_POWERS]->pos.w = menus[MENU_POWERS]->pos.h = eset->resolutions.icon_size;
			}
			// @ATTR log_menu|point|Position for the Log menu button.
			else if (infile.key == "log_menu") {
				int x = Parse::popFirstInt(infile.val);
				int y = Parse::popFirstInt(infile.val);
				menus[MENU_LOG]->setBasePos(x, y, Utils::ALIGN_TOPLEFT);
				menus[MENU_LOG]->pos.w = menus[MENU_LOG]->pos.h = eset->resolutions.icon_size;
			}

			else infile.error("MenuActionBar: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	for (unsigned i=0; i<MENU_COUNT; i++) {
		tablist.add(menus[i]);
	}

	slots_count = static_cast<unsigned>(slots.size());

	hotkeys.resize(slots_count);
	hotkeys_temp.resize(slots_count);
	hotkeys_mod.resize(slots_count);
	locked.resize(slots_count);
	slot_item_count.resize(slots_count);
	slot_enabled.resize(slots_count);
	slot_activated.resize(slots_count);
	slot_cooldown_size.resize(slots_count);
	slot_fail_cooldown.resize(slots_count);

	clear();

	loadGraphics();

	if (!eset->misc.sfx_unable_to_cast.empty())
		sfx_unable_to_cast = snd->load(eset->misc.sfx_unable_to_cast, "MenuActionBar unable to cast");

	align();

	menu_act = this;
}

void MenuActionBar::addSlot(unsigned index, int x, int y, bool is_locked) {
	if (index >= slots.size()) {
		labels.resize(index+1);
		slots.resize(index+1, NULL);
	}

	slots[index] = new WidgetSlot(-1, Input::ACTIONBAR);
	slots[index]->setBasePos(x, y, Utils::ALIGN_TOPLEFT);
	slots[index]->pos.w = slots[index]->pos.h = eset->resolutions.icon_size;
	slots[index]->continuous = true;

	prevent_changing.resize(slots.size());
	prevent_changing[index] = is_locked;

	tablist.add(slots[index]);
}

void MenuActionBar::align() {
	Menu::align();

	for (unsigned i = 0; i < slots_count; i++) {
		if (slots[i]) {
			slots[i]->setPos(window_area.x, window_area.y);
		}
	}
	for (unsigned i=0; i<MENU_COUNT; i++) {
		menus[i]->setPos(window_area.x, window_area.y);
	}

	// set keybinding labels
	for (unsigned i = 0; i < static_cast<unsigned>(SLOT_MAIN1); i++) {
		if (i < slots.size() && slots[i]) {
			labels[i] = msg->get("Hotkey: %s", inpt->getBindingString(i + Input::BAR_1));
		}
	}

	for (unsigned i = SLOT_MAIN1; i < static_cast<unsigned>(SLOT_MAX); i++) {
		if (i < slots.size() && slots[i]) {
			if (settings->mouse_move && ((i == SLOT_MAIN2 && settings->mouse_move_swap) || (i == SLOT_MAIN1 && !settings->mouse_move_swap))) {
				labels[i] = msg->get("Hotkey: %s", inpt->getBindingString(Input::SHIFT) + " + " + inpt->getBindingString(i - SLOT_MAIN1 + Input::MAIN1));
			}
			else {
				labels[i] = msg->get("Hotkey: %s", inpt->getBindingString(i - SLOT_MAIN1 + Input::MAIN1));
			}
		}
	}
	for (unsigned i=0; i<menu_labels.size(); i++) {
		menus[i]->setPos(window_area.x, window_area.y);
		menu_labels[i] = msg->get("Hotkey: %s", inpt->getBindingString(i + Input::CHARACTER));
	}
}

void MenuActionBar::clear() {
	// clear action bar
	for (unsigned i = 0; i < slots_count; i++) {
		hotkeys[i] = 0;
		hotkeys_temp[i] = 0;
		hotkeys_mod[i] = 0;
		slot_item_count[i] = -1;
		slot_enabled[i] = true;
		locked[i] = false;
		slot_activated[i] = false;
		slot_cooldown_size[i] = 0;
		slot_fail_cooldown[i] = 0;
	}

	// clear menu notifications
	for (unsigned i=0; i<MENU_COUNT; i++)
		requires_attention[i] = false;

	twostep_slot = -1;
}

void MenuActionBar::loadGraphics() {
	Image *graphics;

	setBackground("images/menus/actionbar_trim.png");

	Rect icon_clip;
	icon_clip.w = icon_clip.h = eset->resolutions.icon_size;

	graphics = render_device->loadImage("images/menus/slot_empty.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		sprite_emptyslot = graphics->createSprite();
		sprite_emptyslot->setClipFromRect(icon_clip);
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/disabled.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		sprite_disabled = graphics->createSprite();
		sprite_disabled->setClipFromRect(icon_clip);
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/attention_glow.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		sprite_attention = graphics->createSprite();
		graphics->unref();
	}
}

void MenuActionBar::logic() {
	tablist.logic();

	// hero has no powers
	if (pc->power_cast_timers.empty())
		return;

	for (unsigned i = 0; i < slots_count; i++) {
		if (!slots[i]) continue;

		if (hotkeys[i] > 0 && static_cast<unsigned>(hotkeys_mod[i]) < powers->powers.size()) {
			const Power &power = powers->powers[hotkeys_mod[i]];

			if (power.required_items.empty()) {
				setItemCount(i, -1, !IS_EQUIPPED);
			}
			else {
				for (size_t j = 0; j < power.required_items.size(); ++j) {
					if (power.required_items[j].equipped) {
						if (!menu->inv->inventory[MenuInventory::EQUIPMENT].contain(power.required_items[j].id, 1))
							setItemCount(i, 0, IS_EQUIPPED);
						else
							setItemCount(i, 1, IS_EQUIPPED);
					}
					else {
						if (power.required_items[j].quantity == 0) {
							if (!menu->inv->inventory[MenuInventory::CARRIED].contain(power.required_items[j].id, 1))
								setItemCount(i, 0, IS_EQUIPPED);
							else
								setItemCount(i, 1, IS_EQUIPPED);
						}
						else {
							setItemCount(i, menu->inv->inventory[MenuInventory::CARRIED].count(power.required_items[j].id), !IS_EQUIPPED);
						}
					}

					if (power.required_items[j].quantity > 0)
						break;
				}
			}

			//see if the slot should be greyed out
			slot_enabled[i] = pc->power_cooldown_timers[hotkeys_mod[i]].isEnd()
							  && pc->power_cast_timers[hotkeys_mod[i]].isEnd()
							  && pc->stats.canUsePower(hotkeys_mod[i], !StatBlock::CAN_USE_PASSIVE)
							  && (twostep_slot == -1 || static_cast<unsigned>(twostep_slot) == i);

			slots[i]->setIcon(power.icon, WidgetSlot::NO_OVERLAY);
		}
		else {
			slot_enabled[i] = true;
		}

		if (!pc->power_cast_timers[hotkeys_mod[i]].isEnd() && pc->power_cast_timers[hotkeys_mod[i]].getDuration() > 0) {
			slot_cooldown_size[i] = (eset->resolutions.icon_size * pc->power_cast_timers[hotkeys_mod[i]].getCurrent()) / pc->power_cast_timers[hotkeys_mod[i]].getDuration();
		}
		else if (!pc->power_cooldown_timers[hotkeys_mod[i]].isEnd() && pc->power_cooldown_timers[hotkeys_mod[i]].getDuration() > 0) {
			slot_cooldown_size[i] = (eset->resolutions.icon_size * pc->power_cooldown_timers[hotkeys_mod[i]].getCurrent()) / pc->power_cooldown_timers[hotkeys_mod[i]].getDuration();
		}
		else {
			slot_cooldown_size[i] = (slot_enabled[i] ? 0 : eset->resolutions.icon_size);;
		}

		if (slot_fail_cooldown[i] > 0)
			slot_fail_cooldown[i]--;
	}

}

void MenuActionBar::render() {

	Menu::render();

	// draw hotkeyed icons
	for (unsigned i = 0; i < slots_count; i++) {
		if (!slots[i]) continue;

		if (hotkeys[i] != 0) {
			slots[i]->render();
		}
		else {
			if (sprite_emptyslot) {
				sprite_emptyslot->setDestFromRect(slots[i]->pos);
				render_device->render(sprite_emptyslot);
			}
		}

		// render cooldown/disabled overlay
		if (!slot_enabled[i]) {
			Rect clip;
			clip.x = clip.y = 0;
			clip.w = clip.h = eset->resolutions.icon_size;

			// Wipe from bottom to top
			if (twostep_slot == -1 || static_cast<unsigned>(twostep_slot) == i) {
				clip.h = slot_cooldown_size[i];
			}

			if (sprite_disabled && clip.h > 0) {
				sprite_disabled->setClipFromRect(clip);
				sprite_disabled->setDestFromRect(slots[i]->pos);
				render_device->render(sprite_disabled);
			}
		}

		slots[i]->renderSelection();
	}

	// render primary menu buttons
	for (unsigned i=0; i<MENU_COUNT; i++) {
		menus[i]->render();

		if (requires_attention[i] && !menus[i]->in_focus) {
			Rect dest;

			if (sprite_attention) {
				sprite_attention->setDestFromRect(menus[i]->pos);
				render_device->render(sprite_attention);
			}

			// put an asterisk on this icon if in colorblind mode
			if (settings->colorblind) {
				WidgetLabel label;
				label.setPos(menus[i]->pos.x + eset->widgets.colorblind_highlight_offset.x, menus[i]->pos.y + eset->widgets.colorblind_highlight_offset.y);
				label.setText("*");
				label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));
				label.render();
			}
		}
	}
}

/**
 * On mouseover, show tooltip for buttons
 */
void MenuActionBar::renderTooltips(const Point& position) {
	TooltipData tip_data;

	// menus
	for (unsigned i = 0; i < MENU_COUNT; ++i) {
		if (Utils::isWithinRect(menus[i]->pos, position)) {
			if (settings->colorblind && requires_attention[i])
				tip_data.addText(menu_titles[i] + " (*)");
			else
				tip_data.addText(menu_titles[i]);

			if (!menu_labels[i].empty()) {
				tip_data.addText(menu_labels[i]);
			}

			tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
			break;
		}
	}
	tip_data.clear();

	for (unsigned i = 0; i < slots_count; i++) {
		if (slots[i] && Utils::isWithinRect(slots[i]->pos, position)) {
			if (hotkeys_mod[i] != 0) {
				tip_data.addText(powers->powers[hotkeys_mod[i]].name);
			}
			tip_data.addText(labels[i]);
		}
	}

	tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
}

/**
 * After dragging a power or item onto the action bar, set as new hotkey
 */
void MenuActionBar::drop(const Point& mouse, int power_index, bool rearranging) {
	for (unsigned i = 0; i < slots_count; i++) {
		if (slots[i] && !powers->powers[power_index].no_actionbar && Utils::isWithinRect(slots[i]->pos, mouse)) {
			if (rearranging) {
				if (prevent_changing[i]) {
					actionReturn(power_index);
					return;
				}
				if ((locked[i] && !locked[drag_prev_slot]) || (!locked[i] && locked[drag_prev_slot])) {
					locked[i] = !locked[i];
					locked[drag_prev_slot] = !locked[drag_prev_slot];
				}
				hotkeys[drag_prev_slot] = hotkeys[i];
			}
			else if (locked[i] || prevent_changing[i]) return;
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
	drop(last_mouse, power_index, !REORDER);
}

/**
 * CTRL-click a hotkey to clear it
 */
void MenuActionBar::remove(const Point& mouse) {
	for (unsigned i=0; i<slots_count; i++) {
		if (slots[i] && Utils::isWithinRect(slots[i]->pos, mouse)) {
			if (locked[i]) return;
			hotkeys[i] = 0;
			updated = true;
			return;
		}
	}
}

/**
 * If pressing an action key (keyboard or mouseclick) and the power can be used,
 * add that power to the action queue
 */
void MenuActionBar::checkAction(std::vector<ActionData> &action_queue) {
	if (inpt->pressing[Input::ACTIONBAR_USE] && tablist.getCurrent() == -1 && slots_count > 10) {
		tablist.setCurrent(slots[10]);
		slots[10]->in_focus = true;
	}

	bool enable_mm_attack = (!settings->mouse_move || inpt->pressing[Input::SHIFT] || pc->lock_enemy);
	bool enable_main1 = (!platform.is_mobile_device || (!menu->menus_open && menu->touch_controls->checkAllowMain1())) && (settings->mouse_move_swap || enable_mm_attack);
	bool enable_main2 = !settings->mouse_move_swap || enable_mm_attack;

	// check click and hotkey actions
	for (unsigned i = 0; i < slots_count; i++) {
		ActionData action;
		action.hotkey = i;
		bool have_aim = false;
		slot_activated[i] = false;

		if (!slots[i]) continue;

		// part two of two step activation
		if (static_cast<unsigned>(twostep_slot) == i && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
			have_aim = true;
			action.power = hotkeys_mod[i];
			twostep_slot = -1;
			inpt->lock[Input::MAIN1] = true;
		}

		// mouse/touch click
		else if (inpt->usingMouse() && slots[i]->checkClick() == WidgetSlot::ACTIVATED) {
			have_aim = false;
			slot_activated[i] = true;
			action.power = hotkeys_mod[i];

			// if a power requires a fixed target (like teleportation), break up activation into two parts
			// the first step is to mark the slot that was clicked on
			// NOTE only works for enabled slots
			if (action.power > 0) {
				const Power &power = powers->powers[action.power];
				if (power.starting_pos == Power::STARTING_POS_TARGET || power.buff_teleport) {
					if (slot_enabled[i]) {
						twostep_slot = i;
						action.power = 0;
					}
					else {
						twostep_slot = -1;
						action.power = 0;
					}
				}
				else {
					twostep_slot = -1;
				}
			}
		}

		// joystick/keyboard action button
		else if (inpt->pressing[Input::ACTIONBAR_USE] && static_cast<unsigned>(tablist.getCurrent()) == i) {
			have_aim = false;
			slot_activated[i] = true;
			action.power = hotkeys_mod[i];
			twostep_slot = -1;
		}

		// pressing hotkey
		else if (i<10 && inpt->pressing[i + Input::BAR_1]) {
			have_aim = inpt->usingMouse();
			action.power = hotkeys_mod[i];
			twostep_slot = -1;
		}
		else if (i==10 && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1] && !Utils::isWithinRect(window_area, inpt->mouse) && enable_main1) {
			have_aim = inpt->usingMouse();
			action.power = hotkeys_mod[10];
			twostep_slot = -1;
		}
		else if (i==11 && inpt->pressing[Input::MAIN2] && !inpt->lock[Input::MAIN2] && !Utils::isWithinRect(window_area, inpt->mouse) && enable_main2) {
			have_aim = inpt->usingMouse();
			action.power = hotkeys_mod[11];
			twostep_slot = -1;
		}

		// a power slot was activated
		if (action.power > 0 && static_cast<unsigned>(action.power) < powers->powers.size()) {
			const Power& power = powers->powers[action.power];

			if (pc->stats.mp < power.requires_mp && slot_fail_cooldown[i] == 0) {
				slot_fail_cooldown[i] = settings->max_frames_per_sec;
				pc->logMsg(msg->get("Not enough MP."), Avatar::MSG_NORMAL);
				snd->play(sfx_unable_to_cast, "ACT_NO_MP", snd->NO_POS, !snd->LOOP);
				continue;
			}
			else if (!slot_enabled[i]) {
				continue;
			}

			slot_fail_cooldown[i] = pc->power_cast_timers[action.power].getDuration();

			action.instant_item = false;
			if (power.new_state == Power::STATE_INSTANT) {
				for (size_t j = 0; j < power.required_items.size(); ++j) {
					if (power.required_items[j].id > 0 && !power.required_items[j].equipped) {
						action.instant_item = true;
						break;
					}
				}
			}

			bool can_use_power = true;

			// check if we can add this power to the action queue
			for (size_t j=0; j<action_queue.size(); j++) {
				if (action_queue[j].hotkey == i) {
					// this power is already in the action queue, update its target
					action_queue[j].target = setTarget(have_aim, power);
					can_use_power = false;
					break;
				}
				else if (!action.instant_item && !action_queue[j].instant_item) {
					can_use_power = false;
					break;
				}
			}
			if (!can_use_power)
				continue;

			// set the target depending on how the power was triggered
			action.target = setTarget(have_aim, power);

			// add it to the queue
			action_queue.push_back(action);
		}
		else {
			// if we're not triggering an action that is currently in the queue,
			// remove it from the queue
			for (size_t j=0; j<action_queue.size(); j++) {
				if (action_queue[j].hotkey == i)
					action_queue.erase(action_queue.begin()+j);
			}
		}
	}
}

/**
 * If clicking while a menu is open, assume the player wants to rearrange the action bar
 */
int MenuActionBar::checkDrag(const Point& mouse) {
	int power_index;

	for (unsigned i=0; i<slots_count; i++) {
		if (slots[i] && Utils::isWithinRect(slots[i]->pos, mouse)) {
			if (prevent_changing[i])
				return 0;

			drag_prev_slot = i;
			power_index = hotkeys[i];
			hotkeys[i] = 0;
			last_mouse = mouse;
			updated = true;
			twostep_slot = -1;
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
		menus[MENU_CHARACTER]->deactivate();
	}
	else if (menus[MENU_INVENTORY]->checkClick()) {
		menu_i = true;
		menus[MENU_INVENTORY]->deactivate();
	}
	else if (menus[MENU_POWERS]->checkClick()) {
		menu_p = true;
		menus[MENU_POWERS]->deactivate();
	}
	else if (menus[MENU_LOG]->checkClick()) {
		menu_l = true;
		menus[MENU_LOG]->deactivate();
	}

	// also allow ACTIONBAR_USE to open menus
	if (inpt->pressing[Input::ACTIONBAR_USE] && !inpt->lock[Input::ACTIONBAR_USE]) {
		inpt->lock[Input::ACTIONBAR_USE] = true;

		unsigned cur_slot = static_cast<unsigned>(tablist.getCurrent());

		if (cur_slot == tablist.size() - MENU_COUNT)
			menu_c = true;
		else if (cur_slot == tablist.size() - (MENU_COUNT - 1))
			menu_i = true;
		else if (cur_slot == tablist.size() - (MENU_COUNT - 2))
			menu_p = true;
		else if (cur_slot == tablist.size() - (MENU_COUNT - 3))
			menu_l = true;
	}
}

/**
 * Set all hotkeys at once e.g. when loading a game
 */
void MenuActionBar::set(std::vector<int> power_id) {
	for (unsigned i = 0; i < slots_count; i++) {
		if (static_cast<unsigned>(power_id[i]) >= powers->powers.size())
			continue;

		if (powers->powers[power_id[i]].no_actionbar)
			continue;

		hotkeys[i] = power_id[i];
	}
	updated = true;
}

void MenuActionBar::resetSlots() {
	for (unsigned i = 0; i < slots_count; i++) {
		if (slots[i]) {
			slots[i]->checked = false;
			slots[i]->pressed = false;
		}
	}
}

/**
 * Set a target depending on how a power was triggered
 */
FPoint MenuActionBar::setTarget(bool have_aim, const Power& pow) {
	if (have_aim && settings->mouse_aim) {
		FPoint map_pos;
		if (pow.aim_assist)
			map_pos = Utils::screenToMap(inpt->mouse.x,  inpt->mouse.y + eset->misc.aim_assist, mapr->cam.x, mapr->cam.y);
		else
			map_pos = Utils::screenToMap(inpt->mouse.x,  inpt->mouse.y, mapr->cam.x, mapr->cam.y);

		if (pow.target_nearest > 0) {
			if (!pow.requires_corpse && powers->checkNearestTargeting(pow, &pc->stats, false)) {
				map_pos = pc->stats.target_nearest->pos;
			}
			else if (pow.requires_corpse && powers->checkNearestTargeting(pow, &pc->stats, true)) {
				map_pos = pc->stats.target_nearest_corpse->pos;
			}
		}

		return map_pos;
	}
	else {
		return Utils::calcVector(pc->stats.pos, pc->stats.direction, pc->stats.melee_range);
	}
}

void MenuActionBar::setItemCount(unsigned index, int count, bool is_equipped) {
	if (index >= slots_count || !slots[index]) return;

	slot_item_count[index] = count;
	if (count == 0) {
		if (slot_activated[index])
			slots[index]->deactivate();

		slot_enabled[index] = false;
	}

	if (is_equipped)
		// we don't care how many of an equipped item we're carrying
		slots[index]->setAmount(count, 0);
	else if (count >= 0)
		// we can always carry more than 1 of any item, so always display non-equipped item count
		slots[index]->setAmount(count, 2);
	else
		// slot contains a regular power, so ignore item count
		slots[index]->setAmount(0,0);
}

bool MenuActionBar::isWithinSlots(const Point& mouse) {
	for (unsigned i=0; i<slots_count; i++) {
		if (slots[i] && Utils::isWithinRect(slots[i]->pos, mouse))
			return true;
	}
	return false;
}

bool MenuActionBar::isWithinMenus(const Point& mouse) {
	for (unsigned i=0; i<MENU_COUNT; i++) {
		if (Utils::isWithinRect(menus[i]->pos, mouse))
			return true;
	}
	return false;
}

/**
 * Replaces the power(s) in slots that match the target_id with the power of id
 * So a target_id of 0 will place the power in an empty slot, if available
 */
void MenuActionBar::addPower(const int id, const int target_id) {
	if (static_cast<unsigned>(id) >= powers->powers.size())
		return;

	// some powers are explicitly prevented from being placed on the actionbar
	if (powers->powers[id].no_actionbar)
		return;

	// can't put passive powers on the action bar
	if (powers->powers[id].passive)
		return;

	// if we're not replacing an existing power, avoid placing duplicate powers
	if (target_id == 0) {
		for (unsigned i = 0; i < static_cast<unsigned>(SLOT_MAX); ++i) {
			if (hotkeys[i] == id)
				return;
		}
	}

	// MAIN slots have priority
	for (unsigned i=10; i<12; ++i) {
		if (hotkeys[i] == target_id) {
			if (target_id == 0 && prevent_changing[i]) {
				continue;
			}
			hotkeys[i] = id;
			updated = true;
			if (target_id == 0)
				return;
		}
	}

	// now try 0-9 slots
	for (unsigned i=0; i<10; ++i) {
		if (hotkeys[i] == target_id) {
			if (target_id == 0 && prevent_changing[i]) {
				continue;
			}
			hotkeys[i] = id;
			updated = true;
			if (target_id == 0)
				return;
		}
	}
}

Point MenuActionBar::getSlotPos(int slot) {
	if (static_cast<unsigned>(slot) < slots.size()) {
		return Point(slots[slot]->pos.x, slots[slot]->pos.y);
	}
	else if (static_cast<unsigned>(slot) < slots.size() + MENU_COUNT) {
		int slot_size = static_cast<int>(slots.size());
		return Point(menus[slot - slot_size]->pos.x, menus[slot - slot_size]->pos.y);
	}
	return Point();
}

int MenuActionBar::getSlotPower(int slot) {
	if (static_cast<unsigned>(slot) < hotkeys.size()) {
		return hotkeys_mod[slot];
	}
	return 0;
}

MenuActionBar::~MenuActionBar() {

	menu_act = NULL;
	if (sprite_emptyslot)
		delete sprite_emptyslot;
	if (sprite_disabled)
		delete sprite_disabled;
	if (sprite_attention)
		delete sprite_attention;

	labels.clear();
	menu_labels.clear();

	for (unsigned i = 0; i < slots_count; i++)
		delete slots[i];

	for (unsigned i=0; i<MENU_COUNT; i++)
		delete menus[i];

	snd->unload(sfx_unable_to_cast);
}
