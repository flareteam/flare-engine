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
 * @class SDLRenderDevice
 * @see RenderDevice
 * @author Kurt Rinnert
 * @date 2013-07-06
 *
 */

// message passing struct for various sprites rendered map inline
class Renderable {
public:
	SDL_Surface *sprite; // image to be used
	SDL_Rect src; // location on the sprite in pixel coordinates.

	FPoint map_pos;     // The map location on the floor between someone's feet
	Point offset;      // offset from map_pos to topleft corner of sprite
	uint64_t prio;     // 64-32 bit for map position, 31-16 for intertile position, 15-0 user dependent, such as Avatar.
	Renderable()
		: sprite(0)
		, src(SDL_Rect())
		, map_pos()
		, offset()
		, prio(0)
	{}
};

class Sprite {

public:
	Sprite();
	Sprite(const Sprite& other);
	Sprite& operator=(const Sprite& other);
	~Sprite();

	FPoint map_pos;     // The map location on the floor between someone's feet
	SDL_Rect local_frame;
	uint64_t prio;     // 64-32 bit for map position, 31-16 for intertile position, 15-0 user dependent, such as Avatar.
	bool keep_graphics; // don't free the sprite surface when deconstructing, used primarily for animations

	void setGraphics(SDL_Surface *s, bool setClipToFull = true);
	SDL_Surface * getGraphics();
	bool graphicsIsNull();
	void clearGraphics();
	void setOffset(const Point& _offset);
	void setOffset(const int x, const int y);
	Point getOffset();
	void setClip(const SDL_Rect& clip);
	void setClip(const int x, const int y, const int w, const int h);
	void setClipX(const int x);
	void setClipY(const int y);
	void setClipW(const int w);
	void setClipH(const int h);
	SDL_Rect getClip();
	void setDest(const SDL_Rect& dest);
	void setDest(const Point& dest);
	void setDest(int x, int y);
	FPoint getDest();
	int getGraphicsWidth();
	int getGraphicsHeight();

private:
	SDL_Surface *sprite; // image to be used
	SDL_Rect src; // location on the sprite in pixel coordinates.
	Point offset;      // offset from map_pos to topleft corner of sprite
};

class SDLRenderDevice : public RenderDevice {

public:

	/** Initialize base class and report rendering device in use.
	 */
	SDLRenderDevice();

	/** Create context on startup.
	 */
	virtual void createContext(
		int width,
		int height
	);

	/** Render surface to screen.
	 */
	virtual int render(Renderable& r, SDL_Rect dest);
	virtual int render(Sprite& r);

	/** Render text to the screen.
	 */
	virtual int renderText(
		TTF_Font *ttf_font,
		const std::string& text,
		SDL_Color color,
		SDL_Rect& dest
	);

	/** Draw pixel to screen.
	 */
	virtual void drawPixel(
		int x,
		int y,
		Uint32 color
	);

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
	);

	/** Draw line to screen.
	 *
	 *  Draw line connecting p0 and p1 to screen.
	 */
	virtual void drawLine(
		const Point& p0,
		const Point& p1,
		Uint32 color
	);

	/** Draw rectangle to screen.
	 *
	 *  Draw rectangle defined by p0 and p1 to screen.
	 */
	virtual void drawRectangle(
		const Point& p0,
		const Point& p1,
		Uint32 color
	);

	/** Blank the screen.
	 */
	virtual void blankScreen();

	/** Commit the next frame to dispay.
	 */
	virtual void commitFrame();

	/** Destroy context on exit.
	 */
	virtual void destroyContext();

private:

	// Compute clipping and global position from local frame.
	bool local_to_global(Sprite& r);

private:

	// These are for keeping the render stack frame small.
	SDL_Rect m_clip;
	SDL_Rect m_dest;
	Sprite m_ttf_renderable;
};

Uint32 readPixel(SDL_Surface *screen, int x, int y);
void drawPixel(SDL_Surface *screen, int x, int y, Uint32 color);
void drawLine(SDL_Surface *screen, int x0, int y0, int x1, int y1, Uint32 color);
void drawLine(SDL_Surface *screen, Point pos0, Point pos1, Uint32 color);
void drawRectangle(SDL_Surface *surface, Point pos0, Point pos1, Uint32 color);
bool checkPixel(Point px, SDL_Surface *surface);
void setSDL_RGBA(Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask);

/**
 * Creates a SDL_Surface.
 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
 * to settings. The result is a surface which has the same format as the
 * screen surface.
 * Additionally the alpha flag is set, so transparent blits are possible.
 */
SDL_Surface* createAlphaSurface(int width, int height);

/**
 * Creates a SDL_Surface.
 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
 * to settings. The result is a surface which has the same format as the
 * screen surface.
 * The bright pink (rgb 0xff00ff) is set as transparent color.
 */
SDL_Surface* createSurface(int width, int height);

SDL_Surface* scaleSurface(SDL_Surface *source, int width, int height);

/**
 * @brief loadGraphicSurface loads an image from a file.
 * @param filename
 *        The parameter filename is mandatory and specifies the image to be
 *        loaded. The filename will be located via the modmanager.
 * @param errormessage
 *        This is an optional parameter, which defines which error message
 *        should be displayed. If the errormessage is an empty string, no error
 *        message will be printed at all.
 * @param IfNotFoundExit
 *        If this optional boolean parameter is set to true, the program will
 *        shutdown sdl and quit, if the specified image is not found.
 * @param HavePinkColorKey
 *        This optional parameter specifies whether a color key with
 *        RGB(0xff, 0, 0xff) should be applied to the image.
 * @return
 *        Returns the SDL_Surface of the specified image or NULL if not
 *        successful
 */

SDL_Surface* loadGraphicSurface(std::string filename,
								std::string errormessage = "Couldn't load image",
								bool IfNotFoundExit = false,
								bool HavePinkColorKey = false);

#endif // SDLRENDERDEVICE_H
