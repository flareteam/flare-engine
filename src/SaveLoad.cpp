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
#include "EngineSettings.h"
#include "FileParser.h"
#include "FogOfWar.h"
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
	Utils::createSaveDir(game_slot);

	// remove items with zero quantity from inventory
	menu->inv->inventory[MenuInventory::EQUIPMENT].clean();
	menu->inv->inventory[MenuInventory::CARRIED].clean();

	std::ofstream outfile;

	std::stringstream ss;
	ss << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << game_slot << "/avatar.txt";

	outfile.open(Filesystem::convertSlashes(ss.str()).c_str(), std::ios::out);

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
		if (eset->misc.save_hpmp) outfile << "hpmp=" << pc->stats.hp << "," << pc->stats.mp << "\n";

		// stat spec
		outfile << "build=";
		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			outfile << pc->stats.primary[i];
			if (i < eset->primary_stats.list.size() - 1)
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
		// NOTE we need to reset any bonus-modified powers in the action bar before writing
		// we use menu->pow->setUnlockedPowers() after to restore the action bar state
		menu->pow->clearActionBarBonusLevels();
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
		menu->pow->setUnlockedPowers();

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
		outfile << "engine_version=" << VersionInfo::ENGINE.getString() << "\n";

		// save the vendor buyback
		if (eset->misc.save_buyback) {
			std::map<std::string, ItemStorage>::iterator it;

			for (it = menu->vendor->buyback_stock.begin(); it != menu->vendor->buyback_stock.end(); ++it) {
				if (it->second.empty())
					continue;

				outfile << "buyback_item=" << it->first << ";" << it->second.getItems() << "\n";
				outfile << "buyback_quantity=" << it->first << ";" << it->second.getQuantities() << "\n";
			}
		}

		outfile << "questlog_dismissed=" << !menu->act->requires_attention[MenuActionBar::MENU_LOG] << "\n";

		outfile << "stash_tab=" << menu->stash->getTab();

		outfile << std::endl;

		if (outfile.bad()) Utils::logError("SaveLoad: Unable to save the game. No write access or disk is full!");
		outfile.close();
		outfile.clear();

		platform.FSCommit();
	}

	// Save stashes
	for (size_t i = 0; i < menu->stash->tabs.size(); ++i) {
		// shared stashes are not saved for permadeath characters
		if (pc->stats.permadeath && !menu->stash->tabs[i].is_private)
			continue;

		ss.str("");
		ss << settings->path_user << "saves/" << eset->misc.save_prefix;
		if (menu->stash->tabs[i].is_private)
			ss << "/" << game_slot;
		ss << "/" << menu->stash->tabs[i].filename;
		outfile.open(Filesystem::convertSlashes(ss.str()).c_str(), std::ios::out);

		if (outfile.is_open()) {

			// comment
			outfile << "# flare-engine stash file: \"" << menu->stash->tabs[i].id << "\"\n";

			outfile << "quantity=" << menu->stash->tabs[i].stock.getQuantities() << "\n";
			outfile << "item=" << menu->stash->tabs[i].stock.getItems() << "\n";

			outfile << std::endl;

			if (outfile.bad()) Utils::logError("SaveLoad: Unable to save stash. No write access or disk is full!");
			outfile.close();
			outfile.clear();

			platform.FSCommit();
		}
	}
	
	// Save fow
	if (eset->misc.fogofwar && eset->misc.save_fogofwar) {
		ss.str("");
		ss << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << game_slot << "/" << mapr->getFilename();

		outfile.open(Filesystem::convertSlashes(ss.str()).c_str(), std::ios::out);
		
		if (outfile.is_open()) {
			outfile << "[layer]" << std::endl;

			outfile << "type=" << mapr->layernames[fow->layer_id] << std::endl;
			outfile << "data=" << std::endl;

			std::string layer = "";
			for (int line = 0; line < mapr->h; line++) {
				std::stringstream map_row;
				for (int tile = 0; tile < mapr->w; tile++) {
					unsigned short val = mapr->layers[fow->layer_id][tile][line];
					//if (val == FogOfWar::TILE_SIGHT) val = FogOfWar::TILE_VISITED;
					map_row << val << ",";
				}
				layer += map_row.str();
				layer += '\n';
			}
			layer.erase(layer.end()-2, layer.end());
			layer += '\n';

			outfile << layer << std::endl;
			
			if (outfile.bad()) Utils::logError("SaveLoad: Unable to save map data. No write access or disk is full!");
			outfile.close();
			outfile.clear();

			platform.FSCommit();
		}
	}

	settings->prev_save_slot = game_slot-1;

	// display a log message saying that we saved the game
	menu->questlog->add(msg->get("Game saved."), MenuLog::TYPE_MESSAGES, WidgetLog::MSG_NORMAL);
	menu->hudlog->add(msg->get("Game saved."), MenuHUDLog::MSG_NORMAL);
}

/**
 * When loading the game, load from file if possible
 */
