/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2013 Henrik Andersson
Copyright © 2012 Stefan Beller
Copyright © 2012-2015 Justin Jacobs

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
 * class NPC
 */

#include "NPC.h"

#include "Animation.h"
#include "AnimationSet.h"
#include "AnimationManager.h"
#include "CampaignManager.h"
#include "FileParser.h"
#include "ItemManager.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"
#include "UtilsMath.h"

NPC::NPC()
	: Entity()
	, name("")
	, gfx("")
	, pos()
	, direction(0)
	, portrait(NULL)
	, talker(false)
	, vendor(false)
	, stock()
	, stock_count(0)
	, vox_intro()
	, vox_quests()
	, dialog() {
	stock.init(NPC_VENDOR_MAX_STOCK);
}

/**
 * NPCs are stored in simple config files
 *
 * @param npc_id Config file for npc
 */
void NPC::load(const std::string& npc_id) {

	FileParser infile;
	ItemStack stack;

	std::string filename_portrait = "";

	bool clear_random_table = true;

	// @CLASS NPC|Description of NPCs in npcs/
	if (infile.open(npc_id)) {
		while (infile.next()) {
			if (infile.section == "dialog") {
				if (infile.new_section) {
					dialog.push_back(std::vector<Event_Component>());
				}
				Event_Component e;
				e.type = EC_NONE;
				if (infile.key == "him" || infile.key == "her") {
					// @ATTR dialog.him|repeatable(string)|A line of dialog from the NPC.
					// @ATTR dialog.her|repeatable(string)|A line of dialog from the NPC.
					e.type = EC_NPC_DIALOG_THEM;
					e.s = msg->get(infile.val);
				}
				else if (infile.key == "you") {
					// @ATTR dialog.you|repeatable(string)|A line of dialog from the player.
					e.type = EC_NPC_DIALOG_YOU;
					e.s = msg->get(infile.val);
				}
				else if (infile.key == "voice") {
					// @ATTR dialog.voice|repeatable(string)|Filename of a voice sound file to play.
					e.type = EC_NPC_VOICE;
					e.x = loadSound(infile.val, NPC_VOX_QUEST);
				}
				else if (infile.key == "topic") {
					// @ATTR dialog.topic|string|The name of this dialog topic. Displayed when picking a dialog tree.
					e.type = EC_NPC_DIALOG_TOPIC;
					e.s = msg->get(infile.val);
				}
				else if (infile.key == "group") {
					// @ATTR dialog.group|string|Dialog group.
					e.type = EC_NPC_DIALOG_GROUP;
					e.s = infile.val;
				}
				else if (infile.key == "allow_movement") {
					// @ATTR dialog.allow_movement|bool|Restrict the player's mvoement during dialog.
					e.type = EC_NPC_ALLOW_MOVEMENT;
					e.s = infile.val;
				}
				else {
					Event ev;
					EventManager::loadEventComponent(infile, &ev, NULL);

					for (size_t i=0; i<ev.components.size(); ++i) {
						if (ev.components[i].type != EC_NONE) {
							dialog.back().push_back(ev.components[i]);
						}
					}
				}

				if (e.type != EC_NONE) {
					dialog.back().push_back(e);
				}
			}
			else {
				filename = npc_id;

				if (infile.new_section) {
					// APPENDed file
					clear_random_table = true;
				}

				if (infile.key == "name") {
					// @ATTR name|string|NPC's name.
					name = msg->get(infile.val);
				}
				else if (infile.key == "gfx") {
					// @ATTR gfx|filename|Filename of an animation definition.
					gfx = infile.val;
				}

				// handle talkers
				else if (infile.key == "talker") {
					// @ATTR talker|bool|Allows this NPC to be talked to.
					talker = toBool(infile.val);
				}
				else if (infile.key == "portrait") {
					// @ATTR portrait|filename|Filename of a portrait image.
					filename_portrait = infile.val;
				}

				// handle vendors
				else if (infile.key == "vendor") {
					// @ATTR vendor|bool|Allows this NPC to buy/sell items.
					vendor = toBool(infile.val);
				}
				else if (infile.key == "constant_stock") {
					// @ATTR constant_stock|repeatable(list(item_id))|A list of items this vendor has for sale.
					stack.quantity = 1;
					while (infile.val != "") {
						stack.item = toInt(infile.nextValue());
						stock.add(stack);
					}
				}
				else if (infile.key == "status_stock") {
					// @ATTR status_stock|repeatable(string, list(item_id)) : Required status, Item(s)|A list of items this vendor will have for sale if the required status is met.
					if (camp->checkStatus(infile.nextValue())) {
						stack.quantity = 1;
						while (infile.val != "") {
							stack.item = toInt(infile.nextValue());
							stock.add(stack);
						}
					}
				}
				else if (infile.key == "random_stock") {
					// @ATTR random_stock|list(loot)|Use a loot table to add random items to the stock; either a filename or an inline definition.
					if (clear_random_table) {
						random_table.clear();
						clear_random_table = false;
					}

					random_table.push_back(Event_Component());
					loot->parseLoot(infile, &random_table.back(), &random_table);
				}
				else if (infile.key == "random_stock_count") {
					// @ATTR random_stock_count|int, int : Min, Max|Sets the minimum (and optionally, the maximum) amount of random items this npc can have.
					random_table_count.x = toInt(infile.nextValue());
					random_table_count.y = toInt(infile.nextValue());
					if (random_table_count.x != 0 || random_table_count.y != 0) {
						clampFloor(random_table_count.x, 1);
						clampFloor(random_table_count.y, random_table_count.x);
					}
				}

				// handle vocals
				else if (infile.key == "vox_intro") {
					// @ATTR vox_intro|repeatable(filename)|Filename of a sound file to play when initially interacting with the NPC.
					loadSound(infile.val, NPC_VOX_INTRO);
				}

				else {
					infile.error("NPC: '%s' is not a valid key.", infile.key.c_str());
				}
			}
		}
		infile.close();
	}
	loadGraphics(filename_portrait);

	// fill inventory with items from random stock table
	unsigned rand_count = randBetween(random_table_count.x, random_table_count.y);

	std::vector<ItemStack> rand_itemstacks;
	for (unsigned i=0; i<rand_count; ++i) {
		loot->checkLoot(random_table, NULL, &rand_itemstacks);
	}
	std::sort(rand_itemstacks.begin(), rand_itemstacks.end(), compareItemStack);
	for (size_t i=0; i<rand_itemstacks.size(); ++i) {
		stock.add(rand_itemstacks[i]);
	}
}

