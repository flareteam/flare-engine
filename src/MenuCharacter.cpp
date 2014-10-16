/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2014 Henrik Andersson
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
 * class MenuCharacter
 */

#include <string>

#include "FileParser.h"
#include "Menu.h"
#include "MenuCharacter.h"
#include "SharedResources.h"
#include "Settings.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetListBox.h"


using namespace std;

MenuCharacter::MenuCharacter(StatBlock *_stats) {
	stats = _stats;

	// Labels for major stats
	cstat_labels[CSTAT_NAME] = "Name";
	cstat_labels[CSTAT_LEVEL] = "Level";
	cstat_labels[CSTAT_PHYSICAL] = "Physical";
	cstat_labels[CSTAT_MENTAL] = "Mental";
	cstat_labels[CSTAT_OFFENSE] = "Offense";
	cstat_labels[CSTAT_DEFENSE] = "Defense";

	skill_points = 0;

	visible = false;

	for (int i=0; i<CSTAT_COUNT; i++) {
		cstat[i].label = new WidgetLabel();
		cstat[i].value = new WidgetLabel();
		cstat[i].hover.x = cstat[i].hover.y = 0;
		cstat[i].hover.w = cstat[i].hover.h = 0;
		cstat[i].visible = true;
	}
	for (int i=0; i<STATLIST_COUNT; i++) {
		show_stat[i] = true;
	}
	statlist_rows = 10;
	statlist_scrollbar_offset = 0;

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Upgrade buttons
	for (int i=0; i<4; i++) {
		upgradeButton[i] = new WidgetButton("images/menus/buttons/upgrade.png");
		upgradeButton[i]->enabled = false;
		show_upgrade[i] = true;
	}
	physical_up = false;
	mental_up = false;
	offense_up = false;
	defense_up = false;

	// menu title
	labelCharacter = new WidgetLabel();

	// unspent points
	labelUnspent = new WidgetLabel();

	// Load config settings
	FileParser infile;
	// @CLASS MenuCharacter|Description of menus/character.txt
	if (infile.open("menus/character.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|x (integer), y (integer)|Position of the close button.
			if(infile.key == "close") close_pos = toPoint(infile.val);
			// @ATTR label_title|label|Position of the "Character" text.
			else if(infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR upgrade_physical|x (integer), y (integer)|Position of the button used to add a stat point to Physical.
			else if(infile.key == "upgrade_physical") upgrade_pos[0] = toPoint(infile.val);
			// @ATTR upgrade_mental|x (integer), y (integer)|Position of the button used to add a stat point to Mental.
			else if(infile.key == "upgrade_mental")	upgrade_pos[1] = toPoint(infile.val);
			// @ATTR upgrade_offense|x (integer), y (integer)|Position of the button used to add a stat point to Offense.
			else if(infile.key == "upgrade_offense") upgrade_pos[2] = toPoint(infile.val);
			// @ATTR upgrade_defense|x (integer), y (integer)|Position of the button used to add a stat point to Defense.
			else if(infile.key == "upgrade_defense") upgrade_pos[3] = toPoint(infile.val);
			// @ATTR statlist|x (integer), y (integer)|Position of the scrollbox containing non-primary stats.
			else if(infile.key == "statlist") statlist_pos = toPoint(infile.val);
			// @ATTR statlist_rows|integer|The height of the statlist in rows.
			else if (infile.key == "statlist_rows") statlist_rows = toInt(infile.val);
			// @ATTR statlist_scrollbar_offset|integer|Right margin in pixels for the statlist's scrollbar.
			else if (infile.key == "statlist_scrollbar_offset") statlist_scrollbar_offset = toInt(infile.val);

			// @ATTR label_name|label|Position of the "Name" text.
			else if(infile.key == "label_name") {
				label_pos[0] = eatLabelInfo(infile.val);
				cstat[CSTAT_NAME].visible = !label_pos[0].hidden;
			}
			// @ATTR label_level|label|Position of the "Level" text.
			else if(infile.key == "label_level") {
				label_pos[1] = eatLabelInfo(infile.val);
				cstat[CSTAT_LEVEL].visible = !label_pos[1].hidden;
			}
			// @ATTR label_physical|label|Position of the "Physical" text.
			else if(infile.key == "label_physical") {
				label_pos[2] = eatLabelInfo(infile.val);
				cstat[CSTAT_PHYSICAL].visible = !label_pos[2].hidden;
			}
			// @ATTR label_mental|label|Position of the "Mental" text.
			else if(infile.key == "label_mental") {
				label_pos[3] = eatLabelInfo(infile.val);
				cstat[CSTAT_MENTAL].visible = !label_pos[3].hidden;
			}
			// @ATTR label_offense|label|Position of the "Offense" text.
			else if(infile.key == "label_offense") {
				label_pos[4] = eatLabelInfo(infile.val);
				cstat[CSTAT_OFFENSE].visible = !label_pos[4].hidden;
			}
			// @ATTR label_defense|label|Position of the "Defense" text.
			else if(infile.key == "label_defense") {
				label_pos[5] = eatLabelInfo(infile.val);
				cstat[CSTAT_DEFENSE].visible = !label_pos[5].hidden;
			}

			// @ATTR name|x (integer), y (integer), w (integer), h (integer)|Position of the player's name and dimensions of the tooltip hotspot.
			else if(infile.key == "name") value_pos[0] = toRect(infile.val);
			// @ATTR level|x (integer), y (integer), w (integer), h (integer)|Position of the player's level and dimensions of the tooltip hotspot.
			else if(infile.key == "level") value_pos[1] = toRect(infile.val);
			// @ATTR physical|x (integer), y (integer), w (integer), h (integer)|Position of the player's physical stat and dimensions of the tooltip hotspot.
			else if(infile.key == "physical") value_pos[2] = toRect(infile.val);
			// @ATTR mental|x (integer), y (integer), w (integer), h (integer)|Position of the player's mental stat and dimensions of the tooltip hotspot.
			else if(infile.key == "mental") value_pos[3] = toRect(infile.val);
			// @ATTR offense|x (integer), y (integer), w (integer), h (integer)|Position of the player's offense stat and dimensions of the tooltip hotspot.
			else if(infile.key == "offense") value_pos[4] = toRect(infile.val);
			// @ATTR defense|x (integer), y (integer), w (integer), h (integer)|Position of the player's defense stat and dimensions of the tooltip hotspot.
			else if(infile.key == "defense") value_pos[5] = toRect(infile.val);

			// @ATTR upspent|label|Position of the label showing the number of unspent stat points.
			else if(infile.key == "unspent") unspent_pos = eatLabelInfo(infile.val);

			// @ATTR show_upgrade_physical|boolean|Hide the Physical upgrade button if set to false.
			else if (infile.key == "show_upgrade_physical") show_upgrade[0] = toBool(infile.val);
			// @ATTR show_upgrade_mental|boolean|Hide the Mental upgrade button if set to false.
			else if (infile.key == "show_upgrade_mental") show_upgrade[1] = toBool(infile.val);
			// @ATTR show_upgrade_offense|boolean|Hide the Offense upgrade button if set to false.
			else if (infile.key == "show_upgrade_offense") show_upgrade[2] = toBool(infile.val);
			// @ATTR show_upgrade_defense|boolean|Hide the Defense upgrade button if set to false.
			else if (infile.key == "show_upgrade_defense") show_upgrade[3] = toBool(infile.val);

			// @ATTR show_maxhp|boolean|Hide the "Max HP" stat in the statlist if set to false.
			else if (infile.key == "show_maxhp") show_stat[0] = toBool(infile.val);
			// @ATTR show_hpregen|boolean|Hide the "HP Regen" stat in the statlist if set to false.
			else if (infile.key == "show_hpregen") show_stat[1] = toBool(infile.val);
			// @ATTR show_maxmp|boolean|Hide the "Max MP" stat in the statlist if set to false.
			else if (infile.key == "show_maxmp") show_stat[2] = toBool(infile.val);
			// @ATTR show_mpregen|boolean|Hide the "MP Regen" stat in the statlist if set to false.
			else if (infile.key == "show_mpregen") show_stat[3] = toBool(infile.val);
			// @ATTR show_accuracy|boolean|Hide the "Accuracy" stat in the statlist if set to false.
			else if (infile.key == "show_accuracy") show_stat[4] = toBool(infile.val);
			// @ATTR show_avoidance|boolean|Hide the "Avoidance" stat in the statlist if set to false.
			else if (infile.key == "show_avoidance") show_stat[5] = toBool(infile.val);
			// @ATTR show_melee|boolean|Hide the "Melee Damage" stat in the statlist if set to false.
			else if (infile.key == "show_melee") show_stat[6] = toBool(infile.val);
			// @ATTR show_ranged|boolean|Hide the "Ranged Damage" stat in the statlist if set to false.
			else if (infile.key == "show_ranged") show_stat[7] = toBool(infile.val);
			// @ATTR show_mental|boolean|Hide the "Mental Damage" stat in the statlist if set to false.
			else if (infile.key == "show_mental") show_stat[8] = toBool(infile.val);
			// @ATTR show_crit|boolean|Hide the "Crit" stat in the statlist if set to false.
			else if (infile.key == "show_crit") show_stat[9] = toBool(infile.val);
			// @ATTR show_absorb|boolean|Hide the "Absorb" stat in the statlist if set to false.
			else if (infile.key == "show_absorb") show_stat[10] = toBool(infile.val);
			// @ATTR show_poise|boolean|Hide the "Poise" stat in the statlist if set to false.
			else if (infile.key == "show_poise") show_stat[11] = toBool(infile.val);
			// @ATTR show_bonus_xp|boolean|Hide the "Bonus XP" stat in the statlist if set to false.
			else if (infile.key == "show_bonus_xp") show_stat[12] = toBool(infile.val);
			// @ATTR show_bonus_currency|boolean|Hide the "Bonus Gold" stat in the statlist if set to false.
			else if (infile.key == "show_bonus_currency") show_stat[13] = toBool(infile.val);
			// @ATTR show_bonus_itemfind|boolean|Hide the "Bonus Item Find" stat in the statlist if set to false.
			else if (infile.key == "show_bonus_itemfind") show_stat[14] = toBool(infile.val);
			// @ATTR show_bonus_stealth|boolean|Hide the "Stealth" stat in the statlist if set to false.
			else if (infile.key == "show_bonus_stealth") show_stat[15] = toBool(infile.val);
			// @ATTR show_resists|boolean|Hide the elemental "Resistance" stats in the statlist if set to false.
			else if (infile.key == "show_resists") show_stat[16] = toBool(infile.val);

			else infile.error("MenuCharacter: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// stat list
	statList = new WidgetListBox(statlist_rows, "images/menus/buttons/listbox_char.png");
	tablist.add(statList);
	statList->can_select = false;
	statList->scrollbar_offset = statlist_scrollbar_offset;

	setBackground("images/menus/character.png");

	align();
	alignElements();
}

void MenuCharacter::alignElements() {

	// close button
	closeButton->pos.x = window_area.x + close_pos.x;
	closeButton->pos.y = window_area.y + close_pos.y;

	// menu title
	labelCharacter->set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Character"), font->getColor("menu_normal"), title.font_style);

	// upgrade buttons
	for (int i=0; i<4; i++) {
		upgradeButton[i]->pos.x = window_area.x+upgrade_pos[i].x;
		upgradeButton[i]->pos.y = window_area.y+upgrade_pos[i].y;
	}

	// stat list
	statList->pos.x = window_area.x+statlist_pos.x;
	statList->pos.y = window_area.y+statlist_pos.y;

	for (int i=0; i<CSTAT_COUNT; i++) {
		// setup static labels
		cstat[i].label->set(window_area.x+label_pos[i].x, window_area.y+label_pos[i].y, label_pos[i].justify, label_pos[i].valign, msg->get(cstat_labels[i]), font->getColor("menu_normal"), label_pos[i].font_style);

		// setup hotspot locations
		cstat[i].setHover(window_area.x+value_pos[i].x, window_area.y+value_pos[i].y, value_pos[i].w, value_pos[i].h);
	}
}

/**
 * Rebuild all stat values and tooltip info
 */
void MenuCharacter::refreshStats() {

	stats->refresh_stats = false;

	stringstream ss;

	// update stat text
	cstat[CSTAT_NAME].value->set(window_area.x+value_pos[0].x+4, window_area.y+value_pos[0].y+value_pos[0].h/2, JUSTIFY_LEFT, VALIGN_CENTER, stats->name, font->getColor("menu_normal"));

	ss.str("");
	ss << stats->level;
	cstat[CSTAT_LEVEL].value->set(window_area.x+value_pos[1].x+value_pos[1].w/2, window_area.y+value_pos[1].y+value_pos[1].h/2, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), font->getColor("menu_normal"));

	ss.str("");
	ss << stats->get_physical();
	cstat[CSTAT_PHYSICAL].value->set(window_area.x+value_pos[2].x+value_pos[2].w/2, window_area.y+value_pos[2].y+value_pos[2].h/2, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->physical_additional));

	ss.str("");
	ss << stats->get_mental();
	cstat[CSTAT_MENTAL].value->set(window_area.x+value_pos[3].x+value_pos[3].w/2, window_area.y+value_pos[3].y+value_pos[3].h/2, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->mental_additional));

	ss.str("");
	ss << stats->get_offense();
	cstat[CSTAT_OFFENSE].value->set(window_area.x+value_pos[4].x+value_pos[4].w/2, window_area.y+value_pos[4].y+value_pos[4].h/2, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->offense_additional));

	ss.str("");
	ss << stats->get_defense();
	cstat[CSTAT_DEFENSE].value->set(window_area.x+value_pos[5].x+value_pos[5].w/2, window_area.y+value_pos[5].y+value_pos[5].h/2, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->defense_additional));

	ss.str("");
	if (skill_points > 0) ss << skill_points << " " << msg->get("points remaining");
	else ss.str("");
	labelUnspent->set(window_area.x+unspent_pos.x, window_area.y+unspent_pos.y, unspent_pos.justify, unspent_pos.valign, ss.str(), font->getColor("menu_bonus"), unspent_pos.font_style);

	// scrolling stat list
	statList->clear();

	if (show_stat[0]) {
		ss.str("");
		ss << msg->get("Max HP:") << " " << stats->get(STAT_HP_MAX);
		statList->append(ss.str(), statTooltip(STAT_HP_MAX));
	}

	if (show_stat[1]) {
		ss.str("");
		ss << msg->get("HP Regen:") << " " << stats->get(STAT_HP_REGEN);
		statList->append(ss.str(), msg->get("Ticks of HP regen per minute. ") + statTooltip(STAT_HP_REGEN));
	}

	if (show_stat[2]) {
		ss.str("");
		ss << msg->get("Max MP:") << " " << stats->get(STAT_MP_MAX);
		statList->append(ss.str(), statTooltip(STAT_MP_MAX));
	}

	if (show_stat[3]) {
		ss.str("");
		ss << msg->get("MP Regen:") << " " << stats->get(STAT_MP_REGEN);
		statList->append(ss.str(), msg->get("Ticks of MP regen per minute. ") + statTooltip(STAT_MP_REGEN));
	}

	if (show_stat[4]) {
		ss.str("");
		ss << msg->get("Accuracy:") << " " << stats->get(STAT_ACCURACY) << "%";
		statList->append(ss.str(), statTooltip(STAT_ACCURACY));
	}

	if (show_stat[5]) {
		ss.str("");
		ss << msg->get("Avoidance:") << " " << stats->get(STAT_AVOIDANCE) << "%";
		statList->append(ss.str(), statTooltip(STAT_AVOIDANCE));
	}

	if (show_stat[6]) {
		ss.str("");
		ss << msg->get("Melee Damage:") << " ";
		if (stats->get(STAT_DMG_MELEE_MIN) == stats->get(STAT_DMG_MELEE_MAX))
			ss << stats->get(STAT_DMG_MELEE_MIN);
		else
			ss << stats->get(STAT_DMG_MELEE_MIN) << "-" << stats->get(STAT_DMG_MELEE_MAX);
		statList->append(ss.str(),"");
	}

	if (show_stat[7]) {
		ss.str("");
		ss << msg->get("Ranged Damage:") << " ";
		if (stats->get(STAT_DMG_RANGED_MIN) == stats->get(STAT_DMG_RANGED_MAX))
			ss << stats->get(STAT_DMG_RANGED_MIN);
		else
			ss << stats->get(STAT_DMG_RANGED_MIN) << "-" << stats->get(STAT_DMG_RANGED_MAX);
		statList->append(ss.str(),"");
	}

	if (show_stat[8]) {
		ss.str("");
		ss << msg->get("Mental Damage:") << " ";
		if (stats->get(STAT_DMG_MENT_MIN) == stats->get(STAT_DMG_MENT_MAX))
			ss << stats->get(STAT_DMG_MENT_MIN);
		else
			ss << stats->get(STAT_DMG_MENT_MIN) << "-" << stats->get(STAT_DMG_MENT_MAX);
		statList->append(ss.str(),"");
	}

	if (show_stat[9]) {
		ss.str("");
		ss << msg->get("Crit:") << " " << stats->get(STAT_CRIT) << "%";
		statList->append(ss.str(), statTooltip(STAT_CRIT));
	}

	if (show_stat[10]) {
		ss.str("");
		ss << msg->get("Absorb:") << " ";
		if (stats->get(STAT_ABS_MIN) == stats->get(STAT_ABS_MAX))
			ss << stats->get(STAT_ABS_MIN);
		else
			ss << stats->get(STAT_ABS_MIN) << "-" << stats->get(STAT_ABS_MAX);
		statList->append(ss.str(),"");
	}

	if (show_stat[11]) {
		ss.str("");
		ss << msg->get("Poise: ") << stats->get(STAT_POISE) << "%";
		statList->append(ss.str(), msg->get("Reduces your chance of stumbling when hit") + statTooltip(STAT_POISE));
	}

	if (show_stat[12]) {
		ss.str("");
		ss << msg->get("Bonus XP: ") << stats->get(STAT_XP_GAIN) << "%";
		statList->append(ss.str(), msg->get("Increases the XP gained per kill") + statTooltip(STAT_XP_GAIN));
	}

	if (show_stat[13]) {
		ss.str("");
		ss << msg->get("Bonus") << " " << CURRENCY << ": " << stats->get(STAT_CURRENCY_FIND) << "%";
		statList->append(ss.str(), msg->get("Increases the %s found per drop",CURRENCY) + statTooltip(STAT_CURRENCY_FIND));
	}

	if (show_stat[14]) {
		ss.str("");
		ss << msg->get("Bonus Item Find: ") << stats->get(STAT_ITEM_FIND)<< "%";
		statList->append(ss.str(), msg->get("Increases the chance that an enemy will drop an item when killed") + statTooltip(STAT_ITEM_FIND));
	}

	if (show_stat[15]) {
		ss.str("");
		ss << msg->get("Stealth: ") << stats->get(STAT_STEALTH) << "%";
		statList->append(ss.str(), msg->get("Increases your ability to move undetected") + statTooltip(STAT_STEALTH));
	}

	if (show_stat[16]) {
		for (unsigned int j=0; j<stats->vulnerable.size(); j++) {
			ss.str("");
			ss << msg->get(ELEMENTS[j].description) << ": " << (100 - stats->vulnerable[j]) << "%";
			statList->append(ss.str(),"");
		}
	}

	statList->refresh();

	// update tool tips
	cstat[CSTAT_NAME].tip.clear();
	cstat[CSTAT_NAME].tip.addText(stats->getLongClass());

	cstat[CSTAT_LEVEL].tip.clear();
	cstat[CSTAT_LEVEL].tip.addText(msg->get("XP: %d", stats->xp));
	if (stats->level < (int)stats->xp_table.size()) {
		cstat[CSTAT_LEVEL].tip.addText(msg->get("Next: %d", stats->xp_table[stats->level]));
	}

	cstat[CSTAT_PHYSICAL].tip.clear();
	cstat[CSTAT_PHYSICAL].tip.addText(msg->get("Physical (P) increases melee weapon proficiency and total HP."));
	cstat[CSTAT_PHYSICAL].tip.addText(msg->get("base (%d), bonus (%d)", stats->physical_character, stats->physical_additional));

	cstat[CSTAT_MENTAL].tip.clear();
	cstat[CSTAT_MENTAL].tip.addText(msg->get("Mental (M) increases mental weapon proficiency and total MP."));
	cstat[CSTAT_MENTAL].tip.addText(msg->get("base (%d), bonus (%d)", stats->mental_character, stats->mental_additional));

	cstat[CSTAT_OFFENSE].tip.clear();
	cstat[CSTAT_OFFENSE].tip.addText(msg->get("Offense (O) increases ranged weapon proficiency and accuracy."));
	cstat[CSTAT_OFFENSE].tip.addText(msg->get("base (%d), bonus (%d)", stats->offense_character, stats->offense_additional));

	cstat[CSTAT_DEFENSE].tip.clear();
	cstat[CSTAT_DEFENSE].tip.addText(msg->get("Defense (D) increases armor proficiency and avoidance."));
	cstat[CSTAT_DEFENSE].tip.addText(msg->get("base (%d), bonus (%d)", stats->defense_character, stats->defense_additional));
}


