/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012-2014 Henrik Andersson
Copyright © 2012 Stefan Beller
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
 * class GameStatePlay
 *
 * Handles logic and rendering of the main action game play
 * Also handles message passing between child objects, often to avoid circular dependencies.
 */

#include "Avatar.h"
#include "CampaignManager.h"
#include "CombatText.h"
#include "CursorManager.h"
#include "Enemy.h"
#include "EnemyGroupManager.h"
#include "EnemyManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "GameState.h"
#include "GameStateCutscene.h"
#include "GameStatePlay.h"
#include "GameStateTitle.h"
#include "Hazard.h"
#include "HazardManager.h"
#include "InputState.h"
#include "LootManager.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuActionBar.h"
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
#include "MenuPowers.h"
#include "MenuStash.h"
#include "MenuTalker.h"
#include "MenuVendor.h"
#include "ModManager.h"
#include "NPC.h"
#include "NPCManager.h"
#include "PowerManager.h"
#include "QuestLog.h"
#include "RenderDevice.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

#include <cassert>

GameStatePlay::GameStatePlay()
	: GameState()
	, enemy(NULL)
	, npc_id(-1)
	, is_first_map_load(true)
{
	second_timer.setDuration(settings->max_frames_per_sec);

	hasMusic = true;
	has_background = false;
	// GameEngine scope variables

	if (items == NULL)
		items = new ItemManager();

	camp = new CampaignManager();

	loot = new LootManager();
	powers = new PowerManager();
	mapr = new MapRenderer();
	pc = new Avatar();
	enemym = new EnemyManager();
	enemyg = new EnemyGroupManager();
	hazards = new HazardManager();
	menu = new MenuManager();
	npcs = new NPCManager();
	quests = new QuestLog(menu->questlog);

	// load the config file for character titles
	loadTitles();

	refreshWidgets();
}

void GameStatePlay::refreshWidgets() {
	menu->alignAll();
}

/**
 * Reset all game states to a new game.
 */
void GameStatePlay::resetGame() {
	mapr->load("maps/spawn.txt");
	setLoadingFrame();
	camp->resetAllStatuses();
	pc->init();
	pc->stats.currency = 0;
	menu->act->clear();
	menu->inv->inventory[0].clear();
	menu->inv->inventory[1].clear();
	menu->inv->changed_equipment = true;
	menu->inv->currency = 0;
	menu->questlog->clearAll();
	quests->createQuestList();
	menu->hudlog->clear();

	// Finalize new character settings
	menu->talker->setHero(pc->stats);
	pc->loadSounds();

	mapr->executeOnLoadEvents();
}

/**
 * Check mouseover for enemies.
 * class variable "enemy" contains a live enemy on mouseover.
 * This function also sets enemy mouseover for Menu Enemy.
 */
void GameStatePlay::checkEnemyFocus() {
	pc->stats.target_corpse = NULL;
	pc->stats.target_nearest = NULL;
	pc->stats.target_nearest_corpse = NULL;
	pc->stats.target_nearest_dist = 0;
	pc->stats.target_nearest_corpse_dist = 0;

	FPoint src_pos = pc->stats.pos;

	// check the last hit enemy first
	// if there's none, then either get the nearest enemy or one under the mouse (depending on mouse mode)
	if (!inpt->usingMouse()) {
		if (hazards->last_enemy) {
			if (enemy == hazards->last_enemy) {
				if (!menu->enemy->timeout.isEnd() && hazards->last_enemy->stats.hp > 0)
					return;
				else
					hazards->last_enemy = NULL;
			}
			enemy = hazards->last_enemy;
		}
		else {
			enemy = enemym->getNearestEnemy(pc->stats.pos, !EnemyManager::GET_CORPSE, NULL, eset->misc.interact_range);
		}
	}
	else {
		if (hazards->last_enemy) {
			enemy = hazards->last_enemy;
			hazards->last_enemy = NULL;
		}
		else {
			enemy = enemym->enemyFocus(inpt->mouse, mapr->cam, EnemyManager::IS_ALIVE);
			if (enemy) {
				curs->setCursor(CursorManager::CURSOR_ATTACK);
			}
			src_pos = Utils::screenToMap(inpt->mouse.x, inpt->mouse.y, mapr->cam.x, mapr->cam.y);

		}
	}

	if (enemy) {
		// set the actual menu with the enemy selected above
		if (!enemy->stats.suppress_hp) {
			menu->enemy->enemy = enemy;
			menu->enemy->timeout.reset(Timer::BEGIN);
		}
	}
	else if (inpt->usingMouse()) {
		// if we're using a mouse and we didn't select an enemy, try selecting a dead one instead
		Enemy *temp_enemy = enemym->enemyFocus(inpt->mouse, mapr->cam, !EnemyManager::IS_ALIVE);
		if (temp_enemy) {
			pc->stats.target_corpse = &(temp_enemy->stats);
			menu->enemy->enemy = temp_enemy;
			menu->enemy->timeout.reset(Timer::BEGIN);
		}
	}

	// save the highlighted enemy position for auto-targeting purposes
	if (enemy) {
		pc->cursor_enemy = enemy;
	}
	else {
		pc->cursor_enemy = NULL;
	}

	// save the positions of the nearest enemies for powers that use "target_nearest"
	Enemy *nearest = enemym->getNearestEnemy(src_pos, !EnemyManager::GET_CORPSE, &(pc->stats.target_nearest_dist), eset->misc.interact_range);
	if (nearest)
		pc->stats.target_nearest = &(nearest->stats);
	Enemy *nearest_corpse = enemym->getNearestEnemy(src_pos, EnemyManager::GET_CORPSE, &(pc->stats.target_nearest_corpse_dist), eset->misc.interact_range);
	if (nearest_corpse)
		pc->stats.target_nearest_corpse = &(nearest_corpse->stats);
}

