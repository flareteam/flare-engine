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
 * class MenuCharacter
 */

#include <string>

#include "Avatar.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "Menu.h"
#include "MenuCharacter.h"
#include "MessageEngine.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "StatBlock.h"
#include "TooltipManager.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetListBox.h"

MenuCharacter::MenuCharacter()
	: closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, labelCharacter(new WidgetLabel())
	, labelUnspent(new WidgetLabel())
	, skill_points(0)
	, statlist_rows(10)
	, statlist_scrollbar_offset(0)
	, show_resists(true)
	, name_max_width(0)
{
	labelCharacter->setText(msg->get("Character"));
	labelCharacter->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));
	labelUnspent->setColor(font->getColor(FontEngine::COLOR_MENU_BONUS));

	// 2 is added here to account for CSTAT_NAME and CSTAT_LEVEL
	cstat.resize(eset->primary_stats.list.size() + 2);

	// Labels for major stats
	for (size_t i = 0; i < cstat.size(); ++i) {
		cstat[i].label = new WidgetLabel();
		cstat[i].value = new WidgetLabel();
		cstat[i].hover.x = cstat[i].hover.y = 0;
		cstat[i].hover.w = cstat[i].hover.h = 0;

		cstat[i].label->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

		cstat[i].value->setVAlign(LabelInfo::VALIGN_CENTER);
		cstat[i].value->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));
	}
	cstat[CSTAT_NAME].label->setText(msg->get("Name"));
	cstat[CSTAT_LEVEL].label->setText(msg->get("Level"));
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		cstat[i+2].label->setText(eset->primary_stats.list[i].name);
	}
	cstat[CSTAT_NAME].label->setText(msg->get("Name"));
	cstat[CSTAT_LEVEL].label->setText(msg->get("Level"));
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		cstat[i+2].label->setText(eset->primary_stats.list[i].name);
	}

	show_stat.resize(Stats::COUNT + eset->damage_types.count + eset->resource_stats.stat_count + 2);
	for (size_t i = 0; i < show_stat.size(); i++) {
		if (i >= Stats::RESIST_DAMAGE_OVER_TIME && i < Stats::COUNT) {
			// some stats are hidden by default
			show_stat[i] = false;
		}
		else if (i >= Stats::COUNT && i < Stats::COUNT + eset->damage_types.count) {
			size_t damage_sub_index = i - Stats::COUNT;
			size_t damage_index = damage_sub_index / 3;
			size_t damage_sub_stat = damage_sub_index % 3;

			if (eset->damage_types.list[damage_index].is_deprecated_element && damage_sub_stat != 2) {
				// don't show damage for elements loaded from engine/elements.txt by default
				show_stat[i] = false;
			}
			else if (!eset->damage_types.list[damage_index].is_elemental && damage_sub_stat == 2) {
				// don't show resists for non-elemental damage types by default
				show_stat[i] = false;
			}
			else {
				show_stat[i] = true;
			}
		}
		else {
			show_stat[i] = true;
		}
	}

	// Upgrade buttons
	primary_up.resize(eset->primary_stats.list.size());
	upgradeButton.resize(eset->primary_stats.list.size());

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		primary_up[i] = false;
		upgradeButton[i] = new WidgetButton("images/menus/buttons/upgrade.png");
		upgradeButton[i]->enabled = false;
	}

	// Load config settings
	FileParser infile;
	// @CLASS MenuCharacter|Description of menus/character.txt
	if (infile.open("menus/character.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = Parse::toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR label_title|label|Position of the "Character" text.
			else if(infile.key == "label_title") labelCharacter->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			// @ATTR upgrade_primary|predefined_string, point : Primary stat name, Button position|Position of the button used to add a stat point to this primary stat.
			else if(infile.key == "upgrade_primary") {
				std::string prim_stat = Parse::popFirstString(infile.val);
				size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

				if (prim_stat_index != eset->primary_stats.list.size()) {
					Point pos = Parse::toPoint(infile.val);
					upgradeButton[prim_stat_index]->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
				}
				else {
					infile.error("MenuCharacter: '%s' is not a valid primary stat.", prim_stat.c_str());
				}
			}
			// @ATTR statlist|point|Position of the scrollbox containing non-primary stats.
			else if(infile.key == "statlist") statlist_pos = Parse::toPoint(infile.val);
			// @ATTR statlist_rows|int|The height of the statlist in rows.
			else if (infile.key == "statlist_rows") statlist_rows = Parse::toInt(infile.val);
			// @ATTR statlist_scrollbar_offset|int|Right margin in pixels for the statlist's scrollbar.
			else if (infile.key == "statlist_scrollbar_offset") statlist_scrollbar_offset = Parse::toInt(infile.val);

			// @ATTR label_name|label|Position of the "Name" text.
			else if(infile.key == "label_name") {
				cstat[CSTAT_NAME].label->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR label_level|label|Position of the "Level" text.
			else if(infile.key == "label_level") {
				cstat[CSTAT_LEVEL].label->setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			// @ATTR label_primary|predefined_string, label : Primary stat name, Text positioning|Position of the text label for this primary stat.
			else if(infile.key == "label_primary") {
				std::string prim_stat = Parse::popFirstString(infile.val);
				size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

				if (prim_stat_index != eset->primary_stats.list.size()) {
					cstat[prim_stat_index+2].label->setFromLabelInfo(Parse::popLabelInfo(infile.val));
				}
				else {
					infile.error("MenuCharacter: '%s' is not a valid primary stat.", prim_stat.c_str());
				}
			}

			// @ATTR name|rectangle|Position of the player's name and dimensions of the tooltip hotspot.
			else if(infile.key == "name") {
				cstat[CSTAT_NAME].value_pos = Parse::toRect(infile.val);
				cstat[CSTAT_NAME].value->setBasePos(cstat[CSTAT_NAME].value_pos.x, cstat[CSTAT_NAME].value_pos.y + (cstat[CSTAT_NAME].value_pos.h/2), Utils::ALIGN_TOPLEFT);
			}
			// @ATTR level|rectangle|Position of the player's level and dimensions of the tooltip hotspot.
			else if(infile.key == "level") {
				cstat[CSTAT_LEVEL].value_pos = Parse::toRect(infile.val);
				cstat[CSTAT_LEVEL].value->setBasePos(cstat[CSTAT_LEVEL].value_pos.x + (cstat[CSTAT_LEVEL].value_pos.w/2), cstat[CSTAT_LEVEL].value_pos.y + (cstat[CSTAT_LEVEL].value_pos.h/2), Utils::ALIGN_TOPLEFT);
			}
			// @ATTR primary|predefined_string, rectangle : Primary stat name, Hotspot position|Position of this primary stat value display and dimensions of its tooltip hotspot.
			else if(infile.key == "primary") {
				std::string prim_stat = Parse::popFirstString(infile.val);
				size_t prim_stat_index = eset->primary_stats.getIndexByID(prim_stat);

				if (prim_stat_index != eset->primary_stats.list.size()) {
					Rect r = Parse::toRect(infile.val);
					cstat[prim_stat_index+2].value_pos = r;
					cstat[prim_stat_index+2].value->setBasePos(r.x + (r.w/2), r.y + (r.h/2), Utils::ALIGN_TOPLEFT);
				}
				else {
					infile.error("MenuCharacter: '%s' is not a valid primary stat.", prim_stat.c_str());
				}
			}

			// @ATTR unspent|label|Position of the label showing the number of unspent stat points.
			else if(infile.key == "unspent") labelUnspent->setFromLabelInfo(Parse::popLabelInfo(infile.val));

			// @ATTR show_resists|bool|Hide the "Resist Damage" stats in the statlist if set to false.
			else if (infile.key == "show_resists") show_resists = Parse::toBool(infile.val);

			// @ATTR show_stat|stat_id, bool : Stat ID, Visible|Hide the matching stat ID in the statlist if set to false.
			else if (infile.key == "show_stat") {
				parseShowStat(infile);
			}

			// @ATTR name_max_width|int|The maxiumum width, in pixels, that the character name can occupy until it is abbreviated.
			else if (infile.key == "name_max_width") name_max_width = Parse::toInt(infile.val);

			else {
				infile.error("MenuCharacter: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// stat list
	statList = new WidgetListBox(statlist_rows, "images/menus/buttons/listbox_char.png");
	tablist.add(statList);
	statList->can_select = false;
	statList->scrollbar_offset = statlist_scrollbar_offset;
	statList->setBasePos(statlist_pos.x, statlist_pos.y, Utils::ALIGN_TOPLEFT);

	// HACK: During gameplay, the stat list can refresh rapidly when the charcter menu is open and the player has certain effects
	// frequently refreshing trimmed text is slow for Cyrillic characters, so disable it here
	statList->disable_text_trim = true;

	if (!background)
		setBackground("images/menus/character.png");

	align();

	base_stats.resize(eset->primary_stats.list.size());
	base_stats_add.resize(eset->primary_stats.list.size());
	base_bonus.resize(eset->primary_stats.list.size());

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		base_stats[i] = &pc->stats.primary[i];
		base_stats_add[i] = &pc->stats.primary_additional[i];
		base_bonus[i] = &pc->stats.per_primary[i];
	}
}

void MenuCharacter::align() {
	Menu::align();

	// close button
	closeButton->setPos(window_area.x, window_area.y);

	// menu title
	labelCharacter->setPos(window_area.x, window_area.y);

	// upgrade buttons
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		upgradeButton[i]->setPos(window_area.x, window_area.y);
	}

	// stat list
	statList->setPos(window_area.x, window_area.y);

	for (size_t i = 0; i < cstat.size(); ++i) {
		// setup static labels
		cstat[i].label->setPos(window_area.x, window_area.y);

		// setup hotspot locations
		cstat[i].setHover(window_area.x, window_area.y);

		// setup value labels
		cstat[i].value->setPos(window_area.x, window_area.y);
	}

	labelUnspent->setPos(window_area.x, window_area.y);
}

/**
 * Rebuild all stat values and tooltip info
 */
void MenuCharacter::refreshStats() {

	pc->stats.refresh_stats = false;

	std::stringstream ss;

	// update stat text
	std::string trimmed_name;
	if (name_max_width > 0)
		trimmed_name = font->trimTextToWidth(pc->stats.name, name_max_width, FontEngine::USE_ELLIPSIS, 0);
	else
		trimmed_name = pc->stats.name;

	cstat[CSTAT_NAME].value->setText(trimmed_name);

	ss.str("");
	ss << pc->stats.level;
	cstat[CSTAT_LEVEL].value->setText(ss.str());
	cstat[CSTAT_LEVEL].value->setJustify(FontEngine::JUSTIFY_CENTER);

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		ss.str("");
		ss << pc->stats.get_primary(i);
		cstat[i+2].value->setText(ss.str());
		cstat[i+2].value->setJustify(FontEngine::JUSTIFY_CENTER);
		cstat[i+2].value->setColor(bonusColor(pc->stats.primary_additional[i]));
	}

	if (skill_points >= 1) {
		labelUnspent->setText(msg->getv("Available stat points: %d", skill_points));
	}
	else {
		labelUnspent->setText("");
	}

	// scrolling stat list
	unsigned stat_index = 0;
	size_t resource_offset_index = Stats::COUNT + eset->damage_types.count;
	size_t speed_offset_index = resource_offset_index + eset->resource_stats.stat_count;

	ss.str("");
	ss << msg->get("Core Stats");
	statList->set(stat_index, ss.str(), "");
	statList->setRowHighlight(stat_index, true);
	stat_index++;

	for (int i=0; i<Stats::COUNT; ++i) {
		if (Stats::CATEGORY[i] != Stats::CATEGORY_CORE)
			continue;

		if (!show_stat[i]) continue;

		// Stats::ABS_MIN handles both min and max
		if (i == Stats::ABS_MAX) continue;

		ss.str("");
		ss << " " << Stats::NAME[i] << ": " << Utils::floatToString(pc->stats.get(static_cast<Stats::STAT>(i)), 2);
		if (Stats::PERCENT[i]) ss << "%";
		statList->set(stat_index, ss.str(), statTooltip(i));
		stat_index++;
	}

	// insert resource stats (execpt stealing)
	for (size_t j = 0; j < eset->resource_stats.list.size(); ++j) {
		for (size_t k = 0; k < EngineSettings::ResourceStats::STAT_STEAL; ++k) {
			if (show_stat[resource_offset_index + (j * EngineSettings::ResourceStats::STAT_COUNT) + k]) {
				ss.str("");
				ss << " " << eset->resource_stats.list[j].text[k] << ": " << Utils::floatToString(pc->stats.getResourceStat(j, k), eset->number_format.character_menu);
				statList->set(stat_index, ss.str(), resourceStatTooltip(j, k));
				stat_index++;
			}
		}
	}

	ss.str("");
	ss << msg->get("Offensive Stats");
	statList->set(stat_index, ss.str(), "");
	statList->setRowHighlight(stat_index, true);
	stat_index++;

	// insert damage stats
	for (size_t j = 0; j < eset->damage_types.list.size(); ++j) {
		if (show_stat[Stats::COUNT + eset->damage_types.indexToMin(j)] || show_stat[Stats::COUNT + eset->damage_types.indexToMax(j)]) {
			float min_dmg = pc->stats.getDamageMin(j);
			float max_dmg = pc->stats.getDamageMax(j);

			ss.str("");
			ss << " ";

			if (eset->damage_types.list[j].is_deprecated_element)
				ss << msg->getv("Elemental Damage (%s)", eset->damage_types.list[j].name.c_str());
			else
				ss << eset->damage_types.list[j].name;

			ss << ": " << Utils::createMinMaxString(min_dmg, max_dmg, eset->number_format.character_menu);

			statList->set(stat_index, ss.str(), damageTooltip(j));
			stat_index++;
		}
	}
	for (int i=0; i<Stats::COUNT; ++i) {
		if (Stats::CATEGORY[i] != Stats::CATEGORY_OFFENSE)
			continue;

		if (!show_stat[i]) continue;

		ss.str("");
		ss << " " << Stats::NAME[i] << ": " << Utils::floatToString(pc->stats.get(static_cast<Stats::STAT>(i)), 2);
		if (Stats::PERCENT[i]) ss << "%";
		statList->set(stat_index, ss.str(), statTooltip(i));
		stat_index++;
	}

	// insert resource stealing stats after HP/MP steal
	for (size_t j = 0; j < eset->resource_stats.list.size(); ++j) {
		if (show_stat[resource_offset_index + (j * EngineSettings::ResourceStats::STAT_COUNT) + EngineSettings::ResourceStats::STAT_STEAL]) {
			ss.str("");
			ss << " " << eset->resource_stats.list[j].text[EngineSettings::ResourceStats::STAT_STEAL] << ": " << Utils::floatToString(pc->stats.getResourceStat(j, EngineSettings::ResourceStats::STAT_STEAL), eset->number_format.character_menu) << "%";
			statList->set(stat_index, ss.str(), resourceStatTooltip(j, EngineSettings::ResourceStats::STAT_STEAL));
			stat_index++;
		}
	}

	ss.str("");
	ss << msg->get("Defensive Stats");
	statList->set(stat_index, ss.str(), "");
	statList->setRowHighlight(stat_index, true);
	stat_index++;

	ss.str("");
	ss << " " << msg->get("Absorb") << ": " << Utils::createMinMaxString(pc->stats.get(Stats::ABS_MIN), pc->stats.get(Stats::ABS_MAX), eset->number_format.character_menu);
	statList->set(stat_index, ss.str(), statTooltip(Stats::ABS_MIN));
	stat_index++;

	for (int i=0; i<Stats::COUNT; ++i) {
		if (Stats::CATEGORY[i] != Stats::CATEGORY_DEFENSE)
			continue;

		if (!show_stat[i]) continue;

		// absorb was already added to the list
		if (i == Stats::ABS_MIN || i == Stats::ABS_MAX) continue;

		ss.str("");
		ss << " " << Stats::NAME[i] << ": " << Utils::floatToString(pc->stats.get(static_cast<Stats::STAT>(i)), 2);
		if (Stats::PERCENT[i]) ss << "%";
		statList->set(stat_index, ss.str(), statTooltip(i));
		stat_index++;
	}

	if (show_resists) {
		for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
			if (show_stat[Stats::COUNT + eset->damage_types.indexToResist(i)]) {
				ss.str("");
				ss << " " << eset->damage_types.list[i].name_resist << ": " << Utils::floatToString(pc->stats.getDamageResist(i), eset->number_format.character_menu) << "%";
				statList->set(stat_index, ss.str(), resistTooltip(i));
				stat_index++;
			}
		}
	}

	// insert resource stealing stats after HP/MP steal
	for (size_t j = 0; j < eset->resource_stats.list.size(); ++j) {
		if (show_stat[resource_offset_index + (j * EngineSettings::ResourceStats::STAT_COUNT) + EngineSettings::ResourceStats::STAT_RESIST_STEAL]) {
			ss.str("");
			ss << " " << eset->resource_stats.list[j].text[EngineSettings::ResourceStats::STAT_RESIST_STEAL] << ": " << Utils::floatToString(pc->stats.getResourceStat(j, EngineSettings::ResourceStats::STAT_RESIST_STEAL), eset->number_format.character_menu) << "%";
			statList->set(stat_index, ss.str(), resourceStatTooltip(j, EngineSettings::ResourceStats::STAT_RESIST_STEAL));
			stat_index++;
		}
	}


	ss.str("");
	ss << msg->get("Miscellaneous Stats");
	statList->set(stat_index, ss.str(), "");
	statList->setRowHighlight(stat_index, true);
	stat_index++;

	for (int i=0; i<Stats::COUNT; ++i) {
		if (Stats::CATEGORY[i] != Stats::CATEGORY_MISC)
			continue;

		if (!show_stat[i]) continue;

		ss.str("");
		ss << " " << Stats::NAME[i] << ": " << Utils::floatToString(pc->stats.get(static_cast<Stats::STAT>(i)), 2);
		if (Stats::PERCENT[i]) ss << "%";
		statList->set(stat_index, ss.str(), statTooltip(i));
		stat_index++;
	}

    if (show_stat[speed_offset_index]) {
		ss.str("");
		ss << " " << msg->get("Movement Speed") << ": " << pc->stats.effects.speed << "%";
		statList->set(stat_index, ss.str(), "");
		stat_index++;
	}

	if (show_stat[speed_offset_index + 1]) {
		ss.str("");
		ss << " " << msg->get("Attack Speed") << ": " << pc->stats.effects.getAttackSpeed("") << "%";
		statList->set(stat_index, ss.str(), "");
		stat_index++;
	}

	// update tool tips
	cstat[CSTAT_NAME].tip.clear();
	cstat[CSTAT_NAME].tip.addText(pc->stats.name);
	cstat[CSTAT_NAME].tip.addText(pc->stats.getLongClass());

	cstat[CSTAT_LEVEL].tip.clear();
	cstat[CSTAT_LEVEL].tip.addText(msg->getv("XP: %lu", pc->stats.xp));
	if (pc->stats.level < eset->xp.getMaxLevel()) {
		cstat[CSTAT_LEVEL].tip.addText(msg->getv("Next: %lu", eset->xp.getLevelXP(pc->stats.level + 1)));
	}

	for (size_t j = 2; j < cstat.size(); ++j) {
		cstat[j].tip.clear();
		ss.str("");
		ss << cstat[j].label->getText() << " (" << *(base_stats[j-2]);
		if (*(base_stats_add[j-2]) > 0) {
			ss << "+";
		}
		if (*(base_stats_add[j-2]) != 0) {
			ss << *(base_stats_add[j-2]);
		}
		ss << ")";
		cstat[j].tip.addText(ss.str());

		bool have_bonus = false;
		size_t resource_stat_offset = Stats::COUNT + eset->damage_types.count;

		for (int i = 0; i < Stats::COUNT; ++i) {
			// insert resource stats (execpt stealing) before accuracy
			if (i == Stats::ACCURACY) {
				for (size_t k = 0; k < eset->resource_stats.list.size(); ++k) {
					for (size_t l = 0; l < EngineSettings::ResourceStats::STAT_STEAL; ++l) {
						size_t resource_index = resource_stat_offset + (k * EngineSettings::ResourceStats::STAT_COUNT) + l;

						if (base_bonus[j-2]->at(resource_index) > 0 && show_stat[resource_index]) {
							if (!have_bonus) {
								cstat[j].tip.addText("\n" + msg->get("Related stats:"));
								have_bonus = true;
							}
							cstat[j].tip.addText(eset->resource_stats.list[k].text[l]);
						}
					}
				}
			}

			// damage types are displayed before absorb
			if (i == Stats::ABS_MIN) {
				for (size_t k = 0; k < eset->damage_types.list.size(); ++k) {
					// damage min
					if (base_bonus[j-2]->at(Stats::COUNT + eset->damage_types.indexToMin(k)) > 0 && show_stat[Stats::COUNT + eset->damage_types.indexToMin(k)]) {
						if (!have_bonus) {
							cstat[j].tip.addText("\n" + msg->get("Related stats:"));
							have_bonus = true;
						}
						cstat[j].tip.addText(eset->damage_types.list[k].name_min);
					}
					// damage max
					if (base_bonus[j-2]->at(Stats::COUNT + eset->damage_types.indexToMax(k)) > 0 && show_stat[Stats::COUNT + eset->damage_types.indexToMax(k)]) {
						if (!have_bonus) {
							cstat[j].tip.addText("\n" + msg->get("Related stats:"));
							have_bonus = true;
						}
						cstat[j].tip.addText(eset->damage_types.list[k].name_max);
					}
				}
			}

			// non-damage bonuses
			if (base_bonus[j-2]->at(i) > 0 && show_stat[i]) {
				if (!have_bonus) {
					cstat[j].tip.addText("\n" + msg->get("Related stats:"));
					have_bonus = true;
				}
				cstat[j].tip.addText(Stats::NAME[i]);
			}
		}

		// insert resource stealing stats after MP steal
		for (size_t i = 0; i < eset->resource_stats.list.size(); ++i) {
			for (size_t k = EngineSettings::ResourceStats::STAT_STEAL; k < EngineSettings::ResourceStats::STAT_COUNT; ++k) {
				size_t resource_index = resource_stat_offset + (i * EngineSettings::ResourceStats::STAT_COUNT) + k;

				if (base_bonus[j-2]->at(resource_index) > 0 && show_stat[resource_index]) {
					if (!have_bonus) {
						cstat[j].tip.addText("\n" + msg->get("Related stats:"));
						have_bonus = true;
					}
					cstat[j].tip.addText(eset->resource_stats.list[i].text[k]);
				}
			}
		}

		// resistances
		for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
			if (base_bonus[j-2]->at(Stats::COUNT + eset->damage_types.indexToResist(i)) > 0 && show_stat[Stats::COUNT + eset->damage_types.indexToResist(i)]) {
				if (!have_bonus) {
					cstat[j].tip.addText("\n" + msg->get("Related stats:"));
					have_bonus = true;
				}
				cstat[j].tip.addText(eset->damage_types.list[i].name_resist);
			}
		}
	}
}


