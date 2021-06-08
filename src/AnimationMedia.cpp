/*
Copyright © 2020 bloodhero
Copyright © 2020 Justin Jacobs

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

#include "AnimationMedia.h"
#include "RenderDevice.h"
#include "SharedResources.h"

AnimationMedia::AnimationMedia()
	: first_key("") {
}

AnimationMedia::~AnimationMedia() {
}

void AnimationMedia::loadImage(std::string path, std::string key) {
    Image* loaded_img = render_device->loadImage(path, RenderDevice::ERROR_NORMAL);
	if (!loaded_img)
		return;

	if (sprites.find(key) == sprites.end()) {
		sprites[key] = loaded_img;
	}
	else {
		sprites[key]->unref();
		sprites[key] = loaded_img;
	}

	if (sprites.size() == 1) {
		first_key = key;
	}
}

Image* AnimationMedia::getImageFromKey(std::string key) {
	std::map<std::string, Image*>::iterator it = sprites.find(key);
	if (it != sprites.end()) {
		return it->second;
	}
	else if (!sprites.empty()) {
		return sprites[first_key];
	}

	return NULL;
}

void AnimationMedia::unref() {
    std::map<std::string, Image*>::iterator it;
    for (it = sprites.begin(); it != sprites.end(); ++it) {
        it->second->unref();
    }
	sprites.clear();
}

