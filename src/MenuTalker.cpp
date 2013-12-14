/*
Copyright © 2011-2012 Clint Bellanger and morris989
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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
 * class MenuTalker
 */

#include "FileParser.h"
#include "Menu.h"
#include "MenuTalker.h"

#include "NPC.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetScrollBox.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"
#include "MenuManager.h"
#include "MenuNPCActions.h"

using namespace std;


MenuTalker::MenuTalker(MenuManager *_menu)
	: Menu()
	, menu(_menu)
	, dialog_node(0)
	, event_cursor(0)
	, font_who("font_regular")
	, font_dialog("font_regular")
	, color_normal(font->getColor("menu_normal"))
	, npc(NULL)
	, vendor_visible(false)
	, advanceButton(new WidgetButton("images/menus/buttons/right.png"))
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png")) {

	background.setGraphics(loadGraphicSurface("images/menus/dialog_box.png"));

	// Load config settings
	FileParser infile;
	if(infile.open("menus/talker.txt")) {
		while(infile.next()) {
			infile.val = infile.val + ',';

			if(infile.key == "close") {
				close_pos.x = eatFirstInt(infile.val,',');
				close_pos.y = eatFirstInt(infile.val,',');
			}
			else if(infile.key == "advance") {
				advance_pos.x = eatFirstInt(infile.val,',');
				advance_pos.y = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "dialogbox") {
				dialog_pos.x = eatFirstInt(infile.val,',');
				dialog_pos.y = eatFirstInt(infile.val,',');
				dialog_pos.w = eatFirstInt(infile.val,',');
				dialog_pos.h = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "dialogtext") {
				text_pos.x = eatFirstInt(infile.val,',');
				text_pos.y = eatFirstInt(infile.val,',');
				text_pos.w = eatFirstInt(infile.val,',');
				text_pos.h = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "text_offset") {
				text_offset.x = eatFirstInt(infile.val,',');
				text_offset.y = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "portrait_he") {
				portrait_he.x = eatFirstInt(infile.val,',');
				portrait_he.y = eatFirstInt(infile.val,',');
				portrait_he.w = eatFirstInt(infile.val,',');
				portrait_he.h = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "portrait_you") {
				portrait_you.x = eatFirstInt(infile.val,',');
				portrait_you.y = eatFirstInt(infile.val,',');
				portrait_you.w = eatFirstInt(infile.val,',');
				portrait_you.h = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "font_who") {
				font_who = eatFirstString(infile.val,',');
			}
			else if (infile.key == "font_dialog") {
				font_dialog = eatFirstString(infile.val,',');
			}
		}
		infile.close();
	}

	label_name = new WidgetLabel();
	textbox = new WidgetScrollBox(text_pos.w, text_pos.h-(text_offset.y*2));

	tablist.add(advanceButton);
	tablist.add(closeButton);
	tablist.add(textbox);
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

void MenuTalker::update() {
	advanceButton->pos.x = window_area.x + advance_pos.x;
	advanceButton->pos.y = window_area.y + advance_pos.y;

	closeButton->pos.x = window_area.x + close_pos.x;
	closeButton->pos.y = window_area.y + close_pos.y;

	label_name->set(window_area.x+text_pos.x+text_offset.x, window_area.y+text_pos.y+text_offset.y, JUSTIFY_LEFT, VALIGN_TOP, "", color_normal, font_who);

	textbox->pos.x = window_area.x + text_pos.x;
	textbox->pos.y = window_area.y + text_pos.y+text_offset.y+label_name->bounds.h;
	textbox->pos.h -= label_name->bounds.h;
}
/**
 * Menu interaction (enter/space/click to continue)
 */
