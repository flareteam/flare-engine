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

#include "Settings.h"
#include "SharedResources.h"
#include "Utils.h"

#include <cmath>

using namespace std;


int round(float f) {
	return (int)(f + 0.5);
}

Point round(FPoint fp) {
	Point result;
	result.x = round(fp.x);
	result.y = round(fp.y);
	return result;
}

Point screen_to_map(int x, int y, int camx, int camy) {
	Point r;
	if (TILESET_ORIENTATION == TILESET_ISOMETRIC) {
		int scrx = (x - VIEW_W_HALF) /2;
		int scry = (y - VIEW_H_HALF) /2;

		r.x = (UNITS_PER_PIXEL_X * scrx) + (UNITS_PER_PIXEL_Y * scry) + camx;
		r.y = (UNITS_PER_PIXEL_Y * scry) - (UNITS_PER_PIXEL_X * scrx) + camy;
	}
	else {
		r.x = (x - VIEW_W_HALF) * (UNITS_PER_PIXEL_X ) + camx;
		r.y = (y - VIEW_H_HALF) * (UNITS_PER_PIXEL_Y ) + camy;
	}
	return r;
}

/**
 * Returns a point (in map units) of a given (x,y) tupel on the screen
 * when the camera is at a given position.
 */
Point map_to_screen(int x, int y, int camx, int camy) {
	Point r;

	// adjust to the center of the viewport
	// we do this calculation first to avoid negative integer division
	int adjust_x = VIEW_W_HALF * UNITS_PER_PIXEL_X;
	int adjust_y = VIEW_H_HALF * UNITS_PER_PIXEL_Y;

	if (TILESET_ORIENTATION == TILESET_ISOMETRIC) {
		r.x = (x - camx - y + camy + adjust_x)/UNITS_PER_PIXEL_X;
		r.y = (x - camx + y - camy + adjust_y)/UNITS_PER_PIXEL_Y;
	}
	else { //TILESET_ORTHOGONAL
		r.x = (x - camx + adjust_x)/UNITS_PER_PIXEL_X;
		r.y = (y - camy + adjust_y)/UNITS_PER_PIXEL_Y;
	}
	return r;
}

Point center_tile(Point p) {
	if (TILESET_ORIENTATION == TILESET_ORTHOGONAL) {
		p.x += TILE_W_HALF;
		p.y += TILE_H_HALF;
	}
	else //TILESET_ISOMETRIC
		p.y += TILE_H_HALF;
	return p;
}

Point collision_to_map(Point p) {
	p.x = (p.x << TILE_SHIFT) + UNITS_PER_TILE/2;
	p.y = (p.y << TILE_SHIFT) + UNITS_PER_TILE/2;
	return p;
}

Point map_to_collision(Point p) {
	p.x = p.x >> TILE_SHIFT;
	p.y = p.y >> TILE_SHIFT;
	return p;
}

/**
 * Apply parameter distance to position and direction
 */
FPoint calcVector(Point pos, int direction, int dist) {
	FPoint p;
	p.x = (float)(pos.x);
	p.y = (float)(pos.y);

	float dist_straight = (float)dist;
	float dist_diag = ((float)dist) * (float)(0.7071); //  1/sqrt(2)

	switch (direction) {
		case 0:
			p.x -= dist_diag;
			p.y += dist_diag;
			break;
		case 1:
			p.x -= dist_straight;
			break;
		case 2:
			p.x -= dist_diag;
			p.y -= dist_diag;
			break;
		case 3:
			p.y -= dist_straight;
			break;
		case 4:
			p.x += dist_diag;
			p.y -= dist_diag;
			break;
		case 5:
			p.x += dist_straight;
			break;
		case 6:
			p.x += dist_diag;
			p.y += dist_diag;
			break;
		case 7:
			p.y += dist_straight;
			break;
	}
	return p;
}

double calcDist(Point p1, Point p2) {
	int x = p2.x - p1.x;
	int y = p2.y - p1.y;
	double step1 = x*x + y*y;
	return sqrt(step1);
}

/**
 * is target within the area defined by center and radius?
 */
bool isWithin(Point center, int radius, Point target) {
	return (calcDist(center, target) < radius);
}

/**
 * is target within the area defined by rectangle r?
 */
bool isWithin(SDL_Rect r, Point target) {
	return target.x >= r.x && target.y >= r.y && target.x < r.x+r.w && target.y < r.y+r.h;
}


