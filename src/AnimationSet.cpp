/*
Copyright © 2012 Stefan Beller
Copyright © 2014 Henrik Andersson
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


#include "Animation.h"
#include "AnimationSet.h"
#include "AnimationManager.h"

#include "FileParser.h"
#include "SharedResources.h"
#include "Settings.h"
#include "Utils.h"
#include "UtilsParsing.h"

#include <cassert>

Animation *AnimationSet::getAnimation(const std::string &_name) {
	if (!loaded)
		load();

	if (!_name.empty()) {
		for (size_t i = 0; i < animations.size(); i++) {
			if (animations[i]->getName() == _name)
				return new Animation(*animations[i]);
		}
	}

	return new Animation(*defaultAnimation);
}

unsigned AnimationSet::getAnimationFrames(const std::string &_name) {
	if (!loaded)
		load();
	for (size_t i = 0; i < animations.size(); i++)
		if (animations[i]->getName() == _name)
			return animations[i]->getFrameCount();
	return 0;
}

AnimationSet::AnimationSet(const std::string &animationname)
	: name(animationname)
	, loaded(false)
	, parent(NULL)
	, animations()
	, sprite(NULL) {
	defaultAnimation = new Animation("default", "play_once", NULL);
	defaultAnimation->setupUncompressed(Point(), Point(), 0, 1, 0);
}

void AnimationSet::load() {
	assert(!loaded);
	loaded = true;

	FileParser parser;
	// @CLASS AnimationSet|Description of animations in animations/
	if (!parser.open(name, true, "Error loading animation definition: " + name))
		return;

	std::string _name = "";
	unsigned short position = 0;
	unsigned short frames = 0;
	unsigned short duration = 0;
	Point render_size;
	Point render_offset;
	std::string type = "";
	std::string starting_animation = "";
	bool first_section=true;
	bool compressed_loading=false; // is reset every section to false, set by frame keyword
	Animation *newanim = NULL;
	std::vector<short> active_frames;

	unsigned short parent_anim_frames = 0;

	// Parse the file and on each new section create an animation object from the data parsed previously
	while (parser.next()) {
		// create the animation if finished parsing a section
		if (parser.new_section) {
			if (!first_section && !compressed_loading) {
				Animation *a = new Animation(_name, type, sprite);
				a->setupUncompressed(render_size, render_offset, position, frames, duration);
				if (!active_frames.empty())
					a->setActiveFrames(active_frames);
				active_frames.clear();
				animations.push_back(a);
			}
			first_section = false;
			compressed_loading = false;

			if (parent) {
				parent_anim_frames = static_cast<unsigned short>(parent->getAnimationFrames(parser.section));
			}
		}
		if (parser.key == "image") {
			// @ATTR image|filename|Filename of sprite-sheet image.
			if (sprite != NULL) {
				parser.error("AnimationSet: Multiple images specified. Dragons be here!");
				mods->resetModConfig();
				Exit(128);
			}

			sprite = render_device->loadImage(parser.val);
		}
		else if (parser.key == "position") {
			// @ATTR position|int|Number of frames to the right to use as the first frame. Unpacked animations only.
			position = static_cast<unsigned short>(toInt(parser.val));
		}
		else if (parser.key == "frames") {
			// @ATTR frames|int|The total number of frames
			frames = static_cast<unsigned short>(toInt(parser.val));
			if (parent && frames != parent_anim_frames) {
				parser.error("AnimationSet: Frame count %d != %d for matching animation in %s", frames, parent_anim_frames, parent->getName().c_str());
				frames = parent_anim_frames;
			}
		}
		else if (parser.key == "duration") {
			// @ATTR duration|duration|The duration of the entire animation in 'ms' or 's'.
			duration = static_cast<unsigned short>(parse_duration(parser.val));
		}
		else if (parser.key == "type")
			// @ATTR type|["play_once", "back_forth", "looped"]|How to loop (or not loop) this animation.
			type = parser.val;
		else if (parser.key == "render_size") {
			// @ATTR render_size|int, int : Width, Height|Width and height of animation.
			render_size.x = popFirstInt(parser.val);
			render_size.y = popFirstInt(parser.val);
		}
		else if (parser.key == "render_offset") {
			// @ATTR render_offset|int, int : X offset, Y offset|Render x/y offset.
			render_offset.x = popFirstInt(parser.val);
			render_offset.y = popFirstInt(parser.val);
		}
		else if (parser.key == "active_frame") {
			// @ATTR active_frame|[list(int), "all"]|A list of frames marked as "active". Also, "all" can be used to mark all frames as active.
			active_frames.clear();
			std::string nv = popFirstString(parser.val);
			if (nv == "all") {
				active_frames.push_back(-1);
			}
			else {
				while (nv != "") {
					active_frames.push_back(static_cast<short>(toInt(nv)));
					nv = popFirstString(parser.val);
				}
				std::sort(active_frames.begin(), active_frames.end());
				active_frames.erase(std::unique(active_frames.begin(), active_frames.end()), active_frames.end());
			}
		}
		else if (parser.key == "frame") {
			// @ATTR frame|int, int, int, int, int, int, int, int : Index, Direction, X, Y, Width, Height, X offset, Y offset|A single frame of a compressed animation.
			if (compressed_loading == false) { // first frame statement in section
				newanim = new Animation(_name, type, sprite);
				newanim->setup(frames, duration);
				if (!active_frames.empty())
					newanim->setActiveFrames(active_frames);
				active_frames.clear();
				animations.push_back(newanim);
				compressed_loading = true;
			}
			// frame = index, direction, x, y, w, h, offsetx, offsety
			Rect r;
			Point offset;
			const unsigned short index = static_cast<unsigned short>(popFirstInt(parser.val));
			const unsigned short direction = static_cast<unsigned short>(parse_direction(popFirstString(parser.val)));
			r.x = popFirstInt(parser.val);
			r.y = popFirstInt(parser.val);
			r.w = popFirstInt(parser.val);
			r.h = popFirstInt(parser.val);
			offset.x = popFirstInt(parser.val);
			offset.y = popFirstInt(parser.val);
			newanim->addFrame(index, direction, r, offset);
		}
		else {
			parser.error("AnimationSet: '%s' is not a valid key.", parser.key.c_str());
		}

		if (_name == "") {
			// This is the first animation
			starting_animation = parser.section;
		}
		_name = parser.section;
	}
	parser.close();

	if (!compressed_loading) {
		// add final animation
		Animation *a = new Animation(_name, type, sprite);
		a->setupUncompressed(render_size, render_offset, position, frames, duration);
		if (!active_frames.empty())
			a->setActiveFrames(active_frames);
		active_frames.clear();
		animations.push_back(a);
	}

	if (starting_animation != "") {
		Animation *a = getAnimation(starting_animation);
		delete defaultAnimation;
		defaultAnimation = a;
	}
}

AnimationSet::~AnimationSet() {
	if (sprite) sprite->unref();
	for (unsigned i = 0; i < animations.size(); ++i)
		delete animations[i];
	delete defaultAnimation;
}

