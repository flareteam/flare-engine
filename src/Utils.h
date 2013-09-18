/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

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
 * Utils
 *
 * Various utility structures, enums, function
 */

#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "CommonIncludes.h"
#include <stdint.h>

class Point {
public:
	int x, y;
	Point() : x(0), y(0) {}
	Point(int _x, int _y) : x(_x), y(_y) {}
};

class FPoint {
public:
	float x, y;
	FPoint(Point _p) : x((float)_p.x), y((float)_p.y) {}
	FPoint() : x(0), y(0) {}
	FPoint(float _x, float _y) : x(_x), y(_y) {}
};

// message passing struct for various sprites rendered map inline
class Renderable {
public:
	SDL_Surface *sprite; // image to be used
	SDL_Rect src; // location on the sprite in pixel coordinates.

	FPoint map_pos;     // The map location on the floor between someone's feet
	Point offset;      // offset from map_pos to topleft corner of sprite
	uint64_t prio;     // 64-32 bit for map position, 31-16 for intertile position, 15-0 user dependent, such as Avatar.
	Renderable()
		: sprite(0)
		, src(SDL_Rect())
		, map_pos()
		, offset()
		, prio(0)
	{}
};

class Event_Component {
public:
	std::string type;
	std::string s;
	int x;
	int y;
	int z;
	int a;
	int b;

	Event_Component()
		: type("")
		, s("")
		, x(0)
		, y(0)
		, z(0)
		, a(0)
		, b(0)
	{}
};

Point floor(FPoint fp);
FPoint screen_to_map(int x, int y, float camx, float camy);
Point map_to_screen(float x, float y, float camx, float camy);
Point center_tile(Point p);
Point map_to_collision(FPoint p);
FPoint collision_to_map(Point p);
FPoint calcVector(FPoint pos, int direction, float dist);
float calcDist(FPoint p1, FPoint p2);
float calcTheta(float x1, float y1, float x2, float y2);
int calcDirection(float x0, float y0, float x1, float y1);
int calcDirection(const FPoint &src, const FPoint &dst);
bool isWithin(FPoint center, float radius, FPoint target);
bool isWithin(SDL_Rect r, Point target);

Uint32 readPixel(SDL_Surface *screen, int x, int y);
void drawPixel(SDL_Surface *screen, int x, int y, Uint32 color);
void drawLine(SDL_Surface *screen, int x0, int y0, int x1, int y1, Uint32 color);
void drawLine(SDL_Surface *screen, Point pos0, Point pos1, Uint32 color);
void drawRectangle(SDL_Surface *surface, Point pos0, Point pos1, Uint32 color);
bool checkPixel(Point px, SDL_Surface *surface);


/**
 * Creates a SDL_Surface.
 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
 * to settings. The result is a surface which has the same format as the
 * screen surface.
 * Additionally the alpha flag is set, so transparent blits are possible.
 */
SDL_Surface* createAlphaSurface(int width, int height);

/**
 * Creates a SDL_Surface.
 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
 * to settings. The result is a surface which has the same format as the
 * screen surface.
 * The bright pink (rgb 0xff00ff) is set as transparent color.
 */
SDL_Surface* createSurface(int width, int height);

SDL_Surface* scaleSurface(SDL_Surface *source, int width, int height);

/**
 * @brief loadGraphicSurface loads an image from a file.
 * @param filename
 *        The parameter filename is mandatory and specifies the image to be
 *        loaded. The filename will be located via the modmanager.
 * @param errormessage
 *        This is an optional parameter, which defines which error message
 *        should be displayed. If the errormessage is an empty string, no error
 *        message will be printed at all.
 * @param IfNotFoundExit
 *        If this optional boolean parameter is set to true, the program will
 *        shutdown sdl and quit, if the specified image is not found.
 * @param HavePinkColorKey
 *        This optional parameter specifies whether a color key with
 *        RGB(0xff, 0, 0xff) should be applied to the image.
 * @return
 *        Returns the SDL_Surface of the specified image or NULL if not
 *        successful
 */

SDL_Surface* loadGraphicSurface(std::string filename,
								std::string errormessage = "Couldn't load image",
								bool IfNotFoundExit = false,
								bool HavePinkColorKey = false);

void setupSDLVideoMode(unsigned width, unsigned height);

std::string abbreviateKilo(int amount);

#endif
