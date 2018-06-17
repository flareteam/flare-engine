/*
Copyright © 2011-2012 Clint Bellanger
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
 * class LootManager
 *
 * Handles floor loot
 */

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "CommonIncludes.h"
#include "CursorManager.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "InputState.h"
#include "LootManager.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Utils.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetTooltip.h"

#include <limits>
#include <math.h>

LootManager::LootManager()
	: tip(new WidgetTooltip())
	, sfx_loot(snd->load(eset->loot.sfx_loot, "LootManager dropping loot"))
	, hero(NULL)
{
	loadGraphics();
	loadLootTables();
}

/**
 * The "loot" variable on each item refers to the "flying loot" animation for that item.
 * Here we load all the animations used by the item database.
 */
void LootManager::loadGraphics() {
	animations.resize(items->items.size());

	// check all items in the item database
	for (unsigned int i=0; i < items->items.size(); i++) {
		if (items->items[i].loot_animation.empty()) continue;

		animations[i].resize(items->items[i].loot_animation.size());

		for (unsigned int j=0; j<items->items[i].loot_animation.size(); j++) {
			anim->increaseCount(items->items[i].loot_animation[j].name);
			animations[i][j] = anim->getAnimationSet(items->items[i].loot_animation[j].name)->getAnimation("");
		}
	}
}

void LootManager::handleNewMap() {
	loot.clear();
}

void LootManager::logic() {
	std::vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ++it) {

		// animate flying loot
		if (it->animation) {
			it->animation->advanceFrame();
			if (it->animation->isSecondLastFrame()) {
				it->on_ground = true;
			}
		}

		if (it->on_ground && !it->sound_played && !it->stack.empty()) {
			Point pos;
			pos.x = static_cast<int>(it->pos.x);
			pos.y = static_cast<int>(it->pos.y);
			items->playSound(it->stack.item, pos);
			it->sound_played = true;
		}
	}

	checkEnemiesForLoot();
	checkMapForLoot();

	// clear any tiles that were blocked from dropped loot
	for (unsigned i=0; i<tiles_to_unblock.size(); i++) {
		mapr->collider.unblock(static_cast<float>(tiles_to_unblock[i].x), static_cast<float>(tiles_to_unblock[i].y));
	}
	tiles_to_unblock.clear();
}

/**
 * Show all tooltips for loot on the floor
 */
void LootManager::renderTooltips(const FPoint& cam) {
	if (!SHOW_HUD) return;

	Point dest;
	bool tooltip_below = true;

	std::vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ) {
		it->tip_visible = false;

		if (it->on_ground) {
			Point p = map_to_screen(it->pos.x, it->pos.y, cam.x, cam.y);
			dest.x = p.x;
			dest.y = p.y + eset->tileset.tile_h_half;

			// adjust dest.y so that the tooltip floats above the item
			dest.y -= eset->loot.tooltip_margin;

			// set hitbox for mouse hover
			Rect hover;
			hover.x = p.x - eset->tileset.tile_w_half;
			hover.y = p.y - eset->tileset.tile_h_half;
			hover.w = eset->tileset.tile_w;
			hover.h = eset->tileset.tile_h;

			if ((LOOT_TOOLTIPS && !inpt->pressing[Input::ALT]) || (!LOOT_TOOLTIPS && inpt->pressing[Input::ALT]) || isWithinRect(hover, inpt->mouse)) {
				it->tip_visible = true;

				// create tooltip data if needed
				if (it->tip.isEmpty()) {
					if (!it->stack.empty()) {
						it->tip = items->getShortTooltip(it->stack);
					}
				}

				// try to prevent tooltips from overlapping
				tip->prerender(it->tip, dest, STYLE_TOPLABEL);
				std::vector<Loot>::iterator test_it;
				for (test_it = loot.begin(); test_it != it; ) {
					if (rectsOverlap(test_it->tip_bounds, tip->bounds)) {
						if (tooltip_below)
							dest.y = test_it->tip_bounds.y + test_it->tip_bounds.h + eset->tooltips.offset;
						else
							dest.y = test_it->tip_bounds.y - test_it->tip_bounds.h + eset->tooltips.offset;

						tip->bounds.y = dest.y;
					}

					++test_it;
				}

				tip->render(it->tip, dest, STYLE_TOPLABEL);
				it->tip_bounds = tip->bounds;

				// only display one tooltip if we got it from hovering
				if (!LOOT_TOOLTIPS && !inpt->pressing[Input::ALT])
					break;
			}
		}

		tooltip_below = !tooltip_below;

		++it;
	}
}

