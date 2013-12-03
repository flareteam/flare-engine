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

#include "CommonIncludes.h"
#include "GameStateCutscene.h"
#include "GameStatePlay.h"
#include "FileParser.h"
#include "WidgetScrollBox.h"

using namespace std;

Scene::Scene() : frame_counter(0)
	, pause_frames(0)
	, caption("")
	, caption_size(0,0)
	, sid(-1)
	, caption_box(NULL)
	, done(false) {
}

Scene::~Scene() {

	delete caption_box;
	while(!components.empty()) {
		components.front().i.clear_graphics();
		components.pop();
	}
}

bool Scene::logic(FPoint *caption_margins) {
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

			int caption_width = screen->w - (screen->w * (caption_margins->x * 2));
			font->setFont("font_captions");
			caption = components.front().s;
			caption_size = font->calc_size(caption, caption_width);

			delete caption_box;
			caption_box = new WidgetScrollBox(screen->w,caption_size.y);
			caption_box->pos.x = 0;
			caption_box->pos.y = screen->h - caption_size.y - (int)(VIEW_H * caption_margins->y);
			font->renderShadowed(caption, screen->w / 2, 0,
								 JUSTIFY_CENTER,
								 caption_box->contents,
								 caption_width,
								 FONT_WHITE);

		}
		else if (components.front().type == "image") {
			art = components.front().i;
			art_dest.x = (VIEW_W/2) - (art.sprite->w/2);
			art_dest.y = (VIEW_H/2) - (art.sprite->h/2);
			art_dest.w = art.sprite->w;
			art_dest.h = art.sprite->h;
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

	return true;
}

void Scene::render() {
	if (art.sprite != NULL) {
		art.set_dest(art_dest);
		render_device->render(art);
	}

	if (caption != "") {
		caption_box->render();
	}
}

GameStateCutscene::GameStateCutscene(GameState *game_state)
	: previous_gamestate(game_state)
	, scale_graphics(false)
	, caption_margins(0.1, 0)
	, game_slot(-1) {
}

GameStateCutscene::~GameStateCutscene() {
}

void GameStateCutscene::logic() {

	if (scenes.empty()) {
		if (game_slot != -1) {
			GameStatePlay *gsp = new GameStatePlay();
			gsp->resetGame();
			gsp->game_slot = game_slot;
			gsp->loadGame();

			previous_gamestate = gsp;
		}

		/* return to previous gamestate */
		delete requestedGameState;
		requestedGameState = previous_gamestate;
		return;
	}

	while (!scenes.empty() && !scenes.front().logic(&caption_margins))
		scenes.pop();
}

void GameStateCutscene::render() {
	if (!scenes.empty())
		scenes.front().render();
}

bool GameStateCutscene::load(std::string filename) {
	FileParser infile;

	// @CLASS Cutscene|Description of cutscenes in cutscenes/
	if (!infile.open("cutscenes/" + filename, true, false))
		return false;

	// parse the cutscene file
	while (infile.next()) {

		if (infile.new_section) {
			if (infile.section == "scene")
				scenes.push(Scene());
		}

		if (infile.section.empty()) {
			// allow having an empty section (globals such as scale_gfx might be set here
		}
		else if (infile.section == "scene") {
			SceneComponent sc = SceneComponent();

			if (infile.key == "caption") {
				// @ATTR scene.caption|string|A caption that will be shown.
				sc.type = infile.key;
				sc.s = msg->get(infile.val);
			}
			else if (infile.key == "image") {
				// @ATTR scene.image|string|An image that will be shown.
				sc.type = infile.key;
				sc.i.set_graphics(loadImage(infile.val));
				if (sc.i.sprite == NULL) {
					sc.type = "";
				}
				else {
					Renderable& r = sc.i;
					r.set_clip(0,0,r.sprite->w,r.sprite->h);
				}
			}
			else if (infile.key == "pause") {
				// @ATTR scene.pause|integer|Pause before next component
				sc.type = infile.key;
				sc.x = toInt(infile.val);
			}
			else if (infile.key == "soundfx") {
				// @ATTR scene.soundfx|string|A sound that will be played
				sc.type = infile.key;
				sc.s = infile.val;
			}

			if (sc.type != "")
				scenes.back().components.push(sc);

		}
		else {
			fprintf(stderr, "unknown section %s in file %s\n", infile.section.c_str(), infile.getFileName().c_str());
		}

		if (infile.key == "scale_gfx") {
			// @ATTR scale_gfx|bool|The graphics will be scaled to fit screen width
			scale_graphics = toBool(infile.val);
		}
		else if (infile.key == "caption_margins") {
			// @ATTR caption_margins|[x,y]|Percentage-based margins for the caption text based on screen size
			caption_margins.x = toFloat(infile.nextValue())/100.0f;
			caption_margins.y = toFloat(infile.val)/100.0f;
		}
	}

	if (scenes.empty()) {
		fprintf(stderr, "No scenes defined in cutscene file %s\n", filename.c_str());
		return false;
	}

	return true;
}

SDL_Surface *GameStateCutscene::loadImage(std::string filename) {

	std::string image_file = (mods->locate("images/"+ filename));
	SDL_Surface *image = IMG_Load(image_file.c_str());
	if (!image) {
		fprintf(stderr, "Missing cutscene art reference: %s\n", image_file.c_str());
		return NULL;
	}

	/* scale image to fit height */
	if (scale_graphics) {
		float ratio = image->h/(float)image->w;
		SDL_Surface *art = scaleSurface(image, VIEW_W, (int)(VIEW_W*ratio));
		if (art == NULL)
			return image;

		SDL_FreeSurface(image);
		image = art;
	}

	return image;
}

