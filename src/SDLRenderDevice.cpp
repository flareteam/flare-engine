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

#include "SharedResources.h"

#include "SDLRenderDevice.h"

using namespace std;

SDLRenderDevice::SDLRenderDevice() {
	cout << "Using Render Device: SDLRenderDevice" << endl;
}

void SDLRenderDevice::createContext(int width, int height) {

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
}

int SDLRenderDevice::render(Renderable& r) {
	// Drawing order is recalculated every frame.
	r.prio = 0;

	if (r.sprite == NULL) {
		return -1;
	}
	if ( !local_to_global(r) ) {
		return -1;
	}

	return SDL_BlitSurface(r.sprite, &m_clip, screen, &m_dest);
}

int SDLRenderDevice::renderText(
	TTF_Font *ttf_font,
	const std::string& text,
	SDL_Color color,
	SDL_Rect& dest
) {
	int ret = 0;
	m_ttf_renderable.sprite = TTF_RenderUTF8_Blended(ttf_font, text.c_str(), color);
	if (m_ttf_renderable.sprite != NULL) {
		m_ttf_renderable.src.x = 0;
		m_ttf_renderable.src.y = 0;
		m_ttf_renderable.src.w = m_ttf_renderable.sprite->w;
		m_ttf_renderable.src.h = m_ttf_renderable.sprite->h;
		ret = SDL_BlitSurface(
				  m_ttf_renderable.sprite,
				  &m_ttf_renderable.src,
				  screen,
				  &dest
			  );
		SDL_FreeSurface(m_ttf_renderable.sprite);
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

bool SDLRenderDevice::local_to_global(Renderable& r) {
	m_clip = r.src;

	int left = r.map_pos.x - r.offset.x;
	int right = left + r.src.w;
	int up = r.map_pos.y - r.offset.y;
	int down = up + r.src.h;

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
			m_clip.x = r.src.x - left;
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
			m_clip.y = r.src.y - up;
			up = 0;
		};
		down = (down < r.local_frame.h ? down : r.local_frame.h);
		m_clip.h = down - up;
	}

	m_dest.x = left + r.local_frame.x;
	m_dest.y = up + r.local_frame.y;

	return true;
}

