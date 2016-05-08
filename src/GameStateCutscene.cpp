/*
Copyright © 2012-2014 Henrik Andersson
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

#include "CommonIncludes.h"
#include "GameStateCutscene.h"
#include "GameStatePlay.h"
#include "FileParser.h"
#include "WidgetScrollBox.h"
#include "SharedGameResources.h"
#include "SaveLoad.h"

Scene::Scene(const FPoint& _caption_margins, bool _scale_graphics)
	: frame_counter(0)
	, pause_frames(0)
	, caption("")
	, caption_size(0,0)
	, art(NULL)
	, art_scaled(NULL)
	, sid(-1)
	, caption_box(NULL)
	, done(false)
	, caption_margins(_caption_margins)
	, scale_graphics(_scale_graphics)
{
}

Scene::~Scene() {
	if (art) delete art;
	if (art_scaled) delete art_scaled;
	delete caption_box;
	while(!components.empty()) {
		components.pop();
	}
}

bool Scene::logic() {
	if (done) return false;

	bool skip = false;
	if (inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
		inpt->lock[MAIN1] = true;
		skip = true;
	}
	if (inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
		inpt->lock[ACCEPT] = true;
		skip = true;
	}
	if (inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
		inpt->lock[CANCEL] = true;
		done = true;
	}

	/* Pause until specified frame */
	if (!skip && pause_frames != 0 && frame_counter < pause_frames) {
		++frame_counter;
		return true;
	}

	/* parse scene components until next pause */
	while (!components.empty() && components.front().type != "pause") {

		if (components.front().type == "caption") {
			caption = components.front().s;
		}
		else if (components.front().type == "image") {
			if (art) {
				delete art;
				art = NULL;
			}
			if (art_scaled) {
				delete art_scaled;
				art_scaled = NULL;
			}
			Image *graphics = render_device->loadImage(components.front().s);
			if (graphics != NULL) {
				art = graphics->createSprite();
				art_size.x = art->getGraphicsWidth();
				art_size.y = art->getGraphicsHeight();
				graphics->unref();
			}
		}
		else if (components.front().type == "soundfx") {
			if (sid != 0)
				snd->unload(sid);

			sid = snd->load(components.front().s, "Cutscenes");
			snd->play(sid);
		}

		components.pop();
	}

	/* check if current scene has reached the end */
	if (components.empty())
		return false;

	/* setup frame pausing */
	frame_counter = 0;
	pause_frames = components.front().x;
	components.pop();

	refreshWidgets();

	return true;
}

void Scene::refreshWidgets() {
	if (!caption.empty()) {
		int caption_width = VIEW_W - static_cast<int>(VIEW_W * (caption_margins.x * 2.0f));
		font->setFont("font_captions");
		caption_size = font->calc_size(caption, caption_width);

		if (!caption_box) {
			caption_box = new WidgetScrollBox(VIEW_W, caption_size.y);
			caption_box->setBasePos(0, 0, ALIGN_BOTTOM);
		}
		else {
			caption_box->pos.h = caption_size.y;
			caption_box->resize(VIEW_W, caption_size.y);
		}

		caption_box->setPos(0, static_cast<int>(static_cast<float>(VIEW_H) * caption_margins.y) * (-1));

		font->renderShadowed(caption, VIEW_W / 2, 0,
							 JUSTIFY_CENTER,
							 caption_box->contents->getGraphics(),
							 caption_width,
							 FONT_WHITE);
	}

	if (art) {
		Rect art_dest;
		if (scale_graphics) {
			art_dest = resizeToScreen(art_size.x, art_size.y, false, ALIGN_CENTER);

			art->getGraphics()->ref(); // resize unref's our image (which we want to keep), so counter that here
			Image *resized = art->getGraphics()->resize(art_dest.w, art_dest.h);
			if (resized != NULL) {
				if (art_scaled) {
					delete art_scaled;
				}
				art_scaled = resized->createSprite();
				resized->unref();
			}

			if (art_scaled)
				art_scaled->setDest(art_dest);
		}
		else {
			art_dest.w = art_size.x;
			art_dest.h = art_size.y;

			alignToScreenEdge(ALIGN_CENTER, &art_dest);
			art->setDest(art_dest);
		}
	}
}

