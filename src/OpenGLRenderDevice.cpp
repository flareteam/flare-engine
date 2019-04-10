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

#include <SDL_image.h>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CursorManager.h"
#include "EngineSettings.h"
#include "IconManager.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "ModManager.h"
#include "SharedResources.h"
#include "Settings.h"

#include "OpenGLRenderDevice.h"
#include "SDLFontEngine.h"

#ifdef _WIN32
#define glActiveTexture glWinActiveTexture
#endif

/**
 * These will be used for both drawing on screen and image
 */
GLuint g_vertex_shader, g_fragment_shader, g_program;
GLushort g_elementBufferData[4];
GLint g_position, g_color;

int preparePrimitiveProgram()
{
	g_vertex_shader = getShader(GL_VERTEX_SHADER, "shaders/vertex_p.glsl");
	if (g_vertex_shader == 0)
		return 1;

	g_fragment_shader = getShader(GL_FRAGMENT_SHADER, "shaders/fragment_p.glsl");
	if (g_fragment_shader == 0)
		return 1;

	g_program = createProgram(g_vertex_shader, g_fragment_shader);
	if (g_program == 0)
		return 1;

	g_position = glGetAttribLocation(g_program, "position");
	g_color = glGetUniformLocation(g_program, "color");

	g_elementBufferData[0] = 0;
	g_elementBufferData[1] = 1;
	g_elementBufferData[2] = 2;
	g_elementBufferData[3] = 3;

	return 0;
}

void drawPrimitive(GLfloat* vertexData, const Color& color, DRAW_TYPE type)
{
	GLsizei points_count = 1;
	GLenum mode;

	if (type == TYPE_PIXEL)
	{
		points_count = 1;
		mode = GL_POINTS;
	}
	else if (type == TYPE_LINE)
	{
		Utils::logInfo("drawPrimitive(TYPE_LINE) UNIMPLEMENTED");
		points_count = 2;
		mode = GL_LINE_STRIP;
		return;
	}
	else if (type == TYPE_RECT)
	{
		points_count = 4;
		mode = GL_LINE_LOOP;
	}

	GLuint g_vertex_buffer = createBuffer(GL_ARRAY_BUFFER, vertexData, static_cast<GLsizei>(sizeof(GLfloat)*points_count*2));
	GLuint g_element_buffer = createBuffer(GL_ELEMENT_ARRAY_BUFFER, g_elementBufferData, static_cast<GLsizei>(sizeof(GLushort)*points_count));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glUseProgram(g_program);

	GLfloat _color[4];
	_color[0] = static_cast<float>(color.r) / 255.0f;
	_color[1] = static_cast<float>(color.g) / 255.0f;
	_color[2] = static_cast<float>(color.b) / 255.0f;
	_color[3] = static_cast<float>(color.a) / 255.0f;

	glUniform4fv(g_color, 1, _color);

	glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
	glVertexAttribPointer(
		g_position,
		2, GL_FLOAT, GL_FALSE,
		sizeof(GLfloat)*2, (void*)0
	);

	glEnableVertexAttribArray(g_position);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_element_buffer);
	glDrawElements(
		mode,               /* mode */
		points_count,       /* count */
		GL_UNSIGNED_SHORT,  /* type */
		(void*)0            /* element array buffer offset */
	);

	glDisableVertexAttribArray(g_position);

	glDeleteBuffers(1, &g_vertex_buffer);
	glDeleteBuffers(1, &g_element_buffer);
}

OpenGLImage::OpenGLImage(RenderDevice *_device)
	: Image(_device)
	, texture(-1)
	, normalTexture(-1)
	, aoTexture(-1)
	, w(0)
	, h(0) {
}

OpenGLImage::~OpenGLImage() {
	if ((int)texture != -1)
		glDeleteTextures(1, &texture);

	if ((int)normalTexture != -1)
		glDeleteTextures(1, &normalTexture);

	if ((int)aoTexture != -1)
		glDeleteTextures(1, &aoTexture);
}

int OpenGLImage::getWidth() const {
	glBindTexture(GL_TEXTURE_2D, texture);
	return w;
}

int OpenGLImage::getHeight() const {
	glBindTexture(GL_TEXTURE_2D, texture);
	return h;
}

