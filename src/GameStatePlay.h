/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
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
 * class GameStatePlay
 *
 * Handles logic and rendering of the main action game play
 * Also handles message passing between child objects, often to avoid circular dependencies.
 */

#ifndef GAMESTATEPLAY_H
#define GAMESTATEPLAY_H

#include "CommonIncludes.h"
#include "GameState.h"
#include "Utils.h"

class Avatar;
class Enemy;
class MenuManager;
class NPCManager;
class QuestLog;
class WidgetLabel;

class ActionData;

class Title {
public:
	std::string title;
	int level;
	int power;
	std::vector<StatusID> requires_status;
	std::vector<StatusID> requires_not_status;
	std::string primary_stat_1;
	std::string primary_stat_2;

	Title()
		: title("")
		, level(0)
		, power(0)
		, requires_status()
		, requires_not_status()
		, primary_stat_1("")
		, primary_stat_2("") {
	}
};

class GameStatePlay : public GameState {
private:
	Enemy *enemy;

	NPCManager *npcs;
	QuestLog *quests;

	bool restrictPowerUse();
	void checkEnemyFocus();
	void checkLoot();
	void checkLootDrop();
	void checkTeleport();
	void checkCancel();
	void checkLog();
	void checkBook();
	void checkEquipmentChange();
	void checkTitle();
	void checkUsedItems();
	void checkNotifications();
	void checkNPCInteraction();
	void checkStash();
	void checkCutscene();
	void checkSaveEvent();
	void updateActionBar(unsigned index);
	void loadTitles();
	void resetNPC();
	bool checkPrimaryStat(const std::string& first, const std::string& second);

	int npc_id;

	std::vector<Title> titles;

	std::vector<ActionData> action_queue;

	Timer second_timer;

	bool is_first_map_load;

	static const unsigned UPDATE_ACTIONBAR_ALL = 0;

public:
	GameStatePlay();
	~GameStatePlay();
	void refreshWidgets();

	bool isPaused();
	void logic();
	void render();
	void resetGame();
};

#endif