void SaveLoad::loadGame() {
	if (game_slot <= 0) return;

	int saved_hp = 0;
	int saved_mp = 0;
	int currency = 0;
	size_t stash_tab = 0;
	Version save_version(VersionInfo::MIN);

	FileParser infile;
	std::vector<PowerID> hotkeys(MenuActionBar::SLOT_MAX, -1);

	std::stringstream ss;
	ss << settings->path_user << "saves/" << eset->misc.save_prefix << "/" << game_slot << "/avatar.txt";

	if (infile.open(ss.str(), !FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.key == "name") pc->stats.name = infile.val;
			else if (infile.key == "permadeath") {
				pc->stats.permadeath = Parse::toBool(infile.val);
			}
			else if (infile.key == "option") {
				pc->stats.gfx_base = Parse::popFirstString(infile.val);
				pc->stats.gfx_head = Parse::popFirstString(infile.val);
				pc->stats.gfx_portrait = Parse::popFirstString(infile.val);
			}
			else if (infile.key == "class") {
				pc->stats.character_class = Parse::popFirstString(infile.val);
				pc->stats.character_subclass = Parse::popFirstString(infile.val);
			}
			else if (infile.key == "xp") {
				pc->stats.xp = Parse::toUnsignedLong(infile.val);
			}
			else if (infile.key == "hpmp") {
				saved_hp = Parse::popFirstInt(infile.val);
				saved_mp = Parse::popFirstInt(infile.val);
			}
			else if (infile.key == "build") {
				for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
					pc->stats.primary[i] = Parse::popFirstInt(infile.val);
					if (pc->stats.primary[i] < 0 || pc->stats.primary[i] > pc->stats.max_points_per_stat) {
						Utils::logInfo("SaveLoad: Primary stat value for '%s' is out of bounds, setting to zero.", eset->primary_stats.list[i].id.c_str());
						pc->stats.primary[i] = 0;
					}
				}
			}
			else if (infile.key == "currency") {
				currency = Parse::toInt(infile.val);
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
				mapr->teleport_mapname = Parse::popFirstString(infile.val);
				if (mapr->teleport_mapname != "" && Filesystem::fileExists(mods->locate(mapr->teleport_mapname))) {
					mapr->teleport_destination.x = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
					mapr->teleport_destination.y = static_cast<float>(Parse::popFirstInt(infile.val)) + 0.5f;
					mapr->teleportation = true;
					// prevent spawn.txt from putting us on the starting map
					mapr->clearEvents();
				}
				else {
					Utils::logError("SaveLoad: Unable to find %s, loading maps/spawn.txt", mapr->teleport_mapname.c_str());
					mapr->teleport_mapname = "maps/spawn.txt";
					mapr->teleport_destination.x = 0.5f;
					mapr->teleport_destination.y = 0.5f;
					mapr->teleportation = true;
				}
			}
			else if (infile.key == "actionbar") {
				for (int i = 0; i < MenuActionBar::SLOT_MAX; i++) {
					hotkeys[i] = Parse::popFirstInt(infile.val);
					if (hotkeys[i] != 0 && powers->powers[hotkeys[i]].is_empty) {
						Utils::logError("SaveLoad: Hotkey power with id=%d, found on position %d does not exist, skipping", hotkeys[i], i);
						hotkeys[i] = 0;
					}
				}
				menu->act->set(hotkeys, !MenuActionBar::SET_SKIP_EMPTY);
			}
			else if (infile.key == "transformed") {
				pc->stats.transform_type = Parse::popFirstString(infile.val);
				if (pc->stats.transform_type != "") {
					pc->stats.transform_duration = -1;
					pc->stats.manual_untransform = Parse::toBool(Parse::popFirstString(infile.val));
				}
			}
			else if (infile.key == "powers") {
				std::string power;
				while ( (power = Parse::popFirstString(infile.val)) != "") {
					if (Parse::toInt(power) > 0)
						pc->stats.powers_list.push_back(Parse::toInt(power));
				}
			}
			else if (infile.key == "campaign") camp->setAll(infile.val);
			else if (infile.key == "time_played") pc->time_played = Parse::toUnsignedLong(infile.val);
			else if (infile.key == "engine_version") save_version.setFromString(infile.val);
			else if (eset->misc.save_buyback && infile.key == "buyback_item") {
				std::string npc_filename = Parse::popFirstString(infile.val, ';');
				if (!npc_filename.empty()) {
					menu->vendor->buyback_stock[npc_filename].init(NPC::VENDOR_MAX_STOCK);
					menu->vendor->buyback_stock[npc_filename].setItems(infile.val);
				}
			}
			else if (eset->misc.save_buyback && infile.key == "buyback_quantity") {
				std::string npc_filename = Parse::popFirstString(infile.val, ';');
				if (!npc_filename.empty()) {
					menu->vendor->buyback_stock[npc_filename].init(NPC::VENDOR_MAX_STOCK);
					menu->vendor->buyback_stock[npc_filename].setQuantities(infile.val);
				}
			}
			else if (infile.key == "questlog_dismissed") pc->questlog_dismissed = Parse::toBool(infile.val);
			else if (infile.key == "stash_tab") stash_tab = Parse::toInt(infile.val);
		}

		infile.close();
	}
	else Utils::logError("SaveLoad: Unable to open %s!", ss.str().c_str());

	// set starting values for primary stats based on class
	EngineSettings::HeroClasses::HeroClass* pc_class;
	pc_class = eset->hero_classes.getByName(pc->stats.character_class);
	if (pc_class) {
		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			pc->stats.primary_starting[i] = pc_class->primary[i] + 1;
		}
	}

	// add legacy currency to inventory
	menu->inv->addCurrency(currency);

	// apply stats, inventory, and powers
	applyPlayerData();

	// trigger passive effects here? Saved HP/MP values might depend on passively boosted HP/MP
	// powers->activatePassives(pc->stats);
	if (eset->misc.save_hpmp && saved_hp != 0) {
		if (saved_hp < 0 || saved_hp > pc->stats.get(Stats::HP_MAX)) {
			Utils::logError("SaveLoad: HP value is out of bounds, setting to maximum");
			pc->stats.hp = pc->stats.get(Stats::HP_MAX);
		}
		else pc->stats.hp = saved_hp;

		if (saved_mp < 0 || saved_mp > pc->stats.get(Stats::MP_MAX)) {
			Utils::logError("SaveLoad: MP value is out of bounds, setting to maximum");
			pc->stats.mp = pc->stats.get(Stats::MP_MAX);
		}
		else pc->stats.mp = saved_mp;
	}
	else {
		pc->stats.hp = pc->stats.get(Stats::HP_MAX);
		pc->stats.mp = pc->stats.get(Stats::MP_MAX);
	}

	if (save_version != VersionInfo::ENGINE)
		Utils::logInfo("SaveLoad: Warning! Engine version of save file (%s) does not match current engine version (%s). Be on the lookout for bugs.", save_version.getString().c_str(), VersionInfo::ENGINE.getString().c_str());

	// reset character menu
	menu->chr->refreshStats();

	loadPowerTree();

	// disable the shared stash for permadeath characters
	menu->stash->enableSharedTab(pc->stats.permadeath);

	menu->stash->setTab(stash_tab);
}

