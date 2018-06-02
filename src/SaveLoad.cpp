/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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
 * class SaveLoad
 *
 * Save and Load functions for the GameStatePlay.
 *
 * I put these in a separate cpp file just to keep GameStatePlay.cpp devoted to its core.
 *
 */

#include "Avatar.h"
#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameStatePlay.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "MenuCharacter.h"
#include "MenuHUDLog.h"
#include "MenuInventory.h"
#include "MenuLog.h"
#include "MenuManager.h"
#include "MenuPowers.h"
#include "MenuStash.h"
#include "MenuTalker.h"
#include "MenuVendor.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "NPC.h"
#include "Platform.h"
#include "PowerManager.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "Version.h"

SaveLoad::SaveLoad()
	: game_slot(0) {
}

SaveLoad::~SaveLoad() {
}

/**
 * Before exiting the game, save to file
 */
void SaveLoad::saveGame() {

	if (game_slot <= 0) return;

	// if needed, create the save file structure
	createSaveDir(game_slot);

	// remove items with zero quantity from inventory
	menu->inv->inventory[MenuInventory::EQUIPMENT].clean();
	menu->inv->inventory[MenuInventory::CARRIED].clean();

	std::ofstream outfile;

	std::stringstream ss;
	ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/avatar.txt";

	outfile.open(path(&ss).c_str(), std::ios::out);

	if (outfile.is_open()) {

		// comment
		outfile << "## flare-engine save file ##" << "\n";

		// hero name
		outfile << "name=" << pc->stats.name << "\n";

		// permadeath
		outfile << "permadeath=" << pc->stats.permadeath << "\n";

		// hero visual option
		outfile << "option=" << pc->stats.gfx_base << "," << pc->stats.gfx_head << "," << pc->stats.gfx_portrait << "\n";

		// hero class
		outfile << "class=" << pc->stats.character_class << "," << pc->stats.character_subclass << "\n";

		// current experience
		outfile << "xp=" << pc->stats.xp << "\n";

		// hp and mp
		if (SAVE_HPMP) outfile << "hpmp=" << pc->stats.hp << "," << pc->stats.mp << "\n";

		// stat spec
		outfile << "build=";
		for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
			outfile << pc->stats.primary[i];
			if (i < PRIMARY_STATS.size() - 1)
				outfile << ",";
		}
		outfile << "\n";

		// equipped gear
		outfile << "equipped_quantity=" << menu->inv->inventory[MenuInventory::EQUIPMENT].getQuantities() << "\n";
		outfile << "equipped=" << menu->inv->inventory[MenuInventory::EQUIPMENT].getItems() << "\n";

		// carried items
		outfile << "carried_quantity=" << menu->inv->inventory[MenuInventory::CARRIED].getQuantities() << "\n";
		outfile << "carried=" << menu->inv->inventory[MenuInventory::CARRIED].getItems() << "\n";

		// spawn point
		outfile << "spawn=" << mapr->respawn_map << "," << static_cast<int>(mapr->respawn_point.x) << "," << static_cast<int>(mapr->respawn_point.y) << "\n";

		// action bar
		outfile << "actionbar=";
		for (unsigned i = 0; i < static_cast<unsigned>(MenuActionBar::SLOT_MAX); i++) {
			if (i < menu->act->slots_count)
			{
				if (pc->stats.transformed) outfile << menu->act->hotkeys_temp[i];
				else outfile << menu->act->hotkeys[i];
			}
			else
			{
				outfile << 0;
			}
			if (i < MenuActionBar::SLOT_MAX - 1) outfile << ",";
		}
		outfile << "\n";

		//shapeshifter value
		if (pc->stats.transform_type == "untransform" || pc->stats.transform_duration != -1) outfile << "transformed=" << "\n";
		else outfile << "transformed=" << pc->stats.transform_type << "," << pc->stats.manual_untransform << "\n";

		// restore hero powers
		if (pc->stats.transformed && pc->hero_stats) {
			pc->stats.powers_list = pc->hero_stats->powers_list;
		}

		// enabled powers
		outfile << "powers=";
		for (unsigned int i=0; i<pc->stats.powers_list.size(); i++) {
			if (i < pc->stats.powers_list.size()-1) {
				if (pc->stats.powers_list[i] > 0)
					outfile << pc->stats.powers_list[i] << ",";
			}
			else {
				if (pc->stats.powers_list[i] > 0)
					outfile << pc->stats.powers_list[i];
			}
		}
		outfile << "\n";

		// restore transformed powers
		if (pc->stats.transformed && pc->charmed_stats) {
			pc->stats.powers_list = pc->charmed_stats->powers_list;
		}

		// campaign data
		outfile << "campaign=" << camp->getAll() << "\n";

		outfile << "time_played=" << pc->time_played << "\n";

		// save the engine version for troubleshooting purposes
		outfile << "engine_version=" << versionToString(ENGINE_VERSION) << "\n";

		// save the vendor buyback
		if (SAVE_BUYBACK) {
			std::map<std::string, ItemStorage>::iterator it;

			for (it = menu->vendor->buyback_stock.begin(); it != menu->vendor->buyback_stock.end(); ++it) {
				if (it->second.empty())
					continue;

				outfile << "buyback_item=" << it->first << ";" << it->second.getItems() << "\n";
				outfile << "buyback_quantity=" << it->first << ";" << it->second.getQuantities() << "\n";
			}
		}

		outfile << "questlog_dismissed=" << !menu->act->requires_attention[MenuActionBar::MENU_LOG];

		outfile << std::endl;

		if (outfile.bad()) logError("SaveLoad: Unable to save the game. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		PlatformFSCommit();
	}

	// Save stash
	ss.str("");
	if (pc->stats.permadeath)
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/stash_HC.txt";
	else
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/stash.txt";

	outfile.open(path(&ss).c_str(), std::ios::out);

	if (outfile.is_open()) {

		// comment
		outfile << "## flare-engine stash file ##" << "\n";

		outfile << "quantity=" << menu->stash->stock.getQuantities() << "\n";
		outfile << "item=" << menu->stash->stock.getItems() << "\n";

		outfile << std::endl;

		if (outfile.bad()) logError("SaveLoad: Unable to save stash. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		PlatformFSCommit();
	}

	PREV_SAVE_SLOT = game_slot-1;

	// display a log message saying that we saved the game
	menu->questlog->add(msg->get("Game saved."), MenuLog::TYPE_MESSAGES, MenuLog::PREVENT_SPAM, MenuLog::DEFAULT_STYLE);
	menu->hudlog->add(msg->get("Game saved."), MenuHUDLog::PREVENT_SPAM);
}

/**
 * When loading the game, load from file if possible
 */
void SaveLoad::loadGame() {
	if (game_slot <= 0) return;

	int saved_hp = 0;
	int saved_mp = 0;
	int currency = 0;
	Version save_version(VERSION_MIN);

	FileParser infile;
	std::vector<int> hotkeys(MenuActionBar::SLOT_MAX, -1);

	std::stringstream ss;
	ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/avatar.txt";

	if (infile.open(path(&ss), FileParser::FULL_PATH)) {
		while (infile.next()) {
			if (infile.key == "name") pc->stats.name = infile.val;
			else if (infile.key == "permadeath") {
				pc->stats.permadeath = toBool(infile.val);
			}
			else if (infile.key == "option") {
				pc->stats.gfx_base = popFirstString(infile.val);
				pc->stats.gfx_head = popFirstString(infile.val);
				pc->stats.gfx_portrait = popFirstString(infile.val);
			}
			else if (infile.key == "class") {
				pc->stats.character_class = popFirstString(infile.val);
				pc->stats.character_subclass = popFirstString(infile.val);
			}
			else if (infile.key == "xp") {
				pc->stats.xp = toUnsignedLong(infile.val);
			}
			else if (infile.key == "hpmp") {
				saved_hp = popFirstInt(infile.val);
				saved_mp = popFirstInt(infile.val);
			}
			else if (infile.key == "build") {
				for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
					pc->stats.primary[i] = popFirstInt(infile.val);
					if (pc->stats.primary[i] < 0 || pc->stats.primary[i] > pc->stats.max_points_per_stat) {
						logInfo("SaveLoad: Primary stat value for '%s' is out of bounds, setting to zero.", PRIMARY_STATS[i].id.c_str());
						pc->stats.primary[i] = 0;
					}
				}
			}
			else if (infile.key == "currency") {
				currency = toInt(infile.val);
			}
			else if (infile.key == "equipped") {
				menu->inv->inventory[MenuInventory::EQUIPMENT].setItems(infile.val);
			}
			else if (infile.key == "equipped_quantity") {
				menu->inv->inventory[MenuInventory::EQUIPMENT].setQuantities(infile.val);
			}
			else if (infile.key == "carried") {
				menu->inv->inventory[MenuInventory::CARRIED].setItems(infile.val);
			}
			else if (infile.key == "carried_quantity") {
				menu->inv->inventory[MenuInventory::CARRIED].setQuantities(infile.val);
			}
			else if (infile.key == "spawn") {
				mapr->teleport_mapname = popFirstString(infile.val);
				if (mapr->teleport_mapname != "" && fileExists(mods->locate(mapr->teleport_mapname))) {
					mapr->teleport_destination.x = static_cast<float>(popFirstInt(infile.val)) + 0.5f;
					mapr->teleport_destination.y = static_cast<float>(popFirstInt(infile.val)) + 0.5f;
					mapr->teleportation = true;
					// prevent spawn.txt from putting us on the starting map
					mapr->clearEvents();
				}
				else {
					logError("SaveLoad: Unable to find %s, loading maps/spawn.txt", mapr->teleport_mapname.c_str());
					mapr->teleport_mapname = "maps/spawn.txt";
					mapr->teleport_destination.x = 0.5f;
					mapr->teleport_destination.y = 0.5f;
					mapr->teleportation = true;
				}
			}
			else if (infile.key == "actionbar") {
				for (int i = 0; i < MenuActionBar::SLOT_MAX; i++) {
					hotkeys[i] = popFirstInt(infile.val);
					if (hotkeys[i] < 0) {
						logError("SaveLoad: Hotkey power on position %d has negative id, skipping", i);
						hotkeys[i] = 0;
					}
					else if (static_cast<unsigned>(hotkeys[i]) > powers->powers.size()-1) {
						logError("SaveLoad: Hotkey power id (%d) out of bounds 1-%d, skipping", hotkeys[i], static_cast<int>(powers->powers.size()));
						hotkeys[i] = 0;
					}
					else if (hotkeys[i] != 0 && static_cast<unsigned>(hotkeys[i]) < powers->powers.size() && powers->powers[hotkeys[i]].name == "") {
						logError("SaveLoad: Hotkey power with id=%d, found on position %d does not exist, skipping", hotkeys[i], i);
						hotkeys[i] = 0;
					}
				}
				menu->act->set(hotkeys);
			}
			else if (infile.key == "transformed") {
				pc->stats.transform_type = popFirstString(infile.val);
				if (pc->stats.transform_type != "") {
					pc->stats.transform_duration = -1;
					pc->stats.manual_untransform = toBool(popFirstString(infile.val));
				}
			}
			else if (infile.key == "powers") {
				std::string power;
				while ( (power = popFirstString(infile.val)) != "") {
					if (toInt(power) > 0)
						pc->stats.powers_list.push_back(toInt(power));
				}
			}
			else if (infile.key == "campaign") camp->setAll(infile.val);
			else if (infile.key == "time_played") pc->time_played = toUnsignedLong(infile.val);
			else if (infile.key == "engine_version") save_version = stringToVersion(infile.val);
			else if (SAVE_BUYBACK && infile.key == "buyback_item") {
				std::string npc_filename = popFirstString(infile.val, ';');
				if (!npc_filename.empty()) {
					menu->vendor->buyback_stock[npc_filename].init(NPC::VENDOR_MAX_STOCK);
					menu->vendor->buyback_stock[npc_filename].setItems(infile.val);
				}
			}
			else if (SAVE_BUYBACK && infile.key == "buyback_quantity") {
				std::string npc_filename = popFirstString(infile.val, ';');
				if (!npc_filename.empty()) {
					menu->vendor->buyback_stock[npc_filename].init(NPC::VENDOR_MAX_STOCK);
					menu->vendor->buyback_stock[npc_filename].setQuantities(infile.val);
				}
			}
			else if (infile.key == "questlog_dismissed") pc->questlog_dismissed = toBool(infile.val);
		}

		infile.close();
	}
	else logError("SaveLoad: Unable to open %s!", ss.str().c_str());

	// set starting values for primary stats based on class
	HeroClass* pc_class = getHeroClassByName(pc->stats.character_class);
	if (pc_class) {
		for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
			pc->stats.primary_starting[i] = pc_class->primary[i] + 1;
		}
	}

	// add legacy currency to inventory
	menu->inv->addCurrency(currency);

	// apply stats, inventory, and powers
	applyPlayerData();

	// trigger passive effects here? Saved HP/MP values might depend on passively boosted HP/MP
	// powers->activatePassives(pc->stats);
	if (SAVE_HPMP && saved_hp != 0) {
		if (saved_hp < 0 || saved_hp > pc->stats.get(STAT_HP_MAX)) {
			logError("SaveLoad: HP value is out of bounds, setting to maximum");
			pc->stats.hp = pc->stats.get(STAT_HP_MAX);
		}
		else pc->stats.hp = saved_hp;

		if (saved_mp < 0 || saved_mp > pc->stats.get(STAT_MP_MAX)) {
			logError("SaveLoad: MP value is out of bounds, setting to maximum");
			pc->stats.mp = pc->stats.get(STAT_MP_MAX);
		}
		else pc->stats.mp = saved_mp;
	}
	else {
		pc->stats.hp = pc->stats.get(STAT_HP_MAX);
		pc->stats.mp = pc->stats.get(STAT_MP_MAX);
	}

	if (save_version != ENGINE_VERSION)
		logInfo("SaveLoad: Warning! Engine version of save file (%s) does not match current engine version (%s). Be on the lookout for bugs.", versionToString(save_version).c_str(), versionToString(ENGINE_VERSION).c_str());

	// reset character menu
	menu->chr->refreshStats();

	loadPowerTree();
}

