/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert
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
 * class MenuManager
 */

#include "Avatar.h"
#include "EngineSettings.h"
#include "FontEngine.h"
#include "IconManager.h"
#include "InputState.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "MenuActiveEffects.h"
#include "MenuBook.h"
#include "MenuCharacter.h"
#include "MenuDevConsole.h"
#include "MenuEnemy.h"
#include "MenuExit.h"
#include "MenuHUDLog.h"
#include "MenuInventory.h"
#include "MenuLog.h"
#include "MenuManager.h"
#include "MenuMiniMap.h"
#include "MenuNumPicker.h"
#include "MenuPowers.h"
#include "MenuStash.h"
#include "MenuStatBar.h"
#include "MenuTalker.h"
#include "MenuTouchControls.h"
#include "MenuVendor.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "NPC.h"
#include "PowerManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Subtitles.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetSlot.h"

MenuManager::MenuManager()
	: key_lock(false)
	, mouse_dragging(false)
	, keyboard_dragging(false)
	, sticky_dragging(false)
	, drag_stack()
	, drag_power(0)
	, drag_src(DRAG_SRC_NONE)
	, drag_icon(new WidgetSlot(WidgetSlot::NO_ICON, Input::ACCEPT))
	, done(false)
	, act_drag_hover(false)
	, keydrag_pos(Point())
/*std::vector<Menu*> menus;*/
	, inv(NULL)
	, pow(NULL)
	, chr(NULL)
	, questlog(NULL)
	, hudlog(NULL)
	, act(NULL)
	, book(NULL)
	, hp(NULL)
	, mp(NULL)
	, xp(NULL)
	, mini(NULL)
	, num_picker(NULL)
	, enemy(NULL)
	, vendor(NULL)
	, talker(NULL)
	, exit(NULL)
	, effects(NULL)
	, stash(NULL)
	, devconsole(NULL)
	, touch_controls(NULL)
	, subtitles(NULL)
	, pause(false)
	, menus_open(false) {

	hp = new MenuStatBar(MenuStatBar::TYPE_HP);
	mp = new MenuStatBar(MenuStatBar::TYPE_MP);
	xp = new MenuStatBar(MenuStatBar::TYPE_XP);
	effects = new MenuActiveEffects();
	hudlog = new MenuHUDLog();
	act = new MenuActionBar();
	enemy = new MenuEnemy();
	vendor = new MenuVendor();
	talker = new MenuTalker();
	exit = new MenuExit();
	mini = new MenuMiniMap();
	chr = new MenuCharacter();
	inv = new MenuInventory();
	pow = new MenuPowers();
	questlog = new MenuLog();
	stash = new MenuStash();
	book = new MenuBook();
	num_picker = new MenuNumPicker();

	menus.push_back(hp); // menus[0]
	menus.push_back(mp); // menus[1]
	menus.push_back(xp); // menus[2]
	menus.push_back(effects); // menus[3]
	menus.push_back(hudlog); // menus[4]
	menus.push_back(act); // menus[5]
	menus.push_back(enemy); // menus[6]
	menus.push_back(vendor); // menus[7]
	menus.push_back(talker); // menus[8]
	menus.push_back(exit); // menus[9]
	menus.push_back(mini); // menus[10]
	menus.push_back(chr); // menus[11]
	menus.push_back(inv); // menus[12]
	menus.push_back(pow); // menus[13]
	menus.push_back(questlog); // menus[14]
	menus.push_back(stash); // menus[15]
	menus.push_back(book); // menus[16]
	menus.push_back(num_picker); // menus[17]

	if (settings->dev_mode) {
		devconsole = new MenuDevConsole();
	}

	touch_controls = new MenuTouchControls();

	subtitles = new Subtitles();

	closeAll(); // make sure all togglable menus start closed

	settings->show_hud = true;

	drag_icon->enabled = false;
}

void MenuManager::alignAll() {
	for (size_t i=0; i<menus.size(); i++) {
		menus[i]->align();
	}

	if (settings->dev_mode) {
		devconsole->align();
	}

	touch_controls->align();
}

void MenuManager::renderIcon(int x, int y) {
	if (drag_icon->getIcon() != WidgetSlot::NO_ICON) {
		drag_icon->setPos(x,y);
		drag_icon->render();
	}
}

void MenuManager::setDragIcon(int icon_id, int overlay_id) {
	drag_icon->setIcon(icon_id, overlay_id);
	drag_icon->setAmount(0, 0);
}

void MenuManager::setDragIconItem(ItemStack stack) {
	if (stack.empty()) {
		drag_icon->setIcon(WidgetSlot::NO_ICON, WidgetSlot::NO_OVERLAY);
		drag_icon->setAmount(0, 0);
	}
	else {
		drag_icon->setIcon(items->items[stack.item].icon, items->getItemIconOverlay(stack.item));
		drag_icon->setAmount(stack.quantity, items->items[stack.item].max_quantity);
	}
}

