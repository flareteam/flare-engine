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

#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include "CommonIncludes.h"
#include "ItemManager.h"

class Menu;
class MenuInventory;
class MenuPowers;
class MenuCharacter;
class MenuLog;
class MenuHUDLog;
class MenuActionBar;
class MenuBook;
class MenuStatBar;
class MenuMiniMap;
class MenuNPCActions;
class MenuNumPicker;
class MenuEnemy;
class MenuVendor;
class MenuTalker;
class MenuExit;
class MenuActiveEffects;
class MenuStash;
class MenuDevHUD;
class MenuDevConsole;
class StatBlock;
class WidgetTooltip;

const int DRAG_SRC_POWERS = 1;
const int DRAG_SRC_INVENTORY = 2;
const int DRAG_SRC_ACTIONBAR = 3;
const int DRAG_SRC_VENDOR = 4;
const int DRAG_SRC_STASH = 5;

class MenuManager {
private:

	StatBlock *stats;

	TooltipData tip_buf;
	TooltipData keyb_tip_buf_vendor;
	TooltipData keyb_tip_buf_stash;
	TooltipData keyb_tip_buf_pow;
	TooltipData keyb_tip_buf_inv;
	TooltipData keyb_tip_buf_act;

	void handleKeyboardTooltips();

	bool key_lock;

	bool mouse_dragging;
	bool keyboard_dragging;
	bool sticky_dragging;
	ItemStack drag_stack;
	int drag_power;
	int drag_src;
	Sprite *drag_icon;

	bool done;

	bool act_drag_hover;
	Point keydrag_pos;

	void renderIcon(int x, int y);
	void setDragIcon(int icon_id);
	void setDragIconItem(ItemStack stack);

	void handleKeyboardNavigation();
	void dragAndDropWithKeyboard();
	void splitStack(ItemStack stack);

public:
	MenuManager(StatBlock *stats);
	MenuManager(const MenuManager &copy); // not implemented
	~MenuManager();
	void alignAll();
	void logic();
	void render();
	void closeAll();
	void closeLeft();
	void closeRight();
	void resetDrag();

	std::vector<Menu*> menus;
	MenuInventory *inv;
	MenuPowers *pow;
	MenuCharacter *chr;
	MenuLog *questlog;
	MenuHUDLog *hudlog;
	MenuActionBar *act;
	MenuBook *book;
	MenuStatBar *hp;
	MenuStatBar *mp;
	MenuStatBar *xp;
	WidgetTooltip *tip;
	MenuMiniMap *mini;
	MenuNPCActions *npc;
	MenuNumPicker *num_picker;
	MenuEnemy *enemy;
	MenuVendor *vendor;
	MenuTalker *talker;
	MenuExit *exit;
	MenuActiveEffects *effects;
	MenuStash *stash;

	MenuDevHUD *devhud;
	MenuDevConsole *devconsole;

	bool pause;
	bool menus_open;
	std::queue<ItemStack> drop_stack;

	bool isDragging();
	bool requestingExit() {
		return done;
	}
};

#endif