void OpenGLImage::fillWithColor(const Color& color) {
	if ((int)texture == -1) return;

	int channels = 4;
	int bytes = getWidth() * getHeight() * channels;

	unsigned char* buffer = (unsigned char*)malloc(bytes);

	for(int i = 0; i < bytes; i+=4)
	{
		buffer[i] = static_cast<unsigned char>(color.r);
		buffer[i + 1] = static_cast<unsigned char>(color.g);
		buffer[i + 2] = static_cast<unsigned char>(color.b);
		buffer[i + 3] = static_cast<unsigned char>(color.a);
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWidth(), getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	int error = glGetError();
	if (error != GL_NO_ERROR)
		Utils::logInfo("Error while calling glTexImage2D(): %d", error);

	free(buffer);
}

/*
 * Set the pixel at (x, y) to the given value
 */
void OpenGLImage::drawPixel(int x, int y, const Color& color) {
	GLuint frameBuffer;
	GLint view[4];
	glGetIntegerv(GL_VIEWPORT, view);
	configureFrameBuffer(&frameBuffer, this->texture, w, h);

	GLfloat positionData[2];
	positionData[0] = 2.0f * static_cast<float>(x)/static_cast<float>(w) - 1.0f;
	positionData[1] = 2.0f * static_cast<float>(y)/static_cast<float>(h) - 1.0f;

	drawPrimitive(positionData, color, TYPE_PIXEL);

	disableFrameBuffer(&frameBuffer, view);
}

void OpenGLImage::drawLine(
	int x0,
	int y0,
	int x1,
	int y1,
	const Color& color
) {
	GLuint frameBuffer;
	GLint view[4];
	glGetIntegerv(GL_VIEWPORT, view);
	configureFrameBuffer(&frameBuffer, this->texture, w, h);

	GLfloat positionData[4];
	positionData[0] = 2.0f * static_cast<float>(x0)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[1] = 1.0f - 2.0f * static_cast<float>(y0)/static_cast<float>(settings->view_h);
	positionData[2] = 2.0f * static_cast<float>(x1)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[3] = 1.0f - 2.0f * static_cast<float>(y1)/static_cast<float>(settings->view_h);

	drawPrimitive(positionData, color, TYPE_LINE);

	disableFrameBuffer(&frameBuffer, view);
}

/**
 * Resizes an image
 * Deletes the original image and returns a pointer to the resized version
 */
Image* OpenGLImage::resize(int width, int height) {
	if((int)texture == -1 || width <= 0 || height <= 0)
		return NULL;
	// Resize is not needed for this renderer, it's done during rendering
	return this;
}

OpenGLRenderDevice::OpenGLRenderDevice()
	: window(NULL)
	, renderer(NULL)
	, titlebar_icon(NULL)
	, title(NULL)
{
#ifdef __ANDROID__
	//SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	Utils::logInfo("Using Render Device: OpenGLRenderDevice (hardware, SDL2/OpenGL)");
	settings->mouse_scaled = false;

	fullscreen = settings->fullscreen;
	hwsurface = settings->hwsurface;
	vsync = settings->vsync;
	texture_filter = settings->texture_filter;

	min_screen.x = eset->resolutions.min_screen_w;
	min_screen.y = eset->resolutions.min_screen_h;

	m_positionData[0] = -1.0f; m_positionData[1] = -1.0f;
	m_positionData[2] =  1.0f; m_positionData[3] = -1.0f;
	m_positionData[4] = -1.0f; m_positionData[5] =  1.0f;
	m_positionData[6] =  1.0f; m_positionData[7] =  1.0f;

	m_elementBufferData[0] = 0;
	m_elementBufferData[1] = 1;
	m_elementBufferData[2] = 2;
	m_elementBufferData[3] = 3;
}

int OpenGLRenderDevice::createContextInternal() {
	bool settings_changed = (fullscreen != settings->fullscreen || hwsurface != settings->hwsurface || vsync != settings->vsync || texture_filter != settings->texture_filter);

	Uint32 w_flags = 0;
	Uint32 r_flags = 0;
	int window_w = settings->screen_w;
	int window_h = settings->screen_h;

	if (settings->fullscreen) {
		w_flags = w_flags | SDL_WINDOW_FULLSCREEN_DESKTOP;

		// make the window the same size as the desktop resolution
		SDL_DisplayMode desktop;
		if (SDL_GetDesktopDisplayMode(0, &desktop) == 0) {
			window_w = desktop.w;
			window_h = desktop.h;
		}
	}
	else if (fullscreen && is_initialized) {
		// if the game was previously in fullscreen, resize the window when returning to windowed mode
		window_w = eset->resolutions.min_screen_w;
		window_h = eset->resolutions.min_screen_h;
		w_flags = w_flags | SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	}
	else {
		w_flags = w_flags | SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	}

	w_flags = w_flags | SDL_WINDOW_RESIZABLE;

	if (settings->hwsurface) {
		r_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
	}
	else {
		r_flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;
		settings->vsync = false; // can't have software mode & vsync at the same time
	}
	if (settings->vsync) r_flags = r_flags | SDL_RENDERER_PRESENTVSYNC;

	if (settings_changed || !is_initialized) {
		if (is_initialized) {
			// FIXME This fails and leaves us with a black screen
			destroyContext();

			// This partially fixes the issue, but some textures become glitched
			// is_initialized = false;
		}

		window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, w_flags);
		if (window) {
			renderer = SDL_GL_CreateContext(window);
			if (renderer) {
				if (settings->texture_filter && !eset->resolutions.ignore_texture_filter)
					SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
				else
					SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

				windowResize();
			}

			SDL_SetWindowMinimumSize(window, eset->resolutions.min_screen_w, eset->resolutions.min_screen_h);
			// setting minimum size might move the window, so set position again
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		if (!is_initialized)
		{
			init(&renderer);
			buildResources();
		}

		if (window && renderer) {
			if (!is_initialized) {
				// save the system gamma levels if we just created the window
				SDL_GetWindowGammaRamp(window, gamma_r, gamma_g, gamma_b);
				Utils::logInfo("RenderDevice: Window size is %dx%d", settings->screen_w, settings->screen_h);
			}

			fullscreen = settings->fullscreen;
			hwsurface = settings->hwsurface;
			vsync = settings->vsync;
			texture_filter = settings->texture_filter;
			is_initialized = true;

			Utils::logInfo("RenderDevice: Fullscreen=%d, Hardware surfaces=%d, Vsync=%d, Texture Filter=%d", fullscreen, hwsurface, vsync, texture_filter);

#if SDL_VERSION_ATLEAST(2, 0, 4)
			SDL_GetDisplayDPI(0, &ddpi, 0, 0);
			Utils::logInfo("RenderDevice: Display DPI is %f", ddpi);
#else
			Utils::logError("RenderDevice: The SDL version used to compile Flare does not support SDL_GetDisplayDPI(). The virtual_dpi setting will be ignored.");
#endif
		}
	}

	if (is_initialized) {
		// update minimum window size if it has changed
		if (min_screen.x != eset->resolutions.min_screen_w || min_screen.y != eset->resolutions.min_screen_h) {
			min_screen.x = eset->resolutions.min_screen_w;
			min_screen.y = eset->resolutions.min_screen_h;
			SDL_SetWindowMinimumSize(window, eset->resolutions.min_screen_w, eset->resolutions.min_screen_h);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		windowResize();

		// update title bar text and icon
		updateTitleBar();

		// load persistent resources
		delete icons;
		icons = new IconManager();
		delete curs;
		curs = new CursorManager();

		if (settings->change_gamma)
			setGamma(settings->gamma);
		else {
			resetGamma();
			settings->change_gamma = false;
			settings->gamma = 1.0;
		}
	}

	return (is_initialized ? 0 : -1);
}

void OpenGLRenderDevice::createContextError() {
	Utils::logError("SDLHardwareRenderDevice: createContext() failed: %s", SDL_GetError());
	Utils::logErrorDialog("SDLHardwareRenderDevice: createContext() failed: %s", SDL_GetError());
}

int OpenGLRenderDevice::render(Renderable& r, Rect& dest) {
	SDL_Rect src = r.src;
	SDL_Rect _dest = dest;

	m_offset[0] = 2.0f * static_cast<float>(_dest.x)/settings->view_w;
	m_offset[1] = 2.0f * static_cast<float>(_dest.y)/settings->view_h;

	m_offset[2] = static_cast<float>(src.w)/settings->view_w;
	m_offset[3] = static_cast<float>(src.h)/settings->view_h;

	int height = static_cast<OpenGLImage *>(r.image)->getHeight();
	int width = static_cast<OpenGLImage *>(r.image)->getWidth();

	m_texelOffset[0] = static_cast<float>(width) / static_cast<float>(src.w);
	m_texelOffset[1] = static_cast<float>(src.x) / static_cast<float>(width);

	m_texelOffset[2] = static_cast<float>(height) / static_cast<float>(src.h);
	m_texelOffset[3] = static_cast<float>(src.y) / static_cast<float>(height);

	GLuint texture = static_cast<OpenGLImage *>(r.image)->texture;
	GLuint normalTexture = static_cast<OpenGLImage *>(r.image)->normalTexture;
	GLuint aoTexture = static_cast<OpenGLImage *>(r.image)->aoTexture;

	if (texture == 0)
		return 1;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	bool bLightEnabled = ((int)normalTexture != -1) && ((int)aoTexture != -1);
	if (bLightEnabled)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normalTexture);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, aoTexture);
	}

	composeFrame(m_offset, m_texelOffset, bLightEnabled);

	return 0;
}

