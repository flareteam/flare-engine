/*
Copyright © 2011-2012 Clint Bellanger and morris989
Copyright © 2012 Stefan Beller
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
 * class MenuTalker
 */

#include "Avatar.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "InputState.h"
#include "Menu.h"
#include "MenuInventory.h"
#include "MenuManager.h"
#include "MenuTalker.h"
#include "MenuVendor.h"
#include "MessageEngine.h"
#include "NPC.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetScrollBox.h"

MenuTalker::Action::Action()
	: btn(NULL)
	, node_id(0)
	, is_vendor(false)
{}

MenuTalker::Action::~Action()
{}

MenuTalker::MenuTalker()
	: Menu()
	, portrait(NULL)
	, dialog_node(-1)
	, event_cursor(0)
	, first_interaction(false)
	, font_who("font_regular")
	, font_dialog("font_regular")
	, topic_color_normal(font->getColor(FontEngine::COLOR_MENU_BONUS))
	, topic_color_hover(font->getColor(FontEngine::COLOR_WIDGET_NORMAL))
	, topic_color_pressed(font->getColor(FontEngine::COLOR_WIDGET_DISABLED))
	, trade_color_normal(font->getColor(FontEngine::COLOR_MENU_BONUS))
	, trade_color_hover(font->getColor(FontEngine::COLOR_WIDGET_NORMAL))
	, trade_color_pressed(font->getColor(FontEngine::COLOR_WIDGET_DISABLED))
	, npc(NULL)
	, advanceButton(new WidgetButton("images/menus/buttons/right.png"))
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, npc_from_map(true) {

	setBackground("images/menus/dialog_box.png");

	// Load config settings
	FileParser infile;
	// @CLASS MenuTalker|Description of menus/talker.txt
	if(infile.open("menus/talker.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = Parse::toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR advance|point|Position of the button to advance dialog.
			else if(infile.key == "advance") {
				Point pos = Parse::toPoint(infile.val);
				advanceButton->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			// @ATTR dialogbox|rectangle|Position and dimensions of the text box graphics.
			else if (infile.key == "dialogbox") dialog_pos = Parse::toRect(infile.val);
			// @ATTR dialogtext|rectangle|Rectangle where the dialog text is placed.
			else if (infile.key == "dialogtext") text_pos = Parse::toRect(infile.val);
			// @ATTR text_offset|point|Margins for the left/right and top/bottom of the dialog text.
			else if (infile.key == "text_offset") text_offset = Parse::toPoint(infile.val);
			// @ATTR portrait_he|rectangle|Position and dimensions of the NPC portrait graphics.
			else if (infile.key == "portrait_he") portrait_he = Parse::toRect(infile.val);
			// @ATTR portrait_you|rectangle|Position and dimensions of the player's portrait graphics.
			else if (infile.key == "portrait_you") portrait_you = Parse::toRect(infile.val);
			// @ATTR font_who|predefined_string|Font style to use for the name of the currently talking person.
			else if (infile.key == "font_who") font_who = infile.val;
			// @ATTR font_dialog|predefined_string|Font style to use for the dialog text.
			else if (infile.key == "font_dialog") font_dialog = infile.val;

			// @ATTR topic_color_normal|color|The normal color for topic text.
			else if (infile.key == "topic_color_normal") topic_color_normal = Parse::toRGB(infile.val);
			// @ATTR topic_color_hover|color|The color for topic text when highlighted.
			else if (infile.key == "topic_color_hover") topic_color_hover = Parse::toRGB(infile.val);
			// @ATTR topic_color_normal|color|The color for topic text when clicked.
			else if (infile.key == "topic_color_pressed") topic_color_pressed = Parse::toRGB(infile.val);

			// @ATTR trade_color_normal|color|The normal color for the "Trade" text.
			else if (infile.key == "trade_color_normal") trade_color_normal = Parse::toRGB(infile.val);
			// @ATTR trade_color_hover|color|The color for the "Trade" text when highlighted.
			else if (infile.key == "trade_color_hover") trade_color_hover = Parse::toRGB(infile.val);
			// @ATTR trade_color_normal|color|The color for the "Trade" text when clicked.
			else if (infile.key == "trade_color_pressed") trade_color_pressed = Parse::toRGB(infile.val);

			else infile.error("MenuTalker: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	label_name = new WidgetLabel();
	label_name->setBasePos(text_pos.x + text_offset.x, text_pos.y + text_offset.y, Utils::ALIGN_TOPLEFT);
	label_name->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	textbox = new WidgetScrollBox(text_pos.w, text_pos.h-(text_offset.y*2));
	textbox->setBasePos(text_pos.x, text_pos.y + text_offset.y, Utils::ALIGN_TOPLEFT);

	align();
}

void MenuTalker::align() {
	Menu::align();

	advanceButton->setPos(window_area.x, window_area.y);
	closeButton->setPos(window_area.x, window_area.y);

	label_name->setPos(window_area.x, window_area.y);

	textbox->setPos(window_area.x, window_area.y + label_name->getBounds()->h);
	textbox->pos.h = text_pos.h - (text_offset.y*2);
}

void MenuTalker::chooseDialogNode(int request_dialog_node) {
	event_cursor = 0;

	if(request_dialog_node == -1) {
		// display the topic list (or automatically select a topic if there's only one)
		dialog_node = -1;
		createActionBuffer();

		// need to set the portrait here since we don't call processDialog()
		if (!npc->portraits.empty())
			npc->npc_portrait = npc->portraits[0];

		if (actions.size() == 1 && (!actions[0].is_vendor || first_interaction)) {
			executeAction(0);
		}
		else if (actions.empty()) {
			setNPC(NULL); // end dialog
		}
	}
	else {
		dialog_node = request_dialog_node;
		npc->processEvent(dialog_node, event_cursor);
		if (npc->processDialog(dialog_node, event_cursor))
			createBuffer();
		else
			setNPC(NULL); // end dialog
	}

	first_interaction = false;
}

/**
 * Menu interaction (enter/space/click to continue)
 */
void MenuTalker::logic() {

	if (!visible || !npc)
		return;

	tablist.logic();

	if (advanceButton->checkClick() || closeButton->checkClick()) {
		// button was clicked
		nextDialog();
	}
	else if	((advanceButton->enabled || closeButton->enabled) && inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
		// pressed next/more
		inpt->lock[Input::ACCEPT] = true;
		nextDialog();
	}
	else {
		textbox->logic();

		Point mouse = textbox->input_assist(inpt->mouse);
		for (size_t i = 0; i < actions.size(); ++i) {
			if (actions[i].btn->checkClickAt(mouse.x, mouse.y)) {
				executeAction(i);
				break;
			}
		}

		Rect lock_area = dialog_pos;
		lock_area.x += window_area.x;
		lock_area.y += window_area.y;
		if (inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1] && Utils::isWithinRect(lock_area, inpt->mouse)) {
			inpt->lock[Input::MAIN1] = true;
		}
	}
}

void MenuTalker::createActionBuffer() {
	createActionButtons(-1);

	int button_height = 0;
	if (!actions.empty()) {
		button_height = static_cast<int>(actions.size()) * actions[0].btn->pos.h;
	}

	for (size_t i = 0; i < actions.size(); ++i) {
		actions[i].btn->pos.x = text_offset.x;
		actions[i].btn->pos.y = (static_cast<int>(i) * actions[i].btn->pos.h);
		actions[i].btn->refresh();
	}

	label_name->setText(npc->name);
	label_name->setFont(font_who);
	textbox->resize(textbox->pos.w, button_height);

	align();

	closeButton->enabled = true;
	advanceButton->enabled = false;

	setupTabList();
}

void MenuTalker::createBuffer() {
	clearActionButtons();

	if (static_cast<unsigned>(dialog_node) >= npc->dialog.size() || event_cursor >= npc->dialog[dialog_node].size())
		return;

	createActionButtons(dialog_node);

	int button_height = 0;
	if (!actions.empty()) {
		button_height = static_cast<int>(actions.size()) * actions[0].btn->pos.h;
	}

	std::string line;

	// speaker name
	int etype = npc->dialog[dialog_node][event_cursor].type;
	std::string who;

	if (etype == EventComponent::NPC_DIALOG_THEM) {
		who = npc->name;
	}
	else if (etype == EventComponent::NPC_DIALOG_YOU) {
		who = hero_name;
	}

	label_name->setText(who);
	label_name->setFont(font_who);


	line = Utils::substituteVarsInString(npc->dialog[dialog_node][event_cursor].s, pc);

	// render dialog text to the scrollbox buffer
	Point line_size = font->calc_size(line,textbox->pos.w-(text_offset.x*2));
	textbox->resize(textbox->pos.w, line_size.y + button_height);
	font->setFont(font_dialog);
	font->render(
		line,
		text_offset.x,
		0,
		FontEngine::JUSTIFY_LEFT,
		textbox->contents->getGraphics(),
		text_pos.w - text_offset.x*2,
		font->getColor(FontEngine::COLOR_MENU_NORMAL)
	);

	for (size_t i = 0; i < actions.size(); ++i) {
		actions[i].btn->pos.x = text_offset.x;
		actions[i].btn->pos.y = line_size.y + (static_cast<int>(i) * actions[i].btn->pos.h);
		actions[i].btn->refresh();
	}

	align();

	if (!actions.empty()) {
		advanceButton->enabled = false;
		closeButton->enabled = false;
	}
	else if (!npc->dialog[dialog_node].empty() && event_cursor < npc->dialog[dialog_node].size()-1 && npc->dialog[dialog_node][event_cursor+1].type != EventComponent::NONE) {
		advanceButton->enabled = true;
		closeButton->enabled = false;
	}
	else {
		advanceButton->enabled = false;
		closeButton->enabled = true;
	}

	setupTabList();
}

void MenuTalker::render() {
	if (!visible) return;
	Rect src;
	Rect dest;

	int offset_x = window_area.x;
	int offset_y = window_area.y;

	// dialog box
	src.x = 0;
	src.y = 0;
	dest.x = offset_x + dialog_pos.x;
	dest.y = offset_y + dialog_pos.y;
	src.w = dest.w = dialog_pos.w;
	src.h = dest.h = dialog_pos.h;

	setBackgroundClip(src);
	setBackgroundDest(dest);
	Menu::render();

	if (static_cast<unsigned>(dialog_node) < npc->dialog.size() && event_cursor < npc->dialog[dialog_node].size()) {
		// show active portrait
		int etype = npc->dialog[dialog_node][event_cursor].type;
		if (etype == EventComponent::NPC_DIALOG_THEM) {
			if (npc->npc_portrait) {
				src.w = dest.w = portrait_he.w;
				src.h = dest.h = portrait_he.h;
				dest.x = offset_x + portrait_he.x;
				dest.y = offset_y + portrait_he.y;

				npc->npc_portrait->setClipFromRect(src);
				npc->npc_portrait->setDestFromRect(dest);
				render_device->render(npc->npc_portrait);
			}
		}
		else if (etype == EventComponent::NPC_DIALOG_YOU) {
			if (npc->hero_portrait) {
				src.w = dest.w = portrait_you.w;
				src.h = dest.h = portrait_you.h;
				dest.x = offset_x + portrait_you.x;
				dest.y = offset_y + portrait_you.y;
				npc->hero_portrait->setClipFromRect(src);
				npc->hero_portrait->setDestFromRect(dest);
				render_device->render(npc->hero_portrait);
			}
			else if (portrait) {
				src.w = dest.w = portrait_you.w;
				src.h = dest.h = portrait_you.h;
				dest.x = offset_x + portrait_you.x;
				dest.y = offset_y + portrait_you.y;
				portrait->setClipFromRect(src);
				portrait->setDestFromRect(dest);
				render_device->render(portrait);
			}
		}
	}
	else if (dialog_node == -1 && npc->npc_portrait) {
		src.w = dest.w = portrait_he.w;
		src.h = dest.h = portrait_he.h;
		dest.x = offset_x + portrait_he.x;
		dest.y = offset_y + portrait_he.y;

		npc->npc_portrait->setClipFromRect(src);
		npc->npc_portrait->setDestFromRect(dest);
		render_device->render(npc->npc_portrait);
	}

	// name & dialog text
	label_name->render();
	textbox->render();

	// show advance button if there are more event components, or close button if not
	if (advanceButton->enabled)
		advanceButton->render();
	else if (closeButton->enabled)
		closeButton->render();
}

void MenuTalker::setHero(StatBlock &stats) {
	hero_name = stats.name;
	hero_class = stats.getShortClass();

	if (portrait)
		delete portrait;

	if (stats.gfx_portrait == "") return;

	Image *graphics;
	graphics = render_device->loadImage(stats.gfx_portrait, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		portrait = graphics->createSprite();
		graphics->unref();
	}
}

void MenuTalker::setNPC(NPC* _npc) {
	if (npc != _npc) {
		first_interaction = true;
	}

	npc = _npc;

	if (_npc == NULL) {
		visible = false;
		first_interaction = false;
		return;
	}

	visible = true;
}

void MenuTalker::createActionButtons(int node_id) {
	if (!npc)
		return;

	clearActionButtons();

	std::vector<int> nodes;
	if (node_id == -1) {
		// primary topic selection
		npc->getDialogNodes(nodes, !NPC::GET_RESPONSE_NODES);
	}
	else {
		// dialog responses
		npc->getDialogResponses(nodes, node_id, event_cursor);
	}

	// add standard topics
	for (size_t i = nodes.size(); i > 0; i--) {
		std::string topic = npc->getDialogTopic(nodes[i-1]);
		if (topic.empty()) {
			topic = msg->get("<dialog node %d>", nodes[i-1]);
		}

		addAction(topic, nodes[i-1], !Action::IS_VENDOR);
	}

	// add "Trade" topic
	if (node_id == -1 && npc->checkVendor()) {
		addAction(msg->get("Trade"), Action::NO_NODE, Action::IS_VENDOR);
	}

	for (size_t i = 0; i< actions.size(); ++i) {
		textbox->addChildWidget(actions[i].btn);
	}
}

void MenuTalker::clearActionButtons() {
	for (size_t i = 0; i < actions.size(); ++i) {
		delete actions[i].btn;
	}
	actions.clear();
	textbox->clearChildWidgets();
}

void MenuTalker::executeAction(size_t index) {
	if (index >= actions.size())
		return;

	int node_id = actions[index].node_id;

	if (actions[index].is_vendor) {
		// begin trading
		NPC *temp_npc = npc;
		menu->closeAll();
		menu->vendor->setNPC(temp_npc);
		menu->inv->visible = true;
	}
	else if (node_id != -1) {
		// begin talking
		chooseDialogNode(node_id);
		if (npc && npc_from_map) {
			pc->allow_movement = npc->checkMovement(node_id);
		}
	}
}

void MenuTalker::nextDialog() {
	bool more = false;

	if (dialog_node != -1) {
		npc->processEvent(dialog_node, event_cursor);
		event_cursor++;
		more = npc->processDialog(dialog_node, event_cursor);
	}
	else {
		more = false;
	}

	if (more)
		createBuffer();
	else {
		if (dialog_node != -1) {
			// return to the topic selection
			int prev_node = dialog_node;
			chooseDialogNode(-1);

			// when returning to the topic selection, a topic is auto-selected if there is only one
			// in this case, we don't want to repeat the same topic, so we check for that here
			if (actions.empty() && dialog_node == prev_node)
				setNPC(NULL);
		}
		else
			setNPC(NULL); // end dialog
	}
}

void MenuTalker::setupTabList() {
	tablist.clear();

	tablist.add(textbox);

	if (advanceButton->enabled) {
		tablist.add(advanceButton);
		tablist.setCurrent(advanceButton);
	}
	else if (closeButton->enabled) {
		tablist.add(closeButton);
		tablist.setCurrent(closeButton);
	}
}

void MenuTalker::addAction(const std::string& label, int node_id, bool is_vendor) {
	if ((node_id != Action::NO_NODE && is_vendor) || (node_id == Action::NO_NODE && !is_vendor)) {
		Utils::logError("MenuTalker: addAction() parameters are incompatible, skipping action.");
		return;
	}

	actions.push_back(Action());

	actions.back().btn = new WidgetButton(WidgetButton::NO_FILE);
	actions.back().btn->setLabel(label);

	if (node_id != Action::NO_NODE) {
		actions.back().node_id = node_id;
		actions.back().btn->setTextColor(WidgetButton::BUTTON_NORMAL, topic_color_normal);
		actions.back().btn->setTextColor(WidgetButton::BUTTON_HOVER, topic_color_hover);
		actions.back().btn->setTextColor(WidgetButton::BUTTON_PRESSED, topic_color_pressed);
	}
	else {
		actions.back().is_vendor = true;
		actions.back().btn->setTextColor(WidgetButton::BUTTON_NORMAL, trade_color_normal);
		actions.back().btn->setTextColor(WidgetButton::BUTTON_HOVER, trade_color_hover);
		actions.back().btn->setTextColor(WidgetButton::BUTTON_PRESSED, trade_color_pressed);
	}
}

MenuTalker::~MenuTalker() {
	clearActionButtons();

	delete portrait;
	delete label_name;
	delete textbox;
	delete advanceButton;
	delete closeButton;
}
