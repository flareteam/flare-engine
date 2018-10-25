/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
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
class MenuNumPicker;
class MenuEnemy;
class MenuVendor;
class MenuTalker;
class MenuExit;
class MenuActiveEffects;
class MenuStash;
class MenuDevConsole;
class MenuTouchControls;
class StatBlock;
class Subtitles;
class WidgetSlot;

class MenuManager {
private:
	enum {
		DRAG_SRC_NONE = 0,
		DRAG_SRC_POWERS = 1,
		DRAG_SRC_INVENTORY = 2,
		DRAG_SRC_ACTIONBAR = 3,
		DRAG_SRC_VENDOR = 4,
		DRAG_SRC_STASH = 5
	};

	void handleKeyboardTooltips();

	bool key_lock;

	bool mouse_dragging;
	bool keyboard_dragging;
	bool sticky_dragging;
	ItemStack drag_stack;
	int drag_power;
	int drag_src;
	WidgetSlot *drag_icon;

	bool done;

	bool act_drag_hover;
	Point keydrag_pos;

	void renderIcon(int x, int y);
	void setDragIcon(int icon_id, int overlay_id);
	void setDragIconItem(ItemStack stack);

	void handleKeyboardNavigation();
	void dragAndDropWithKeyboard();

public:
	explicit MenuManager();
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
	MenuMiniMap *mini;
	MenuNumPicker *num_picker;
	MenuEnemy *enemy;
	MenuVendor *vendor;
	MenuTalker *talker;
	MenuExit *exit;
	MenuActiveEffects *effects;
	MenuStash *stash;

	MenuDevConsole *devconsole;
	MenuTouchControls *touch_controls;

	Subtitles *subtitles;

	bool pause;
	bool menus_open;
	std::queue<ItemStack> drop_stack;

	bool isDragging();
	bool requestingExit() {
		return done;
	}
	bool isNPCMenuVisible();
	void showExitMenu();
};

#endif