void* openShaderFile(const std::string& filename, GLint *length)
{
	std::string full_filename = mods->locate(filename);

	FILE *f = fopen(full_filename.c_str(), "r");
	void *buffer;

	if (!f) {
		Utils::logError("Unable to open shader file %s for reading", full_filename.c_str());
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	*length = static_cast<GLint>(ftell(f));
	fseek(f, 0, SEEK_SET);

	buffer = malloc(*length+1);
	*length = static_cast<GLint>(fread(buffer, 1, *length, f));
	fclose(f);
	((char*)buffer)[*length] = '\0';

	return buffer;
}

GLuint getShader(GLenum type, const std::string& filename)
{
	GLint length;
	GLchar *source = (char*)openShaderFile(filename, &length);
	GLuint shader;
	GLint shader_ok;

	if (!source)
		return 1;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	free(source);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
	if (!shader_ok)
	{
		Utils::logError("Failed to compile %s:", filename.c_str());

		GLchar glsl_log[BUFSIZ];
		glGetShaderInfoLog(shader, BUFSIZ, NULL, glsl_log);
		Utils::logError("%s", glsl_log);

		glDeleteShader(shader);
		return 1;
	}
	return shader;
}

GLuint createProgram(GLuint vertex_shader, GLuint fragment_shader)
{
	GLint program_ok;

	GLuint program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
	if (!program_ok)
	{
		Utils::logError("Failed to link shader program:");
		glDeleteProgram(program);
		return 1;
	}
	return program;
}

GLuint createBuffer(GLenum target, const void *buffer_data, GLsizei buffer_size)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(target, buffer);
	glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);
	return buffer;
}

