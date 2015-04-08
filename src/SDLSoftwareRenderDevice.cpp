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
#include <string.h>
#include "SDL_gfxBlitFunc.h"

#include "SharedResources.h"
#include "Settings.h"

#include "SDLSoftwareRenderDevice.h"
#include "SDLFontEngine.h"

SDLSoftwareImage::SDLSoftwareImage(RenderDevice *_device)
	: Image(_device)
	, surface(NULL) {
}

SDLSoftwareImage::~SDLSoftwareImage() {
}

int SDLSoftwareImage::getWidth() const {
	return surface ? surface->w : 0;
}

int SDLSoftwareImage::getHeight() const {
	return surface ? surface->h : 0;
}

void SDLSoftwareImage::fillWithColor(Uint32 color) {
	if (!surface) return;

	SDL_FillRect(surface, NULL, color);
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 *
 * Source: SDL Documentation
 * http://www.libsdl.org/docs/html/guidevideo.html
 */
void SDLSoftwareImage::drawPixel(int x, int y, Uint32 pixel) {
	if (!surface) return;

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

Uint32 SDLSoftwareImage::MapRGB(Uint8 r, Uint8 g, Uint8 b) {
	if (!surface) return 0;
	return SDL_MapRGB(surface->format, r, g, b);
}

Uint32 SDLSoftwareImage::MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	if (!surface) return 0;
	return SDL_MapRGBA(surface->format, r, g, b, a);
}

/**
 * Resizes an image
 * Deletes the original image and returns a pointer to the resized version
 */
Image* SDLSoftwareImage::resize(int width, int height) {
	if(!surface || width <= 0 || height <= 0)
		return NULL;

	SDLSoftwareImage *scaled = new SDLSoftwareImage(device);

	if (scaled) {
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
					Uint32 spixel = readPixel(x, y);
					for(Uint32 o_y = 0; o_y < _stretch_factor_y; ++o_y) {
						for(Uint32 o_x = 0; o_x < _stretch_factor_x; ++o_x) {
							Uint32 dx = (Sint32)(_stretch_factor_x * x) + o_x;
							Uint32 dy = (Sint32)(_stretch_factor_y * y) + o_y;
							scaled->drawPixel(dx, dy, spixel);
						}
					}
				}
			}
			// delete the old image and return the new one
			this->unref();
			return scaled;
		}
		else {
			delete scaled;
		}
	}

	return NULL;
}

