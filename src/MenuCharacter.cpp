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

#include "FileParser.h"
#include "Menu.h"
#include "MenuCharacter.h"
#include "SharedResources.h"
#include "Settings.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetListBox.h"

MenuCharacter::MenuCharacter(StatBlock *_stats) {
	stats = _stats;

	// Labels for major stats
	cstat_labels[CSTAT_NAME] = msg->get("Name");
	cstat_labels[CSTAT_LEVEL] = msg->get("Level");
	cstat_labels[CSTAT_PHYSICAL] = msg->get("Physical");
	cstat_labels[CSTAT_MENTAL] = msg->get("Mental");
	cstat_labels[CSTAT_OFFENSE] = msg->get("Offense");
	cstat_labels[CSTAT_DEFENSE] = msg->get("Defense");

	skill_points = 0;

	visible = false;

	for (int i=0; i<CSTAT_COUNT; i++) {
		cstat[i].label = new WidgetLabel();
		cstat[i].value = new WidgetLabel();
		cstat[i].hover.x = cstat[i].hover.y = 0;
		cstat[i].hover.w = cstat[i].hover.h = 0;
		cstat[i].visible = true;
	}
	for (int i=0; i<STAT_COUNT; i++) {
		show_stat[i] = true;
	}

	// these two are hidden by default, as they are currently unused
	show_stat[STAT_HP_PERCENT] = false;
	show_stat[STAT_MP_PERCENT] = false;

	show_resists = true;

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

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR label_title|label|Position of the "Character" text.
			else if(infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR upgrade_physical|point|Position of the button used to add a stat point to Physical.
			else if(infile.key == "upgrade_physical") {
				Point pos = toPoint(infile.val);
				upgradeButton[0]->setBasePos(pos.x, pos.y);
			}
			// @ATTR upgrade_mental|point|Position of the button used to add a stat point to Mental.
			else if(infile.key == "upgrade_mental")	{
				Point pos = toPoint(infile.val);
				upgradeButton[1]->setBasePos(pos.x, pos.y);
			}
			// @ATTR upgrade_offense|point|Position of the button used to add a stat point to Offense.
			else if(infile.key == "upgrade_offense") {
				Point pos = toPoint(infile.val);
				upgradeButton[2]->setBasePos(pos.x, pos.y);
			}
			// @ATTR upgrade_defense|point|Position of the button used to add a stat point to Defense.
			else if(infile.key == "upgrade_defense") {
				Point pos = toPoint(infile.val);
				upgradeButton[3]->setBasePos(pos.x, pos.y);
			}
			// @ATTR statlist|point|Position of the scrollbox containing non-primary stats.
			else if(infile.key == "statlist") statlist_pos = toPoint(infile.val);
			// @ATTR statlist_rows|int|The height of the statlist in rows.
			else if (infile.key == "statlist_rows") statlist_rows = toInt(infile.val);
			// @ATTR statlist_scrollbar_offset|int|Right margin in pixels for the statlist's scrollbar.
			else if (infile.key == "statlist_scrollbar_offset") statlist_scrollbar_offset = toInt(infile.val);

			// @ATTR label_name|label|Position of the "Name" text.
			else if(infile.key == "label_name") {
				label_pos[CSTAT_NAME] = eatLabelInfo(infile.val);
				cstat[CSTAT_NAME].visible = !label_pos[CSTAT_NAME].hidden;
			}
			// @ATTR label_level|label|Position of the "Level" text.
			else if(infile.key == "label_level") {
				label_pos[CSTAT_LEVEL] = eatLabelInfo(infile.val);
				cstat[CSTAT_LEVEL].visible = !label_pos[CSTAT_LEVEL].hidden;
			}
			// @ATTR label_physical|label|Position of the "Physical" text.
			else if(infile.key == "label_physical") {
				label_pos[CSTAT_PHYSICAL] = eatLabelInfo(infile.val);
				cstat[CSTAT_PHYSICAL].visible = !label_pos[CSTAT_PHYSICAL].hidden;
			}
			// @ATTR label_mental|label|Position of the "Mental" text.
			else if(infile.key == "label_mental") {
				label_pos[CSTAT_MENTAL] = eatLabelInfo(infile.val);
				cstat[CSTAT_MENTAL].visible = !label_pos[CSTAT_MENTAL].hidden;
			}
			// @ATTR label_offense|label|Position of the "Offense" text.
			else if(infile.key == "label_offense") {
				label_pos[CSTAT_OFFENSE] = eatLabelInfo(infile.val);
				cstat[CSTAT_OFFENSE].visible = !label_pos[CSTAT_OFFENSE].hidden;
			}
			// @ATTR label_defense|label|Position of the "Defense" text.
			else if(infile.key == "label_defense") {
				label_pos[CSTAT_DEFENSE] = eatLabelInfo(infile.val);
				cstat[CSTAT_DEFENSE].visible = !label_pos[CSTAT_DEFENSE].hidden;
			}

			// @ATTR name|rectangle|Position of the player's name and dimensions of the tooltip hotspot.
			else if(infile.key == "name") {
				value_pos[CSTAT_NAME] = toRect(infile.val);
				cstat[CSTAT_NAME].value->setBasePos(value_pos[CSTAT_NAME].x + 4, value_pos[CSTAT_NAME].y + (value_pos[CSTAT_NAME].h/2)); // TODO remove 4 from x value
			}
			// @ATTR level|rectangle|Position of the player's level and dimensions of the tooltip hotspot.
			else if(infile.key == "level") {
				value_pos[CSTAT_LEVEL] = toRect(infile.val);
				cstat[CSTAT_LEVEL].value->setBasePos(value_pos[CSTAT_LEVEL].x + (value_pos[CSTAT_LEVEL].w/2), value_pos[CSTAT_LEVEL].y + (value_pos[CSTAT_LEVEL].h/2));
			}
			// @ATTR physical|rectangle|Position of the player's physical stat and dimensions of the tooltip hotspot.
			else if(infile.key == "physical") {
				value_pos[CSTAT_PHYSICAL] = toRect(infile.val);
				cstat[CSTAT_PHYSICAL].value->setBasePos(value_pos[CSTAT_PHYSICAL].x + (value_pos[CSTAT_PHYSICAL].w/2), value_pos[CSTAT_PHYSICAL].y + (value_pos[CSTAT_PHYSICAL].h/2));
			}
			// @ATTR mental|rectangle|Position of the player's mental stat and dimensions of the tooltip hotspot.
			else if(infile.key == "mental") {
				value_pos[CSTAT_MENTAL] = toRect(infile.val);
				cstat[CSTAT_MENTAL].value->setBasePos(value_pos[CSTAT_MENTAL].x + (value_pos[CSTAT_MENTAL].w/2), value_pos[CSTAT_MENTAL].y + (value_pos[CSTAT_MENTAL].h/2));
			}
			// @ATTR offense|rectangle|Position of the player's offense stat and dimensions of the tooltip hotspot.
			else if(infile.key == "offense") {
				value_pos[CSTAT_OFFENSE] = toRect(infile.val);
				cstat[CSTAT_OFFENSE].value->setBasePos(value_pos[CSTAT_OFFENSE].x + (value_pos[CSTAT_OFFENSE].w/2), value_pos[CSTAT_OFFENSE].y + (value_pos[CSTAT_OFFENSE].h/2));
			}
			// @ATTR defense|rectangle|Position of the player's defense stat and dimensions of the tooltip hotspot.
			else if(infile.key == "defense") {
				value_pos[CSTAT_DEFENSE] = toRect(infile.val);
				cstat[CSTAT_DEFENSE].value->setBasePos(value_pos[CSTAT_DEFENSE].x + (value_pos[CSTAT_DEFENSE].w/2), value_pos[CSTAT_DEFENSE].y + (value_pos[CSTAT_DEFENSE].h/2));
			}

			// @ATTR unspent|label|Position of the label showing the number of unspent stat points.
			else if(infile.key == "unspent") unspent_pos = eatLabelInfo(infile.val);

			// @ATTR show_upgrade_physical|bool|Hide the Physical upgrade button if set to false.
			else if (infile.key == "show_upgrade_physical") show_upgrade[0] = toBool(infile.val);
			// @ATTR show_upgrade_mental|bool|Hide the Mental upgrade button if set to false.
			else if (infile.key == "show_upgrade_mental") show_upgrade[1] = toBool(infile.val);
			// @ATTR show_upgrade_offense|bool|Hide the Offense upgrade button if set to false.
			else if (infile.key == "show_upgrade_offense") show_upgrade[2] = toBool(infile.val);
			// @ATTR show_upgrade_defense|bool|Hide the Defense upgrade button if set to false.
			else if (infile.key == "show_upgrade_defense") show_upgrade[3] = toBool(infile.val);

			// @ATTR show_resists|bool|Hide the elemental "Resistance" stats in the statlist if set to false.
			else if (infile.key == "show_resists") show_resists = toBool(infile.val);

			// @ATTR show_stat|string, bool : Stat name, Visible|Hide the matching stat in the statlist if set to false.
			else if (infile.key == "show_stat") {
				std::string stat_name = popFirstString(infile.val, ',');

				for (unsigned i=0; i<STAT_COUNT; ++i) {
					if (stat_name == STAT_KEY[i]) {
						show_stat[i] = toBool(popFirstString(infile.val, ','));
						break;
					}
				}
			}

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
	statList->setBasePos(statlist_pos.x, statlist_pos.y);

	// HACK: During gameplay, the stat list can refresh rapidly when the charcter menu is open and the player has certain effects
	// frequently refreshing trimmed text is slow for Cyrillic characters, so disable it here
	statList->disable_text_trim = true;

	setBackground("images/menus/character.png");

	align();

	base_stats[0] = &stats->physical_character;
	base_stats[1] = &stats->mental_character;
	base_stats[2] = &stats->offense_character;
	base_stats[3] = &stats->defense_character;

	base_stats_add[0] = &stats->physical_additional;
	base_stats_add[1] = &stats->mental_additional;
	base_stats_add[2] = &stats->offense_additional;
	base_stats_add[3] = &stats->defense_additional;

	base_bonus[0] = &stats->per_physical;
	base_bonus[1] = &stats->per_mental;
	base_bonus[2] = &stats->per_offense;
	base_bonus[3] = &stats->per_defense;
}

void MenuCharacter::align() {
	Menu::align();

	// close button
	closeButton->setPos(window_area.x, window_area.y);

	// menu title
	labelCharacter->set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Character"), font->getColor("menu_normal"), title.font_style);

	// upgrade buttons
	for (int i=0; i<4; i++) {
		upgradeButton[i]->setPos(window_area.x, window_area.y);
	}

	// stat list
	statList->setPos(window_area.x, window_area.y);

	for (int i=0; i<CSTAT_COUNT; i++) {
		// setup static labels
		cstat[i].label->set(window_area.x+label_pos[i].x, window_area.y+label_pos[i].y, label_pos[i].justify, label_pos[i].valign, cstat_labels[i], font->getColor("menu_normal"), label_pos[i].font_style);

		// setup hotspot locations
		cstat[i].setHover(window_area.x+value_pos[i].x, window_area.y+value_pos[i].y, value_pos[i].w, value_pos[i].h);

		// setup value labels
		cstat[i].value->setPos(window_area.x, window_area.y);
	}

	labelUnspent->setX(window_area.x + unspent_pos.x);
	labelUnspent->setY(window_area.y + unspent_pos.y);
}

/**
 * Rebuild all stat values and tooltip info
 */
void MenuCharacter::refreshStats() {

	stats->refresh_stats = false;

	std::stringstream ss;

	// update stat text
	cstat[CSTAT_NAME].value->set(cstat[CSTAT_NAME].value->pos.x, cstat[CSTAT_NAME].value->pos.y, JUSTIFY_LEFT, VALIGN_CENTER, stats->name, font->getColor("menu_normal"));

	ss.str("");
	ss << stats->level;
	cstat[CSTAT_LEVEL].value->set(cstat[CSTAT_LEVEL].value->pos.x, cstat[CSTAT_LEVEL].value->pos.y, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), font->getColor("menu_normal"));

	ss.str("");
	ss << stats->get_physical();
	cstat[CSTAT_PHYSICAL].value->set(cstat[CSTAT_PHYSICAL].value->pos.x, cstat[CSTAT_PHYSICAL].value->pos.y, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->physical_additional));

	ss.str("");
	ss << stats->get_mental();
	cstat[CSTAT_MENTAL].value->set(cstat[CSTAT_MENTAL].value->pos.x, cstat[CSTAT_MENTAL].value->pos.y, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->mental_additional));

	ss.str("");
	ss << stats->get_offense();
	cstat[CSTAT_OFFENSE].value->set(cstat[CSTAT_OFFENSE].value->pos.x, cstat[CSTAT_OFFENSE].value->pos.y, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->offense_additional));

	ss.str("");
	ss << stats->get_defense();
	cstat[CSTAT_DEFENSE].value->set(cstat[CSTAT_DEFENSE].value->pos.x, cstat[CSTAT_DEFENSE].value->pos.y, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->defense_additional));

	ss.str("");
	if (skill_points > 0) ss << skill_points << " " << msg->get("points remaining");
	else ss.str("");
	labelUnspent->set(window_area.x+unspent_pos.x, window_area.y+unspent_pos.y, unspent_pos.justify, unspent_pos.valign, ss.str(), font->getColor("menu_bonus"), unspent_pos.font_style);

	// scrolling stat list
	for (unsigned i=0; i<STAT_COUNT; ++i) {
		if (!show_stat[i]) continue;

		ss.str("");
		ss << STAT_NAME[i] << ": " << stats->get((STAT)i);
		if (STAT_PERCENT[i]) ss << "%";

		std::string stat_tooltip = statTooltip(i);
		std::string full_tooltip = "";
		if (STAT_DESC[i] != "")
			full_tooltip += STAT_DESC[i];
		if (full_tooltip != "" && stat_tooltip != "")
			full_tooltip += "\n";
		full_tooltip += stat_tooltip;

		statList->set(i, ss.str(), full_tooltip);
	}

	if (show_resists) {
		for (unsigned int j=0; j<stats->vulnerable.size(); ++j) {
			ss.str("");
			ss << msg->get("%s Resistance", ELEMENTS[j].name.c_str()) << ": " << (100 - stats->vulnerable[j]) << "%";
			statList->set(j+STAT_COUNT-2, ss.str(),"");
		}
	}

	// update tool tips
	cstat[CSTAT_NAME].tip.clear();
	cstat[CSTAT_NAME].tip.addText(stats->getLongClass());

	cstat[CSTAT_LEVEL].tip.clear();
	cstat[CSTAT_LEVEL].tip.addText(msg->get("XP: %d", stats->xp));
	if (static_cast<unsigned>(stats->level) < stats->xp_table.size()) {
		cstat[CSTAT_LEVEL].tip.addText(msg->get("Next: %d", stats->xp_table[stats->level]));
	}

	for (unsigned j=2; j<static_cast<unsigned>(CSTAT_COUNT); ++j) {
		cstat[j].tip.clear();
		cstat[j].tip.addText(cstat_labels[j]);
		cstat[j].tip.addText(msg->get("base (%d), bonus (%d)", *(base_stats[j-2]), *(base_stats_add[j-2])));
		bool have_bonus = false;
		for (unsigned i=0; i<STAT_COUNT; ++i) {
			if (base_bonus[j-2]->at(i) > 0) {
				if (!have_bonus) {
					cstat[j].tip.addText("\n" + msg->get("Related stats:"));
					have_bonus = true;
				}
				cstat[j].tip.addText(STAT_NAME[i]);
			}
		}
	}
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

	int spent = stats->physical_character + stats->mental_character + stats->offense_character + stats->defense_character -4;
	skill_points = (stats->level * stats->stat_points_per_level) - spent;

	if (skill_points == 0) {
		// upgrade buttons
		for (int i=0; i<4; i++) {
			upgradeButton[i]->enabled = false;
			tablist.remove(upgradeButton[i]);
		}

		if (tablist.getCurrent() >= static_cast<int>(tablist.size())) {
			tablist.defocus();
			tablist.getNext();
		}
	}

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