/**
 * Color-coding for positive/negative/no bonus
 */
Color MenuCharacter::bonusColor(int stat) {
	if (stat > 0) return font->getColor(FontEngine::COLOR_MENU_BONUS);
	if (stat < 0) return font->getColor(FontEngine::COLOR_MENU_PENALTY);
	return font->getColor(FontEngine::COLOR_MENU_NORMAL);
}

void MenuCharacter::tooltipCreateBonusText(size_t min_index, size_t max_index, std::string* min_text, std::string* max_text) {
	// per-level bonus
	float min_per_level = pc->stats.per_level[min_index];
	float max_per_level = pc->stats.per_level[max_index];

	if (min_per_level > 0) {
		*min_text += msg->getv("Each level grants %s.", Utils::floatToString(min_per_level, eset->number_format.character_menu).c_str());
	}

	if (max_text && max_per_level > 0) {
		*max_text += msg->getv("Each level grants %s.", Utils::floatToString(max_per_level, eset->number_format.character_menu).c_str());
	}

	// per-primary bonuses
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		float min_per_primary = pc->stats.per_primary[i][min_index];
		float max_per_primary = pc->stats.per_primary[i][max_index];

		if (min_per_primary > 0) {
			if (!min_text->empty())
				*min_text += "\n";

			*min_text += msg->getv("Each point of %s grants %s.", eset->primary_stats.list[i].name.c_str(), Utils::floatToString(min_per_primary, eset->number_format.character_menu).c_str());
		}

		if (max_text && max_per_primary > 0) {
			if (!max_text->empty())
				*max_text += "\n";

			*max_text += msg->getv("Each point of %s grants %s.", eset->primary_stats.list[i].name.c_str(), Utils::floatToString(max_per_primary, eset->number_format.character_menu).c_str());
		}
	}
}

