/*
Copyright © 2011-2012 kitano
Copyright © 2012 Stefan Beller
Copyright © 2014 Henrik Andersson
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

/**
 * class Animation
 *
 * The Animation class handles the logic of advancing frames based on the animation type
 * and returning a renderable frame.
 *
 * The intention with the class is to keep it as flexible as possible so that the animations
 * can be used not only for character animations but any animated in-game objects.
 */

#include "Animation.h"
#include "RenderDevice.h"

Animation::Animation(const std::string &_name, const std::string &_type, AnimationMedia *_sprite, uint8_t _blend_mode, uint8_t _alpha_mod, Color _color_mod)
	: reverse_playback(false)
	, active_frame_triggered(false)
	, type(	_type == "play_once" ? ANIMTYPE_PLAY_ONCE :
			_type == "back_forth" ? ANIMTYPE_BACK_FORTH :
			_type == "looped" ? ANIMTYPE_LOOPED :
			ANIMTYPE_NONE)
	, active_sub_frame(ACTIVE_SUBFRAME_END)
	, blend_mode(_blend_mode)
	, alpha_mod(_alpha_mod)
	, total_frame_count(0)
	, cur_frame(0)
	, sub_frame(0)
	, times_played(0)
	, frame_count(0)
	, color_mod(_color_mod)
	, sub_frame_f(0)
	, speed(1.0f)
	, sprite(_sprite)
	, gfx()
	, render_offset()
	, active_frames()
	, sub_frames()
	, name(_name)
{
	if (type == ANIMTYPE_NONE)
		Utils::logError("Animation: Type %s is unknown", _type.c_str());
}

void Animation::setupUncompressed(const Point& _render_size, const Point& _render_offset, unsigned short _position, unsigned short _frames, unsigned short _duration) {
	setup(_frames, _duration);

	for (unsigned short i = 0 ; i < _frames; i++) {
		int base_index = DIRECTIONS * i;
		for (unsigned short dir = 0 ; dir < DIRECTIONS; dir++) {
			// TODO handle multiple images for uncompressed animation defintions
			gfx[base_index + dir].first = sprite->getImageFromKey("");
			gfx[base_index + dir].second.x = _render_size.x * (_position + i);
			gfx[base_index + dir].second.y = _render_size.y * dir;
			gfx[base_index + dir].second.w = _render_size.x;
			gfx[base_index + dir].second.h = _render_size.y;
			render_offset[base_index + dir].x = _render_offset.x;
			render_offset[base_index + dir].y = _render_offset.y;
		}
	}
}

void Animation::setup(unsigned short _frames, unsigned short _duration) {
	frame_count = _frames;

	sub_frames.clear();

	if (_frames > 0 && _duration % _frames == 0) {
		// if we can evenly space frames among the duration, do it
		const unsigned short divided = _duration/_frames;
		for (unsigned short i = 0; i < _frames; ++i) {
			for (unsigned j = 0; j < divided; ++j) {
				sub_frames.push_back(i);
			}
		}
	}
	else {
		// we can't evenly space frames, so we try using Bresenham's line algorithm to lay them out
		// TODO the plain Bresenham algorithm isn't ideal and can cause weird results. Experimentation is needed here
		int x0 = 0;
		unsigned short y0 = 0;
		int x1 = _duration-1;
		int y1 = _frames-1;

		int dx = x1-x0;
		int dy = y1-y0;

		int D = 2*dy - dx;

		sub_frames.push_back(y0);

		int x = x0+1;
		unsigned short y = y0;

		while (x<=x1) {
			if (D > 0) {
				y++;
				sub_frames.push_back(y);
				D = D + ((2*dy)-(2*dx));
			}
			else {
				sub_frames.push_back(y);
				D = D + (2*dy);
			}
			x++;
		}
	}

	if (!sub_frames.empty()) total_frame_count = static_cast<unsigned short>(sub_frames.back()+1);

	if (type == ANIMTYPE_BACK_FORTH) {
		total_frame_count = static_cast<unsigned short>(2 * total_frame_count);
	}
	cur_frame = 0;
	sub_frame = 0;
	sub_frame_f = 0;
	times_played = 0;
	reverse_playback = false;

	active_frames.push_back(static_cast<unsigned short>(total_frame_count-1)/2);

	unsigned i = DIRECTIONS * _frames;
	gfx.resize(i);
	render_offset.resize(i);
}

