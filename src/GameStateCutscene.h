/*
Copyright © 2012-2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert
Copyright © 2013-2016 Justin Jacobs

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
#include "WidgetButton.h"
#include "WidgetLabel.h"

enum {
	CUTSCENE_STATIC = 0,
	CUTSCENE_VSCROLL = 1
};

class WidgetScrollBox;

class CutsceneSettings {
public:
	FPoint caption_margins;
	Color caption_background;
	bool scale_graphics;
	float vscroll_speed;
	CutsceneSettings()
		: caption_background(0,0,0,200)
		, scale_graphics(false)
		, vscroll_speed(4.0)
	{}
};

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
		, z(0) {
	}
};

class VScrollComponent {
public:
	Point pos;
	Sprite *image;
	Point image_size;
	WidgetLabel *text;
	int separator_h;

	VScrollComponent()
		: image(NULL)
		, text(NULL)
		, separator_h(0)
	{}
};

class Scene {
private:
	CutsceneSettings settings;
	int frame_counter;
	int pause_frames;
	std::string caption;
	Sprite *art;
	Sprite *art_scaled;
	Point art_size;
	SoundManager::SoundID sid;
	WidgetScrollBox *caption_box;
	WidgetButton *button_next;
	WidgetButton *button_close;
	WidgetButton *button_advance;
	bool done;
	int vscroll_offset;
	int vscroll_ticks;

public:
	Scene(const CutsceneSettings& _settings, short _cutscene_type);
	~Scene();
	void refreshWidgets();
	bool logic();
	void render();

	short cutscene_type;
	bool is_last_scene;
	std::queue<SceneComponent> components;
	std::vector<VScrollComponent> vscroll_components;
};

class GameStateCutscene : public GameState {
private:
	GameState *previous_gamestate;
	std::string dest_map;
	Point dest_pos;

	std::queue<Scene*> scenes;

public:
	explicit GameStateCutscene(GameState *game_state);
	~GameStateCutscene();

	bool load(const std::string& filename);
	void logic();
	void render();

	int game_slot;
};

#endif

