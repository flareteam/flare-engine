/*
Copyright © 2011-2012 Clint Bellanger and morris989
Copyright © 2012 Stefan Beller

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
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"

using namespace std;


MenuTalker::MenuTalker(CampaignManager *_camp) {
	camp = _camp;
	npc = NULL;
	background = NULL;
	portrait = NULL;
	msg_buffer = NULL;

	advanceButton = new WidgetButton(mods->locate("images/menus/buttons/right.png"));

	closeButton = new WidgetButton(mods->locate("images/menus/buttons/button_x.png"));

	vendorButton = new WidgetButton(mods->locate("images/menus/buttons/button_default.png"));
	vendorButton->label = msg->get("Trade");

	visible = false;
	vendor_visible = false;
	has_vendor_button = false;

	// step through NPC dialog nodes
	dialog_node = 0;
	event_cursor = 0;

	loadGraphics();

	// Load config settings
	FileParser infile;
	if(infile.open(mods->locate("menus/talker.txt"))) {
		while(infile.next()) {
			infile.val = infile.val + ',';

			if(infile.key == "close") {
				close_pos.x = eatFirstInt(infile.val,',');
				close_pos.y = eatFirstInt(infile.val,',');
			} else if(infile.key == "advance") {
				advance_pos.x = eatFirstInt(infile.val,',');
				advance_pos.y = eatFirstInt(infile.val,',');
			} else if (infile.key == "vendor"){
				vendor_pos.x = eatFirstInt(infile.val,',');
				vendor_pos.y = eatFirstInt(infile.val,',');
			} else if (infile.key == "dialogbox"){
				dialog_pos.x = eatFirstInt(infile.val,',');
				dialog_pos.y = eatFirstInt(infile.val,',');
				dialog_pos.w = eatFirstInt(infile.val,',');
				dialog_pos.h = eatFirstInt(infile.val,',');
			} else if (infile.key == "dialogtext"){
				text_pos.x = eatFirstInt(infile.val,',');
				text_pos.y = eatFirstInt(infile.val,',');
				text_pos.w = eatFirstInt(infile.val,',');
				text_pos.h = eatFirstInt(infile.val,',');
			} else if (infile.key == "text_offset"){
				text_offset.x = eatFirstInt(infile.val,',');
				text_offset.y = eatFirstInt(infile.val,',');
			} else if (infile.key == "portrait_he"){
				portrait_he.x = eatFirstInt(infile.val,',');
				portrait_he.y = eatFirstInt(infile.val,',');
				portrait_he.w = eatFirstInt(infile.val,',');
				portrait_he.h = eatFirstInt(infile.val,',');
			} else if (infile.key == "portrait_you"){
				portrait_you.x = eatFirstInt(infile.val,',');
				portrait_you.y = eatFirstInt(infile.val,',');
				portrait_you.w = eatFirstInt(infile.val,',');
				portrait_you.h = eatFirstInt(infile.val,',');
			}
		}
		infile.close();
	} else fprintf(stderr, "Unable to open menus/talker.txt!\n");

	color_normal = font->getColor("menu_normal");
}

void MenuTalker::loadGraphics() {

	SDL_FreeSurface(background);
	background = IMG_Load(mods->locate("images/menus/dialog_box.png").c_str());
	if(!background) {
		fprintf(stderr, "Couldn't load image dialog_box.png: %s\n", IMG_GetError());
	} else {
		// optimize
		SDL_Surface *cleanup = background;
		background = SDL_DisplayFormatAlpha(background);
		SDL_FreeSurface(cleanup);
	}
}

void MenuTalker::chooseDialogNode() {
	event_cursor = 0;
	dialog_node = npc->chooseDialogNode();
	npc->processDialog(dialog_node, event_cursor);
	createBuffer();
}

void MenuTalker::update() {
	advanceButton->pos.x = window_area.x + advance_pos.x;
	advanceButton->pos.y = window_area.y + advance_pos.y;

	closeButton->pos.x = window_area.x + close_pos.x;
	closeButton->pos.y = window_area.y + close_pos.y;

	vendorButton->pos.x = window_area.x + vendor_pos.x;
	vendorButton->pos.y = window_area.y + vendor_pos.y;
	vendorButton->refresh();
}
/**
 * Menu interaction (enter/space/click to continue)
 */