/**
 * Load a class definition, index
 */
void SaveLoad::loadClass(int index) {
	if (game_slot <= 0) return;

	if (index < 0 || static_cast<unsigned>(index) >= HERO_CLASSES.size()) {
		logError("SaveLoad: Class index out of bounds.");
		return;
	}

	pc->stats.character_class = HERO_CLASSES[index].name;
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		// Avatar::init() sets primary stats to 1, so we add to that here
		pc->stats.primary[i] += HERO_CLASSES[index].primary[i];
		pc->stats.primary_starting[i] = pc->stats.primary[i];
	}
	menu->inv->addCurrency(HERO_CLASSES[index].currency);
	menu->inv->inventory[MenuInventory::EQUIPMENT].setItems(HERO_CLASSES[index].equipment);
	for (unsigned i=0; i<HERO_CLASSES[index].powers.size(); i++) {
		pc->stats.powers_list.push_back(HERO_CLASSES[index].powers[i]);
	}
	for (unsigned i=0; i<HERO_CLASSES[index].statuses.size(); i++) {
		camp->setStatus(HERO_CLASSES[index].statuses[i]);
	}
	menu->act->set(HERO_CLASSES[index].hotkeys);

	// Add carried items
	std::string carried = HERO_CLASSES[index].carried;
	ItemStack stack;
	stack.quantity = 1;
	while (carried != "") {
		stack.item = popFirstInt(carried);
		menu->inv->add(stack, MenuInventory::CARRIED, ItemStorage::NO_SLOT, !MenuInventory::ADD_PLAY_SOUND, !MenuInventory::ADD_AUTO_EQUIP);
	}

	// apply stats, inventory, and powers
	applyPlayerData();

	// reset character menu
	menu->chr->refreshStats();

	loadPowerTree();
}