/**
 * Create tooltip text showing the per_* values of a stat
 */
std::string MenuCharacter::statTooltip(int stat) {
	std::string text, min_text;

	std::string& description = Stats::DESC[stat];
	if (!description.empty())
		text += description;

	size_t min_index = stat;

	if (stat == Stats::ABS_MIN) {
		std::string max_text;

		size_t max_index = Stats::ABS_MAX;

		tooltipCreateBonusText(min_index, max_index, &min_text, &max_text);

		if (!min_text.empty()) {
			if (!text.empty())
				text += "\n\n";
			text += Stats::NAME[min_index] + ":\n" + min_text;
		}

		if (!max_text.empty()) {
			if (!text.empty())
				text += "\n\n";
			text += Stats::NAME[max_index] + ":\n" + max_text;
		}
	}
	else {
		tooltipCreateBonusText(min_index, min_index, &min_text, NULL);

		if (!min_text.empty()) {
			if (!text.empty())
				text += "\n\n";
			text += min_text;
		}
	}

	return text;
}

/**
 * Create tooltip text showing the per_* values of a damage stat
 */
std::string MenuCharacter::damageTooltip(size_t dmg_type) {
	std::string text, min_text, max_text;

	std::string& description = eset->damage_types.list[dmg_type].description;
	if (!description.empty())
		text += description;

	size_t min_index = Stats::COUNT + eset->damage_types.indexToMin(dmg_type);
	size_t max_index = Stats::COUNT + eset->damage_types.indexToMax(dmg_type);

	tooltipCreateBonusText(min_index, max_index, &min_text, &max_text);

	if (!min_text.empty()) {
		if (!text.empty())
			text += "\n\n";
		text += eset->damage_types.list[dmg_type].name_min + ":\n" + min_text;
	}

	if (!max_text.empty()) {
		if (!text.empty())
			text += "\n\n";
		text += eset->damage_types.list[dmg_type].name_max + ":\n" + max_text;
	}

	return text;
}

