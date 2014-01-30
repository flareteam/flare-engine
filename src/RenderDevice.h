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

#pragma once
#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include <vector>
#include <SDL_ttf.h>
#include "Utils.h"

class Image;
class RenderDevice;

/** A Sprite representation
 *
 * A Sprite is instantiated from a Image instance using
 * Image::createSprite() which will increase the reference counter of
 * the image. Sprite::~Sprite() will release the reference to the
 * source image instance.
 *
 * A Sprite represents an area in a Image, it can be the full image or
 * just parts of the image such as an image atlas / spritemap.
 *
 * Sprite constructor is private to prevent creation of Sprites
 * outside of Image instance.
 *
 * @class Sprite
 * @author Henrik Andersson
 * @date 2014-01-30
 *
 */
class Sprite {

public:
        virtual ~Sprite();

	Rect local_frame;

	virtual Image * getGraphics();
	virtual void setOffset(const Point& _offset);
	virtual void setOffset(const int x, const int y);
	virtual Point getOffset();
	virtual void setClip(const Rect& clip);
	virtual void setClip(const int x, const int y, const int w, const int h);
	virtual void setClipX(const int x);
	virtual void setClipY(const int y);
	virtual void setClipW(const int w);
	virtual void setClipH(const int h);
	virtual Rect getClip();
	virtual void setDest(const Rect& _dest);
	virtual void setDest(const Point& _dest);
	virtual void setDest(int x, int y);
	virtual void setDestX(int x);
	virtual void setDestY(int y);
	virtual FPoint getDest();
	virtual int getGraphicsWidth();
	virtual int getGraphicsHeight();
private:
        Sprite(Image *);
	friend class Image;

protected:
	/** reference to source image */
	Image *image;
	Rect src; // location on the sprite in pixel coordinates.
	Point offset;      // offset from map_pos to topleft corner of sprite
	FPoint dest;
};

/** An image representation
 * An image can only be instantiated, and is owned, by a RenderDevice
 * For a SDL render device this means SDL_Surface or a SDL_Texture, and
 * by OpenGL render device this is a texture.
 * 
 * Image uses a refrence counter to control when to free the resource, when the
 * last reference is released, the Image is freed using RenderDevice::freeImage().
 *
 * The caller who instantiates an Image is responsible for release the reference
 * to the image when not used anymore. 
 *
 * Image is a source for a Sprite and is therefor responsible for instantiating
 * Sprites using Image::createSprite().
 * 
 * Creating a Sprite of a Image increases the reference counter, destructor of a
 * Sprite will release the reference to the image.
 *
 * @class Image
 * @author Henrik Andersson
 * @date 2014-01-30 
 */
class Image {
public:
  void ref();
  void unref();
  int getWidth() const;
  int getHeight() const;

  class Sprite *createSprite(bool clipToSize = true);

  // TODO Only use this in a derivative class in SDLRenderDevice
  SDL_Surface *surface;

private:
  Image(RenderDevice *device);
  ~Image();
  friend class SDLRenderDevice;

private:
  RenderDevice *device;
  uint32_t ref_counter;
};

struct Renderable {
public:
	Image *sprite; // image to be used
	Rect src; // location on the sprite in pixel coordinates.

	FPoint map_pos;     // The map location on the floor between someone's feet
	Point offset;      // offset from map_pos to topleft corner of sprite
	uint64_t prio;     // 64-32 bit for map position, 31-16 for intertile position, 15-0 user dependent, such as Avatar.
	Renderable()
		: sprite(NULL)
		, src(Rect())
		, map_pos()
		, offset()
		, prio(0)
	{}
};



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
 * @author Henrik Andersson
 * @date 2013-07-06
 *
 */
class RenderDevice {

public:
        RenderDevice();
	virtual ~RenderDevice();

	/** Create context on startup.
	 */
	virtual int createContext(int width, int height) = 0;

	virtual Rect getContextSize() = 0;

