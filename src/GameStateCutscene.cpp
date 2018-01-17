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

Scene::Scene(const CutsceneSettings& _settings, short _cutscene_type)
	: settings(_settings)
	, frame_counter(0)
	, pause_frames(0)
	, caption("")
	, art(NULL)
	, art_scaled(NULL)
	, sid(-1)
	, caption_box(NULL)
	, button_next(new WidgetButton("images/menus/buttons/right.png"))
	, button_close(new WidgetButton("images/menus/buttons/button_x.png"))
	, button_advance(NULL)
	, done(false)
	, vscroll_offset(0)
	, vscroll_ticks(0)
	, cutscene_type(_cutscene_type)
	, is_last_scene(false)
{
}

Scene::~Scene() {
	if (art) delete art;
	if (art_scaled) delete art_scaled;
	delete caption_box;
	delete button_next;
	delete button_close;
	while(!components.empty()) {
		components.pop();
	}

	for (size_t i = 0; i < vscroll_components.size(); ++i) {
		if (vscroll_components[i].image)
			delete vscroll_components[i].image;
		if (vscroll_components[i].text)
			delete vscroll_components[i].text;
	}
}

bool Scene::logic() {
	if (done) return false;

	if (is_last_scene && (cutscene_type == CUTSCENE_VSCROLL || components.empty()))
		button_advance = button_close;
	else
		button_advance = button_next;

	bool skip = false;
	bool skip_scroll = false;
	if (button_advance->checkClick()) {
		skip = true;
		skip_scroll = true;
	}
	if (!button_advance->pressed) {
		if (inpt->pressing[MAIN1] && (!inpt->lock[MAIN1] || cutscene_type == CUTSCENE_VSCROLL)) {
			inpt->lock[MAIN1] = true;
			skip = true;
		}
		if (inpt->pressing[ACCEPT] && (!inpt->lock[ACCEPT] || cutscene_type == CUTSCENE_VSCROLL)) {
			inpt->lock[ACCEPT] = true;
			skip = true;
		}
		if (inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
			inpt->lock[CANCEL] = true;
			done = true;
		}
	}

	if (cutscene_type == CUTSCENE_STATIC) {
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
	}
	else if (cutscene_type == CUTSCENE_VSCROLL) {
		// populate the list of text/images from config file data
		int next_y = 0;
		while (!components.empty()) {
			if (components.front().type == "text") {
				VScrollComponent vsc;
				vsc.pos.x = VIEW_W/2;
				vsc.pos.y = VIEW_H/2 + next_y;

				vsc.text = new WidgetLabel();
				if (vsc.text) {
					vsc.text->set(vsc.pos.x, vsc.pos.y, JUSTIFY_CENTER, VALIGN_TOP, components.front().s, font->getColor("widget_normal"), "font_captions");
					next_y += vsc.text->bounds.h;
				}

				vscroll_components.push_back(vsc);
			}
			else if (components.front().type == "image") {
				VScrollComponent vsc;

				Image *graphics = render_device->loadImage(components.front().s);
				if (graphics != NULL) {
					vsc.image = graphics->createSprite();
					if (vsc.image) {
						vsc.image_size.x = vsc.image->getGraphicsWidth();
						vsc.image_size.y = vsc.image->getGraphicsHeight();

						vsc.pos.x = VIEW_W/2 - vsc.image_size.x/2;
						vsc.pos.y = VIEW_H/2 + next_y;

						next_y += vsc.image_size.y;

						vscroll_components.push_back(vsc);
					}
					graphics->unref();
				}
			}
			else if (components.front().type == "separator") {
				VScrollComponent vsc;
				vsc.pos.y = VIEW_H/2 + next_y + components.front().x/2;
				next_y += components.front().x;

				vscroll_components.push_back(vsc);
			}
			components.pop();
		}

		vscroll_offset = static_cast<int>(static_cast<float>(vscroll_ticks) * (settings.vscroll_speed * MAX_FRAMES_PER_SEC) / VIEW_H);
		if (skip_scroll)
			return false;
		else if (skip)
			vscroll_ticks += 8;
		else
			vscroll_ticks++;

		refreshWidgets();

		// scroll has reached the end, quit the scene
		if (!vscroll_components.empty()) {
			VScrollComponent& vsc = vscroll_components.back();
			if (vsc.text && (vsc.text->bounds.y + vsc.text->bounds.h < 0)) {
				return false;
			}
			else if ((vsc.pos.y + vsc.separator_h) - vscroll_offset < 0) {
				return false;
			}
		}
	}

	return true;
}

