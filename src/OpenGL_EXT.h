/*

Copyright © 2014 Igor Paliychuk
Copyright © 2014 Paul Wortmann

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

#ifndef OPENGL_EXT_H
#define OPENGL_EXT_H

#ifdef _WIN32
#include <SDL_opengl.h>
#define glGetProcAddressARB(x) wglGetProcAddress(x)
#elif __ANDROID__
#include <SDL_opengles2.h>
#define GL_BGRA GL_RGBA
#else
#include <GL/gl.h>
#include <GL/glx.h>
#define glGetProcAddressARB(x) (*glXGetProcAddressARB)((const GLubyte*)(x))
#endif


#ifdef _WIN32
extern PFNGLACTIVETEXTUREARBPROC         glWinActiveTexture;
#endif

#ifndef __ANDROID__
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC   glCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC     glFramebufferTexture2D;
extern PFNGLGENFRAMEBUFFERSPROC          glGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC       glDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC          glBindFramebuffer;

extern PFNGLGENVERTEXARRAYSPROC          glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC          glBindVertexArray;
extern PFNGLGENBUFFERSPROC               glGenBuffers;
extern PFNGLBINDBUFFERPROC               glBindBuffer;
extern PFNGLBUFFERDATAPROC               glBufferData;
extern PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray;
extern PFNGLCREATESHADERPROC             glCreateShader;
extern PFNGLSHADERSOURCEPROC             glShaderSource;
extern PFNGLCOMPILESHADERPROC            glCompileShader;
extern PFNGLGETSHADERIVPROC              glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC            glCreateProgram;
extern PFNGLATTACHSHADERPROC             glAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC       glBindAttribLocation;
extern PFNGLLINKPROGRAMPROC              glLinkProgram;
extern PFNGLGETPROGRAMIVPROC             glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC               glUseProgram;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLDETACHSHADERPROC             glDetachShader;
extern PFNGLDELETEPROGRAMPROC            glDeleteProgram;
extern PFNGLDELETESHADERPROC             glDeleteShader;
extern PFNGLDELETEBUFFERSPROC            glDeleteBuffers;
extern PFNGLDELETEVERTEXARRAYSPROC       glDeleteVertexArrays;
extern PFNGLGETSTRINGIPROC               glGetStringi;
extern PFNGLGETATTRIBLOCATIONPROC        glGetAttribLocation;
extern PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation;
extern PFNGLUNIFORM1IPROC                glUniform1i;
extern PFNGLUNIFORM4FVPROC               glUniform4fv;
#endif
extern void init(void **context);

#endif //OPENGL_EXT_H
