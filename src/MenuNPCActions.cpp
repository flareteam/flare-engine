/*
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert
Copyright © 2013-2015 Justin Jacobs

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
 * class MenuNPCActions
 */
#include "CommonIncludes.h"
#include "FileParser.h"
#include "Menu.h"
#include "MenuNPCActions.h"
#include "NPC.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

#define SEPARATOR_HEIGHT 2
#define ITEM_SPACING 2
#define MENU_BORDER 8

class Action {
public:
	Action(const std::string& _id = "", std::string _label = "")
		: id(_id)
		, label(id != "" ? new WidgetLabel() : NULL) {
		if (label)
			label->set(_label);
	}

	Action(const Action &r)
		: id(r.id)
		, label(id != "" ? new WidgetLabel() : NULL) {
		if (label)
			label->set(r.label->get());

		rect = r.rect;
	}

	virtual ~Action() {
		delete label;
	}

	std::string id;
	WidgetLabel *label;
	Rect rect;
};

MenuNPCActions::MenuNPCActions()
	: Menu()
	, npc(NULL)
	, is_selected(false)
	, is_empty(true)
	, first_dialog_node(-1)
	, current_action(-1)
	, action_menu(NULL)
	, vendor_label(msg->get("Trade"))
	, cancel_label(msg->get("Cancel"))
	, dialog_selected(false)
	, vendor_selected(false)
	, cancel_selected(false)
	, selected_dialog_node(-1) {
	// Load config settings
	FileParser infile;
	// @CLASS MenuNPCActions|Description of menus/npc.txt
	if (infile.open("menus/npc.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR background_color|color, int : Color, Alpha|Color and alpha of the menu's background.
			if(infile.key == "background_color") background_color = toRGBA(infile.val);
			// @ATTR topic_normal_color|color|The normal color of a generic topic text.
			else if(infile.key == "topic_normal_color") topic_normal_color = toRGB(infile.val);
			// @ATTR topic_hilight_color|color|The color of generic topic text when it's hovered over or selected.
			else if(infile.key == "topic_hilight_color") topic_hilight_color = toRGB(infile.val);
			// @ATTR vendor_normal_color|color|The normal color of the vendor option text.
			else if(infile.key == "vendor_normal_color") vendor_normal_color = toRGB(infile.val);
			// @ATTR vendor_hilight_color|color|The color of vendor option text when it's hovered over or selected.
			else if(infile.key == "vendor_hilight_color") vendor_hilight_color = toRGB(infile.val);
			// @ATTR cancel_normal_color|color|The normal color of the option to close the menu.
			else if(infile.key == "cancel_normal_color") cancel_normal_color = toRGB(infile.val);
			// @ATTR cancel_hilight_color|color|The color of the option to close the menu when it's hovered over or selected.
			else if(infile.key == "cancel_hilight_color") cancel_hilight_color = toRGB(infile.val);

			else infile.error("MenuNPCActions: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	// save the x/y pos coordinates here, since we call align() during each update() call
	base_pos.x = window_area.x;
	base_pos.y = window_area.y;
}

void MenuNPCActions::update() {
	/* get max width and height of action menu */
	int w = 0, h = 0;
	for(size_t i=0; i<npc_actions.size(); i++) {
		h += ITEM_SPACING;
		if (npc_actions[i].label) {
			w = std::max(static_cast<int>(npc_actions[i].label->bounds.w), w);
			h += npc_actions[i].label->bounds.h;
		}
		else
			h += SEPARATOR_HEIGHT;

		h += ITEM_SPACING;
	}

	/* set action menu position */
	window_area.x = base_pos.x;
	window_area.y = base_pos.y;
	window_area.w = w+(MENU_BORDER*2);
	window_area.h = h+(MENU_BORDER*2);

	align();

	/* update all action menu items */
	int yoffs = MENU_BORDER;
	Color text_color;
	for(size_t i=0; i<npc_actions.size(); i++) {
		npc_actions[i].rect.x = window_area.x + MENU_BORDER;
		npc_actions[i].rect.y = window_area.y + yoffs;
		npc_actions[i].rect.w = w;

		if (npc_actions[i].label) {
			npc_actions[i].rect.h = npc_actions[i].label->bounds.h + (ITEM_SPACING*2);

			if (i == current_action) {
				if (npc_actions[i].id == "id_cancel")
					text_color = cancel_hilight_color;
				else if (npc_actions[i].id == "id_vendor")
					text_color = vendor_hilight_color;
				else
					text_color = topic_hilight_color;

				npc_actions[i].label->set(MENU_BORDER + (w/2),
										  yoffs + (npc_actions[i].rect.h/2) ,
										  JUSTIFY_CENTER, VALIGN_CENTER,
										  npc_actions[i].label->get(), text_color);
			}
			else {
				if (npc_actions[i].id == "id_cancel")
					text_color = cancel_normal_color;
				else if (npc_actions[i].id == "id_vendor")
					text_color = vendor_normal_color;
				else
					text_color = topic_normal_color;

				npc_actions[i].label->set(MENU_BORDER + (w/2),
										  yoffs + (npc_actions[i].rect.h/2),
										  JUSTIFY_CENTER, VALIGN_CENTER,
										  npc_actions[i].label->get(), text_color);
			}

		}
		else
			npc_actions[i].rect.h = SEPARATOR_HEIGHT + (ITEM_SPACING*2);

		yoffs += npc_actions[i].rect.h;
	}

	w += (MENU_BORDER*2);
	h += (MENU_BORDER*2);

	int old_w = -1, old_h = -1;
	if (action_menu) {
		old_w = action_menu->getGraphicsWidth();
		old_h = action_menu->getGraphicsHeight();
	}
	// create background surface if necessary
	if ( old_w != w || old_h != h ) {
		if (action_menu) {
			delete action_menu;
			action_menu = NULL;
		}
		Image *graphics = render_device->createImage(w,h);
		if (graphics) {
			graphics->fillWithColor(background_color);
			action_menu = graphics->createSprite();
			graphics->unref();
		}
	}

}

void MenuNPCActions::setNPC(NPC *pnpc) {
	if (npc == pnpc)
		return;

	// clear actions menu
	npc_actions.clear();

	// reset states
	is_empty = true;
	is_selected = false;
	int topics = 0;
	first_dialog_node = -1;
	current_action = -1;

	npc = pnpc;

	if (npc == NULL) {
		visible = false;
		return;
	}

	// reset selection
	dialog_selected = vendor_selected = cancel_selected = false;

	/* enumerate available dialog topics */
	std::vector<int> nodes;
	npc->getDialogNodes(nodes);
	for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; i--) {

		if (first_dialog_node == -1 && topics == 0)
			first_dialog_node = nodes[i];

		std::string topic = npc->getDialogTopic(nodes[i]);
		if (topic.length() == 0)
			continue;

		std::stringstream ss;
		ss.str("");
		ss << "id_dialog_" << nodes[i];

		npc_actions.push_back(Action(ss.str(), topic));
		topics++;
		is_empty = false;
	}

	if (first_dialog_node != -1 && topics == 0)
		topics = 1;

	/* if npc is a vendor add entry */
	bool can_trade = npc->checkVendor();
	if (can_trade) {
		if (topics)
			npc_actions.push_back(Action());
		npc_actions.push_back(Action("id_vendor", vendor_label));
		is_empty = false;
	}

	npc_actions.push_back(Action());
	npc_actions.push_back(Action("id_cancel", cancel_label));

	/* if npc is not a vendor and only one topic is
	 available select the topic automatically */
	if (!can_trade && topics == 1) {
		dialog_selected = true;
		selected_dialog_node = first_dialog_node;
		is_selected = true;
		return;
	}

	/* if there is no dialogs and npc is a vendor set
	 vendor_selected automatically */
	if (can_trade && topics == 0) {
		vendor_selected = true;
		is_selected = true;
		return;
	}

	update();

}

bool MenuNPCActions::empty() {
	return is_empty;
}

bool MenuNPCActions::selection() {
	return is_selected;
}

void MenuNPCActions::logic() {
	if (!visible) return;

	if (!inpt->usingMouse()) {
		if (inpt->lock[ACCEPT])
			return;
		keyboardLogic();
	}
	else {
		if (inpt->lock[MAIN1])
			return;

		/* get action under mouse */
		bool got_action = false;
		for (size_t i=0; i<npc_actions.size(); i++) {

			if (!isWithinRect(npc_actions[i].rect, inpt->mouse))
				continue;

			got_action = true;

			if (current_action != i) {
				current_action = i;
				update();
			}

			break;
		}

		/* if we dont have an action under mouse skip main1 check */
		if (!got_action) {
			current_action = -1;
			update();
			return;
		}
	}

	/* is main1 pressed */
	if (static_cast<int>(current_action) > -1 && ((inpt->pressing[MAIN1] && inpt->usingMouse()) || (inpt->pressing[ACCEPT] && NO_MOUSE))) {
		if (inpt->pressing[MAIN1]) inpt->lock[MAIN1] = true;
		if (inpt->pressing[ACCEPT]) inpt->lock[ACCEPT] = true;


		if (npc_actions[current_action].label == NULL)
			return;
		else if (npc_actions[current_action].id == "id_cancel")
			cancel_selected = true;

		else if (npc_actions[current_action].id == "id_vendor")
			vendor_selected = true;

		else if (npc_actions[current_action].id.compare("id_dialog_")) {
			dialog_selected = true;
			std::stringstream ss;
			std::string tmp(10,' ');
			ss.str(npc_actions[current_action].id);
			ss.read(&tmp[0], 10);
			ss >> selected_dialog_node;
		}

		is_selected = true;
	}

}

void MenuNPCActions::keyboardLogic() {
	if (inpt->pressing[LEFT]) inpt->lock[LEFT] = true;
	if (inpt->pressing[RIGHT]) inpt->lock[RIGHT] = true;

	if (inpt->pressing[UP] && !inpt->lock[UP]) {
		inpt->lock[UP] = true;
		do {
			current_action--;
			if (static_cast<int>(current_action) < 0)
				current_action = npc_actions.size()-1;
		}
		while (npc_actions[current_action].label == NULL);
	}
	if (inpt->pressing[DOWN] && !inpt->lock[DOWN]) {
		inpt->lock[DOWN] = true;
		do {
			current_action++;
			if (current_action >= npc_actions.size())
				current_action = 0;
		}
		while (npc_actions[current_action].label == NULL);
	}
	update();
}

void MenuNPCActions::render() {
	if (!visible) return;
	if (!action_menu) return;

	action_menu->setDest(window_area);
	render_device->render(action_menu);
	for(size_t i=0; i<npc_actions.size(); i++) {
		if (npc_actions[i].label) {
			npc_actions[i].label->local_frame.x = window_area.x;
			npc_actions[i].label->local_frame.y = window_area.y;
			npc_actions[i].label->render();
		}
	}
}

MenuNPCActions::~MenuNPCActions() {
	if (action_menu)
		delete action_menu;
}

