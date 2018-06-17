/*
Copyright © 2014 Henrik Andersson
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

#include "EngineSettings.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

/*
 * Image
 */
Image::Image(RenderDevice *_device)
	: device(_device)
	, ref_counter(1) {
}

Image::~Image() {
	// remove this image from the cache
	device->freeImage(this);
}

void Image::ref() {
	++ref_counter;
}

void Image::unref() {
	--ref_counter;
	if (ref_counter == 0)
		delete this;
}

uint32_t Image::getRefCount() const {
	return ref_counter;
}

int Image::getWidth() const {
	return 0;
}

int Image::getHeight() const {
	return 0;
}

Sprite *Image::createSprite() {
	Sprite *sprite;
	sprite = new Sprite(this);
	sprite->setClip(0, 0, this->getWidth(), this->getHeight());
	return sprite;
}


/*
 * Sprite
 */
Sprite::Sprite(Image *_image)
	: local_frame(Rect())
	, image(_image)
	, src(Rect())
	, offset()
	, dest() {
	image->ref();
}

Sprite::~Sprite() {
	image->unref();
}

void Sprite::setOffset(const Point& _offset) {
	this->offset = _offset;
}

void Sprite::setOffset(const int x, const int y) {
	this->offset.x = x;
	this->offset.y = y;
}

Point Sprite::getOffset() {

	return offset;
}

void Sprite::setClip(const Rect& clip) {
	src = clip;
}

void Sprite::setClip(const int x, const int y, const int w, const int h) {
	src.x = x;
	src.y = y;
	src.w = w;
	src.h = h;
}

void Sprite::setClipX(const int x) {
	src.x = x;
}

void Sprite::setClipY(const int y) {
	src.y = y;
}

void Sprite::setClipW(const int w) {
	src.w = w;
}

void Sprite::setClipH(const int h) {
	src.h = h;
}


Rect Sprite::getClip() {
	return src;
}
void Sprite::setDest(const Rect& _dest) {
	dest.x = static_cast<float>(_dest.x);
	dest.y = static_cast<float>(_dest.y);
}

void Sprite::setDest(const Point& _dest) {
	dest.x = static_cast<float>(_dest.x);
	dest.y = static_cast<float>(_dest.y);
}

void Sprite::setDest(int x, int y) {
	dest.x = static_cast<float>(x);
	dest.y = static_cast<float>(y);
}

void Sprite::setDestX(int x) {
	dest.x = static_cast<float>(x);
}

void Sprite::setDestY(int y) {
	dest.y = static_cast<float>(y);
}

FPoint Sprite::getDest() {
	return dest;
}

int Sprite::getGraphicsWidth() {
	return image->getWidth();
}

int Sprite::getGraphicsHeight() {
	return image->getHeight();
}

Image * Sprite::getGraphics() {
	return static_cast<Image *>(image);
}


/*
 * RenderDevice
 */
RenderDevice::RenderDevice()
	: fullscreen(false)
	, hwsurface(false)
	, vsync(false)
	, texture_filter(false)
	, min_screen(640, 480)
	, is_initialized(false)
	, reload_graphics(false)
	, ddpi(0)
{
	// don't bother initializing gamma_r, gamma_g, gamma_b
	// it is up to the implemented render device to initialize them
	// for example, SDL_GetWindowGammaRamp() can fill them in
}

RenderDevice::~RenderDevice() {
}

int RenderDevice::createContext() {
	int status = createContextInternal();

	if (status == -1) {
		// try previous setting first
		FULLSCREEN = fullscreen;
		HWSURFACE = hwsurface;
		VSYNC = vsync;
		TEXTURE_FILTER = texture_filter;

		status = createContextInternal();
	}

	if (status == -1) {
		// last resort, try turning everything off
		FULLSCREEN = false;
		HWSURFACE = false;
		VSYNC = false;
		TEXTURE_FILTER = false;

		status = createContextInternal();
	}

	if (status == -1) {
		// all attempts have failed, abort!
		createContextError();
		Exit(1);
	}

	return status;
}

void RenderDevice::destroyContext() {
	if (!cache.empty()) {
		IMAGE_CACHE_CONTAINER_ITER it;
		logError("RenderDevice: Image cache still holding these images:");
		it = cache.begin();
		while (it != cache.end()) {
			logError("%s %d", it->first.c_str(), it->second->getRefCount());
			++it;
		}
	}
	assert(cache.empty());
}

Image * RenderDevice::cacheLookup(const std::string &filename) {
	IMAGE_CACHE_CONTAINER_ITER it;
	it = cache.find(filename);
	if (it != cache.end()) {
		it->second->ref();
		return it->second;
	}
	return NULL;
}

void RenderDevice::cacheStore(const std::string &filename, Image *image) {
	if (image == NULL) return;
	cache[filename] = image;
}

void RenderDevice::cacheRemove(Image *image) {
	IMAGE_CACHE_CONTAINER_ITER it = cache.begin();
	while (it != cache.end()) {
		if (it->second == image)
			break;
		++it;
	}

	if (it != cache.end()) {
		cache.erase(it);
	}
}