void Scene::render() {
	if (inpt->window_resized)
		refreshWidgets();

	if (art_scaled) {
		render_device->render(art_scaled);
	}
	else if (art) {
		render_device->render(art);
	}

	if (caption_box && caption != "") {
		caption_box->render();
	}
}

GameStateCutscene::GameStateCutscene(GameState *game_state)
	: previous_gamestate(game_state)
	, scale_graphics(false)
	, caption_margins(0.1f, 0.0f)
	, game_slot(-1)
{
	has_background = false;
}

GameStateCutscene::~GameStateCutscene() {
}

void GameStateCutscene::logic() {

	if (scenes.empty()) {
		if (game_slot != -1) {
			GameStatePlay *gsp = new GameStatePlay();
			gsp->resetGame();
			save_load->setGameSlot(game_slot);
			save_load->loadGame();

			previous_gamestate = gsp;
		}

		/* return to previous gamestate */
		delete requestedGameState;
		requestedGameState = previous_gamestate;
		requestedGameState->refreshWidgets();
		return;
	}

	while (!scenes.empty() && !scenes.front()->logic()) {
		delete scenes.front();
		scenes.pop();
	}
}

void GameStateCutscene::render() {
	if (!scenes.empty())
		scenes.front()->render();
}

bool GameStateCutscene::load(const std::string& filename) {
	FileParser infile;

	// @CLASS Cutscene|Description of cutscenes in cutscenes/
	if (!infile.open(filename))
		return false;

	// parse the cutscene file
	while (infile.next()) {

		if (infile.new_section) {
			if (infile.section == "scene")
				scenes.push(new Scene(caption_margins, scale_graphics));
		}

		if (infile.section.empty()) {
			if (infile.key == "scale_gfx") {
				// @ATTR scale_gfx|bool|The graphics will be scaled to fit screen width
				scale_graphics = toBool(infile.val);
			}
			else if (infile.key == "caption_margins") {
				// @ATTR caption_margins|float, float : X margin, Y margin|Percentage-based margins for the caption text based on screen size
				caption_margins.x = toFloat(infile.nextValue())/100.0f;
				caption_margins.y = toFloat(infile.val)/100.0f;
			}
			else if (infile.key == "menu_backgrounds") {
				// @ATTR menu_backgrounds|bool|This cutscene will use a random fullscreen background image, like the title screen does
				has_background = true;
			}
			else {
				infile.error("GameStateCutscene: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		else if (infile.section == "scene") {
			SceneComponent sc = SceneComponent();

			if (infile.key == "caption") {
				// @ATTR scene.caption|string|A caption that will be shown.
				sc.type = infile.key;
				sc.s = msg->get(infile.val);
			}
			else if (infile.key == "image") {
				// @ATTR scene.image|filename|Filename of an image that will be shown.
				sc.type = infile.key;
				sc.s = infile.val;
			}
			else if (infile.key == "pause") {
				// @ATTR scene.pause|duration|Pause before next component in 'ms' or 's'.
				sc.type = infile.key;
				sc.x = parse_duration(infile.val);
			}
			else if (infile.key == "soundfx") {
				// @ATTR scene.soundfx|filename|Filename of a sound that will be played
				sc.type = infile.key;
				sc.s = infile.val;
			}
			else {
				infile.error("GameStateCutscene: '%s' is not a valid key.", infile.key.c_str());
			}

			if (sc.type != "")
				scenes.back()->components.push(sc);

		}
		else {
			infile.error("GameStateCutscene: '%s' is not a valid section.", infile.section.c_str());
		}

	}

	infile.close();

	if (scenes.empty()) {
		logError("GameStateCutscene: No scenes defined in cutscene file %s", filename.c_str());
		return false;
	}

	return true;
}

