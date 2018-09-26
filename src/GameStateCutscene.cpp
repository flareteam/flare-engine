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
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateCutscene.h"
#include "GameStatePlay.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "SoundManager.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetScrollBox.h"

Scene::Scene(const CutsceneSettings& _settings, short _cutscene_type)
	: cutscene_settings(_settings)
	, frame_counter(0)
	, pause_frames(0)
	, caption("")
	, art(NULL)
	, art_scaled(NULL)
	, art_scale_type(CUTSCENE_SCALE_NONE)
	, sid(-1)
	, caption_box(NULL)
	, button_prev(new WidgetButton("images/menus/buttons/left.png"))
	, button_next(new WidgetButton("images/menus/buttons/right.png"))
	, button_close(new WidgetButton("images/menus/buttons/button_x.png"))
	, button_advance(NULL)
	, vscroll_offset(0)
	, vscroll_y(0)
	, sub_index(0)
	, prev_sub_index(0)
	, cutscene_type(_cutscene_type)
	, is_first_scene(false)
	, is_last_scene(false)
{
}

Scene::Scene(const Scene& other) {
	*this = other;
}

Scene& Scene::operator=(const Scene& other) {
	if (this == &other)
		return *this;

	cutscene_settings = other.cutscene_settings;
	frame_counter = other.frame_counter;
	pause_frames = other.pause_frames;
	caption = other.caption;
	art = NULL;
	art_scaled = NULL;
	art_scale_type = other.art_scale_type;
	sid = other.sid;
	caption_box = NULL;
	button_prev = new WidgetButton("images/menus/buttons/left.png");
	button_next = new WidgetButton("images/menus/buttons/right.png");
	button_close = new WidgetButton("images/menus/buttons/button_x.png");
	button_advance = NULL;
	vscroll_offset = other.vscroll_offset;
	vscroll_y = other.vscroll_y;
	sub_index = other.sub_index;
	prev_sub_index = other.prev_sub_index;
	cutscene_type = other.cutscene_type;
	is_first_scene = other.is_first_scene;
	is_last_scene = other.is_last_scene;

	return *this;
}

Scene::~Scene() {
	clearArt();
	clearSound();
	delete caption_box;
	delete button_prev;
	delete button_next;
	delete button_close;

	for (size_t i = 0; i < vscroll_components.size(); ++i) {
		if (vscroll_components[i].image)
			delete vscroll_components[i].image;
		if (vscroll_components[i].text)
			delete vscroll_components[i].text;
	}
}

void Scene::clearArt() {
	delete art;
	art = NULL;
	delete art_scaled;
	art_scaled = NULL;

	art_size = Point(0, 0);
}

void Scene::clearSound() {
	if (sid != 0) {
		snd->unload(sid);
	}
	sid = 0;
}

void Scene::reset() {
	frame_counter = 0;
	pause_frames = 0;
	caption = "";
	clearArt();
	clearSound();
	delete caption_box;
	caption_box = NULL;
	vscroll_offset = 0;
	vscroll_y = 0;
	for (size_t i = 0; i < vscroll_components.size(); ++i) {
		if (vscroll_components[i].image)
			delete vscroll_components[i].image;
		if (vscroll_components[i].text)
			delete vscroll_components[i].text;
	}
	vscroll_components.clear();

	prev_sub_index = 0;
	if (sub_index > 0)
		prev_sub_index = sub_index - 1;

	sub_index = 0;

	refreshWidgets();
}

