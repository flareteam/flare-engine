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

void Image::beginPixelBatch() {
}

void Image::endPixelBatch() {
}

/*
 * Sprite
 */
Sprite::Sprite(Image *_image)
	: local_frame(Rect())
	, color_mod(255, 255, 255)
	, alpha_mod(255)
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
	offset = _offset;
}

const Point& Sprite::getOffset() {
	return offset;
}

void Sprite::setClipFromRect(const Rect& clip) {
	src = clip;
}

void Sprite::setClip(const int x, const int y, const int w, const int h) {
	src.x = x;
	src.y = y;
	src.w = w;
	src.h = h;
}

const Rect& Sprite::getClip() {
	return src;
}
void Sprite::setDestFromRect(const Rect& _dest) {
	dest.x = _dest.x;
	dest.y = _dest.y;
}

void Sprite::setDestFromPoint(const Point& _dest) {
	dest.x = _dest.x;
	dest.y = _dest.y;
}

void Sprite::setDest(int x, int y) {
	dest.x = x;
	dest.y = y;
}

const Point& Sprite::getDest() {
	return dest;
}

int Sprite::getGraphicsWidth() {
	return image->getWidth();
}

int Sprite::getGraphicsHeight() {
	return image->getHeight();
}

Image * Sprite::getGraphics() {
	return image;
}


/*
 * RenderDevice
 */

const unsigned char RenderDevice::BITS_PER_PIXEL = 32;

RenderDevice::RenderDevice()
	: fullscreen(false)
	, hwsurface(false)
	, vsync(false)
	, texture_filter(false)
	, ignore_texture_filter(false)
	, min_screen(640, 480)
	, destructive_fullscreen(false)
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
		settings->fullscreen = fullscreen;
		settings->hwsurface = hwsurface;
		settings->vsync = vsync;
		settings->texture_filter = texture_filter;

		status = createContextInternal();
	}

	if (status == -1) {
		// last resort, try turning everything off
		settings->fullscreen = false;
		settings->hwsurface = false;
		settings->vsync = false;
		settings->texture_filter = false;

		status = createContextInternal();
	}

	if (status == -1) {
		// all attempts have failed, abort!
		createContextError();
		Utils::Exit(1);
	}

	return status;
}

void RenderDevice::destroyContext() {
	if (!cache.empty()) {
		IMAGE_CACHE_CONTAINER_ITER it;
		Utils::logError("RenderDevice: Image cache still holding these images:");
		it = cache.begin();
		while (it != cache.end()) {
			Utils::logError("%s %d", it->first.c_str(), it->second->getRefCount());
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

	int left = r->getDest().x - r->getOffset().x;
	int right = left + m_clip.w;
	int up = r->getDest().y - r->getOffset().y;
	int down = up + m_clip.h;

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
			m_clip.x = m_clip.x - left;
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
			m_clip.y = m_clip.y - up;
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
	unsigned short old_view_w = settings->view_w;
	unsigned short old_view_h = settings->view_h;
	unsigned short old_screen_w = settings->screen_w;
	unsigned short old_screen_h = settings->screen_h;

	getWindowSize(&settings->screen_w, &settings->screen_h);

	unsigned short temp_screen_h;
	if (settings->dpi_scaling && ddpi > 0 && eset->resolutions.virtual_dpi > 0) {
		temp_screen_h = static_cast<unsigned short>(static_cast<float>(settings->screen_h) * (eset->resolutions.virtual_dpi / ddpi));
	}
	else {
		temp_screen_h = settings->screen_h;
	}
	settings->view_h = temp_screen_h;

	// scale virtual height when outside of VIRTUAL_HEIGHTS range
	if (!eset->resolutions.virtual_heights.empty()) {
		if (temp_screen_h < eset->resolutions.virtual_heights.front())
			settings->view_h = eset->resolutions.virtual_heights.front();
		else if (temp_screen_h >= eset->resolutions.virtual_heights.back())
			settings->view_h = eset->resolutions.virtual_heights.back();
	}

	settings->view_h_half = settings->view_h / 2;

	settings->view_scaling = static_cast<float>(settings->view_h) / static_cast<float>(settings->screen_h);
	settings->view_w = static_cast<unsigned short>(static_cast<float>(settings->screen_w) * settings->view_scaling);

	// letterbox if too tall
	if (settings->view_w < eset->resolutions.min_screen_w) {
		settings->view_w = eset->resolutions.min_screen_w;
		settings->view_scaling = static_cast<float>(settings->view_w) / static_cast<float>(settings->screen_w);
	}

	settings->view_w_half = settings->view_w/2;

	if (settings->view_w != old_view_w || settings->view_h != old_view_h) {
		Utils::logInfo("RenderDevice: Internal render size is %dx%d", settings->view_w, settings->view_h);
	}
	if (settings->screen_w != old_screen_w || settings->screen_h != old_screen_h) {
		Utils::logInfo("RenderDevice: Window size changed to %dx%d", settings->screen_w, settings->screen_h);
	}
}

void RenderDevice::setBackgroundColor(Color color) {
	// print out the color to avoid unused variable compiler warning
	Utils::logInfo("RenderDevice: Trying to set background color to (%d,%d,%d,%d).", color.r, color.g, color.b, color.a);
	Utils::logError("RenderDevice: Renderer does not support setting background color!");
}

void RenderDevice::setFullscreen(bool enable_fullscreen) {
	Utils::logInfo("RenderDevice: Trying to set fullscreen=%d, without recreating the rendering context, but setFullscreen() is not implemented for this renderer.", enable_fullscreen);
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
