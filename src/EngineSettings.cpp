/*
Copyright © 2018 Justin Jacobs

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
 * class EngineSettings
 */

#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "MenuActionBar.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsParsing.h"

void EngineSettings::load() {
	misc.load();
	resolutions.load();
	gameplay.load();
	combat.load();
	elements.load();
	equip_flags.load();
	primary_stats.load();
	hero_classes.load(); // depends on primary_stats
	damage_types.load();
	death_penalty.load();
	tooltips.load();
	loot.load(); // depends on misc
	tileset.load();
	widgets.load();
	xp.load();
}

void EngineSettings::Misc::load() {
	// reset to defaults
	save_hpmp = false;
	corpse_timeout = 60 * settings->max_frames_per_sec;
	sell_without_vendor = true;
	aim_assist = 0;
	window_title = "Flare";
	save_prefix = "";
	sound_falloff = 15;
	party_exp_percentage = 100;
	enable_ally_collision = true;
	enable_ally_collision_ai = true;
	currency_id = 1;
	interact_range = 3;
	menus_pause = false;
	save_onload = true;
	save_onexit = true;
	save_pos_onexit = false;
	save_oncutscene = true;
	save_onstash = SAVE_ONSTASH_ALL;
	save_anywhere = false;
	camera_speed = 10.f * (static_cast<float>(settings->max_frames_per_sec) / Settings::LOGIC_FPS);
	save_buyback = true;
	keep_buyback_on_map_change = true;
	sfx_unable_to_cast = "";
	combat_aborts_npc_interact = true;
	fogofwar = 1;
	save_fogofwar = true;

	FileParser infile;
	// @CLASS EngineSettings: Misc|Description of engine/misc.txt
	if (infile.open("engine/misc.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR save_hpmp|bool|When saving the game, keep the hero's current HP and MP.
			if (infile.key == "save_hpmp")
				save_hpmp = Parse::toBool(infile.val);
			// @ATTR corpse_timeout|duration|Duration that a corpse can exist on the map in 'ms' or 's'.
			else if (infile.key == "corpse_timeout")
				corpse_timeout = Parse::toDuration(infile.val);
			// @ATTR sell_without_vendor|bool|Allows selling items when not at a vendor via CTRL-Click.
			else if (infile.key == "sell_without_vendor")
				sell_without_vendor = Parse::toBool(infile.val);
			// @ATTR aim_assist|int|The pixel offset for powers that use aim_assist.
			else if (infile.key == "aim_assist")
				aim_assist = Parse::toInt(infile.val);
			// @ATTR window_title|string|Sets the text in the window's titlebar.
			else if (infile.key == "window_title")
				window_title = infile.val;
			// @ATTR save_prefix|string|A string that's prepended to save filenames to prevent conflicts between mods.
			else if (infile.key == "save_prefix")
				save_prefix = infile.val;
			// @ATTR sound_falloff|int|The maximum radius in tiles that any single sound is audible.
			else if (infile.key == "sound_falloff")
				sound_falloff = Parse::toInt(infile.val);
			// @ATTR party_exp_percentage|int|The percentage of XP given to allies.
			else if (infile.key == "party_exp_percentage")
				party_exp_percentage = Parse::toInt(infile.val);
			// @ATTR enable_ally_collision|bool|Allows allies to block the player's path.
			else if (infile.key == "enable_ally_collision")
				enable_ally_collision = Parse::toBool(infile.val);
			// @ATTR enable_ally_collision_ai|bool|Allows allies to block the path of other AI creatures.
			else if (infile.key == "enable_ally_collision_ai")
				enable_ally_collision_ai = Parse::toBool(infile.val);
			else if (infile.key == "currency_id") {
				// @ATTR currency_id|item_id|An item id that will be used as currency.
				currency_id = Parse::toItemID(infile.val);
				if (currency_id < 1) {
					currency_id = 1;
					Utils::logError("EngineSettings: Currency ID below the minimum allowed value. Resetting it to %d", currency_id);
				}
			}
			// @ATTR interact_range|float|Distance where the player can interact with objects and NPCs.
			else if (infile.key == "interact_range")
				interact_range = Parse::toFloat(infile.val);
			// @ATTR menus_pause|bool|Opening any menu will pause the game.
			else if (infile.key == "menus_pause")
				menus_pause = Parse::toBool(infile.val);
			// @ATTR save_onload|bool|Save the game upon changing maps.
			else if (infile.key == "save_onload")
				save_onload = Parse::toBool(infile.val);
			// @ATTR save_onexit|bool|Save the game upon quitting to the title screen or desktop.
			else if (infile.key == "save_onexit")
				save_onexit = Parse::toBool(infile.val);
			// @ATTR save_pos_onexit|bool|If the game gets saved on exiting, store the player's current position instead of the map spawn position.
			else if (infile.key == "save_pos_onexit")
				save_pos_onexit = Parse::toBool(infile.val);
			// @ATTR save_oncutscene|bool|Saves the game when triggering any cutscene via an Event.
			else if (infile.key == "save_oncutscene")
				save_oncutscene = Parse::toBool(infile.val);
			// @ATTR save_onstash|[bool, "private", "shared"]|Saves the game when changing the contents of a stash. The default is true (i.e. save when using both stash types). Use caution with the values "private" and false, since not saving shared stashes exposes an item duplication exploit.
			else if (infile.key == "save_onstash") {
				if (infile.val == "private")
					save_onstash = SAVE_ONSTASH_PRIVATE;
				else if (infile.val == "shared")
					save_onstash = SAVE_ONSTASH_SHARED;
				else {
					if (Parse::toBool(infile.val))
						save_onstash = SAVE_ONSTASH_ALL;
					else
						save_onstash = SAVE_ONSTASH_NONE;
				}
			}
			// @ATTR save_anywhere|bool|Saves the game when using a button.
			else if (infile.key == "save_anywhere")
				save_anywhere = Parse::toBool(infile.val);
			// @ATTR camera_speed|float|Modifies how fast the camera moves to recenter on the player. Larger values mean a slower camera. Default value is 10.
			else if (infile.key == "camera_speed") {
				camera_speed = Parse::toFloat(infile.val);
				if (camera_speed <= 0)
					camera_speed = 1;
			}
			// @ATTR save_buyback|bool|Saves the vendor buyback stock whenever the game is saved.
			else if (infile.key == "save_buyback")
				save_buyback = Parse::toBool(infile.val);
			// @ATTR keep_buyback_on_map_change|bool|If true, NPC buyback stocks will persist when the map changes. If false, save_buyback is disabled.
			else if (infile.key == "keep_buyback_on_map_change")
				keep_buyback_on_map_change = Parse::toBool(infile.val);
			// @ATTR sfx_unable_to_cast|filename|Sound to play when the player lacks the MP to cast a power.
			else if (infile.key == "sfx_unable_to_cast")
				sfx_unable_to_cast = infile.val;
			// @ATTR combat_aborts_npc_interact|bool|If true, the NPC dialog and vendor menus will be closed if the player is attacked.
			else if (infile.key == "combat_aborts_npc_interact")
				combat_aborts_npc_interact = Parse::toBool(infile.val);
			// @ATTR fogofwar|int|0-disabled, 1-tint 2-overlay
			else if (infile.key == "fogofwar")
				fogofwar = static_cast<unsigned short>(Parse::toInt(infile.val));
			// @ATTR fogofwar|bool|If true, the fog of war layer keeps track of the progress.
			else if (infile.key == "save_fogofwar")
				save_fogofwar = Parse::toBool(infile.val);

			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	if (save_prefix == "") {
		Utils::logError("EngineSettings: save_prefix not found in engine/misc.txt, setting to 'default'. This may cause save file conflicts between games that have no save_prefix.");
		save_prefix = "default";
	}

	if (save_buyback && !keep_buyback_on_map_change) {
		Utils::logError("EngineSettings: Warning, save_buyback=true is ignored when keep_buyback_on_map_change=false.");
		save_buyback = false;
	}
}

void EngineSettings::Resolutions::load() {
	// reset to defaults
	frame_w = 0;
	frame_h = 0;
	icon_size = 0;
	min_screen_w = 640;
	min_screen_h = 480;
	virtual_heights.clear();
	virtual_dpi = 0;
	ignore_texture_filter = false;

	FileParser infile;
	// @CLASS EngineSettings: Resolution|Description of engine/resolutions.txt
	if (infile.open("engine/resolutions.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR menu_frame_width|int|Width of frame for New Game, Configuration, etc. menus.
			if (infile.key == "menu_frame_width")
				frame_w = static_cast<unsigned short>(Parse::toInt(infile.val));
			// @ATTR menu_frame_height|int|Height of frame for New Game, Configuration, etc. menus.
			else if (infile.key == "menu_frame_height")
				frame_h = static_cast<unsigned short>(Parse::toInt(infile.val));
			// @ATTR icon_size|int|Size of icons.
			else if (infile.key == "icon_size")
				icon_size = static_cast<unsigned short>(Parse::toInt(infile.val));
			// @ATTR required_width|int|Minimum window/screen resolution width.
			else if (infile.key == "required_width") {
				min_screen_w = static_cast<unsigned short>(Parse::toInt(infile.val));
			}
			// @ATTR required_height|int|Minimum window/screen resolution height.
			else if (infile.key == "required_height") {
				min_screen_h = static_cast<unsigned short>(Parse::toInt(infile.val));
			}
			// @ATTR virtual_height|list(int)|A list of heights (in pixels) that the game can use for its actual rendering area. The virtual height chosen is based on the current window height. The width will be resized to match the window's aspect ratio, and everything will be scaled up to fill the window.
			else if (infile.key == "virtual_height") {
				virtual_heights.clear();
				std::string v_height = Parse::popFirstString(infile.val);
				while (!v_height.empty()) {
					int test_v_height = Parse::toInt(v_height);
					if (test_v_height <= 0) {
						Utils::logError("EngineSettings: virtual_height must be greater than zero.");
					}
					else {
						virtual_heights.push_back(static_cast<unsigned short>(test_v_height));
					}
					v_height = Parse::popFirstString(infile.val);
				}

				std::sort(virtual_heights.begin(), virtual_heights.end());

				if (!virtual_heights.empty()) {
					settings->view_h = virtual_heights.back();
				}

				settings->view_h_half = settings->view_h / 2;
			}
			// @ATTR virtual_dpi|float|A target diagonal screen DPI used to determine how much to scale the internal render resolution.
			else if (infile.key == "virtual_dpi") {
				virtual_dpi = Parse::toFloat(infile.val);
			}
			// @ATTR ignore_texture_filter|bool|If true, this ignores the "Texture Filtering" video setting and uses only nearest-neighbor scaling. This is good for games that use pixel art assets.
			else if (infile.key == "ignore_texture_filter") {
				ignore_texture_filter = Parse::toBool(infile.val);
			}
			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// prevent the window from being too small
	if (settings->screen_w < min_screen_w) settings->screen_w = min_screen_w;
	if (settings->screen_h < min_screen_h) settings->screen_h = min_screen_h;

	// icon size can not be zero, so we set a default of 32x32, which is fantasycore's icon size
	if (icon_size == 0) {
		Utils::logError("EngineSettings: icon_size is undefined. Setting it to 32.");
		icon_size = 32;
	}
}

void EngineSettings::Gameplay::load() {
	// reset to defaults
	enable_playgame = false;

	FileParser infile;
	// @CLASS EngineSettings: Gameplay|Description of engine/gameplay.txt
	if (infile.open("engine/gameplay.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.key == "enable_playgame") {
				// @ATTR enable_playgame|bool|Enables the "Play Game" button on the main menu.
				enable_playgame = Parse::toBool(infile.val);
			}
			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
}

void EngineSettings::Combat::load() {
	// reset to defaults
	min_absorb = 0;
	max_absorb = 100;
	min_resist = 0;
	max_resist = 100;
	min_block = 0;
	max_block = 100;
	min_avoidance = 0;
	max_avoidance = 100;
	min_miss_damage = 0;
	max_miss_damage = 0;
	min_crit_damage = 200;
	max_crit_damage = 200;
	min_overhit_damage = 100;
	max_overhit_damage = 100;

	FileParser infile;
	// @CLASS EngineSettings: Combat|Description of engine/combat.txt
	if (infile.open("engine/combat.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.key == "absorb_percent") {
				// @ATTR absorb_percent|int, int : Minimum, Maximum|Limits the percentage of damage that can be absorbed. A max value less than 100 will ensure that the target always takes at least 1 damage from non-elemental attacks.
				min_absorb = Parse::popFirstInt(infile.val);
				max_absorb = Parse::popFirstInt(infile.val);
				max_absorb = std::max(max_absorb, min_absorb);
			}
			else if (infile.key == "resist_percent") {
				// @ATTR resist_percent|int, int : Minimum, Maximum|Limits the percentage of damage that can be resisted. A max value less than 100 will ensure that the target always takes at least 1 damage from elemental attacks.
				min_resist = Parse::popFirstInt(infile.val);
				max_resist = Parse::popFirstInt(infile.val);
				max_resist = std::max(max_resist, min_resist);
			}
			else if (infile.key == "block_percent") {
				// @ATTR block_percent|int, int : Minimum, Maximum|Limits the percentage of damage that can be absorbed when the target is in the 'block' animation state. A max value less than 100 will ensure that the target always takes at least 1 damage from non-elemental attacks.
				min_block = Parse::popFirstInt(infile.val);
				max_block = Parse::popFirstInt(infile.val);
				max_block = std::max(max_block, min_block);
			}
			else if (infile.key == "avoidance_percent") {
				// @ATTR avoidance_percent|int, int : Minimum, Maximum|Limits the percentage chance that damage will be avoided.
				min_avoidance = Parse::popFirstInt(infile.val);
				max_avoidance = Parse::popFirstInt(infile.val);
				max_avoidance = std::max(max_avoidance, min_avoidance);
			}
			// @ATTR miss_damage_percent|int, int : Minimum, Maximum|The percentage of damage dealt when a miss occurs.
			else if (infile.key == "miss_damage_percent") {
				min_miss_damage = Parse::popFirstInt(infile.val);
				max_miss_damage = Parse::popFirstInt(infile.val);
				max_miss_damage = std::max(max_miss_damage, min_miss_damage);
			}
			// @ATTR crit_damage_percent|int, int : Minimum, Maximum|The percentage of damage dealt when a critical hit occurs.
			else if (infile.key == "crit_damage_percent") {
				min_crit_damage = Parse::popFirstInt(infile.val);
				max_crit_damage = Parse::popFirstInt(infile.val);
				max_crit_damage = std::max(max_crit_damage, min_crit_damage);
			}
			// @ATTR overhit_damage_percent|int, int : Minimum, Maximum|The percentage of damage dealt when an overhit occurs.
			else if (infile.key == "overhit_damage_percent") {
				min_overhit_damage = Parse::popFirstInt(infile.val);
				max_overhit_damage = Parse::popFirstInt(infile.val);
				max_overhit_damage = std::max(max_overhit_damage, min_overhit_damage);
			}

			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
}

void EngineSettings::Elements::load() {
	// reset to defaults
	list.clear();

	FileParser infile;
	// @CLASS EngineSettings: Elements|Description of engine/elements.txt
	if (infile.open("engine/elements.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "element") {
					// check if the previous element and remove it if there is no identifier
					if (!list.empty() && list.back().id == "") {
						list.pop_back();
					}
					list.resize(list.size()+1);
				}
			}

			if (list.empty() || infile.section != "element")
				continue;

			// @ATTR element.id|string|An identifier for this element.
			if (infile.key == "id") list.back().id = infile.val;
			// @ATTR element.name|string|The displayed name of this element.
			else if (infile.key == "name") list.back().name = msg->get(infile.val);

			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();

		// check if the last element and remove it if there is no identifier
		if (!list.empty() && list.back().id == "") {
			list.pop_back();
		}
	}
}

void EngineSettings::EquipFlags::load() {
	// reset to defaults
	list.clear();

	FileParser infile;
	// @CLASS EngineSettings: Equip flags|Description of engine/equip_flags.txt
	if (infile.open("engine/equip_flags.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "flag") {
					// check if the previous flag and remove it if there is no identifier
					if (!list.empty() && list.back().id == "") {
						list.pop_back();
					}
					list.resize(list.size()+1);
				}
			}

			if (list.empty() || infile.section != "flag")
				continue;

			// @ATTR flag.id|string|An identifier for this equip flag.
			if (infile.key == "id") list.back().id = infile.val;
			// @ATTR flag.name|string|The displayed name of this equip flag.
			else if (infile.key == "name") list.back().name = infile.val;

			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();

		// check if the last flag and remove it if there is no identifier
		if (!list.empty() && list.back().id == "") {
			list.pop_back();
		}
	}
}

void EngineSettings::PrimaryStats::load() {
	// reset to defaults
	list.clear();

	FileParser infile;
	// @CLASS EngineSettings: Primary Stats|Description of engine/primary_stats.txt
	if (infile.open("engine/primary_stats.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "stat") {
					// check if the previous stat is empty and remove it if there is no identifier
					if (!list.empty() && list.back().id == "") {
						list.pop_back();
					}
					list.resize(list.size()+1);
				}
			}

			if (list.empty() || infile.section != "stat")
				continue;

			// @ATTR stat.id|string|An identifier for this primary stat.
			if (infile.key == "id") list.back().id = infile.val;
			// @ATTR stat.name|string|The displayed name of this primary stat.
			else if (infile.key == "name") list.back().name = msg->get(infile.val);

			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();

		// check if the last stat is empty and remove it if there is no identifier
		if (!list.empty() && list.back().id == "") {
			list.pop_back();
		}
	}
}

size_t EngineSettings::PrimaryStats::getIndexByID(const std::string& id) {
	for (size_t i = 0; i < list.size(); ++i) {
		if (id == list[i].id)
			return i;
	}

	return list.size();
}

EngineSettings::HeroClasses::HeroClass::HeroClass()
	: name("")
	, description("")
	, currency(0)
	, equipment("")
	, carried("")
	, primary((eset ? eset->primary_stats.list.size() : 0), 0)
	, hotkeys(std::vector<PowerID>(MenuActionBar::SLOT_MAX, 0))
	, power_tree("")
	, default_power_tab(-1)
{
}

void EngineSettings::HeroClasses::load() {
	// reset to defaults
	list.clear();

	FileParser infile;
	// @CLASS EngineSettings: Classes|Description of engine/classes.txt
	if (infile.open("engine/classes.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "class") {
					// check if the previous class and remove it if there is no name
					if (!list.empty() && list.back().name == "") {
						list.pop_back();
					}
					list.resize(list.size()+1);
				}
			}

			if (list.empty() || infile.section != "class")
				continue;

			if (!list.empty()) {
				// @ATTR name|string|The displayed name of this class.
				if (infile.key == "name") list.back().name = infile.val;
				// @ATTR description|string|A description of this class.
				else if (infile.key == "description") list.back().description = infile.val;
				// @ATTR currency|int|The amount of currency this class will start with.
				else if (infile.key == "currency") list.back().currency = Parse::toInt(infile.val);
				// @ATTR equipment|list(item_id)|A list of items that are equipped when starting with this class.
				else if (infile.key == "equipment") list.back().equipment = infile.val;
				// @ATTR carried|list(item_id)|A list of items that are placed in the normal inventorty when starting with this class.
				else if (infile.key == "carried") list.back().carried = infile.val;
				// @ATTR primary|predefined_string, int : Primary stat name, Default value|Class starts with this value for the specified stat.
				else if (infile.key == "primary") {
					std::string prim_stat = Parse::popFirstString(infile.val);
					size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

					if (prim_stat_index != eset->primary_stats.list.size()) {
						list.back().primary[prim_stat_index] = Parse::toInt(infile.val);
					}
					else {
						infile.error("EngineSettings: '%s' is not a valid primary stat.", prim_stat.c_str());
					}
				}

				else if (infile.key == "actionbar") {
					// @ATTR actionbar|list(power_id)|A list of powers to place in the action bar for the class.
					for (int i=0; i<12; i++) {
						list.back().hotkeys[i] = Parse::toPowerID(Parse::popFirstString(infile.val));
					}
				}
				else if (infile.key == "powers") {
					// @ATTR powers|list(power_id)|A list of powers that are unlocked when starting this class.
					std::string power;
					while ( (power = Parse::popFirstString(infile.val)) != "") {
						list.back().powers.push_back(Parse::toPowerID(power));
					}
				}
				else if (infile.key == "campaign") {
					// @ATTR campaign|list(string)|A list of campaign statuses that are set when starting this class.
					std::string status;
					while ( (status = Parse::popFirstString(infile.val)) != "") {
						list.back().statuses.push_back(status);
					}
				}
				else if (infile.key == "power_tree") {
					// @ATTR power_tree|string|Power tree that will be loaded by MenuPowers
					list.back().power_tree = infile.val;
				}
				else if (infile.key == "hero_options") {
					// @ATTR hero_options|list(int)|A list of indicies of the hero options this class can use.
					std::string hero_option;
					while ( (hero_option = Parse::popFirstString(infile.val)) != "") {
						list.back().options.push_back(Parse::toInt(hero_option));
					}

					std::sort(list.back().options.begin(), list.back().options.end());
				}
				else if (infile.key == "default_power_tab") {
					// @ATTR default_power_tab|int|Index of the tab to switch to when opening the Powers menu
					list.back().default_power_tab = Parse::toInt(infile.val);
				}

				else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();

		// check if the last class and remove it if there is no name
		if (!list.empty() && list.back().name == "") {
			list.pop_back();
		}
	}
	// Make a default hero class if none were found
	if (list.empty()) {
		HeroClass c;
		c.name = "Adventurer";
		msg->get("Adventurer"); // this is needed for translation
		list.push_back(c);
	}
}

EngineSettings::HeroClasses::HeroClass* EngineSettings::HeroClasses::getByName(const std::string& name) {
	if (name.empty())
		return NULL;

	for (size_t i = 0; i < list.size(); ++i) {
		if (name == list[i].name) {
			return &list[i];
		}
	}

	return NULL;
}

void EngineSettings::DamageTypes::load() {
	// reset to defaults
	list.clear();
	count = 0;

	FileParser infile;
	// @CLASS EngineSettings: Damage Types|Description of engine/damage_types.txt
	if (infile.open("engine/damage_types.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "damage_type") {
					list.resize(list.size()+1);
				}
			}

			if (list.empty() || infile.section != "damage_type")
				continue;

			if (!list.empty()) {
				// @ATTR damage_type.id|string|The identifier used for Item damage_type and Power base_damage.
				if (infile.key == "id") list.back().id = infile.val;
				// @ATTR damage_type.name|string|The displayed name for the value of this damage type.
				else if (infile.key == "name") list.back().name = msg->get(infile.val);
				// @ATTR damage_type.name_min|string|The displayed name for the minimum value of this damage type.
				else if (infile.key == "name_min") list.back().name_min = msg->get(infile.val);
				// @ATTR damage_type.name_max|string|The displayed name for the maximum value of this damage type.
				else if (infile.key == "name_max") list.back().name_max = msg->get(infile.val);
				// @ATTR damage_type.description|string|The description that will be displayed in the Character menu tooltips.
				else if (infile.key == "description") list.back().description = msg->get(infile.val);
				// @ATTR damage_type.min|string|The identifier used as a Stat type and an Effect type, for the minimum damage of this type.
				else if (infile.key == "min") list.back().min = infile.val;
				// @ATTR damage_type.max|string|The identifier used as a Stat type and an Effect type, for the maximum damage of this type.
				else if (infile.key == "max") list.back().max = infile.val;

				else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
	count = list.size() * 2;

	// use the IDs if the damage type doesn't have printable names
	for (size_t i = 0; i < list.size(); ++i) {
		if (list[i].name.empty()) {
			list[i].name = list[i].id;
		}
		if (list[i].name_min.empty()) {
			list[i].name_min = list[i].min;
		}
		if (list[i].name_max.empty()) {
			list[i].name_max = list[i].max;
		}
	}
}

void EngineSettings::DeathPenalty::load() {
	// reset to defaults
	enabled = true;
	permadeath = false;
	currency = 50;
	xp = 0;
	xp_current = 0;
	item = false;

	FileParser infile;
	// @CLASS EngineSettings: Death penalty|Description of engine/death_penalty.txt
	if (infile.open("engine/death_penalty.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR enable|bool|Enable the death penalty.
			if (infile.key == "enable") enabled = Parse::toBool(infile.val);
			// @ATTR permadeath|bool|Force permadeath for all new saves.
			else if (infile.key == "permadeath") permadeath = Parse::toBool(infile.val);
			// @ATTR currency|int|Remove this percentage of currency.
			else if (infile.key == "currency") currency = Parse::toInt(infile.val);
			// @ATTR xp_total|int|Remove this percentage of total XP.
			else if (infile.key == "xp_total") xp = Parse::toInt(infile.val);
			// @ATTR xp_current_level|int|Remove this percentage of the XP gained since the last level.
			else if (infile.key == "xp_current_level") xp_current = Parse::toInt(infile.val);
			// @ATTR random_item|bool|Removes a random item from the player's inventory.
			else if (infile.key == "random_item") item = Parse::toBool(infile.val);

			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

}

void EngineSettings::Tooltips::load() {
	// reset to defaults
	offset = 0;
	width = 1;
	margin = 0;
	margin_npc = 0;
	background_border = 0;

	FileParser infile;
	// @CLASS EngineSettings: Tooltips|Description of engine/tooltips.txt
	if (infile.open("engine/tooltips.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR tooltip_offset|int|Offset in pixels from the origin point (usually mouse cursor).
			if (infile.key == "tooltip_offset")
				offset = Parse::toInt(infile.val);
			// @ATTR tooltip_width|int|Maximum width of tooltip in pixels.
			else if (infile.key == "tooltip_width")
				width = Parse::toInt(infile.val);
			// @ATTR tooltip_margin|int|Padding between the text and the tooltip borders.
			else if (infile.key == "tooltip_margin")
				margin = Parse::toInt(infile.val);
			// @ATTR npc_tooltip_margin|int|Vertical offset for NPC labels.
			else if (infile.key == "npc_tooltip_margin")
				margin_npc = Parse::toInt(infile.val);
			// @ATTR tooltip_background_border|int|The pixel size of the border in "images/menus/tooltips.png".
			else if (infile.key == "tooltip_background_border")
				background_border = Parse::toInt(infile.val);

			else infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
}

void EngineSettings::Loot::load() {
	// reset to defaults
	tooltip_margin = 0;
	autopickup_currency = false;
	autopickup_range = eset->misc.interact_range;
	currency = "Gold";
	vendor_ratio = 0.25f;
	vendor_ratio_buyback = 0;
	sfx_loot = "";
	drop_max = 1;
	drop_radius = 1;
	hide_radius = 3.f;

	FileParser infile;
	// @CLASS EngineSettings: Loot|Description of engine/loot.txt
	if (infile.open("engine/loot.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.key == "tooltip_margin") {
				// @ATTR tooltip_margin|int|Vertical offset of the loot tooltip from the loot itself.
				tooltip_margin = Parse::toInt(infile.val);
			}
			else if (infile.key == "autopickup_currency") {
				// @ATTR autopickup_currency|bool|Enable autopickup for currency
				autopickup_currency = Parse::toBool(infile.val);
			}
			else if (infile.key == "autopickup_range") {
				// @ATTR autopickup_range|float|Minimum distance the player must be from loot to trigger autopickup.
				autopickup_range = Parse::toFloat(infile.val);
			}
			else if (infile.key == "currency_name") {
				// @ATTR currency_name|string|Define the name of currency in game
				currency = msg->get(infile.val);
			}
			else if (infile.key == "vendor_ratio") {
				// @ATTR vendor_ratio|int|Percentage of item buying price to use as selling price. Also used as the buyback price until the player leaves the map.
				vendor_ratio = static_cast<float>(Parse::toInt(infile.val)) / 100.0f;
			}
			else if (infile.key == "vendor_ratio_buyback") {
				// @ATTR vendor_ratio_buyback|int|Percentage of item buying price to use as the buying price for previously sold items.
				vendor_ratio_buyback = static_cast<float>(Parse::toInt(infile.val)) / 100.0f;
			}
			else if (infile.key == "sfx_loot") {
				// @ATTR sfx_loot|filename|Filename of a sound effect to play for dropping loot.
				sfx_loot = infile.val;
			}
			else if (infile.key == "drop_max") {
				// @ATTR drop_max|int|The maximum number of random item stacks that can drop at once
				drop_max = std::max(Parse::toInt(infile.val), 1);
			}
			else if (infile.key == "drop_radius") {
				// @ATTR drop_radius|int|The distance (in tiles) away from the origin that loot can drop
				drop_radius = std::max(Parse::toInt(infile.val), 1);
			}
			else if (infile.key == "hide_radius") {
				// @ATTR hide_radius|float|If an entity is within this radius relative to a piece of loot, the label will be hidden unless highlighted with the cursor.
				hide_radius = Parse::toFloat(infile.val);
			}
			else {
				infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
}

void EngineSettings::Tileset::load() {
	// reset to defaults
	units_per_pixel_x = 2;
	units_per_pixel_y = 4;
	tile_w = 64;
	tile_h = 32;
	tile_w_half = tile_w/2;
	tile_h_half = tile_h/2;
	orientation = TILESET_ISOMETRIC;

	FileParser infile;
	// @CLASS EngineSettings: Tileset config|Description of engine/tileset_config.txt
	if (infile.open("engine/tileset_config.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.key == "tile_size") {
				// @ATTR tile_size|int, int : Width, Height|The width and height of a tile.
				tile_w = static_cast<unsigned short>(Parse::popFirstInt(infile.val));
				tile_h = static_cast<unsigned short>(Parse::popFirstInt(infile.val));
				tile_w_half = tile_w /2;
				tile_h_half = tile_h /2;
			}
			else if (infile.key == "orientation") {
				// @ATTR orientation|["isometric", "orthogonal"]|The perspective of tiles; isometric or orthogonal.
				if (infile.val == "isometric")
					orientation = TILESET_ISOMETRIC;
				else if (infile.val == "orthogonal")
					orientation = TILESET_ORTHOGONAL;
			}
			else {
				infile.error("EngineSettings: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
	else {
		Utils::logError("Unable to open engine/tileset_config.txt! Defaulting to 64x32 isometric tiles.");
	}

	// Init automatically calculated parameters
	if (orientation == TILESET_ISOMETRIC) {
		if (tile_w > 0 && tile_h > 0) {
			units_per_pixel_x = 2.0f / tile_w;
			units_per_pixel_y = 2.0f / tile_h;
		}
		else {
			Utils::logError("EngineSettings: Tile dimensions must be greater than 0. Resetting to the default size of 64x32.");
			tile_w = 64;
			tile_h = 32;
		}
	}
	else { // TILESET_ORTHOGONAL
		if (tile_w > 0 && tile_h > 0) {
			units_per_pixel_x = 1.0f / tile_w;
			units_per_pixel_y = 1.0f / tile_h;
		}
		else {
			Utils::logError("EngineSettings: Tile dimensions must be greater than 0. Resetting to the default size of 64x32.");
			tile_w = 64;
			tile_h = 32;
		}
	}
	if (units_per_pixel_x == 0 || units_per_pixel_y == 0) {
		Utils::logError("EngineSettings: One of UNITS_PER_PIXEL values is zero! %dx%d", static_cast<int>(units_per_pixel_x), static_cast<int>(units_per_pixel_y));
		Utils::logErrorDialog("EngineSettings: One of UNITS_PER_PIXEL values is zero! %dx%d", static_cast<int>(units_per_pixel_x), static_cast<int>(units_per_pixel_y));
		mods->resetModConfig();
		Utils::Exit(1);
	}
};

void EngineSettings::Widgets::load() {
	// reset to defaults
	selection_rect_color = Color(255, 248, 220, 255);
	colorblind_highlight_offset = Point(2, 2);

	tab_padding = Point(8, 0);

	slot_quantity_label = LabelInfo();
	slot_quantity_color = font->getColor(FontEngine::COLOR_WIDGET_NORMAL);
	slot_quantity_bg_color = Color(0, 0, 0, 0);
	slot_hotkey_label = LabelInfo();
	slot_hotkey_color = font->getColor(FontEngine::COLOR_WIDGET_NORMAL);
	slot_hotkey_label.hidden = true;
	slot_hotkey_bg_color = Color(0, 0, 0, 0);

	listbox_text_margin = Point(8, 8);

	horizontal_list_text_width = 150;

	scrollbar_bg_color = Color(0,0,0,64);

	FileParser infile;
	// @CLASS EngineSettings: Widgets|Description of engine/widget_settings.txt
	if (infile.open("engine/widget_settings.txt", FileParser::MOD_FILE, FileParser::ERROR_NONE)) {
		while (infile.next()) {
			if (infile.section == "misc") {
				if (infile.key == "selection_rect_color") {
					// @ATTR misc.selection_rect_color|color, int : Color, Alpha|Color of the selection rectangle when navigating widgets without a mouse.
					selection_rect_color = Parse::toRGBA(infile.val);
				}
				else if (infile.key == "colorblind_highlight_offset") {
					// @ATTR misc.colorblind_highlight_offset|int, int : X offset, Y offset|The pixel offset of the '*' marker on highlighted icons in colorblind mode.
					colorblind_highlight_offset = Parse::toPoint(infile.val);
				}
			}
			else if (infile.section == "tab") {
				if (infile.key == "padding") {
					// @ATTR tab.padding|int, int : Left/right padding, Top padding|The pixel padding around tabs. Controls how the left and right edges are drawn.
					tab_padding = Parse::toPoint(infile.val);
				}
			}
			else if (infile.section == "slot") {
				if (infile.key == "quantity_label") {
					// @ATTR slot.quantity_label|label|Setting for the slot quantity text.
					slot_quantity_label = Parse::popLabelInfo(infile.val);
				}
				else if (infile.key == "quantity_color") {
					// @ATTR slot.quantity_color|color|Text color for the slot quantity text.
					slot_quantity_color = Parse::toRGB(infile.val);
				}
				else if (infile.key == "quantity_bg_color") {
					// @ATTR slot.quantity_bg_color|color, int : Color, Alpha|If a slot has a quantity, a rectangle filled with this color will be placed beneath the text.
					slot_quantity_bg_color = Parse::toRGBA(infile.val);
				}
				else if (infile.key == "hotkey_label") {
					// @ATTR slot.hotkey_label|label|Setting for the slot hotkey text.
					slot_hotkey_label = Parse::popLabelInfo(infile.val);
				}
				else if (infile.key == "hotkey_color") {
					// @ATTR slot.hotkey_color|color|Text color for the slot hotkey text.
					slot_hotkey_color = Parse::toRGB(infile.val);
				}
				else if (infile.key == "hotkey_bg_color") {
					// @ATTR slot.hotkey_bg_color|color, int : Color, Alpha|If a slot has a hotkey, a rectangle filled with this color will be placed beneath the text.
					slot_hotkey_bg_color = Parse::toRGBA(infile.val);
				}
			}
			else if (infile.section == "listbox") {
				if (infile.key == "text_margin") {
					// @ATTR listbox.text_margin|int, int : Left margin, Right margin|The pixel margin to leave on the left and right sides of listbox element text.
					listbox_text_margin = Parse::toPoint(infile.val);
				}
			}
			else if (infile.section == "horizontal_list") {
				if (infile.key == "text_width") {
					// @ATTR horizontal_list.text_width|int|The pixel width of the text area that displays the currently selected item. Default is 150 pixels;
					horizontal_list_text_width = Parse::toInt(infile.val);
				}
			}
			else if (infile.section == "scrollbar") {
				if (infile.key == "bg_color") {
					// @ATTR scrollbar.bg_color|color, int : Color, Alpha|The background color for the entire scrollbar.
					scrollbar_bg_color = Parse::toRGBA(infile.val);
				}
			}
		}
	}
}

void EngineSettings::XPTable::load() {
	xp_table.clear();

	FileParser infile;
	// @CLASS EngineSettings: XP table|Description of engine/xp_table.txt
	if (infile.open("engine/xp_table.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (infile.key == "level") {
				// @ATTR level|int, int : Level, XP|The amount of XP required for this level.
				unsigned lvl_id = Parse::popFirstInt(infile.val);
				unsigned long lvl_xp = Parse::toUnsignedLong(Parse::popFirstString(infile.val));

				if (lvl_id > xp_table.size())
					xp_table.resize(lvl_id);

				xp_table[lvl_id - 1] = lvl_xp;
			}
		}
		infile.close();
	}

	if (xp_table.empty()) {
		Utils::logError("EngineSettings: No XP table defined.");
		xp_table.push_back(0);
	}
}

unsigned long EngineSettings::XPTable::getLevelXP(int level) {
	if (level <= 1 || xp_table.empty())
		return 0;
	else if (level > static_cast<int>(xp_table.size()))
		return xp_table.back();
	else
		return xp_table[level - 1];
}

int EngineSettings::XPTable::getMaxLevel() {
	return static_cast<int>(xp_table.size());
}

int EngineSettings::XPTable::getLevelFromXP(unsigned long level_xp) {
	int level = 0;

	for (size_t i = 0; i < xp_table.size(); ++i) {
		if (level_xp >= xp_table[i])
			level = static_cast<int>(i + 1);
	}

	return level;
}