int Scene::logic() {
	button_prev->enabled = !(is_first_scene && sub_index == 0);

	if (is_last_scene && (cutscene_type == CUTSCENE_VSCROLL || sub_index+1 >= subscenes.size()))
		button_advance = button_close;
	else
		button_advance = button_next;

	int skip = SKIP_NONE;
	if (button_prev->checkClick()) {
		skip = SKIP_PREV;
	}
	else if (button_advance->checkClick()) {
		skip = SKIP_NEXT;
	}
	else if (inpt->pressing[Input::MAIN1] && Utils::isWithinRect(button_prev->pos, inpt->mouse)) {
		inpt->lock[Input::MAIN1] = true;
	}

	if (!button_prev->pressed && !button_advance->pressed) {
		if (inpt->pressing[Input::MAIN1] && (!inpt->lock[Input::MAIN1] || cutscene_type == CUTSCENE_VSCROLL)) {
			inpt->lock[Input::MAIN1] = true;
			skip = SKIP_SUBSCENE;
		}
		else if (inpt->pressing[Input::ACCEPT] && (!inpt->lock[Input::ACCEPT] || cutscene_type == CUTSCENE_VSCROLL)) {
			inpt->lock[Input::ACCEPT] = true;
			skip = SKIP_SUBSCENE;
		}
		else if (inpt->pressing[Input::RIGHT] && !inpt->lock[Input::RIGHT]) {
			inpt->lock[Input::RIGHT] = true;
			skip = SKIP_NEXT;
		}
		else if (button_prev->enabled && inpt->pressing[Input::LEFT] && !inpt->lock[Input::LEFT]) {
			inpt->lock[Input::LEFT] = true;
			skip = SKIP_PREV;
		}
		else if (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) {
			inpt->lock[Input::CANCEL] = true;
			return DONE;
		}
		else if (cutscene_type == CUTSCENE_VSCROLL && inpt->pressing[Input::UP]) {
			vscroll_y -= VSCROLL_SPEED;
			if (vscroll_y < 0)
				vscroll_y = 0;
		}
		else if (cutscene_type == CUTSCENE_VSCROLL && inpt->pressing[Input::DOWN]) {
			vscroll_y += VSCROLL_SPEED;
		}
	}

	if (cutscene_type == CUTSCENE_STATIC) {
		if (skip == SKIP_PREV) {
			// Go to the previous subscene (or scene if we're on the first subscene)
			if (sub_index == 0) {
				reset();
				return PREV;
			}
			else {
				reset();
			}
		}
		else if (skip == SKIP_NONE && pause_frames != 0 && (frame_counter < pause_frames || pause_frames == -1)) {
			// Pause until specified frame
			// we use -1 to pause indefinitely
			if (pause_frames > 0)
				++frame_counter;
			return NO_CHANGE;
		}
		else if (skip == SKIP_SUBSCENE || skip == SKIP_NEXT || (pause_frames != 0 && frame_counter == pause_frames)) {
			// Go to the next subscene/scene
			sub_index++;
		}

		std::string image_filename;
		std::string sfx_filename;

		// parse scene components until next pause
		if (sub_index < subscenes.size()) {
			for (size_t i = subscenes[sub_index]; i < components.size(); ++i) {
				if (components[i].type == "caption") {
					caption = components[i].s;
				}
				else if (components[i].type == "image") {
					image_filename = components[i].s;
					art_scale_type = components[i].x;
				}
				else if (components[i].type == "soundfx") {
					sfx_filename = components[i].s;
				}
				else if (components[i].type == "pause") {
					if (sub_index < prev_sub_index) {
						// if the prev_sub_index > 0, we skip pauses until the sub_index catches up
						sub_index++;
					}
					else {
						// set the pause counter
						pause_frames = components[i].x;
						frame_counter = 0;
						break;
					}

					// reset the prev_sub_index when sub_index has caught up
					if (sub_index == prev_sub_index) {
						prev_sub_index = 0;
					}
				}
			}
		}

		// set subscene image
		if (!image_filename.empty()) {
			clearArt();

			Image *graphics = render_device->loadImage(image_filename, RenderDevice::ERROR_NORMAL);
			if (graphics != NULL) {
				art = graphics->createSprite();
				art_size.x = art->getGraphicsWidth();
				art_size.y = art->getGraphicsHeight();
				graphics->unref();
			}
		}

		// set subscene soundfx
		if (!sfx_filename.empty()) {
			clearSound();

			sid = snd->load(sfx_filename, "Cutscenes");
			snd->play(sid, snd->DEFAULT_CHANNEL, snd->NO_POS, !snd->LOOP);
		}

		// check if current scene has reached the end
		if (sub_index >= subscenes.size() || subscenes[sub_index] >= components.size())
			return NEXT;

		refreshWidgets();
	}
	else if (cutscene_type == CUTSCENE_VSCROLL) {
		if (skip == SKIP_PREV) {
			reset();
			return PREV;
		}

		if (vscroll_components.empty()) {
			// populate the list of text/images from config file data
			int next_y = 0;
			for (size_t i = 0; i < components.size(); ++i) {
				if (components[i].type == "text") {
					VScrollComponent vsc;
					vsc.pos.x = settings->view_w/2;
					vsc.pos.y = settings->view_h/2 + next_y;

					vsc.text = new WidgetLabel();
					if (vsc.text) {
						vsc.text->setPos(vsc.pos.x, vsc.pos.y);
						vsc.text->setJustify(FontEngine::JUSTIFY_CENTER);
						vsc.text->setText(components[i].s);
						vsc.text->setFont("font_captions");
						next_y += vsc.text->getBounds()->h;
					}

					vscroll_components.push_back(vsc);
				}
				else if (components[i].type == "image") {
					VScrollComponent vsc;

					Image *graphics = render_device->loadImage(components[i].s, RenderDevice::ERROR_NORMAL);
					if (graphics != NULL) {
						vsc.image = graphics->createSprite();
						if (vsc.image) {
							vsc.image_size.x = vsc.image->getGraphicsWidth();
							vsc.image_size.y = vsc.image->getGraphicsHeight();

							vsc.pos.x = settings->view_w/2 - vsc.image_size.x/2;
							vsc.pos.y = settings->view_h/2 + next_y;

							next_y += vsc.image_size.y;

							vscroll_components.push_back(vsc);
						}
						graphics->unref();
					}
				}
				else if (components[i].type == "separator") {
					VScrollComponent vsc;
					vsc.pos.y = settings->view_h/2 + next_y + components[i].x/2;
					next_y += components[i].x;

					vscroll_components.push_back(vsc);
				}
			}
		}

		vscroll_offset = static_cast<int>(static_cast<float>(vscroll_y) * (cutscene_settings.vscroll_speed * settings->max_frames_per_sec) / settings->view_h);
		if (skip == SKIP_NEXT)
			return NEXT;
		else if (skip == SKIP_SUBSCENE)
			vscroll_y += VSCROLL_SPEED;
		else
			vscroll_y++;

		refreshWidgets();

		// scroll has reached the end, quit the scene
		if (!vscroll_components.empty()) {
			VScrollComponent& vsc = vscroll_components.back();
			if (vsc.text && (vsc.text->getBounds()->y + vsc.text->getBounds()->h < 0)) {
				return NEXT;
			}
			else if ((vsc.pos.y + vsc.separator_h) - vscroll_offset < 0) {
				return NEXT;
			}
		}
	}

	return NO_CHANGE;
}