/**
 * Create tooltip text showing the per_* values of a resistance stat
 */
std::string MenuCharacter::resistTooltip(size_t resist_type) {
	std::string text, bonus_text;

	text += msg->get("Reduces damage taken from attacks of this damage type.");

	size_t resist_index = Stats::COUNT + eset->damage_types.indexToResist(resist_type);

	tooltipCreateBonusText(resist_index, resist_index, &bonus_text, NULL);

	if (!bonus_text.empty()) {
		if (!text.empty())
			text += "\n\n";
		text += bonus_text;
	}

	return text;
}

std::string MenuCharacter::resourceStatTooltip(size_t resource_index, size_t stat_index) {
	std::string text, bonus_text;

	std::string& description = eset->resource_stats.list[resource_index].text_desc[stat_index];
	if (!description.empty())
		text += description;

	size_t offset_index = Stats::COUNT + eset->damage_types.count;
	size_t resource_stat_index = offset_index + (resource_index * EngineSettings::ResourceStats::STAT_COUNT) + stat_index;

	tooltipCreateBonusText(resource_stat_index, resource_stat_index, &bonus_text, NULL);

	if (!bonus_text.empty()) {
		if (!text.empty())
			text += "\n\n";
		text += bonus_text;
	}

	return text;
}

void MenuCharacter::logic() {
	if (!visible) return;

	tablist.logic();

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
	}

	bool have_skill_points = checkSkillPoints();

	if (pc->stats.hp > 0 && have_skill_points) {
		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			if (pc->stats.primary[i] < pc->stats.max_points_per_stat) {
				upgradeButton[i]->enabled = true;
				tablist.add(upgradeButton[i]);
			}
			else {
				upgradeButton[i]->enabled = false;
				tablist.remove(upgradeButton[i]);
			}
		}

		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			if (upgradeButton[i]->checkClick())
				primary_up[i] = true;
		}
	}
	else {
		// no skill points to allocate; remove upgrade buttons
		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			upgradeButton[i]->enabled = false;
			tablist.remove(upgradeButton[i]);
		}

		if (tablist.getCurrent() >= static_cast<int>(tablist.size())) {
			tablist.defocus();
			tablist.getNext(TabList::GET_INNER, TabList::WIDGET_SELECT_AUTO);
		}
	}

	statList->checkClick();

	if (pc->stats.refresh_stats) refreshStats();
}