void NPC::loadGraphics(const std::string& filename_portrait) {

	if (gfx != "") {
		anim->increaseCount(gfx);
		animationSet = anim->getAnimationSet(gfx);
		activeAnimation = animationSet->getAnimation();
	}

	if (filename_portrait != "") {
		Image *graphics;
		graphics = render_device->loadImage(filename_portrait,
				   "Couldn't load NPC portrait", false);
		if (graphics) {
			portrait = graphics->createSprite();
			graphics->unref();
		}
	}
}

/**
 * filename assumes the file is in soundfx/npcs/
 * type is a const int enum, see NPC.h
 * returns -1 if not loaded or error.
 * returns index in specific vector where to be found.
 */
int NPC::loadSound(const std::string& fname, int type) {

	SoundManager::SoundID a = snd->load(fname, "NPC voice");

	if (!a)
		return -1;

	if (type == NPC_VOX_INTRO) {
		vox_intro.push_back(a);
		return static_cast<int>(vox_intro.size()) - 1;
	}

	if (type == NPC_VOX_QUEST) {
		vox_quests.push_back(a);
		return static_cast<int>(vox_quests.size()) - 1;
	}
	return -1;
}

void NPC::logic() {
	activeAnimation->advanceFrame();
}

/**
 * type is a const int enum, see NPC.h
 */
bool NPC::playSound(int type, int id) {
	if (type == NPC_VOX_INTRO) {
		int roll;
		if (vox_intro.empty()) return false;
		roll = rand() % static_cast<int>(vox_intro.size());
		snd->play(vox_intro[roll], "NPC_VOX");
		return true;
	}
	if (type == NPC_VOX_QUEST) {
		if (id < 0 || id >= static_cast<int>(vox_quests.size())) return false;
		snd->play(vox_quests[id], "NPC_VOX");
		return true;
	}
	return false;
}

/**
 * get list of available dialogs with NPC
 */