/**
 * Color-coding for positive/negative/no bonus
 */
Color MenuCharacter::bonusColor(int stat) {
	if (stat > 0) return font->getColor("menu_bonus");
	if (stat < 0) return font->getColor("menu_penalty");
	return font->getColor("menu_label");
}

/**
 * Create tooltip text showing the per_* values of a stat
 */
std::string MenuCharacter::statTooltip(int stat) {
	std::string tooltip_text;

	if (stats->per_level[stat] > 0)
		tooltip_text += msg->get("Each level grants %d. ", stats->per_level[stat]);
	if (stats->per_physical[stat] > 0)
		tooltip_text += msg->get("Each point of Physical grants %d. ", stats->per_physical[stat]);
	if (stats->per_mental[stat] > 0)
		tooltip_text += msg->get("Each point of Mental grants %d. ", stats->per_mental[stat]);
	if (stats->per_offense[stat] > 0)
		tooltip_text += msg->get("Each point of Offense grants %d. ", stats->per_offense[stat]);
	if (stats->per_defense[stat] > 0)
		tooltip_text += msg->get("Each point of Defense grants %d. ", stats->per_defense[stat]);

	return tooltip_text;
}

void MenuCharacter::logic() {
	if (!visible) return;

	tablist.logic();

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}

	// upgrade buttons
	for (int i=0; i<4; i++) {
		upgradeButton[i]->enabled = false;
		tablist.remove(upgradeButton[i]);
	}

	int spent = stats->physical_character + stats->mental_character + stats->offense_character + stats->defense_character -4;
	skill_points = (stats->level * stats->stat_points_per_level) - spent;

	if (stats->hp > 0 && spent < (stats->level * stats->stat_points_per_level) && spent < stats->max_spendable_stat_points) {
		if (stats->physical_character < stats->max_points_per_stat && show_upgrade[0]) {
			upgradeButton[0]->enabled = true;
			tablist.add(upgradeButton[0]);
		}
		if (stats->mental_character  < stats->max_points_per_stat && show_upgrade[1]) {
			upgradeButton[1]->enabled = true;
			tablist.add(upgradeButton[1]);
		}
		if (stats->offense_character < stats->max_points_per_stat && show_upgrade[2]) {
			upgradeButton[2]->enabled = true;
			tablist.add(upgradeButton[2]);
		}
		if (stats->defense_character < stats->max_points_per_stat && show_upgrade[3]) {
			upgradeButton[3]->enabled = true;
			tablist.add(upgradeButton[3]);
		}
	}

	if (upgradeButton[0]->checkClick()) physical_up = true;
	if (upgradeButton[1]->checkClick()) mental_up = true;
	if (upgradeButton[2]->checkClick()) offense_up = true;
	if (upgradeButton[3]->checkClick()) defense_up = true;

	statList->checkClick();

	if (stats->refresh_stats) refreshStats();
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
	for (int i=0; i<CSTAT_COUNT; i++) {
		if (cstat[i].visible) {
			cstat[i].label->render();
			cstat[i].value->render();
		}
	}

	// upgrade buttons
	for (int i=0; i<4; i++) {
		if (upgradeButton[i]->enabled) upgradeButton[i]->render();
	}

	statList->render();
}


