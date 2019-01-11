/*
Copyright © 2013 Kurt Rinnert
Copyright © 2013 Igor Paliychuk
Copyright © 2014 Henrik Andersson
Copyright © 2013-2016 Justin Jacobs

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
	~Sprite();

	Rect local_frame;
	Color color_mod;
	uint8_t alpha_mod;

	Image * getGraphics();
	void setOffset(const Point& _offset);
	const Point& getOffset();
	void setClipFromRect(const Rect& clip);
	void setClip(const int x, const int y, const int w, const int h);
	const Rect& getClip();
	void setDestFromRect(const Rect& _dest);
	void setDestFromPoint(const Point& _dest);
	void setDest(int x, int y);
	const Point& getDest();
	int getGraphicsWidth();
	int getGraphicsHeight();
private:
	explicit Sprite(Image *);
	friend class Image;

protected:
	/** reference to source image */
	Image *image;
	Rect src; // location on the sprite in pixel coordinates.
	Point offset;      // offset from map_pos to topleft corner of sprite
	Point dest;
};

/** An image representation
 * An image can only be instantiated, and is owned, by a RenderDevice
 * For a SDL render device this means SDL_Surface or a SDL_Texture, and
 * by OpenGL render device this is a texture.
 *
 * Image uses a refrence counter to control when to free the resource, when the
 * last reference is released, the Image is deleted and then removed from cache
 * using RenderDevice::freeImage().
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

	virtual void fillWithColor(const Color& color) = 0;
	virtual void drawPixel(int x, int y, const Color& color) = 0;
	virtual void drawLine(int x0, int y0, int x1, int y1, const Color& color) = 0;
	virtual void beginPixelBatch();
	virtual void endPixelBatch();
	virtual Image* resize(int width, int height) = 0;

	class Sprite *createSprite();

private:
	explicit Image(RenderDevice *device);
	virtual ~Image();
	friend class SDLSoftwareImage;
	friend class SDLHardwareImage;

private:
	RenderDevice *device;
	uint32_t ref_counter;
};

class Renderable {
public:
	enum {
		BLEND_NORMAL = 0,
		BLEND_ADD = 1
	};

	enum {
		TYPE_NORMAL = 0,
		TYPE_HERO = 1,
		TYPE_ENEMY = 2,
		TYPE_ALLY = 3
	};

	Image *image; // image to be used
	Rect src; // location on the sprite in pixel coordinates.

	FPoint map_pos;     // The map location on the floor between someone's feet
	Point offset;      // offset from map_pos to topleft corner of sprite
	uint64_t prio;     // 64-32 bit for map position, 31-16 for intertile position, 15-0 user dependent, such as Avatar.

	uint8_t blend_mode;
	Color color_mod;
	uint8_t alpha_mod;

	uint8_t type;

	Renderable()
		: image(NULL)
		, src(Rect())
		, map_pos()
		, offset()
		, prio(0)
		, blend_mode(BLEND_NORMAL)
		, color_mod(255, 255, 255)
		, alpha_mod(255)
		, type(TYPE_NORMAL) {
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
	enum {
		ERROR_NONE = 0,
		ERROR_NORMAL = 1,
		ERROR_EXIT = 2
	};

	static const unsigned char BITS_PER_PIXEL;

	RenderDevice();
	virtual ~RenderDevice();

	/** Context operations */
	int createContext();
	virtual void destroyContext() = 0;
	virtual void setGamma(float g) = 0;
	virtual void resetGamma() = 0;
	virtual void updateTitleBar() = 0;

	/** factory functions for Image */
	virtual Image *loadImage(const std::string& filename, int error_type) = 0;
	virtual Image *createImage(int width, int height) = 0;
	void freeImage(Image *image);

	/** Screen operations */
	virtual int render(Sprite* r) = 0;
	virtual int render(Renderable& r, Rect& dest) = 0;
	virtual int renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest) = 0;
	virtual Image* renderTextToImage(FontStyle* font_style, const std::string& text, const Color& color, bool blended) = 0;
	virtual void blankScreen() = 0;
	virtual void commitFrame() = 0;
	virtual void drawPixel(int x, int y, const Color& color) = 0;
	virtual void drawLine(int x0, int y0, int x1, int y1, const Color& color) = 0;
	virtual void drawRectangle(const Point& p0, const Point& p1, const Color& color) = 0;
	void drawEllipse(int x0, int y0, int x1, int y1, const Color& color, float step);
	virtual void windowResize() = 0;
	virtual void setBackgroundColor(Color color);
	virtual void setFullscreen(bool enable_fullscreen);

	bool reloadGraphics();

protected:
	/* Compute clipping and global position from local frame. */
	bool localToGlobal(Sprite *r);

	/* Image cache operations */
	Image *cacheLookup(const std::string &filename);
	void cacheStore(const std::string &filename, Image *);
	void cacheRemove(Image *image);
	void cacheRemoveAll();
	void windowResizeInternal();

	/** Context operations */
	virtual int createContextInternal() = 0;
	virtual void createContextError() = 0;

	bool fullscreen;
	bool hwsurface;
	bool vsync;
	bool texture_filter;
	bool ignore_texture_filter;
	Point min_screen;
	bool destructive_fullscreen;

	bool is_initialized;
	bool reload_graphics;

	float ddpi;

	Rect m_clip;
	Rect m_dest;

	/* Stores the system gamma levels so they can be restored later */
	uint16_t gamma_r[256];
	uint16_t gamma_g[256];
	uint16_t gamma_b[256];

private:
	typedef std::map<std::string, Image *> IMAGE_CACHE_CONTAINER;
	typedef IMAGE_CACHE_CONTAINER::iterator IMAGE_CACHE_CONTAINER_ITER;

	IMAGE_CACHE_CONTAINER cache;

	virtual void getWindowSize(short unsigned *screen_w, short unsigned *screen_h) = 0;
};

#endif // RENDERDEVICE_H