void NPC::getDialogNodes(std::vector<int> &result) {
	result.clear();
	if (!talker)
		return;

	std::string group;
	typedef std::vector<int> Dialogs;
	typedef std::map<std::string, Dialogs > DialogGroups;
	DialogGroups groups;

	for (size_t i=dialog.size(); i>0; i--) {
		bool is_available = true;
		bool is_grouped = false;
		for (size_t j=0; j<dialog[i-1].size(); j++) {
			if (dialog[i-1][j].type == EC_NPC_DIALOG_GROUP) {
				is_grouped = true;
				group = dialog[i-1][j].s;
			}
			else {
				if (camp->checkAllRequirements(dialog[i-1][j]))
					continue;

				is_available = false;
				break;
			}
		}

		if (is_available) {
			if (!is_grouped) {
				result.push_back(static_cast<int>(i-1));
			}
			else {
				DialogGroups::iterator it;
				it = groups.find(group);
				if (it == groups.end()) {
					groups.insert(DialogGroups::value_type(group, Dialogs()));
				}
				else
					it->second.push_back(static_cast<int>(i-1));

			}
		}
	}

	/* Iterate over dialoggroups and roll a dialog to add to result */
	DialogGroups::iterator it;
	it = groups.begin();
	if (it == groups.end())
		return;

	while (it != groups.end()) {
		/* roll a dialog for this group and add to result */
		int di = it->second[rand() % it->second.size()];
		result.push_back(di);
		++it;
	}
}

std::string NPC::getDialogTopic(unsigned int dialog_node) {
	if (!talker)
		return "";

	for (unsigned int j=0; j<dialog[dialog_node].size(); j++) {
		if (dialog[dialog_node][j].type == EC_NPC_DIALOG_TOPIC)
			return dialog[dialog_node][j].s;
	}

	return "";
}

/**
 * Check if the hero can move during this dialog branch
 */
bool NPC::checkMovement(unsigned int dialog_node) {
	if (dialog_node < dialog.size()) {
		for (unsigned int i=0; i<dialog[dialog_node].size(); i++) {
			if (dialog[dialog_node][i].type == EC_NPC_ALLOW_MOVEMENT)
				return toBool(dialog[dialog_node][i].s);
		}
	}
	return true;
}

/**
 * Process the current dialog
 *
 * Return false if the dialog has ended
 */
bool NPC::processDialog(unsigned int dialog_node, unsigned int &event_cursor) {
	if (dialog_node >= dialog.size())
		return false;

	while (event_cursor < dialog[dialog_node].size()) {

		// we've already determined requirements are met, so skip these
		if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_STATUS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_NOT_STATUS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_LEVEL) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_NOT_LEVEL) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_CURRENCY) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_NOT_CURRENCY) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_ITEM) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_NOT_ITEM) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_CLASS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_REQUIRES_NOT_CLASS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EC_NPC_DIALOG_THEM) {
			return true;
		}
		else if (dialog[dialog_node][event_cursor].type == EC_NPC_DIALOG_YOU) {
			return true;
		}
		else if (dialog[dialog_node][event_cursor].type == EC_NPC_VOICE) {
			playSound(NPC_VOX_QUEST, dialog[dialog_node][event_cursor].x);
		}
		else if (dialog[dialog_node][event_cursor].type == EC_NONE) {
			// conversation ends
			return false;
		}

		event_cursor++;
	}
	return false;
}

void NPC::processEvent(unsigned int dialog_node, unsigned int cursor) {

	Event ev;

	if (dialog_node < dialog.size() && cursor < dialog[dialog_node].size() && isDialogType(dialog[dialog_node][cursor].type)) {
		cursor++;
	}

	while (dialog_node < dialog.size() && cursor < dialog[dialog_node].size() && !isDialogType(dialog[dialog_node][cursor].type)) {
		ev.components.push_back(dialog[dialog_node][cursor]);
		cursor++;
	}

	EventManager::executeEvent(ev);
}

Renderable NPC::getRender() {
	Renderable r = activeAnimation->getCurrentFrame(direction);
	r.map_pos.x = pos.x;
	r.map_pos.y = pos.y;

	return r;
}

bool NPC::isDialogType(const EVENT_COMPONENT_TYPE &type) {
	return type == EC_NPC_DIALOG_THEM || type == EC_NPC_DIALOG_YOU || type == EC_NPC_VOICE;
}

NPC::~NPC() {

	if (portrait) delete portrait;

	if (gfx != "") {
		anim->decreaseCount(gfx);
	}

	while (!vox_intro.empty()) {
		snd->unload(vox_intro.back());
		vox_intro.pop_back();
	}
	while (!vox_quests.empty()) {
		snd->unload(vox_quests.back());
		vox_quests.pop_back();
	}
}