int OpenGLRenderDevice::buildResources()
{
	m_vertex_buffer = createBuffer(GL_ARRAY_BUFFER, m_positionData, sizeof(m_positionData));
	m_element_buffer = createBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferData, sizeof(m_elementBufferData));

	m_vertex_shader = getShader(GL_VERTEX_SHADER, "shaders/vertex.glsl");
	if (m_vertex_shader == 0)
		return 1;

	m_fragment_shader = getShader(GL_FRAGMENT_SHADER, "shaders/fragment.glsl");
	if (m_fragment_shader == 0)
		return 1;

	m_program = createProgram(m_vertex_shader, m_fragment_shader);
	if (m_program == 0)
		return 1;

	attributes.position = glGetAttribLocation(m_program, "position");

	uniforms.texture = glGetUniformLocation(m_program, "texture");
	uniforms.offset = glGetUniformLocation(m_program, "offset");
	uniforms.texelOffset = glGetUniformLocation(m_program, "texelOffset");

	uniforms.normals = glGetUniformLocation(m_program, "normals");
	uniforms.aotexture = glGetUniformLocation(m_program, "aotexture");
	uniforms.light = glGetUniformLocation(m_program, "lightEnabled");

	uniforms.screenWidth = glGetUniformLocation(m_program, "screenWidth");
	uniforms.screenHeight = glGetUniformLocation(m_program, "screenHeight");

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	preparePrimitiveProgram();

	return 0;
}