bool Animation::addFrame(unsigned short index, unsigned short direction, const Rect& rect, const Point& _render_offset, const std::string& key) {
	if (index >= gfx.size() / DIRECTIONS || direction > DIRECTIONS-1) {
		return false;
	}

	unsigned i = (DIRECTIONS * index) + direction;
	gfx[i].first = sprite->getImageFromKey(key);
	gfx[i].second = rect;
	render_offset[i] = _render_offset;

	return true;
}

void Animation::advanceFrame() {
	if (sub_frames.empty()) {
		sub_frame = 0;
		sub_frame_f = 0;
		times_played++;
		return;
	}

	unsigned short last_base_index = static_cast<unsigned short>(sub_frames.size()-1);
	switch(type) {
		case ANIMTYPE_PLAY_ONCE:

			if (sub_frame < last_base_index) {
				sub_frame_f += speed;
				sub_frame = static_cast<unsigned short>(sub_frame_f);
			}
			else
				times_played = 1;
			break;

		case ANIMTYPE_LOOPED:
			if (sub_frame < last_base_index) {
				sub_frame_f += speed;
				sub_frame = static_cast<unsigned short>(sub_frame_f);
			}
			else {
				sub_frame = 0;
				sub_frame_f = 0;
				times_played++;
			}
			break;

		case ANIMTYPE_BACK_FORTH:

			if (!reverse_playback) {
				if (sub_frame < last_base_index) {
					sub_frame_f += speed;
					sub_frame = static_cast<unsigned short>(sub_frame_f);
				}
				else {
					reverse_playback = true;
					if (frame_count == 1)
						times_played++;
				}
			}
			else if (reverse_playback) {
				if (sub_frame > 0) {
					sub_frame_f -= speed;
					sub_frame = static_cast<unsigned short>(sub_frame_f);
				}
				else {
					reverse_playback = false;
					times_played++;
				}
			}
			break;

		case ANIMTYPE_NONE:
			break;
	}
	sub_frame = std::max<short>(0, sub_frame);
	sub_frame = (sub_frame > last_base_index ? last_base_index : sub_frame);

	cur_frame = sub_frames[sub_frame];
}

Renderable Animation::getCurrentFrame(unsigned short direction) {
	Renderable r;
	if (!sub_frames.empty()) {
		const unsigned short index = static_cast<unsigned short>(DIRECTIONS * sub_frames[sub_frame]) + direction;
		r.src.x = gfx[index].second.x;
		r.src.y = gfx[index].second.y;
		r.src.w = gfx[index].second.w;
		r.src.h = gfx[index].second.h;
		r.offset.x = render_offset[index].x;
		r.offset.y = render_offset[index].y;
		r.image = gfx[index].first;
		r.blend_mode = blend_mode;
		r.color_mod = color_mod;
		r.alpha_mod = alpha_mod;
	}
	return r;
}

void Animation::reset() {
	cur_frame = 0;
	sub_frame = 0;
	sub_frame_f = 0;
	times_played = 0;
	reverse_playback = false;
	active_frame_triggered = false;
}

bool Animation::syncTo(const Animation *other) {
	cur_frame = other->cur_frame;
	sub_frame = other->sub_frame;
	sub_frame_f = other->sub_frame_f;
	times_played = other->times_played;
	reverse_playback = other->reverse_playback;

	if (sub_frame >= sub_frames.size()) {
		if (sub_frames.empty()) {
			Utils::logError("Animation: '%s' animation has no frames, but current frame index is greater than 0.", name.c_str());
			sub_frame = 0;
			sub_frame_f = 0;
			return false;
		}
		else {
			Utils::logError("Animation: Current frame index (%d) was larger than the last frame index (%d) when syncing '%s' animation.", sub_frame, sub_frames.size()-1, name.c_str());
			sub_frame = static_cast<unsigned short>(sub_frames.size()-1);
			sub_frame_f = sub_frame;
			return false;
		}
	}

	return true;
}

void Animation::setActiveFrames(const std::vector<short> &_active_frames) {
	if (_active_frames.size() == 1 && _active_frames[0] == -1) {
		active_frames.clear();
		for (unsigned short i = 0; i < total_frame_count; ++i)
			active_frames.push_back(i);
	}
	else {
		active_frames = std::vector<short>(_active_frames);
	}

	// verify that each active frame is not out of bounds
	// this works under the assumption that frames are not dropped from the middle of animations
	// if an animation has too many frames to display in a specified duration, they are dropped from the end of the frame list
	bool have_last_frame = std::find(active_frames.begin(), active_frames.end(), total_frame_count-1) != active_frames.end();
	for (unsigned i=0; i<active_frames.size(); ++i) {
		if (active_frames[i] >= total_frame_count) {
			if (have_last_frame)
				active_frames.erase(active_frames.begin()+i);
			else {
				active_frames[i] = static_cast<short>(total_frame_count-1);
				have_last_frame = true;
			}
		}
	}
}

