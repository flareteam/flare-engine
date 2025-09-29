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
	: first_key("")
	, first_path("") {
}

AnimationMedia::~AnimationMedia() {
}

void AnimationMedia::loadImage(const std::string& path, const std::string& key) {
	render_device->pushQueuedImage(path, RenderDevice::ERROR_NORMAL);

	if (sprites.find(key) == sprites.end()) {
		sprites[key] = NULL;
		paths[key] = path;
	}
	else {
		if (sprites[key] != NULL)
			sprites[key]->unref();

		sprites[key] = NULL;
		paths[key] = path;
	}

	if (sprites.size() == 1) {
		first_key = key;
		first_path = path;
	}
}

Image* AnimationMedia::getImageFromKey(const std::string& key) {
	std::map<std::string, Image*>::iterator it = sprites.find(key);
	if (it != sprites.end()) {
		if (it->second == NULL) {
			it->second = render_device->loadImage(paths[key], RenderDevice::ERROR_NORMAL);
		}
		return it->second;
	}
	else if (!sprites.empty()) {
		if (sprites[first_key] == NULL) {
			sprites[first_key] = render_device->loadImage(first_path, RenderDevice::ERROR_NORMAL);
		}
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