/**
 * If mouse_move is enabled, and the mouse is over a live enemy,
 * Do not allow power use with button MAIN1
 */
bool GameStatePlay::restrictPowerUse() {
	if (settings->mouse_move) {
		int mm_key = (settings->mouse_move_swap ? Input::MAIN2 : Input::MAIN1);
		if(inpt->pressing[mm_key] && !inpt->pressing[Input::SHIFT] && !menu->act->isWithinSlots(inpt->mouse) && !menu->act->isWithinMenus(inpt->mouse)) {
			return true;
		}
	}

	return false;
}

/**
 * Check to see if the player is picking up loot on the ground
 */
void GameStatePlay::checkLoot() {

	if (!pc->stats.alive)
		return;

	if (menu->isDragging())
		return;

	ItemStack pickup;

	// Autopickup
	if (eset->loot.autopickup_currency) {
		pickup = loot->checkAutoPickup(pc->stats.pos);
		if (!pickup.empty()) {
			menu->inv->add(pickup, MenuInventory::CARRIED, ItemStorage::NO_SLOT, MenuInventory::ADD_PLAY_SOUND, MenuInventory::ADD_AUTO_EQUIP);
			pickup.clear();
		}
	}

	// Normal pickups
	if (!pc->using_main1) {
		pickup = loot->checkPickup(inpt->mouse, mapr->cam, pc->stats.pos);
	}

	if (!pickup.empty()) {
		menu->inv->add(pickup, MenuInventory::CARRIED, ItemStorage::NO_SLOT, MenuInventory::ADD_PLAY_SOUND, MenuInventory::ADD_AUTO_EQUIP);
		StatusID pickup_status = camp->registerStatus(items->items[pickup.item].pickup_status);
		camp->setStatus(pickup_status);
		pickup.clear();
	}

}