void Scene::refreshWidgets() {
	if (cutscene_type ==  CUTSCENE_STATIC) {
		if (!caption.empty()) {
			int caption_width = settings->view_w - static_cast<int>(settings->view_w * (cutscene_settings.caption_margins.x * 2.0f));
			font->setFont("font_captions");
			int padding = font->getLineHeight()/4;
			Point caption_size = font->calc_size(caption, caption_width);
			Point caption_size_padded(caption_size.x + padding*2, caption_size.y + padding*2);

			if (!caption_box) {
				caption_box = new WidgetScrollBox(caption_size_padded.x, caption_size_padded.y);
				caption_box->setBasePos(0, 0, Utils::ALIGN_BOTTOM);
				caption_box->bg = cutscene_settings.caption_background;
				caption_box->resize(caption_size_padded.x, caption_size_padded.y);
			}
			else {
				caption_box->pos.h = caption_size_padded.y;
				caption_box->resize(caption_size_padded.x, caption_size_padded.y);
			}

			caption_box->setPos(0, static_cast<int>(static_cast<float>(settings->view_h) * cutscene_settings.caption_margins.y) * (-1));

			font->renderShadowed(caption, (padding / 2) + (caption_size_padded.x / 2), padding,
								 FontEngine::JUSTIFY_CENTER,
								 caption_box->contents->getGraphics(),
								 caption_width,
								 font->getColor(FontEngine::COLOR_WHITE));
		}

		if (art) {
			Rect art_dest;
			if (art_scale_type != CUTSCENE_SCALE_NONE) {
				if (art_scale_type == CUTSCENE_SCALE_SCREEN)
					art_dest = Utils::resizeToScreen(art_size.x, art_size.y, false, Utils::ALIGN_CENTER);
				else if (art_scale_type == CUTSCENE_SCALE_HEIGHT)
					art_dest = Utils::resizeToScreen(art_size.x, art_size.y, true, Utils::ALIGN_CENTER);

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
					art_scaled->setDestFromRect(art_dest);
			}
			else {
				art_dest.w = art_size.x;
				art_dest.h = art_size.y;

				Utils::alignToScreenEdge(Utils::ALIGN_CENTER, &art_dest);
				art->setDestFromRect(art_dest);
			}
		}
	}
	else if (cutscene_type == CUTSCENE_VSCROLL) {
		// position elements relative to the vertical offset
		for (size_t i = 0; i < vscroll_components.size(); ++i) {
			if (vscroll_components[i].text) {
				vscroll_components[i].text->setPos(settings->view_w / 2, vscroll_components[i].pos.y - vscroll_offset);
			}
			else if (vscroll_components[i].image) {
				int x = settings->view_w/2 - vscroll_components[i].image_size.x/2;
				int y = vscroll_components[i].pos.y - vscroll_offset;
				vscroll_components[i].image->setDest(x, y);
			}
		}
	}

	button_prev->setBasePos(0, 0, Utils::ALIGN_TOPLEFT);
	button_prev->setPos(button_prev->pos.w/2, button_prev->pos.h/2);
	button_next->setBasePos(0, 0, Utils::ALIGN_TOPRIGHT);
	button_next->setPos(-(button_next->pos.w/2), button_next->pos.h/2);
	button_close->setBasePos(0, 0, Utils::ALIGN_TOPRIGHT);
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
				if (vsc.text->getBounds()->y <= settings->view_h && (vsc.text->getBounds()->y + vsc.text->getBounds()->h >= 0)) {
					vsc.text->render();
				}
			}
			else if (vsc.image) {
				Point dest(vsc.image->getDest());
				if (dest.y <= settings->view_h && (dest.y + vsc.image_size.y >= 0)) {
					render_device->render(vsc.image);
				}
			}
		}
	}

	// only display the back button if there's more than one scene or subscene
	bool only_scene = is_first_scene && is_last_scene;
	if (!only_scene || (only_scene && cutscene_type == CUTSCENE_STATIC && subscenes.size() > 1))
		button_prev->render();

	button_advance->render();
}