/**
 * Enemies that drop loot raise a "loot_drop" flag to notify this loot
 * manager to create loot based on that creature's level and position.
 */
void LootManager::checkEnemiesForLoot() {
	for (unsigned i=0; i < enemiesDroppingLoot.size(); ++i) {
		Enemy *e = enemiesDroppingLoot[i];

		if (e->stats.quest_loot_id != 0) {
			// quest loot
			Event_Component ec;
			ec.type = EC_LOOT;
			ec.c = e->stats.quest_loot_id;
			ec.a = ec.b = 1;
			ec.z = 0;

			e->stats.loot_table.push_back(ec);
		}

		if (!e->stats.loot_table.empty()) {
			unsigned drops;
			if (e->stats.loot_count.y != 0) {
				drops = randBetween(e->stats.loot_count.x, e->stats.loot_count.y);
			}
			else {
				drops = randBetween(1, eset->loot.drop_max);
			}

			for (unsigned j=0; j<drops; ++j) {
				checkLoot(e->stats.loot_table, &e->stats.pos, NULL);
			}

			e->stats.loot_table.clear();
		}
	}
	enemiesDroppingLoot.clear();
}

/**
 * As map events occur, some might have a component named "loot"
 */
void LootManager::checkMapForLoot() {
	if (!mapr->loot.empty()) {
		unsigned drops;
		if (mapr->loot_count.y != 0) {
			drops = randBetween(mapr->loot_count.x, mapr->loot_count.y);
		}
		else {
			drops = randBetween(1, eset->loot.drop_max);
		}

		for (unsigned i=0; i<drops; ++i) {
			checkLoot(mapr->loot, NULL, NULL);
		}

		mapr->loot.clear();
		mapr->loot_count.x = 0;
		mapr->loot_count.y = 0;
	}
}

void LootManager::addEnemyLoot(Enemy *e) {
	enemiesDroppingLoot.push_back(e);
}

