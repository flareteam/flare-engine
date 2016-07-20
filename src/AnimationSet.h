/*
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

#ifndef ANIMATION_SET_H
#define ANIMATION_SET_H

#include "CommonIncludes.h"

class Animation;

/**
 * The animation set contains all animations of one entity, hence it
 * they are all using the same spritesheet.
 *
 * The animation set is responsible for the spritesheet to be freed.
 */
class AnimationSet {
private:
	const std::string name; //i.e. animations/goblin_runner.txt, matches the animations filename.
	std::string imagefile;
	Animation *defaultAnimation; // has always a non-null animation, in case of successfull load it contains the first animation in the animation file.
	bool loaded;
	AnimationSet *parent;

	void load();
	unsigned getAnimationFrames(const std::string &_name);

public:

	std::vector<Animation*> animations;

	Image *sprite;

	explicit AnimationSet(const std::string &animationname);
	AnimationSet(const AnimationSet &a); // copy constructor not implemented.
	~AnimationSet();

	/**
	 * callee is responsible to free the returned animation.
	 * Returns the animation specified by \a name. If that animation is not found
	 * a default animation is returned.
	 */
	Animation *getAnimation(const std::string &name);

	const std::string &getName() {
		return name;
	}

	void setParent(AnimationSet *other) {
		parent = other;
	}
};

#endif // __ANIMATION_SET__
