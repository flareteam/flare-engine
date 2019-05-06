/*
Copyright © 2013 Igor Paliychuk
Copyright © 2013-2014 Justin Jacobs
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

#include "SDLHardwareRenderDevice.h"
#include "SDLFontEngine.h"

SDLHardwareImage::SDLHardwareImage(RenderDevice *_device, SDL_Renderer *_renderer)
	: Image(_device)
	, renderer(_renderer)
	, surface(NULL)
	, pixel_batch_surface(NULL) {
}

SDLHardwareImage::~SDLHardwareImage() {
	if (surface)
		SDL_DestroyTexture(surface);
	if (pixel_batch_surface)
		SDL_FreeSurface(pixel_batch_surface);
}

int SDLHardwareImage::getWidth() const {
	int w, h;
	SDL_QueryTexture(surface, NULL, NULL, &w, &h);
	return (surface ? w : 0);
}

int SDLHardwareImage::getHeight() const {
	int w, h;
	SDL_QueryTexture(surface, NULL, NULL, &w, &h);
	return (surface ? h : 0);
}

void SDLHardwareImage::fillWithColor(const Color& color) {
	if (!surface) return;

	SDL_SetRenderTarget(renderer, surface);
	SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g , color.b, color.a);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, NULL);
}

/*
 * Set the pixel at (x, y) to the given value
 */
void SDLHardwareImage::drawPixel(int x, int y, const Color& color) {
	if (!surface) return;

	if (pixel_batch_surface) {
		// Taken from SDLSoftwareImage::drawPixel()
		Uint32 pixel = SDL_MapRGBA(pixel_batch_surface->format, color.r, color.g, color.b, color.a);

		int bpp = pixel_batch_surface->format->BytesPerPixel;
		/* Here p is the address to the pixel we want to set */
		Uint8 *p = static_cast<Uint8*>(pixel_batch_surface->pixels) + y * pixel_batch_surface->pitch + x * bpp;

		if (SDL_MUSTLOCK(pixel_batch_surface)) {
			SDL_LockSurface(pixel_batch_surface);
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
		if (SDL_MUSTLOCK(pixel_batch_surface)) {
			SDL_UnlockSurface(pixel_batch_surface);
		}
	}
	else {
		SDL_SetRenderTarget(renderer, surface);
		SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderDrawPoint(renderer, x, y);
		SDL_SetRenderTarget(renderer, NULL);
	}
}

void SDLHardwareImage::drawLine(int x0, int y0, int x1, int y1, const Color& color) {
	SDL_SetRenderTarget(renderer, surface);
	SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
	SDL_SetRenderTarget(renderer, NULL);
}


/**
 * Creates a non-accelerated SDL_Surface as a pixel buffer
 * The buffer is drawn when SDLHardwareImag::endPixelBatch() is called
 * This way, the number of video card draw calls is vastly reduced, especially when we're drawing lots of pixels (aka the minimap)
 *
 * SDL_RenderDrawPoints() is supposed to be the "right" way to do this, but SDL_Surface provides a nice structure so we don't have to create the points array ourselves
 * Performance-wise, it's probably the same, since it's a bunch of system-memory ops followed by one draw call
 */
void SDLHardwareImage::beginPixelBatch() {
	if (!surface) return;

	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	if (pixel_batch_surface)
		SDL_FreeSurface(pixel_batch_surface);

	pixel_batch_surface = SDL_CreateRGBSurface(0, getWidth(), getHeight(), device->BITS_PER_PIXEL, rmask, gmask, bmask, amask);
}

void SDLHardwareImage::endPixelBatch() {
	if (!surface || !pixel_batch_surface) return;

	SDL_Texture *pixel_batch_texture = SDL_CreateTextureFromSurface(renderer, pixel_batch_surface);

	if (pixel_batch_texture) {
		SDL_SetRenderTarget(renderer, surface);
		SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_BLEND);
		SDL_RenderCopy(renderer, pixel_batch_texture, NULL, NULL);
		SDL_SetRenderTarget(renderer, NULL);

		SDL_DestroyTexture(pixel_batch_texture);
	}

	SDL_FreeSurface(pixel_batch_surface);
	pixel_batch_surface = NULL;
}

Image* SDLHardwareImage::resize(int width, int height) {
	if(!surface || width <= 0 || height <= 0)
		return NULL;

	SDLHardwareImage *scaled = new SDLHardwareImage(device, renderer);
	if (!scaled) return NULL;

	scaled->surface = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);

	if (scaled->surface != NULL) {
		// copy the source texture to the new texture, stretching it in the process
		SDL_SetRenderTarget(renderer, scaled->surface);
		SDL_RenderCopyEx(renderer, surface, NULL, NULL, 0, NULL, SDL_FLIP_NONE);
		SDL_SetRenderTarget(renderer, NULL);

		// Remove the old surface
		this->unref();
		return scaled;
	}
	else {
		delete scaled;
	}

	return NULL;
}

