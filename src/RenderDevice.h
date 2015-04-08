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

#ifndef RENDERDEVICE_H
#define RENDERDEVICE_H

#include <vector>
#include <map>
#include "Utils.h"

class Image;
class RenderDevice;
class FontStyle;

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
	uint32_t getRefCount() const;

	virtual int getWidth() const;
	virtual int getHeight() const;

	virtual void fillWithColor(Uint32 color) = 0;
	virtual void drawPixel(int x, int y, Uint32 color) = 0;
	virtual Uint32 MapRGB(Uint8 r, Uint8 g, Uint8 b) = 0;
	virtual Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;
	virtual Image* resize(int width, int height) = 0;

	class Sprite *createSprite(bool clipToSize = true);

private:
	Image(RenderDevice *device);
	virtual ~Image();
	friend class SDLSoftwareImage;
	friend class SDLHardwareImage;

private:
	RenderDevice *device;
	uint32_t ref_counter;
};

struct Renderable {
public:
	Image *image; // image to be used
	Rect src; // location on the sprite in pixel coordinates.

	FPoint map_pos;     // The map location on the floor between someone's feet
	Point offset;      // offset from map_pos to topleft corner of sprite
	uint64_t prio;     // 64-32 bit for map position, 31-16 for intertile position, 15-0 user dependent, such as Avatar.
	Renderable()
		: image(NULL)
		, src(Rect())
		, map_pos()
		, offset()
		, prio(0) {
	}
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

	/** Context operations */
	virtual int createContext() = 0;
	virtual void destroyContext() = 0;
	virtual Rect getContextSize() = 0;
	virtual void setGamma(float g) = 0;
	virtual void updateTitleBar() = 0;

	/** factory functions for Image */
	virtual Image *loadImage(std::string filename,
							 std::string errormessage = "Couldn't load image",
							 bool IfNotFoundExit = false) = 0;
	virtual Image *createImage(int width, int height) = 0;
	virtual void freeImage(Image *image) = 0;

	/** Screen operations */
	virtual int render(Sprite* r) = 0;
	virtual int render(Renderable& r, Rect dest) = 0;
	virtual int renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest,
							  bool dest_is_transparent = false) = 0;
	virtual int renderText(FontStyle *font, const std::string& text, Color color, Rect& dest) = 0;
	virtual Image* renderTextToImage(FontStyle* font, const std::string& text, Color color, bool blended = true) = 0;
	virtual void blankScreen() = 0;
	virtual void commitFrame() = 0;
	virtual void drawPixel(int x, int y, Uint32 color) = 0;
	virtual void drawRectangle(const Point& p0, const Point& p1, Uint32 color) = 0;
	virtual Uint32 MapRGB(Uint8 r, Uint8 g, Uint8 b) = 0;
	virtual Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;
	virtual void windowResize() = 0;

protected:
	/* Compute clipping and global position from local frame. */
	bool localToGlobal(Sprite *r);

	/* Image cache operations */
	Image *cacheLookup(std::string &filename);
	void cacheStore(std::string &filename, Image *);
	void cacheRemove(Image *image);

	bool fullscreen;
	bool hwsurface;
	bool vsync;
	bool texture_filter;
	Point min_screen;

	bool is_initialized;

	Rect m_clip;
	Rect m_dest;

private:
	typedef std::map<std::string, Image *> IMAGE_CACHE_CONTAINER;
	typedef IMAGE_CACHE_CONTAINER::iterator IMAGE_CACHE_CONTAINER_ITER;

	IMAGE_CACHE_CONTAINER cache;

	virtual void drawLine(int x0, int y0, int x1, int y1, Uint32 color) = 0;
};

#endif // RENDERDEVICE_H