int OpenGLRenderDevice::render(Sprite *r) {
	if (r == NULL) {
		return -1;
	}

	if ( !localToGlobal(r) ) {
		return -1;
	}

	// negative x and y clip causes weird stretching
	// adjust for that here
	if (m_clip.x < 0) {
		m_clip.w -= abs(m_clip.x);
		m_dest.x += abs(m_clip.x);
		m_clip.x = 0;
	}
	if (m_clip.y < 0) {
		m_clip.h -= abs(m_clip.y);
		m_dest.y += abs(m_clip.y);
		m_clip.y = 0;
	}

	m_dest.w = m_clip.w;
	m_dest.h = m_clip.h;

	SDL_Rect src = m_clip;
	SDL_Rect dest = m_dest;

	m_offset[0] = 2.0f * static_cast<float>(dest.x)/settings->view_w;
	m_offset[1] = 2.0f * static_cast<float>(dest.y)/settings->view_h;

	m_offset[2] = static_cast<float>(src.w)/settings->view_w;
	m_offset[3] = static_cast<float>(src.h)/settings->view_h;

	int height = r->getGraphics()->getHeight();
	int width = r->getGraphics()->getWidth();

	m_texelOffset[0] = static_cast<float>(width) / static_cast<float>(src.w);
	m_texelOffset[1] = static_cast<float>(src.x) / static_cast<float>(width);

	m_texelOffset[2] = static_cast<float>(height) / static_cast<float>(src.h);
	m_texelOffset[3] = static_cast<float>(src.y) / static_cast<float>(height);

	GLuint texture = static_cast<OpenGLImage *>(r->getGraphics())->texture;
	GLuint normalTexture = static_cast<OpenGLImage *>(r->getGraphics())->normalTexture;
	GLuint aoTexture = static_cast<OpenGLImage *>(r->getGraphics())->aoTexture;

	if (texture == 0)
		return 1;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	bool bLightEnabled = ((int)normalTexture != -1) && ((int)aoTexture != -1);
	if (bLightEnabled)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normalTexture);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, aoTexture);
	}

	composeFrame(m_offset, m_texelOffset , bLightEnabled);

	return 0;
}

void OpenGLRenderDevice::composeFrame(GLfloat* offset, GLfloat* texelOffset, bool withLight)
{
	glUseProgram(m_program);

	glUniform1i(uniforms.texture, 0);

	if (withLight)
	{
		glUniform1i(uniforms.light, 1);
		glUniform1i(uniforms.normals, 1);
		glUniform1i(uniforms.aotexture, 2);
	}
	else
	{
		glUniform1i(uniforms.light, 0);
	}
	glUniform1i(uniforms.screenWidth, settings->screen_w);
	glUniform1i(uniforms.screenHeight, settings->screen_h);

	glUniform4fv(uniforms.offset, 1, offset);
	glUniform4fv(uniforms.texelOffset, 1, texelOffset);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
	glVertexAttribPointer(
		attributes.position,
		2, GL_FLOAT, GL_FALSE,
		sizeof(GLfloat)*2, (void*)0
	);

	glEnableVertexAttribArray(attributes.position);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_element_buffer);
	glDrawElements(
		GL_TRIANGLE_STRIP,  /* mode */
		4,                  /* count */
		GL_UNSIGNED_SHORT,  /* type */
		(void*)0            /* element array buffer offset */
	);
	GLenum error = glGetError();
	if (error)
		Utils::logInfo("Error while calling glDrawElements(): %d", error);

	glDisableVertexAttribArray(attributes.position);
}

void configureFrameBuffer(GLuint* frameBuffer, GLuint frameTexture, int frame_w, int frame_h)
{
	glGenFramebuffers(1, frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *frameBuffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frameTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameTexture, 0);

	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (error != GL_FRAMEBUFFER_COMPLETE)
		Utils::logInfo("Failed to initialize framebuffer: %d", error);

	glViewport(0, 0, frame_w, frame_h);
}

