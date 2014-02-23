/*
Copyright © 2012-2013 Henrik Andersson
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

#ifndef GAMESTATECUTSCENE_H
#define GAMESTATECUTSCENE_H

#include "CommonIncludes.h"
#include "GameState.h"
#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"
#include "UtilsParsing.h"

#include <list>

class WidgetScrollBox;

class SceneComponent {
public:
	std::string type;
	std::string s;
	int x,y,z;
	SceneComponent()
		: type("")
		, s("")
		, x(0)
		, y(0)
		, z(0)
	{}
};

class Scene {
private:
	Image *loadImage(std::string filename, bool scale_graphics);

	int frame_counter;
	int pause_frames;
	std::string caption;
	Point caption_size;
	Sprite *art;
	Rect art_dest;
	SoundManager::SoundID sid;
	WidgetScrollBox *caption_box;
	bool done;

public:
	Scene();
	~Scene();
	bool logic(FPoint *caption_margins, bool scale_graphics);
	void render();

	std::queue<SceneComponent> components;

};

class GameStateCutscene : public GameState {
private:
	GameState *previous_gamestate;
	std::string dest_map;
	Point dest_pos;
	bool scale_graphics;
	FPoint caption_margins;

	std::queue<Scene> scenes;

public:
	GameStateCutscene(GameState *game_state);
	~GameStateCutscene();

	bool load(std::string filename);
	void logic();
	void render();

	int game_slot;
};

#endif

