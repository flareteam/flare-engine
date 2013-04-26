/*
Copyright © 2012 Justin Jacobs

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
 * class WidgetSlider
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <SDL.h>
#include <SDL_image.h>

#include "Widget.h"
#include "WidgetSlider.h"
#include "SharedResources.h"
#include "UtilsDebug.h"
#include "SDL_gfxBlitFunc.h"

using namespace std;

WidgetSlider::WidgetSlider (const string  & fname)
	: enabled(true)
	, sl(NULL)
	, pressed(false)
	, minimum(0)
	, maximum(0)
	, value(0) {
	sl = loadGraphicSurface(fname);
	if (!sl) {
		SDL_Quit();
		exit(1);
	}

	pos.w = sl->w;
	pos.h = sl->h / 2;

	pos_knob.w = sl->w / 8;
	pos_knob.h = sl->h / 2;

	render_to_alpha = false;
}

WidgetSlider::~WidgetSlider () {
	SDL_FreeSurface(sl);
}


bool WidgetSlider::checkClick() {
	if (!enabled) return false;
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}


bool WidgetSlider::checkClick (int x, int y) {
	if (!enabled) return false;
	Point mouse(x, y);
	//
	//	We are just grabbing the knob
	//
	if (!pressed && inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
		if (isWithin(pos_knob, mouse)) {
			pressed = true;
			inpt->lock[MAIN1] = true;
			return true;
		}
		return false;
	}

	// buttons already in use, new click not allowed
	if (inpt->lock[UP]) return false;
	if (inpt->lock[DOWN]) return false;

	if (!pressed && !inpt->lock[UP] && !inpt->lock[DOWN]) {
		return true;
	}
	if (pressed) {
		//
		// The knob has been released
		//
		// create a temporary SDL_Rect slightly wider than the slider
		SDL_Rect tmp_pos;
		tmp_pos.x = pos.x - (pos_knob.w*2);
		tmp_pos.y = pos.y;
		tmp_pos.w = pos.w + (pos_knob.w*4);
		tmp_pos.h = pos.h;

		if (!isWithin(tmp_pos, mouse)) {
			pressed = false;
			return false;
		}
		if (!inpt->lock[MAIN1]) {
			pressed = false;
		}

		// set the value of the slider
		int tmp = std::max(0, std::min(mouse.x - pos.x, static_cast<int>(pos.w)));

		pos_knob.x = pos.x + tmp - (pos_knob.w/2);
		value = minimum + (tmp*(maximum-minimum))/pos.w;

		return true;
	}
	return false;
}


void WidgetSlider::set (int min, int max, int val) {
	minimum = min;
	maximum = max;
	value = val;

	pos_knob.x = pos.x + ((val-min)* pos.w)/(max-min) - (pos_knob.w/2);
	pos_knob.y = pos.y;
}


int WidgetSlider::getValue () const {
	return value;
}


void WidgetSlider::render (SDL_Surface *target) {
	if (target == NULL) {
		target = screen;
	}

	SDL_Rect	base;
	base.x = 0;
	base.y = 0;
	base.h = pos.h;
	base.w = pos.w;

	SDL_Rect	knob;
	knob.x = 0;
	knob.y = pos.h;
	knob.h = pos_knob.h;
	knob.w = pos_knob.w;

	if (render_to_alpha) {
		SDL_gfxBlitRGBA(sl, &base, target, &pos);
		SDL_gfxBlitRGBA(sl, &knob, target, &pos_knob);
	}
	else {
		SDL_BlitSurface(sl, &base, target, &pos);
		SDL_BlitSurface(sl, &knob, target, &pos_knob);
	}

	if (in_focus) {
		Point topLeft;
		Point bottomRight;
		Uint32 color;

		topLeft.x = pos.x;
		topLeft.y = pos.y;
		bottomRight.x = pos.x + pos.w;
		bottomRight.y = pos.y + pos.h;
		color = SDL_MapRGB(target->format, 255,248,220);

		drawRectangle(target, topLeft, bottomRight, color);
	}
}

bool WidgetSlider::getNext() {
	if (!enabled) return false;

	value -= (maximum - minimum)/10;
	if (value < minimum)
		value = minimum;

	pos_knob.x = pos.x + ((value-minimum)* pos.w)/(maximum-minimum) - (pos_knob.w/2);
	pos_knob.y = pos.y;

	return true;
}

bool WidgetSlider::getPrev() {
	if (!enabled) return false;

	value += (maximum - minimum)/10;
	if (value > maximum)
		value = maximum;

	pos_knob.x = pos.x + ((value-minimum)* pos.w)/(maximum-minimum) - (pos_knob.w/2);
	pos_knob.y = pos.y;

	return true;
}


