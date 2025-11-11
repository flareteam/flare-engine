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

#ifndef ENGINESETTINGS_H
#define ENGINESETTINGS_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "WidgetLabel.h"

class EngineSettings {
public:
	void load();

	class Misc {
	public:
		void load();

		enum {
			SAVE_ONSTASH_NONE = 0,
			SAVE_ONSTASH_PRIVATE = 1,
			SAVE_ONSTASH_SHARED = 2,
			SAVE_ONSTASH_ALL = 3,
		};

		bool save_hpmp;
		int corpse_timeout;
		bool corpse_timeout_enabled;
		bool sell_without_vendor;
		int aim_assist;
		std::string window_title;
		std::string save_prefix;
		int sound_falloff;
		float party_exp_percentage;
		bool enable_ally_collision;
		bool enable_ally_collision_ai;
		ItemID currency_id;
		float interact_range;
		bool menus_pause;
		bool save_onload;
		bool save_onexit;
		bool save_pos_onexit;
		bool save_oncutscene;
		int save_onstash;
		bool save_anywhere;
		float camera_speed;
		bool save_buyback;
		bool keep_buyback_on_map_change;
		std::string sfx_unable_to_cast;
		bool combat_aborts_npc_interact;
		unsigned short fogofwar;
		bool save_fogofwar;
		bool mouse_move_enabled;
		float mouse_move_deadzone_moving;
		float mouse_move_deadzone_not_moving;
		bool passive_trigger_effect_stacking;
		uint8_t fade_wall_alpha;
	};

	class Resolutions {
	public:
		void load();

		unsigned short frame_w;
		unsigned short frame_h;
		unsigned short icon_size;
		unsigned short min_screen_w;
		unsigned short min_screen_h;
		std::vector<unsigned short> virtual_heights;
		float virtual_dpi;
		bool ignore_texture_filter;
	};

	class Gameplay {
	public:
		void load();

		bool enable_playgame;
	};

	class Combat {
	public:
		void load();
		float resourceRound(const float resource_val);

		enum {
			RESOURCE_ROUND_METHOD_NONE = 0,
			RESOURCE_ROUND_METHOD_ROUND,
			RESOURCE_ROUND_METHOD_FLOOR,
			RESOURCE_ROUND_METHOD_CEIL
		};

		float min_absorb;
		float max_absorb;
		float min_resist;
		float max_resist;
		float min_block;
		float max_block;
		float min_avoidance;
		float max_avoidance;
		float min_miss_damage;
		float max_miss_damage;
		float min_crit_damage;
		float max_crit_damage;
		float min_overhit_damage;
		float max_overhit_damage;
		unsigned short resource_round_method;
		bool offscreen_enemy_encounters;
	};

	class EquipFlags {
	public:
		class EquipFlag {
		public:
			std::string id;
			std::string name;
		};

		void load();

		std::vector<EquipFlag> list;
	};

	class PrimaryStats {
	public:
		class PrimaryStat {
		public:
			std::string id;
			std::string name;
		};

		void load();
		size_t getIndexByID(const std::string& id);

		std::vector<PrimaryStat> list;
	};

	class HeroClasses {
	public:
		class HeroClass {
		public:
			HeroClass();

			std::string name;
			std::string description;
			int currency;
			std::string equipment;
			std::string carried;
			std::vector<int> primary;
			std::vector<PowerID> hotkeys;
			std::vector<PowerID> powers;
			std::vector<std::string> statuses;
			std::string power_tree;
			int default_power_tab;
			std::vector<int> options;
			std::vector< std::pair<unsigned, std::string> > equipment_sets;
		};

		void load();
		HeroClass* getByName(const std::string& name);

		std::vector<HeroClass> list;
	};

	class DamageTypes {
	public:
		class DamageType {
		public:
			DamageType();

			bool is_elemental;
			bool is_deprecated_element;
			std::string id;
			std::string name;
			std::string name_short;
			std::string name_min;
			std::string name_max;
			std::string name_resist;
			std::string description;
			std::string min;
			std::string max;
			std::string resist;
		};

