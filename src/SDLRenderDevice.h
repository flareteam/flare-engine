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
#ifndef SDLRENDERDEVICE_H
#define SDLRENDERDEVICE_H

#include "RenderDevice.h"

/** Provide rendering device using SDL_BlitSurface backend.
 *
 * Provide an SDL_BlitSurface implementation for renderning a Renderable to
 * the screen.  Simply dispatches rendering to SDL_BlitSurface().
 *
 * As this is for the FLARE engine, the implementation uses the engine's
 * global settings context, which is included by the interface.
 *
 * @class SDLBlitRenderDevice
 * @see RenderDevice
 * @author Kurt Rinnert
 * @date 2013-07-06
 *
 */
class SDLRenderDevice : public RenderDevice {

public:

	/** Initialize base class and report rendering device in use.
	 */
	SDLRenderDevice();

	/** Create context on startup.
	 */
	virtual SDL_Surface *create_context(
		int width,
		int height,
		bool full_screen
	);

	/** Render surface to screen.
	 */
	virtual int render(Renderable& r);

	/** Render text to the screen.
	 */
	virtual int render_text(
		TTF_Font *ttf_font,
		const std::string& text,
		SDL_Color color,
		SDL_Rect& dest
	);

	/** Draw pixel to screen.
	 */
	virtual void draw_pixel(
		int x,
		int y,
		Uint32 color
	);

	/** Draw line to screen.
	 *
	 *  Draw line connecting (x0,y0) and (x1,y1) to screen.
	 */
	virtual void draw_line(
		int x0,
		int y0,
		int x1,
		int y1,
		Uint32 color
	);

	/** Draw line to screen.
	 *
	 *  Draw line connecting p0 and p1 to screen.
	 */
	virtual void draw_line(
		const Point& p0,
		const Point& p1,
		Uint32 color
	);

	/** Draw rectangle to screen.
	 *
	 *  Draw rectangle defined by p0 and p1 to screen.
	 */
	virtual void draw_rectangle(
		const Point& p0,
		const Point& p1,
		Uint32 color
	);

	/** Blank the screen.
	 */
	virtual void blank_screen();

	/** Commit the next frame to dispay.
	 */
	virtual void commit_frame();

	/** Destroy context on exit.
	 */
	virtual void destroy_context();

private:

	// Compute clipping and global position from local frame.
	bool local_to_global(Renderable& r);

private:

	// These are for keeping the render stack frame small.
	SDL_Rect m_clip;
	SDL_Rect m_dest;
	Renderable m_ttf_renderable;
};

#endif // SDLRENDERDEVICE_H