void disableFrameBuffer(GLuint* frameBuffer, GLint *view_rect)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, frameBuffer);
	glViewport(view_rect[0], view_rect[1], view_rect[2], view_rect[3]);
}

int OpenGLRenderDevice::renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest) {
	if (!src_image || !dest_image) return -1;

	SDL_Rect _src = src;
	SDL_Rect _dest = dest;

	GLuint src_texture = static_cast<OpenGLImage *>(src_image)->texture;
	GLuint dst_texture = static_cast<OpenGLImage *>(dest_image)->texture;

	if (dst_texture == 0)
		return 1;

	int frameW = static_cast<OpenGLImage *>(dest_image)->getWidth();
	int frameH = static_cast<OpenGLImage *>(dest_image)->getHeight();

	GLint view[4];
	glGetIntegerv(GL_VIEWPORT, view);
	configureFrameBuffer(&m_frameBuffer, dst_texture, frameW, frameH);

	m_offset[0] = 2.0f * static_cast<float>(_dest.x)/static_cast<float>(frameW);
	m_offset[1] = 2.0f * static_cast<float>(_dest.y)/static_cast<float>(frameH);
	m_offset[2] = static_cast<float>(_src.w)/static_cast<float>(frameW);
	m_offset[3] = static_cast<float>(_src.h)/static_cast<float>(frameH);

	int height = static_cast<OpenGLImage *>(src_image)->getHeight();
	int width = static_cast<OpenGLImage *>(src_image)->getWidth();

	m_texelOffset[0] = static_cast<float>(width) / static_cast<float>(_src.w);
	m_texelOffset[1] = static_cast<float>(_src.x) / static_cast<float>(width);
	m_texelOffset[2] = static_cast<float>(height) / static_cast<float>(_src.h);
	m_texelOffset[3] = static_cast<float>(_src.y) / static_cast<float>(height);

	// NOTE: when drawing texture to texture you don't need to flip y coordinate in shader, so flip y coordinate back
	// NOTE: review this formula
	m_offset[1] = 2.0f - m_offset[1];
	m_offset[3] *= (-1.0f);

	if (src_texture == 0)
		return 1;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, src_texture);

	composeFrame(m_offset, m_texelOffset, false);

	disableFrameBuffer(&m_frameBuffer, view);

	return 0;
}

Image * OpenGLRenderDevice::renderTextToImage(FontStyle* font_style, const std::string& text, const Color& color, bool blended) {
	OpenGLImage *image = new OpenGLImage(this);
	if (!image) return NULL;

	SDL_Color _color = color;

	SDL_Surface* surface = NULL;
	if (blended)
		surface = TTF_RenderUTF8_Blended(static_cast<SDLFontStyle *>(font_style)->ttfont, text.c_str(), _color);
	else
		surface = TTF_RenderUTF8_Solid(static_cast<SDLFontStyle *>(font_style)->ttfont, text.c_str(), _color);

	if (surface)
	{
		image->w = surface->w;
		image->h = surface->h;
		glGenTextures(1, &(image->texture));

		glBindTexture(GL_TEXTURE_2D, image->texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
		int error = glGetError();
		if (error != GL_NO_ERROR)
			Utils::logInfo("Error while calling glTexImage2D(): %d", error);
		SDL_FreeSurface(surface);
	}

	if ((int)image->texture != -1)
		return image;

	delete image;
	return NULL;
}

void OpenGLRenderDevice::drawPixel(
	int x,
	int y,
	const Color& color
) {
	GLfloat positionData[2];
	positionData[0] = 2.0f * static_cast<float>(x)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[1] = 1.0f - 2.0f * static_cast<float>(y)/static_cast<float>(settings->view_h);

	drawPrimitive(positionData, color, TYPE_PIXEL);
}

void OpenGLRenderDevice::drawLine(
	int x0,
	int y0,
	int x1,
	int y1,
	const Color& color
) {
	GLfloat positionData[4];
	positionData[0] = 2.0f * static_cast<float>(x0)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[1] = 1.0f - 2.0f * static_cast<float>(y0)/static_cast<float>(settings->view_h);
	positionData[2] = 2.0f * static_cast<float>(x1)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[3] = 1.0f - 2.0f * static_cast<float>(y1)/static_cast<float>(settings->view_h);

	drawPrimitive(positionData, color, TYPE_LINE);
}

void OpenGLRenderDevice::drawRectangle(
	const Point& p0,
	const Point& p1,
	const Color& color
) {
	GLfloat positionData[8];
	positionData[0] = 2.0f * static_cast<float>(p0.x)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[1] = 1.0f - 2.0f * static_cast<float>(p0.y)/static_cast<float>(settings->view_h);
	positionData[2] = 2.0f * static_cast<float>(p0.x)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[3] = 1.0f - 2.0f * static_cast<float>(p1.y)/static_cast<float>(settings->view_h);
	positionData[4] = 2.0f * static_cast<float>(p1.x)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[5] = 1.0f - 2.0f * static_cast<float>(p1.y)/static_cast<float>(settings->view_h);
	positionData[6] = 2.0f * static_cast<float>(p1.x)/static_cast<float>(settings->view_w) - 1.0f;
	positionData[7] = 1.0f - 2.0f * static_cast<float>(p0.y)/static_cast<float>(settings->view_h);

	drawPrimitive(positionData, color, TYPE_RECT);
}

void OpenGLRenderDevice::blankScreen() {
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);

	return;
}