/**
 * This is used to load the stash when starting a new game
 */
void SaveLoad::loadStash() {
	// Load stash
	FileParser infile;
	std::stringstream ss;
	if (pc->stats.permadeath)
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/stash_HC.txt";
	else
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/stash.txt";

	if (infile.open(path(&ss), (FileParser::FULL_PATH | FileParser::NO_ERROR))) {
		while (infile.next()) {
			if (infile.key == "item") {
				menu->stash->stock.setItems(infile.val);
			}
			else if (infile.key == "quantity") {
				menu->stash->stock.setQuantities(infile.val);
			}
		}
		infile.close();
	}
	else logInfo("SaveLoad: Could not open stash file '%s'. This may be because it hasn't been created yet.", ss.str().c_str());

	menu->stash->stock.clean();
}

/**
 * Performs final calculations after loading a save or a new class
 */
void SaveLoad::applyPlayerData() {
	menu->inv->fillEquipmentSlots();

	// remove items with zero quantity from inventory
	menu->inv->inventory[MenuInventory::EQUIPMENT].clean();
	menu->inv->inventory[MenuInventory::CARRIED].clean();

	// Load stash
	loadStash();

	// initialize vars
	pc->stats.recalc();
	pc->stats.loadHeroSFX();
	menu->inv->applyEquipment();
	pc->stats.logic(); // run stat logic once to apply items bonuses

	// just for aesthetics, turn the hero to face the camera
	pc->stats.direction = 6;

	// set up MenuTalker for this hero
	menu->talker->setHero(pc->stats);

	// load sounds (gender specific)
	pc->loadSounds();

	// apply power upgrades
	menu->pow->applyPowerUpgrades();
}

void SaveLoad::loadPowerTree() {
	HeroClass* pc_class = getHeroClassByName(pc->stats.character_class);
	if (pc_class && !pc_class->power_tree.empty()) {
		menu->pow->loadPowerTree(pc_class->power_tree);
		return;
	}

	// fall back to the default power tree
	menu->pow->loadPowerTree("powers/trees/default.txt");
}