void MenuCharacter::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// title
	labelCharacter->render();

	// unspent points
	labelUnspent->render();

	// labels and values
	for (size_t i = 0; i < cstat.size(); ++i) {
		if (!cstat[i].label->isHidden()) {
			cstat[i].label->render();
			cstat[i].value->render();
		}
	}

	// upgrade buttons
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (upgradeButton[i]->enabled) upgradeButton[i]->render();
	}

	statList->render();
}


/**
 * Display various mouseovers tooltips depending on cursor location
 */
void MenuCharacter::renderTooltips(const Point& position) {
	if (!visible || !Utils::isWithinRect(window_area, position))
		return;

	for (size_t i = 0; i < cstat.size(); ++i) {
		if (Utils::isWithinRect(cstat[i].hover, position) && !cstat[i].tip.isEmpty() && !cstat[i].label->isHidden()) {
			tooltipm->push(cstat[i].tip, position, TooltipData::STYLE_FLOAT);
			break;
		}
	}
}

/**
 * User might click this menu to upgrade a stat.  Check for this situation.
 * Return true if a stat was upgraded.
 */
bool MenuCharacter::checkUpgrade() {
	// check to see if there are skill points available
	if (pc->stats.hp > 0 && checkSkillPoints()) {
		for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
			if (primary_up[i]) {
				pc->stats.primary[i]++;
				pc->stats.recalc(); // equipment applied by MenuManager
				primary_up[i] = false;
				return true;
			}
		}
	}

	return false;
}