/**
 * Display various mouseovers tooltips depending on cursor location
 */
TooltipData MenuCharacter::checkTooltip() {

	for (int i=0; i<CSTAT_COUNT; i++) {
		if (isWithin(cstat[i].hover, inpt->mouse) && !cstat[i].tip.isEmpty() && cstat[i].visible)
			return cstat[i].tip;
	}

	return statList->checkTooltip(inpt->mouse);
}

/**
 * User might click this menu to upgrade a stat.  Check for this situation.
 * Return true if a stat was upgraded.
 */
bool MenuCharacter::checkUpgrade() {
	int spent = stats->physical_character + stats->mental_character + stats->offense_character + stats->defense_character -4;
	skill_points = (stats->level * stats->stat_points_per_level) - spent;

	// check to see if there are skill points available
	if (spent < (stats->level * stats->stat_points_per_level) && spent < stats->max_spendable_stat_points) {

		// physical
		if (physical_up) {
			stats->physical_character++;
			stats->recalc(); // equipment applied by MenuManager
			physical_up = false;
			return true;
		}
		// mental
		else if (mental_up) {
			stats->mental_character++;
			stats->recalc(); // equipment applied by MenuManager
			mental_up = false;
			return true;
		}
		// offense
		else if (offense_up) {
			stats->offense_character++;
			stats->recalc(); // equipment applied by MenuManager
			offense_up = false;
			return true;
		}
		// defense
		else if (defense_up) {
			stats->defense_character++;
			stats->recalc(); // equipment applied by MenuManager
			defense_up = false;
			return true;
		}
	}

	return false;
}

MenuCharacter::~MenuCharacter() {
	delete closeButton;
	delete labelCharacter;
	delete labelUnspent;
	for (int i=0; i<CSTAT_COUNT; i++) {
		delete cstat[i].label;
		delete cstat[i].value;
	}
	for (int i=0; i<4; i++) {
		delete upgradeButton[i];
	}
	delete statList;
}