SDLHardwareRenderDevice::SDLHardwareRenderDevice()
	: window(NULL)
	, renderer(NULL)
	, texture(NULL)
	, titlebar_icon(NULL)
	, title(NULL)
	, background_color(0,0,0,0)
{
	Utils::logInfo("Using Render Device: SDLHardwareRenderDevice (hardware, SDL 2, %s)", SDL_GetCurrentVideoDriver());

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

int SDLHardwareRenderDevice::createContextInternal() {
#ifdef _WIN32
	// We make heavy use of SDL_TEXTUREACCESS_TARGET for things such as text and the minimap
	// If we use the 'direct3d' backend on Windows, these textures get lost on window resizing events
	// So to bypass this, we force 'opengl' on Windows
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif

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
		w_flags = w_flags | SDL_WINDOW_SHOWN;
	}
	else {
		w_flags = w_flags | SDL_WINDOW_SHOWN;
	}

	w_flags = w_flags | SDL_WINDOW_RESIZABLE;

	if (settings->hwsurface) {
		r_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
	}
	else {
		r_flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;
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

		if (window && renderer) {
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

void SDLHardwareRenderDevice::createContextError() {
	Utils::logError("SDLHardwareRenderDevice: createContext() failed: %s", SDL_GetError());
	Utils::logErrorDialog("SDLHardwareRenderDevice: createContext() failed: %s", SDL_GetError());
}

int SDLHardwareRenderDevice::render(Renderable& r, Rect& dest) {
	dest.w = r.src.w;
	dest.h = r.src.h;
    SDL_Rect src = r.src;
    SDL_Rect _dest = dest;
	SDL_SetRenderTarget(renderer, texture);

	SDL_Texture *surface = static_cast<SDLHardwareImage *>(r.image)->surface;

	if (r.blend_mode == Renderable::BLEND_ADD) {
		SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_ADD);
	}
	else { // Renderable::BLEND_NORMAL
		SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_BLEND);
	}

	SDL_SetTextureColorMod(surface, r.color_mod.r, r.color_mod.g, r.color_mod.b);
	SDL_SetTextureAlphaMod(surface, r.alpha_mod);

	return SDL_RenderCopy(renderer, surface, &src, &_dest);
}

int SDLHardwareRenderDevice::render(Sprite *r) {
	if (r == NULL) {
		return -1;
	}
	if ( !localToGlobal(r) ) {
		return -1;
	}

	// negative x and y clip causes weird stretching
	// adjust for that here
	if (m_clip.x < 0) {
		m_clip.w -= abs(m_clip.x);
		m_dest.x += abs(m_clip.x);
		m_clip.x = 0;
	}
	if (m_clip.y < 0) {
		m_clip.h -= abs(m_clip.y);
		m_dest.y += abs(m_clip.y);
		m_clip.y = 0;
	}

	m_dest.w = m_clip.w;
	m_dest.h = m_clip.h;

    SDL_Rect src = m_clip;
    SDL_Rect dest = m_dest;
	SDL_SetRenderTarget(renderer, texture);

	SDL_Texture *surface = static_cast<SDLHardwareImage *>(r->getGraphics())->surface;
	SDL_SetTextureColorMod(surface, r->color_mod.r, r->color_mod.g, r->color_mod.b);
	SDL_SetTextureAlphaMod(surface, r->alpha_mod);

	return SDL_RenderCopy(renderer, static_cast<SDLHardwareImage *>(r->getGraphics())->surface, &src, &dest);
}

int SDLHardwareRenderDevice::renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest) {
	if (!src_image || !dest_image)
		return -1;

	if (SDL_SetRenderTarget(renderer, static_cast<SDLHardwareImage *>(dest_image)->surface) != 0)
		return -1;

	dest.w = src.w;
	dest.h = src.h;
    SDL_Rect _src = src;
    SDL_Rect _dest = dest;

	SDL_SetTextureBlendMode(static_cast<SDLHardwareImage *>(dest_image)->surface, SDL_BLENDMODE_BLEND);
	SDL_RenderCopy(renderer, static_cast<SDLHardwareImage *>(src_image)->surface, &_src, &_dest);
	SDL_SetRenderTarget(renderer, NULL);
	return 0;
}

Image * SDLHardwareRenderDevice::renderTextToImage(FontStyle* font_style, const std::string& text, const Color& color, bool blended) {
	SDLHardwareImage *image = new SDLHardwareImage(this, renderer);

	SDL_Surface *cleanup;

	if (blended) {
		cleanup = TTF_RenderUTF8_Blended(static_cast<SDLFontStyle *>(font_style)->ttfont, text.c_str(), color);
	}
	else {
		cleanup = TTF_RenderUTF8_Solid(static_cast<SDLFontStyle *>(font_style)->ttfont, text.c_str(), color);
	}

	if (cleanup) {
		image->surface = SDL_CreateTextureFromSurface(renderer, cleanup);
		SDL_FreeSurface(cleanup);
		return image;
	}

	delete image;
	return NULL;
}

