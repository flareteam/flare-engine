/*
Copyright © 2011-2012 Clint Bellanger and morris989
Copyright © 2013-2014 Henrik Andersson
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

#ifndef MENU_TALKER_H
#define MENU_TALKER_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "Widget.h"

class CampaignManager;
class MenuNPCActions;
class NPC;
class StatBlock;
class WidgetButton;
class WidgetLabel;
class WidgetScrollBox;

class MenuTalker : public Menu {
private:
	MenuNPCActions *npc_menu;

	Sprite *portrait;
	std::string hero_name;
	std::string hero_class;

	int dialog_node;
	unsigned int event_cursor;

	Rect dialog_pos;
	Rect text_pos;
	Point text_offset;
	Rect portrait_he;
	Rect portrait_you;

	std::string font_who;
	std::string font_dialog;

	WidgetLabel *label_name;
	WidgetScrollBox *textbox;

public:
	explicit MenuTalker(MenuNPCActions *_npc_menu);
	~MenuTalker();
	void align();

	NPC *npc;

	void chooseDialogNode(int requested_node);
	void logic();
	void render();
	void setHero(StatBlock &stats);
	void createBuffer();
	void setNPC(NPC* _npc);

	WidgetButton *advanceButton;
	WidgetButton *closeButton;
};

#endif
