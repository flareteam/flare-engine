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

#pragma once
#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include <SDL_ttf.h>
#include "Utils.h"


/** Provide abstract interface for FLARE engine rendering devices.
 *
 * Provide an abstract interface for renderning a Renderable to the screen.
 * Each rendering device implementation must fully implement the interface.
 * The idea is that the render() method replicates the behaviour of
 * SDL_BlitSurface() with different rendering backends, but bases on the
 * information carried in a Renderable struct.
 *
 * As this is for the FLARE engine, implementations use the the engine's global
 * settings context.
 *
 * @class RenderDevice
 * @author Kurt Rinnert
 * @date 2013-07-06
 *
 */

class Renderable;
class Sprite;

#define Image SDL_Surface

class RenderDevice {

public:

	/** Constuctor.
	 */
	RenderDevice()
		: is_initialized(false) {
	}

	/** Destructor.
	 */
	virtual ~RenderDevice() {
	}

	/** Create context on startup.
	 */
	virtual int createContext(
		int width,
		int height
	) = 0;

	virtual SDL_Rect getContextSize(
	) = 0;

	/** Render a Renderable to the screen.
	 */
	virtual int render(Sprite& r) = 0;
	virtual int render(Renderable& r, SDL_Rect dest) = 0;
	virtual int renderImage(Image* image, SDL_Rect& src) = 0;
	virtual int renderToImage(Image* src_image, SDL_Rect& src, Image* dest_image, SDL_Rect& dest, bool dest_is_transparent = false) = 0;

	/** Render text to the screen.
	 */
	virtual int renderText(
		TTF_Font *ttf_font,
		const std::string& text,
		SDL_Color color,
		SDL_Rect& dest
	) = 0;

	/** Draw pixel to screen.
	 */
	virtual void drawPixel(
		int x,
		int y,
		Uint32 color
	) = 0;

	/** Draw line to screen.
	 *
	 *  Draw line connecting (x0,y0) and (x1,y1) to screen.
	 */
	virtual void drawLine(
		int x0,
		int y0,
		int x1,
		int y1,
		Uint32 color
	) = 0;

	/** Draw line to screen.
	 *
	 *  Draw line connecting p0 and p1 to screen.
	 */
	virtual void drawLine(
		const Point& p0,
		const Point& p1,
		Uint32 color
	) = 0;

	/** Draw rectangle to screen.
	 *
	 *  Draw rectangle defined by p0 and p1 to screen.
	 */
	virtual void drawRectangle(
		const Point& p0,
		const Point& p1,
		Uint32 color
	) = 0;

	/** Blank the screen.
	 */
	virtual void blankScreen() = 0;

	/** Commit the next frame to the display.
	 */
	virtual void commitFrame() = 0;

	/** Destroy context on exit.
	 */
	virtual void destroyContext() = 0;

	/**
	 * Map a RGB color value to a pixel format.
	 */
	virtual Uint32 MapRGB(SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b) = 0;

	/**
	 * Map a RGB color value to a screen pixel format.
	 */
	virtual Uint32 MapRGB(Uint8 r, Uint8 g, Uint8 b) = 0;

	/**
	 * Map a RGBA color value to a pixel format.
	 */
	virtual Uint32 MapRGBA(SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;

	/**
	 * Map a RGBA color value to a screen pixel format.
	 */
	virtual Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;

protected:

	bool is_initialized; ///< true if a context was created once
};

void loadIcons();

#endif // RENDERDEVICE_H
