/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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
 * class LootManager
 *
 * Handles floor loot
 */

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "CommonIncludes.h"
#include "EnemyManager.h"
#include "FileParser.h"
#include "LootManager.h"
#include "Menu.h"
#include "MenuInventory.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetTooltip.h"

#include <limits>
#include <math.h>

using namespace std;

LootManager::LootManager(StatBlock *_hero) {
	hero = _hero; // we need the player's position for dropping loot in a valid spot

	tip = new WidgetTooltip();

	FileParser infile;
	// load loot animation settings from engine config file
	// @CLASS Loot|Description of engine/loot.txt
	if (infile.open("engine/loot.txt")) {
		while (infile.next()) {
			infile.val = infile.val + ',';

			if (infile.key == "loot_animation") {
				// @ATTR loot_animation|x(int), y(int), w(int), h(int)|
				animation_pos.x = eatFirstInt(infile.val, ',');
				animation_pos.y = eatFirstInt(infile.val, ',');
				animation_pos.w = eatFirstInt(infile.val, ',');
				animation_pos.h = eatFirstInt(infile.val, ',');
			}
			else if (infile.key == "loot_animation_offset") {
				// @ATTR loot_animation_offset|x (integer), y (integer)|
				animation_offset.x = eatFirstInt(infile.val, ',');
				animation_offset.y = eatFirstInt(infile.val, ',');
			}
			else if (infile.key == "tooltip_margin") {
				// @ATTR tooltip_margin|integer|
				tooltip_margin = eatFirstInt(infile.val, ',');
			}
			else if (infile.key == "autopickup_range") {
				// @ATTR autopickup_range|float|Define the range for autopickup feature
				AUTOPICKUP_RANGE = toFloat(infile.val);
			}
			else if (infile.key == "autopickup_currency") {
				// @ATTR autopickup_currency|boolean|Enable autopickup for currency
				AUTOPICKUP_CURRENCY = toBool(eatFirstString(infile.val, ','));
			}
			else if (infile.key == "currency_name") {
				// @ATTR currenct_name|string|Define the name of currency in game
				CURRENCY = msg->get(eatFirstString(infile.val, ','));
			}
			else if (infile.key == "vendor_ratio") {
				// @ATTR vendor_ratio|integer|Prices ratio for vendors
				VENDOR_RATIO = eatFirstInt(infile.val, ',') / 100.0f;
			}
			else if (infile.key == "sfx_loot") {
				// @ATTR sfx_loot|string|Sound effect for dropping loot.
				sfx_loot =  snd->load(eatFirstString(infile.val, ','), "LootManager dropping loot");
			}
		}
		infile.close();
	}

	// reset current map loot
	loot.clear();

	loadGraphics();

	full_msg = false;
}

/**
 * The "loot" variable on each item refers to the "flying loot" animation for that item.
 * Here we load all the animations used by the item database.
 */
void LootManager::loadGraphics() {

	// check all items in the item database
	for (unsigned int i=0; i < items->items.size(); i++) {
		if (items->items[i].loot_animation.empty()) continue;

		for (unsigned int j=0; j<items->items[i].loot_animation.size(); j++) {
			string animationname = "animations/loot/" + items->items[i].loot_animation[j].name + ".txt";
			anim->increaseCount(animationname);
		}
	}
}

void LootManager::handleNewMap() {
	loot.clear();
}

void LootManager::logic() {
	vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ++it) {

		// animate flying loot
		it->animation->advanceFrame();

		if (it->animation->isSecondLastFrame()) {
			if (it->stack.item > 0) {
				Point pos;
				pos.x = (int)it->pos.x;
				pos.y = (int)it->pos.y;
				items->playSound(it->stack.item, pos);
			}
		}
	}

	checkEnemiesForLoot();
	checkMapForLoot();
}

/**
 * Show all tooltips for loot on the floor
 */
