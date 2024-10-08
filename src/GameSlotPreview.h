/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Henrik Andersson
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
 * class GameSlotPreview
 *
 * Contains logic and rendering routines for the previews in GameStateLoad.
 */

#ifndef AVATAR_GRAPHICS_H
#define AVATAR_GRAPHICS_H

#include "CommonIncludes.h"
#include "Utils.h"

class Animation;
class AnimationSet;
class MenuInventory;
class StatBlock;

class GameSlotPreview {
private:
	StatBlock *stats;
	Point pos;
	unsigned char static_direction;
	unsigned char* direction;

	std::vector<AnimationSet*> animsets; // hold the animations for all equipped items in the right order of drawing.
	std::vector<Animation*> anims; // hold the animations for all equipped items in the right order of drawing.

	std::vector<std::string> default_gfx;

public:
	GameSlotPreview();
	~GameSlotPreview();

	void setAnimation(const std::string& name);
	void setStatBlock(StatBlock *_stats);
	void setPos(Point _pos);
	void setDirection(unsigned char dir);
	void loadGraphics(std::vector<std::string> _img_gfx);
	void logic();
	void addRenders(std::vector<Renderable> &r);
	void render();
	void loadDefaultGraphics();
	void loadGraphicsFromInventory(MenuInventory* menu_inv);

	Animation *activeAnimation;
	AnimationSet *animationSet;

	std::vector<std::string> layer_reference_order;
	std::vector<std::vector<unsigned> > layer_def;
};

#endif