void GameStatePlay::checkTeleport() {
	bool on_load_teleport = false;

	// both map events and player powers can cause teleportation
	if (mapr->teleportation || pc->stats.teleportation) {

		mapr->collider.unblock(pc->stats.pos.x, pc->stats.pos.y);

		if (mapr->teleportation) {
			// camera gets interpolated movement during intramap teleport
			// during intermap teleport, we set the camera to the player position
			pc->stats.pos.x = mapr->teleport_destination.x;
			pc->stats.pos.y = mapr->teleport_destination.y;
			pc->teleport_camera_lock = true;
		}
		else {
			pc->stats.pos.x = pc->stats.teleport_destination.x;
			pc->stats.pos.y = pc->stats.teleport_destination.y;
		}

		// if we're not changing map, move allies to a the player's new position
		// when changing maps, enemym->handleNewMap() does something similar to this
		if (mapr->teleport_mapname.empty()) {
			FPoint spawn_pos = mapr->collider.getRandomNeighbor(Point(pc->stats.pos), 1, !MapCollision::IGNORE_BLOCKED);
			for (unsigned int i=0; i < enemym->enemies.size(); i++) {
				if(enemym->enemies[i]->stats.hero_ally && enemym->enemies[i]->stats.alive) {
					mapr->collider.unblock(enemym->enemies[i]->stats.pos.x, enemym->enemies[i]->stats.pos.y);
					enemym->enemies[i]->stats.pos = spawn_pos;
					mapr->collider.block(enemym->enemies[i]->stats.pos.x, enemym->enemies[i]->stats.pos.y, MapCollision::IS_ALLY);
				}
			}
		}

		// process intermap teleport
		if (mapr->teleportation && !mapr->teleport_mapname.empty()) {
			mapr->cam.x = pc->stats.pos.x;
			mapr->cam.y = pc->stats.pos.y;
			std::string teleport_mapname = mapr->teleport_mapname;
			mapr->teleport_mapname = "";
			inpt->lock_all = (teleport_mapname == "maps/spawn.txt");
			mapr->executeOnMapExitEvents();
			showLoading();
			mapr->load(teleport_mapname);
			setLoadingFrame();

			// use the default hero spawn position for this map
			if (mapr->teleport_destination.x == -1 && mapr->teleport_destination.y == -1) {
				mapr->cam.x = pc->stats.pos.x = mapr->hero_pos.x;
				mapr->cam.y = pc->stats.pos.y = mapr->hero_pos.y;
			}

			// store this as the new respawn point (provided the tile is open)
			if (mapr->collider.isValidPosition(pc->stats.pos.x, pc->stats.pos.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_HERO)) {
				mapr->respawn_map = teleport_mapname;
				mapr->respawn_point = pc->stats.pos;
			}
			else {
				Utils::logError("GameStatePlay: Spawn position (%d, %d) is blocked.", static_cast<int>(pc->stats.pos.x), static_cast<int>(pc->stats.pos.y));
			}

			pc->handleNewMap();
			hazards->handleNewMap();
			loot->handleNewMap();
			powers->handleNewMap(&mapr->collider);
			menu->enemy->handleNewMap();
			menu->stash->visible = false;

			// switch off teleport flag so we can check if an on_load event has teleportation
			mapr->teleportation = false;

			mapr->executeOnLoadEvents();
			if (mapr->teleportation)
				on_load_teleport = true;

			// enemies and npcs should be initialized AFTER on_load events execute
			enemym->handleNewMap();
			npcs->handleNewMap();
			resetNPC();

			menu->mini->prerender(&mapr->collider, mapr->w, mapr->h);

			// return to title (permadeath) OR auto-save
			if (pc->stats.permadeath && pc->stats.cur_state == StatBlock::AVATAR_DEAD) {
				snd->stopMusic();
				showLoading();
				setRequestedGameState(new GameStateTitle());
			}
			else if (eset->misc.save_onload) {
				if (!is_first_map_load)
					save_load->saveGame();
				else
					is_first_map_load = false;
			}
		}

		if (mapr->collider.isOutsideMap(pc->stats.pos.x, pc->stats.pos.y)) {
			Utils::logError("GameStatePlay: Teleport position is outside of map bounds.");
			pc->stats.pos.x = 0.5f;
			pc->stats.pos.y = 0.5f;
		}

		mapr->collider.block(pc->stats.pos.x, pc->stats.pos.y, !MapCollision::IS_ALLY);

		pc->stats.teleportation = false;
	}

	if (!on_load_teleport && mapr->teleport_mapname.empty())
		mapr->teleportation = false;
}

/**
 * Check for cancel key to exit menus or exit the game.
 * Also check closing the game window entirely.
 */
void GameStatePlay::checkCancel() {
	bool save_on_exit = eset->misc.save_onexit && !(pc->stats.permadeath && pc->stats.cur_state == StatBlock::AVATAR_DEAD);

	if (save_on_exit && eset->misc.save_pos_onexit) {
		mapr->respawn_point = pc->stats.pos;
	}

	// if user has clicked exit game from exit menu
	if (menu->requestingExit()) {
		menu->closeAll();

		if (save_on_exit)
			save_load->saveGame();

		// audio levels can be changed in the pause menu, so update our settings file
		settings->saveSettings();
		inpt->saveKeyBindings();

		snd->stopMusic();
		showLoading();
		setRequestedGameState(new GameStateTitle());

		save_load->setGameSlot(0);
	}

	// if user closes the window
	if (inpt->done) {
		menu->closeAll();

		if (save_on_exit)
			save_load->saveGame();

		settings->saveSettings();
		inpt->saveKeyBindings();

		snd->stopMusic();
		exitRequested = true;
	}
}

/**
 * Check for log messages from various child objects
 */
