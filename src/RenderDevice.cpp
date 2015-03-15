/*
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

#include <assert.h>
#include <stdio.h>
#include "RenderDevice.h"


/*
 * Image
 */
Image::Image(RenderDevice *_device)
	: device(_device)
	, ref_counter(1) {
}

Image::~Image() {
	/* free resource allocated by renderdevice */
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

Sprite *Image::createSprite(bool clipToSize) {
	Sprite *sprite;
	sprite = new Sprite(this);
	if (clipToSize)
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
	dest.x = (float)_dest.x;
	dest.y = (float)_dest.y;
}

void Sprite::setDest(const Point& _dest) {
	dest.x = (float)_dest.x;
	dest.y = (float)_dest.y;
}

void Sprite::setDest(int x, int y) {
	dest.x = (float)x;
	dest.y = (float)y;
}

void Sprite::setDestX(int x) {
	dest.x = (float)x;
}

void Sprite::setDestY(int y) {
	dest.y = (float)y;
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
	return (Image *)image;
}


/*
 * RenderDevice
 */
RenderDevice::RenderDevice()
	: fullscreen(false)
	, hwsurface(false)
	, doublebuf(false)
	, min_screen(640, 480)
	, is_initialized(false) {
}

RenderDevice::~RenderDevice() {
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

Image * RenderDevice::cacheLookup(std::string &filename) {
	IMAGE_CACHE_CONTAINER_ITER it;
	it = cache.find(filename);
	if (it != cache.end()) {
		it->second->ref();
		return it->second;
	}
	return NULL;
}

void RenderDevice::cacheStore(std::string &filename, Image *image) {
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

bool RenderDevice::localToGlobal(Sprite *r) {
	m_clip = r->getClip();

	int left = int(r->getDest().x - r->getOffset().x);
	int right = left + r->getClip().w;
	int up = int(r->getDest().y - r->getOffset().y);
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
