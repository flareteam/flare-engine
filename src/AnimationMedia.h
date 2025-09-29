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

#ifndef ANIMATION_MEDIA_H
#define ANIMATION_MEDIA_H

#include "CommonIncludes.h"

class AnimationMedia {
private:
	std::map<std::string, Image*> sprites;
	std::map<std::string, std::string> paths;
	std::string first_key;
	std::string first_path;

public:
    AnimationMedia();
    ~AnimationMedia();
    void loadImage(const std::string& path, const std::string& key);
    Image* getImageFromKey(const std::string& key);
    void unref();
};

#endif