bool MenuCharacter::checkSkillPoints() {
	int spent = 0;
	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		spent += pc->stats.primary[i] - pc->stats.primary_starting[i];
	}

	skill_points = ((pc->stats.level - 1) * pc->stats.stat_points_per_level) - spent;

	return (spent < ((pc->stats.level - 1) * pc->stats.stat_points_per_level) && spent < pc->stats.max_spendable_stat_points);
}

void MenuCharacter::parseShowStat(FileParser& infile) {
	std::string stat_name = Parse::popFirstString(infile.val);
	bool value = Parse::toBool(Parse::popFirstString(infile.val));
	size_t offset_index = 0;

	for (int i=0; i<Stats::COUNT; ++i) {
		if (stat_name == Stats::KEY[i]) {
			show_stat[i] = value;
			return;
		}
	}
	offset_index += Stats::COUNT;

	for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
		if (stat_name == eset->damage_types.list[i].min) {
			show_stat[offset_index + eset->damage_types.indexToMin(i)] = value;
			return;
		}
		else if (stat_name == eset->damage_types.list[i].max) {
			show_stat[offset_index + eset->damage_types.indexToMax(i)] = value;
			return;
		}
		else if (stat_name == eset->damage_types.list[i].resist) {
			show_stat[offset_index + eset->damage_types.indexToResist(i)] = value;
			return;
		}
	}
	offset_index += eset->damage_types.count;

	for (size_t i = 0; i < eset->resource_stats.list.size(); ++i) {
		for (size_t j = 0; j < EngineSettings::ResourceStats::STAT_COUNT; ++j) {
			if (stat_name == eset->resource_stats.list[i].ids[j]) {
				show_stat[offset_index + (i * EngineSettings::ResourceStats::STAT_COUNT) + j] = value;
				return;
			}
		}
	}
	offset_index += eset->resource_stats.stat_count;

	if (stat_name == "speed") {
		show_stat[offset_index] = value;
	}
	else if (stat_name == "attack_speed") {
		show_stat[offset_index + 1] = value;
	}
}

MenuCharacter::~MenuCharacter() {
	delete closeButton;
	delete labelCharacter;
	delete labelUnspent;
	for (size_t i = 0; i < cstat.size(); ++i) {
		delete cstat[i].label;
		delete cstat[i].value;
	}
	for (size_t i = 0; i < upgradeButton.size(); ++i) {
		delete upgradeButton[i];
	}
	delete statList;
}