void GameStatePlay::checkLog() {

	// If the player has just respawned, we want to clear the HUD log
	if (pc->respawn) {
		menu->hudlog->clear();
	}

	while (!pc->log_msg.empty()) {
		const std::string& str = pc->log_msg.front().first;
		const int msg_type = pc->log_msg.front().second;

		menu->questlog->add(str, MenuLog::TYPE_MESSAGES, msg_type);
		menu->hudlog->add(str, msg_type);

		pc->log_msg.pop();
	}
}

/**
 * Check if we need to open book
 */
void GameStatePlay::checkBook() {
	// Map events can open books
	if (mapr->show_book != "") {
		menu->book->book_name = mapr->show_book;
		mapr->show_book = "";
	}

	// items can be readable books
	if (menu->inv->show_book != "") {
		menu->book->book_name = menu->inv->show_book;
		menu->inv->show_book = "";
	}
}

void GameStatePlay::loadTitles() {
	FileParser infile;
	// @CLASS GameStatePlay: Titles|Description of engine/titles.txt
	if (infile.open("engine/titles.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section && infile.section == "title") {
				Title t;
				titles.push_back(t);
			}

			if (titles.empty()) continue;

			if (infile.key == "title") {
				// @ATTR title.title|string|The displayed title.
				titles.back().title = infile.val;
			}
			else if (infile.key == "level") {
				// @ATTR title.level|int|Requires level.
				titles.back().level = Parse::toInt(infile.val);
			}
			else if (infile.key == "power") {
				// @ATTR title.power|power_id|Requires power.
				titles.back().power = Parse::toInt(infile.val);
			}
			else if (infile.key == "requires_status") {
				// @ATTR title.requires_status|list(string)|Requires status.
				std::string repeat_val = Parse::popFirstString(infile.val);
				while (repeat_val != "") {
					titles.back().requires_status.push_back(camp->registerStatus(repeat_val));
					repeat_val = Parse::popFirstString(infile.val);
				}
			}
			else if (infile.key == "requires_not_status") {
				// @ATTR title.requires_not_status|list(string)|Requires not status.
				std::string repeat_val = Parse::popFirstString(infile.val);
				while (repeat_val != "") {
					titles.back().requires_not_status.push_back(camp->registerStatus(repeat_val));
					repeat_val = Parse::popFirstString(infile.val);
				}
			}
			else if (infile.key == "primary_stat") {
				// @ATTR title.primary_stat|predefined_string, predefined_string : Primary stat, Lesser primary stat|Required primary stat(s). The lesser stat is optional.
				titles.back().primary_stat_1 = Parse::popFirstString(infile.val);
				titles.back().primary_stat_2 = Parse::popFirstString(infile.val);
			}
			else infile.error("GameStatePlay: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
}

void GameStatePlay::checkTitle() {
	if (!pc->stats.check_title || titles.empty())
		return;

	int title_id = -1;

	for (unsigned i=0; i<titles.size(); i++) {
		if (titles[i].title.empty())
			continue;

		if (titles[i].level > 0 && pc->stats.level < titles[i].level)
			continue;
		if (titles[i].power > 0 && std::find(pc->stats.powers_list.begin(), pc->stats.powers_list.end(), titles[i].power) == pc->stats.powers_list.end())
			continue;
		if (!titles[i].primary_stat_1.empty() && !checkPrimaryStat(titles[i].primary_stat_1, titles[i].primary_stat_2))
			continue;

		bool status_failed = false;
		for (size_t j = 0; j < titles[i].requires_status.size(); ++j) {
			if (!camp->checkStatus(titles[i].requires_status[j])) {
				status_failed = true;
				break;
			}
		}
		for (size_t j = 0; j < titles[i].requires_not_status.size(); ++j) {
			if (camp->checkStatus(titles[i].requires_not_status[j])) {
				status_failed = true;
				break;
			}
		}

		if (status_failed)
			continue;

		// Title meets the requirements
		title_id = i;
		break;
	}

	if (title_id != -1) pc->stats.character_subclass = titles[title_id].title;
	pc->stats.check_title = false;
	pc->stats.refresh_stats = true;
}

void GameStatePlay::checkEquipmentChange() {
	// force the actionbar to update when we change gear
	if (menu->inv->changed_equipment) {
		menu->act->updated = true;
	}

	if (menu->inv->changed_equipment) {

		int feet_index = -1;
		std::vector<Avatar::Layer_gfx> img_gfx;
		// load only displayable layers
		for (unsigned int j=0; j<pc->layer_reference_order.size(); j++) {
			Avatar::Layer_gfx gfx;
			gfx.gfx = "";
			gfx.type = pc->layer_reference_order[j];
			for (int i=0; i<menu->inv->inventory[MenuInventory::EQUIPMENT].getSlotNumber(); i++) {
				if (pc->layer_reference_order[j] == menu->inv->inventory[MenuInventory::EQUIPMENT].slot_type[i]) {
					gfx.gfx = items->items[menu->inv->inventory[MenuInventory::EQUIPMENT][i].item].gfx;
					gfx.type = menu->inv->inventory[MenuInventory::EQUIPMENT].slot_type[i];
				}
				if (menu->inv->inventory[MenuInventory::EQUIPMENT].slot_type[i] == "feet") {
					feet_index = i;
				}
			}
			// special case: if we don't have a head, use the portrait's head
			if (gfx.gfx == "" && pc->layer_reference_order[j] == "head") {
				gfx.gfx = pc->stats.gfx_head;
				gfx.type = "head";
			}
			// fall back to default if it exists
			if (gfx.gfx == "") {
				bool exists = Filesystem::fileExists(mods->locate("animations/avatar/" + pc->stats.gfx_base + "/default_" + gfx.type + ".txt"));
				if (exists) gfx.gfx = "default_" + gfx.type;
			}
			img_gfx.push_back(gfx);
		}
		assert(pc->layer_reference_order.size()==img_gfx.size());
		pc->loadGraphics(img_gfx);

		if (feet_index != -1)
			pc->loadStepFX(items->items[menu->inv->inventory[MenuInventory::EQUIPMENT][feet_index].item].stepfx);
	}

	menu->inv->changed_equipment = false;
}

void GameStatePlay::checkLootDrop() {

	// if the player has dropped an item from the inventory
	while (!menu->drop_stack.empty()) {
		if (!menu->drop_stack.front().empty()) {
			loot->addLoot(menu->drop_stack.front(), pc->stats.pos, LootManager::DROPPED_BY_HERO);
		}
		menu->drop_stack.pop();
	}

	// if the player has dropped a quest reward because inventory full
	while (!camp->drop_stack.empty()) {
		if (!camp->drop_stack.front().empty()) {
			loot->addLoot(camp->drop_stack.front(), pc->stats.pos, LootManager::DROPPED_BY_HERO);
		}
		camp->drop_stack.pop();
	}

	// if the player been directly given items, but their inventory is full
	// this happens when adding currency from older save files
	while (!menu->inv->drop_stack.empty()) {
		if (!menu->inv->drop_stack.front().empty()) {
			loot->addLoot(menu->inv->drop_stack.front(), pc->stats.pos, LootManager::DROPPED_BY_HERO);
		}
		menu->inv->drop_stack.pop();
	}
}

/**
 * Removes items as required by certain powers
 */
void GameStatePlay::checkUsedItems() {
	for (unsigned i=0; i<powers->used_items.size(); i++) {
		menu->inv->remove(powers->used_items[i]);
	}
	for (unsigned i=0; i<powers->used_equipped_items.size(); i++) {
		menu->inv->inventory[MenuInventory::EQUIPMENT].remove(powers->used_equipped_items[i], 1);
		menu->inv->applyEquipment();
	}
	powers->used_items.clear();
	powers->used_equipped_items.clear();
}

/**
 * Marks the menu if it needs attention.
 */
void GameStatePlay::checkNotifications() {
	if (pc->newLevelNotification || menu->chr->getUnspent() > 0) {
		pc->newLevelNotification = false;
		menu->act->requires_attention[MenuActionBar::MENU_CHARACTER] = !menu->chr->visible;
	}
	if (menu->pow->newPowerNotification) {
		menu->pow->newPowerNotification = false;
		menu->act->requires_attention[MenuActionBar::MENU_POWERS] = !menu->pow->visible;
	}
	if (quests->newQuestNotification) {
		quests->newQuestNotification = false;
		menu->act->requires_attention[MenuActionBar::MENU_LOG] = !menu->questlog->visible && !pc->questlog_dismissed;
		pc->questlog_dismissed = false;
	}

	// if the player is transformed into a creature, don't notifications for the powers menu
	if (pc->stats.transformed) {
		menu->act->requires_attention[MenuActionBar::MENU_POWERS] = false;
	}
}

/**
 * If the player has clicked on an NPC, the game mode might be changed.
 * If a player walks away from an NPC, end the interaction with that NPC
 * If an NPC is giving a reward, process it
 */
void GameStatePlay::checkNPCInteraction() {
	if (pc->using_main1 || !pc->stats.humanoid)
		return;

	// reset movement restrictions when we're not in dialog
	if (!menu->talker->visible) {
		pc->allow_movement = true;
	}

	if (npc_id != -1 && !menu->isNPCMenuVisible()) {
		// if we have an NPC, but no NPC windows are open, clear the NPC
		resetNPC();
	}

	// get NPC by ID
	// event NPCs take precedence over map NPCs
	if (mapr->event_npc != "") {
		// if the player is already interacting with an NPC when triggering an event NPC, clear the current NPC
		if (npc_id != -1) {
			resetNPC();
		}
		npc_id = mapr->npc_id = npcs->getID(mapr->event_npc);
		menu->talker->npc_from_map = false;
	}
	else if (mapr->npc_id != -1) {
		npc_id = mapr->npc_id;
		menu->talker->npc_from_map = true;
	}
	mapr->event_npc = "";
	mapr->npc_id = -1;

	if (npc_id == -1)
		return;

	if (npc_id != -1) {
		bool interact_with_npc = false;
		if (menu->talker->npc_from_map) {
			float interact_distance = Utils::calcDist(pc->stats.pos, npcs->npcs[npc_id]->pos);

			if (interact_distance < eset->misc.interact_range) {
				interact_with_npc = true;
			}
			else {
				resetNPC();
			}
		}
		else {
			// npc is from event
			interact_with_npc = true;

			// since its impossible for the player to walk away from event NPCs, we disable their movement here
			pc->allow_movement = false;
		}

		if (interact_with_npc) {
			if (!menu->isNPCMenuVisible()) {
				if (inpt->pressing[Input::MAIN1] && inpt->usingMouse()) inpt->lock[Input::MAIN1] = true;
				if (inpt->pressing[Input::ACCEPT]) inpt->lock[Input::ACCEPT] = true;

				menu->closeAll();
				menu->talker->setNPC(npcs->npcs[npc_id]);
				menu->talker->chooseDialogNode(-1);
			}
		}
	}
}

void GameStatePlay::checkStash() {
	if (mapr->stash) {
		// If triggered, open the stash and inventory menus
		menu->closeAll();
		menu->inv->visible = true;
		menu->stash->visible = true;
		menu->stash->setTab(MenuStash::STASH_PRIVATE);
		mapr->stash = false;
		menu->stash->validate(menu->drop_stack);
	}
	else if (menu->stash->visible) {
		// Close stash if inventory is closed
		if (!menu->inv->visible) {
			menu->resetDrag();
			menu->stash->visible = false;
		}

		// If the player walks away from the stash, close its menu
		float interact_distance = Utils::calcDist(pc->stats.pos, mapr->stash_pos);
		if (interact_distance > eset->misc.interact_range || !pc->stats.alive) {
			menu->resetDrag();
			menu->stash->visible = false;
		}

	}

	// If the stash has been updated, save the game
	if (menu->stash->updated) {
		menu->stash->updated = false;
		save_load->saveGame();
	}
}

void GameStatePlay::checkCutscene() {
	if (!mapr->cutscene)
		return;

	showLoading();
	GameStateCutscene *cutscene = new GameStateCutscene(NULL);

	if (!cutscene->load(mapr->cutscene_file)) {
		delete cutscene;
		mapr->cutscene = false;
		return;
	}

	// handle respawn point and set game play game_slot
	cutscene->game_slot = save_load->getGameSlot();

	if (mapr->teleportation) {

		if (mapr->teleport_mapname != "")
			mapr->respawn_map = mapr->teleport_mapname;

		mapr->respawn_point = mapr->teleport_destination;

	}
	else {
		mapr->respawn_point = pc->stats.pos;
	}

	if (eset->misc.save_onload)
		save_load->saveGame();

	if (menu->devconsole) {
		menu->devconsole->closeWindow();
	}

	setRequestedGameState(cutscene);
}

void GameStatePlay::checkSaveEvent() {
	if (mapr->save_game) {
		mapr->respawn_point = pc->stats.pos;
		save_load->saveGame();
		mapr->save_game = false;
	}
}

/**
 * Recursively update the action bar powers based on equipment
 */
void GameStatePlay::updateActionBar(unsigned index) {
	if (menu->act->slots_count == 0 || index > menu->act->slots_count - 1) return;

	if (items->items.empty()) return;

	for (unsigned i = index; i < menu->act->slots_count; i++) {
		if (menu->act->hotkeys[i] == 0) continue;

		int id = menu->inv->getPowerMod(menu->act->hotkeys_mod[i]);
		if (id > 0) {
			menu->act->hotkeys_mod[i] = id;
			return updateActionBar(i);
		}
	}
}

/**
 * Process all actions for a single frame
 * This includes some message passing between child object
 */
void GameStatePlay::logic() {
	if (inpt->window_resized)
		refreshWidgets();

	curs->setLowHP(pc->isLowHpCursorEnabled() && pc->isLowHp());

	checkCutscene();

	// check menus first (top layer gets mouse click priority)
	menu->logic();

	if (!isPaused()) {
		if (!second_timer.isEnd())
			second_timer.tick();
		else {
			pc->time_played++;
			second_timer.reset(Timer::BEGIN);
		}

		// these actions only occur when the game isn't paused
		if (pc->stats.alive) checkLoot();
		checkEnemyFocus();
		if (pc->stats.alive) {
			mapr->checkHotspots();
			mapr->checkNearestEvent();
			checkNPCInteraction();
		}
		checkTitle();

		menu->act->checkAction(action_queue);
		pc->logic(action_queue, restrictPowerUse());

		// Transform powers change the actionbar layout,
		// so we need to prevent accidental clicks if a new power is placed under the slot we clicked on.
		// It's a bit hacky, but it works
		if (pc->isTransforming()) {
			menu->act->resetSlots();
		}

		// transfer hero data to enemies, for AI use
		if (pc->stats.get(Stats::STEALTH) > 100) enemym->hero_stealth = 100;
		else enemym->hero_stealth = pc->stats.get(Stats::STEALTH);

		enemym->logic();
		hazards->logic();
		loot->logic();
		enemym->checkEnemiesforXP();
		npcs->logic();

		snd->logic(pc->stats.pos);

		comb->logic(mapr->cam);
	}

	// close menus when the player dies, but still allow them to be reopened
	if (pc->close_menus) {
		pc->close_menus = false;
		menu->closeAll();
	}

	// these actions occur whether the game is paused or not.
	// TODO Why? Some of these probably don't need to be executed when paused
	checkTeleport();
	checkLootDrop();
	checkLog();
	checkBook();
	checkEquipmentChange();
	checkUsedItems();
	checkStash();
	checkSaveEvent();
	checkNotifications();
	checkCancel();

	mapr->logic(isPaused());
	mapr->enemies_cleared = enemym->isCleared();
	quests->logic();

	pc->checkTransform();

	// change hero powers on transformation
	if (pc->setPowers) {
		pc->setPowers = false;
		if (!pc->stats.humanoid && menu->pow->visible) menu->closeRight();
		// save ActionBar state and lock slots from removing/replacing power
		for (int i = 0; i < MenuActionBar::SLOT_MAX ; i++) {
			menu->act->hotkeys_temp[i] = menu->act->hotkeys[i];
			menu->act->hotkeys[i] = 0;
		}
		int count = MenuActionBar::SLOT_MAIN1;
		// put creature powers on action bar
		for (size_t i=0; i<pc->charmed_stats->powers_ai.size(); i++) {
			if (pc->charmed_stats->powers_ai[i].id != 0 && powers->powers[pc->charmed_stats->powers_ai[i].id].beacon != true) {
				menu->act->hotkeys[count] = pc->charmed_stats->powers_ai[i].id;
				menu->act->locked[count] = true;
				count++;
				if (count == MenuActionBar::SLOT_MAX)
					count = 0;
				else if (count == MenuActionBar::SLOT_MAIN1)
					// we've filled the actionbar, stop adding powers to it
					break;
			}
		}
		if (pc->stats.manual_untransform && pc->untransform_power > 0) {
			menu->act->hotkeys[count] = pc->untransform_power;
			menu->act->locked[count] = true;
		}
		else if (pc->stats.manual_untransform && pc->untransform_power == 0)
			Utils::logError("GameStatePlay: Untransform power not found, you can't untransform manually");

		menu->act->updated = true;

		// reapply equipment if the transformation allows it
		if (pc->stats.transform_with_equipment)
			menu->inv->applyEquipment();
	}
	// revert hero powers
	if (pc->revertPowers) {
		pc->revertPowers = false;

		// restore ActionBar state
		for (int i = 0; i < MenuActionBar::SLOT_MAX; i++) {
			menu->act->hotkeys[i] = menu->act->hotkeys_temp[i];
			menu->act->locked[i] = false;
		}

		menu->act->updated = true;

		// also reapply equipment here, to account items that give bonuses to base stats
		menu->inv->applyEquipment();
	}

	// when the hero (re)spawns, reapply equipment & passive effects
	if (pc->respawn) {
		pc->stats.alive = true;
		pc->stats.corpse = false;
		pc->stats.cur_state = StatBlock::AVATAR_STANCE;
		menu->inv->applyEquipment();
		menu->inv->changed_equipment = true;
		checkEquipmentChange();
		pc->stats.hp = pc->stats.get(Stats::HP_MAX);
		pc->stats.logic();
		pc->stats.recalc();
		powers->activatePassives(&pc->stats);
		pc->respawn = false;
	}

	// use a normal mouse cursor is menus are open
	if (menu->menus_open) {
		curs->setCursor(CursorManager::CURSOR_NORMAL);
	}

	// update the action bar as it may have been changed by items
	if (menu->act->updated) {
		menu->act->updated = false;

		// set all hotkeys to their base powers
		for (unsigned i = 0; i < menu->act->slots_count; i++) {
			menu->act->hotkeys_mod[i] = menu->act->hotkeys[i];
		}

		updateActionBar(UPDATE_ACTIONBAR_ALL);
	}

	// reload music if changed in the pause menu
	if (menu->exit->reload_music) {
		mapr->loadMusic();
		menu->exit->reload_music = false;
	}
}


/**
 * Render all graphics for a single frame
 */
void GameStatePlay::render() {

	// Create a list of Renderables from all objects not already on the map.
	// split the list into the beings alive (may move) and dead beings (must not move)
	std::vector<Renderable> rens;
	std::vector<Renderable> rens_dead;

	pc->addRenders(rens);

	enemym->addRenders(rens, rens_dead);

	npcs->addRenders(rens); // npcs cannot be dead

	loot->addRenders(rens, rens_dead);

	hazards->addRenders(rens, rens_dead);


	// render the static map layers plus the renderables
	mapr->render(rens, rens_dead);

	// mouseover tooltips
	loot->renderTooltips(mapr->cam);

	if (mapr->map_change) {
		menu->mini->prerender(&mapr->collider, mapr->w, mapr->h);
		mapr->map_change = false;
	}
	menu->mini->setMapTitle(mapr->title);
	menu->mini->render(pc->stats.pos);
	menu->render();

	// render combat text last - this should make it obvious you're being
	// attacked, even if you have menus open
	if (!isPaused())
		comb->render();
}

bool GameStatePlay::isPaused() {
	return menu->pause;
}

void GameStatePlay::resetNPC() {
	npc_id = -1;
	menu->talker->npc_from_map = true;
	menu->vendor->setNPC(NULL);
	menu->talker->setNPC(NULL);
}

bool GameStatePlay::checkPrimaryStat(const std::string& first, const std::string& second) {
	int high = 0;
	size_t high_index = eset->primary_stats.list.size();
	size_t low_index = eset->primary_stats.list.size();

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		int stat = pc->stats.get_primary(i);
		if (stat > high) {
			if (high_index != eset->primary_stats.list.size()) {
				low_index = high_index;
			}
			high = stat;
			high_index = i;
		}
		else if (stat == high && low_index == eset->primary_stats.list.size()) {
			low_index = i;
		}
		else if (low_index == eset->primary_stats.list.size() || (low_index < eset->primary_stats.list.size() && stat > pc->stats.get_primary(low_index))) {
			low_index = i;
		}
	}

	// if the first primary stat doesn't match, we don't care about the second one
	if (high_index != eset->primary_stats.list.size() && first != eset->primary_stats.list[high_index].id)
		return false;

	if (!second.empty()) {
		if (low_index != eset->primary_stats.list.size() && second != eset->primary_stats.list[low_index].id)
			return false;
	}
	else if (second.empty() && pc->stats.get_primary(high_index) == pc->stats.get_primary(low_index)) {
		// titles that require a single stat are ignored if two stats are equal
		return false;
	}

	return true;
}

GameStatePlay::~GameStatePlay() {
	curs->setLowHP(false);

	delete quests;
	delete npcs;
	delete hazards;
	delete enemym;
	delete pc;
	delete mapr;
	delete menu;
	delete loot;
	delete camp;
	delete items;
	delete powers;

	delete enemyg;

	// NULL-ify shared game resources
	pc = NULL;
	menu = NULL;
	camp = NULL;
	enemyg = NULL;
	enemym = NULL;
	items = NULL;
	loot = NULL;
	mapr = NULL;
	menu_act = NULL;
	menu_powers = NULL;
	powers = NULL;
}

