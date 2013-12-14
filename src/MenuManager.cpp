/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert

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

#include "FileParser.h"
#include "UtilsParsing.h"
#include "UtilsFileSystem.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuActionBar.h"
#include "MenuCharacter.h"
#include "MenuStatBar.h"
#include "MenuHUDLog.h"
#include "MenuInventory.h"
#include "MenuMiniMap.h"
#include "MenuNPCActions.h"
#include "MenuPowers.h"
#include "MenuEnemy.h"
#include "MenuVendor.h"
#include "MenuTalker.h"
#include "MenuExit.h"
#include "MenuActiveEffects.h"
#include "MenuStash.h"
#include "MenuLog.h"
#include "ModManager.h"
#include "NPC.h"
#include "SharedResources.h"
#include "WidgetTooltip.h"
#include "SharedGameResources.h"

MenuManager::MenuManager(StatBlock *_stats)
	: stats(_stats)
	, tip_buf()
	, keyb_tip_buf_vendor()
	, keyb_tip_buf_stash()
	, keyb_tip_buf_pow()
	, keyb_tip_buf_inv()
	, keyb_tip_buf_act()
	, key_lock(false)
	, mouse_dragging(0)
	, keyboard_dragging(0)
	, drag_stack()
	, drag_power(0)
	, drag_src(0)
	, done(false)
	, act_drag_hover(false)
	, keydrag_pos(Point())
/*std::vector<Menu*> menus;*/
	, inv(NULL)
	, pow(NULL)
	, chr(NULL)
	, log(NULL)
	, hudlog(NULL)
	, act(NULL)
	, hp(NULL)
	, mp(NULL)
	, xp(NULL)
	, tip(NULL)
	, mini(NULL)
	, npc(NULL)
	, enemy(NULL)
	, vendor(NULL)
	, talker(NULL)
	, exit(NULL)
	, effects(NULL)
	, stash(NULL)
	, pause(false)
	, menus_open(false)
	, drop_stack() {

	hp = new MenuStatBar("hp");
	menus.push_back(hp); // menus[0]
	mp = new MenuStatBar("mp");
	menus.push_back(mp); // menus[1]
	xp = new MenuStatBar("xp");
	menus.push_back(xp); // menus[2]
	effects = new MenuActiveEffects();
	menus.push_back(effects); // menus[3]
	hudlog = new MenuHUDLog();
	menus.push_back(hudlog); // menus[4]
	act = new MenuActionBar(pc);
	menus.push_back(act); // menus[5]
	enemy = new MenuEnemy();
	menus.push_back(enemy); // menus[6]
	vendor = new MenuVendor(stats);
	menus.push_back(vendor); // menus[7]
	talker = new MenuTalker(this);
	menus.push_back(talker); // menus[8]
	exit = new MenuExit();
	menus.push_back(exit); // menus[9]
	mini = new MenuMiniMap();
	menus.push_back(mini); // menus[10]
	chr = new MenuCharacter(stats);
	menus.push_back(chr); // menus[11]
	inv = new MenuInventory(stats);
	menus.push_back(inv); // menus[12]
	pow = new MenuPowers(stats, act);
	menus.push_back(pow); // menus[13]
	log = new MenuLog();
	menus.push_back(log); // menus[14]
	stash = new MenuStash(stats);
	menus.push_back(stash); // menus[15]
	npc = new MenuNPCActions();
	menus.push_back(npc); // menus[16]

	tip = new WidgetTooltip();

	// Load the menu layout and sound effects from menus/menus.txt
	FileParser infile;
	if (infile.open("menus/menus.txt")) {

		int menu_index = -1;

		while (infile.next()) {

			if (infile.key == "id") {

				/* finalize previously parsed menu */
				if (menu_index != -1)
					menus[menu_index]->align();

				if (infile.val == "hp") menu_index = 0;
				else if (infile.val == "mp") menu_index = 1;
				else if (infile.val == "xp") menu_index = 2;
				else if (infile.val == "effects") menu_index = 3;
				else if (infile.val == "hudlog") menu_index = 4;
				else if (infile.val == "actionbar") menu_index = 5;
				else if (infile.val == "enemy") menu_index = 6;
				else if (infile.val == "vendor") menu_index = 7;
				else if (infile.val == "talker") menu_index = 8;
				else if (infile.val == "exit") menu_index = 9;
				else if (infile.val == "minimap") menu_index = 10;
				else if (infile.val == "character") menu_index = 11;
				else if (infile.val == "inventory") menu_index = 12;
				else if (infile.val == "powers") menu_index = 13;
				else if (infile.val == "log") menu_index = 14;
				else if (infile.val == "stash") menu_index = 15;
				else if (infile.val == "npc") menu_index = 16;
				else menu_index = -1;

			}

			if (menu_index == -1)
				continue;

			if (infile.key == "layout") {

				infile.val = infile.val + ',';
				int x = eatFirstInt(infile.val, ',');
				int y = eatFirstInt(infile.val, ',');
				int w = eatFirstInt(infile.val, ',');
				int h = eatFirstInt(infile.val, ',');

				menus[menu_index]->window_area.x = x;
				menus[menu_index]->window_area.y = y;
				menus[menu_index]->window_area.w = w;
				menus[menu_index]->window_area.h = h;

			}
			else if (infile.key == "align") {
				menus[menu_index]->alignment = infile.val;
			}
			else if (infile.key == "soundfx_open") {
				menus[menu_index]->sfx_open = snd->load(infile.val, "MenuManager open tab");
			}
			else if (infile.key == "soundfx_close") {
				menus[menu_index]->sfx_close = snd->load(infile.val, "MenuManager close tab");
			}

		}

		infile.close();
	}

	// Some menus need to be updated to apply their new dimensions
	act->update();
	vendor->update();
	vendor->buyback_stock.init(NPC_VENDOR_MAX_STOCK);
	talker->update();
	exit->update();
	chr->update();
	inv->update();
	pow->update();
	log->update();
	stash->update();

	pause = false;
	mouse_dragging = false;
	keyboard_dragging = false;
	drag_stack.item = 0;
	drag_stack.quantity = 0;
	drag_power = 0;
	drag_src = 0;
	drop_stack.item = 0;
	drop_stack.quantity = 0;

	done = false;

	closeAll(); // make sure all togglable menus start closed
}