GameStateCutscene::GameStateCutscene(GameState *game_state)
	: previous_gamestate(game_state)
	, scene_index(0)
	, initialized(false)
	, status(Scene::NO_CHANGE)
	, game_slot(-1)
{
	has_background = false;
}

GameStateCutscene::~GameStateCutscene() {
	if (!music.empty())
		snd->stopMusic();

	for (size_t i = 0; i < scenes.size(); ++i) {
		delete scenes[i];
	}
}

void GameStateCutscene::logic() {
	if (!initialized) {
		if (settings->music_volume > 0 && !music.empty()) {
			// restart music so that game devs can sync with cutscene playback
			snd->stopMusic();
			snd->loadMusic(music);
		}

		initialized = true;
	}

	if (scenes.empty() || scene_index >= scenes.size()) {
		if (game_slot != -1) {
			showLoading();
			GameStatePlay *gsp = new GameStatePlay();
			gsp->resetGame();
			save_load->setGameSlot(game_slot);
			save_load->loadGame();

			setRequestedGameState(gsp);
			return;
		}

		// return to previous gamestate
		showLoading();
		setRequestedGameState(previous_gamestate);
		return;
	}

	// status is processed after we render this scene
	if (scene_index < scenes.size())
		status = scenes[scene_index]->logic();
}

void GameStateCutscene::render() {
	if (!scenes.empty() && scene_index < scenes.size()) {
		scenes[scene_index]->render();

		// process logic() status
		if (status == Scene::DONE) {
			scene_index = scenes.size();
		}
		else if (status == Scene::NEXT) {
			scene_index++;
		}
		else if (status == Scene::PREV && scene_index > 0) {
			scenes[scene_index]->reset();
			scene_index--;
			scenes[scene_index]->reset();
		}
	}
}

