/*
Copyright © 2011-2012 kitano
Copyright © 2012 Stefan Beller
Copyright © 2014 Henrik Andersson
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
 * class Animation
 *
 * The Animation class handles the logic of advancing frames based on the animation type
 * and returning a renderable frame.
 *
 * The intention with the class is to keep it as flexible as possible so that the animations
 * can be used not only for character animations but any animated in-game objects.
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "AnimationMedia.h"

class Animation {
protected:
	// animations consist of:
	// 1. frames, as defined in the animation data files
	// 2. sub-frames, which are generated in this class. Each is associated with a frame (more than one sub-frame can point to the same frame)
	static const unsigned short DIRECTIONS = 8; // may change in the future, but we currently hard-code 8 directions engine-wide

	enum {
		ANIMTYPE_NONE       = 0,
		ANIMTYPE_PLAY_ONCE  = 1, // just iterates over the images one time. it holds the final image when finished.
		ANIMTYPE_LOOPED     = 2, // going over the images again and again.
		ANIMTYPE_BACK_FORTH = 3  // similar to looped, but alternates the playback direction
	};

	enum {
		ACTIVE_SUBFRAME_END = 0,
		ACTIVE_SUBFRAME_START = 1,
		ACTIVE_SUBFRAME_ALL = 2,
	};

	bool reverse_playback;  // only for type == BACK_FORTH
	bool active_frame_triggered;

	const uint8_t type; // see ANIMTYPE enum above
	uint8_t active_sub_frame;
	uint8_t blend_mode;
	uint8_t alpha_mod;

	unsigned short total_frame_count; // the total number of frames for this animation (is different from frame_count for back/forth animations)
	unsigned short cur_frame;     // counts up until reaching total_frame_count.
	unsigned short sub_frame; // which frame in this animation is currently being displayed? range: 0..gfx.size()-1
	short times_played; // how often this animation was played (loop counter for type LOOPED)

	unsigned frame_count; // the frame count as it appears in the data files (i.e. not converted to engine frames)

	Color color_mod;

	float sub_frame_f; // more granular control over sub_frame
	float speed; // how fast the sub-frames advance

	AnimationMedia *sprite;

	std::vector<std::pair<Image*, Rect> > gfx; // graphics for each frame taken from the spritesheet
	std::vector<Point> render_offset; // "virtual point on the floor"
	std::vector<short> active_frames;	// frames that are marked as "active". Active frames are used to trigger various states (i.e power activation or hazard danger)
	std::vector<unsigned short> sub_frames; // a list of frames to play on each tick

	const std::string name;

	unsigned short getFirstSubFrame(const short &frame); // given a frame, gets the last sub frame that points to it
	unsigned short getLastSubFrame(const short &frame); // given a frame, gets the last sub frame that points to it

public:
	Animation(const std::string &_name, const std::string &_type, AnimationMedia *_sprite, uint8_t _blend_mode, uint8_t _alpha_mod, Color _color_mod);

	// Traditional way to create an animation.
	// The frames are stored in a grid like fashion, so the individual frame
	// position can be calculated based on a few things.
	// The spritesheet has 8 rows, each containing the data of one direction.
	// Within a row starting at (_position) there will be (_frames) frames,
	// which all belong to this animation.
	// The render_offset is constant for all frames. The render_size is also
	// the grid size.
	void setupUncompressed(const Point& render_size, const Point& render_offset, unsigned short _position, unsigned short _frames, unsigned short _duration);

	void setup(unsigned short _frames, unsigned short _duration);

	bool addFrame(unsigned short index, unsigned short direction, const Rect& rect, const Point& _render_offset, const std::string &key);

	// advance the animation one frame
	void advanceFrame();

	// sets the frame counters to the same values as the given Animation.
	// returns false on error. Error may occur when frame count of other is
	// larger than this animation's
	bool syncTo(const Animation *other);

	// return the Renderable of the current frame
	Renderable getCurrentFrame(unsigned short direction);

	bool isFirstFrame();
	bool isLastFrame();
	bool isSecondLastFrame();

	bool isActiveFrame();

	// in a looped animation returns how many times it's been played
	// in a play once animation returns 1 when the animation is finished
	int getTimesPlayed();

	// resets to beginning of the animation
	void reset();

	std::string getName();
	int getDuration();

	// a vector of indexes of gfx passed into.
	// if { -1 } is passed, all frames are set to active.
	void setActiveFrames(const std::vector<short> &_active_frames);

	void setActiveSubFrame(const std::string& _active_sub_frame);

	bool isCompleted();

	unsigned getFrameCount() { return frame_count; }

	void setSpeed(float val);
};

#endif