void MenuManager::renderIcon(int icon_id, int x, int y) {
	SDL_Rect src;
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	src.w = src.h = dest.w = dest.h = ICON_SIZE;

	int columns = icons.getGraphicsWidth() / ICON_SIZE;
	src.x = (icon_id % columns) * ICON_SIZE;
	src.y = (icon_id / columns) * ICON_SIZE;

	icons.setClip(src);
	icons.setDest(dest);
	render_device->render(icons);
}

void MenuManager::handleKeyboardNavigation() {
	// switching between menus
	if (drag_src == 0) {
		const int VENDOR_ROWS = vendor->getRowsCount() * 2; //Vendor Menu has two tabs
		const int STASH_ROWS = stash->getRowsCount();
		const int INVENTORY_ROWS = inv->getCarriedRows();
		const int EQUIPPED_SLOTS = inv->getEquippedCount();

		// left -> right
		if (inv->visible || pow->visible) {
			if (vendor->visible && vendor->tablist.getCurrent() != -1 && !vendor->tablist.isLocked()) {
				if (((vendor->tablist.getCurrent() + 1) % (vendor->tablist.size()/VENDOR_ROWS) == 0) &&
						inpt->pressing[RIGHT] && !inpt->lock[RIGHT]) {
					inpt->lock[RIGHT] = true;
					vendor->tablist.lock();
					vendor->tablist.defocus();
					inv->tablist.unlock();
					inv->tablist.getNext();
				}
			}
			else if (stash->visible && stash->tablist.getCurrent() != -1 && !stash->tablist.isLocked()) {
				if (((stash->tablist.getCurrent() + 1) % (stash->tablist.size()/STASH_ROWS) == 0) &&
						inpt->pressing[RIGHT] && !inpt->lock[RIGHT]) {
					inpt->lock[RIGHT] = true;
					stash->tablist.lock();
					stash->tablist.defocus();
					inv->tablist.unlock();
					inv->tablist.getNext();
				}
			}
			else if (chr->visible && chr->tablist.getCurrent() != -1 && !chr->tablist.isLocked()) {
				if ((chr->tablist.getCurrent() + 1 == (int)chr->tablist.size()) &&
						inpt->pressing[RIGHT] && !inpt->lock[RIGHT]) {
					inpt->lock[RIGHT] = true;
					chr->tablist.lock();
					chr->tablist.defocus();
					if (inv->visible) {
						inv->tablist.unlock();
						inv->tablist.getNext();
					}
					else if (pow->visible) {
						pow->tablist.unlock();
						pow->tablist.getNext();
					}
				}
			}
			else if (log->visible && log->tablist.getCurrent() != -1 && !log->tablist.isLocked()) {
				if (inpt->pressing[RIGHT] && !inpt->lock[RIGHT]) {
					inpt->lock[RIGHT] = true;
					log->tablist.lock();
					log->tablist.defocus();
					if (inv->visible) {
						inv->tablist.unlock();
						inv->tablist.getNext();
					}
					else if (pow->visible) {
						pow->tablist.unlock();
						pow->tablist.getNext();
					}
				}
			}
		}
		// right -> left
		if (vendor->visible || stash->visible || chr->visible || log->visible) {
			if (inv->visible && (inv->tablist.getCurrent() - EQUIPPED_SLOTS) >= 0 && !inv->tablist.isLocked()) {
				if (((inv->tablist.getCurrent() - EQUIPPED_SLOTS + 1) % ((inv->tablist.size() - EQUIPPED_SLOTS)/INVENTORY_ROWS) == 1) &&
						inpt->pressing[LEFT] && !inpt->lock[LEFT]) {
					inpt->lock[LEFT] = true;
					inv->tablist.lock();
					inv->tablist.defocus();
					if (stash->visible) {
						stash->tablist.unlock();
						stash->tablist.getPrev();
					}
					else if (vendor->visible) {
						vendor->tablist.unlock();
						vendor->tablist.getPrev();
					}
					else if (chr->visible) {
						chr->tablist.unlock();
						chr->tablist.getNext();
					}
					else if (log->visible) {
						log->tablist.unlock();
						log->tablist.getNext();
					}
				}
			}
			else if (pow->visible && pow->tablist.getCurrent() != -1 && !pow->tablist.isLocked()) {
				if (pow->tablist.getCurrent() == 0 && inpt->pressing[LEFT] && !inpt->lock[LEFT]) {
					inpt->lock[LEFT] = true;
					pow->tablist.lock();
					pow->tablist.defocus();
					if (chr->visible) {
						chr->tablist.unlock();
						chr->tablist.getNext();
					}
					else if (log->visible) {
						log->tablist.unlock();
						log->tablist.getNext();
					}
				}
			}
		}
	}

	// unlock menus if only one side is showing
	if (!inv->visible && !pow->visible) {
		stash->tablist.unlock();
		vendor->tablist.unlock();
		chr->tablist.unlock();
		log->tablist.unlock();
	}
	else if (!vendor->visible && ! stash->visible && !chr->visible && !log->visible) {
		inv->tablist.unlock();
		pow->tablist.unlock();
	}

	// lock left and right where buy/sell slots meet
	if (vendor->visible && drag_src != 0) {
		if (vendor->tablist.getCurrent() == 0 || vendor->tablist.getCurrent() == (int)vendor->tablist.size()/2)
			inpt->lock[LEFT] = true;
		if (vendor->tablist.getCurrent() == (int)vendor->tablist.size()-1 || vendor->tablist.getCurrent() == (int)vendor->tablist.size()/2 - 1)
			inpt->lock[RIGHT] = true;
	}

	// UP/DOWN scrolling in vendor menu
	if (vendor->visible && !vendor->tablist.isLocked()) {
		int VENDOR_ROWS = vendor->getRowsCount() * 2;
		int VENDOR_COLS = vendor->tablist.size()/VENDOR_ROWS;

		bool buy_down = vendor->tablist.getCurrent() >= 0 && vendor->tablist.getCurrent() < (int)vendor->tablist.size()/2-VENDOR_COLS;
		bool sell_down = vendor->tablist.getCurrent() >= (int)vendor->tablist.size()/2 && vendor->tablist.getCurrent() < (int)vendor->tablist.size()-VENDOR_COLS;
		bool buy_up = vendor->tablist.getCurrent() >= VENDOR_COLS && vendor->tablist.getCurrent() < (int)vendor->tablist.size()/2;
		bool sell_up = vendor->tablist.getCurrent() >= (int)vendor->tablist.size()/2+VENDOR_COLS && vendor->tablist.getCurrent() < (int)vendor->tablist.size();

		if (inpt->pressing[DOWN] && !inpt->lock[DOWN]) {
			inpt->lock[DOWN] = true;
			if (drag_src == 0 || buy_down || sell_down) {
				for (unsigned i = 0; i < vendor->tablist.size()/VENDOR_ROWS; i++)
					vendor->tablist.getNext();
			}
		}
		if (inpt->pressing[UP] && !inpt->lock[UP]) {
			inpt->lock[UP] = true;
			if (drag_src == 0 || buy_up || sell_up) {
				for (unsigned i = 0; i < vendor->tablist.size()/VENDOR_ROWS; i++)
					vendor->tablist.getPrev();
			}
		}
	}
	// UP/DOWN scrolling in inventory
	if (inv->visible && !inv->tablist.isLocked()) {
		int INVENTORY_ROWS = inv->getCarriedRows();
		int EQUIPPED_SLOTS = inv->getEquippedCount();

		if (inv->tablist.getCurrent() + 1 > EQUIPPED_SLOTS && inpt->pressing[DOWN] && !inpt->lock[DOWN]) {
			inpt->lock[DOWN] = true;
			for (unsigned i = 0; i < (inv->tablist.size() - EQUIPPED_SLOTS)/INVENTORY_ROWS; i++)
				inv->tablist.getNext();
		}
		if (inv->tablist.getCurrent() + 1 > EQUIPPED_SLOTS && inpt->pressing[UP] && !inpt->lock[UP]) {
			inpt->lock[UP] = true;
			for (unsigned i = 0; i < (inv->tablist.size() - EQUIPPED_SLOTS)/INVENTORY_ROWS; i++)
				inv->tablist.getPrev();
		}
	}
	// UP/DOWN scrolling in stash
	if (stash->visible && !stash->tablist.isLocked()) {
		if (inpt->pressing[DOWN] && !inpt->lock[DOWN]) {
			inpt->lock[DOWN] = true;
			for (unsigned i = 0; i < stash->tablist.size()/stash->getRowsCount(); i++)
				stash->tablist.getNext();
		}
		if (inpt->pressing[UP] && !inpt->lock[UP]) {
			inpt->lock[UP] = true;
			for (unsigned i = 0; i < stash->tablist.size()/stash->getRowsCount(); i++)
				stash->tablist.getPrev();
		}
	}

	// stash and vendor always start locked
	if (!stash->visible) stash->tablist.lock();
	if (!vendor->visible) vendor->tablist.lock();

	// inventory always starts unlocked
	if (!inv->visible) inv->tablist.unlock();

	// position the drag hover icon depending on the last key press
	if (!act_drag_hover && (inpt->pressing[ACTIONBAR_BACK] || inpt->pressing[ACTIONBAR_FORWARD]))
		act_drag_hover = true;
	else if (act_drag_hover && (inpt->pressing[LEFT] || inpt->pressing[RIGHT] || inpt->pressing[UP] || inpt->pressing[DOWN]))
		act_drag_hover = false;

	// don't allow dropping actionbar items in other menus
	if (keyboard_dragging && drag_src == DRAG_SRC_ACTIONBAR) {
		inpt->lock[ACCEPT] = true;
	}
}
void MenuManager::logic() {

	bool clicking_character = false;
	bool clicking_inventory = false;
	bool clicking_powers = false;
	bool clicking_log = false;
	ItemStack stack;

	hp->update(stats->hp,stats->get(STAT_HP_MAX),inpt->mouse,"");
	mp->update(stats->mp,stats->get(STAT_MP_MAX),inpt->mouse,"");
	xp->update((stats->xp - stats->xp_table[stats->level-1]),(stats->xp_table[stats->level] - stats->xp_table[stats->level-1]),inpt->mouse,msg->get("XP: %d/%d", stats->xp, stats->xp_table[stats->level]));
	effects->update(stats);

	if (NO_MOUSE)
		handleKeyboardNavigation();

	act->logic();
	hudlog->logic();
	enemy->logic();
	chr->logic();
	inv->logic();
	vendor->logic();
	pow->logic();
	log->logic();
	talker->logic();
	stash->logic();

	if (chr->checkUpgrade() || stats->level_up) {
		// apply equipment and max hp/mp
		inv->applyEquipment(inv->inventory[EQUIPMENT].storage);
		stats->hp = stats->get(STAT_HP_MAX);
		stats->mp = stats->get(STAT_MP_MAX);
		stats->level_up = false;
	}

	// only allow the vendor window to be open if the inventory is open
	if (vendor->visible && !(inv->visible)) {
		snd->play(vendor->sfx_close);
		closeLeft();
		if (vendor->talker_visible && !(inv->visible))
			closeRight();
	}

	if (!inpt->pressing[INVENTORY] && !inpt->pressing[POWERS] && !inpt->pressing[CHARACTER] && !inpt->pressing[LOG])
		key_lock = false;

	// handle npc action menu
	if (npc->visible) {
		npc->logic();
	}

	// check if mouse-clicking a menu button
	act->checkMenu(clicking_character, clicking_inventory, clicking_powers, clicking_log);

	if (exit->visible) {
		exit->logic();
		if (exit->isExitRequested()) {
			done = true;
		}
	}

	// cancel dragging and defocus menu tablists
	if (!key_lock && inpt->pressing[CANCEL] && !inpt->lock[CANCEL] && !stats->corpse) {
		if (keyboard_dragging || mouse_dragging) {
			inpt->lock[CANCEL] = true;
			resetDrag();
			keyboard_dragging = false;
			mouse_dragging = false;
		}
		if (inv->tablist.getCurrent() != -1 || vendor->tablist.getCurrent() != -1 || stash->tablist.getCurrent() != -1 || act->tablist.getCurrent() != -1 || pow->tablist.getCurrent() != -1 || chr->tablist.getCurrent() != -1 || log->tablist.getCurrent() != -1) {
			inpt->lock[CANCEL] = true;
			inv->tablist.defocus();
			vendor->tablist.defocus();
			stash->tablist.defocus();
			act->tablist.defocus();
			pow->tablist.defocus();
			chr->tablist.defocus();
			log->tablist.defocus();
		}
	}

	// exit menu toggle
	if ((!key_lock && !mouse_dragging && !keyboard_dragging) && !(stats->corpse && stats->permadeath)) {
		if (inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
			inpt->lock[CANCEL] = true;
			key_lock = true;
			if (menus_open) {
				closeAll();
			}
			else {
				exit->visible = !exit->visible;
			}
		}
	}

	// inventory menu toggle
	if ((inpt->pressing[INVENTORY] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_inventory) {
		key_lock = true;
		if (inv->visible) {
			snd->play(inv->sfx_close);
			closeRight();
		}
		else {
			closeRight();
			act->requires_attention[MENU_INVENTORY] = false;
			inv->visible = true;
			snd->play(inv->sfx_open);
		}

	}

	// powers menu toggle
	if (((inpt->pressing[POWERS] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_powers) && !stats->transformed) {
		key_lock = true;
		if (pow->visible) {
			snd->play(pow->sfx_close);
			closeRight();
		}
		else {
			closeRight();
			act->requires_attention[MENU_POWERS] = false;
			pow->visible = true;
			snd->play(pow->sfx_open);
		}
	}
	act->requires_attention[MENU_POWERS] = pow->getUnspent() > 0;

	// character menu toggleggle
	if ((inpt->pressing[CHARACTER] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_character) {
		key_lock = true;
		if (chr->visible) {
			snd->play(chr->sfx_close);
			closeLeft();
		}
		else {
			closeLeft();
			act->requires_attention[MENU_CHARACTER] = false;
			chr->visible = true;
			snd->play(chr->sfx_open);
			// Make sure the stat list isn't scrolled when we open the character menu
			inpt->resetScroll();
		}
	}
	act->requires_attention[MENU_CHARACTER] = chr->getUnspent() > 0;

	// log menu toggle
	if ((inpt->pressing[LOG] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_log) {
		key_lock = true;
		if (log->visible) {
			snd->play(log->sfx_close);
			closeLeft();
		}
		else {
			closeLeft();
			act->requires_attention[MENU_LOG] = false;
			log->visible = true;
			snd->play(log->sfx_open);
			// Make sure the log isn't scrolled when we open the log menu
			inpt->resetScroll();
		}
	}

	menus_open = (inv->visible || pow->visible || chr->visible || log->visible || vendor->visible || talker->visible || npc->visible);
	pause = (MENUS_PAUSE && menus_open) || exit->visible;

	if (stats->alive) {

		// handle right-click
		if (!mouse_dragging && inpt->pressing[MAIN2] && !inpt->lock[MAIN2]) {
			// exit menu
			if (exit->visible && isWithin(exit->window_area, inpt->mouse)) {
				inpt->lock[MAIN2] = true;
			}

			// activate inventory item
			else if (inv->visible && isWithin(inv->window_area, inpt->mouse)) {
				inpt->lock[MAIN2] = true;
				if (isWithin(inv->carried_area, inpt->mouse)) {
					inv->activate(inpt->mouse);
				}
			}
		}

		// handle left-click
		if (!mouse_dragging && inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
			// clear keyboard dragging
			if (keyboard_dragging) {
				resetDrag();
				keyboard_dragging = false;
			}

			// exit menu
			if (exit->visible && isWithin(exit->window_area, inpt->mouse)) {
				inpt->lock[MAIN1] = true;
			}

			if (chr->visible && isWithin(chr->window_area, inpt->mouse)) {
				inpt->lock[MAIN1] = true;
			}

			if (vendor->visible && isWithin(vendor->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
				vendor->tabsLogic();
				if (inpt->pressing[CTRL]) {
					// buy item from a vendor
					stack = vendor->click(inpt->mouse);
					if (stack.item > 0) {
						if (!inv->buy(stack,vendor->getTab())) {
							log->add(msg->get("Not enough %s.", CURRENCY), LOG_TYPE_MESSAGES);
							hudlog->add(msg->get("Not enough %s.", CURRENCY));
							vendor->itemReturn( stack);
						}
						else {
							if (inv->full(stack)) {
								log->add(msg->get("Inventory is full."), LOG_TYPE_MESSAGES);
								hudlog->add(msg->get("Inventory is full."));
								drop_stack = stack;
							}
							else {
								inv->add(stack);
							}
						}
					}
				}
				else {
					// start dragging a vendor item
					drag_stack = vendor->click(inpt->mouse);
					if (drag_stack.item > 0) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_VENDOR;
					}
				}
			}

			if (stash->visible && isWithin(stash->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
				if (inpt->pressing[CTRL]) {
					// take an item from the stash
					stack = stash->click(inpt->mouse);
					if (stack.item > 0) {
						if (inv->full(stack)) {
							log->add(msg->get("Inventory is full."), LOG_TYPE_MESSAGES);
							hudlog->add(msg->get("Inventory is full."));
							splitStack(stack);
						}
						else {
							inv->add(stack);
						}
						stash->updated = true;
					}
				}
				else {
					// start dragging a stash item
					drag_stack = stash->click(inpt->mouse);
					if (drag_stack.item > 0) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_STASH;
					}
				}
			}

			if (log->visible && isWithin(log->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
			}

			// pick up an inventory item
			if (inv->visible && isWithin(inv->window_area,inpt->mouse)) {
				if (inpt->pressing[CTRL]) {
					inpt->lock[MAIN1] = true;
					stack = inv->click(inpt->mouse);
					if (stack.item > 0) {
						if (stash->visible) {
							if (inv->stashAdd(stack) && !stash->full(stack.item)) {
								stash->add(stack);
								stash->updated = true;
							}
							else {
								inv->itemReturn(stack);
							}
						}
						else {
							// The vendor could have a limited amount of currency in the future. It will be tested here.
							if ((SELL_WITHOUT_VENDOR || vendor->visible) && inv->sell(stack)) {
								vendor->setTab(VENDOR_SELL);
								vendor->add(stack);
							}
							else {
								inv->itemReturn(stack);
							}
						}
					}
				}
				else {
					inpt->lock[MAIN1] = true;
					drag_stack = inv->click(inpt->mouse);
					if (drag_stack.item > 0) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_INVENTORY;
					}
				}
			}
			// pick up a power
			if (pow->visible && isWithin(pow->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;

				// check for unlock first
				if (!pow->unlockClick(inpt->mouse)) {

					// otherwise, check for dragging
					drag_power = pow->click(inpt->mouse);
					if (drag_power > 0) {
						mouse_dragging = true;
						keyboard_dragging = false;
						drag_src = DRAG_SRC_POWERS;
					}
				}
			}
			// action bar
			if (isWithin(act->numberArea,inpt->mouse) || isWithin(act->mouseArea,inpt->mouse) || isWithin(act->menuArea, inpt->mouse)) {
				inpt->lock[MAIN1] = true;

				// ctrl-click action bar to clear that slot
				if (inpt->pressing[CTRL]) {
					act->remove(inpt->mouse);
				}
				// allow drag-to-rearrange action bar
				else if (!isWithin(act->menuArea, inpt->mouse)) {
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

		// handle dropping
		if (mouse_dragging && !inpt->pressing[MAIN1]) {

			// putting a power on the Action Bar
			if (drag_src == DRAG_SRC_POWERS) {
				if (isWithin(act->numberArea,inpt->mouse) || isWithin(act->mouseArea,inpt->mouse)) {
					act->drop(inpt->mouse, drag_power, 0);
				}
			}

			// rearranging the action bar
			else if (drag_src == DRAG_SRC_ACTIONBAR) {
				if (isWithin(act->numberArea,inpt->mouse) || isWithin(act->mouseArea,inpt->mouse)) {
					act->drop(inpt->mouse, drag_power, 1);
					// for locked slots forbid power dropping
				}
				else if (act->locked[act->drag_prev_slot]) {
					act->hotkeys[act->drag_prev_slot] = drag_power;
				}
				drag_power = 0;
			}

			// rearranging inventory or dropping items
			else if (drag_src == DRAG_SRC_INVENTORY) {

				if (inv->visible && isWithin(inv->window_area, inpt->mouse)) {
					inv->drop(inpt->mouse, drag_stack);
				}
				else if (isWithin(act->numberArea,inpt->mouse) || isWithin(act->mouseArea,inpt->mouse)) {
					// The action bar is not storage!
					inv->itemReturn(drag_stack);

					// put an item with a power on the action bar
					if (items->items[drag_stack.item].power != 0) {
						act->drop(inpt->mouse, items->items[drag_stack.item].power, false);
					}
				}
				else if (vendor->visible && isWithin(vendor->slots_area, inpt->mouse)) {
					if (inv->sell( drag_stack)) {
						vendor->setTab(VENDOR_SELL);
						vendor->add( drag_stack);
					}
					else {
						inv->itemReturn(drag_stack);
					}
				}
				else if (stash->visible && isWithin(stash->slots_area, inpt->mouse)) {
					if (inv->stashAdd( drag_stack) && !stash->full(drag_stack.item)) {
						stash->drop(inpt->mouse, drag_stack);
						stash->updated = true;
					}
					else {
						inv->itemReturn(drag_stack);
					}
				}
				else {
					// if dragging and the source was inventory, drop item to the floor

					// quest items cannot be dropped
					if (items->items[drag_stack.item].type != "quest") {
						drop_stack = drag_stack;
						inv->clearHighlight();
					}
					else {
						inv->itemReturn(drag_stack);
					}
				}
			}

			else if (drag_src == DRAG_SRC_VENDOR) {

				// dropping an item from vendor (we only allow to drop into the carried area)
				if (inv->visible && isWithin( inv->carried_area, inpt->mouse)) {
					if (!inv->buy(drag_stack,vendor->getTab())) {
						log->add(msg->get("Not enough %s.", CURRENCY), LOG_TYPE_MESSAGES);
						hudlog->add(msg->get("Not enough %s.", CURRENCY));
						vendor->itemReturn( drag_stack);
					}
					else {
						if (inv->full(drag_stack)) {
							log->add(msg->get("Inventory is full."), LOG_TYPE_MESSAGES);
							hudlog->add(msg->get("Inventory is full."));
							drop_stack = drag_stack;
						}
						else {
							inv->drop(inpt->mouse,drag_stack);
						}
					}
				}
				else {
					vendor->itemReturn(drag_stack);
				}
			}

			else if (drag_src == DRAG_SRC_STASH) {

				// dropping an item from stash (we only allow to drop into the carried area)
				if (inv->visible && isWithin( inv->carried_area, inpt->mouse)) {
					if (inv->full(drag_stack)) {
						log->add(msg->get("Inventory is full."), LOG_TYPE_MESSAGES);
						hudlog->add(msg->get("Inventory is full."));
						splitStack(drag_stack);
					}
					else {
						inv->drop(inpt->mouse,drag_stack);
					}
					stash->updated = true;
				}
				else if (stash->visible && isWithin(stash->slots_area, inpt->mouse)) {
					stash->drop(inpt->mouse,drag_stack);
				}
				else {
					drop_stack = drag_stack;
				}
			}

			drag_stack.item = 0;
			drag_stack.quantity = 0;
			drag_power = 0;
			drag_src = 0;
			mouse_dragging = false;
		}
		if (NO_MOUSE)
			dragAndDropWithKeyboard();
	}
	else {
		if (mouse_dragging || keyboard_dragging) {
			resetDrag();
			mouse_dragging = false;
			keyboard_dragging = false;
		}
	}

	// handle equipment changes affecting hero stats
	if (inv->changed_equipment || inv->changed_artifact) {
		inv->applyEquipment(inv->inventory[EQUIPMENT].storage);
		inv->changed_artifact = false;
		// the equipment flag is reset after the new sprites are loaded
	}

	// for action-bar powers that represent items, lookup the current item count
	for (int i=0; i<12; i++) {
		act->slot_enabled[i] = true;
		act->slot_item_count[i] = -1;

		if (act->hotkeys[i] != -1) {
			int item_id = powers->powers[act->hotkeys[i]].requires_item;
			if (item_id != -1 && items->items[item_id].type == "consumable") {
				act->slot_item_count[i] = inv->getItemCountCarried(item_id);
				if (act->slot_item_count[i] == 0) {
					act->slot_enabled[i] = false;
				}
			}
			else if (item_id != -1) {

				// if a non-consumable item power is unequipped, disable that slot
				if (!inv->isItemEquipped(item_id)) {
					act->slot_item_count[i] = 0;
					act->slot_enabled[i] = false;
				}
			}
		}
	}

}

void MenuManager::dragAndDropWithKeyboard() {
	// inventory menu
	if (inv->visible && inv->tablist.getCurrent() != -1 && drag_src != DRAG_SRC_ACTIONBAR) {
		CLICK_TYPE slotClick;
		Point src_slot;
		WidgetSlot * inv_slot;

		if (inv->tablist.getCurrent() < inv->getEquippedCount())
			inv_slot = inv->inventory[EQUIPMENT].slots[inv->tablist.getCurrent()];
		else
			inv_slot = inv->inventory[CARRIED].slots[inv->tablist.getCurrent() - inv->getEquippedCount()];

		src_slot.x = inv_slot->pos.x;
		src_slot.y = inv_slot->pos.y;
		slotClick = inv_slot->checkClick();

		// pick up item
		if (slotClick == CHECKED && drag_stack.item == 0) {
			drag_stack = inv->click(src_slot);
			if (drag_stack.item > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_INVENTORY;
			}
		}
		// rearrange item
		else if (slotClick == CHECKED && drag_stack.item > 0) {
			inv->drop(src_slot, drag_stack);
			inv_slot->checked = false;
			drag_src = 0;
			drag_stack.item = 0;
			keyboard_dragging = false;
		}
		// sell, stash, or use item
		else if (slotClick == ACTIVATED && drag_stack.item > 0) {
			bool not_quest_item = items->items[drag_stack.item].type != "quest";
			if (vendor->visible && inv->sell(drag_stack) && not_quest_item) {
				vendor->setTab(VENDOR_SELL);
				vendor->add(drag_stack);
			}
			else if (stash->visible && !stash->full(drag_stack.item) && not_quest_item) {
				stash->add(drag_stack);
			}
			else {
				inv->itemReturn(drag_stack);
				if (!vendor->visible && !stash->visible)
					inv->activate(src_slot);
			}
			inv->clearHighlight();
			drag_src = 0;
			drag_stack.item = 0;
			keyboard_dragging = false;
		}
	}

	// vendor menu
	if (vendor->visible && vendor->tablist.getCurrent() != -1 && drag_src != DRAG_SRC_ACTIONBAR) {
		CLICK_TYPE slotClick;
		Point src_slot;
		WidgetSlot * vendor_slot;

		if (vendor->tablist.getCurrent() < (int)vendor->tablist.size()/2)
			vendor_slot = vendor->stock[VENDOR_BUY].slots[vendor->tablist.getCurrent()];
		else
			vendor_slot = vendor->stock[VENDOR_SELL].slots[vendor->tablist.getCurrent() - vendor->tablist.size()/2];

		src_slot.x = vendor_slot->pos.x;
		src_slot.y = vendor_slot->pos.y;
		slotClick = vendor_slot->checkClick();

		// buy item
		if (slotClick == CHECKED && drag_stack.item == 0) {
			drag_stack = vendor->click(src_slot);
			if (drag_stack.item > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_VENDOR;
			}
		}
		else if (slotClick == CHECKED && drag_stack.item > 0) {
			vendor->itemReturn(drag_stack);
			vendor_slot->checked = false;
			drag_src = 0;
			drag_stack.item = 0;
			keyboard_dragging = false;
		}
		else if (slotClick == ACTIVATED && drag_stack.item > 0) {
			if (!inv->buy(drag_stack,vendor->getTab())) {
				log->add(msg->get("Not enough %s.", CURRENCY), LOG_TYPE_MESSAGES);
				hudlog->add(msg->get("Not enough %s.", CURRENCY));
				vendor->itemReturn(drag_stack);
			}
			else {
				if (inv->full(drag_stack)) {
					log->add(msg->get("Inventory is full."), LOG_TYPE_MESSAGES);
					hudlog->add(msg->get("Inventory is full."));
					drop_stack = drag_stack;
				}
				else {
					inv->add(drag_stack);
				}
			}
			drag_src = 0;
			drag_stack.item = 0;
			keyboard_dragging = false;
		}
	}

	// stash menu
	if (stash->visible && stash->tablist.getCurrent() != -1 && drag_src != DRAG_SRC_ACTIONBAR) {
		CLICK_TYPE slotClick = stash->stock.slots[stash->tablist.getCurrent()]->checkClick();
		Point src_slot(stash->stock.slots[stash->tablist.getCurrent()]->pos.x, stash->stock.slots[stash->tablist.getCurrent()]->pos.y);

		// pick up item
		if (slotClick == CHECKED && drag_stack.item == 0) {
			drag_stack = stash->click(src_slot);
			if (drag_stack.item > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_STASH;
			}
		}
		// rearrange item
		else if (slotClick == CHECKED && drag_stack.item > 0) {
			stash->stock.slots[stash->tablist.getCurrent()]->checked = false;
			stash->drop(src_slot, drag_stack);
			drag_src = 0;
			drag_stack.item = 0;
			keyboard_dragging = false;
		}
		// send to inventory
		else if (slotClick == ACTIVATED && drag_stack.item > 0) {
			if (!inv->full(drag_stack)) {
				inv->add(drag_stack);
			}
			else {
				log->add(msg->get("Inventory is full."), LOG_TYPE_MESSAGES);
				hudlog->add(msg->get("Inventory is full."));
				splitStack(drag_stack);
			}
			drag_src = 0;
			drag_stack.item = 0;
			keyboard_dragging = false;
		}
	}

	// powers menu
	if (pow->visible && pow->tablist.getCurrent() != -1 && drag_src != DRAG_SRC_ACTIONBAR) {
		CLICK_TYPE slotClick = pow->slots[pow->tablist.getCurrent()]->checkClick();
		if (slotClick == CHECKED) {
			// check for unlock first
			Point src_slot(pow->slots[pow->tablist.getCurrent()]->pos.x, pow->slots[pow->tablist.getCurrent()]->pos.y);
			if (!pow->unlockClick(src_slot)) {

				// otherwise, check for dragging
				drag_power = pow->click(src_slot);
				if (drag_power > 0) {
					keyboard_dragging = true;
					drag_src = DRAG_SRC_POWERS;
				}
			}
			else {
				pow->slots[pow->tablist.getCurrent()]->checked = false;
			}
		}
		// clear power dragging if power slot was pressed twice
		else if (slotClick == ACTIVATED) {
			drag_src = 0;
			drag_power = 0;
			keyboard_dragging = false;
		}
	}

	// actionbar
	if (act->tablist.getCurrent() >= 0 && act->tablist.getCurrent() < 12) {
		CLICK_TYPE slotClick = act->slots[act->tablist.getCurrent()]->checkClick();
		Point dest_slot(act->slots[act->tablist.getCurrent()]->pos.x, act->slots[act->tablist.getCurrent()]->pos.y);

		// pick up power
		if (slotClick == CHECKED && drag_stack.item == 0 && drag_power == 0) {
			drag_power = act->checkDrag(dest_slot);
			if (drag_power > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_ACTIONBAR;
			}
		}
		// drop power/item from other menu
		else if (slotClick == CHECKED && drag_src != DRAG_SRC_ACTIONBAR && (drag_stack.item > 0 || drag_power > 0)) {
			if (drag_src == DRAG_SRC_POWERS) {
				act->drop(dest_slot, drag_power, 0);
				pow->slots[pow->tablist.getCurrent()]->checked = false;
			}
			else if (drag_src == DRAG_SRC_INVENTORY) {
				if (inv->tablist.getCurrent() < inv->getEquippedCount())
					inv->inventory[EQUIPMENT].slots[inv->tablist.getCurrent()]->checked = false;
				else
					inv->inventory[CARRIED].slots[inv->tablist.getCurrent() - inv->getEquippedCount()]->checked = false;

				if (items->items[drag_stack.item].power != 0) {
					act->drop(dest_slot, items->items[drag_stack.item].power, false);
				}
			}
			act->slots[act->tablist.getCurrent()]->checked = false;
			resetDrag();
			keyboard_dragging = false;
		}
		// rearrange actionbar
		else if ((slotClick == CHECKED || slotClick == ACTIVATED) && drag_src == DRAG_SRC_ACTIONBAR && drag_power > 0) {
			if (slotClick == CHECKED) act->slots[act->tablist.getCurrent()]->checked = false;
			act->drop(dest_slot, drag_power, 1);
			drag_src = 0;
			drag_power = 0;
			keyboard_dragging = false;
			inpt->lock[ACCEPT] = false;
		}
	}
}

void MenuManager::resetDrag() {
	if (drag_src == DRAG_SRC_VENDOR) vendor->itemReturn(drag_stack);
	else if (drag_src == DRAG_SRC_STASH) stash->itemReturn(drag_stack);
	else if (drag_src == DRAG_SRC_INVENTORY) inv->itemReturn(drag_stack);
	else if (drag_src == DRAG_SRC_ACTIONBAR) act->actionReturn(drag_power);
	drag_src = 0;
	drag_stack.item = 0;
	drag_stack.quantity = 0;
	drag_power = 0;

	if (keyboard_dragging && DRAG_SRC_ACTIONBAR) {
		inpt->lock[ACCEPT] = false;
	}
}

void MenuManager::render() {
	for (unsigned int i=0; i<menus.size(); i++) {
		menus[i]->render();
	}

	TooltipData tip_new;

	// Find tooltips depending on mouse position
	if (chr->visible && isWithin(chr->window_area,inpt->mouse)) {
		tip_new = chr->checkTooltip();
	}
	if (vendor->visible && isWithin(vendor->window_area,inpt->mouse)) {
		tip_new = vendor->checkTooltip(inpt->mouse);
	}
	if (stash->visible && isWithin(stash->window_area,inpt->mouse)) {
		tip_new = stash->checkTooltip(inpt->mouse);
	}
	if (pow->visible && isWithin(pow->window_area,inpt->mouse)) {
		tip_new = pow->checkTooltip(inpt->mouse);
	}
	if (inv->visible && !mouse_dragging && isWithin(inv->window_area,inpt->mouse)) {
		tip_new = inv->checkTooltip(inpt->mouse);
	}
	if (isWithin(act->window_area,inpt->mouse)) {
		tip_new = act->checkTooltip(inpt->mouse);
	}

	if (!tip_new.isEmpty()) {

		// when we render a tooltip it buffers the rasterized text for performance.
		// If this new tooltip is the same as the existing one, reuse.

		if (!tip_new.compare(&tip_buf)) {
			tip_buf.clear();
			tip_buf = tip_new;
		}
		tip->render(tip_buf, inpt->mouse, STYLE_FLOAT);
		TOOLTIP_CONTEXT = TOOLTIP_MENU;
	}
	else if (TOOLTIP_CONTEXT != TOOLTIP_MAP) {
		TOOLTIP_CONTEXT = TOOLTIP_NONE;
	}

	if (NO_MOUSE)
		handleKeyboardTooltips();

	// draw icon under cursor if dragging
	if (mouse_dragging) {
		if (drag_src == DRAG_SRC_INVENTORY || drag_src == DRAG_SRC_VENDOR || drag_src == DRAG_SRC_STASH)
			items->renderIcon(drag_stack, inpt->mouse.x - ICON_SIZE/2, inpt->mouse.y - ICON_SIZE/2, ICON_SIZE);
		else if (drag_src == DRAG_SRC_POWERS || drag_src == DRAG_SRC_ACTIONBAR)
			renderIcon(powers->powers[drag_power].icon, inpt->mouse.x-ICON_SIZE/2, inpt->mouse.y-ICON_SIZE/2);
	}
	else if (keyboard_dragging) {
		if (drag_src == DRAG_SRC_INVENTORY || drag_src == DRAG_SRC_VENDOR || drag_src == DRAG_SRC_STASH)
			items->renderIcon(drag_stack, keydrag_pos.x - ICON_SIZE/2, keydrag_pos.y - ICON_SIZE/2, ICON_SIZE);
		else if (drag_src == DRAG_SRC_POWERS || drag_src == DRAG_SRC_ACTIONBAR)
			renderIcon(powers->powers[drag_power].icon, keydrag_pos.x-ICON_SIZE/2, keydrag_pos.y-ICON_SIZE/2);
	}

}

void MenuManager::handleKeyboardTooltips() {

	TooltipData keyb_tip_new_vendor;
	TooltipData keyb_tip_new_stash;
	TooltipData keyb_tip_new_pow;
	TooltipData keyb_tip_new_inv;
	TooltipData keyb_tip_new_act;

	if (vendor->visible && vendor->tablist.getCurrent() != -1) {
		if (vendor->tablist.getCurrent() < (int)vendor->tablist.size()/2) {
			keydrag_pos.x = vendor->stock[VENDOR_BUY].slots[vendor->tablist.getCurrent()]->pos.x;
			keydrag_pos.y = vendor->stock[VENDOR_BUY].slots[vendor->tablist.getCurrent()]->pos.y;
		}
		else {
			keydrag_pos.x = vendor->stock[VENDOR_SELL].slots[vendor->tablist.getCurrent() - (int)vendor->tablist.size()/2]->pos.x;
			keydrag_pos.y = vendor->stock[VENDOR_SELL].slots[vendor->tablist.getCurrent() - (int)vendor->tablist.size()/2]->pos.y;
		}
		keyb_tip_new_vendor = vendor->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_vendor.isEmpty()) {
			if (!keyb_tip_new_vendor.compare(&keyb_tip_buf_vendor)) {
				keyb_tip_buf_vendor.clear();
				keyb_tip_buf_vendor = keyb_tip_new_vendor;
			}
			tip->render(keyb_tip_buf_vendor, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (stash->visible && stash->tablist.getCurrent() != -1) {
		keydrag_pos.x = stash->stock.slots[stash->tablist.getCurrent()]->pos.x;
		keydrag_pos.y = stash->stock.slots[stash->tablist.getCurrent()]->pos.y;
		keyb_tip_new_stash = stash->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_stash.isEmpty()) {
			if (!keyb_tip_new_stash.compare(&keyb_tip_buf_stash)) {
				keyb_tip_buf_stash.clear();
				keyb_tip_buf_stash = keyb_tip_new_stash;
			}
			tip->render(keyb_tip_buf_stash, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (pow->visible && pow->tablist.getCurrent() != -1) {
		keydrag_pos.x = pow->slots[pow->tablist.getCurrent()]->pos.x;
		keydrag_pos.y = pow->slots[pow->tablist.getCurrent()]->pos.y;
		keyb_tip_new_pow = pow->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_pow.isEmpty()) {
			if (!keyb_tip_new_pow.compare(&keyb_tip_buf_pow)) {
				keyb_tip_buf_pow.clear();
				keyb_tip_buf_pow = keyb_tip_new_pow;
			}
			tip->render(keyb_tip_buf_pow, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (inv->visible && inv->tablist.getCurrent() != -1) {
		if (inv->tablist.getCurrent() < inv->getEquippedCount()) {
			keydrag_pos.x = inv->inventory[EQUIPMENT].slots[inv->tablist.getCurrent()]->pos.x;
			keydrag_pos.y = inv->inventory[EQUIPMENT].slots[inv->tablist.getCurrent()]->pos.y;
		}
		else {
			keydrag_pos.x = inv->inventory[CARRIED].slots[inv->tablist.getCurrent() - inv->getEquippedCount()]->pos.x;
			keydrag_pos.y = inv->inventory[CARRIED].slots[inv->tablist.getCurrent() - inv->getEquippedCount()]->pos.y;
		}
		keyb_tip_new_inv = inv->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_inv.isEmpty()) {
			if (!keyb_tip_new_inv.compare(&keyb_tip_buf_inv)) {
				keyb_tip_buf_inv.clear();
				keyb_tip_buf_inv = keyb_tip_new_inv;
			}
			tip->render(keyb_tip_buf_inv, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (act_drag_hover && act->tablist.getCurrent() != -1) {
		keydrag_pos.x = act->slots[act->tablist.getCurrent()]->pos.x;
		keydrag_pos.y = act->slots[act->tablist.getCurrent()]->pos.y;
		keyb_tip_new_act = act->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_act.isEmpty()) {
			if (!keyb_tip_new_act.compare(&keyb_tip_buf_act)) {
				keyb_tip_buf_act.clear();
				keyb_tip_buf_act = keyb_tip_new_act;
			}
			tip->render(keyb_tip_buf_act, keydrag_pos, STYLE_FLOAT);
		}
	}
}

void MenuManager::closeAll() {
	if (!mouse_dragging && !keyboard_dragging) {
		closeLeft();
		closeRight();
		vendor->talker_visible = false;
	}
}

void MenuManager::closeLeft() {
	if (!mouse_dragging && !keyboard_dragging) {
		chr->visible = false;
		log->visible = false;
		vendor->visible = false;
		talker->visible = false;
		exit->visible = false;
		stash->visible = false;
		npc->visible = false;
	}
}

void MenuManager::closeRight() {
	if (!mouse_dragging && !keyboard_dragging) {
		inv->visible = false;
		pow->visible = false;
		talker->visible = false;
		exit->visible = false;
		npc->visible = false;
	}
}

bool MenuManager::isDragging() {
	return drag_src != 0;
}

/**
 * Splits an item stack between the stash and the inventory when the latter is full
 */
void MenuManager::splitStack(ItemStack stack) {
	if (stack.item == 0) return;

	if (items->items[stack.item].max_quantity > 1) {
		inv->add(stack);
		stash->add(inv->drop_stack);
		inv->drop_stack.item = 0;
		inv->drop_stack.quantity = 0;
	}
	else {
		stash->itemReturn(stack);
	}
}

MenuManager::~MenuManager() {

	tip_buf.clear();

	delete hp;
	delete mp;
	delete xp;
	delete mini;
	delete inv;
	delete pow;
	delete chr;
	delete hudlog;
	delete log;
	delete act;
	delete tip;
	delete vendor;
	delete talker;
	delete exit;
	delete enemy;
	delete effects;
	delete stash;
	delete npc;
}