Uint32 readPixel(SDL_Surface *surface, int x, int y)
{
	SDL_LockSurface(surface);
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	Uint32 pixel;

	switch (bpp) {
	case 1:
		pixel = *p;
		break;

	case 2:
		pixel = *(Uint16 *)p;
		break;

	case 3:
	  if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
		  pixel = p[0] << 16 | p[1] << 8 | p[2];
	  else
		  pixel = p[0] | p[1] << 8 | p[2] << 16;
	  break;

	case 4:
		pixel = *(Uint32 *)p;
		break;

	default:
		SDL_UnlockSurface(surface);
		return 0;
	}

	SDL_UnlockSurface(surface);
	return pixel;
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 *
 * Source: SDL Documentation
 * http://www.libsdl.org/docs/html/guidevideo.html
 */
void drawPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(Uint16 *)p = pixel;
		break;

	case 3:
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
#else
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
#endif
		break;

	case 4:
		*(Uint32 *)p = pixel;
		break;
	}
}



/**
 * draw line to the screen
 * from http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm#Simplification
 */
void drawLine(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 color) {
	const int dx = abs(x1-x0);
	const int dy = abs(y1-y0);
	const int sx = x0 < x1 ? 1 : -1;
	const int sy = y0 < y1 ? 1 : -1;
	int err = dx-dy;

	do {
		//skip draw if outside screen
		if (x0 > 0 && y0 > 0 && x0 < VIEW_W && y0 < VIEW_H)
			drawPixel(surface,x0,y0,color);

		int e2 = 2*err;
		if (e2 > -dy) {
			err = err - dy;
			x0 = x0 + sx;
		}
		if (e2 <  dx) {
			err = err + dx;
			y0 = y0 + sy;
		}
	} while(x0 != x1 || y0 != y1);
}

void drawLine(SDL_Surface *surface, Point pos0, Point pos1, Uint32 color) {
	drawLine(surface, pos0.x, pos0.y, pos1.x, pos1.y, color);
}

void drawRectangle(SDL_Surface *surface, Point pos0, Point pos1, Uint32 color) {
	drawLine(surface, pos0.x, pos0.y, pos1.x, pos0.y, color);
	drawLine(surface, pos1.x, pos0.y, pos1.x, pos1.y, color);
	drawLine(surface, pos0.x, pos0.y, pos0.x, pos1.y, color);
	drawLine(surface, pos0.x, pos1.y, pos1.x, pos1.y, color);
}

void setSDL_RGBA(Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	*rmask = 0xff000000;
	*gmask = 0x00ff0000;
	*bmask = 0x0000ff00;
	*amask = 0x000000ff;
#else
	*rmask = 0x000000ff;
	*gmask = 0x0000ff00;
	*bmask = 0x00ff0000;
	*amask = 0xff000000;
#endif
}

/**
 * create blank surface
 * based on example: http://www.libsdl.org/docs/html/sdlcreatergbsurface.html
 */
SDL_Surface* createAlphaSurface(int width, int height) {

	SDL_Surface *surface;
	Uint32 rmask, gmask, bmask, amask;

	setSDL_RGBA(&rmask, &gmask, &bmask, &amask);

	if (HWSURFACE)
		surface = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCALPHA, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);
	else
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);

	if(surface == NULL) {
		fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
	}

	// optimize
	SDL_Surface *cleanup = surface;
	surface = SDL_DisplayFormatAlpha(surface);
	SDL_FreeSurface(cleanup);

	return surface;
}

SDL_Surface* createSurface(int width, int height) {

	SDL_Surface *surface;
	Uint32 rmask, gmask, bmask, amask;

	setSDL_RGBA(&rmask, &gmask, &bmask, &amask);

	if (HWSURFACE)
		surface = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);
	else
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);

	if(surface == NULL) {
		fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
	}

	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, SDL_MapRGB(surface->format,255,0,255));

	SDL_Surface *cleanup = surface;
	surface = SDL_DisplayFormat(surface);
	SDL_FreeSurface(cleanup);

	return surface;
}

SDL_Surface* loadGraphicSurface(std::string filename, std::string errormessage, bool IfNotFoundExit, bool HavePinkColorKey)
{
	SDL_Surface *ret = NULL;
	SDL_Surface *cleanup = IMG_Load(mods->locate(filename).c_str());
	if(!cleanup) {
		if (!errormessage.empty())
			fprintf(stderr, "%s: %s\n", errormessage.c_str(), IMG_GetError());
		if (IfNotFoundExit) {
			SDL_Quit();
			exit(1);
		}
	} else {
		if (HavePinkColorKey)
			SDL_SetColorKey(cleanup, SDL_SRCCOLORKEY, SDL_MapRGB(cleanup->format, 255, 0, 255));
		ret = SDL_DisplayFormatAlpha(cleanup);
		SDL_FreeSurface(cleanup);
	}
	return ret;
}

