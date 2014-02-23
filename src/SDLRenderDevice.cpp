/*
Copyright © 2013 Kurt Rinnert
Copyright © 2013 Igor Paliychuk
Copyright © 2014 Henrik Andersson

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

SDLImage::SDLImage(RenderDevice *_device)
	: Image(_device)
	, surface(NULL) {
}

SDLImage::~SDLImage() {
}

int SDLImage::getWidth() const {
	return surface ? surface->w : 0;
}

int SDLImage::getHeight() const {
	return surface ? surface->h : 0;
}

SDLRenderDevice::SDLRenderDevice()
	: screen(NULL)
	, titlebar_icon(NULL) {
	cout << "Using Render Device: SDLRenderDevice" << endl;
}

int SDLRenderDevice::createContext(int width, int height) {
	if (is_initialized) {
		destroyContext();
	}

	// Add Window Titlebar Icon
	titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());
	SDL_WM_SetIcon(titlebar_icon, NULL);

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

	if (is_initialized) {
		// Window title
		const char* title = msg->get(WINDOW_TITLE).c_str();
		SDL_WM_SetCaption(title, title);
	}

	return (screen != NULL ? 0 : -1);
}

Rect SDLRenderDevice::getContextSize() {
	Rect size;
	size.x = size.y = 0;
	size.h = screen->h;
	size.w = screen->w;
	return size;
}

int SDLRenderDevice::render(Renderable& r, Rect dest) {
	SDL_Rect src = r.src;
	SDL_Rect _dest = dest;
	return SDL_BlitSurface(static_cast<SDLImage *>(r.sprite)->surface, &src, screen, &_dest);
}

int SDLRenderDevice::render(Sprite *r) {
	if (r == NULL) {
		return -1;
	}

	if ( !localToGlobal(r) ) {
		return -1;
	}

	SDL_Rect src = m_clip;
	SDL_Rect dest = m_dest;
	return SDL_BlitSurface(static_cast<SDLImage *>(r->getGraphics())->surface, &src, screen, &dest);
}

int SDLRenderDevice::renderImage(Image* image, Rect& src) {
	if (!image) return -1;
	SDL_Rect _src = src;
	return SDL_BlitSurface(static_cast<SDLImage *>(image)->surface, &_src, screen , 0);
}

int SDLRenderDevice::renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest, bool dest_is_transparent) {
	if (!src_image || !dest_image) return -1;

	SDL_Rect _src = src;
	SDL_Rect _dest = dest;

	if (dest_is_transparent)
		return SDL_gfxBlitRGBA(static_cast<SDLImage *>(src_image)->surface, &_src,
							   static_cast<SDLImage *>(dest_image)->surface, &_dest);
	else
		return SDL_BlitSurface(static_cast<SDLImage *>(src_image)->surface, &_src,
							   static_cast<SDLImage *>(dest_image)->surface, &_dest);
}

int SDLRenderDevice::renderText(
	TTF_Font *ttf_font,
	const std::string& text,
	Color color,
	Rect& dest
) {
	int ret = 0;
	SDL_Color _color = color;

	SDL_Surface *surface = TTF_RenderUTF8_Blended(ttf_font, text.c_str(), _color);

	if (surface == NULL)
		return -1;

	SDL_Rect _dest = dest;
	ret = SDL_BlitSurface(surface, NULL, screen, &_dest);

	SDL_FreeSurface(surface);

	return ret;
}

Image * SDLRenderDevice::renderTextToImage(TTF_Font* ttf_font, const std::string& text, Color color, bool blended) {
	SDLImage *image = new SDLImage(this);
	SDL_Color _color = color;

	if (blended)
		image->surface = TTF_RenderUTF8_Blended(ttf_font, text.c_str(), _color);
	else
		image->surface = TTF_RenderUTF8_Solid(ttf_font, text.c_str(), _color);

	if (image->surface)
		return image;

	delete image;
	return NULL;
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

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 *
 * Source: SDL Documentation
 * http://www.libsdl.org/docs/html/guidevideo.html
 */
