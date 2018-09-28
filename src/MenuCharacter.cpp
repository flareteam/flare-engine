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

	show_stat.resize(Stats::COUNT + eset->damage_types.count);
	for (size_t i = 0; i < Stats::COUNT + eset->damage_types.count; i++) {
		show_stat[i] = true;
	}

	// these two are hidden by default, as they are currently unused
	show_stat[Stats::HP_PERCENT] = false;
	show_stat[Stats::MP_PERCENT] = false;

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

			// @ATTR show_resists|bool|Hide the elemental "Resistance" stats in the statlist if set to false.
			else if (infile.key == "show_resists") show_resists = Parse::toBool(infile.val);

			// @ATTR show_stat|predefined_string, bool : Stat name / Damage type, Visible|Hide the matching stat or damage type min/max in the statlist if set to false.
			else if (infile.key == "show_stat") {
				std::string stat_name = Parse::popFirstString(infile.val);

				for (int i=0; i<Stats::COUNT; ++i) {
					if (stat_name == Stats::KEY[i]) {
						show_stat[i] = Parse::toBool(Parse::popFirstString(infile.val));
						break;
					}
				}
				for (size_t i = 0; i < eset->damage_types.list.size(); ++i) {
					if (stat_name == eset->damage_types.list[i].min) {
						show_stat[Stats::COUNT + (i*2)] = Parse::toBool(Parse::popFirstString(infile.val));
					}
					else if (stat_name == eset->damage_types.list[i].max) {
						show_stat[Stats::COUNT + (i*2) + 1] = Parse::toBool(Parse::popFirstString(infile.val));
					}
				}
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

	ss.str("");
	if (skill_points == 1) {
		ss << msg->get("%d unspent stat point", skill_points);
	}
	else if (skill_points > 1) {
		ss << msg->get("%d unspent stat points", skill_points);
	}
	labelUnspent->setText(ss.str());

	// scrolling stat list
	unsigned stat_index = 0;
	for (int i=0; i<Stats::COUNT; ++i) {
		if (!show_stat[i]) continue;

		// insert damage stats before absorb min
		if (i == Stats::ABS_MIN) {
			for (size_t j = 0; j < eset->damage_types.list.size(); ++j) {
				if (show_stat[Stats::COUNT + (j*2)]) {
					// min
					ss.str("");
					ss << eset->damage_types.list[j].name_min << ": " << pc->stats.getDamageMin(j);
					statList->set(stat_index, ss.str(), damageTooltip(j*2));
					stat_index++;
				}

				if (show_stat[Stats::COUNT + (j*2) + 1]) {
					// max
					ss.str("");
					ss << eset->damage_types.list[j].name_max << ": " << pc->stats.getDamageMax(j);
					statList->set(stat_index, ss.str(), damageTooltip((j*2) + 1));
					stat_index++;
				}
			}
		}

		ss.str("");
		ss << Stats::NAME[i] << ": " << pc->stats.get(static_cast<Stats::STAT>(i));
		if (Stats::PERCENT[i]) ss << "%";

		statList->set(stat_index, ss.str(), statTooltip(i));
		stat_index++;
	}

	if (show_resists) {
		for (unsigned int j=0; j<pc->stats.vulnerable.size(); ++j) {
			ss.str("");
			ss << msg->get("Resistance (%s)", eset->elements.list[j].name) << ": " << (100 - pc->stats.vulnerable[j]) << "%";
			statList->set(j+stat_index, ss.str(), msg->get("Reduces the damage taken from \"%s\" elemental attacks.", eset->elements.list[j].name));
		}
	}

	// update tool tips
	cstat[CSTAT_NAME].tip.clear();
	cstat[CSTAT_NAME].tip.addText(pc->stats.name);
	cstat[CSTAT_NAME].tip.addText(pc->stats.getLongClass());

	cstat[CSTAT_LEVEL].tip.clear();
	cstat[CSTAT_LEVEL].tip.addText(msg->get("XP: %d", pc->stats.xp));
	if (pc->stats.level < eset->xp.getMaxLevel()) {
		cstat[CSTAT_LEVEL].tip.addText(msg->get("Next: %d", eset->xp.getLevelXP(pc->stats.level + 1)));
	}

	for (size_t j = 2; j < cstat.size(); ++j) {
		cstat[j].tip.clear();
		cstat[j].tip.addText(cstat[j].label->getText());
		cstat[j].tip.addText(msg->get("base (%d), bonus (%d)", *(base_stats[j-2]), *(base_stats_add[j-2])));
		bool have_bonus = false;
		for (int i = 0; i < Stats::COUNT; ++i) {
			// damage types are displayed before absorb
			if (i == Stats::ABS_MIN) {
				for (size_t k = 0; k < eset->damage_types.count; ++k) {
					if (base_bonus[j-2]->at(Stats::COUNT + k) > 0) {
						if (!have_bonus) {
							cstat[j].tip.addText("\n" + msg->get("Related stats:"));
							have_bonus = true;
						}
						if (k % 2 == 0)
							cstat[j].tip.addText(eset->damage_types.list[k / 2].name_min);
						else
							cstat[j].tip.addText(eset->damage_types.list[k / 2].name_max);
					}
				}
			}

			// non-damage bonuses
			if (base_bonus[j-2]->at(i) > 0) {
				if (!have_bonus) {
					cstat[j].tip.addText("\n" + msg->get("Related stats:"));
					have_bonus = true;
				}
				cstat[j].tip.addText(Stats::NAME[i]);
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

/**
 * Create tooltip text showing the per_* values of a stat
 */
std::string MenuCharacter::statTooltip(int stat) {
	std::string tooltip_text;

	if (pc->stats.per_level[stat] > 0)
		tooltip_text += msg->get("Each level grants %d.", pc->stats.per_level[stat]) + ' ';

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (pc->stats.per_primary[i][stat] > 0)
			tooltip_text += msg->get("Each point of %s grants %d.", eset->primary_stats.list[i].name, pc->stats.per_primary[i][stat]) + ' ';
	}

	std::string full_tooltip = "";
	if (Stats::DESC[stat] != "")
		full_tooltip += Stats::DESC[stat];
	if (full_tooltip != "" && tooltip_text != "")
		full_tooltip += "\n";
	full_tooltip += tooltip_text;

	return full_tooltip;
}

/**
 * Create tooltip text showing the per_* values of a damage stat
 */
std::string MenuCharacter::damageTooltip(size_t dmg_type) {
	std::string tooltip_text;

	if (pc->stats.per_level[Stats::COUNT + dmg_type] > 0)
		tooltip_text += msg->get("Each level grants %d.", pc->stats.per_level[Stats::COUNT + dmg_type]) + ' ';

	for (size_t i = 0; i < eset->primary_stats.list.size(); ++i) {
		if (pc->stats.per_primary[i][Stats::COUNT + dmg_type] > 0)
			tooltip_text += msg->get("Each point of %s grants %d.", eset->primary_stats.list[i].name, pc->stats.per_primary[i][Stats::COUNT + dmg_type]) + ' ';
	}

	size_t real_dmg_type = dmg_type / 2;

	std::string full_tooltip = "";
	if (eset->damage_types.list[real_dmg_type].description != "")
		full_tooltip += eset->damage_types.list[real_dmg_type].description;
	if (full_tooltip != "" && tooltip_text != "")
		full_tooltip += "\n";
	full_tooltip += tooltip_text;

	return full_tooltip;
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