void MenuManager::handleKeyboardNavigation() {

	stash->tablist_private.setNextTabList(NULL);
	stash->tablist_shared.setNextTabList(NULL);
	vendor->tablist_buy.setNextTabList(NULL);
	vendor->tablist_sell.setNextTabList(NULL);
	chr->tablist.setNextTabList(NULL);
	questlog->setNextTabList(&questlog->tablist);
	inv->tablist.setPrevTabList(NULL);
	pow->setNextTabList(NULL);

	// unlock menus if only one side is showing
	if (!inv->visible && !pow->visible) {
		stash->tablist.unlock();
		vendor->tablist.unlock();
		chr->tablist.unlock();
		if (!questlog->getCurrentTabList())
			questlog->tablist.unlock();

	}
	else if (!vendor->visible && !stash->visible && !chr->visible && !questlog->visible) {
		inv->tablist.unlock();
		if (!pow->getCurrentTabList())
			pow->tablist.unlock();
	}

	if (drag_src == DRAG_SRC_NONE) {
		if (inv->visible) {
			stash->tablist_private.setNextTabList(&inv->tablist);
			stash->tablist_shared.setNextTabList(&inv->tablist);
			vendor->tablist_buy.setNextTabList(&inv->tablist);
			vendor->tablist_sell.setNextTabList(&inv->tablist);
			chr->tablist.setNextTabList(&inv->tablist);
			questlog->setNextTabList(&inv->tablist);

			if (stash->visible) {
				inv->tablist.setPrevTabList(&stash->tablist);
			}
			else if (vendor->visible) {
				inv->tablist.setPrevTabList(&vendor->tablist);
			}
			else if (chr->visible) {
				inv->tablist.setPrevTabList(&chr->tablist);
			}
			else if (questlog->visible) {
				inv->tablist.setPrevTabList(&questlog->tablist);
			}
		}
		else if (pow->visible) {
			stash->tablist_private.setNextTabList(&pow->tablist);
			stash->tablist_shared.setNextTabList(&pow->tablist);
			vendor->tablist_buy.setNextTabList(&pow->tablist);
			vendor->tablist_sell.setNextTabList(&pow->tablist);
			chr->tablist.setNextTabList(&pow->tablist);
			questlog->setNextTabList(&pow->tablist);

			// NOTE stash and vendor are only visible with inventory, so we don't need to handle them here
			if (chr->visible) {
				pow->tablist.setPrevTabList(&chr->tablist);
			}
			else if (questlog->visible) {
				pow->tablist.setPrevTabList(&questlog->tablist);
			}
		}
	}

	// stash and vendor always start locked
	if (!stash->visible) {
		stash->tablist.lock();
		stash->tablist_private.lock();
		stash->tablist_shared.lock();
	}
	if (!vendor->visible) {
		vendor->tablist.lock();
		vendor->tablist_buy.lock();
		vendor->tablist_sell.lock();
	}

	// inventory always starts unlocked
	if (!inv->visible) inv->tablist.unlock();

	// position the drag hover icon depending on the last key press
	if (!act_drag_hover && (inpt->pressing[Input::ACTIONBAR_BACK] || inpt->pressing[Input::ACTIONBAR_FORWARD] || inpt->pressing[Input::ACTIONBAR]))
		act_drag_hover = true;
	else if (act_drag_hover && (inpt->pressing[Input::LEFT] || inpt->pressing[Input::RIGHT] || inpt->pressing[Input::UP] || inpt->pressing[Input::DOWN]) && !(inpt->pressing[Input::ACTIONBAR_BACK] || inpt->pressing[Input::ACTIONBAR_FORWARD]))
		act_drag_hover = false;

	// don't allow dropping actionbar items in other menus
	if (keyboard_dragging && drag_src == DRAG_SRC_ACTIONBAR) {
		inpt->lock[Input::ACCEPT] = true;
	}
}

