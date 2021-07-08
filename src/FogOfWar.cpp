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
 * class FogOfWar
 *
 * Contains logic and rendering routines for fog of war.
 */

#include "FogOfWar.h"
#include "MapRenderer.h"

FogOfWar::FogOfWar()
	: map_size(Point()) {	
}

void FogOfWar::setMap(const Map_Layer& _colmap, unsigned short w, unsigned short h) {
	colmap.resize(w);
	for (unsigned i=0; i<w; ++i) {
		colmap[i].resize(h);
	}
	for (unsigned i=0; i<w; i++)
		for (unsigned j=0; j<h; j++)
			colmap[i][j] = _colmap[i][j];

	map_size.x = w;
	map_size.y = h;	

	std::cout << "FogOfWar layer loaded" << std:: endl;	}

FogOfWar::~FogOfWar() {
}