bool GameStateCutscene::load(const std::string& filename) {
	CutsceneSettings cutscene_settings;
	FileParser infile;

	// @CLASS Cutscene|Description of cutscenes in cutscenes/
	if (!infile.open(filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL))
		return false;

	Utils::logInfo("GameStateCutscene: Loading cutscene '%s'", filename.c_str());

	// parse the cutscene file
	while (infile.next()) {

		if (infile.new_section) {
			if (infile.section == "scene") {
				scenes.push_back(new Scene(cutscene_settings, Scene::CUTSCENE_STATIC));
				scenes.back()->subscenes.push_back(0);
			}
			else if (infile.section == "vscroll") {
				// if the previous scene was also a vertical scroller, don't create a new scene
				// instead, the previous scene will be extended
				if (scenes.empty() || scenes.back()->cutscene_type != Scene::CUTSCENE_VSCROLL) {
					scenes.push_back(new Scene(cutscene_settings, Scene::CUTSCENE_VSCROLL));
				}
			}
		}

		if (infile.section.empty()) {
			if (infile.key == "caption_margins") {
				// @ATTR caption_margins|float, float : X margin, Y margin|Percentage-based margins for the caption text based on screen size
				cutscene_settings.caption_margins.x = Parse::toFloat(Parse::popFirstString(infile.val))/100.0f;
				cutscene_settings.caption_margins.y = Parse::toFloat(Parse::popFirstString(infile.val))/100.0f;
			}
			else if (infile.key == "caption_background") {
				// @ATTR caption_background|color, int : Color, Alpha|Color (RGBA) of the caption area background.
				cutscene_settings.caption_background = Parse::toRGBA(infile.val);
			}
			else if (infile.key == "vscroll_speed") {
				// @ATTR vscroll_speed|float|The speed at which elements will scroll in 'vscroll' scenes.
				cutscene_settings.vscroll_speed = Parse::toFloat(infile.val);
			}
			else if (infile.key == "menu_backgrounds") {
				// @ATTR menu_backgrounds|bool|This cutscene will use a random fullscreen background image, like the title screen does
				has_background = true;
			}
			else if (infile.key == "music") {
				// @ATTR music|filename|The music file that will play during this cutscene.
				music = infile.val;
				hasMusic = true;
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
				// @ATTR scene.image|filename, int : Filename, Scaling type|Filename of an image that will be shown. The scaling type is a value between 0-2, corresponding to: none, fit height, fit screen.
				sc.type = infile.key;
				sc.s = Parse::popFirstString(infile.val);
				sc.x = Parse::popFirstInt(infile.val);
				if (sc.x < Scene::CUTSCENE_SCALE_NONE || sc.x > Scene::CUTSCENE_SCALE_SCREEN) {
					infile.error("GameStateCutscene: '%d' is not a valid scaling type.", sc.x);
					sc.x = Scene::CUTSCENE_SCALE_NONE;
				}
			}
			else if (infile.key == "pause") {
				// @ATTR scene.pause|duration|Pause before next component in 'ms' or 's'. A value of '-1' may be used to pause indefinitely.
				sc.type = infile.key;
				std::string temp = Parse::popFirstString(infile.val);
				if (temp == "-1")
					sc.x = -1;
				else
					sc.x = Parse::toDuration(temp);
				// next subscene starts on the component after this pause
				scenes.back()->subscenes.push_back(scenes.back()->components.size() + 1);
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
				scenes.back()->components.push_back(sc);

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
				sc.x = Parse::toInt(infile.val);
			}
			else {
				infile.error("GameStateCutscene: '%s' is not a valid key.", infile.key.c_str());
			}

			if (sc.type != "")
				scenes.back()->components.push_back(sc);

		}
		else {
			infile.error("GameStateCutscene: '%s' is not a valid section.", infile.section.c_str());
		}

	}

	infile.close();

	if (scenes.empty()) {
		Utils::logInfo("GameStateCutscene: No scenes defined in cutscene file %s", filename.c_str());
		return false;
	}
	else {
		if (scenes.back()->components.back().type == "pause") {
			scenes.back()->subscenes.pop_back();
		}

		scenes.front()->is_first_scene = true;
		scenes.back()->is_last_scene = true;
	}

	render_device->setBackgroundColor(Color(0,0,0,0));

	return true;
}