void LootManager::renderTooltips(FPoint cam) {
	Point dest;

	vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ++it) {
		if (it->animation->isLastFrame()) {
			Point p = map_to_screen(it->pos.x, it->pos.y, cam.x, cam.y);
			dest.x = p.x;
			dest.y = p.y + TILE_H_HALF;

			// adjust dest.y so that the tooltip floats above the item
			dest.y -= tooltip_margin;

			// create tooltip data if needed
			if (it->tip.isEmpty()) {

				if (it->stack.item > 0) {
					it->tip = items->getShortTooltip(it->stack);
				}
			}

			tip->render(it->tip, dest, STYLE_TOPLABEL);
		}
	}
}

/**
 * Enemies that drop loot raise a "loot_drop" flag to notify this loot
 * manager to create loot based on that creature's level and position.
 */
void LootManager::checkEnemiesForLoot() {
	ItemStack istack;
	istack.quantity = 1;

	for (unsigned i=0; i < enemiesDroppingLoot.size(); ++i) {
		const Enemy *e = enemiesDroppingLoot[i];
		if (e->stats.quest_loot_id != 0) {
			// quest loot
			istack.item = e->stats.quest_loot_id;
			addLoot(istack, e->stats.pos);
		}
		else { // random loot
			//determine position
			FPoint pos = hero->pos;
			if (mapr->collider.is_valid_position(e->stats.pos.x, e->stats.pos.y, MOVEMENT_NORMAL, false))
				pos = e->stats.pos;

			determineLootByEnemy(e, pos);
		}
	}
	enemiesDroppingLoot.clear();
}

void LootManager::addEnemyLoot(const Enemy *e) {
	enemiesDroppingLoot.push_back(e);
}

/**
 * As map events occur, some might have a component named "loot"
 * Loot is created at component x,y
 */
void LootManager::checkMapForLoot() {
	FPoint p;
	Event_Component *ec;
	ItemStack new_loot;
	std::vector<int> possible_ids;
	int common_chance = -1;

	int chance = rand() % 100;

	// first drop any 'fixed' (0% chance) items
	for (unsigned i = mapr->loot.size(); i > 0; i--) {
		ec = &mapr->loot[i-1];
		if (ec->z == 0) {
			p.x = ec->x + 0.5f;
			p.y = ec->y + 0.5f;

			new_loot.quantity = randBetween(ec->a,ec->b);

			// an item id of 0 means we should drop currency instead
			if (ec->s == "currency" || toInt(ec->s) == 0 || toInt(ec->s) == CURRENCY_ID) {
				new_loot.item = CURRENCY_ID;
				new_loot.quantity = new_loot.quantity * (100 + hero->get(STAT_CURRENCY_FIND)) / 100;
			}
			else {
				new_loot.item = toInt(ec->s);
			}

			addLoot(new_loot, p);

			mapr->loot.erase(mapr->loot.begin()+i-1);
		}
	}

	// now pick up to 1 random item to drop
	for (unsigned i = mapr->loot.size(); i > 0; i--) {
		ec = &mapr->loot[i-1];

		if (possible_ids.empty()) {
			// Don't use item find bonus for currency
			int max_chance = ec->z;
			if (ec->s != "currency" && toInt(ec->s) != 0 && toInt(ec->s) != CURRENCY_ID)
				max_chance = ec->z * (hero->get(STAT_ITEM_FIND) + 100) / 100;

			// find the rarest loot less than the chance roll
			if (chance < max_chance) {
				possible_ids.push_back(i-1);
				common_chance = ec->z;
				i=mapr->loot.size(); // start searching from the beginning
				continue;
			}
		}
		else {
			// include loot with identical chances
			if (ec->z == common_chance)
				possible_ids.push_back(i-1);
		}
	}
	if (!possible_ids.empty()) {
		// if there was more than one item with the same chance, randomly pick one of them
		int chosen_loot = 0;
		if (possible_ids.size() > 1) chosen_loot = rand() % possible_ids.size();

		ec = &mapr->loot[chosen_loot];
		p.x = ec->x;
		p.y = ec->y;

		new_loot.quantity = randBetween(ec->a,ec->b);

		// an item id of 0 means we should drop currency instead
		if (ec->s == "currency" || toInt(ec->s) == 0 || toInt(ec->s) == CURRENCY_ID) {
			new_loot.item = CURRENCY_ID;
			new_loot.quantity = new_loot.quantity * (100 + hero->get(STAT_CURRENCY_FIND)) / 100;
		}
		else {
			new_loot.item = toInt(ec->s);
		}

		addLoot(new_loot, p);
	}

	mapr->loot.clear();
}

