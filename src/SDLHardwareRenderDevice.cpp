/*
Copyright © 2013 Igor Paliychuk
Copyright © 2013-2014 Justin Jacobs

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

#include "SharedResources.h"
#include "Settings.h"

#include "SDLHardwareRenderDevice.h"
#include "SDLFontEngine.h"

SDLHardwareImage::SDLHardwareImage(RenderDevice *_device, SDL_Renderer *_renderer)
	: Image(_device)
	, renderer(_renderer)
	, surface(NULL) {
}

SDLHardwareImage::~SDLHardwareImage() {
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

void SDLHardwareImage::fillWithColor(Uint32 color) {
	if (!surface) return;

	Uint32 u_format;
	SDL_QueryTexture(surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(color, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderTarget(renderer, surface);
	SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g , rgba.b, rgba.a);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, NULL);
}

/*
 * Set the pixel at (x, y) to the given value
 */
void SDLHardwareImage::drawPixel(int x, int y, Uint32 pixel) {
	if (!surface) return;

	Uint32 u_format;
	SDL_QueryTexture(surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(pixel, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderTarget(renderer, surface);
	SDL_SetTextureBlendMode(surface, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
	SDL_RenderDrawPoint(renderer, x, y);
	SDL_SetRenderTarget(renderer, NULL);
}

Uint32 SDLHardwareImage::MapRGB(Uint8 r, Uint8 g, Uint8 b) {
	if (!surface) return 0;

	Uint32 u_format;
	SDL_QueryTexture(surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGB(format, r, g, b);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
}

Uint32 SDLHardwareImage::MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	if (!surface) return 0;

	Uint32 u_format;
	SDL_QueryTexture(surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGBA(format, r, g, b, a);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
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
{
	logInfo("Using Render Device: SDLHardwareRenderDevice (hardware, SDL 2)");

	fullscreen = FULLSCREEN;
	hwsurface = HWSURFACE;
	vsync = VSYNC;
	texture_filter = TEXTURE_FILTER;

	min_screen.x = MIN_SCREEN_W;
	min_screen.y = MIN_SCREEN_H;
}

int SDLHardwareRenderDevice::createContext() {
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
		w_flags = w_flags | SDL_WINDOW_SHOWN;
	}
	else {
		w_flags = w_flags | SDL_WINDOW_SHOWN;
	}

	w_flags = w_flags | SDL_WINDOW_RESIZABLE;

	if (HWSURFACE) {
		r_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
	}
	else {
		r_flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;
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

		bool window_created = window != NULL && renderer != NULL;

		if (!window_created && !is_initialized) {
			// If this is the first attempt and it failed we are not
			// getting anywhere.
			logError("SDLHardwareRenderDevice: createContext() failed: %s", SDL_GetError());
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

int SDLHardwareRenderDevice::render(Renderable& r, Rect dest) {
	dest.w = r.src.w;
	dest.h = r.src.h;
    SDL_Rect src = r.src;
    SDL_Rect _dest = dest;
	SDL_SetRenderTarget(renderer, texture);
	return SDL_RenderCopy(renderer, static_cast<SDLHardwareImage *>(r.image)->surface, &src, &_dest);
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

int SDLHardwareRenderDevice::renderText(
	FontStyle *font_style,
	const std::string& text,
	Color color,
	Rect& dest
) {
	int ret = 0;
	SDL_Texture *surface = NULL;

	SDL_Surface *cleanup = TTF_RenderUTF8_Blended(static_cast<SDLFontStyle *>(font_style)->ttfont, text.c_str(), color);
	if (cleanup) {
		surface = SDL_CreateTextureFromSurface(renderer,cleanup);
		SDL_FreeSurface(cleanup);
	}

	if (surface == NULL)
		return -1;

	SDL_Rect clip;
	int w, h;
	SDL_QueryTexture(surface, NULL, NULL, &w, &h);

	clip.x = clip.y = 0;
	clip.w = w;
	clip.h = h;

	dest.w = clip.w;
	dest.h = clip.h;
	SDL_Rect _dest = dest;

	SDL_SetRenderTarget(renderer, texture);
	ret = SDL_RenderCopy(renderer, surface, &clip, &_dest);

	SDL_DestroyTexture(surface);

	return ret;
}

Image * SDLHardwareRenderDevice::renderTextToImage(FontStyle* font_style, const std::string& text, Color color, bool blended) {
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

void SDLHardwareRenderDevice::drawPixel(
	int x,
	int y,
	Uint32 color
) {
	Uint32 u_format = SDL_GetWindowPixelFormat(window);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(color, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
	SDL_RenderDrawPoint(renderer, x, y);
}

void SDLHardwareRenderDevice::drawLine(
	int x0,
	int y0,
	int x1,
	int y1,
	Uint32 color
) {
	Uint32 u_format = SDL_GetWindowPixelFormat(window);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(color, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
	SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
}

void SDLHardwareRenderDevice::drawRectangle(
	const Point& p0,
	const Point& p1,
	Uint32 color
) {
	drawLine(p0.x, p0.y, p1.x, p0.y, color);
	drawLine(p1.x, p0.y, p1.x, p1.y, color);
	drawLine(p0.x, p0.y, p0.x, p1.y, color);
	drawLine(p0.x, p1.y, p1.x, p1.y, color);
}

void SDLHardwareRenderDevice::blankScreen() {
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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
	SDL_FreeSurface(titlebar_icon);
	titlebar_icon = NULL;

	SDL_DestroyRenderer(renderer);
	renderer = NULL;

	SDL_DestroyWindow(window);
	window = NULL;

	SDL_DestroyTexture(texture);
	texture = NULL;

	if (title) {
		free(title);
		title = NULL;
	}

	return;
}

Uint32 SDLHardwareRenderDevice::MapRGB(Uint8 r, Uint8 g, Uint8 b) {
	Uint32 u_format = SDL_GetWindowPixelFormat(window);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGB(format, r, g, b);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
}

Uint32 SDLHardwareRenderDevice::MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	Uint32 u_format = SDL_GetWindowPixelFormat(window);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGBA(format, r, g, b, a);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
}

/**
 * create blank surface
 */
Image *SDLHardwareRenderDevice::createImage(int width, int height) {

	SDLHardwareImage *image = new SDLHardwareImage(this, renderer);

	if (width > 0 && height > 0) {
		image->surface = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);
		if(image->surface == NULL) {
			logError("SDLHardwareRenderDevice: SDL_CreateTexture failed: %s", SDL_GetError());
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

void SDLHardwareRenderDevice::updateTitleBar() {
	if (title) free(title);
	title = NULL;
	if (titlebar_icon) SDL_FreeSurface(titlebar_icon);
	titlebar_icon = NULL;

	if (!window) return;

	title = strdup(msg->get(WINDOW_TITLE).c_str());
	titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());

	if (title) SDL_SetWindowTitle(window, title);
	if (titlebar_icon) SDL_SetWindowIcon(window, titlebar_icon);
}

Image *SDLHardwareRenderDevice::loadImage(std::string filename, std::string errormessage, bool IfNotFoundExit) {
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
		if (!errormessage.empty())
			logError("SDLHardwareRenderDevice: %s: %s", errormessage.c_str(), IMG_GetError());
		if (IfNotFoundExit) {
			SDL_Quit();
			exit(1);
		}
		return NULL;
	}

	// store image to cache
	cacheStore(filename, image);
	return image;
}

void SDLHardwareRenderDevice::freeImage(Image *image) {
	if (!image) return;

	cacheRemove(image);

	if (static_cast<SDLHardwareImage *>(image)->surface) {
		SDL_DestroyTexture(static_cast<SDLHardwareImage *>(image)->surface);
	}
}

void SDLHardwareRenderDevice::windowResize() {
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
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, VIEW_W, VIEW_H);
	SDL_SetRenderTarget(renderer, texture);
}