		void load();
		static size_t indexToMin(size_t list_index);
		static size_t indexToMax(size_t list_index);
		static size_t indexToResist(size_t list_index);

		std::vector<DamageType> list;
		size_t count; // damage_types.size() * 3, to account for min, max, and resist
	};

	class DeathPenalty {
	public:
		void load();

		bool enabled;
		bool permadeath;
		float currency;
		float xp;
		float xp_current;
		bool item;
	};

	class Tooltips {
	public:
		void load();

		int offset;
		int width;
		int margin;
		int margin_npc;
		int background_border;
		size_t visible_max;
	};

	class Loot {
	public:
		void load();

		int tooltip_margin;
		bool autopickup_currency;
		float autopickup_range;
		std::string currency;
		float vendor_ratio_buy;
		float vendor_ratio_sell;
		float vendor_ratio_sell_old;
		std::string sfx_loot;
		int drop_max;
		int drop_radius;
		float hide_radius;
		ItemID extended_items_offset;
	};

	class Tileset {
	public:
		void load();

		enum {
			TILESET_ISOMETRIC = 0,
			TILESET_ORTHOGONAL = 1
		};

		float units_per_pixel_x;
		float units_per_pixel_y;
		unsigned short tile_w;
		unsigned short tile_h;
		unsigned short tile_w_half;
		unsigned short tile_h_half;
		unsigned short orientation;
	};

	class Widgets {
	public:
		void load();

		Color selection_rect_color;
		int selection_rect_corner_size;
		Point colorblind_highlight_offset;
		Point tab_padding;
		int tab_text_padding;
		LabelInfo slot_quantity_label;
		Color slot_quantity_color;
		Color slot_quantity_bg_color;
		LabelInfo slot_hotkey_label;
		Color slot_hotkey_color;
		Color slot_hotkey_bg_color;
		Point listbox_text_margin;
		int horizontal_list_text_width;
		Color scrollbar_bg_color;
		int log_padding;
		std::string sound_activate;
	};

	class XPTable {
	public:
		void load();

		unsigned long getLevelXP(int level);
		int getMaxLevel();
		int getLevelFromXP(unsigned long level_xp);

	private:
		std::vector<unsigned long> xp_table;
	};

	class NumberFormat {
	public:
		void load();

		int player_statbar;
		int enemy_statbar;
		int combat_text;
		int character_menu;
		int item_tooltips;
		int power_tooltips;
		int durations;
		int death_penalty;
	};

	class ResourceStats {
	public:
		enum {
			// statblock & effect
			STAT_BASE = 0,
			STAT_REGEN,
			STAT_STEAL,
			STAT_RESIST_STEAL,

			// effect only
			STAT_HEAL,
			STAT_HEAL_PERCENT,

			STAT_EFFECT_COUNT
		};
		static const size_t STAT_COUNT = STAT_HEAL;
		static const size_t EFFECT_COUNT = STAT_EFFECT_COUNT - STAT_COUNT;

		class ResourceStat {
		public:
			ResourceStat();

			std::vector<std::string> ids;
			std::vector<std::string> text;
			std::vector<std::string> text_desc;

			std::string menu_filename;

			std::string text_combat_heal;
			std::string text_log_restore;
			std::string text_log_low;
			std::string text_tooltip_heal;
			std::string text_tooltip_cost;
		};

		void load();

		std::vector<ResourceStat> list;
		size_t stat_count;
		size_t effect_count;
		size_t stat_effect_count;
	};

	Misc misc;
	Resolutions resolutions;
	Gameplay gameplay;
	Combat combat;
	EquipFlags equip_flags;
	PrimaryStats primary_stats;
	HeroClasses hero_classes;
	DamageTypes damage_types;
	DeathPenalty death_penalty;
	Tooltips tooltips;
	Loot loot;
	Tileset tileset;
	Widgets widgets;
	XPTable xp;
	NumberFormat number_format;
	ResourceStats resource_stats;
};

#endif // ENGINESETTINGS_H
