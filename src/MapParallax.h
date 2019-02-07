/*
Copyright © 2017 Justin Jacobs

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


#ifndef MAPPARALLAX_H
#define MAPPARALLAX_H

#include "CommonIncludes.h"
#include "Utils.h"

class Sprite;

class MapParallaxLayer {
public:
	Sprite *sprite;
	float speed;
	FPoint fixed_speed;
	FPoint fixed_offset;
	std::string map_layer;

	MapParallaxLayer()
		: sprite(NULL)
		, speed(0)
		, map_layer("")
	{}
};

class MapParallax {
public:
	MapParallax();
	~MapParallax();
	void clear();
	void load(const std::string& filename);
	void setMapCenter(int x, int y);
	void render(const FPoint& cam, const std::string& map_layer);

private:
	std::vector<MapParallaxLayer> layers;
	FPoint map_center;
	size_t current_layer;
	bool loaded;
	std::string current_filename;
};

#endif
