/*
Copyright © 2013 Kurt Rinnert

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

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include "SDL_gfxBlitFunc.h"

#include "SharedResources.h"
#include "Settings.h"

#include "SDLRenderDevice.h"

using namespace std;

Sprite::Sprite()
	: dest()
	, local_frame(SDL_Rect())
	, keep_graphics(false)
	, sprite(NULL)
	, src(SDL_Rect())
	, offset()
{}

Sprite::Sprite(const Sprite& other)
	: dest(other.dest)
	, local_frame(other.local_frame)
	, keep_graphics(false)
	, src(other.src)
	, offset(other.offset)
{
	if (other.sprite != NULL) {
		sprite = SDL_DisplayFormatAlpha(other.sprite);
	} else {
		sprite = NULL;
	}
}

Sprite& Sprite::operator=(const Sprite& other) {
	if (other.sprite != NULL) {
		sprite = SDL_DisplayFormatAlpha(other.sprite);
	} else {
		sprite = NULL;
	}
	dest = other.dest;
	local_frame = other.local_frame;
	keep_graphics = other.keep_graphics;
	src = other.src;
	offset = other.offset;

	return *this;
}

Sprite::~Sprite() {
	if (sprite != NULL && !keep_graphics) {
		SDL_FreeSurface(sprite);
		sprite = NULL;
	}
}

/**
 * Set the graphics context of a Sprite.
 * Initialize graphics resources. That is the SLD_surface buffer
 *
 * It is important that, if the client owns the graphics resources,
 * clearGraphics() method is called first in case this Sprite holds the
 * last references to avoid resource leaks.
 */
void Sprite::setGraphics(Image *s, bool setClipToFull) {

	sprite = s;

	if (setClipToFull && s != NULL) {
		src.x = 0;
		src.y = 0;
		src.w = sprite->w;
		src.h = sprite->h;
	}

}

Image * Sprite::getGraphics() {

	return sprite;
}

bool Sprite::graphicsIsNull() {

	return (sprite == NULL);
}

/**
 * Clear the graphics context of a Sprite.
 * Release graphics resources. That is the SLD_surface buffer
 *
 * It is important that this method is only called by clients who own the
 * graphics resources.
 */
void Sprite::clearGraphics() {

	if (sprite != NULL) {
		SDL_FreeSurface(sprite);
		sprite = NULL;
	}
}

void Sprite::setOffset(const Point& _offset) {
	this->offset = _offset;
}

void Sprite::setOffset(const int x, const int y) {
	this->offset.x = x;
	this->offset.y = y;
}

Point Sprite::getOffset() {

	return offset;
}
/**
 * Set the clipping rectangle for the sprite
 */
void Sprite::setClip(const SDL_Rect& clip) {
	src = clip;
}

/**
 * Set the clipping rectangle for the sprite
 */
void Sprite::setClip(const int x, const int y, const int w, const int h) {
	src.x = x;
	src.y = y;
	src.w = w;
	src.h = h;
}

void Sprite::setClipX(const int x) {
	src.x = x;
}

void Sprite::setClipY(const int y) {
	src.y = y;
}

void Sprite::setClipW(const int w) {
	src.w = w;
}

void Sprite::setClipH(const int h) {
	src.h = h;
}


SDL_Rect Sprite::getClip() {
	return src;
}
void Sprite::setDest(const SDL_Rect& _dest) {
	dest.x = (float)_dest.x;
	dest.y = (float)_dest.y;
}

void Sprite::setDest(const Point& _dest) {
	dest.x = (float)_dest.x;
	dest.y = (float)_dest.y;
}

void Sprite::setDest(int x, int y) {
	dest.x = (float)x;
	dest.y = (float)y;
}

FPoint Sprite::getDest() {
	return dest;
}

int Sprite::getGraphicsWidth() {
	return sprite->w;
}

int Sprite::getGraphicsHeight() {
	return sprite->h;
}

SDLRenderDevice::SDLRenderDevice() {
	cout << "Using Render Device: SDLRenderDevice" << endl;
}

int SDLRenderDevice::createContext(int width, int height) {

	if (is_initialized) {
		SDL_FreeSurface(screen);
	}

	Uint32 flags = 0;

	if (FULLSCREEN) flags = flags | SDL_FULLSCREEN;
	if (DOUBLEBUF) flags = flags | SDL_DOUBLEBUF;
	if (HWSURFACE)
		flags = flags | SDL_HWSURFACE | SDL_HWACCEL;
	else
		flags = flags | SDL_SWSURFACE;

	screen = SDL_SetVideoMode (width, height, 0, flags);

	if (screen == NULL && !is_initialized) {
			// If this is the first attempt and it failed we are not
			// getting anywhere.
			SDL_Quit();
			exit(1);
	}
	else {
		is_initialized = true;
	}

	return (screen != NULL ? 0 : -1);
}

SDL_Rect SDLRenderDevice::getContextSize()
{
	SDL_Rect size;
	size.x = size.y = 0;
	size.h = screen->h;
	size.w = screen->w;
	return size;
}