/**
 * This function is called when there definitely is a piece of loot dropping
 * calls addLoot()
 */
void LootManager::determineLootByEnemy(const Enemy *e, FPoint pos) {
	ItemStack new_loot;
	std::vector<int> possible_ids;
	std::vector<Point> possible_ranges;
	Point range;
	int common_chance = -1;

	int chance = rand() % 100;

	for (unsigned i=0; i<e->stats.loot.size(); i++) {
		if (possible_ids.empty()) {
			// Don't use item find bonus for currency
			int max_chance = e->stats.loot[i].chance;
			if (e->stats.loot[i].id != 0 && e->stats.loot[i].id != CURRENCY_ID)
				max_chance = e->stats.loot[i].chance * (hero->get(STAT_ITEM_FIND) + 100) / 100;

			// find the rarest loot less than the chance roll
			if (chance < max_chance) {
				possible_ids.push_back(e->stats.loot[i].id);
				common_chance = e->stats.loot[i].chance;

				range.x = e->stats.loot[i].count_min;
				range.y = e->stats.loot[i].count_max;
				possible_ranges.push_back(range);

				i=-1; // start searching from the beginning
				continue;
			}
		}
		else {
			// include loot with identical chances
			if (e->stats.loot[i].chance == common_chance) {
				possible_ids.push_back(e->stats.loot[i].id);

				range.x = e->stats.loot[i].count_min;
				range.y = e->stats.loot[i].count_max;
				possible_ranges.push_back(range);

			}
		}
	}

	if (!possible_ids.empty()) {

		int roll = rand() % possible_ids.size();
		new_loot.item = possible_ids[roll];
		new_loot.quantity = randBetween(possible_ranges[roll].x, possible_ranges[roll].y);

		// an item id of 0 means we should drop currency instead
		if (new_loot.item == 0 || new_loot.item == CURRENCY_ID) {
			new_loot.item = CURRENCY_ID;
			new_loot.quantity = (new_loot.quantity * (100 + hero->get(STAT_CURRENCY_FIND))) / 100;
		}

		addLoot(new_loot, pos);

	}
}

void LootManager::addLoot(ItemStack stack, FPoint pos) {
	// TODO: z-sort insert?
	Loot ld;
	ld.stack = stack;
	ld.pos.x = pos.x;
	ld.pos.y = pos.y;

	int index = items->items[stack.item].loot_animation.size()-1;
	for (unsigned int i=0; i<items->items[stack.item].loot_animation.size(); i++) {
		if (stack.quantity >= items->items[stack.item].loot_animation[i].low && (stack.quantity <= items->items[stack.item].loot_animation[i].high || items->items[stack.item].loot_animation[i].high == 0)) {
			index = i;
			break;
		}
	}
	const string anim_id = items->items[stack.item].loot_animation[index].name;
	const string animationname = "animations/loot/" + anim_id + ".txt";
	ld.loadAnimation(animationname);
	loot.push_back(ld);
	snd->play(sfx_loot, GLOBAL_VIRTUAL_CHANNEL, pos, false);
}

/**
 * Click on the map to pick up loot.  We need the camera position to translate
 * screen coordinates to map locations.  We need the hero position because
 * the hero has to be within range to pick up an item.
 */
