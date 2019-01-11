/*
Copyright © 2013 Kurt Rinnert
Copyright © 2013 Igor Paliychuk
Copyright © 2014 Henrik Andersson
Copyright © 2014-2016 Justin Jacobs

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

#include <SDL_image.h>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CursorManager.h"
#include "EngineSettings.h"
#include "IconManager.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "SharedResources.h"
#include "Settings.h"

#include "SDLSoftwareRenderDevice.h"
#include "SDLFontEngine.h"

SDLSoftwareImage::SDLSoftwareImage(RenderDevice *_device)
	: Image(_device)
	, surface(NULL) {
}

SDLSoftwareImage::~SDLSoftwareImage() {
	if (surface)
		SDL_FreeSurface(surface);
}

int SDLSoftwareImage::getWidth() const {
	return surface ? surface->w : 0;
}

int SDLSoftwareImage::getHeight() const {
	return surface ? surface->h : 0;
}

void SDLSoftwareImage::fillWithColor(const Color& color) {
	if (!surface) return;

	SDL_FillRect(surface, NULL, MapRGBA(color.r, color.g, color.b, color.a));
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 *
 * Source: SDL Documentation
 * http://www.libsdl.org/docs/html/guidevideo.html
 */
void SDLSoftwareImage::drawPixel(int x, int y, const Color& color) {
	if (!surface) return;

	Uint32 pixel = MapRGBA(color.r, color.g, color.b, color.a);

	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = static_cast<Uint8*>(surface->pixels) + y * surface->pitch + x * bpp;

	if (SDL_MUSTLOCK(surface)) {
		SDL_LockSurface(surface);
	}
	switch(bpp) {
		case 1:
			*p = static_cast<Uint8>(pixel);
			break;

		case 2:
			*(reinterpret_cast<Uint16*>(p)) = static_cast<Uint16>(pixel);
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
			*(reinterpret_cast<Uint32*>(p)) = pixel;
			break;
	}
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

void SDLSoftwareImage::drawLine(int x0, int y0, int x1, int y1, const Color& color) {
	const int dx = abs(x1-x0);
	const int dy = abs(y1-y0);
	const int sx = x0 < x1 ? 1 : -1;
	const int sy = y0 < y1 ? 1 : -1;
	int err = dx-dy;

	int max_width = getWidth();
	int max_height = getHeight();

	do {
		//skip draw if outside screen
		if (x0 > 0 && y0 > 0 && x0 < max_width && y0 < max_height) {
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
			SDL_BlitScaled(surface, NULL, scaled->surface, NULL);

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

SDLSoftwareRenderDevice::SDLSoftwareRenderDevice()
	: screen(NULL)
	, window(NULL)
	, renderer(NULL)
	, texture(NULL)
	, titlebar_icon(NULL)
	, title(NULL)
	, background_color(0) {
	Utils::logInfo("RenderDevice: Using SDLSoftwareRenderDevice (software, SDL 2, %s)", SDL_GetCurrentVideoDriver());

	fullscreen = settings->fullscreen;
	hwsurface = settings->hwsurface;
	vsync = settings->vsync;
	texture_filter = settings->texture_filter;

	min_screen.x = eset->resolutions.min_screen_w;
	min_screen.y = eset->resolutions.min_screen_h;

	SDL_DisplayMode desktop;
	if (SDL_GetDesktopDisplayMode(0, &desktop) == 0) {
		// we only support display #0
		Utils::logInfo("RenderDevice: %d display(s), using display 0 (%dx%d @ %dhz)", SDL_GetNumVideoDisplays(), desktop.w, desktop.h, desktop.refresh_rate);
	}
}

int SDLSoftwareRenderDevice::createContextInternal() {
	bool settings_changed = ((fullscreen != settings->fullscreen && destructive_fullscreen) ||
			                 hwsurface != settings->hwsurface ||
							 vsync != settings->vsync ||
							 texture_filter != settings->texture_filter ||
							 ignore_texture_filter != eset->resolutions.ignore_texture_filter);

	Uint32 w_flags = 0;
	Uint32 r_flags = 0;
	int window_w = settings->screen_w;
	int window_h = settings->screen_h;

	if (settings->fullscreen) {
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
		window_w = eset->resolutions.min_screen_w;
		window_h = eset->resolutions.min_screen_h;
	}

	w_flags = w_flags | SDL_WINDOW_RESIZABLE;

	if (settings->hwsurface) {
		r_flags = r_flags | SDL_RENDERER_ACCELERATED;
	}
	else {
		r_flags = r_flags | SDL_RENDERER_SOFTWARE;
		settings->vsync = false; // can't have software mode & vsync at the same time
	}
	if (settings->vsync) r_flags = r_flags | SDL_RENDERER_PRESENTVSYNC;

	if (settings_changed || !is_initialized) {
		destroyContext();

		window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, w_flags);
		if (window) {
			renderer = SDL_CreateRenderer(window, -1, r_flags);
			if (renderer) {
				if (settings->texture_filter && !eset->resolutions.ignore_texture_filter)
					SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
				else
					SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

				windowResize();
			}

			SDL_SetWindowMinimumSize(window, eset->resolutions.min_screen_w, eset->resolutions.min_screen_h);
			// setting minimum size might move the window, so set position again
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		if (window && renderer && screen && texture) {
			if (!is_initialized) {
				// save the system gamma levels if we just created the window
				SDL_GetWindowGammaRamp(window, gamma_r, gamma_g, gamma_b);
				Utils::logInfo("RenderDevice: Window size is %dx%d", settings->screen_w, settings->screen_h);
			}

			fullscreen = settings->fullscreen;
			hwsurface = settings->hwsurface;
			vsync = settings->vsync;
			texture_filter = settings->texture_filter;
			ignore_texture_filter = eset->resolutions.ignore_texture_filter;
			is_initialized = true;

			Utils::logInfo("RenderDevice: Fullscreen=%d, Hardware surfaces=%d, Vsync=%d, Texture Filter=%d", fullscreen, hwsurface, vsync, texture_filter);

#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_GetDisplayDPI(0, &ddpi, 0, 0);
			Utils::logInfo("RenderDevice: Display DPI is %f", ddpi);
#else
			Utils::logError("RenderDevice: The SDL version used to compile Flare does not support SDL_GetDisplayDPI(). The virtual_dpi setting will be ignored.");
#endif
		}
	}

	if (is_initialized) {
		// update minimum window size if it has changed
		if (min_screen.x != eset->resolutions.min_screen_w || min_screen.y != eset->resolutions.min_screen_h) {
			min_screen.x = eset->resolutions.min_screen_w;
			min_screen.y = eset->resolutions.min_screen_h;
			SDL_SetWindowMinimumSize(window, eset->resolutions.min_screen_w, eset->resolutions.min_screen_h);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		windowResize();

		// update title bar text and icon
		updateTitleBar();

		// load persistent resources
		delete icons;
		icons = new IconManager();
		delete curs;
		curs = new CursorManager();

		if (settings->change_gamma)
			setGamma(settings->gamma);
		else {
			resetGamma();
			settings->change_gamma = false;
			settings->gamma = 1.0;
		}
	}

	return (is_initialized ? 0 : -1);
}

void SDLSoftwareRenderDevice::createContextError() {
	Utils::logError("SDLSoftwareRenderDevice: createContext() failed: %s", SDL_GetError());
	Utils::logErrorDialog("SDLSoftwareRenderDevice: createContext() failed: %s", SDL_GetError());
}

int SDLSoftwareRenderDevice::render(Renderable& r, Rect& dest) {
	SDL_Rect src = r.src;
	SDL_Rect _dest = dest;

	SDL_Surface *surface = static_cast<SDLSoftwareImage *>(r.image)->surface;

	if (r.blend_mode == Renderable::BLEND_ADD) {
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_ADD);
	}
	else { // Renderable::BLEND_NORMAL
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	}

	SDL_SetSurfaceColorMod(surface, r.color_mod.r, r.color_mod.g, r.color_mod.b);
	SDL_SetSurfaceAlphaMod(surface, r.alpha_mod);

	return SDL_BlitSurface(surface, &src, screen, &_dest);
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

	SDL_Surface *surface = static_cast<SDLSoftwareImage *>(r->getGraphics())->surface;
	SDL_SetSurfaceColorMod(surface, r->color_mod.r, r->color_mod.g, r->color_mod.b);
	SDL_SetSurfaceAlphaMod(surface, r->alpha_mod);

	return SDL_BlitSurface(surface, &src, screen, &dest);
}

int SDLSoftwareRenderDevice::renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest) {
	if (!src_image || !dest_image) return -1;

	SDL_Rect _src = src;
	SDL_Rect _dest = dest;

	return SDL_BlitSurface(static_cast<SDLSoftwareImage *>(src_image)->surface, &_src,
						   static_cast<SDLSoftwareImage *>(dest_image)->surface, &_dest);
}

Image* SDLSoftwareRenderDevice::renderTextToImage(FontStyle* font_style, const std::string& text, const Color& color, bool blended) {
	SDLSoftwareImage *image = new SDLSoftwareImage(this);
	if (!image) return NULL;

	SDL_Color _color = color;

	if (blended)
		image->surface = TTF_RenderUTF8_Blended(static_cast<SDLFontStyle *>(font_style)->ttfont, text.c_str(), _color);
	else
		image->surface = TTF_RenderUTF8_Solid(static_cast<SDLFontStyle *>(font_style)->ttfont, text.c_str(), _color);

	if (image->surface)
		return image;

	delete image;
	return NULL;
}

void SDLSoftwareRenderDevice::drawPixel(int x, int y, const Color& color) {
	Uint32 pixel = MapRGBA(color.r, color.g, color.b, color.a);

	int bpp = screen->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = static_cast<Uint8*>(screen->pixels) + y * screen->pitch + x * bpp;

	if (SDL_MUSTLOCK(screen)) {
		SDL_LockSurface(screen);
	}
	switch(bpp) {
		case 1:
			*p = static_cast<Uint8>(pixel);
			break;

		case 2:
			*(reinterpret_cast<Uint16*>(p)) = static_cast<Uint16>(pixel);
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
			*(reinterpret_cast<Uint32*>(p)) = pixel;
			break;
	}
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}

	return;
}

void SDLSoftwareRenderDevice::drawLine(int x0, int y0, int x1, int y1, const Color& color) {
	const int dx = abs(x1-x0);
	const int dy = abs(y1-y0);
	const int sx = x0 < x1 ? 1 : -1;
	const int sy = y0 < y1 ? 1 : -1;
	int err = dx-dy;

	do {
		//skip draw if outside screen
		if (x0 > 0 && y0 > 0 && x0 < settings->view_w && y0 < settings->view_h) {
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

void SDLSoftwareRenderDevice::drawRectangle(const Point& p0, const Point& p1, const Color& color) {
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
	SDL_FillRect(screen, NULL, background_color);
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
	resetGamma();

	// we need to free all loaded graphics as they may be tied to the current context
	RenderDevice::cacheRemoveAll();
	reload_graphics = true;

	if (icons) {
		delete icons;
		icons = NULL;
	}
	if (curs) {
		delete curs;
		curs = NULL;
	}
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
		Utils::logError("SDLSoftwareRenderDevice: CreateRGBSurface failed: %s", SDL_GetError());
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

void SDLSoftwareRenderDevice::resetGamma() {
	SDL_SetWindowGammaRamp(window, gamma_r, gamma_g, gamma_b);
}

void SDLSoftwareRenderDevice::updateTitleBar() {
	if (title) free(title);
	if (titlebar_icon) SDL_FreeSurface(titlebar_icon);

	if (!window) return;

	title = Utils::strdup(msg->get(eset->misc.window_title));
	titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());

	if (title) SDL_SetWindowTitle(window, title);
	if (titlebar_icon) SDL_SetWindowIcon(window, titlebar_icon);
}

Image *SDLSoftwareRenderDevice::loadImage(const std::string& filename, int error_type) {
	// lookup image in cache
	Image *img;
	img = cacheLookup(filename);
	if (img != NULL) return img;

	// load image
	SDLSoftwareImage *image;
	image = NULL;
	SDL_Surface *cleanup = IMG_Load(mods->locate(filename).c_str());
	if(!cleanup) {
		if (error_type != ERROR_NONE)
			Utils::logError("SDLSoftwareRenderDevice: Couldn't load image: '%s'. %s", filename.c_str(), IMG_GetError());

		if (error_type == ERROR_EXIT) {
			Utils::logErrorDialog("SDLSoftwareRenderDevice: Couldn't load image: '%s'.\n%s", filename.c_str(), IMG_GetError());
			mods->resetModConfig();
			Utils::Exit(1);
		}

		return NULL;
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

void SDLSoftwareRenderDevice::getWindowSize(short unsigned *screen_w, short unsigned *screen_h) {
	int w,h;
	SDL_GetWindowSize(window, &w, &h);
	*screen_w = static_cast<unsigned short>(w);
	*screen_h = static_cast<unsigned short>(h);
}

void SDLSoftwareRenderDevice::windowResize() {
	windowResizeInternal();

	SDL_RenderSetLogicalSize(renderer, settings->view_w, settings->view_h);

	if (texture) SDL_DestroyTexture(texture);
	if (screen) SDL_FreeSurface(screen);

	Uint32 rmask, gmask, bmask, amask;
	int bpp = static_cast<int>(BITS_PER_PIXEL);
	SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_ARGB8888, &bpp, &rmask, &gmask, &bmask, &amask);
	screen = SDL_CreateRGBSurface(0, settings->view_w, settings->view_h, bpp, rmask, gmask, bmask, amask);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, settings->view_w, settings->view_h);

	settings->updateScreenVars();
}

void SDLSoftwareRenderDevice::setBackgroundColor(Color color) {
	background_color = SDL_MapRGBA(screen->format, color.r, color.g, color.b, color.a);
}

void SDLSoftwareRenderDevice::setFullscreen(bool enable_fullscreen) {
	if (!destructive_fullscreen) {
		if (enable_fullscreen) {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else {
			SDL_SetWindowFullscreen(window, 0);
		}
		windowResize();
	}
}