void Animation::setActiveSubFrame(const std::string& _active_sub_frame) {
	if (_active_sub_frame == "start")
		active_sub_frame = ACTIVE_SUBFRAME_START;
	else if (_active_sub_frame == "all")
		active_sub_frame = ACTIVE_SUBFRAME_ALL;
	else
		active_sub_frame = ACTIVE_SUBFRAME_END;
}

bool Animation::isFirstFrame() {
	return sub_frame == 0 && static_cast<float>(sub_frame) == sub_frame_f;
}

bool Animation::isLastFrame() {
	return sub_frame == static_cast<short>(getLastSubFrame(static_cast<short>(total_frame_count-1)));
}

bool Animation::isSecondLastFrame() {
	return sub_frame == static_cast<short>(getLastSubFrame(static_cast<short>(total_frame_count-2)));
}

bool Animation::isActiveFrame() {
	if (active_frames.empty())
		return false;

	// active frames only apply to the initial "forward" play of back/forth animations
	if (type == ANIMTYPE_BACK_FORTH && (reverse_playback || times_played > 0))
		return false;

	if (std::find(active_frames.begin(), active_frames.end(), cur_frame) != active_frames.end()) {
		if (active_sub_frame == ACTIVE_SUBFRAME_END && sub_frame == getLastSubFrame(cur_frame) && static_cast<float>(sub_frame) == sub_frame_f) {
			if (type == ANIMTYPE_PLAY_ONCE)
				active_frame_triggered = true;
			return true;
		}
		else if (active_sub_frame == ACTIVE_SUBFRAME_START && sub_frame == getFirstSubFrame(cur_frame) && static_cast<float>(sub_frame) == sub_frame_f) {
			if (type == ANIMTYPE_PLAY_ONCE)
				active_frame_triggered = true;
			return true;
		}
		else if (active_sub_frame == ACTIVE_SUBFRAME_ALL) {
			if (type == ANIMTYPE_PLAY_ONCE)
				active_frame_triggered = true;
			return true;
		}
	}
	else if (type == ANIMTYPE_PLAY_ONCE && isLastFrame() && !active_frame_triggered) {
		return true;
	}

	return false;
}

int Animation::getTimesPlayed() {
	return times_played;
}

std::string Animation::getName() {
	return name;
}

int Animation::getDuration() {
	return static_cast<int>(static_cast<float>(sub_frames.size()) / speed);
}

bool Animation::isCompleted() {
	return (type == ANIMTYPE_PLAY_ONCE && times_played > 0);
}

unsigned short Animation::getFirstSubFrame(const short &frame) {
	if (sub_frames.empty() || frame < 0) return 0;

	if (type == ANIMTYPE_BACK_FORTH && reverse_playback) {
		// since the animation is advancing backwards here, the last frame index is actually the first
		for (size_t i=sub_frames.size(); i>0; i--) {
			if (sub_frames[i-1] == frame)
				return static_cast<unsigned short>(i-1);
		}
		return static_cast<unsigned short>(sub_frames.size()-1);
	}
	else {
		// normal animation
		for (unsigned short i=0; i<sub_frames.size(); i++) {
			if (sub_frames[i] == frame) return i;
		}
		return 0;
	}
}

unsigned short Animation::getLastSubFrame(const short &frame) {
	if (sub_frames.empty() || frame < 0) return 0;

	if (type == ANIMTYPE_BACK_FORTH && reverse_playback) {
		// since the animation is advancing backwards here, the first frame index is actually the last
		for (unsigned short i=0; i<sub_frames.size(); i++) {
			if (sub_frames[i] == frame) return i;
		}
		return 0;
	}
	else {
		// normal animation
		for (size_t i=sub_frames.size(); i>0; i--) {
			if (sub_frames[i-1] == frame)
				return static_cast<unsigned short>(i-1);
		}
		return static_cast<unsigned short>(sub_frames.size()-1);
	}
}

void Animation::setSpeed(float val) {
	speed = val / 100.0f;
}