void SDLRenderDevice::drawPixel(Image *image, int x, int y, Uint32 pixel) {
	if (!image || !static_cast<SDLImage *>(image)->surface) return;
	SDL_Surface *surface = static_cast<SDLImage *>(image)->surface;

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
	if (titlebar_icon)
		SDL_FreeSurface(titlebar_icon);
	if (screen)
		SDL_FreeSurface(screen);

	return;
}

void SDLRenderDevice::fillImageWithColor(Image *dst, Rect *dstrect, Uint32 color) {
	if (!dst) return;
	if (dstrect) {
		SDL_Rect dest = *dstrect;
		SDL_FillRect(static_cast<SDLImage *>(dst)->surface, &dest, color);
	}
	else {
		SDL_FillRect(static_cast<SDLImage *>(dst)->surface, NULL, color);
	}
}

Uint32 SDLRenderDevice::MapRGB(Image *src, Uint8 r, Uint8 g, Uint8 b) {
	if (!src || !static_cast<SDLImage *>(src)->surface) return 0;
	return SDL_MapRGB(static_cast<SDLImage *>(src)->surface->format, r, g, b);
}

Uint32 SDLRenderDevice::MapRGB(Uint8 r, Uint8 g, Uint8 b) {
	return SDL_MapRGB(screen->format, r, g, b);
}

Uint32 SDLRenderDevice::MapRGBA(Image *src, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	if (!src || !static_cast<SDLImage *>(src)->surface) return 0;
	return SDL_MapRGBA(static_cast<SDLImage *>(src)->surface->format, r, g, b, a);
}

Uint32 SDLRenderDevice::MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	return SDL_MapRGBA(screen->format, r, g, b, a);
}

/**
 * create blank surface
 * based on example: http://www.libsdl.org/docs/html/sdlcreatergbsurface.html
 */
Image *SDLRenderDevice::createAlphaSurface(int width, int height) {

	SDLImage *image = new SDLImage(this);
	Uint32 rmask, gmask, bmask, amask;

	setSDL_RGBA(&rmask, &gmask, &bmask, &amask);

	if (HWSURFACE)
		image->surface = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCALPHA, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);
	else
		image->surface = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);

	if(image->surface == NULL) {
		fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
		delete image;
		return NULL;
	}

	// optimize
	SDL_Surface *cleanup = image->surface;
	image->surface = SDL_DisplayFormatAlpha(image->surface);
	SDL_FreeSurface(cleanup);

	return image;
}

Image *SDLRenderDevice::createSurface(int width, int height) {

	SDLImage *image = new SDLImage(this);
	Uint32 rmask, gmask, bmask, amask;

	setSDL_RGBA(&rmask, &gmask, &bmask, &amask);

	if (HWSURFACE)
		image->surface = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);
	else
		image->surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);

	if(image->surface == NULL) {
		fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
		delete image;
		return NULL;
	}

	SDL_SetColorKey(image->surface, SDL_SRCCOLORKEY, SDL_MapRGB(image->surface->format,255,0,255));
	SDL_Surface *cleanup = image->surface;
	image->surface = SDL_DisplayFormat(image->surface);
	SDL_FreeSurface(cleanup);

	return image;
}

void SDLRenderDevice::setGamma(float g) {
	SDL_SetGamma(g, g, g);
}