/*
 * Returns false if a pixel at Point px is transparent
 *
 * Source: SDL Documentation
 * http://www.libsdl.org/cgi/docwiki.cgi/Introduction_to_SDL_Video#getpixel
 */
bool checkPixel(Point px, SDL_Surface *surface) {
	SDL_LockSurface(surface);

	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + px.y * surface->pitch + px.x * bpp;
	Uint32 pixel;

	switch (bpp) {
		case 1:
			pixel = *p;
			break;

		case 2:
			pixel = *(Uint16 *)p;
			break;

		case 3:
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
				pixel = p[0] << 16 | p[1] << 8 | p[2];
#else
				pixel = p[0] | p[1] << 8 | p[2] << 16;
#endif
			break;

		case 4:
			pixel = *(Uint32 *)p;
			break;

		default:
			SDL_UnlockSurface(surface);
			return false;
	}

	Uint8 r,g,b,a;
	SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

	if (r == 255 && g == 0 && b ==255 && a == 255) {
		SDL_UnlockSurface(surface);
		return false;
	}
	if (a == 0) {
		SDL_UnlockSurface(surface);
		return false;
	}

	SDL_UnlockSurface(surface);

	return true;
}

SDL_Surface* scaleSurface(SDL_Surface *source, int width, int height)
{
	if(!source || !width || !height)
		return 0;

	double _stretch_factor_x, _stretch_factor_y;
	SDL_Surface *_ret = SDL_CreateRGBSurface(source->flags, width, height,
						 source->format->BitsPerPixel,
						 source->format->Rmask,
						 source->format->Gmask,
						 source->format->Bmask,
						 source->format->Amask);

	_stretch_factor_x = width / (double)source->w;
	_stretch_factor_y = height / (double)source->h;

	for(Uint32 y = 0; y < (Uint32)source->h; y++)
		for(Uint32 x = 0; x < (Uint32)source->w; x++)
		{
			Uint32 spixel = readPixel(source, x, y);
			for(Uint32 o_y = 0; o_y < _stretch_factor_y; ++o_y)
				for(Uint32 o_x = 0; o_x < _stretch_factor_x; ++o_x)
				{
					Uint32 dx = (Sint32)(_stretch_factor_x * x) + o_x;
					Uint32 dy = (Sint32)(_stretch_factor_y * y) + o_y;
					drawPixel(_ret, dx, dy, spixel);
				}
		}

	return _ret;
}

int calcDirection(const Point &src, const Point &dst)
{
	return calcDirection(src.x, src.y, dst.x, dst.y);
}

int calcDirection(int x0, int y0, int x1, int y1)
{
	// TODO: use calcTheta instead and check for the areas between -PI and PI

	// inverting Y to convert map coordinates to standard cartesian coordinates
	int dx = x1 - x0;
	int dy = y0 - y1;

	// avoid div by zero
	if (dx == 0) {
		if (dy > 0) return 3;
		else return 7;
	}

	float slope = ((float)dy)/((float)dx);
	if (0.5 <= slope && slope <= 2.0) {
		if (dy > 0) return 4;
		else return 0;
	}
	if (-0.5 <= slope && slope <= 0.5) {
		if (dx > 0) return 5;
		else return 1;
	}
	if (-2.0 <= slope && slope <= -0.5) {
		if (dx > 0) return 6;
		else return 2;
	}
	if (2.0 <= slope || -2.0 >= slope) {
		if (dy > 0) return 3;
		else return 7;
	}
	return 0;
}

// convert cartesian to polar theta where (x1,x2) is the origin
float calcTheta(int x1, int y1, int x2, int y2) {

	float pi = 3.1415926535898f;

	// calculate base angle
	float dx = (float)x2 - (float)x1;
	float dy = (float)y2 - (float)y1;
	int exact_dx = x2 - x1;
	float theta;

	// convert cartesian to polar coordinates
	if (exact_dx == 0) {
		if (dy > 0.0) theta = pi/2.0f;
		else theta = -pi/2.0f;
	}
	else {
		theta = atan(dy/dx);
		if (dx < 0.0 && dy >= 0.0) theta += pi;
		if (dx < 0.0 && dy < 0.0) theta -= pi;
	}
	return theta;
}

void setupSDLVideoMode(unsigned width, unsigned height)
{
	Uint32 flags = 0;

	if (FULLSCREEN) flags = flags | SDL_FULLSCREEN;
	if (DOUBLEBUF) flags = flags | SDL_DOUBLEBUF;
	if (HWSURFACE)
		flags = flags | SDL_HWSURFACE | SDL_HWACCEL;
	else
		flags = flags | SDL_SWSURFACE;

	screen = SDL_SetVideoMode (width, height, 0, flags);
}