void Scene::refreshWidgets() {
	if (cutscene_type ==  CUTSCENE_STATIC) {
		if (!caption.empty()) {
			int caption_width = VIEW_W - static_cast<int>(VIEW_W * (settings.caption_margins.x * 2.0f));
			font->setFont("font_captions");
			int padding = font->getLineHeight()/4;
			Point caption_size = font->calc_size(caption, caption_width);
			Point caption_size_padded(caption_size.x + padding*2, caption_size.y + padding*2);

			if (!caption_box) {
				caption_box = new WidgetScrollBox(caption_size_padded.x, caption_size_padded.y);
				caption_box->setBasePos(0, 0, ALIGN_BOTTOM);
				caption_box->bg = settings.caption_background;
				caption_box->transparent = false;
				caption_box->resize(caption_size_padded.x, caption_size_padded.y);
			}
			else {
				caption_box->pos.h = caption_size_padded.y;
				caption_box->resize(caption_size_padded.x, caption_size_padded.y);
			}

			caption_box->setPos(0, static_cast<int>(static_cast<float>(VIEW_H) * settings.caption_margins.y) * (-1));

			font->renderShadowed(caption, (padding / 2) + (caption_size_padded.x / 2), padding,
								 JUSTIFY_CENTER,
								 caption_box->contents->getGraphics(),
								 caption_width,
								 FONT_WHITE);
		}

		if (art) {
			Rect art_dest;
			if (settings.scale_graphics) {
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
	else if (cutscene_type == CUTSCENE_VSCROLL) {
		// position elements relative to the vertical offset
		for (size_t i = 0; i < vscroll_components.size(); ++i) {
			if (vscroll_components[i].text) {
				vscroll_components[i].text->setX(VIEW_W/2);
				vscroll_components[i].text->setY(vscroll_components[i].pos.y - vscroll_offset);
			}
			else if (vscroll_components[i].image) {
				int x = VIEW_W/2 - vscroll_components[i].image_size.x/2;
				int y = vscroll_components[i].pos.y - vscroll_offset;
				vscroll_components[i].image->setDest(x, y);
			}
		}
	}

	button_next->setBasePos(0, 0, ALIGN_TOPRIGHT);
	button_next->setPos(-(button_next->pos.w/2), button_next->pos.h/2);
	button_close->setBasePos(0, 0, ALIGN_TOPRIGHT);
	button_close->setPos(-(button_close->pos.w/2), button_close->pos.h/2);
}

void Scene::render() {
	if (inpt->window_resized)
		refreshWidgets();

	if (cutscene_type == CUTSCENE_STATIC) {
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
	else if (cutscene_type == CUTSCENE_VSCROLL) {
		// Color color = font->getColor("widget_normal");

		for (size_t i = 0; i < vscroll_components.size(); ++i) {
			VScrollComponent& vsc = vscroll_components[i];

			if (vsc.text) {
				if (vsc.text->bounds.y <= VIEW_H && (vsc.text->bounds.y + vsc.text->bounds.h >= 0)) {
					vsc.text->render();
				}
			}
			else if (vsc.image) {
				Point dest = FPointToPoint(vsc.image->getDest());
				if (dest.y <= VIEW_H && (dest.y + vsc.image_size.y >= 0)) {
					render_device->render(vsc.image);
				}
			}
		}
	}

	button_advance->render();
}

GameStateCutscene::GameStateCutscene(GameState *game_state)
	: previous_gamestate(game_state)
	, game_slot(-1)
{
	has_background = false;
}

GameStateCutscene::~GameStateCutscene() {
}

void GameStateCutscene::logic() {

	if (scenes.empty()) {
		if (game_slot != -1) {
			showLoading();
			GameStatePlay *gsp = new GameStatePlay();
			gsp->resetGame();
			save_load->setGameSlot(game_slot);
			save_load->loadGame();

			setRequestedGameState(gsp);
			return;
		}

		/* return to previous gamestate */
		showLoading();
		setRequestedGameState(previous_gamestate);
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
	CutsceneSettings settings;
	FileParser infile;

	// @CLASS Cutscene|Description of cutscenes in cutscenes/
	if (!infile.open(filename))
		return false;

	// parse the cutscene file
	while (infile.next()) {

		if (infile.new_section) {
			if (infile.section == "scene") {
				scenes.push(new Scene(settings, CUTSCENE_STATIC));
			}
			else if (infile.section == "vscroll") {
				// if the previous scene was also a vertical scroller, don't create a new scene
				// instead, the previous scene will be extended
				if (scenes.empty() || scenes.front()->cutscene_type != CUTSCENE_VSCROLL) {
					scenes.push(new Scene(settings, CUTSCENE_VSCROLL));
				}
			}
		}

		if (infile.section.empty()) {
			if (infile.key == "scale_gfx") {
				// @ATTR scale_gfx|bool|The graphics will be scaled to fit screen width
				settings.scale_graphics = toBool(infile.val);
			}
			else if (infile.key == "caption_margins") {
				// @ATTR caption_margins|float, float : X margin, Y margin|Percentage-based margins for the caption text based on screen size
				settings.caption_margins.x = toFloat(popFirstString(infile.val))/100.0f;
				settings.caption_margins.y = toFloat(popFirstString(infile.val))/100.0f;
			}
			else if (infile.key == "caption_background") {
				// @ATTR caption_background|color, int : Color, Alpha|Color (RGBA) of the caption area background.
				settings.caption_background = toRGBA(infile.val);
			}
			else if (infile.key == "vscroll_speed") {
				// @ATTR vscroll_speed|float|The speed at which elements will scroll in 'vscroll' scenes.
				settings.vscroll_speed = toFloat(infile.val);
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
		else if (infile.section == "vscroll") {
			SceneComponent sc = SceneComponent();

			if (infile.key == "text") {
				// @ATTR vscroll.text|string|A single, non-wrapping line of text.
				sc.type = infile.key;
				sc.s = msg->get(infile.val);
			}
			else if (infile.key == "image") {
				// @ATTR vscroll.image|filename|Filename of an image that will be shown.
				sc.type = infile.key;
				sc.s = infile.val;
			}
			else if (infile.key == "separator") {
				// @ATTR vscroll.separator|int|Places an invisible gap of a specified height between elements.
				sc.type = infile.key;
				sc.x = toInt(infile.val);
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
	else {
		scenes.back()->is_last_scene = true;
	}

	render_device->setBackgroundColor(Color(0,0,0,0));

	return true;
}

