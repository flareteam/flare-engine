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


#ifndef MAPBACKGROUND_H
#define MAPBACKGROUND_H

#include "CommonIncludes.h"
#include "RenderDevice.h"
#include "Utils.h"

class MapBackground {
public:
	MapBackground();
	~MapBackground();
	void clear();
	void load(const std::string& filename);
	void setMapCenter(int x, int y);
	void render(const FPoint& cam);

private:
	std::vector<Sprite*> sprites;
	std::vector<float> speeds;
	std::vector<FPoint> fixed_speeds;
	std::vector<FPoint> fixed_offsets;
	FPoint map_center;
};

#endif