int SDLRenderDevice::render(Renderable& r, SDL_Rect dest) {
	return SDL_BlitSurface(r.sprite, &r.src, screen, &dest);
}

int SDLRenderDevice::render(Sprite& r) {
	if (r.graphicsIsNull()) {
		return -1;
	}
	if ( !local_to_global(r) ) {
		return -1;
	}

	return SDL_BlitSurface(r.getGraphics(), &m_clip, screen, &m_dest);
}

int SDLRenderDevice::renderImage(Image* image, SDL_Rect& src) {
	return SDL_BlitSurface(image, &src, screen , 0);
}

int SDLRenderDevice::renderToImage(Image* src_image, SDL_Rect& src, Image* dest_image, SDL_Rect& dest, bool dest_is_transparent) {
	if (dest_is_transparent)
		return SDL_gfxBlitRGBA(src_image, &src, dest_image, &dest);
	else
		return SDL_BlitSurface(src_image, &src, dest_image, &dest);
}

int SDLRenderDevice::renderText(
	TTF_Font *ttf_font,
	const std::string& text,
	SDL_Color color,
	SDL_Rect& dest
) {
	int ret = 0;
	m_ttf_renderable.setGraphics(TTF_RenderUTF8_Blended(ttf_font, text.c_str(), color));
	if (!m_ttf_renderable.graphicsIsNull()) {
		SDL_Rect clip = m_ttf_renderable.getClip();
		ret = SDL_BlitSurface(
				  m_ttf_renderable.getGraphics(),
				  &clip,
				  screen,
				  &dest
			  );
		SDL_FreeSurface(m_ttf_renderable.getGraphics());
	}
	else {
		ret = -1;
	}

	return ret;
}



void SDLRenderDevice::drawPixel(
	int x,
	int y,
	Uint32 color
) {
	int bpp = screen->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)screen->pixels + y * screen->pitch + x * bpp;

	if (SDL_MUSTLOCK(screen)) {
		SDL_LockSurface(screen);
	}
	switch(bpp) {
		case 1:
			*p = color;
			break;

		case 2:
			*(Uint16 *)p = color;
			break;

		case 3:
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			p[0] = (color >> 16) & 0xff;
			p[1] = (color >> 8) & 0xff;
			p[2] = color & 0xff;
#else
			p[0] = color & 0xff;
			p[1] = (color >> 8) & 0xff;
			p[2] = (color >> 16) & 0xff;
#endif
			break;

		case 4:
			*(Uint32 *)p = color;
			break;
	}
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}

	return;
}

void SDLRenderDevice::drawLine(
	int x0,
	int y0,
	int x1,
	int y1,
	Uint32 color
) {
	const int dx = abs(x1-x0);
	const int dy = abs(y1-y0);
	const int sx = x0 < x1 ? 1 : -1;
	const int sy = y0 < y1 ? 1 : -1;
	int err = dx-dy;

	do {
		//skip draw if outside screen
		if (x0 > 0 && y0 > 0 && x0 < VIEW_W && y0 < VIEW_H) {
			this->drawPixel(x0,y0,color);
		}

		int e2 = 2*err;
		if (e2 > -dy) {
			err = err - dy;
			x0 = x0 + sx;
		}
		if (e2 <  dx) {
			err = err + dx;
			y0 = y0 + sy;
		}
	}
	while(x0 != x1 || y0 != y1);
}

void SDLRenderDevice::drawLine(
	const Point& p0,
	const Point& p1,
	Uint32 color
) {
	if (SDL_MUSTLOCK(screen)) {
		SDL_LockSurface(screen);
	}
	this->drawLine(p0.x, p0.y, p1.x, p1.y, color);
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
}

void SDLRenderDevice::drawRectangle(
	const Point& p0,
	const Point& p1,
	Uint32 color
) {
	if (SDL_MUSTLOCK(screen)) {
		SDL_LockSurface(screen);
	}
	this->drawLine(p0.x, p0.y, p1.x, p0.y, color);
	this->drawLine(p1.x, p0.y, p1.x, p1.y, color);
	this->drawLine(p0.x, p0.y, p0.x, p1.y, color);
	this->drawLine(p0.x, p1.y, p1.x, p1.y, color);
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
}

void SDLRenderDevice::blankScreen() {
	SDL_FillRect(screen, NULL, 0);
	return;
}

void SDLRenderDevice::commitFrame() {
	SDL_Flip(screen);
	return;
}

void SDLRenderDevice::destroyContext() {
	// Nothing to be done; SDL_Quit() will handle it all
	// for this render device.
	return;
}

Uint32 SDLRenderDevice::MapRGB(SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b)
{
	return SDL_MapRGB(fmt, r, g, b);
}

Uint32 SDLRenderDevice::MapRGB(Uint8 r, Uint8 g, Uint8 b)
{
	return SDL_MapRGB(screen->format, r, g, b);
}

