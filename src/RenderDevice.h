/*
Copyright © 2013 Kurt Rinnert
Copyright © 2013 Igor Paliychuk

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

#define Image SDL_Surface

struct Renderable {
public:
	Image *sprite; // image to be used
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

class ISprite {

public:
	ISprite()
		: local_frame(SDL_Rect())
		, keep_graphics(false)
		, sprite(NULL)
		, src(SDL_Rect())
		, offset()
		, dest()
	{}
	virtual ~ISprite() {}

	SDL_Rect local_frame;
	bool keep_graphics; // don't free the sprite surface when deconstructing, used primarily for animations

	virtual void setGraphics(Image *s, bool setClipToFull = true) = 0;
	virtual Image * getGraphics() = 0;
	virtual bool graphicsIsNull() = 0;
	virtual void clearGraphics() = 0;
	virtual void setOffset(const Point& _offset) = 0;
	virtual void setOffset(const int x, const int y) = 0;
	virtual Point getOffset() = 0;
	virtual void setClip(const SDL_Rect& clip) = 0;
	virtual void setClip(const int x, const int y, const int w, const int h) = 0;
	virtual void setClipX(const int x) = 0;
	virtual void setClipY(const int y) = 0;
	virtual void setClipW(const int w) = 0;
	virtual void setClipH(const int h) = 0;
	virtual SDL_Rect getClip() = 0;
	virtual void setDest(const SDL_Rect& _dest) = 0;
	virtual void setDest(const Point& _dest) = 0;
	virtual void setDest(int x, int y) = 0;
	virtual void setDestX(int x) = 0;
	virtual void setDestY(int y) = 0;
	virtual FPoint getDest() = 0;
	virtual int getGraphicsWidth() = 0;
	virtual int getGraphicsHeight() = 0;

protected:
	Image *sprite; // image to be used
	SDL_Rect src; // location on the sprite in pixel coordinates.
	Point offset;      // offset from map_pos to topleft corner of sprite
	FPoint dest;
};

// inherit this class and implement it as xRenderDevice class
class RenderDevice {

public:

	/** Constuctor.
	 */
	RenderDevice() : is_initialized(false) {}

	/** Destructor.
	 */
	virtual ~RenderDevice() {}

	/** Create context on startup.
	 */
	virtual int createContext(int width, int height) = 0;

	virtual SDL_Rect getContextSize() = 0;

	/** Render a Renderable to the screen.
	 */
	virtual int render(ISprite& r) = 0;
	virtual int render(Renderable& r, SDL_Rect dest) = 0;
	virtual int renderImage(Image* image, SDL_Rect& src) = 0;
	virtual int renderToImage(Image* src_image, SDL_Rect& src, Image* dest_image, SDL_Rect& dest, bool dest_is_transparent = false) = 0;

	/** Render text to the screen.
	 */
	virtual int renderText(TTF_Font *ttf_font, const std::string& text, SDL_Color color, SDL_Rect& dest) = 0;

	/** Draw pixel to screen.
	 */
	virtual void drawPixel(int x, int y, Uint32 color) = 0;

	/** Draw line to screen.
	 *
	 *  Draw line connecting (x0,y0) and (x1,y1) to screen.
	 */
	virtual void drawLine(int x0, int y0, int x1, int y1, Uint32 color) = 0;

	/** Draw line to screen.
	 *
	 *  Draw line connecting p0 and p1 to screen.
	 */
	virtual void drawLine(const Point& p0, const Point& p1, Uint32 color) = 0;

	/** Draw rectangle to screen.
	 *
	 *  Draw rectangle defined by p0 and p1 to screen.
	 */
	virtual void drawRectangle(const Point& p0, const Point& p1, Uint32 color) = 0;

	/** Blank the screen.
	 */
	virtual void blankScreen() = 0;

	/** Commit the next frame to the display.
	 */
	virtual void commitFrame() = 0;

	/** Destroy context on exit.
	 */
	virtual void destroyContext() = 0;

	virtual void fillImageWithColor(Image *dst, SDL_Rect *dstrect, Uint32 color) = 0;

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

	virtual void loadIcons() = 0;

protected:

	bool is_initialized; ///< true if a context was created once
};

Uint32 readPixel(Image *screen, int x, int y);
void drawPixel(Image *screen, int x, int y, Uint32 color);
void drawLine(Image *screen, int x0, int y0, int x1, int y1, Uint32 color);
void drawLine(Image *screen, Point pos0, Point pos1, Uint32 color);
void drawRectangle(Image *surface, Point pos0, Point pos1, Uint32 color);
bool checkPixel(Point px, Image *surface);
void setSDL_RGBA(Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask);
void setColorKey(Image *surface, int flag, int key);
void setAlpha(Image *surface, int flag, int alpha);

/**
 * Creates a SDL_Surface.
 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
 * to settings. The result is a surface which has the same format as the
 * screen surface.
 * Additionally the alpha flag is set, so transparent blits are possible.
 */
Image* createAlphaSurface(int width, int height);

/**
 * Creates a SDL_Surface.
 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
 * to settings. The result is a surface which has the same format as the
 * screen surface.
 * The bright pink (rgb 0xff00ff) is set as transparent color.
 */
Image* createSurface(int width, int height);

Image* scaleSurface(Image *source, int width, int height);

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

Image* loadGraphicSurface(std::string filename,
								std::string errormessage = "Couldn't load image",
								bool IfNotFoundExit = false,
								bool HavePinkColorKey = false);

void freeImage(Image* image);

#endif // RENDERDEVICE_H