void RenderDevice::cacheRemoveAll() {
	IMAGE_CACHE_CONTAINER_ITER it = cache.begin();

	while (it != cache.end()) {
		cacheRemove(it->second);
		it = cache.begin();
	}
}

bool RenderDevice::localToGlobal(Sprite *r) {
	m_clip = r->getClip();

	int left = static_cast<int>(r->getDest().x) - r->getOffset().x;
	int right = left + r->getClip().w;
	int up = static_cast<int>(r->getDest().y) - r->getOffset().y;
	int down = up + r->getClip().h;

	// Check whether we need to render.
	// If so, compute the correct clipping.
	if (r->local_frame.w) {
		if (left > r->local_frame.w) {
			return false;
		}
		if (right < 0) {
			return false;
		}
		if (left < 0) {
			m_clip.x = r->getClip().x - left;
			left = 0;
		};
		right = (right < r->local_frame.w ? right : r->local_frame.w);
		m_clip.w = right - left;
	}
	if (r->local_frame.h) {
		if (up > r->local_frame.h) {
			return false;
		}
		if (down < 0) {
			return false;
		}
		if (up < 0) {
			m_clip.y = r->getClip().y - up;
			up = 0;
		};
		down = (down < r->local_frame.h ? down : r->local_frame.h);
		m_clip.h = down - up;
	}

	m_dest.x = left + r->local_frame.x;
	m_dest.y = up + r->local_frame.y;

	return true;
}

bool RenderDevice::reloadGraphics() {
	if (reload_graphics) {
		reload_graphics = false;
		return true;
	}

	return false;
}

void RenderDevice::freeImage(Image *image) {
	if (!image) return;

	cacheRemove(image);
}

void RenderDevice::windowResizeInternal() {
	unsigned short old_view_w = VIEW_W;
	unsigned short old_view_h = VIEW_H;
	unsigned short old_screen_w = SCREEN_W;
	unsigned short old_screen_h = SCREEN_H;

	getWindowSize(&SCREEN_W, &SCREEN_H);

	unsigned short temp_screen_h;
	if (DPI_SCALING && ddpi > 0 && eset->resolutions.virtual_dpi > 0) {
		temp_screen_h = static_cast<unsigned short>(static_cast<float>(SCREEN_H) * (eset->resolutions.virtual_dpi / ddpi));
	}
	else {
		temp_screen_h = SCREEN_H;
	}
	VIEW_H = temp_screen_h;

	// scale virtual height when outside of VIRTUAL_HEIGHTS range
	if (!eset->resolutions.virtual_heights.empty()) {
		if (temp_screen_h < eset->resolutions.virtual_heights.front())
			VIEW_H = eset->resolutions.virtual_heights.front();
		else if (temp_screen_h >= eset->resolutions.virtual_heights.back())
			VIEW_H = eset->resolutions.virtual_heights.back();
	}

	VIEW_H_HALF = VIEW_H / 2;

	VIEW_SCALING = static_cast<float>(VIEW_H) / static_cast<float>(SCREEN_H);
	VIEW_W = static_cast<unsigned short>(static_cast<float>(SCREEN_W) * VIEW_SCALING);

	// letterbox if too tall
	if (VIEW_W < eset->resolutions.min_screen_w) {
		VIEW_W = eset->resolutions.min_screen_w;
		VIEW_SCALING = static_cast<float>(VIEW_W) / static_cast<float>(SCREEN_W);
	}

	VIEW_W_HALF = VIEW_W/2;

	if (VIEW_W != old_view_w || VIEW_H != old_view_h) {
		logInfo("RenderDevice: Internal render size is %dx%d", VIEW_W, VIEW_H);
	}
	if (SCREEN_W != old_screen_w || SCREEN_H != old_screen_h) {
		logInfo("RenderDevice: Window size changed to %dx%d", SCREEN_W, SCREEN_H);
	}
}

void RenderDevice::setBackgroundColor(Color color) {
	// print out the color to avoid unused variable compiler warning
	logInfo("RenderDevice: Trying to set background color to (%d,%d,%d,%d).", color.r, color.g, color.b, color.a);
	logError("RenderDevice: Renderer does not support setting background color!");
}

void RenderDevice::drawEllipse(int x0, int y0, int x1, int y1, const Color& color, float step) {
	float rx = static_cast<float>(x1 - x0) / 2.f;
	float ry = static_cast<float>(y1 - y0) / 2.f;
	float cx = static_cast<float>(x1 + x0) / 2.f;
	float cy = static_cast<float>(y1 + y0) / 2.f;

	float rad = (step/180) * static_cast<float>(M_PI);

	float lastx = 0;
	float lasty = 0;

	for (float i = 0; i < M_PI*2; i+=rad) {
		float curx = cx + cosf(i) * rx;
		float cury = cy + sinf(i) * ry;

		if (i > 0)
			drawLine(static_cast<int>(lastx), static_cast<int>(lasty), static_cast<int>(curx), static_cast<int>(cury), color);

		lastx = curx;
		lasty = cury;
	}
}