void MenuTalker::logic() {

	if (!visible || npc==NULL) return;

	advanceButton->enabled = false;
	closeButton->enabled = false;

	// determine active button
	if (event_cursor < npc->dialog[dialog_node].size()-1) {
		if (npc->dialog[dialog_node][event_cursor+1].type != "") {
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

	if (has_vendor_button) {
		if (vendorButton->checkClick())
			vendor_visible = true;
	}
	if (advanceButton->checkClick() || closeButton->checkClick()) {
		// button was clicked
		event_cursor++;
		more = npc->processDialog(dialog_node, event_cursor);
	}
	else if	(inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
		inpt->lock[ACCEPT] = true;
		// pressed next/more
		event_cursor++;
		more = npc->processDialog(dialog_node, event_cursor);
	}
	else {
		return;
	}

	if (more) {
		createBuffer();
	}
	else {
		// end dialog
		npc = NULL;
		visible = false;
	}
}

void MenuTalker::createBuffer() {

	string line;

	// speaker name
	string etype = npc->dialog[dialog_node][event_cursor].type;
	if (etype == "him" || etype == "her") {
		line = npc->name + ": ";
	}
	else if (etype == "you") {
		line = hero_name + ": ";
	}

	line = line + npc->dialog[dialog_node][event_cursor].s;

	// render text to back buffer
	SDL_FreeSurface(msg_buffer);
	msg_buffer = createAlphaSurface(text_pos.w,text_pos.h);
	font->setFont("font_regular");
	font->render(line, text_offset.x, text_offset.y, JUSTIFY_LEFT, msg_buffer, text_pos.w - text_offset.x*2, color_normal);

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
	SDL_BlitSurface(background, &src, screen, &dest);

	// show active portrait
	string etype = npc->dialog[dialog_node][event_cursor].type;
	if (etype == "him" || etype == "her") {
		if (npc->portrait != NULL) {
			src.w = dest.w = portrait_he.w;
			src.h = dest.h = portrait_he.h;
			dest.x = offset_x + portrait_he.x;
			dest.y = offset_y + portrait_he.y;
			SDL_BlitSurface(npc->portrait, &src, screen, &dest);
		}
	}
	else if (etype == "you") {
		if (portrait != NULL) {
			src.w = dest.w = portrait_you.w;
			src.h = dest.h = portrait_you.h;
			dest.x = offset_x + portrait_you.x;
			dest.y = offset_y + portrait_you.y;
			SDL_BlitSurface(portrait, &src, screen, &dest);
		}
	}

	// text overlay
	dest.x = offset_x + text_pos.x;
	dest.y = offset_y + text_pos.y;
	SDL_BlitSurface(msg_buffer, NULL, screen, &dest);

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

	// show the vendor button if the npc is a vendor
	if (has_vendor_button)
		vendorButton->render();
}

void MenuTalker::setHero(const string& name, const string& portrait_filename) {
	hero_name = name;

	SDL_FreeSurface(portrait);
	portrait = IMG_Load(mods->locate("images/portraits/" + portrait_filename + ".png").c_str());
	if(!portrait) {
		fprintf(stderr, "Couldn't load portrait: %s\n", IMG_GetError());

		// keep playing, just don't show this portrait
	}
	else {
		// optimize
		SDL_Surface *cleanup = portrait;
		portrait = SDL_DisplayFormatAlpha(portrait);
		SDL_FreeSurface(cleanup);
	}
}

MenuTalker::~MenuTalker() {
	SDL_FreeSurface(msg_buffer);
	SDL_FreeSurface(background);
	SDL_FreeSurface(portrait);
	delete advanceButton;
	delete closeButton;
	delete vendorButton;
}