void MenuManager::logic() {
	ItemStack stack;

	subtitles->logic(snd->getLastPlayedSID());

	hp->update(0, pc->stats.hp, pc->stats.get(Stats::HP_MAX));
	mp->update(0, pc->stats.mp, pc->stats.get(Stats::MP_MAX));

	if (pc->stats.level == eset->xp.getMaxLevel()) {
		xp->setCustomString(msg->get("XP: %d", pc->stats.xp));
	}
	else {
		// displays xp relative to current pc level
		xp->setCustomString(msg->get("XP: %d/%d", pc->stats.xp - eset->xp.getLevelXP(pc->stats.level), eset->xp.getLevelXP(pc->stats.level + 1) - eset->xp.getLevelXP(pc->stats.level)));
	}
	// xp relative to current level (from 0 to ammount need for next level)
	xp->update(0, pc->stats.xp - eset->xp.getLevelXP(pc->stats.level), eset->xp.getLevelXP(pc->stats.level + 1) - eset->xp.getLevelXP(pc->stats.level));

	// when selecting item quantities, don't process other menus
	if (num_picker->visible) {
		num_picker->logic();

		if (num_picker->confirm_clicked) {
			// start dragging items
			// removes the desired quantity from the source stack

			if (drag_src == DRAG_SRC_INVENTORY) {
				drag_stack.quantity = num_picker->getValue();
				inv->removeFromPrevSlot(drag_stack.quantity);
			}
			else if (drag_src == DRAG_SRC_VENDOR) {
				drag_stack.quantity = num_picker->getValue();
				vendor->removeFromPrevSlot(drag_stack.quantity);
			}
			else if (drag_src == DRAG_SRC_STASH) {
				drag_stack.quantity = num_picker->getValue();
				stash->removeFromPrevSlot(drag_stack.quantity);
			}

			num_picker->confirm_clicked = false;
			num_picker->visible = false;
			if (inpt->usingMouse()) {
				sticky_dragging = true;
			}
		}
		else if (num_picker->cancel_clicked) {
			// cancel item dragging
			drag_stack.quantity = 0;
			drag_stack.item = 0;
			resetDrag();
			num_picker->cancel_clicked = false;
			num_picker->visible = false;
		}
		else {
			pause = true;
			return;
		}
	}

	if (!inpt->usingMouse())
		handleKeyboardNavigation();

	// Check if the mouse is within any of the visible windows. Excludes the minimap and the exit/pause menu
	bool is_within_menus = (Utils::isWithinRect(act->window_area, inpt->mouse) ||
	(book->visible && Utils::isWithinRect(book->window_area, inpt->mouse)) ||
	(chr->visible && Utils::isWithinRect(chr->window_area, inpt->mouse)) ||
	(inv->visible && Utils::isWithinRect(inv->window_area, inpt->mouse)) ||
	(vendor->visible && Utils::isWithinRect(vendor->window_area, inpt->mouse)) ||
	(pow->visible && Utils::isWithinRect(pow->window_area, inpt->mouse)) ||
	(questlog->visible && Utils::isWithinRect(questlog->window_area, inpt->mouse)) ||
	(talker->visible && Utils::isWithinRect(talker->window_area, inpt->mouse)) ||
	(stash->visible && Utils::isWithinRect(stash->window_area, inpt->mouse)) ||
	(settings->dev_mode && devconsole->visible && Utils::isWithinRect(devconsole->window_area, inpt->mouse)));

	// Stop attacking if the cursor is inside an interactable menu
	if ((pc->using_main1 || pc->using_main2) && is_within_menus) {
		inpt->pressing[Input::MAIN1] = false;
		inpt->pressing[Input::MAIN2] = false;
	}

	if (!exit->visible && !is_within_menus)
		mini->logic();

	book->logic();
	effects->logic();
	act->logic();
	hudlog->logic();
	enemy->logic();
	chr->logic();
	inv->logic();
	vendor->logic();
	pow->logic();
	questlog->logic();
	talker->logic();
	stash->logic();

	if (settings->dev_mode) {
		devconsole->logic();
	}

	touch_controls->logic();

	if (chr->checkUpgrade() || pc->stats.level_up) {
		// apply equipment and max hp/mp
		inv->applyEquipment();
		pc->stats.hp = pc->stats.get(Stats::HP_MAX);
		pc->stats.mp = pc->stats.get(Stats::MP_MAX);
		pc->stats.level_up = false;
	}

	// only allow the vendor window to be open if the inventory is open
	if (vendor->visible && !(inv->visible)) {
		snd->play(vendor->sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
		closeAll();
	}

	// context-sensistive help tooltip in inventory menu
	if (inv->visible && vendor->visible) {
		inv->inv_ctrl = MenuInventory::CTRL_VENDOR;
	}
	else if (inv->visible && stash->visible) {
		inv->inv_ctrl = MenuInventory::CTRL_STASH;
	}
	else {
		inv->inv_ctrl = MenuInventory::CTRL_NONE;
	}

	if (!inpt->pressing[Input::INVENTORY] && !inpt->pressing[Input::POWERS] && !inpt->pressing[Input::CHARACTER] && !inpt->pressing[Input::LOG])
		key_lock = false;

	if (settings->dev_mode && devconsole->inputFocus())
		key_lock = true;

	// cancel dragging and defocus menu tablists
	if (!key_lock && inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL] && !pc->stats.corpse) {
		if (keyboard_dragging || mouse_dragging) {
			inpt->lock[Input::CANCEL] = true;
			resetDrag();
		}
		for (size_t i=0; i<menus.size(); i++) {
			TabList *tablist = menus[i]->getCurrentTabList();
			if (tablist) {
				inpt->lock[Input::CANCEL] = true;
				menus[i]->defocusTabLists();
			}
			if (settings->dev_mode) {
				tablist = devconsole->getCurrentTabList();
				if (tablist) {
					inpt->lock[Input::CANCEL] = true;
					devconsole->defocusTabLists();
				}
			}
		}
	}

	// exit menu toggle
	if (!key_lock && !mouse_dragging && !keyboard_dragging) {
		if (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) {
			inpt->lock[Input::CANCEL] = true;
			key_lock = true;
			if (act->twostep_slot != -1) {
				act->twostep_slot = -1;
			}
			else if (settings->dev_mode && devconsole->visible) {
				devconsole->closeWindow();
			}
			else if (menus_open) {
				closeAll();
			}
			else {
				exit->handleCancel();
			}
		}
	}

	if (exit->visible) {
		exit->logic();
		if (exit->isExitRequested()) {
			done = true;
		}
		// if dpi scaling is changed, we need to realign the menus
		if (inpt->window_resized) {
			alignAll();
		}
	}
	else {
		bool clicking_character = false;
		bool clicking_inventory = false;
		bool clicking_powers = false;
		bool clicking_log = false;

		// check if mouse-clicking a menu button
		act->checkMenu(clicking_character, clicking_inventory, clicking_powers, clicking_log);

		// inventory menu toggle
		if ((inpt->pressing[Input::INVENTORY] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_inventory) {
			key_lock = true;
			if (inv->visible) {
				snd->play(inv->sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
				closeRight();
			}
			else {
				closeRight();
				act->requires_attention[MenuActionBar::MENU_INVENTORY] = false;
				inv->visible = true;
				snd->play(inv->sfx_open, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
			}

		}

		// powers menu toggle
		if (((inpt->pressing[Input::POWERS] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_powers) && !pc->stats.transformed) {
			key_lock = true;
			if (pow->visible) {
				snd->play(pow->sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
				closeRight();
			}
			else {
				closeRight();
				act->requires_attention[MenuActionBar::MENU_POWERS] = false;
				pow->visible = true;
				snd->play(pow->sfx_open, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
			}
		}

		// character menu toggleggle
		if ((inpt->pressing[Input::CHARACTER] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_character) {
			key_lock = true;
			if (chr->visible) {
				snd->play(chr->sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
				closeLeft();
			}
			else {
				closeLeft();
				act->requires_attention[MenuActionBar::MENU_CHARACTER] = false;
				chr->visible = true;
				snd->play(chr->sfx_open, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
				// Make sure the stat list isn't scrolled when we open the character menu
				inpt->resetScroll();
			}
		}

		// log menu toggle
		if ((inpt->pressing[Input::LOG] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_log) {
			key_lock = true;
			if (questlog->visible) {
				snd->play(questlog->sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
				closeLeft();
			}
			else {
				closeLeft();
				act->requires_attention[MenuActionBar::MENU_LOG] = false;
				questlog->visible = true;
				snd->play(questlog->sfx_open, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
				// Make sure the log isn't scrolled when we open the log menu
				inpt->resetScroll();
			}
		}

		//developer console
		if (settings->dev_mode && inpt->pressing[Input::DEVELOPER_MENU] && !inpt->lock[Input::DEVELOPER_MENU] && !mouse_dragging && !keyboard_dragging) {
			inpt->lock[Input::DEVELOPER_MENU] = true;
			if (devconsole->visible) {
				closeAll();
				key_lock = false;
			}
			else {
				closeAll();
				devconsole->visible = true;
			}
		}
	}

	bool console_open = settings->dev_mode && devconsole->visible;
	menus_open = (inv->visible || pow->visible || chr->visible || questlog->visible || vendor->visible || talker->visible || book->visible || console_open);
	pause = (eset->misc.menus_pause && menus_open) || exit->visible || console_open || book->visible;

	touch_controls->visible = !menus_open && !exit->visible;

	if (pc->stats.alive) {

		// handle right-click
		if (!mouse_dragging && inpt->pressing[Input::MAIN2]) {
			// exit menu
			if (exit->visible && Utils::isWithinRect(exit->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}

			// book menu
			else if (book->visible && Utils::isWithinRect(book->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}

			// inventory
			else if (inv->visible && Utils::isWithinRect(inv->window_area, inpt->mouse)) {
				if (!inpt->lock[Input::MAIN2] && Utils::isWithinRect(inv->carried_area, inpt->mouse)) {
					// activate inventory item
					inv->activate(inpt->mouse);
				}
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}

			// other menus
			else if (talker->visible && Utils::isWithinRect(talker->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}
			else if (pow->visible && Utils::isWithinRect(pow->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}
			else if (chr->visible && Utils::isWithinRect(chr->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}
			else if (questlog->visible && Utils::isWithinRect(questlog->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}
			else if (vendor->visible && Utils::isWithinRect(vendor->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}
			else if (stash->visible && Utils::isWithinRect(stash->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN2] = true;
				inpt->pressing[Input::MAIN2] = false;
			}
		}

		// handle left-click for book menu first
		if (!mouse_dragging && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
			if (book->visible && Utils::isWithinRect(book->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;
			}
		}

		// handle left-click
		if (!mouse_dragging && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
			resetDrag();

			for (size_t i=0; i<menus.size(); ++i) {
				if (!menus[i]->visible || !Utils::isWithinRect(menus[i]->window_area, inpt->mouse)) {
					menus[i]->defocusTabLists();
				}
			}

			// exit menu
			if (exit->visible && Utils::isWithinRect(exit->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;
			}


			if (chr->visible && Utils::isWithinRect(chr->window_area, inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;
			}

			if (vendor->visible && Utils::isWithinRect(vendor->window_area,inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;
				if (inpt->pressing[Input::CTRL]) {
					// buy item from a vendor
					stack = vendor->click(inpt->mouse);
					if (!inv->buy(stack, vendor->getTab(), !MenuInventory::IS_DRAGGING)) {
						vendor->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
				}
				else {
					// start dragging a vendor item
					drag_stack = vendor->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_VENDOR;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[Input::SHIFT] || !inpt->usingMouse() || inpt->touch_locked)) {
						int max_quantity = std::min(inv->getMaxPurchasable(drag_stack, vendor->getTab()), drag_stack.quantity);
						if (max_quantity >= 1) {
							num_picker->setValueBounds(1, max_quantity);
							num_picker->visible = true;
						}
						else {
							drag_stack.clear();
							resetDrag();
						}
					}
				}
			}

			if (stash->visible && Utils::isWithinRect(stash->window_area,inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;
				if (inpt->pressing[Input::CTRL]) {
					// take an item from the stash
					stack = stash->click(inpt->mouse);
					if (!inv->add(stack, MenuInventory::CARRIED, ItemStorage::NO_SLOT, MenuInventory::ADD_PLAY_SOUND, MenuInventory::ADD_AUTO_EQUIP)) {
						stash->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					stash->updated = true;
				}
				else {
					// start dragging a stash item
					drag_stack = stash->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_STASH;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[Input::SHIFT] || !inpt->usingMouse() || inpt->touch_locked)) {
						num_picker->setValueBounds(1, drag_stack.quantity);
						num_picker->visible = true;
					}
				}
			}

			if (questlog->visible && Utils::isWithinRect(questlog->window_area,inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;
			}

			// pick up an inventory item
			if (inv->visible && Utils::isWithinRect(inv->window_area,inpt->mouse)) {
				if (inpt->pressing[Input::CTRL]) {
					inpt->lock[Input::MAIN1] = true;
					stack = inv->click(inpt->mouse);
					if (stash->visible) {
						if (!stash->add(stack, MenuStash::NO_SLOT, MenuStash::ADD_PLAY_SOUND)) {
							inv->itemReturn(stash->drop_stack.front());
							stash->drop_stack.pop();
						}
					}
					else {
						// The vendor could have a limited amount of currency in the future. It will be tested here.
						if ((eset->misc.sell_without_vendor || vendor->visible) && inv->sell(stack)) {
							if (vendor->visible) {
								vendor->setTab(ItemManager::VENDOR_SELL);
								vendor->add(stack);
							}
						}
						else {
							inv->itemReturn(stack);
						}
					}
				}
				else {
					inpt->lock[Input::MAIN1] = true;
					drag_stack = inv->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_INVENTORY;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[Input::SHIFT] || !inpt->usingMouse() || inpt->touch_locked)) {
						num_picker->setValueBounds(1, drag_stack.quantity);
						num_picker->visible = true;
					}
				}
			}
			// pick up a power
			if (pow->visible && Utils::isWithinRect(pow->window_area,inpt->mouse)) {
				inpt->lock[Input::MAIN1] = true;

				// check for unlock/dragging
				drag_power = pow->click(inpt->mouse);
				if (drag_power > 0) {
					mouse_dragging = true;
					keyboard_dragging = false;
					drag_src = DRAG_SRC_POWERS;
				}
			}
			// action bar
			if (!exit->visible && !inpt->touch_locked && (act->isWithinSlots(inpt->mouse) || act->isWithinMenus(inpt->mouse))) {
				inpt->lock[Input::MAIN1] = true;

				// ctrl-click action bar to clear that slot
				if (inpt->pressing[Input::CTRL]) {
					act->remove(inpt->mouse);
				}
				// allow drag-to-rearrange action bar
				else if (!act->isWithinMenus(inpt->mouse)) {
					drag_power = act->checkDrag(inpt->mouse);
					if (drag_power > 0) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_ACTIONBAR;
					}
				}

				// else, clicking action bar to use a power?
				// this check is done by GameEngine when calling Avatar::logic()


			}
		}

		// highlight matching inventory slots based on what we're dragging
		if (inv->visible && (mouse_dragging || keyboard_dragging)) {
			inv->inventory[MenuInventory::EQUIPMENT].highlightMatching(items->items[drag_stack.item].type);
		}

		// handle dropping
		if (mouse_dragging && ((sticky_dragging && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) || (!sticky_dragging && !inpt->pressing[Input::MAIN1]))) {
			if (sticky_dragging) {
				inpt->lock[Input::MAIN1] = true;
				sticky_dragging = false;
			}

			// putting a power on the Action Bar
			if (drag_src == DRAG_SRC_POWERS) {
				if (act->isWithinSlots(inpt->mouse)) {
					act->drop(inpt->mouse, drag_power, !MenuActionBar::REORDER);
				}
			}

			// rearranging the action bar
			else if (drag_src == DRAG_SRC_ACTIONBAR) {
				if (act->isWithinSlots(inpt->mouse)) {
					act->drop(inpt->mouse, drag_power, MenuActionBar::REORDER);
					// for locked slots forbid power dropping
				}
				else if (act->locked[act->drag_prev_slot]) {
					act->hotkeys[act->drag_prev_slot] = drag_power;
				}
				drag_power = 0;
			}

			// rearranging inventory or dropping items
			else if (drag_src == DRAG_SRC_INVENTORY) {

				if (inv->visible && Utils::isWithinRect(inv->window_area, inpt->mouse)) {
					inv->drop(inpt->mouse, drag_stack);
				}
				else if (act->isWithinSlots(inpt->mouse)) {
					// The action bar is not storage!
					inv->itemReturn(drag_stack);
					inv->applyEquipment();

					// put an item with a power on the action bar
					if (items->items[drag_stack.item].power != 0) {
						act->drop(inpt->mouse, items->items[drag_stack.item].power, !MenuActionBar::REORDER);
					}
				}
				else if (vendor->visible && Utils::isWithinRect(vendor->window_area, inpt->mouse)) {
					if (inv->sell( drag_stack)) {
						vendor->setTab(ItemManager::VENDOR_SELL);
						vendor->add( drag_stack);
					}
					else {
						inv->itemReturn(drag_stack);
					}
				}
				else if (stash->visible && Utils::isWithinRect(stash->window_area, inpt->mouse)) {
					stash->stock[MenuStash::STASH_PRIVATE].drag_prev_slot = -1;
					stash->stock[MenuStash::STASH_SHARED].drag_prev_slot = -1;
					if (!stash->drop(inpt->mouse, drag_stack)) {
						inv->itemReturn(stash->drop_stack.front());
						stash->drop_stack.pop();
					}
				}
				else {
					// if dragging and the source was inventory, drop item to the floor

					// quest items cannot be dropped
					if (!items->items[drag_stack.item].quest_item) {
						drop_stack.push(drag_stack);
					}
					else {
						pc->logMsg(msg->get("This item can not be dropped."), Avatar::MSG_NORMAL);
						items->playSound(drag_stack.item);

						inv->itemReturn(drag_stack);
					}
				}
				inv->clearHighlight();
			}

			else if (drag_src == DRAG_SRC_VENDOR) {

				// dropping an item from vendor (we only allow to drop into the carried area)
				if (inv->visible && Utils::isWithinRect(inv->window_area, inpt->mouse)) {
					if (!inv->buy(drag_stack, vendor->getTab(), MenuInventory::IS_DRAGGING)) {
						vendor->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
				}
				else {
					vendor->itemReturn(drag_stack);
				}
			}

			else if (drag_src == DRAG_SRC_STASH) {

				// dropping an item from stash (we only allow to drop into the carried area)
				if (inv->visible && Utils::isWithinRect(inv->window_area, inpt->mouse)) {
					if (!inv->drop(inpt->mouse, drag_stack)) {
						stash->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					stash->updated = true;
				}
				else if (stash->visible && Utils::isWithinRect(stash->window_area, inpt->mouse)) {
					if (!stash->drop(inpt->mouse,drag_stack)) {
						drop_stack.push(stash->drop_stack.front());
						stash->drop_stack.pop();
					}
				}
				else {
					drop_stack.push(drag_stack);
					stash->updated = true;
				}
			}

			drag_stack.clear();
			drag_power = 0;
			drag_src = DRAG_SRC_NONE;
			mouse_dragging = false;
		}
		if (!inpt->usingMouse())
			dragAndDropWithKeyboard();
	}
	else {
		if (mouse_dragging || keyboard_dragging) {
			resetDrag();
		}
	}

	// return items that are currently begin dragged when returning to title screen or exiting game
	if (done || inpt->done) {
		resetDrag();
	}

	// handle equipment changes affecting hero stats
	if (inv->changed_equipment) {
		inv->applyEquipment();
		// the equipment flags get reset in GameStatePlay
	}

	if (!(mouse_dragging || keyboard_dragging)) {
		setDragIcon(WidgetSlot::NO_ICON, WidgetSlot::NO_OVERLAY);
	}
}

void MenuManager::dragAndDropWithKeyboard() {
	// inventory menu

	if (inv->visible && inv->getCurrentTabList() && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = inv->getCurrentTabList()->getCurrent();
		Point src_slot;
		WidgetSlot * inv_slot;

		if (slot_index < inv->getEquippedCount())
			inv_slot = inv->inventory[MenuInventory::EQUIPMENT].slots[slot_index];
		else if (slot_index < inv->getTotalSlotCount())
			inv_slot = inv->inventory[MenuInventory::CARRIED].slots[slot_index - inv->getEquippedCount()];
		else
			inv_slot = NULL;

		if (inv_slot) {
			src_slot.x = inv_slot->pos.x;
			src_slot.y = inv_slot->pos.y;

			WidgetSlot::CLICK_TYPE slotClick = inv_slot->checkClick();

			// pick up item
			if (slotClick == WidgetSlot::CHECKED && drag_stack.empty()) {
				drag_stack = inv->click(src_slot);
				if (!drag_stack.empty()) {
					keyboard_dragging = true;
					drag_src = DRAG_SRC_INVENTORY;
				}
				if (drag_stack.quantity > 1) {
					num_picker->setValueBounds(1, drag_stack.quantity);
					num_picker->visible = true;
				}
			}
			// rearrange item
			else if (slotClick == WidgetSlot::CHECKED && !drag_stack.empty()) {
				inv->drop(src_slot, drag_stack);
				inv_slot->checked = false;
				drag_src = DRAG_SRC_NONE;
				drag_stack.clear();
				keyboard_dragging = false;
				sticky_dragging = false;
			}
			// sell, stash, or use item
			else if (slotClick == WidgetSlot::ACTIVATED && !drag_stack.empty()) {
				if (vendor->visible && inv->sell(drag_stack)) {
					vendor->setTab(ItemManager::VENDOR_SELL);
					vendor->add(drag_stack);
				}
				else if (stash->visible) {
					if (!stash->add(drag_stack, MenuStash::NO_SLOT, MenuStash::ADD_PLAY_SOUND)) {
						inv->itemReturn(stash->drop_stack.front());
						stash->drop_stack.pop();
					}
				}
				else {
					inv->itemReturn(drag_stack);
					if (!vendor->visible && !stash->visible)
						inv->activate(src_slot);
				}
				inv->clearHighlight();
				drag_src = DRAG_SRC_NONE;
				drag_stack.clear();
				keyboard_dragging = false;
				sticky_dragging = false;
			}
		}
	}

	// vendor menu
	if (vendor->visible && vendor->getCurrentTabList() && vendor->getCurrentTabList() != (&vendor->tablist) && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = vendor->getCurrentTabList()->getCurrent();
		Point src_slot;
		WidgetSlot * vendor_slot;

		if (vendor->getTab() == ItemManager::VENDOR_SELL)
			vendor_slot = vendor->stock[ItemManager::VENDOR_SELL].slots[slot_index];
		else
			vendor_slot = vendor->stock[ItemManager::VENDOR_BUY].slots[slot_index];

		src_slot.x = vendor_slot->pos.x;
		src_slot.y = vendor_slot->pos.y;

		WidgetSlot::CLICK_TYPE slotClick = vendor_slot->checkClick();

		// buy item
		if (slotClick == WidgetSlot::CHECKED && drag_stack.empty()) {
			drag_stack = vendor->click(src_slot);
			if (!drag_stack.empty()) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_VENDOR;
				vendor->lockTabControl();
			}
			if (drag_stack.quantity > 1) {
				int max_quantity = std::min(inv->getMaxPurchasable(drag_stack, vendor->getTab()), drag_stack.quantity);
				if (max_quantity >= 1) {
					num_picker->setValueBounds(1, max_quantity);
					num_picker->visible = true;
				}
				else {
					drag_stack.clear();
					resetDrag();
				}
			}
		}

		// if we selected a single item buy it imediately
		// otherwise, wait until we get a result from num_picker
		if (vendor_slot->checked && !drag_stack.empty() && !num_picker->visible) {
			if (!inv->buy(drag_stack, vendor->getTab(), !MenuInventory::IS_DRAGGING)) {
				vendor->itemReturn(inv->drop_stack.front());
				inv->drop_stack.pop();
			}
			drag_src = DRAG_SRC_NONE;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
			vendor_slot->checked = false;
			vendor->unlockTabControl();
		}
	}

	// stash menu
	if (stash->visible && stash->getCurrentTabList() && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = stash->getCurrentTabList()->getCurrent();
		int tab = stash->getTab();
		WidgetSlot::CLICK_TYPE slotClick = stash->stock[tab].slots[slot_index]->checkClick();
		Point src_slot(stash->stock[tab].slots[slot_index]->pos.x, stash->stock[tab].slots[slot_index]->pos.y);

		// pick up item
		if (slotClick == WidgetSlot::CHECKED && drag_stack.empty()) {
			drag_stack = stash->click(src_slot);
			if (!drag_stack.empty()) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_STASH;
			}
			if (drag_stack.quantity > 1) {
				num_picker->setValueBounds(1, drag_stack.quantity);
				num_picker->visible = true;
			}
		}
		// rearrange item
		else if (slotClick == WidgetSlot::CHECKED && !drag_stack.empty()) {
			stash->stock[tab].slots[slot_index]->checked = false;
			if (!stash->drop(src_slot, drag_stack)) {
				drop_stack.push(stash->drop_stack.front());
				stash->drop_stack.pop();
			}
			drag_src = DRAG_SRC_NONE;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
		}
		// send to inventory
		else if (slotClick == WidgetSlot::ACTIVATED && !drag_stack.empty()) {
			if (!inv->add(drag_stack, MenuInventory::CARRIED, ItemStorage::NO_SLOT, MenuInventory::ADD_PLAY_SOUND, MenuInventory::ADD_AUTO_EQUIP)) {
				stash->itemReturn(inv->drop_stack.front());
				inv->drop_stack.pop();
			}
			drag_src = DRAG_SRC_NONE;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
			stash->updated = true;
		}
	}

	// powers menu
	if (pow->visible && pow->isTabListSelected() && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = pow->getSelectedCellIndex();
		WidgetSlot::CLICK_TYPE slotClick = pow->slots[slot_index]->checkClick();

		if (slotClick == WidgetSlot::CHECKED) {
			Point src_slot(pow->slots[slot_index]->pos.x, pow->slots[slot_index]->pos.y);
			// check for unlock/dragging
			drag_power = pow->click(src_slot);
			if (drag_power > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_POWERS;
			}
			else {
				pow->slots[slot_index]->checked = false;
			}
		}
		// clear power dragging if power slot was pressed twice
		else if (slotClick == WidgetSlot::ACTIVATED) {
			if (drag_power > 0) {
				pow->upgradeBySlotIndex(slot_index);
			}
			drag_src = DRAG_SRC_NONE;
			drag_power = 0;
			keyboard_dragging = false;
		}
	}

	// actionbar
	if (act->getCurrentTabList() && static_cast<unsigned>(act->getCurrentTabList()->getCurrent()) < act->slots.size()) {
		int slot_index = act->getCurrentTabList()->getCurrent();
		WidgetSlot::CLICK_TYPE slotClick = act->slots[slot_index]->checkClick();
		Point dest_slot = act->getSlotPos(slot_index);

		// pick up power
		if (slotClick == WidgetSlot::CHECKED && drag_stack.empty() && drag_power == 0) {
			drag_power = act->checkDrag(dest_slot);
			if (drag_power > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_ACTIONBAR;
			}
			else {
				act->slots[slot_index]->deactivate();
			}
		}
		// drop power/item from other menu
		else if (slotClick == WidgetSlot::CHECKED && drag_src != DRAG_SRC_ACTIONBAR && (!drag_stack.empty() || drag_power > 0)) {
			if (drag_src == DRAG_SRC_POWERS) {
				act->drop(dest_slot, drag_power, !MenuActionBar::REORDER);
				pow->slots[slot_index]->checked = false;
			}
			else if (drag_src == DRAG_SRC_INVENTORY) {
				if (slot_index< inv->getEquippedCount())
					inv->inventory[MenuInventory::EQUIPMENT].slots[slot_index]->checked = false;
				else
					inv->inventory[MenuInventory::CARRIED].slots[slot_index - inv->getEquippedCount()]->checked = false;

				if (items->items[drag_stack.item].power != 0) {
					act->drop(dest_slot, items->items[drag_stack.item].power, !MenuActionBar::REORDER);
				}
			}
			act->slots[slot_index]->checked = false;
			resetDrag();
			inv->applyEquipment();
		}
		// rearrange actionbar
		else if ((slotClick == WidgetSlot::CHECKED || slotClick == WidgetSlot::ACTIVATED) && drag_src == DRAG_SRC_ACTIONBAR && drag_power > 0) {
			if (slotClick == WidgetSlot::CHECKED) act->slots[slot_index]->checked = false;
			act->drop(dest_slot, drag_power, MenuActionBar::REORDER);
			drag_src = DRAG_SRC_NONE;
			drag_power = 0;
			keyboard_dragging = false;
			inpt->lock[Input::ACCEPT] = false;
		}
	}
}

void MenuManager::resetDrag() {
	if (drag_src == DRAG_SRC_VENDOR) {
		vendor->itemReturn(drag_stack);
		vendor->unlockTabControl();
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_STASH) {
		stash->itemReturn(drag_stack);
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_INVENTORY) {
		inv->itemReturn(drag_stack);
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_ACTIONBAR) act->actionReturn(drag_power);
	drag_src = DRAG_SRC_NONE;
	drag_stack.clear();
	drag_power = 0;

	if (keyboard_dragging && drag_src == DRAG_SRC_ACTIONBAR) {
		inpt->lock[Input::ACCEPT] = false;
	}

	setDragIcon(WidgetSlot::NO_ICON, WidgetSlot::NO_OVERLAY);

	vendor->stock[ItemManager::VENDOR_BUY].drag_prev_slot = -1;
	vendor->stock[ItemManager::VENDOR_SELL].drag_prev_slot = -1;
	stash->stock[MenuStash::STASH_PRIVATE].drag_prev_slot = -1;
	stash->stock[MenuStash::STASH_SHARED].drag_prev_slot = -1;
	inv->drag_prev_src = -1;
	inv->inventory[MenuInventory::EQUIPMENT].drag_prev_slot = -1;
	inv->inventory[MenuInventory::CARRIED].drag_prev_slot = -1;

	keyboard_dragging = false;
	mouse_dragging = false;
	sticky_dragging = false;
}

void MenuManager::render() {
	if (!settings->show_hud) {
		// if the hud is disabled, only show a few necessary menus

		// exit menu
		menus[9]->render();

		// dev console
		if (settings->dev_mode)
			devconsole->render();

		return;
	}

	bool hudlog_overlapped = false;
	if (chr->visible && Utils::rectsOverlap(hudlog->window_area, chr->window_area)) {
		hudlog_overlapped = true;
	}
	if (questlog->visible && Utils::rectsOverlap(hudlog->window_area, questlog->window_area)) {
		hudlog_overlapped = true;
	}
	if (inv->visible && Utils::rectsOverlap(hudlog->window_area, inv->window_area)) {
		hudlog_overlapped = true;
	}
	if (pow->visible && Utils::rectsOverlap(hudlog->window_area, pow->window_area)) {
		hudlog_overlapped = true;
	}
	if (vendor->visible && Utils::rectsOverlap(hudlog->window_area, vendor->window_area)) {
		hudlog_overlapped = true;
	}
	if (stash->visible && Utils::rectsOverlap(hudlog->window_area, stash->window_area)) {
		hudlog_overlapped = true;
	}
	if (talker->visible && Utils::rectsOverlap(hudlog->window_area, talker->window_area)) {
		hudlog_overlapped = true;
	}

	for (size_t i=0; i<menus.size(); i++) {
		if (menus[i] == hudlog && hudlog_overlapped && !hudlog->hide_overlay) {
			continue;
		}

		menus[i]->render();
	}

	if (hudlog_overlapped && !hudlog->hide_overlay) {
		hudlog->renderOverlay();
	}

	subtitles->render();

	touch_controls->render();

	if (!num_picker->visible && !mouse_dragging && !sticky_dragging) {
		if (!inpt->usingMouse() || settings->touchscreen)
			handleKeyboardTooltips();
		else {
			// Find tooltips depending on mouse position
			if (!book->visible) {
				chr->renderTooltips(inpt->mouse);
				vendor->renderTooltips(inpt->mouse);
				stash->renderTooltips(inpt->mouse);
				pow->renderTooltips(inpt->mouse);
				inv->renderTooltips(inpt->mouse);
			}
			if (!exit->visible) {
				effects->renderTooltips(inpt->mouse);
				act->renderTooltips(inpt->mouse);
			}
		}
	}

	// draw icon under cursor if dragging
	if (mouse_dragging && !num_picker->visible) {
		if (drag_src == DRAG_SRC_INVENTORY || drag_src == DRAG_SRC_VENDOR || drag_src == DRAG_SRC_STASH)
			setDragIconItem(drag_stack);
		else if (drag_src == DRAG_SRC_POWERS || drag_src == DRAG_SRC_ACTIONBAR)
			setDragIcon(powers->powers[drag_power].icon, -1);

		if (settings->touchscreen && sticky_dragging)
			renderIcon(keydrag_pos.x - eset->resolutions.icon_size/2, keydrag_pos.y - eset->resolutions.icon_size/2);
		else
			renderIcon(inpt->mouse.x - eset->resolutions.icon_size/2, inpt->mouse.y - eset->resolutions.icon_size/2);
	}
	else if (keyboard_dragging && !num_picker->visible) {
		if (drag_src == DRAG_SRC_INVENTORY || drag_src == DRAG_SRC_VENDOR || drag_src == DRAG_SRC_STASH)
			setDragIconItem(drag_stack);
		else if (drag_src == DRAG_SRC_POWERS || drag_src == DRAG_SRC_ACTIONBAR)
			setDragIcon(powers->powers[drag_power].icon, -1);

		renderIcon(keydrag_pos.x - eset->resolutions.icon_size/2, keydrag_pos.y - eset->resolutions.icon_size/2);
	}

	// render the dev console above everything else
	if (settings->dev_mode) {
		devconsole->render();
	}
}

void MenuManager::handleKeyboardTooltips() {
	if (book->visible)
		return;

	if (vendor->visible && vendor->getCurrentTabList() && vendor->getCurrentTabList() != (&vendor->tablist)) {
		int slot_index = vendor->getCurrentTabList()->getCurrent();

		if (vendor->getTab() == ItemManager::VENDOR_BUY) {
			keydrag_pos.x = vendor->stock[ItemManager::VENDOR_BUY].slots[slot_index]->pos.x;
			keydrag_pos.y = vendor->stock[ItemManager::VENDOR_BUY].slots[slot_index]->pos.y;
		}
		else if (vendor->getTab() == ItemManager::VENDOR_SELL) {
			keydrag_pos.x = vendor->stock[ItemManager::VENDOR_SELL].slots[slot_index]->pos.x;
			keydrag_pos.y = vendor->stock[ItemManager::VENDOR_SELL].slots[slot_index]->pos.y;
		}

		vendor->renderTooltips(keydrag_pos);
	}

	if (stash->visible && stash->getCurrentTabList()) {
		int slot_index = stash->getCurrentTabList()->getCurrent();
		int tab = stash->getTab();

		keydrag_pos.x = stash->stock[tab].slots[slot_index]->pos.x;
		keydrag_pos.y = stash->stock[tab].slots[slot_index]->pos.y;

		stash->renderTooltips(keydrag_pos);
	}

	if (pow->visible && pow->isTabListSelected()) {
		int slot_index = pow->getSelectedCellIndex();

		keydrag_pos.x = pow->slots[slot_index]->pos.x;
		keydrag_pos.y = pow->slots[slot_index]->pos.y;

		pow->renderTooltips(keydrag_pos);
	}

	if (inv->visible && inv->getCurrentTabList()) {
		int slot_index = inv->getCurrentTabList()->getCurrent();

		if (slot_index < inv->getEquippedCount()) {
			keydrag_pos.x = inv->inventory[MenuInventory::EQUIPMENT].slots[slot_index]->pos.x;
			keydrag_pos.y = inv->inventory[MenuInventory::EQUIPMENT].slots[slot_index]->pos.y;
		}
		else if (slot_index < inv->getTotalSlotCount()) {
			keydrag_pos.x = inv->inventory[MenuInventory::CARRIED].slots[slot_index - inv->getEquippedCount()]->pos.x;
			keydrag_pos.y = inv->inventory[MenuInventory::CARRIED].slots[slot_index - inv->getEquippedCount()]->pos.y;
		}
		else {
			Widget *temp_widget = inv->getCurrentTabList()->getWidgetByIndex(slot_index);
			keydrag_pos.x = temp_widget->pos.x;
			keydrag_pos.y = temp_widget->pos.y;
		}

		inv->renderTooltips(keydrag_pos);
	}

	if (act_drag_hover && act->getCurrentTabList()) {
		int slot_index = act->getCurrentTabList()->getCurrent();

		keydrag_pos = act->getSlotPos(slot_index);

		act->renderTooltips(keydrag_pos);
	}
}

void MenuManager::closeAll() {
	closeLeft();
	closeRight();
}

void MenuManager::closeLeft() {
	resetDrag();
	chr->visible = false;
	questlog->visible = false;
	exit->visible = false;
	stash->visible = false;
	book->closeWindow();
	book->book_name = "";
	num_picker->visible = false;

	talker->setNPC(NULL);
	vendor->setNPC(NULL);

	if (settings->dev_mode && devconsole->visible) {
		devconsole->closeWindow();
	}
}

void MenuManager::closeRight() {
	resetDrag();
	inv->visible = false;
	pow->visible = false;
	exit->visible = false;
	book->closeWindow();
	book->book_name = "";
	num_picker->visible = false;

	talker->setNPC(NULL);

	if (settings->dev_mode && devconsole->visible) {
		devconsole->closeWindow();
	}
}

bool MenuManager::isDragging() {
	return drag_src != DRAG_SRC_NONE;
}

bool MenuManager::isNPCMenuVisible() {
	return talker->visible || vendor->visible;
}

void MenuManager::showExitMenu() {
	pause = true;
	closeAll();
	if (exit)
		exit->visible = true;
}

MenuManager::~MenuManager() {

	delete hp;
	delete mp;
	delete xp;
	delete mini;
	delete inv;
	delete pow;
	delete chr;
	delete hudlog;
	delete questlog;
	delete act;
	delete vendor;
	delete talker;
	delete exit;
	delete enemy;
	delete effects;
	delete stash;
	delete book;
	delete num_picker;

	if (settings->dev_mode) {
		delete devconsole;
	}

	delete touch_controls;

	delete subtitles;

	delete drag_icon;
}
