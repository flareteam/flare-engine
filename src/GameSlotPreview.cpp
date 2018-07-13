/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
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

#include "Animation.h"
#include "AnimationManager.h"
#include "AnimationSet.h"
#include "CommonIncludes.h"
#include "GameSlotPreview.h"
#include "FileParser.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "Utils.h"
#include "UtilsParsing.h"

GameSlotPreview::GameSlotPreview()
	: stats(NULL)
{
	// load the hero's animations from hero definition file
	anim->increaseCount("animations/hero.txt");
	animationSet = anim->getAnimationSet("animations/hero.txt");
	activeAnimation = animationSet->getAnimation("");

	loadLayerDefinitions();
}

/**
 * Load avatar sprite layer definitions into vector.
 */
void GameSlotPreview::loadLayerDefinitions() {
	layer_def = std::vector<std::vector<unsigned> >(8, std::vector<unsigned>());
	layer_reference_order = std::vector<std::string>();

	// NOTE: This is documented in Avatar.cpp
	FileParser infile;
	if (infile.open("engine/hero_layers.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (infile.key == "layer") {
				unsigned dir = Parse::toDirection(Parse::popFirstString(infile.val));
				if (dir>7) {
					infile.error("GameSlotPreview: Hero layer direction must be in range [0,7]");
					Utils::logErrorDialog("GameSlotPreview: Hero layer direction must be in range [0,7]");
					mods->resetModConfig();
					Utils::Exit(1);
				}
				std::string layer = Parse::popFirstString(infile.val);
				while (layer != "") {
					// check if already in layer_reference:
					unsigned ref_pos;
					for (ref_pos = 0; ref_pos < layer_reference_order.size(); ++ref_pos)
						if (layer == layer_reference_order[ref_pos])
							break;
					if (ref_pos == layer_reference_order.size())
						layer_reference_order.push_back(layer);
					layer_def[dir].push_back(ref_pos);

					layer = Parse::popFirstString(infile.val);
				}
			}
			else {
				infile.error("GameSlotPreview: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// There are the positions of the items relative to layer_reference_order
	// so if layer_reference_order=main,body,head,off
	// and we got a layer=3,off,body,head,main
	// then the layer_def[3] looks like (3,1,2,0)
}

void GameSlotPreview::loadGraphics(std::vector<std::string> _img_gfx) {
	if (!stats)
		return;

	loadLayerDefinitions();

	for (unsigned int i=0; i<animsets.size(); i++) {
		if (animsets[i])
			anim->decreaseCount(animsets[i]->getName());
		delete anims[i];
	}
	animsets.clear();
	anims.clear();

	for (unsigned int i=0; i<_img_gfx.size(); i++) {
		if (_img_gfx[i] != "") {
			std::string name = "animations/avatar/"+stats->gfx_base+"/"+_img_gfx[i] +".txt";
			anim->increaseCount(name);
			animsets.push_back(anim->getAnimationSet(name));
			animsets.back()->setParent(animationSet);
			anims.push_back(animsets.back()->getAnimation(activeAnimation->getName()));
			setAnimation("stance");
			if(!anims.back()->syncTo(activeAnimation)) {
				Utils::logError("GameSlotPreview: Error syncing animation in '%s' to 'animations/hero.txt'.", animsets.back()->getName().c_str());
			}
		}
		else {
			animsets.push_back(NULL);
			anims.push_back(NULL);
		}
	}
	anim->cleanUp();

	setAnimation("stance");
}

void GameSlotPreview::logic() {
	// handle animation
	activeAnimation->advanceFrame();
	for (unsigned i=0; i < anims.size(); i++) {
		if (anims[i] != NULL)
			anims[i]->advanceFrame();
	}
}

void GameSlotPreview::setAnimation(std::string name) {
	if (name == activeAnimation->getName())
		return;

	if (activeAnimation)
		delete activeAnimation;

	activeAnimation = animationSet->getAnimation(name);

	for (unsigned i=0; i < animsets.size(); i++) {
		delete anims[i];
		if (animsets[i])
			anims[i] = animsets[i]->getAnimation(name);
		else
			anims[i] = 0;
	}
}

void GameSlotPreview::addRenders(std::vector<Renderable> &r) {
	if (!stats)
		return;

	for (unsigned i = 0; i < layer_def[stats->direction].size(); ++i) {
		unsigned index = layer_def[stats->direction][i];
		if (anims[index]) {
			Renderable ren = anims[index]->getCurrentFrame(stats->direction);
			ren.prio = i+1;
			r.push_back(ren);
		}
	}
}

void GameSlotPreview::setStatBlock(StatBlock *_stats) {
	stats = _stats;
}

void GameSlotPreview::setPos(Point _pos) {
	pos = _pos;
}

void GameSlotPreview::render() {
	std::vector<Renderable> r;
	addRenders(r);

	for (size_t i = 0; i < r.size(); ++i) {
		if (r[i].image) {
			Rect dest;
			dest.x = pos.x - r[i].offset.x;
			dest.y = pos.y - r[i].offset.y;
			render_device->render(r[i], dest);
		}
	}
}

GameSlotPreview::~GameSlotPreview() {
	anim->decreaseCount("animations/hero.txt");
	if (activeAnimation)
		delete activeAnimation;

	for (unsigned int i=0; i<animsets.size(); i++) {
		if (animsets[i])
			anim->decreaseCount(animsets[i]->getName());
		delete anims[i];
	}
	anim->cleanUp();
}