void OpenGLRenderDevice::commitFrame() {
	glFlush();
	SDL_GL_SwapWindow(window);

	return;
}

void OpenGLRenderDevice::destroyContext() {
	resetGamma();

	// we need to free all loaded graphics as they may be tied to the current context
	RenderDevice::cacheRemoveAll();
	reload_graphics = true;

	if (icons) {
		delete icons;
		icons = NULL;
	}
	if (curs) {
		delete curs;
		curs = NULL;
	}

	glDeleteBuffers(1, &m_vertex_buffer);
	glDeleteBuffers(1, &m_element_buffer);

	glDeleteProgram(m_program);
	glDeleteShader(m_vertex_shader);
	glDeleteShader(m_fragment_shader);

	glDeleteProgram(g_program);
	glDeleteShader(g_vertex_shader);
	glDeleteShader(g_fragment_shader);

	SDL_FreeSurface(titlebar_icon);
	titlebar_icon = NULL;

	SDL_GL_DeleteContext(renderer);
	renderer = NULL;

	SDL_DestroyWindow(window);
	window = NULL;

	if (title) {
		free(title);
		title = NULL;
	}

	return;
}

/**
 * create blank surface
 */
Image *OpenGLRenderDevice::createImage(int width, int height) {

	OpenGLImage *image = new OpenGLImage(this);

	if (!image)
		return NULL;

	image->w = width;
	image->h = height;
	int channels = 4;
	char* buffer = (char*)calloc(width * height * channels, sizeof(char));

	glGenTextures(1, &(image->texture));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	int error = glGetError();
	if (error != GL_NO_ERROR)
		Utils::logInfo("Error while calling glTexImage2D(): %d", error);

	free(buffer);

	return image;
}

void OpenGLRenderDevice::setGamma(float g) {
	Uint16 ramp[256];
	SDL_CalculateGammaRamp(g, ramp);
	SDL_SetWindowGammaRamp(window, ramp, ramp, ramp);
}

void OpenGLRenderDevice::resetGamma() {
	SDL_SetWindowGammaRamp(window, gamma_r, gamma_g, gamma_b);
}

void OpenGLRenderDevice::updateTitleBar() {
	if (title) free(title);
	title = NULL;
	if (titlebar_icon) SDL_FreeSurface(titlebar_icon);
	titlebar_icon = NULL;

	if (!window) return;

	title = Utils::strdup(msg->get(eset->misc.window_title));
	titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());

	if (title) SDL_SetWindowTitle(window, title);
	if (titlebar_icon) SDL_SetWindowIcon(window, titlebar_icon);
}

