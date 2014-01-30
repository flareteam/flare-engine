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

class Sprite : public ISprite {

public:
	Sprite() {};
	Sprite(const Sprite& other);
	Sprite& operator=(const Sprite& other);
	~Sprite();

	void setGraphics(Image s, bool setClipToFull = true);
	Image * getGraphics();
	bool graphicsIsNull();
	void clearGraphics();
	void setOffset(const Point& _offset);
	void setOffset(const int x, const int y);
	Point getOffset();
	void setClip(const Rect& clip);
	void setClip(const int x, const int y, const int w, const int h);
	void setClipX(const int x);
	void setClipY(const int y);
	void setClipW(const int w);
	void setClipH(const int h);
	Rect getClip();
	void setDest(const Rect& _dest);
	void setDest(const Point& _dest);
	void setDest(int x, int y);
	void setDestX(int x);
	void setDestY(int y);
	FPoint getDest();
	int getGraphicsWidth();
	int getGraphicsHeight();
};

class SDLRenderDevice : public RenderDevice {

public:

	SDLRenderDevice();
	int createContext(int width, int height);
	Rect getContextSize();

	virtual int render(Renderable& r, Rect dest);
	virtual int render(ISprite& r);
	virtual int renderImage(Image* image, Rect& src);
	virtual int renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest, bool dest_is_transparent = false);

	int renderText(TTF_Font *ttf_font, const std::string& text, Color color, Rect& dest);

	void renderTextToImage(Image* image, TTF_Font* ttf_font, const std::string& text, Color color, bool blended = true);

	void drawPixel(int x, int y, Uint32 color);

	void drawPixel(Image *image, int x, int y, Uint32 pixel);

	void drawLine(int x0, int y0, int x1, int y1, Uint32 color);

	void drawLine(const Point& p0, const Point& p1, Uint32 color);

	void drawLine(Image *image, int x0, int y0, int x1, int y1, Uint32 color);

	void drawLine(Image *image, Point pos0, Point pos1, Uint32 color);

	void drawRectangle(const Point& p0, const Point& p1, Uint32 color);

	void drawRectangle(Image *image, Point pos0, Point pos1, Uint32 color);

	void blankScreen();

	void commitFrame();

	void destroyContext();

	void fillImageWithColor(Image *dst, Rect *dstrect, Uint32 color);

	Uint32 MapRGB(Image *src, Uint8 r, Uint8 g, Uint8 b);

	Uint32 MapRGB(Uint8 r, Uint8 g, Uint8 b);

	Uint32 MapRGBA(Image *src, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a);

	Image createAlphaSurface(int width, int height);

	Image createSurface(int width, int height);

	void scaleSurface(Image *source, int width, int height);

	void setGamma(float g);

	void listModes(std::vector<Rect> &modes);

	Uint32 readPixel(Image *image, int x, int y);

	bool checkPixel(Point px, Image *image);

	void freeImage(Image *image);

	Image loadGraphicSurface(std::string filename,
								std::string errormessage = "Couldn't load image",
								bool IfNotFoundExit = false,
								bool HavePinkColorKey = false);
private:

	// Compute clipping and global position from local frame.
	bool local_to_global(ISprite& r);

	SDL_Surface* screen;
	SDL_Surface* titlebar_icon;

	// These are for keeping the render stack frame small.
	Rect m_clip;
	Rect m_dest;
	Sprite m_ttf_renderable;
};

#endif // SDLRENDERDEVICE_H