Uint32 SDLRenderDevice::MapRGBA(SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return SDL_MapRGBA(fmt, r, g, b, a);
}

Uint32 SDLRenderDevice::MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return SDL_MapRGBA(screen->format, r, g, b, a);
}

bool SDLRenderDevice::local_to_global(Sprite& r) {
	m_clip = r.getClip();

	int left = r.dest.x - r.getOffset().x;
	int right = left + r.getClip().w;
	int up = r.dest.y - r.getOffset().y;
	int down = up + r.getClip().h;

	// Check whether we need to render.
	// If so, compute the correct clipping.
	if (r.local_frame.w) {
		if (left > r.local_frame.w) {
			return false;
		}
		if (right < 0) {
			return false;
		}
		if (left < 0) {
			m_clip.x = r.getClip().x - left;
			left = 0;
		};
		right = (right < r.local_frame.w ? right : r.local_frame.w);
		m_clip.w = right - left;
	}
	if (r.local_frame.h) {
		if (up > r.local_frame.h) {
			return false;
		}
		if (down < 0) {
			return false;
		}
		if (up < 0) {
			m_clip.y = r.getClip().y - up;
			up = 0;
		};
		down = (down < r.local_frame.h ? down : r.local_frame.h);
		m_clip.h = down - up;
	}

	m_dest.x = left + r.local_frame.x;
	m_dest.y = up + r.local_frame.y;

	return true;
}


Uint32 readPixel(Image *surface, int x, int y) {
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
void drawPixel(Image *surface, int x, int y, Uint32 pixel) {
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
 * NOTE: The surface must be locked before calling this!
 *
 * from http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm#Simplification
 */
void drawLine(Image *surface, int x0, int y0, int x1, int y1, Uint32 color) {
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
	}
	while(x0 != x1 || y0 != y1);
}

void drawLine(Image *surface, Point pos0, Point pos1, Uint32 color) {
	if (SDL_MUSTLOCK(surface))
		SDL_LockSurface(surface);
	drawLine(surface, pos0.x, pos0.y, pos1.x, pos1.y, color);
	if (SDL_MUSTLOCK(surface))
		SDL_UnlockSurface(surface);
}

void drawRectangle(Image *surface, Point pos0, Point pos1, Uint32 color) {
	if (SDL_MUSTLOCK(surface))
		SDL_LockSurface(surface);
	drawLine(surface, pos0.x, pos0.y, pos1.x, pos0.y, color);
	drawLine(surface, pos1.x, pos0.y, pos1.x, pos1.y, color);
	drawLine(surface, pos0.x, pos0.y, pos0.x, pos1.y, color);
	drawLine(surface, pos0.x, pos1.y, pos1.x, pos1.y, color);
	if (SDL_MUSTLOCK(surface))
		SDL_UnlockSurface(surface);
}


/**
 * create blank surface
 * based on example: http://www.libsdl.org/docs/html/sdlcreatergbsurface.html
 */
Image* createAlphaSurface(int width, int height) {

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

Image* createSurface(int width, int height) {

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
	else {

		SDL_SetColorKey(surface, SDL_SRCCOLORKEY, render_device->MapRGB(surface->format,255,0,255));

		SDL_Surface *cleanup = surface;
		surface = SDL_DisplayFormat(surface);
		SDL_FreeSurface(cleanup);
	}

	return surface;
}

Image* loadGraphicSurface(std::string filename, std::string errormessage, bool IfNotFoundExit, bool HavePinkColorKey) {
	SDL_Surface *ret = NULL;
	SDL_Surface *cleanup = IMG_Load(mods->locate(filename).c_str());
	if(!cleanup) {
		if (!errormessage.empty())
			fprintf(stderr, "%s: %s\n", errormessage.c_str(), IMG_GetError());
		if (IfNotFoundExit) {
			SDL_Quit();
			exit(1);
		}
	}
	else {
		if (HavePinkColorKey)
			SDL_SetColorKey(cleanup, SDL_SRCCOLORKEY, render_device->MapRGB(cleanup->format, 255, 0, 255));
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
bool checkPixel(Point px, Image *surface) {
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

Image* scaleSurface(Image *source, int width, int height) {
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
		for(Uint32 x = 0; x < (Uint32)source->w; x++) {
			Uint32 spixel = readPixel(source, x, y);
			for(Uint32 o_y = 0; o_y < _stretch_factor_y; ++o_y)
				for(Uint32 o_x = 0; o_x < _stretch_factor_x; ++o_x) {
					Uint32 dx = (Sint32)(_stretch_factor_x * x) + o_x;
					Uint32 dy = (Sint32)(_stretch_factor_y * y) + o_y;
					drawPixel(_ret, dx, dy, spixel);
				}
		}

	return _ret;
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

void loadIcons() {
	icons.clearGraphics();
	icons.setGraphics(loadGraphicSurface("images/icons/icons.png", "Couldn't load icons", false), false);
}

void freeImage(Image* image) {
	if (image != NULL) {
		SDL_FreeSurface(image);
		image = NULL;
	}
}