Image *OpenGLRenderDevice::loadImage(const std::string& filename, int error_type) {
	// lookup image in cache
	Image *img;
	img = cacheLookup(filename);
	if (img != NULL) return img;

	// load image
	OpenGLImage *image = NULL;
	SDL_Surface *cleanup = IMG_Load(mods->locate(filename).c_str());
	if(!cleanup) {
		if (error_type != ERROR_NONE)
			Utils::logError("OpenGLRenderDevice: Couldn't load image: '%s'. %s", filename.c_str(), IMG_GetError());

		if (error_type == ERROR_EXIT) {
			Utils::logErrorDialog("OpenGLRenderDevice: Couldn't load image: '%s'.\n%s", filename.c_str(), IMG_GetError());
			mods->resetModConfig();
			Utils::Exit(1);
		}
	}
	else {
		image = new OpenGLImage(this);
		SDL_Surface *surface = SDL_ConvertSurfaceFormat(cleanup, SDL_PIXELFORMAT_ABGR8888, 0);
		image->w = surface->w;
		image->h = surface->h;

		glGenTextures(1, &(image->texture));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, image->texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		int error = glGetError();
		if (error != GL_NO_ERROR)
			Utils::logInfo("Error while calling glTexImage2D(): %d", error);

		SDL_FreeSurface(surface);
		SDL_FreeSurface(cleanup);
	}

	// load normal texture
	std::string normalFileName = filename.substr(0, filename.size() - 4) + "_N.png";
	normalFileName = mods->locate(normalFileName);

	SDL_Surface *cleanupN = IMG_Load(normalFileName.c_str());
	if(cleanupN && cleanupN->w == image->w && cleanupN->h == image->h) {
		SDL_Surface *surfaceN = SDL_ConvertSurfaceFormat(cleanupN, SDL_PIXELFORMAT_ABGR8888, 0);

		glGenTextures(1, &(image->normalTexture));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, image->normalTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surfaceN->w, surfaceN->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surfaceN->pixels);
		int error = glGetError();
		if (error != GL_NO_ERROR)
			Utils::logInfo("Error while calling glTexImage2D(): %d", error);

		SDL_FreeSurface(surfaceN);
		SDL_FreeSurface(cleanupN);
	}
	else if (cleanupN) {
		Utils::logInfo("Skip loading image %s, it has wrong size", normalFileName.c_str());
		SDL_FreeSurface(cleanupN);
	}

	// load ambient occlusion texture
	std::string aoFileName = filename.substr(0, filename.size() - 4) + "_AO.png";
	aoFileName = mods->locate(aoFileName);

	SDL_Surface *cleanupAO = IMG_Load(aoFileName.c_str());
	if(cleanupAO && cleanupAO->w == image->w && cleanupAO->h == image->h) {
		SDL_Surface *surfaceAO = SDL_ConvertSurfaceFormat(cleanupAO, SDL_PIXELFORMAT_ABGR8888, 0);

		glGenTextures(1, &(image->aoTexture));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, image->aoTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surfaceAO->w, surfaceAO->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surfaceAO->pixels);
		int error = glGetError();
		if (error != GL_NO_ERROR)
			Utils::logInfo("Error while calling glTexImage2D(): %d", error);

		SDL_FreeSurface(surfaceAO);
		SDL_FreeSurface(cleanupAO);
	}
	else if (cleanupAO) {
		Utils::logInfo("Skip loading image %s, it has wrong size", aoFileName.c_str());
		SDL_FreeSurface(cleanupAO);
	}

	// store image to cache
	cacheStore(filename, image);
	return image;
}

void OpenGLRenderDevice::getWindowSize(short unsigned *screen_w, short unsigned *screen_h) {
	int w,h;
	SDL_GetWindowSize(window, &w, &h);
	*screen_w = static_cast<unsigned short>(w);
	*screen_h = static_cast<unsigned short>(h);
}

void OpenGLRenderDevice::windowResize() {
	windowResizeInternal();

	int offsetY = static_cast<int>(settings->screen_h - settings->view_h / settings->view_scaling) / 2;
	glViewport(0, offsetY, static_cast<GLint>(settings->view_w / settings->view_scaling), static_cast<GLint>(settings->view_h / settings->view_scaling));

	settings->updateScreenVars();
}
