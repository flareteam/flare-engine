/*
Copyright © 2011-2012 kitano
Copyright © 2012 Stefan Beller
Copyright © 2014 Henrik Andersson

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

using namespace std;

Animation::Animation(const std::string &_name, const std::string &_type, Image *_sprite)
	: name(_name)
	, type(	_type == "play_once" ? PLAY_ONCE :
			_type == "back_forth" ? BACK_FORTH :
			_type == "looped" ? LOOPED :
			NONE)
	, sprite(_sprite)
	, number_frames(0)
	, cur_frame(0)
	, cur_frame_index(0)
	, cur_frame_duration(0)
	, max_kinds(0)
	, additional_data(0)
	, times_played(0)
	, gfx()
	, render_offset()
	, frames()
	, active_frames()
	, elapsed_frames(0) {
	if (type == NONE)
		logError("Animation: Type %s is unknown\n", _type.c_str());
}

Animation::Animation(const Animation& a)
	: name(a.name)
	, type(a.type)
	, sprite(a.sprite)
	, number_frames(a.number_frames)
	, cur_frame(0)
	, cur_frame_index(a.cur_frame_index)
	, cur_frame_duration(a.cur_frame_duration)
	, max_kinds(a.max_kinds)
	, additional_data(a.additional_data)
	, times_played(0)
	, gfx(std::vector<Rect>(a.gfx))
	, render_offset(std::vector<Point>(a.render_offset))
	, frames(std::vector<unsigned short>(a.frames))
	, active_frames(std::vector<short>(a.active_frames))
	, elapsed_frames(0) {
}

void Animation::setupUncompressed(Point _render_size, Point _render_offset, int _position, int _frames, int _duration, unsigned short _maxkinds) {
	setup(_frames, _duration, _maxkinds);

	for (unsigned short i = 0 ; i < _frames; i++) {
		int base_index = max_kinds*i;
		for (unsigned short kind = 0 ; kind < max_kinds; kind++) {
			gfx[base_index + kind].x = _render_size.x * (_position + i);
			gfx[base_index + kind].y = _render_size.y * kind;
			gfx[base_index + kind].w = _render_size.x;
			gfx[base_index + kind].h = _render_size.y;
			render_offset[base_index + kind].x = _render_offset.x;
			render_offset[base_index + kind].y = _render_offset.y;
		}
	}
}

void Animation::setup(unsigned short _frames, unsigned short _duration, unsigned short _maxkinds) {
	calculateFrames(frames, _frames, _duration);

	if (!frames.empty()) number_frames = frames.back()+1;

	if (type == PLAY_ONCE) {
		additional_data = 0;
	}
	else if (type == LOOPED) {
		additional_data = 0;
	}
	else if (type == BACK_FORTH) {
		number_frames = 2 * number_frames;
		additional_data = 1;
	}
	cur_frame = 0;
	cur_frame_index = 0;
	max_kinds = _maxkinds;
	times_played = 0;

	active_frames.push_back(number_frames/2);

	gfx.resize(max_kinds*_frames);
	render_offset.resize(max_kinds*_frames);
}

void Animation::addFrame(	unsigned short index,
							unsigned short kind,
							Rect rect,
							Point _render_offset) {

	if (index > gfx.size()/max_kinds) {
		logError("Animation: Animation(%s) adding rect(%d, %d, %d, %d) to frame index(%u) out of bounds. must be in [0, %d]\n",
				name.c_str(), rect.x, rect.y, rect.w, rect.h, index, (int)gfx.size()/max_kinds);
		return;
	}
	if (kind > max_kinds-1) {
		logError("Animation: Animation(%s) adding rect(%d, %d, %d, %d) to frame(%u) kind(%u) out of bounds. must be in [0, %d]\n",
				name.c_str(), rect.x, rect.y, rect.w, rect.h, index, kind, max_kinds-1);
		return;
	}
	gfx[max_kinds*index+kind] = rect;
	render_offset[max_kinds*index+kind] = _render_offset;
}

void Animation::advanceFrame() {

	if (!this)
		return;


	unsigned short last_base_index = frames.size()-1;
	switch(type) {
		case PLAY_ONCE:

			if (cur_frame_index < last_base_index)
				cur_frame_index++;
			else
				times_played = 1;
			break;

		case LOOPED:
			if (cur_frame_index < last_base_index) {
				cur_frame_index++;
			}
			else {
				cur_frame_index = 0;
				times_played++;
			}
			break;

		case BACK_FORTH:

			if (additional_data == 1) {
				if (cur_frame_index < last_base_index)
					cur_frame_index++;
				else
					additional_data = -1;
			}
			else if (additional_data == -1) {
				if (cur_frame_index > 0)
					cur_frame_index--;
				else {
					additional_data = 1;
					times_played++;
				}
			}
			break;

		case NONE:
			break;
	}

	if (cur_frame != frames[cur_frame_index]) elapsed_frames++;
	cur_frame = frames[cur_frame_index];
}

Renderable Animation::getCurrentFrame(int kind) {
	Renderable r;
	if (this) {
		const int index = (max_kinds*frames[cur_frame_index]) + kind;
		r.src.x = gfx[index].x;
		r.src.y = gfx[index].y;
		r.src.w = gfx[index].w;
		r.src.h = gfx[index].h;
		r.offset.x = render_offset[index].x;
		r.offset.y = render_offset[index].y;
		r.image = sprite;
	}
	return r;
}

void Animation::reset() {
	cur_frame = 0;
	cur_frame_index = 0;
	times_played = 0;
	additional_data = 1;
	elapsed_frames = 0;
}

void Animation::syncTo(const Animation *other) {
	cur_frame = other->cur_frame;
	cur_frame_index = other->cur_frame_index;
	times_played = other->times_played;
	additional_data = other->additional_data;
	elapsed_frames = other->elapsed_frames;
}

void Animation::setActiveFrames(const std::vector<short> &_active_frames) {
	if (_active_frames.size() == 1 && _active_frames[0] == -1) {
		for (unsigned short i = 0; i < number_frames; ++i)
			active_frames.push_back(i);
	}
	else {
		active_frames = std::vector<short>(_active_frames);
	}

	// verify that each active frame is not out of bounds
	// this works under the assumption that frames are not dropped from the middle of animations
	// if an animation has too many frames to display in a specified duration, they are dropped from the end of the frame list
	bool have_last_frame = std::find(active_frames.begin(), active_frames.end(), number_frames-1) != active_frames.end();
	for (unsigned i=0; i<active_frames.size(); ++i) {
		if (active_frames[i] >= number_frames) {
			if (have_last_frame)
				active_frames.erase(active_frames.begin()+i);
			else {
				active_frames[i] = number_frames-1;
				have_last_frame = true;
			}
		}
	}
}

bool Animation::isFirstFrame() {
	return cur_frame_index == 0;
}

bool Animation::isLastFrame() {
	return cur_frame_index == getLastFrameIndex((short)number_frames-1);
}

bool Animation::isSecondLastFrame() {
	return cur_frame_index == getLastFrameIndex((short)number_frames-2);
}

bool Animation::isActiveFrame() {
	if (type == BACK_FORTH) {
		if (std::find(active_frames.begin(), active_frames.end(), elapsed_frames) != active_frames.end())
			return cur_frame_index == getLastFrameIndex(cur_frame);
	}
	else {
		if (std::find(active_frames.begin(), active_frames.end(), cur_frame) != active_frames.end())
			return cur_frame_index == getLastFrameIndex(cur_frame);
	}
	return false;
}

int Animation::getTimesPlayed() {
	return times_played;
}

std::string Animation::getName() {
	return name;
}

bool Animation::isCompleted() {
	return (type == PLAY_ONCE && times_played > 0);
}

void Animation::calculateFrames(std::vector<unsigned short> &fvec, const unsigned short &_frames, const unsigned short &_duration) {
	fvec.clear();

	if (_frames > 0 && _duration % _frames == 0) {
		const unsigned short divided = _duration/_frames;
		// if we can evenly space frames among the duration, do it
		for (unsigned i = 0; i < _frames; ++i) {
			for (unsigned j = 0; j < divided; ++j) {
				fvec.push_back(i);
			}
		}
	}
	else {
		// we can't evenly space frames, so we try using Bresenham's line algorithm to lay them out
		// TODO the plain Bresenham algorithm isn't ideal and can cause weird results. Experimentation is needed here
		int x0 = 0;
		int y0 = 0;
		int x1 = _duration-1;
		int y1 = _frames-1;

		int dx = x1-x0;
		int dy = y1-y0;

		int D= 2*dy - dx;

		fvec.push_back(y0);

		int x = x0+1;
		int y = y0;

		while (x<=x1) {
			if (D > 0) {
				y++;
				fvec.push_back(y);
				D = D + ((2*dy)-(2*dx));
			}
			else {
				fvec.push_back(y);
				D = D + (2*dy);
			}
			x++;
		}
	}
}

unsigned short Animation::getLastFrameIndex(const short &frame) {
	if (frames.empty() || frame < 0) return 0;

	if (type == BACK_FORTH && additional_data == -1) {
		// since the animation is advancing backwards here, the first frame index is actually the last
		for (unsigned i=0; i<frames.size(); i++) {
			if (frames[i] == frame) return i;
		}
		return 0;
	}
	else {
		// normal animation
		for (unsigned i=frames.size(); i>0; i--) {
			if (frames[i-1] == frame) return i-1;
		}
		return frames.size()-1;
	}
}
