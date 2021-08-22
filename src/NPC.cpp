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

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "CampaignManager.h"
#include "EntityBehavior.h"
#include "EntityManager.h"
#include "EventManager.h"
#include "FileParser.h"
#include "ItemManager.h"
#include "LootManager.h"
#include "MapRenderer.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "NPC.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

NPC::NPC(const Entity& e)
	: Entity(e)
	, gfx("")
	, vox_intro()
	, vox_quests()
	, name("")
	, direction(0)
	, show_on_minimap(true)
	, npc_portrait(NULL)
	, hero_portrait(NULL)
	, talker(false)
	, vendor(false)
	, reset_buyback(true)
	, stock()
	, dialog()
{
	stock.init(VENDOR_MAX_STOCK);
}

/**
 * NPCs are stored in simple config files
 *
 * @param npc_id Config file for npc
 */
void NPC::load(const std::string& npc_id) {

	FileParser infile;
	ItemStack stack;

	portrait_filenames.resize(1);

	// @CLASS NPC|Description of NPCs in npcs/
	if (infile.open(npc_id, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		bool clear_random_table = true;

		while (infile.next()) {
			if (infile.section == "stats") {
				// handled by StatBlock::load()
				continue;
			}
			else if (infile.section == "dialog") {
				if (infile.new_section) {
					dialog.push_back(std::vector<EventComponent>());
				}
				EventComponent e;
				e.type = EventComponent::NONE;
				if (infile.key == "id") {
					// @ATTR dialog.id|string|A unique identifer used to reference this dialog.
					e.type = EventComponent::NPC_DIALOG_ID;
					e.s = infile.val;
				}
				else if (infile.key == "him" || infile.key == "her") {
					// @ATTR dialog.him|repeatable(string)|A line of dialog from the NPC.
					// @ATTR dialog.her|repeatable(string)|A line of dialog from the NPC.
					e.type = EventComponent::NPC_DIALOG_THEM;
					e.s = msg->get(infile.val);
				}
				else if (infile.key == "you") {
					// @ATTR dialog.you|repeatable(string)|A line of dialog from the player.
					e.type = EventComponent::NPC_DIALOG_YOU;
					e.s = msg->get(infile.val);
				}
				else if (infile.key == "voice") {
					// @ATTR dialog.voice|repeatable(string)|Filename of a voice sound file to play.
					e.type = EventComponent::NPC_VOICE;
					e.x = loadSound(infile.val, VOX_QUEST);
				}
				else if (infile.key == "topic") {
					// @ATTR dialog.topic|string|The name of this dialog topic. Displayed when picking a dialog tree.
					e.type = EventComponent::NPC_DIALOG_TOPIC;
					e.s = msg->get(infile.val);
				}
				else if (infile.key == "group") {
					// @ATTR dialog.group|string|Dialog group.
					e.type = EventComponent::NPC_DIALOG_GROUP;
					e.s = infile.val;
				}
				else if (infile.key == "allow_movement") {
					// @ATTR dialog.allow_movement|bool|Restrict the player's mvoement during dialog.
					e.type = EventComponent::NPC_ALLOW_MOVEMENT;
					e.s = infile.val;
				}
				else if (infile.key == "portrait_him" || infile.key == "portrait_her") {
					// @ATTR dialog.portrait_him|repeatable(filename)|Filename of a portrait to display for the NPC during this dialog.
					// @ATTR dialog.portrait_her|repeatable(filename)|Filename of a portrait to display for the NPC during this dialog.
					e.type = EventComponent::NPC_PORTRAIT_THEM;
					e.s = infile.val;
					portrait_filenames.push_back(e.s);
				}
				else if (infile.key == "portrait_you") {
					// @ATTR dialog.portrait_you|repeatable(filename)|Filename of a portrait to display for the player during this dialog.
					e.type = EventComponent::NPC_PORTRAIT_YOU;
					e.s = infile.val;
					portrait_filenames.push_back(e.s);
				}
				else if (infile.key == "take_a_party") {
					// @ATTR dialog.take_a_party|bool|Start/stop taking a party with player.
					e.type = EventComponent::NPC_TAKE_A_PARTY;
					e.x = Parse::toBool(infile.val);
				}
				else if (infile.key == "response") {
					// @ATTR dialog.response|repeatable(string)|A dialog ID to present as a selectable response. This key must precede the dialog text line.
					e.type = EventComponent::NPC_DIALOG_RESPONSE;
					e.s = infile.val;
				}
				else if (infile.key == "response_only") {
					// @ATTR dialog.response_only|bool|If true, this dialog topic will only appear when explicitly referenced with the "response" key.
					e.type = EventComponent::NPC_DIALOG_RESPONSE_ONLY;
					e.x = Parse::toBool(infile.val);
				}
				else {
					Event ev;
					EventManager::loadEventComponent(infile, &ev, NULL);

					for (size_t i=0; i<ev.components.size(); ++i) {
						if (ev.components[i].type != EventComponent::NONE) {
							dialog.back().push_back(ev.components[i]);
						}
					}
				}

				if (e.type != EventComponent::NONE) {
					dialog.back().push_back(e);
				}
			}
			else if (infile.section.empty() || infile.section == "npc") {
				filename = npc_id;

				if (infile.new_section) {
					// APPENDed file
					clear_random_table = true;
				}

				if (infile.key == "name") {
					// @ATTR name|string|NPC's name.
					name = msg->get(infile.val);
				}
				else if (infile.key == "animations" || infile.key == "gfx") {
					// TODO "gfx" is deprecated
				}
				else if (infile.key == "direction") {
					// @ATTR direction|direction|The direction to use for this NPC's stance animation.
					direction = Parse::toDirection(infile.val);
				}
				else if (infile.key == "show_on_minimap") {
					// @ATTR show_on_minimap|bool|If true, this NPC will be shown on the minimap. The default is true.
					show_on_minimap = Parse::toBool(infile.val);
				}

				// handle talkers
				else if (infile.key == "talker") {
					// @ATTR talker|bool|Allows this NPC to be talked to.
					talker = Parse::toBool(infile.val);
				}
				else if (infile.key == "portrait") {
					// @ATTR portrait|filename|Filename of the default portrait image.
					portrait_filenames[0] = infile.val;
				}

				// handle vendors
				else if (infile.key == "vendor") {
					// @ATTR vendor|bool|Allows this NPC to buy/sell items.
					vendor = Parse::toBool(infile.val);
				}
				else if (infile.key == "vendor_requires_status") {
					// @ATTR vendor_requires_status|list(string)|The player must have these statuses in order to use this NPC as a vendor.
					while (infile.val != "") {
						vendor_requires_status.push_back(camp->registerStatus(Parse::popFirstString(infile.val)));
					}
				}
				else if (infile.key == "vendor_requires_not_status") {
					// @ATTR vendor_requires_not_status|list(string)|The player must not have these statuses in order to use this NPC as a vendor.
					while (infile.val != "") {
						vendor_requires_not_status.push_back(camp->registerStatus(Parse::popFirstString(infile.val)));
					}
				}
				else if (infile.key == "constant_stock") {
					// @ATTR constant_stock|repeatable(list(item_id))|A list of items this vendor has for sale. Quantity can be specified by appending ":Q" to the item_id, where Q is an integer.
					while (infile.val != "") {
						stack = Parse::toItemQuantityPair(Parse::popFirstString(infile.val));
						stock.add(stack, ItemStorage::NO_SLOT);
					}
				}
				else if (infile.key == "status_stock") {
					// @ATTR status_stock|repeatable(string, list(item_id)) : Required status, Item(s)|A list of items this vendor will have for sale if the required status is met. Quantity can be specified by appending ":Q" to the item_id, where Q is an integer.
					if (camp->checkStatus(camp->registerStatus(Parse::popFirstString(infile.val)))) {
						while (infile.val != "") {
							stack = Parse::toItemQuantityPair(Parse::popFirstString(infile.val));
							stock.add(stack, ItemStorage::NO_SLOT);
						}
					}
				}
				else if (infile.key == "random_stock") {
					// @ATTR random_stock|list(loot)|Use a loot table to add random items to the stock; either a filename or an inline definition.
					if (clear_random_table) {
						random_table.clear();
						clear_random_table = false;
					}

					random_table.push_back(EventComponent());
					loot->parseLoot(infile.val, &random_table.back(), &random_table);
				}
				else if (infile.key == "random_stock_count") {
					// @ATTR random_stock_count|int, int : Min, Max|Sets the minimum (and optionally, the maximum) amount of random items this npc can have.
					random_table_count.x = Parse::popFirstInt(infile.val);
					random_table_count.y = Parse::popFirstInt(infile.val);
					if (random_table_count.x != 0 || random_table_count.y != 0) {
						random_table_count.x = std::max(random_table_count.x, 1);
						random_table_count.y = std::max(random_table_count.y, random_table_count.x);
					}
				}

				// handle vocals
				else if (infile.key == "vox_intro") {
					// @ATTR vox_intro|repeatable(filename)|Filename of a sound file to play when initially interacting with the NPC.
					loadSound(infile.val, VOX_INTRO);
				}

				else {
					infile.error("NPC: '%s' is not a valid key.", infile.key.c_str());
				}
			}
		}
		infile.close();
	}

	loadGraphics();

	// fill inventory with items from random stock table
	unsigned rand_count = Math::randBetween(random_table_count.x, random_table_count.y);

	std::vector<ItemStack> rand_itemstacks;
	for (unsigned i=0; i<rand_count; ++i) {
		loot->checkLoot(random_table, NULL, &rand_itemstacks);
	}
	std::sort(rand_itemstacks.begin(), rand_itemstacks.end(), compareItemStack);
	for (size_t i=0; i<rand_itemstacks.size(); ++i) {
		stock.add(rand_itemstacks[i], ItemStorage::NO_SLOT);
	}

	// warn if dialog nodes lack a topic
	std::string full_filename = mods->locate(npc_id);
	for (size_t i=0; i<dialog.size(); ++i) {
		std::string topic = getDialogTopic(static_cast<int>(i));
		if (topic.empty()) {
			Utils::logInfo("[%s] NPC: Dialog node %d does not have a topic.", full_filename.c_str(), i);
		}
	}
}

void NPC::loadGraphics() {

	if (stats.animations != "") {
		anim->increaseCount(stats.animations);
		animationSet = anim->getAnimationSet(stats.animations);
		activeAnimation = animationSet->getAnimation("");
	}

	portraits.resize(portrait_filenames.size(), NULL);

	for (size_t i = 0; i < portrait_filenames.size(); ++i) {
		if (!portrait_filenames[i].empty()) {
			Image *graphics;
			graphics = render_device->loadImage(portrait_filenames[i], RenderDevice::ERROR_NORMAL);
			if (graphics) {
				portraits[i] = graphics->createSprite();
				graphics->unref();
			}
		}
	}
}

/**
 * filename assumes the file is in soundfx/npcs/
 * vox_type is a const int enum, see NPC.h
 * returns -1 if not loaded or error.
 * returns index in specific vector where to be found.
 */
int NPC::loadSound(const std::string& fname, int vox_type) {

	SoundID a = snd->load(fname, "NPC voice");

	if (!a)
		return -1;

	if (vox_type == VOX_INTRO) {
		vox_intro.push_back(a);
		return static_cast<int>(vox_intro.size()) - 1;
	}

	if (vox_type == VOX_QUEST) {
		vox_quests.push_back(a);
		return static_cast<int>(vox_quests.size()) - 1;
	}
	return -1;
}

void NPC::logic() {
	mapr->collider.unblock(stats.pos.x, stats.pos.y);

	Entity::logic();
	moveMapEvents();

	if (!stats.hero_ally)
		mapr->collider.block(stats.pos.x, stats.pos.y, true);
}

bool NPC::playSoundIntro() {
	if (vox_intro.empty())
		return false;

	size_t roll = static_cast<size_t>(rand()) % vox_intro.size();
	snd->play(vox_intro[roll], "NPC_VOX", snd->NO_POS, !snd->LOOP);
	return true;
}

bool NPC::playSoundQuest(int id) {
	if (id < 0 || id >= static_cast<int>(vox_quests.size()))
		return false;

	snd->play(vox_quests[id], "NPC_VOX", snd->NO_POS, !snd->LOOP);
	return true;
}

/**
 * get list of available dialogs with NPC
 */
void NPC::getDialogNodes(std::vector<int> &result, bool allow_responses) {
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
			if (dialog[i-1][j].type == EventComponent::NPC_DIALOG_GROUP) {
				is_grouped = true;
				group = dialog[i-1][j].s;
			}
			else if (dialog[i-1][j].type == EventComponent::NPC_DIALOG_RESPONSE_ONLY) {
				if (dialog[i-1][j].x && !allow_responses) {
					is_available = false;
					break;
				}
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

void NPC::getDialogResponses(std::vector<int>& result, size_t node_id, size_t event_cursor) {
	if (node_id >= dialog.size())
		return;

	if (event_cursor >= dialog[node_id].size())
		return;

	std::vector<size_t> response_ids;
	for (size_t i = event_cursor; i > 0; i--) {
		if (isDialogType(dialog[node_id][i-1].type))
			break;

		if (dialog[node_id][i-1].type == EventComponent::NPC_DIALOG_RESPONSE)
			response_ids.push_back(i-1);
	}

	if (response_ids.empty())
		return;

	std::vector<int> nodes;
	getDialogNodes(nodes, GET_RESPONSE_NODES);

	for (size_t i = 0; i < response_ids.size(); i++) {
		for (size_t j = 0; j < nodes.size(); j++) {
			std::string id;

			for (size_t k = 0; k < dialog[nodes[j]].size(); k++) {
				if (dialog[nodes[j]][k].type == EventComponent::NPC_DIALOG_ID) {
					id = dialog[nodes[j]][k].s;
					break;
				}
			}

			if (id.empty())
				continue;

			if (id == dialog[node_id][response_ids[i]].s) {
				result.push_back(nodes[j]);
				break;
			}
		}
	}
}

std::string NPC::getDialogTopic(unsigned int dialog_node) {
	if (!talker)
		return "";

	for (unsigned int j=0; j<dialog[dialog_node].size(); j++) {
		if (dialog[dialog_node][j].type == EventComponent::NPC_DIALOG_TOPIC)
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
			if (dialog[dialog_node][i].type == EventComponent::NPC_ALLOW_MOVEMENT)
				return Parse::toBool(dialog[dialog_node][i].s);
		}
	}
	return true;
}

void NPC::moveMapEvents() {

	// Update event position after NPC has moved
	for (size_t i = 0; i < mapr->events.size(); i++)
	{
		if (mapr->events[i].type == filename)
		{
			mapr->events[i].location.x = static_cast<int>(stats.pos.x);
			mapr->events[i].location.y = static_cast<int>(stats.pos.y);

			mapr->events[i].hotspot.x = static_cast<int>(stats.pos.x);
			mapr->events[i].hotspot.y = static_cast<int>(stats.pos.y);

			mapr->events[i].center.x =
				static_cast<float>(stats.pos.x) + static_cast<float>(mapr->events[i].hotspot.w)/2;
			mapr->events[i].center.y =
				static_cast<float>(stats.pos.y) + static_cast<float>(mapr->events[i].hotspot.h)/2;

			for (size_t ci = 0; ci < mapr->events[i].components.size(); ci++)
			{
				if (mapr->events[i].components[ci].type == EventComponent::NPC_HOTSPOT)
				{
					mapr->events[i].components[ci].x = static_cast<int>(stats.pos.x);
					mapr->events[i].components[ci].y = static_cast<int>(stats.pos.y);
				}
			}
		}
	}
}

bool NPC::checkVendor() {
	if (!vendor)
		return false;

	for (size_t i = 0; i < vendor_requires_status.size(); ++i) {
		if (!camp->checkStatus(vendor_requires_status[i]))
			return false;
	}

	for (size_t i = 0; i < vendor_requires_not_status.size(); ++i) {
		if (camp->checkStatus(vendor_requires_not_status[i]))
			return false;
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

	npc_portrait = portraits[0];
	hero_portrait = NULL;

	while (event_cursor < dialog[dialog_node].size()) {

		// we've already determined requirements are met, so skip these
		if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_STATUS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_NOT_STATUS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_LEVEL) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_NOT_LEVEL) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_CURRENCY) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_NOT_CURRENCY) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_ITEM) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_NOT_ITEM) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_CLASS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::REQUIRES_NOT_CLASS) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_DIALOG_ID) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_DIALOG_RESPONSE) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_DIALOG_RESPONSE_ONLY) {
			// continue to next event component
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_DIALOG_THEM) {
			return true;
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_DIALOG_YOU) {
			return true;
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_VOICE) {
			playSoundQuest(dialog[dialog_node][event_cursor].x);
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_PORTRAIT_THEM) {
			npc_portrait = portraits[0];
			for (size_t i = 0; i < portrait_filenames.size(); ++i) {
				if (dialog[dialog_node][event_cursor].s == portrait_filenames[i]) {
					npc_portrait = portraits[i];
					break;
				}
			}
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_PORTRAIT_YOU) {
			hero_portrait = NULL;
			for (size_t i = 0; i < portrait_filenames.size(); ++i) {
				if (dialog[dialog_node][event_cursor].s == portrait_filenames[i]) {
					hero_portrait = portraits[i];
					break;
				}
			}
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NPC_TAKE_A_PARTY) {
			bool new_hero_ally = dialog[dialog_node][event_cursor].x == 0 ? false : true;
			if (stats.hero_ally != new_hero_ally) {
				stats.hero_ally = new_hero_ally;
				if (stats.hero_ally) {
					entitym->entities.push_back(this);
				}
				else {
					for (size_t i = entitym->entities.size(); i > 0; --i) {
						if (entitym->entities[i-1] == this)
							entitym->entities.erase(entitym->entities.begin() + i - 1);
					}
				}
			}
		}
		else if (dialog[dialog_node][event_cursor].type == EventComponent::NONE) {
			// conversation ends
			return false;
		}

		event_cursor++;
	}
	return false;
}

void NPC::processEvent(unsigned int dialog_node, unsigned int cursor) {
	if (dialog_node >= dialog.size())
		return;

	Event ev;

	if (cursor < dialog[dialog_node].size() && isDialogType(dialog[dialog_node][cursor].type)) {
		cursor++;
	}

	while (cursor < dialog[dialog_node].size() && !isDialogType(dialog[dialog_node][cursor].type)) {
		ev.components.push_back(dialog[dialog_node][cursor]);
		cursor++;
	}

	EventManager::executeEvent(ev);
}

bool NPC::isDialogType(const int &event_type) {
	return event_type == EventComponent::NPC_DIALOG_THEM || event_type == EventComponent::NPC_DIALOG_YOU;
}

NPC::~NPC() {

	for (size_t i = 0; i < portraits.size(); ++i) {
		delete portraits[i];
	}

	if (stats.animations != "") {
		anim->decreaseCount(stats.animations);
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
