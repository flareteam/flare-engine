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
#include "Avatar.h"
#include "CampaignManager.h"
#include "CommonIncludes.h"
#include "CursorManager.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FogOfWar.h"
#include "InputState.h"
#include "LootManager.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetTooltip.h"

#include <limits>

LootManager::LootManager()
	: sfx_loot(snd->load(eset->loot.sfx_loot, "LootManager dropping loot"))
	, sfx_loot_channel("loot")
{
	loadGraphics();
	loadLootTables();
}

/**
 * The "loot" variable on each item refers to the "flying loot" animation for that item.
 * Here we load all the animations used by the item database.
 */
void LootManager::loadGraphics() {
	if (!animations.empty()) {
		Utils::logError("LootManger: loadGraphics() detected existing animations, aborting.");
		return;
	}

	animations.resize(items->items.size(), NULL);

	// check all items in the item database
	for (size_t i = 1; i < items->items.size(); ++i) {
		Item* item = items->items[i];

		if (!item || item->loot_animation.empty())
			continue;

		animations[i] = new std::vector<Animation*>(item->loot_animation.size(), NULL);

		for (size_t j = 0; j < item->loot_animation.size(); ++j) {
			anim->increaseCount(item->loot_animation[j].name);
			(*animations[i])[j] = anim->getAnimationSet(item->loot_animation[j].name)->getAnimation("");
		}
	}
}

void LootManager::handleNewMap() {
	loot.clear();
	enemiesDroppingLoot.clear();
}