void LootManager::checkLoot(std::vector<Event_Component> &loot_table, FPoint *pos, std::vector<ItemStack> *itemstack_vec) {
	if (hero == NULL) {
		logError("LootManager: checkLoot() failed, no hero.");
		return;
	}

	FPoint p;
	Event_Component *ec;
	ItemStack new_loot;
	std::vector<Event_Component*> possible_ids;

	int chance = rand() % 100;

	// first drop any 'fixed' (0% chance) items
	for (size_t i = loot_table.size(); i > 0; i--) {
		ec = &loot_table[i-1];
		if (ec->z == 0) {
			Point src;
			if (pos) {
				src = FPointToPoint(*pos);
			}
			else {
				src.x = ec->x;
				src.y = ec->y;
			}
			p.x = static_cast<float>(src.x) + 0.5f;
			p.y = static_cast<float>(src.y) + 0.5f;

			if (!mapr->collider.is_valid_position(p.x, p.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_NORMAL)) {
				p = mapr->collider.get_random_neighbor(src, eset->loot.drop_radius, !MapCollision::IGNORE_BLOCKED);

				if (!mapr->collider.is_valid_position(p.x, p.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_NORMAL)) {
					p = hero->pos;
				}
				else {
					if (src.x == static_cast<int>(p.x) && src.y == static_cast<int>(p.y))
						p = hero->pos;

					mapr->collider.block(p.x, p.y, !MapCollision::IS_ALLY);
					tiles_to_unblock.push_back(FPointToPoint(p));
				}
			}

			new_loot.quantity = randBetween(ec->a,ec->b);

			// an item id of 0 means we should drop currency instead
			if (ec->c == 0 || ec->c == eset->misc.currency_id) {
				new_loot.item = eset->misc.currency_id;
				new_loot.quantity = new_loot.quantity * (100 + hero->get(STAT_CURRENCY_FIND)) / 100;
			}
			else {
				new_loot.item = ec->c;
			}

			if (itemstack_vec)
				itemstack_vec->push_back(new_loot);
			else
				addLoot(new_loot, p, !DROPPED_BY_HERO);

			loot_table.erase(loot_table.begin()+i-1);
		}
	}

	// now pick up to 1 random item to drop
	int threshold = hero->get(STAT_ITEM_FIND) + 100;
	for (unsigned i = 0; i < loot_table.size(); i++) {
		ec = &loot_table[i];

		int real_chance = ec->z;

		if (ec->c != 0 && ec->c != eset->misc.currency_id) {
			real_chance = static_cast<int>(static_cast<float>(ec->z) * static_cast<float>(hero->get(STAT_ITEM_FIND) + 100) / 100.f);
		}

		if (real_chance >= chance) {
			if (real_chance <= threshold) {
				if (real_chance != threshold) {
					possible_ids.clear();
				}

				threshold = real_chance;
			}

			if (chance <= threshold) {
				possible_ids.push_back(ec);
			}
		}
	}

	if (!possible_ids.empty()) {
		// if there was more than one item with the same chance, randomly pick one of them
		size_t chosen_loot = static_cast<size_t>(rand()) % possible_ids.size();

		ec = possible_ids[chosen_loot];

		Point src;
		if (pos) {
			src = FPointToPoint(*pos);
		}
		else {
			src.x = ec->x;
			src.y = ec->y;
		}
		p.x = static_cast<float>(src.x) + 0.5f;
		p.y = static_cast<float>(src.y) + 0.5f;

		if (!mapr->collider.is_valid_position(p.x, p.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_NORMAL)) {
			p = mapr->collider.get_random_neighbor(src, eset->loot.drop_radius, !MapCollision::IGNORE_BLOCKED);

			if (!mapr->collider.is_valid_position(p.x, p.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_NORMAL)) {
				p = hero->pos;
			}
			else {
				if (src.x == static_cast<int>(p.x) && src.y == static_cast<int>(p.y))
					p = hero->pos;

				mapr->collider.block(p.x, p.y, !MapCollision::IS_ALLY);
				tiles_to_unblock.push_back(FPointToPoint(p));
			}
		}

		new_loot.quantity = randBetween(ec->a,ec->b);

		// an item id of 0 means we should drop currency instead
		if (ec->c == 0 || ec->c == eset->misc.currency_id) {
			new_loot.item = eset->misc.currency_id;
			new_loot.quantity = new_loot.quantity * (100 + hero->get(STAT_CURRENCY_FIND)) / 100;
		}
		else {
			new_loot.item = ec->c;
		}

		if (itemstack_vec)
			itemstack_vec->push_back(new_loot);
		else
			addLoot(new_loot, p, !DROPPED_BY_HERO);
	}
}

void LootManager::addLoot(ItemStack stack, const FPoint& pos, bool dropped_by_hero) {
	if (static_cast<size_t>(stack.item) >= items->items.size()) {
		logError("LootManager: Loot item with id %d is not valid.", stack.item);
		return;
	}

	Loot ld;
	ld.stack = stack;
	ld.pos.x = pos.x;
	ld.pos.y = pos.y;
	alignFPoint(&ld.pos);
	ld.dropped_by_hero = dropped_by_hero;

	if (!items->items[stack.item].loot_animation.empty()) {
		size_t index = items->items[stack.item].loot_animation.size()-1;

		for (unsigned int i=0; i<items->items[stack.item].loot_animation.size(); i++) {
			int low = items->items[stack.item].loot_animation[i].low;
			int high = items->items[stack.item].loot_animation[i].high;
			if (stack.quantity >= low && (stack.quantity <= high || high == 0)) {
				index = i;
				break;
			}
		}
		ld.loadAnimation(items->items[stack.item].loot_animation[index].name);
	}
	else {
		// immediately place the loot on the ground if there's no animation
		ld.on_ground = true;
	}

	loot.push_back(ld);
	snd->play(sfx_loot, GLOBAL_VIRTUAL_CHANNEL, pos, false);
}

/**
 * Click on the map to pick up loot.  We need the camera position to translate
 * screen coordinates to map locations.
 */