	/** Render a Renderable to the screen.
	 */
	virtual int render(Sprite* r) = 0;
	virtual int render(Renderable& r, Rect dest) = 0;
	virtual int renderImage(Image* image, Rect& src) = 0;
	virtual int renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest, bool dest_is_transparent = false) = 0;

	/** Render text to the screen.
	 */
	virtual int renderText(TTF_Font *ttf_font, const std::string& text, Color color, Rect& dest) = 0;

	/** Renders text to an image, but does not actually blit it
	 */
	virtual Image *renderTextToImage(TTF_Font* ttf_font, const std::string& text, Color color, bool blended = true) = 0;

	/** Draw pixel to screen.
	 */
	virtual void drawPixel(int x, int y, Uint32 color) = 0;

	/** Draw pixel to Image.
	 */
	virtual void drawPixel(Image *image, int x, int y, Uint32 color) = 0;

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

	/** Draw line to screen.
	 *
	 *  Draw line connecting (x0,y0) and (x1,y1) to screen.
	 */
	virtual void drawLine(Image *image, int x0, int y0, int x1, int y1, Uint32 color) = 0;

	/** Draw line to screen.
	 *
	 *  Draw line connecting p0 and p1 to Image.
	 */
	virtual void drawLine(Image *image, Point pos0, Point pos1, Uint32 color) = 0;

	/** Draw rectangle to screen.
	 *
	 *  Draw rectangle defined by p0 and p1 to screen.
	 */
	virtual void drawRectangle(const Point& p0, const Point& p1, Uint32 color) = 0;

	/** Draw rectangle to screen.
	 *
	 *  Draw rectangle defined by p0 and p1 to Image.
	 */
	virtual void drawRectangle(Image *image, Point pos0, Point pos1, Uint32 color) = 0;

	/** Blank the screen.
	 */
	virtual void blankScreen() = 0;

	/** Commit the next frame to the display.
	 */
	virtual void commitFrame() = 0;

	/** Destroy context on exit.
	 */
	virtual void destroyContext() = 0;

	virtual void fillImageWithColor(Image *dst, Rect *dstrect, Uint32 color) = 0;

	/**
	 * Map a RGB color value to a pixel format.
	 */
	virtual Uint32 MapRGB(Image *src, Uint8 r, Uint8 g, Uint8 b) = 0;

	/**
	 * Map a RGB color value to a screen pixel format.
	 */
	virtual Uint32 MapRGB(Uint8 r, Uint8 g, Uint8 b) = 0;

	/**
	 * Map a RGBA color value to a pixel format.
	 */
	virtual Uint32 MapRGBA(Image *src, Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;

	/**
	 * Map a RGBA color value to a screen pixel format.
	 */
	virtual Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;

	/**
	 * Creates a SDL_Surface.
	 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
	 * to settings. The result is a surface which has the same format as the
	 * screen surface.
	 * Additionally the alpha flag is set, so transparent blits are possible.
	 */
	virtual Image *createAlphaSurface(int width, int height) = 0;

	/**
	 * Creates a SDL_Surface.
	 * The SDL_HWSURFACE or SDL_SWSURFACE flag is set according
	 * to settings. The result is a surface which has the same format as the
	 * screen surface.
	 * The bright pink (rgb 0xff00ff) is set as transparent color.
	 */
	virtual Image *createSurface(int width, int height) = 0;

	virtual void scaleSurface(Image *source, int width, int height) = 0;

	virtual void setGamma(float g) = 0;

	virtual void listModes(std::vector<Rect> &modes) = 0;

	virtual Uint32 readPixel(Image *image, int x, int y) = 0;

	virtual bool checkPixel(Point px, Image *image) = 0;

	virtual void freeImage(Image *image) = 0;

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
	 *        Returns the an Image * of the specified image or NULL if not
	 *        successful
	 */
	virtual Image *loadGraphicSurface(std::string filename,
					  std::string errormessage = "Couldn't load image",
					  bool IfNotFoundExit = false,
					  bool HavePinkColorKey = false) = 0;

protected:

	bool is_initialized; ///< true if a context was created once
};

void setSDL_RGBA(Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask);

#endif // RENDERDEVICE_H