ItemStack LootManager::checkPickup(Point mouse, FPoint cam, FPoint hero_pos, MenuInventory *inv) {
	SDL_Rect r;
	ItemStack loot_stack;
	loot_stack.item = 0;
	loot_stack.quantity = 0;

	// I'm starting at the end of the loot list so that more recently-dropped
	// loot is picked up first.  If a player drops several loot in the same
	// location, picking it back up will work like a stack.
	vector<Loot>::iterator it;
	for (it = loot.end(); it != loot.begin(); ) {
		--it;

		// loot close enough to pickup?
		if (fabs(hero_pos.x - it->pos.x) < LOOT_RANGE && fabs(hero_pos.y - it->pos.y) < LOOT_RANGE && !it->isFlying()) {

			Point p = map_to_screen(it->pos.x, it->pos.y, cam.x, cam.y);

			r.w = 32;
			r.h = 48;
			r.x = p.x - 16;
			r.y = p.y - 32;

			// clicked in pickup hotspot?
			if (mouse.x > r.x && mouse.x < r.x+r.w &&
					mouse.y > r.y && mouse.y < r.y+r.h) {

				if (it->stack.item > 0 && !(inv->full(it->stack.item))) {
					loot_stack = it->stack;
					it = loot.erase(it);
					return loot_stack;
				}
				else if (it->stack.item > 0) {
					full_msg = true;
				}
			}
		}
	}
	return loot_stack;
}

/**
 * Autopickup loot if enabled in the engine
 * Currently, only currency is checked for autopickup
 */
ItemStack LootManager::checkAutoPickup(FPoint hero_pos, MenuInventory *inv) {
	ItemStack loot_stack;
	loot_stack.item = 0;
	loot_stack.quantity = 0;

	vector<Loot>::iterator it;
	for (it = loot.end(); it != loot.begin(); ) {
		--it;
		if (fabs(hero_pos.x - it->pos.x) < AUTOPICKUP_RANGE && fabs(hero_pos.y - it->pos.y) < AUTOPICKUP_RANGE && !it->isFlying()) {
			if (it->stack.item == CURRENCY_ID && AUTOPICKUP_CURRENCY) {
				if (!(inv->full(it->stack.item))) {
					loot_stack = it->stack;
					it = loot.erase(it);
					return loot_stack;
				}
			}
		}
	}
	return loot_stack;
}

ItemStack LootManager::checkNearestPickup(FPoint hero_pos, MenuInventory *inv) {
	ItemStack loot_stack;
	loot_stack.item = 0;
	loot_stack.quantity = 0;

	float best_distance = std::numeric_limits<float>::max();

	vector<Loot>::iterator it;
	vector<Loot>::iterator nearest = loot.end();

	for (it = loot.end(); it != loot.begin(); ) {
		--it;

		float distance = calcDist(hero_pos, it->pos);
		if (distance < LOOT_RANGE && distance < best_distance) {
			best_distance = distance;
			nearest = it;
		}
	}

	if (nearest != loot.end()) {
		if (nearest->stack.item > 0 && !(inv->full(nearest->stack.item))) {
			loot_stack = nearest->stack;
			loot.erase(nearest);
			return loot_stack;
		}
		else if (nearest->stack.item > 0) {
			full_msg = true;
		}
	}

	return loot_stack;
}

void LootManager::addRenders(vector<Renderable> &ren, vector<Renderable> &ren_dead) {
	vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ++it) {
		Renderable r = it->animation->getCurrentFrame(0);
		r.map_pos.x = it->pos.x;
		r.map_pos.y = it->pos.y;

		(it->animation->isLastFrame() ? ren_dead : ren).push_back(r);
	}
}

LootManager::~LootManager() {
	// remove all items in the item database
	for (unsigned int i=0; i < items->items.size(); i++) {
		if (items->items[i].loot_animation.empty()) continue;

		for (unsigned int j=0; j<items->items[i].loot_animation.size(); j++) {
			string animationname = "animations/loot/" + items->items[i].loot_animation[j].name + ".txt";
			anim->decreaseCount(animationname);
		}
	}

	// remove items, so Loots get destroyed!
	loot.clear();

	anim->cleanUp();

	snd->unload(sfx_loot);

	delete tip;
}