Uint32 SDLSoftwareImage::readPixel(int x, int y) {
	if (!surface) return 0;

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

SDLSoftwareRenderDevice::SDLSoftwareRenderDevice()
	: screen(NULL)
	, window(NULL)
	, renderer(NULL)
	, texture(NULL)
	, titlebar_icon(NULL)
	, title(NULL) {
	logInfo("Using Render Device: SDLSoftwareRenderDevice (software, SDL 2)");

	fullscreen = FULLSCREEN;
	hwsurface = HWSURFACE;
	vsync = VSYNC;
	texture_filter = TEXTURE_FILTER;

	min_screen.x = MIN_SCREEN_W;
	min_screen.y = MIN_SCREEN_H;
}

int SDLSoftwareRenderDevice::createContext() {
	bool settings_changed = (fullscreen != FULLSCREEN || hwsurface != HWSURFACE || vsync != VSYNC || texture_filter != TEXTURE_FILTER);

	Uint32 w_flags = 0;
	Uint32 r_flags = 0;
	int window_w = SCREEN_W;
	int window_h = SCREEN_H;

	if (FULLSCREEN) {
		w_flags = w_flags | SDL_WINDOW_FULLSCREEN_DESKTOP;

		// make the window the same size as the desktop resolution
		SDL_DisplayMode desktop;
		if (SDL_GetDesktopDisplayMode(0, &desktop) == 0) {
			window_w = desktop.w;
			window_h = desktop.h;
		}
	}
	else if (fullscreen && is_initialized) {
		// if the game was previously in fullscreen, resize the window when returning to windowed mode
		window_w = MIN_SCREEN_W;
		window_h = MIN_SCREEN_H;
	}

	w_flags = w_flags | SDL_WINDOW_RESIZABLE;

	if (HWSURFACE) {
		r_flags = r_flags | SDL_RENDERER_ACCELERATED;
	}
	else {
		r_flags = r_flags | SDL_RENDERER_SOFTWARE;
		VSYNC = false; // can't have software mode & vsync at the same time
	}
	if (VSYNC) r_flags = r_flags | SDL_RENDERER_PRESENTVSYNC;

	if (settings_changed || !is_initialized) {
		if (is_initialized) {
			destroyContext();
		}

		window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, w_flags);
		if (window) {
			renderer = SDL_CreateRenderer(window, -1, r_flags);
			if (renderer) {
				if (TEXTURE_FILTER && !IGNORE_TEXTURE_FILTER)
					SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
				else
					SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

				windowResize();
			}

			SDL_SetWindowMinimumSize(window, MIN_SCREEN_W, MIN_SCREEN_H);
			// setting minimum size might move the window, so set position again
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		bool window_created = window != NULL && renderer != NULL && screen != NULL && texture != NULL;

		if (!window_created && !is_initialized) {
			// If this is the first attempt and it failed we are not
			// getting anywhere.
			logError("SDLSoftwareRenderDevice: createContext() failed: %s", SDL_GetError());
			SDL_Quit();
			exit(1);
		}
		else if (!window_created) {
			// try previous setting first
			FULLSCREEN = fullscreen;
			HWSURFACE = hwsurface;
			VSYNC = vsync;
			TEXTURE_FILTER = texture_filter;
			if (createContext() == -1) {
				// last resort, try turning everything off
				FULLSCREEN = false;
				HWSURFACE = false;
				VSYNC = false;
				TEXTURE_FILTER = false;
				return createContext();
			}
			else {
				return 0;
			}
		}
		else {
			fullscreen = FULLSCREEN;
			hwsurface = HWSURFACE;
			vsync = VSYNC;
			texture_filter = TEXTURE_FILTER;
			is_initialized = true;
		}
	}

	if (is_initialized) {
		// update minimum window size if it has changed
		if (min_screen.x != MIN_SCREEN_W || min_screen.y != MIN_SCREEN_H) {
			min_screen.x = MIN_SCREEN_W;
			min_screen.y = MIN_SCREEN_H;
			SDL_SetWindowMinimumSize(window, MIN_SCREEN_W, MIN_SCREEN_H);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		windowResize();

		// update title bar text and icon
		updateTitleBar();

		// load persistent resources
		SharedResources::loadIcons();
		curs = new CursorManager();
	}

	return (is_initialized ? 0 : -1);
}

Rect SDLSoftwareRenderDevice::getContextSize() {
	Rect size;
	size.x = size.y = 0;
	size.h = screen->h;
	size.w = screen->w;
	return size;
}

int SDLSoftwareRenderDevice::render(Renderable& r, Rect dest) {
	SDL_Rect src = r.src;
	SDL_Rect _dest = dest;
	return SDL_BlitSurface(static_cast<SDLSoftwareImage *>(r.image)->surface, &src, screen, &_dest);
}

int SDLSoftwareRenderDevice::render(Sprite *r) {
	if (r == NULL) {
		return -1;
	}

	if ( !localToGlobal(r) ) {
		return -1;
	}

	SDL_Rect src = m_clip;
	SDL_Rect dest = m_dest;
	return SDL_BlitSurface(static_cast<SDLSoftwareImage *>(r->getGraphics())->surface, &src, screen, &dest);
}

int SDLSoftwareRenderDevice::renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest, bool dest_is_transparent) {
	if (!src_image || !dest_image) return -1;

	SDL_Rect _src = src;
	SDL_Rect _dest = dest;

	if (dest_is_transparent)
		return SDL_gfxBlitRGBA(static_cast<SDLSoftwareImage *>(src_image)->surface, &_src,
							   static_cast<SDLSoftwareImage *>(dest_image)->surface, &_dest);
	else
		return SDL_BlitSurface(static_cast<SDLSoftwareImage *>(src_image)->surface, &_src,
							   static_cast<SDLSoftwareImage *>(dest_image)->surface, &_dest);
}

int SDLSoftwareRenderDevice::renderText(
	FontStyle *font,
	const std::string& text,
	Color color,
	Rect& dest
) {
	int ret = 0;
	SDL_Color _color = color;

	SDL_Surface *surface = TTF_RenderUTF8_Blended(static_cast<SDLFontStyle *>(font)->ttfont, text.c_str(), _color);

	if (surface == NULL)
		return -1;

	SDL_Rect _dest = dest;
	ret = SDL_BlitSurface(surface, NULL, screen, &_dest);

	SDL_FreeSurface(surface);

	return ret;
}

Image* SDLSoftwareRenderDevice::renderTextToImage(FontStyle* font, const std::string& text, Color color, bool blended) {
	SDLSoftwareImage *image = new SDLSoftwareImage(this);
	if (!image) return NULL;

	SDL_Color _color = color;

	if (blended)
		image->surface = TTF_RenderUTF8_Blended(static_cast<SDLFontStyle *>(font)->ttfont, text.c_str(), _color);
	else
		image->surface = TTF_RenderUTF8_Solid(static_cast<SDLFontStyle *>(font)->ttfont, text.c_str(), _color);

	if (image->surface)
		return image;

	delete image;
	return NULL;
}

void SDLSoftwareRenderDevice::drawPixel(
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

void SDLSoftwareRenderDevice::drawLine(
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

void SDLSoftwareRenderDevice::drawRectangle(
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

void SDLSoftwareRenderDevice::blankScreen() {
	SDL_FillRect(screen, NULL, 0);
	return;
}

void SDLSoftwareRenderDevice::commitFrame() {
	SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	inpt->window_resized = false;

	return;
}

void SDLSoftwareRenderDevice::destroyContext() {
	if (title) {
		free(title);
		title = NULL;
	}
	if (titlebar_icon) {
		SDL_FreeSurface(titlebar_icon);
		titlebar_icon = NULL;
	}
	if (screen) {
		SDL_FreeSurface(screen);
		screen = NULL;
	}
	if (texture) {
		SDL_DestroyTexture(texture);
		texture = NULL;
	}
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}
	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

	return;
}

Uint32 SDLSoftwareRenderDevice::MapRGB(Uint8 r, Uint8 g, Uint8 b) {
	return SDL_MapRGB(screen->format, r, g, b);
}

Uint32 SDLSoftwareRenderDevice::MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	return SDL_MapRGBA(screen->format, r, g, b, a);
}

/**
 * create blank surface
 * based on example: http://www.libsdl.org/docs/html/sdlcreatergbsurface.html
 */
Image *SDLSoftwareRenderDevice::createImage(int width, int height) {

	SDLSoftwareImage *image = new SDLSoftwareImage(this);

	if (!image)
		return NULL;

	Uint32 rmask, gmask, bmask, amask;
	setSDL_RGBA(&rmask, &gmask, &bmask, &amask);

	image->surface = SDL_CreateRGBSurface(0, width, height, BITS_PER_PIXEL, rmask, gmask, bmask, amask);

	if(image->surface == NULL) {
		logError("SDLSoftwareRenderDevice: CreateRGBSurface failed: %s", SDL_GetError());
		delete image;
		return NULL;
	}

	// optimize
	SDL_Surface *cleanup = image->surface;
	image->surface = SDL_ConvertSurfaceFormat(cleanup, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_FreeSurface(cleanup);

	return image;
}

void SDLSoftwareRenderDevice::setGamma(float g) {
	Uint16 ramp[256];
	SDL_CalculateGammaRamp(g, ramp);
	SDL_SetWindowGammaRamp(window, ramp, ramp, ramp);
}

void SDLSoftwareRenderDevice::updateTitleBar() {
	if (title) free(title);
	if (titlebar_icon) SDL_FreeSurface(titlebar_icon);

	if (!window) return;

	title = strdup(msg->get(WINDOW_TITLE).c_str());
	titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());

	if (title) SDL_SetWindowTitle(window, title);
	if (titlebar_icon) SDL_SetWindowIcon(window, titlebar_icon);
}

Image *SDLSoftwareRenderDevice::loadImage(std::string filename, std::string errormessage, bool IfNotFoundExit) {
	// lookup image in cache
	Image *img;
	img = cacheLookup(filename);
	if (img != NULL) return img;

	// load image
	SDLSoftwareImage *image;
	image = NULL;
	SDL_Surface *cleanup = IMG_Load(mods->locate(filename).c_str());
	if(!cleanup) {
		if (!errormessage.empty())
			logError("SDLSoftwareRenderDevice: %s: %s", errormessage.c_str(), IMG_GetError());
		if (IfNotFoundExit) {
			SDL_Quit();
			exit(1);
		}
	}
	else {
		image = new SDLSoftwareImage(this);
		image->surface = SDL_ConvertSurfaceFormat(cleanup, SDL_PIXELFORMAT_ARGB8888, 0);
		SDL_FreeSurface(cleanup);
	}

	// store image to cache
	cacheStore(filename, image);
	return image;
}

void SDLSoftwareRenderDevice::freeImage(Image *image) {
	if (!image) return;

	cacheRemove(image);

	if (static_cast<SDLSoftwareImage *>(image)->surface)
		SDL_FreeSurface(static_cast<SDLSoftwareImage *>(image)->surface);
}

void SDLSoftwareRenderDevice::setSDL_RGBA(Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask) {
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

void SDLSoftwareRenderDevice::windowResize() {
	int w,h;
	SDL_GetWindowSize(window, &w, &h);
	SCREEN_W = w;
	SCREEN_H = h;

	float scale = (float)VIEW_H / (float)SCREEN_H;
	VIEW_W = (int)((float)SCREEN_W * scale);

	// letterbox if too tall
	if (VIEW_W < MIN_SCREEN_W) {
		VIEW_W = MIN_SCREEN_W;
	}

	VIEW_W_HALF = VIEW_W/2;

	SDL_RenderSetLogicalSize(renderer, VIEW_W, VIEW_H);

	if (texture) SDL_DestroyTexture(texture);
	if (screen) SDL_FreeSurface(screen);

	Uint32 rmask, gmask, bmask, amask;
	int bpp = (int)BITS_PER_PIXEL;
	SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ARGB8888, &bpp, &rmask, &gmask, &bmask, &amask);
	screen = SDL_CreateRGBSurface(0, VIEW_W, VIEW_H, bpp, rmask, gmask, bmask, amask);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, VIEW_W, VIEW_H);
}