void SDLRenderDevice::listModes(std::vector<Rect> &modes) {
	SDL_Rect** detect_modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

	// Check if there are any modes available
	if (detect_modes == (SDL_Rect**)0) {
		fprintf(stderr, "No modes available!\n");
		return;
	}

	// Check if our resolution is restricted
	if (detect_modes == (SDL_Rect**)-1) {
		fprintf(stderr, "All resolutions available.\n");
	}

	for (unsigned i=0; detect_modes[i]; ++i) {
		modes.push_back(Rect(*detect_modes[i]));
		if (detect_modes[i]->w < MIN_VIEW_W || detect_modes[i]->h < MIN_VIEW_H) {
			// make sure the resolution fits in the constraints of MIN_VIEW_W and MIN_VIEW_H
			modes.pop_back();
		}
		else {
			// check previous resolutions for duplicates. If one is found, drop the one we just added
			for (unsigned j=0; j<modes.size()-1; ++j) {
				if (modes[j].w == detect_modes[i]->w && modes[j].h == detect_modes[i]->h) {
					modes.pop_back();
					break;
				}
			}
		}
	}

}


Image *SDLRenderDevice::loadGraphicSurface(std::string filename, std::string errormessage, bool IfNotFoundExit, bool HavePinkColorKey) {
	// lookup image in cache
	Image *img;
	img = cacheLookup(filename);
	if (img != NULL) return img;

	// load image
	SDLImage *image;
	image = NULL;
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
			SDL_SetColorKey(cleanup, SDL_SRCCOLORKEY, SDL_MapRGB(cleanup->format, 255, 0, 255));
		image = new SDLImage(this);
		image->surface = SDL_DisplayFormatAlpha(cleanup);
		SDL_FreeSurface(cleanup);
	}

	// store image to cache
	cacheStore(filename, image);
	return image;
}

void SDLRenderDevice::scaleSurface(Image *source, int width, int height) {
	if(!source || !width || !height)
		return;

	SDLImage *scaled = new SDLImage(this);
	SDL_Surface *surface = static_cast<SDLImage *>(source)->surface;
	scaled->surface = SDL_CreateRGBSurface(surface->flags, width, height,
										   surface->format->BitsPerPixel,
										   surface->format->Rmask,
										   surface->format->Gmask,
										   surface->format->Bmask,
										   surface->format->Amask);

	if (scaled->surface) {
		double _stretch_factor_x, _stretch_factor_y;
		_stretch_factor_x = width / (double)surface->w;
		_stretch_factor_y = height / (double)surface->h;

		for(Uint32 y = 0; y < (Uint32)surface->h; y++) {
			for(Uint32 x = 0; x < (Uint32)surface->w; x++) {
				Uint32 spixel = readPixel(source, x, y);
				for(Uint32 o_y = 0; o_y < _stretch_factor_y; ++o_y) {
					for(Uint32 o_x = 0; o_x < _stretch_factor_x; ++o_x) {
						Uint32 dx = (Sint32)(_stretch_factor_x * x) + o_x;
						Uint32 dy = (Sint32)(_stretch_factor_y * y) + o_y;
						drawPixel(scaled, dx, dy, spixel);
					}
				}
			}
		}
		/* swap surface from scaled to source */
		SDL_FreeSurface(surface);
		static_cast<SDLImage *>(source)->surface = scaled->surface;
		scaled->surface = NULL;
		scaled->unref();
	}
}

Uint32 SDLRenderDevice::readPixel(Image *image, int x, int y) {
	if (!image || !static_cast<SDLImage *>(image)->surface) return 0;
	SDL_Surface *surface = static_cast<SDLImage *>(image)->surface;

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
 * Returns false if a pixel at Point px is transparent
 *
 * Source: SDL Documentation
 * http://www.libsdl.org/cgi/docwiki.cgi/Introduction_to_SDL_Video#getpixel
 */
bool SDLRenderDevice::checkPixel(Point px, Image *image) {
	if (!image || !static_cast<SDLImage *>(image)->surface) return false;
	SDL_Surface *surface = static_cast<SDLImage *>(image)->surface;

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

void SDLRenderDevice::freeImage(Image *image) {
	if (!image) return;

	cacheRemove(image);

	if (static_cast<SDLImage *>(image)->surface)
		SDL_FreeSurface(static_cast<SDLImage *>(image)->surface);
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