void LootManager::logic() {
	if (inpt->pressing[Input::LOOT_TOOLTIP_MODE] && !inpt->lock[Input::LOOT_TOOLTIP_MODE]) {
		inpt->lock[Input::LOOT_TOOLTIP_MODE] = true;
		settings->loot_tooltips++;
		if (settings->loot_tooltips > Settings::LOOT_TIPS_HIDE_ALL)
			settings->loot_tooltips = Settings::LOOT_TIPS_DEFAULT;

		if (settings->loot_tooltips == Settings::LOOT_TIPS_HIDE_ALL)
			pc->logMsg(msg->get("Loot tooltip visibility") + ": " + msg->get("Hidden"), Avatar::MSG_UNIQUE);
		else if (settings->loot_tooltips == Settings::LOOT_TIPS_DEFAULT)
			pc->logMsg(msg->get("Loot tooltip visibility") + ": " + msg->get("Default"), Avatar::MSG_UNIQUE);
		else if (settings->loot_tooltips == Settings::LOOT_TIPS_SHOW_ALL)
			pc->logMsg(msg->get("Loot tooltip visibility") + ": " + msg->get("Show All"), Avatar::MSG_UNIQUE);
	}

	std::vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ++it) {

		// animate flying loot
		if (it->animation) {
			it->animation->advanceFrame();
			if (!it->on_ground && it->animation->isSecondLastFrame()) {
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
	if (!settings->show_hud) return;

	Point dest;
	bool tooltip_below = true;
	Rect screen_rect(0, 0, settings->view_w, settings->view_h);

	std::vector<Loot>::iterator it;
	for (it = loot.begin(); it != loot.end(); ) {
		it->tip_visible = false;

		if (it->on_ground) {
			if (mapr->fogofwar > FogOfWar::TYPE_MINIMAP) {
				float delta = Utils::calcDist(pc->stats.pos, it->pos);
				if (delta > fow->mask_radius-1.0) {
					break;
				}
			}

			Point p = Utils::mapToScreen(it->pos.x, it->pos.y, cam.x, cam.y);
			if (!Utils::isWithinRect(screen_rect, p)) {
				++it;
				continue;
			}

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

			bool forced_visibility = false;
			bool default_visibility = true;

			if (settings->loot_tooltips == Settings::LOOT_TIPS_DEFAULT && eset->loot.hide_radius > 0) {
				if (Utils::calcDist(pc->stats.pos, it->pos) < eset->loot.hide_radius) {
					default_visibility = false;
				}
				else {
					Entity* test_enemy = entitym->getNearestEntity(it->pos, !EntityManager::GET_CORPSE, NULL, eset->loot.hide_radius);
					if (test_enemy) {
						default_visibility = false;
					}
				}
			}
			else if (settings->loot_tooltips == Settings::LOOT_TIPS_HIDE_ALL) {
				default_visibility = false;
			}
			else if (settings->loot_tooltips == Settings::LOOT_TIPS_SHOW_ALL && inpt->pressing[Input::ALT]) {
				default_visibility = false;
			}

			if (!default_visibility)
				forced_visibility = Utils::isWithinRect(hover, inpt->mouse) || (inpt->pressing[Input::ALT] && settings->loot_tooltips != Settings::LOOT_TIPS_SHOW_ALL);

			if (default_visibility || forced_visibility) {
				it->tip_visible = true;

				// create tooltip data if needed
				if (it->tip.isEmpty()) {
					if (!it->stack.empty()) {
						it->tip = items->getShortTooltip(it->stack);
					}
				}

				// try to prevent tooltips from overlapping
				it->wtip->prerender(it->tip, dest, TooltipData::STYLE_TOPLABEL);
				std::vector<Loot>::iterator test_it;
				for (test_it = loot.begin(); test_it != it; ) {
					if (test_it->tip_visible && Utils::rectsOverlap(test_it->wtip->bounds, it->wtip->bounds)) {
						if (tooltip_below)
							dest.y = test_it->wtip->bounds.y + test_it->wtip->bounds.h + eset->tooltips.offset;
						else
							dest.y = test_it->wtip->bounds.y - test_it->wtip->bounds.h + eset->tooltips.offset;

						it->wtip->bounds.y = dest.y;
					}

					++test_it;
				}

				it->wtip->render(it->tip, dest, TooltipData::STYLE_TOPLABEL);

				if (settings->loot_tooltips == Settings::LOOT_TIPS_HIDE_ALL && !inpt->pressing[Input::ALT])
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
		StatBlock *e = enemiesDroppingLoot[i];

		if (!e)
			continue;

		if (e->quest_loot_id != 0) {
			// quest loot
			std::vector<EventComponent> quest_loot_table;
			EventComponent ec;
			ec.type = EventComponent::LOOT;
			ec.id = e->quest_loot_id;
			ec.data[LOOT_EC_QUANTITY_MIN].Int = 1;
			ec.data[LOOT_EC_QUANTITY_MAX].Int = 1;
			ec.data[LOOT_EC_CHANCE].Float = 0; // "fixed" chance

			quest_loot_table.push_back(ec);
			checkLoot(quest_loot_table, &e->pos, NULL);
		}

		if (!e->loot_table.empty()) {
			unsigned drops;
			if (e->loot_count.y != 0) {
				drops = Math::randBetween(e->loot_count.x, e->loot_count.y);
			}
			else {
				drops = Math::randBetween(1, eset->loot.drop_max);
			}

			for (unsigned j=0; j<drops; ++j) {
				checkLoot(e->loot_table, &e->pos, NULL);
			}

			e->loot_table.clear();
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
			drops = Math::randBetween(mapr->loot_count.x, mapr->loot_count.y);
		}
		else {
			drops = Math::randBetween(1, eset->loot.drop_max);
		}

		for (unsigned i=0; i<drops; ++i) {
			checkLoot(mapr->loot, NULL, NULL);
		}

		mapr->loot.clear();
		mapr->loot_count.x = 0;
		mapr->loot_count.y = 0;
	}
}

void LootManager::addEnemyLoot(StatBlock *e) {
	enemiesDroppingLoot.push_back(e);
}

void LootManager::checkLoot(std::vector<EventComponent> &loot_table, FPoint *pos, std::vector<ItemStack> *itemstack_vec) {
	FPoint p;
	EventComponent *ec;
	ItemStack new_loot;
	std::vector<EventComponent*> possible_ids;

	float chance = Math::randBetweenF(0,100);

	// first drop any 'fixed' (0% chance) items
	for (size_t i = loot_table.size(); i > 0; i--) {
		ec = &loot_table[i-1];

		if (ec->data[LOOT_EC_TYPE].Int == LOOT_EC_TYPE_TABLE)
			continue;

		if (ec->data[LOOT_EC_CHANCE].Float == 0) {
			if (ec->status == 0 || (ec->status > 0 && camp->checkStatus(ec->status))) {
				checkLootComponent(ec, pos, itemstack_vec);
			}
			loot_table.erase(loot_table.begin()+i-1);
		}
	}

	// now pick up to 1 random item to drop
	float threshold = static_cast<float>(pc->stats.get(Stats::ITEM_FIND) + 100);
	for (unsigned i = 0; i < loot_table.size(); i++) {
		ec = &loot_table[i];

		if (ec->data[LOOT_EC_TYPE].Int == LOOT_EC_TYPE_TABLE)
			continue;

		float real_chance = ec->data[LOOT_EC_CHANCE].Float;

		if (ec->id != 0) {
			real_chance = real_chance * (pc->stats.get(Stats::ITEM_FIND) + 100.f) / 100.f;
		}

		bool level_check = true;
		if (ec->data[LOOT_EC_REQUIRES_LEVEL_MIN].Int > 0 && ec->data[LOOT_EC_REQUIRES_LEVEL_MAX].Int > 0) {
			level_check = pc->stats.level >= ec->data[LOOT_EC_REQUIRES_LEVEL_MIN].Int && pc->stats.level <= ec->data[LOOT_EC_REQUIRES_LEVEL_MAX].Int;
		}

		if (real_chance >= chance && level_check && (ec->status == 0 || (ec->status > 0 && camp->checkStatus(ec->status)))) {
			if (real_chance <= threshold) {
				if (real_chance != threshold) {
					possible_ids.clear();
				}

				threshold = real_chance;
			}

			if (chance <= threshold && ec->data[LOOT_EC_MAX_DROPS].Int != 0) {
				possible_ids.push_back(ec);
			}
		}
	}

	if (!possible_ids.empty()) {
		// if there was more than one item with the same chance, randomly pick one of them
		size_t chosen_loot = static_cast<size_t>(rand()) % possible_ids.size();
		ec = possible_ids[chosen_loot];
		checkLootComponent(ec, pos, itemstack_vec);

		if (ec->data[LOOT_EC_MAX_DROPS].Int > 0) {
			ec->data[LOOT_EC_MAX_DROPS].Int--;
		}
	}
}

void LootManager::addLoot(ItemStack stack, const FPoint& pos, bool dropped_by_hero) {
	if (stack.empty() || !items->isValid(stack.item))
		return;

	Loot ld;
	ld.stack = stack;
	ld.pos.x = pos.x;
	ld.pos.y = pos.y;
	ld.pos.align(); // prevent "rounding jitter"
	ld.dropped_by_hero = dropped_by_hero;

	// merge stacks that have the same item id and position
	std::vector<Loot>::iterator it;
	for (it = loot.end(); it != loot.begin(); ) {
		--it;
		if (it->stack.item == ld.stack.item && it->pos.x == ld.pos.x && it->pos.y == ld.pos.y) {
			it->stack.quantity += ld.stack.quantity;
			it->tip.clear();
			snd->play(sfx_loot, sfx_loot_channel, pos, false);
			return;
		}
	}

	if (!items->items[stack.item]->loot_animation.empty()) {
		size_t index = items->items[stack.item]->loot_animation.size()-1;

		for (size_t i = 0; i < items->items[stack.item]->loot_animation.size(); ++i) {
			int low = items->items[stack.item]->loot_animation[i].low;
			int high = items->items[stack.item]->loot_animation[i].high;
			if (stack.quantity >= low && (stack.quantity <= high || high == 0)) {
				index = i;
				break;
			}
		}
		ld.loadAnimation(items->items[stack.item]->loot_animation[index].name);
	}
	else {
		// immediately place the loot on the ground if there's no animation
		ld.on_ground = true;
	}

	loot.push_back(ld);
	snd->play(sfx_loot, sfx_loot_channel, pos, false);
}

/**
 * Click on the map to pick up loot.  We need the camera position to translate
 * screen coordinates to map locations.
 */
ItemStack LootManager::checkPickup(const Point& mouse, const FPoint& cam, const FPoint& hero_pos) {
	ItemStack loot_stack;

	// check left mouse click
	if (inpt->usingMouse()) {
		Point mouse_pos = mouse;

		// we may have targeted a once distant piece of loot while using mouse-move
		// if so, we want to automatically interact with it without requiring any mouse clicks
		bool mouse_move_target = pc->mm_target_object == Avatar::MM_TARGET_LOOT && pc->isNearMMtarget();
		if (mouse_move_target && (pc->stats.cur_state == StatBlock::ENTITY_STANCE || pc->stats.cur_state == StatBlock::ENTITY_MOVE)) {
			pc->stats.cur_state = StatBlock::ENTITY_STANCE;
			mouse_pos = Utils::mapToScreen(pc->mm_target_object_pos.x, pc->mm_target_object_pos.y, cam.x, cam.y);
		}
		else if (pc->mm_target_object == Avatar::MM_TARGET_LOOT && pc->stats.cur_state == StatBlock::ENTITY_STANCE) {
			pc->stats.cur_state = StatBlock::ENTITY_MOVE;
		}

		// I'm starting at the end of the loot list so that more recently-dropped
		// loot is picked up first.  If a player drops several loot in the same
		// location, picking it back up will work like a stack.
		std::vector<Loot>::iterator it, it_match = loot.end();
		for (it = loot.end(); it != loot.begin(); ) {
			--it;

			// only loot on the ground can be picked up
			if (!it->isFlying()) {
				Point p = Utils::mapToScreen(it->pos.x, it->pos.y, cam.x, cam.y);

				Rect r;
				r.x = p.x - eset->tileset.tile_w_half;
				r.y = p.y - eset->tileset.tile_h_half;
				r.w = eset->tileset.tile_w;
				r.h = eset->tileset.tile_h;

				if (it->tip_visible && Utils::isWithinRect(it->wtip->bounds, mouse_pos)) {
					it_match = it;

					// tooltips take priority over hotspots, so we can jump out here if we clicked on a tooltip
					break;
				}
				else if (it_match == loot.end() && Utils::isWithinRect(r, mouse_pos)) {
					it_match = it;
				}
			}
		}

		int interact_key = (settings->mouse_move && settings->mouse_move_swap) ? Input::MAIN2 : Input::MAIN1;

		if (it_match != loot.end() && !it_match->stack.empty()) {
			if (Utils::calcDist(hero_pos, it_match->pos) < eset->misc.interact_range) {
				curs->setCursor(CursorManager::CURSOR_INTERACT);

				if (!mouse_move_target && inpt->pressing[interact_key] && !inpt->lock[interact_key]) {
					inpt->lock[interact_key] = true;
					loot_stack = it_match->stack;
					loot.erase(it_match);
					return loot_stack;
				}
				else if (mouse_move_target) {
					pc->mm_target_object = Avatar::MM_TARGET_NONE;
					loot_stack = it_match->stack;
					loot.erase(it_match);
					return loot_stack;
				}
			}
			else if (settings->mouse_move) {
				curs->setCursor(CursorManager::CURSOR_INTERACT);

				if (inpt->pressing[interact_key] && !inpt->lock[interact_key]) {
					// loot is out of range, but we're clicking on it. For mouse-move, we'll set this as the desired target
					inpt->lock[interact_key] = true;

					pc->setDesiredMMTarget(it_match->pos);

					pc->mm_target_object = Avatar::MM_TARGET_LOOT;
					pc->mm_target_object_pos = it_match->pos;
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

	// if auto-loot is disabled, just return an empty stack
	if (settings->auto_loot == 0)
		return loot_stack;

	std::vector<Loot>::iterator it;
	for (it = loot.end(); it != loot.begin(); ) {
		--it;
		if (!it->dropped_by_hero && Utils::calcDist(hero_pos, it->pos) < eset->loot.autopickup_range && !it->isFlying()) {
			bool do_pickup = false;
			if (it->stack.item == eset->misc.currency_id && eset->loot.autopickup_currency) {
				do_pickup = true;
			}
			else if (settings->auto_loot == 1 && items->checkAutoPickup(it->stack.item)) {
				do_pickup = true;
			}

			if (do_pickup) {
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

		float distance = Utils::calcDist(hero_pos, it->pos);
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
		if (mapr->fogofwar > FogOfWar::TYPE_MINIMAP) {
			float delta = Utils::calcDist(pc->stats.pos, it->pos);
			if (delta > fow->mask_radius-1.0) {
				continue;
			}
		}

		if (it->animation) {
			Renderable r = it->animation->getCurrentFrame(0);
			r.map_pos.x = it->pos.x;
			r.map_pos.y = it->pos.y;

			(it->animation->isLastFrame() ? ren_dead : ren).push_back(r);
		}
	}
}

void LootManager::parseLoot(std::string &val, EventComponent *e, std::vector<EventComponent> *ec_list) {
	if (e == NULL) return;

	std::string chance;
	bool first_is_filename = false;
	e->s = Parse::popFirstString(val);

	// store loot_ec_type, because we may overwrite in the case of the first loot entry being a table file
	int loot_ec_type = e->data[LOOT_EC_TYPE].Int;

	if (e->s == "currency")
		e->id = eset->misc.currency_id;
	else if (Parse::toInt(e->s, -1) != -1)
		e->id = items->verifyID(Parse::toItemID(e->s), NULL, !ItemManager::VERIFY_ALLOW_ZERO, !ItemManager::VERIFY_ALLOCATE);
	else if (ec_list) {
		// load entire loot table
		e->data[LOOT_EC_TYPE].Int = LOOT_EC_TYPE_TABLE;

		getLootTable(e->s, ec_list);
		first_is_filename = true;
	}

	if (!first_is_filename) {
		// make sure the type is "loot"
		e->type = EventComponent::LOOT;

		// drop chance
		chance = Parse::popFirstString(val);
		if (chance == "fixed")
			e->data[LOOT_EC_CHANCE].Float = 0;
		else
			e->data[LOOT_EC_CHANCE].Float = Parse::toFloat(chance);

		// quantity min/max
		e->data[LOOT_EC_QUANTITY_MIN].Int = std::max(Parse::popFirstInt(val), 1);
		e->data[LOOT_EC_QUANTITY_MAX].Int = std::max(Parse::popFirstInt(val), e->data[LOOT_EC_QUANTITY_MIN].Int);

		if (items->isValid(e->id))
			e->data[LOOT_EC_MAX_DROPS].Int = items->items[e->id]->loot_drops_max;
	}

	// add repeating loot
	if (ec_list) {
		std::string repeat_val = Parse::popFirstString(val);
		while (repeat_val != "") {
			ec_list->push_back(EventComponent());
			EventComponent *ec = &ec_list->at(ec_list->size()-1);
			ec->type = EventComponent::LOOT;
			ec->data[LOOT_EC_TYPE].Int = loot_ec_type;

			ec->s = repeat_val;
			if (ec->s == "currency")
				ec->id = eset->misc.currency_id;
			else if (Parse::toInt(ec->s, -1) != -1)
				ec->id = items->verifyID(Parse::toItemID(ec->s), NULL, !ItemManager::VERIFY_ALLOW_ZERO, !ItemManager::VERIFY_ALLOCATE);
			else {
				ec->data[LOOT_EC_TYPE].Int = LOOT_EC_TYPE_TABLE;

				getLootTable(repeat_val, ec_list);

				repeat_val = Parse::popFirstString(val);
				continue;
			}

			chance = Parse::popFirstString(val);
			if (chance == "fixed")
				ec->data[LOOT_EC_CHANCE].Float = 0;
			else
				ec->data[LOOT_EC_CHANCE].Float = Parse::toFloat(chance);

			ec->data[LOOT_EC_QUANTITY_MIN].Int = std::max(Parse::popFirstInt(val), 1);
			ec->data[LOOT_EC_QUANTITY_MAX].Int = std::max(Parse::popFirstInt(val), ec->data[LOOT_EC_QUANTITY_MIN].Int);

			if (items->isValid(ec->id))
				ec->data[LOOT_EC_MAX_DROPS].Int = items->items[ec->id]->loot_drops_max;

			repeat_val = Parse::popFirstString(val);
		}
	}
}

void LootManager::loadLootTables() {
	std::vector<std::string> filenames = mods->list("loot", !ModManager::LIST_FULL_PATHS);

	// @CLASS LootManger|Description of loot tables in loot/
	for (unsigned i=0; i<filenames.size(); i++) {
		FileParser infile;
		if (!infile.open(filenames[i], FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
			continue;

		std::vector<EventComponent> *ec_list = &loot_tables[filenames[i]];
		EventComponent *ec = NULL;
		bool skip_to_next = false;

		while (infile.next()) {
			if (infile.section == "") {
				// @ATTR loot|loot|Compact form of defining a loot table entry.
				if (infile.key == "loot") {
					ec_list->push_back(EventComponent());
					ec = &ec_list->at(ec_list->size()-1);
					ec->data[LOOT_EC_TYPE].Int = LOOT_EC_TYPE_TABLE_ROW;
					parseLoot(infile.val, ec, ec_list);
				}
				// @ATTR status_loot|string, loot : Required status, Loot definition|Compact form of defining a loot table entry with a required campaign status.
				else if (infile.key == "status_loot") {
					ec_list->push_back(EventComponent());
					ec = &ec_list->at(ec_list->size()-1);
					ec->data[LOOT_EC_TYPE].Int = LOOT_EC_TYPE_TABLE_ROW;
					ec->status = camp->registerStatus(Parse::popFirstString(infile.val));
					parseLoot(infile.val, ec, ec_list);
				}
			}
			else if (infile.section == "loot") {
				if (infile.new_section) {
					ec_list->push_back(EventComponent());
					ec = &ec_list->at(ec_list->size()-1);
					ec->type = EventComponent::LOOT;
					ec->data[LOOT_EC_TYPE].Int = LOOT_EC_TYPE_TABLE_ROW;
					skip_to_next = false;
				}

				if (skip_to_next || ec == NULL)
					continue;

				// @ATTR loot.id|[item_id, "currency"]|The ID of the loot item. "currency" will use the item ID defined as currency_id in engine/misc.txt.
				if (infile.key == "id") {
					ec->s = infile.val;

					if (ec->s == "currency")
						ec->id = eset->misc.currency_id;
					else if (Parse::toInt(ec->s, -1) != -1)
						ec->id = Parse::toItemID(ec->s);
					else {
						skip_to_next = true;
						infile.error("LootManager: Invalid item id for loot.");
					}

					if (!skip_to_next) {
						ec->data[LOOT_EC_MAX_DROPS].Int = items->items[ec->id]->loot_drops_max;
					}
				}
				// @ATTR loot.chance|[float, "fixed"]|The chance that the item will drop. "fixed" will drop the item no matter what before the random items are picked. This is different than setting a chance of 100, in which the item could be replaced with another random item.
				else if (infile.key == "chance") {
					if (infile.val == "fixed")
						ec->data[LOOT_EC_CHANCE].Float = 0;
					else
						ec->data[LOOT_EC_CHANCE].Float = Parse::toFloat(infile.val);
				}
				// @ATTR loot.quantity|int, int : Min quantity, Max quantity (optional)|The quantity of item in the dropped loot stack.
				else if (infile.key == "quantity") {
					ec->data[LOOT_EC_QUANTITY_MIN].Int = std::max(Parse::popFirstInt(infile.val), 1);
					ec->data[LOOT_EC_QUANTITY_MAX].Int = std::max(Parse::popFirstInt(infile.val), ec->data[LOOT_EC_QUANTITY_MIN].Int);
				}
				// @ATTR loot.requires_status|string|A single campaign status that is required for the item to be able to drop.
				else if (infile.key == "requires_status") {
					ec->status = camp->registerStatus(Parse::popFirstString(infile.val));
				}
				// @ATTR loot.requires_level|int, int : Min player level, Max player level|The player's level must be in this range for the item to drop.
				else if (infile.key == "requires_level") {
					ec->data[LOOT_EC_REQUIRES_LEVEL_MIN].Int = std::max(Parse::popFirstInt(infile.val), 0);
					ec->data[LOOT_EC_REQUIRES_LEVEL_MAX].Int = std::max(Parse::popFirstInt(infile.val), ec->data[LOOT_EC_REQUIRES_LEVEL_MIN].Int);
				}
				// @ATTR loot.quantity_per_level|int, int: Min quantity, Max quantity|Multiplied by the player level minus one and added on top of 'quantity'.
				else if (infile.key == "quantity_per_level") {
					ec->data[LOOT_EC_QUANTITY_PER_LEVEL_MIN].Int = std::max(Parse::popFirstInt(infile.val), 0);
					ec->data[LOOT_EC_QUANTITY_PER_LEVEL_MAX].Int = std::max(Parse::popFirstInt(infile.val), ec->data[LOOT_EC_QUANTITY_PER_LEVEL_MIN].Int);
				}
			}
		}

		infile.close();
	}
}

void LootManager::getLootTable(const std::string &filename, std::vector<EventComponent> *ec_list) {
	if (!ec_list)
		return;

	std::map<std::string, std::vector<EventComponent> >::iterator it;
	for (it = loot_tables.begin(); it != loot_tables.end(); ++it) {
		if (it->first == Filesystem::convertSlashes(filename)) {
			std::vector<EventComponent> *loot_defs = &it->second;
			for (unsigned i=0; i<loot_defs->size(); ++i) {
				ec_list->push_back((*loot_defs)[i]);
			}
			break;
		}
	}
}

void LootManager::checkLootComponent(EventComponent* ec, FPoint *pos, std::vector<ItemStack> *itemstack_vec) {
	FPoint p;
	ItemStack new_loot;
	Point src;

	if (pos) {
		src = Point(*pos);
	}
	else {
		src.x = ec->data[LOOT_EC_POSX].Int;
		src.y = ec->data[LOOT_EC_POSY].Int;
	}
	p.x = static_cast<float>(src.x) + 0.5f;
	p.y = static_cast<float>(src.y) + 0.5f;

	if (!mapr->collider.isValidPosition(p.x, p.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_TYPE_ALL_ENTITIES)) {
		p = mapr->collider.getRandomNeighbor(src, eset->loot.drop_radius, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_TYPE_ALL_ENTITIES);

		if (!mapr->collider.isValidPosition(p.x, p.y, MapCollision::MOVE_NORMAL, MapCollision::COLLIDE_TYPE_ALL_ENTITIES)) {
			p = pc->stats.pos;
		}
		else {
			if (src.x == static_cast<int>(p.x) && src.y == static_cast<int>(p.y))
				p = pc->stats.pos;

			mapr->collider.block(p.x, p.y, !MapCollision::IS_ALLY);
			tiles_to_unblock.push_back(Point(p));
		}
	}

	std::vector<ItemStack> ex_stacks;

	int quantity_min = ec->data[LOOT_EC_QUANTITY_MIN].Int + ((pc->stats.level-1) * ec->data[LOOT_EC_QUANTITY_PER_LEVEL_MIN].Int);
	int quantity_max = ec->data[LOOT_EC_QUANTITY_MAX].Int + ((pc->stats.level-1) * ec->data[LOOT_EC_QUANTITY_PER_LEVEL_MAX].Int);

	new_loot.quantity = Math::randBetween(quantity_min, quantity_max);

	// an item id of 0 means we should drop currency instead
	if (ec->id == 0 || ec->id == eset->misc.currency_id) {
		new_loot.item = eset->misc.currency_id;
		new_loot.quantity = static_cast<int>(static_cast<float>(new_loot.quantity) * (100 + pc->stats.get(Stats::CURRENCY_FIND)) / 100);
		ex_stacks.push_back(new_loot);
	}
	else {
		new_loot.item = ec->id;
		items->getExtendedStacks(new_loot.item, new_loot.quantity, ex_stacks);
	}

	for (size_t i = 0; i < ex_stacks.size(); ++i) {
		if (itemstack_vec)
			itemstack_vec->push_back(ex_stacks[i]);
		else
			addLoot(ex_stacks[i], p, !DROPPED_BY_HERO);
	}
}

void LootManager::removeFromEnemiesDroppingLoot(const StatBlock* sb) {
	for (size_t i = enemiesDroppingLoot.size(); i > 0; i--) {
		// enemies will actually be removed the next time checkEnemiesForLoot() runs
		if (enemiesDroppingLoot[i-1] == sb)
			enemiesDroppingLoot[i-1] = NULL;
	}
}

LootManager::~LootManager() {
	// remove all items in the item database
	for (size_t i = 0; i < animations.size(); ++i) {
		if (!animations[i])
			continue;

		if (items->isValid(i)) {
			for (size_t j = 0; j < items->items[i]->loot_animation.size(); ++j) {
				anim->decreaseCount(items->items[i]->loot_animation[j].name);
				delete (*animations[i])[j];
			}
		}
		delete animations[i];
	}

	// remove items, so Loots get destroyed!
	loot.clear();

	anim->cleanUp();

	snd->unload(sfx_loot);
}