/**
 * Load a class definition, index
 */
void SaveLoad::loadClass(int index) {
	if (game_slot <= 0) return;

	if (index < 0 || static_cast<unsigned>(index) >= eset->hero_classes.list.size()) {
		Utils::logError("SaveLoad: Class index out of bounds.");
		return;
	}

	pc->stats.character_class = eset->hero_classes.list[index].name;
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		// Avatar::init() sets primary stats to 1, so we add to that here
		pc->stats.primary[i] += eset->hero_classes.list[index].primary[i];
		pc->stats.primary_starting[i] = pc->stats.primary[i];
	}
	menu->inv->addCurrency(eset->hero_classes.list[index].currency);
	menu->inv->inventory[MenuInventory::EQUIPMENT].setItems(eset->hero_classes.list[index].equipment);
	for (unsigned i=0; i<eset->hero_classes.list[index].powers.size(); i++) {
		pc->stats.powers_list.push_back(eset->hero_classes.list[index].powers[i]);
	}
	for (unsigned i=0; i<eset->hero_classes.list[index].statuses.size(); i++) {
		StatusID class_status = camp->registerStatus(eset->hero_classes.list[index].statuses[i]);
		camp->setStatus(class_status);
	}
	menu->act->set(eset->hero_classes.list[index].hotkeys, !MenuActionBar::SET_SKIP_EMPTY);

	// Add carried items
	std::string carried = eset->hero_classes.list[index].carried;
	ItemStack stack;
	stack.quantity = 1;
	while (carried != "") {
		stack.item = Parse::popFirstInt(carried);
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

	for (size_t i = 0; i < menu->stash->tabs.size(); ++i) {
		// shared stashes are not loaded for permadeath characters
		if (pc->stats.permadeath && !menu->stash->tabs[i].is_private)
			continue;

		ss.str("");
		ss << settings->path_user << "saves/" << eset->misc.save_prefix;
		if (menu->stash->tabs[i].is_private)
			ss << "/" << game_slot;
		ss << "/" << menu->stash->tabs[i].filename;

		if (infile.open(ss.str(), !FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
			while (infile.next()) {
				if (infile.key == "item") {
					menu->stash->tabs[i].stock.setItems(infile.val);
				}
				else if (infile.key == "quantity") {
					menu->stash->tabs[i].stock.setQuantities(infile.val);
				}
			}
			infile.close();
		}
		else Utils::logInfo("SaveLoad: Could not open stash file '%s'. This may be because it hasn't been created yet.", ss.str().c_str());

		menu->stash->tabs[i].stock.clean();
	}
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
	menu->pow->setUnlockedPowers();
}

void SaveLoad::loadPowerTree() {
	EngineSettings::HeroClasses::HeroClass* pc_class;
	pc_class = eset->hero_classes.getByName(pc->stats.character_class);
	if (pc_class && !pc_class->power_tree.empty()) {
		menu->pow->loadPowerTree(pc_class->power_tree);
		return;
	}

	// fall back to the default power tree
	menu->pow->loadPowerTree("powers/trees/default.txt");
}