ItemStack LootManager::checkPickup(const Point& mouse, const FPoint& cam, const FPoint& hero_pos) {
	Rect r;
	ItemStack loot_stack;

	// check left mouse click
	if (inpt->usingMouse()) {
		// I'm starting at the end of the loot list so that more recently-dropped
		// loot is picked up first.  If a player drops several loot in the same
		// location, picking it back up will work like a stack.
		std::vector<Loot>::iterator it;
		for (it = loot.end(); it != loot.begin(); ) {
			--it;

			// loot close enough to pickup?
			if (fabs(hero_pos.x - it->pos.x) < eset->misc.interact_range && fabs(hero_pos.y - it->pos.y) < eset->misc.interact_range && !it->isFlying()) {
				Point p = map_to_screen(it->pos.x, it->pos.y, cam.x, cam.y);

				r.x = p.x - eset->tileset.tile_w_half;
				r.y = p.y - eset->tileset.tile_h_half;
				r.w = eset->tileset.tile_w;
				r.h = eset->tileset.tile_h;

				// clicked in pickup hotspot?
				if ((it->tip_visible && isWithinRect(it->tip_bounds, mouse)) || isWithinRect(r, mouse)) {
					curs->setCursor(CursorManager::CURSOR_INTERACT);
					if (inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
						inpt->lock[Input::MAIN1] = true;
						if (!it->stack.empty()) {
							loot_stack = it->stack;
							it = loot.erase(it);
							return loot_stack;
						}
					}
				}
			}
		}
	}

	// check pressing Enter/Return
	if (inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
		loot_stack = checkNearestPickup(hero_pos);
		if (!loot_stack.empty()) {
			inpt->lock[Input::ACCEPT] = true;
		}
	}

	return loot_stack;
}

/**
 * Autopickup loot if enabled in the engine
 * Currently, only currency is checked for autopickup
 */
ItemStack LootManager::checkAutoPickup(const FPoint& hero_pos) {
	ItemStack loot_stack;

	std::vector<Loot>::iterator it;
	for (it = loot.end(); it != loot.begin(); ) {
		--it;
		if (!it->dropped_by_hero && fabs(hero_pos.x - it->pos.x) < eset->loot.autopickup_range && fabs(hero_pos.y - it->pos.y) < eset->loot.autopickup_range && !it->isFlying()) {
			if (it->stack.item == eset->misc.currency_id && eset->loot.autopickup_currency) {
				loot_stack = it->stack;
				it = loot.erase(it);
				return loot_stack;
			}
		}
	}
	return loot_stack;
}

ItemStack LootManager::checkNearestPickup(const FPoint& hero_pos) {
	ItemStack loot_stack;

	float best_distance = std::numeric_limits<float>::max();

	std::vector<Loot>::iterator it;
	std::vector<Loot>::iterator nearest = loot.end();

	for (it = loot.end(); it != loot.begin(); ) {
		--it;

		float distance = calcDist(hero_pos, it->pos);
		if (distance < eset->misc.interact_range && distance < best_distance) {
			best_distance = distance;
			nearest = it;
		}
	}

	if (nearest != loot.end() && !nearest->stack.empty()) {
		loot_stack = nearest->stack;
		loot.erase(nearest);
		return loot_stack;
	}

	return loot_stack;
}

void LootManager::addRenders(std::vector<Renderable> &ren, std::vector<Renderable> &ren_dead) {
	std::vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ++it) {
		if (it->animation) {
			Renderable r = it->animation->getCurrentFrame(0);
			r.map_pos.x = it->pos.x;
			r.map_pos.y = it->pos.y;

			(it->animation->isLastFrame() ? ren_dead : ren).push_back(r);
		}
	}
}