void MenuTalker::logic() {

	if (!visible || npc==NULL) return;

	if (NO_MOUSE) {
		tablist.logic();
	}

	advanceButton->enabled = false;
	closeButton->enabled = false;

	// determine active button
	if (event_cursor < npc->dialog[dialog_node].size()-1) {
		if (npc->dialog[dialog_node][event_cursor+1].type != "") {
			advanceButton->enabled = true;
			tablist.remove(closeButton);
			tablist.add(advanceButton);
		}
		else {
			closeButton->enabled = true;
			tablist.remove(advanceButton);
			tablist.add(closeButton);
		}
	}
	else {
		closeButton->enabled = true;
		tablist.remove(advanceButton);
		tablist.add(closeButton);
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
		npc = NULL;
		visible = false;
	}
}

void MenuTalker::createBuffer() {
	if (event_cursor >= npc->dialog[dialog_node].size()) return;

	string line;

	// speaker name
	string etype = npc->dialog[dialog_node][event_cursor].type;
	string who;

	if (etype == "him" || etype == "her") {
		who = npc->name;
	}
	else if (etype == "you") {
		who = hero_name;
	}

	label_name->set(who);

	line = parseLine(npc->dialog[dialog_node][event_cursor].s);

	// render dialog text to the scrollbox buffer
	Point line_size = font->calc_size(line,textbox->pos.w-(text_offset.x*2));
	textbox->resize(line_size.y);
	textbox->line_height = font->getLineHeight();
	font->setFont(font_dialog);
	font->render(
		line,
		text_offset.x,
		0,
		JUSTIFY_LEFT,
		textbox->contents.getGraphics(),
		text_pos.w - text_offset.x*2,
		color_normal
	);

}

void MenuTalker::render() {
	if (!visible) return;
	SDL_Rect src;
	SDL_Rect dest;

	int offset_x = window_area.x;
	int offset_y = window_area.y;

	// dialog box
	src.x = 0;
	src.y = 0;
	dest.x = offset_x + dialog_pos.x;
	dest.y = offset_y + dialog_pos.y;
	src.w = dest.w = dialog_pos.w;
	src.h = dest.h = dialog_pos.h;

	background.setClip(src);
	background.setDest(dest);
	render_device->render(background);

	// show active portrait
	string etype = npc->dialog[dialog_node][event_cursor].type;
	if (etype == "him" || etype == "her") {
		Renderable& r = npc->portrait;
		if (!r.graphicsIsNull()) {
			src.w = dest.w = portrait_he.w;
			src.h = dest.h = portrait_he.h;
			dest.x = offset_x + portrait_he.x;
			dest.y = offset_y + portrait_he.y;

			r.setClip(src);
			r.setDest(dest);
			render_device->render(r);
		}
	}
	else if (etype == "you") {
		if (!portrait.graphicsIsNull()) {
			src.w = dest.w = portrait_you.w;
			src.h = dest.h = portrait_you.h;
			dest.x = offset_x + portrait_you.x;
			dest.y = offset_y + portrait_you.y;
			portrait.setClip(src);
			portrait.setDest(dest);
			render_device->render(portrait);
		}
	}

	// name & dialog text
	label_name->render();
	textbox->render();

	// show advance button if there are more event components, or close button if not
	if (event_cursor < npc->dialog[dialog_node].size()-1) {
		if (npc->dialog[dialog_node][event_cursor+1].type != "") {
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

void MenuTalker::setHero(const string& name, const string& class_name, const string& portrait_filename) {
	hero_name = name;
	hero_class = msg->get(class_name);

	portrait.clearGraphics();
	portrait.setGraphics(loadGraphicSurface("images/portraits/" + portrait_filename + ".png", "Couldn't load portrait"));
}

string MenuTalker::parseLine(const string &line) {
	string new_line = line;

	// name
	size_t index = new_line.find("%N");
	if (index != string::npos) new_line = new_line.replace(index, 2, hero_name);

	// class
	index = new_line.find("%C");
	if (index != string::npos) new_line.replace(index, 2, hero_class);

	return new_line;
}

MenuTalker::~MenuTalker() {
	delete label_name;
	delete textbox;
	delete advanceButton;
	delete closeButton;
}