void SDLHardwareRenderDevice::drawPixel(int x, int y, const Color& color) {
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawPoint(renderer, x, y);
}

void SDLHardwareRenderDevice::drawLine(int x0, int y0, int x1, int y1, const Color& color) {
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
}

void SDLHardwareRenderDevice::drawRectangle(const Point& p0, const Point& p1, const Color& color) {
	SDL_Rect r;
	r.x = p0.x;
	r.y = p0.y;
	r.w = p1.x - p0.x;
	r.h = p1.y - p0.y;
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawRect(renderer, &r);
}

void SDLHardwareRenderDevice::blankScreen() {
	SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, background_color.a);
	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, texture);
	SDL_RenderClear(renderer);
	return;
}

void SDLHardwareRenderDevice::commitFrame() {
	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	inpt->window_resized = false;

	return;
}

void SDLHardwareRenderDevice::destroyContext() {
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

	SDL_FreeSurface(titlebar_icon);
	titlebar_icon = NULL;

	SDL_DestroyTexture(texture);
	texture = NULL;

	SDL_DestroyRenderer(renderer);
	renderer = NULL;

	SDL_DestroyWindow(window);
	window = NULL;

	if (title) {
		free(title);
		title = NULL;
	}

	return;
}

/**
 * create blank surface
 */
Image *SDLHardwareRenderDevice::createImage(int width, int height) {

	SDLHardwareImage *image = new SDLHardwareImage(this, renderer);

	if (width > 0 && height > 0) {
		image->surface = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);
		if(image->surface == NULL) {
			Utils::logError("SDLHardwareRenderDevice: SDL_CreateTexture failed: %s", SDL_GetError());
		}
		else {
				SDL_SetRenderTarget(renderer, image->surface);
				SDL_SetTextureBlendMode(image->surface, SDL_BLENDMODE_BLEND);
				SDL_SetRenderDrawColor(renderer, 0,0,0,0);
				SDL_RenderClear(renderer);
				SDL_SetRenderTarget(renderer, NULL);
		}
	}

	return image;
}

void SDLHardwareRenderDevice::setGamma(float g) {
	Uint16 ramp[256];
	SDL_CalculateGammaRamp(g, ramp);
	SDL_SetWindowGammaRamp(window, ramp, ramp, ramp);
}

void SDLHardwareRenderDevice::resetGamma() {
	SDL_SetWindowGammaRamp(window, gamma_r, gamma_g, gamma_b);
}

void SDLHardwareRenderDevice::updateTitleBar() {
	if (title) free(title);
	title = NULL;
	if (titlebar_icon) SDL_FreeSurface(titlebar_icon);
	titlebar_icon = NULL;

	if (!window) return;

	title = Utils::strdup(msg->get(eset->misc.window_title));
	titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());

	if (title) SDL_SetWindowTitle(window, title);
	if (titlebar_icon) SDL_SetWindowIcon(window, titlebar_icon);
}

Image *SDLHardwareRenderDevice::loadImage(const std::string& filename, int error_type) {
	// lookup image in cache
	Image *img;
	img = cacheLookup(filename);
	if (img != NULL) return img;

	// load image
	SDLHardwareImage *image = new SDLHardwareImage(this, renderer);
	if (!image) return NULL;

	image->surface = IMG_LoadTexture(renderer, mods->locate(filename).c_str());

	if(image->surface == NULL) {
		delete image;
		if (error_type != ERROR_NONE)
			Utils::logError("SDLHardwareRenderDevice: Couldn't load image: '%s'. %s", filename.c_str(), IMG_GetError());

		if (error_type == ERROR_EXIT) {
			Utils::logErrorDialog("SDLHardwareRenderDevice: Couldn't load image: '%s'.\n%s", filename.c_str(), IMG_GetError());
			mods->resetModConfig();
			Utils::Exit(1);
		}

		return NULL;
	}

	// store image to cache
	cacheStore(filename, image);
	return image;
}

void SDLHardwareRenderDevice::getWindowSize(short unsigned *screen_w, short unsigned *screen_h) {
	int w,h;
	SDL_GetWindowSize(window, &w, &h);
	*screen_w = static_cast<unsigned short>(w);
	*screen_h = static_cast<unsigned short>(h);
}

void SDLHardwareRenderDevice::windowResize() {
	windowResizeInternal();

	SDL_RenderSetLogicalSize(renderer, settings->view_w, settings->view_h);

	if (texture) SDL_DestroyTexture(texture);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, settings->view_w, settings->view_h);
	SDL_SetRenderTarget(renderer, texture);

	settings->updateScreenVars();
}

void SDLHardwareRenderDevice::setBackgroundColor(Color color) {
	background_color = color;
}

void SDLHardwareRenderDevice::setFullscreen(bool enable_fullscreen) {
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
