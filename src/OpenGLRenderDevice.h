/*
Copyright © 2014 Igor Paliychuk

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

#ifndef OPENGL_RENDERDEVICE_H
#define OPENGL_RENDERDEVICE_H

#include "RenderDevice.h"

#include "OpenGL_EXT.h"

/** Provide rendering device using OpenGL backend.
 *
 * Provide an OpenGL implementation for renderning a Renderable to
 * the screen.
 *
 * As this is for the FLARE engine, the implementation uses the engine's
 * global settings context, which is included by the interface.
 *
 * @class OpenGLRenderDevice
 * @see RenderDevice
 *
 */

/**
 * Shared functions for drawing on screen and image
 */
enum DRAW_TYPE {TYPE_LINE, TYPE_PIXEL, TYPE_RECT};

void* openShaderFile(const std::string& filename, GLint *length);
void configureFrameBuffer(GLuint* frameBuffer, GLuint frameTexture, int frame_w, int frame_h);
void disableFrameBuffer(GLuint* frameBuffer, GLint *view_rect);
GLuint getShader(GLenum type, const std::string& filename);
GLuint createProgram(GLuint vertex_shader, GLuint fragment_shader);
GLuint createBuffer(GLenum target, const void *buffer_data, GLsizei buffer_size);
int preparePrimitiveProgram();
void drawPrimitive(GLfloat* vertexData, const Color& color, DRAW_TYPE type);

/** OpenGL Image */
class OpenGLImage : public Image {
public:
	OpenGLImage(RenderDevice *device);
	virtual ~OpenGLImage();
	int getWidth() const;
	int getHeight() const;

	void fillWithColor(const Color& color);
	void drawPixel(int x, int y, const Color& color);
	void drawLine(int x0, int y0, int x1, int y1, const Color& color);
	Image* resize(int width, int height);

	GLuint texture;
	GLuint normalTexture;
	GLuint aoTexture;
	int w;
	int h;
};

class OpenGLRenderDevice : public RenderDevice {

public:

	OpenGLRenderDevice();

	virtual int render(Renderable& r, Rect& dest);
	virtual int render(Sprite* r);
	virtual int renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest);

	Image *renderTextToImage(FontStyle* font_style, const std::string& text, const Color& color, bool blended = true);
	void drawPixel(int x, int y, const Color& color);
	void drawRectangle(const Point& p0, const Point& p1, const Color& color);
	void blankScreen();
	void commitFrame();
	void destroyContext();
	void windowResize();
	Image *createImage(int width, int height);
	void setGamma(float g);
	void resetGamma();
	void updateTitleBar();

	Image* loadImage(const std::string& filename, int error_type);

protected:
	int createContextInternal();
	void createContextError();

private:
	void drawLine(int x0, int y0, int x1, int y1, const Color& color);
	void getWindowSize(short unsigned *screen_w, short unsigned *screen_h);

	int buildResources();
	void composeFrame(GLfloat* offset, GLfloat* texelOffset, bool withLight = false);

	SDL_Window *window;
	SDL_GLContext renderer;
	SDL_Surface* titlebar_icon;
	char* title;

	/* Stores the system gamma levels so they can be restored later */
	uint16_t gamma_r[256];
	uint16_t gamma_g[256];
	uint16_t gamma_b[256];

	GLuint m_vertex_buffer, m_element_buffer;
	GLuint m_vertex_shader, m_fragment_shader, m_program, m_frameBuffer;

	struct {
		GLint texture;
		GLint aotexture;
		GLint normals;
		GLint light;
		GLint screenWidth;
		GLint screenHeight;
		GLint offset;
		GLint texelOffset;
	} uniforms;

	struct {
		GLint position;
	} attributes;

	GLushort m_elementBufferData[4];
	GLfloat m_positionData[8];
	GLfloat m_offset[4]; //x, y, width, height
	GLfloat m_texelOffset[4]; // 1/width, x, 1/height, y
};

#endif // OPENGL_RENDERDEVICE_H
