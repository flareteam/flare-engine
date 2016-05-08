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

#include "FileParser.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuNPCActions.h"
#include "MenuTalker.h"
#include "NPC.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetScrollBox.h"

MenuTalker::MenuTalker(MenuManager *_menu)
	: Menu()
	, menu(_menu)
	, portrait(NULL)
	, dialog_node(0)
	, event_cursor(0)
	, font_who("font_regular")
	, font_dialog("font_regular")
	, color_normal(font->getColor("menu_normal"))
	, npc(NULL)
	, advanceButton(new WidgetButton("images/menus/buttons/right.png"))
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png")) {

	setBackground("images/menus/dialog_box.png");

	// Load config settings
	FileParser infile;
	// @CLASS MenuTalker|Description of menus/talker.txt
	if(infile.open("menus/talker.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR advance|point|Position of the button to advance dialog.
			else if(infile.key == "advance") {
				Point pos = toPoint(infile.val);
				advanceButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR dialogbox|rectangle|Position and dimensions of the text box graphics.
			else if (infile.key == "dialogbox") dialog_pos = toRect(infile.val);
			// @ATTR dialogtext|rectangle|Rectangle where the dialog text is placed.
			else if (infile.key == "dialogtext") text_pos = toRect(infile.val);
			// @ATTR text_offset|point|Margins for the left/right and top/bottom of the dialog text.
			else if (infile.key == "text_offset") text_offset = toPoint(infile.val);
			// @ATTR portrait_he|rectangle|Position and dimensions of the NPC portrait graphics.
			else if (infile.key == "portrait_he") portrait_he = toRect(infile.val);
			// @ATTR portrait_you|rectangle|Position and dimensions of the player's portrait graphics.
			else if (infile.key == "portrait_you") portrait_you = toRect(infile.val);
			// @ATTR font_who|string|Font style to use for the name of the currently talking person.
			else if (infile.key == "font_who") font_who = infile.val;
			// @ATTR font_dialog|string|Font style to use for the dialog text.
			else if (infile.key == "font_dialog") font_dialog = infile.val;

			else infile.error("MenuTalker: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	label_name = new WidgetLabel();
	label_name->setBasePos(text_pos.x + text_offset.x, text_pos.y + text_offset.y);

	textbox = new WidgetScrollBox(text_pos.w, text_pos.h-(text_offset.y*2));
	textbox->setBasePos(text_pos.x, text_pos.y + text_offset.y);

	align();
}

void MenuTalker::align() {
	Menu::align();

	advanceButton->setPos(window_area.x, window_area.y);
	closeButton->setPos(window_area.x, window_area.y);

	label_name->setPos(window_area.x, window_area.y);

	textbox->setPos(window_area.x, window_area.y + label_name->bounds.h);
	textbox->pos.h = text_pos.h - (text_offset.y*2);
}

void MenuTalker::chooseDialogNode(int request_dialog_node) {
	event_cursor = 0;

	if(request_dialog_node == -1)
		return;

	dialog_node = request_dialog_node;
	npc->processEvent(dialog_node, event_cursor);
	npc->processDialog(dialog_node, event_cursor);
	createBuffer();
}

/**
 * Menu interaction (enter/space/click to continue)
 */
void MenuTalker::logic() {

	if (!visible || npc==NULL) return;

	advanceButton->enabled = false;
	closeButton->enabled = false;

	// determine active button
	if (static_cast<unsigned>(dialog_node) < npc->dialog.size() && !npc->dialog[dialog_node].empty() && event_cursor < npc->dialog[dialog_node].size()-1) {
		if (npc->dialog[dialog_node][event_cursor+1].type != EC_NONE) {
			advanceButton->enabled = true;
		}
		else {
			closeButton->enabled = true;
		}
	}
	else {
		closeButton->enabled = true;
	}

	bool more;
	if (advanceButton->checkClick() || closeButton->checkClick()) {
		// button was clicked
		npc->processEvent(dialog_node, event_cursor);
		event_cursor++;
		more = npc->processDialog(dialog_node, event_cursor);
	}
	else if	(inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
		inpt->lock[ACCEPT] = true;
		// pressed next/more
		npc->processEvent(dialog_node, event_cursor);
		event_cursor++;
		more = npc->processDialog(dialog_node, event_cursor);
	}
	else {
		textbox->logic();
		return;
	}

	if (more) {
		createBuffer();
	}
	else {
		// show the NPC Action Menu
		menu->npc->setNPC(npc);

		if (!menu->npc->selection())
			menu->npc->visible = true;
		else
			menu->npc->setNPC(NULL);

		// end dialog
		setNPC(NULL);
	}
}

void MenuTalker::createBuffer() {
	if (static_cast<unsigned>(dialog_node) >= npc->dialog.size() || event_cursor >= npc->dialog[dialog_node].size())
		return;

	std::string line;

	// speaker name
	EVENT_COMPONENT_TYPE etype = npc->dialog[dialog_node][event_cursor].type;
	std::string who;

	if (etype == EC_NPC_DIALOG_THEM) {
		who = npc->name;
	}
	else if (etype == EC_NPC_DIALOG_YOU) {
		who = hero_name;
	}

	label_name->set(window_area.x+text_pos.x+text_offset.x, window_area.y+text_pos.y+text_offset.y, JUSTIFY_LEFT, VALIGN_TOP, who, color_normal, font_who);


	line = substituteVarsInString(npc->dialog[dialog_node][event_cursor].s, pc);

	// render dialog text to the scrollbox buffer
	Point line_size = font->calc_size(line,textbox->pos.w-(text_offset.x*2));
	textbox->resize(textbox->pos.w, line_size.y);
	textbox->line_height = font->getLineHeight();
	font->setFont(font_dialog);
	font->render(
		line,
		text_offset.x,
		0,
		JUSTIFY_LEFT,
		textbox->contents->getGraphics(),
		text_pos.w - text_offset.x*2,
		color_normal
	);

	align();
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
		EVENT_COMPONENT_TYPE etype = npc->dialog[dialog_node][event_cursor].type;
		if (etype == EC_NPC_DIALOG_THEM) {
			Sprite *r = npc->portrait;
			if (r) {
				src.w = dest.w = portrait_he.w;
				src.h = dest.h = portrait_he.h;
				dest.x = offset_x + portrait_he.x;
				dest.y = offset_y + portrait_he.y;

				r->setClip(src);
				r->setDest(dest);
				render_device->render(r);
			}
		}
		else if (etype == EC_NPC_DIALOG_YOU) {
			if (portrait) {
				src.w = dest.w = portrait_you.w;
				src.h = dest.h = portrait_you.h;
				dest.x = offset_x + portrait_you.x;
				dest.y = offset_y + portrait_you.y;
				portrait->setClip(src);
				portrait->setDest(dest);
				render_device->render(portrait);
			}
		}
	}

	// name & dialog text
	label_name->render();
	textbox->render();

	// show advance button if there are more event components, or close button if not
	if (static_cast<unsigned>(dialog_node) < npc->dialog.size() && !npc->dialog[dialog_node].empty() && event_cursor < npc->dialog[dialog_node].size()-1) {
		if (npc->dialog[dialog_node][event_cursor+1].type != EC_NONE) {
			advanceButton->render();
		}
		else {
			closeButton->render();
		}
	}
	else {
		closeButton->render();
	}
}

void MenuTalker::setHero(StatBlock &stats) {
	hero_name = stats.name;
	hero_class = stats.getShortClass();

	if (portrait)
		delete portrait;

	if (stats.gfx_portrait == "") return;

	Image *graphics;
	graphics = render_device->loadImage(stats.gfx_portrait, "Couldn't load portrait");
	if (graphics) {
		portrait = graphics->createSprite();
		graphics->unref();
	}
}

void MenuTalker::setNPC(NPC* _npc) {
	npc = _npc;

	if (_npc == NULL) {
		visible = false;
		return;
	}

	if (!visible) {
		visible = true;
	}
}

MenuTalker::~MenuTalker() {
	if (portrait) delete portrait;
	delete label_name;
	delete textbox;
	delete advanceButton;
	delete closeButton;
}