void LootManager::parseLoot(std::string &val, Event_Component *e, std::vector<Event_Component> *ec_list) {
	if (e == NULL) return;

	std::string chance;
	bool first_is_filename = false;
	e->s = popFirstString(val);

	if (e->s == "currency")
		e->c = eset->misc.currency_id;
	else if (toInt(e->s, -1) != -1)
		e->c = toInt(e->s);
	else if (ec_list) {
		// load entire loot table
		std::string filename = e->s;

		// remove the last event component, since getLootTable() will create a new one
		if (e == &ec_list->back())
			ec_list->pop_back();

		getLootTable(filename, ec_list);
		first_is_filename = true;
	}

	if (!first_is_filename) {
		// make sure the type is "loot"
		e->type = EC_LOOT;

		// drop chance
		chance = popFirstString(val);
		if (chance == "fixed") e->z = 0;
		else e->z = toInt(chance);

		// quantity min/max
		e->a = std::max(popFirstInt(val), 1);
		e->b = std::max(popFirstInt(val), e->a);
	}

	// add repeating loot
	if (ec_list) {
		std::string repeat_val = popFirstString(val);
		while (repeat_val != "") {
			ec_list->push_back(Event_Component());
			Event_Component *ec = &ec_list->back();
			ec->type = EC_LOOT;

			ec->s = repeat_val;
			if (ec->s == "currency")
				ec->c = eset->misc.currency_id;
			else if (toInt(ec->s, -1) != -1)
				ec->c = toInt(ec->s);
			else {
				// remove the last event component, since getLootTable() will create a new one
				ec_list->pop_back();

				getLootTable(repeat_val, ec_list);

				repeat_val = popFirstString(val);
				continue;
			}

			chance = popFirstString(val);
			if (chance == "fixed") ec->z = 0;
			else ec->z = toInt(chance);

			ec->a = std::max(popFirstInt(val), 1);
			ec->b = std::max(popFirstInt(val), ec->a);

			repeat_val = popFirstString(val);
		}
	}
}

void LootManager::loadLootTables() {
	std::vector<std::string> filenames = mods->list("loot", !ModManager::LIST_FULL_PATHS);

	for (unsigned i=0; i<filenames.size(); i++) {
		FileParser infile;
		if (!infile.open(filenames[i], FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
			continue;

		std::vector<Event_Component> *ec_list = &loot_tables[filenames[i]];
		Event_Component *ec = NULL;
		bool skip_to_next = false;

		while (infile.next()) {
			if (infile.section == "") {
				if (infile.key == "loot") {
					ec_list->push_back(Event_Component());
					ec = &ec_list->back();
					parseLoot(infile.val, ec, ec_list);
				}
			}
			else if (infile.section == "loot") {
				if (infile.new_section) {
					ec_list->push_back(Event_Component());
					ec = &ec_list->back();
					ec->type = EC_LOOT;
					skip_to_next = false;
				}

				if (skip_to_next || ec == NULL)
					continue;

				if (infile.key == "id") {
					ec->s = infile.val;

					if (ec->s == "currency")
						ec->c = eset->misc.currency_id;
					else if (toInt(ec->s, -1) != -1)
						ec->c = toInt(ec->s);
					else {
						skip_to_next = true;
						infile.error("LootManager: Invalid item id for loot.");
					}
				}
				else if (infile.key == "chance") {
					if (infile.val == "fixed")
						ec->z = 0;
					else
						ec->z = toInt(infile.val);
				}
				else if (infile.key == "quantity") {
					ec->a = std::max(popFirstInt(infile.val), 1);
					ec->b = std::max(popFirstInt(infile.val), ec->a);
				}
			}
		}

		infile.close();
	}
}

void LootManager::getLootTable(const std::string &filename, std::vector<Event_Component> *ec_list) {
	if (!ec_list)
		return;

	std::map<std::string, std::vector<Event_Component> >::iterator it;
	for (it = loot_tables.begin(); it != loot_tables.end(); ++it) {
		if (it->first == filename) {
			std::vector<Event_Component> *loot_defs = &it->second;
			for (unsigned i=0; i<loot_defs->size(); ++i) {
				ec_list->push_back((*loot_defs)[i]);
			}
			break;
		}
	}
}

LootManager::~LootManager() {
	// remove all items in the item database
	for (unsigned int i=0; i < items->items.size(); i++) {
		if (items->items[i].loot_animation.empty()) continue;

		for (unsigned int j=0; j<items->items[i].loot_animation.size(); j++) {
			anim->decreaseCount(items->items[i].loot_animation[j].name);
			delete animations[i][j];
		}
	}

	// remove items, so Loots get destroyed!
	loot.clear();

	anim->cleanUp();

	snd->unload(sfx_loot);

	delete tip;
}
