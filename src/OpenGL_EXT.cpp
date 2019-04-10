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

#include "OpenGL_EXT.h"

#ifdef _WIN32
PFNGLACTIVETEXTUREARBPROC         glWinActiveTexture         = NULL;
#endif

#ifndef __ANDROID__
PFNGLCHECKFRAMEBUFFERSTATUSPROC   glCheckFramebufferStatus   = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC     glFramebufferTexture2D     = NULL;
PFNGLGENFRAMEBUFFERSPROC          glGenFramebuffers          = NULL;
PFNGLDELETEFRAMEBUFFERSPROC       glDeleteFramebuffers       = NULL;
PFNGLBINDFRAMEBUFFERPROC          glBindFramebuffer          = NULL;

PFNGLGENVERTEXARRAYSPROC          glGenVertexArrays          = NULL;
PFNGLBINDVERTEXARRAYPROC          glBindVertexArray          = NULL;
PFNGLGENBUFFERSPROC               glGenBuffers               = NULL;
PFNGLBINDBUFFERPROC               glBindBuffer               = NULL;
PFNGLBUFFERDATAPROC               glBufferData               = NULL;
PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer      = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray  = NULL;
PFNGLCREATESHADERPROC             glCreateShader             = NULL;
PFNGLSHADERSOURCEPROC             glShaderSource             = NULL;
PFNGLCOMPILESHADERPROC            glCompileShader            = NULL;
PFNGLGETSHADERIVPROC              glGetShaderiv              = NULL;
PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog         = NULL;
PFNGLCREATEPROGRAMPROC            glCreateProgram            = NULL;
PFNGLATTACHSHADERPROC             glAttachShader             = NULL;
PFNGLBINDATTRIBLOCATIONPROC       glBindAttribLocation       = NULL;
PFNGLLINKPROGRAMPROC              glLinkProgram              = NULL;
PFNGLGETPROGRAMIVPROC             glGetProgramiv             = NULL;
PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog        = NULL;
PFNGLUSEPROGRAMPROC               glUseProgram               = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = NULL;
PFNGLDETACHSHADERPROC             glDetachShader             = NULL;
PFNGLDELETEPROGRAMPROC            glDeleteProgram            = NULL;
PFNGLDELETESHADERPROC             glDeleteShader             = NULL;
PFNGLDELETEBUFFERSPROC            glDeleteBuffers            = NULL;
PFNGLDELETEVERTEXARRAYSPROC       glDeleteVertexArrays       = NULL;
PFNGLGETSTRINGIPROC               glGetStringi               = NULL;
PFNGLGETATTRIBLOCATIONPROC        glGetAttribLocation        = NULL;
PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation       = NULL;
PFNGLUNIFORM1IPROC                glUniform1i                = NULL;
PFNGLUNIFORM4FVPROC               glUniform4fv               = NULL;
#endif

void init(void **context)
{
	if (context != NULL)
	{
		#ifdef _WIN32
		glWinActiveTexture         = (PFNGLACTIVETEXTUREARBPROC)         glGetProcAddressARB("glActiveTextureARB");
		#endif

		#ifndef __ANDROID__
		glCheckFramebufferStatus   = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)   glGetProcAddressARB("glCheckFramebufferStatus");
		glFramebufferTexture2D     = (PFNGLFRAMEBUFFERTEXTURE2DPROC)     glGetProcAddressARB("glFramebufferTexture2D");
		glGenFramebuffers          = (PFNGLGENFRAMEBUFFERSPROC)          glGetProcAddressARB("glGenFramebuffers");
		glDeleteFramebuffers       = (PFNGLDELETEFRAMEBUFFERSPROC)       glGetProcAddressARB("glDeleteFramebuffers");
		glBindFramebuffer          = (PFNGLBINDFRAMEBUFFERPROC)          glGetProcAddressARB("glBindFramebuffer");

		glGenVertexArrays          = (PFNGLGENVERTEXARRAYSPROC)          glGetProcAddressARB("glGenVertexArrays");
		glBindVertexArray          = (PFNGLBINDVERTEXARRAYPROC)          glGetProcAddressARB("glBindVertexArray");
		glGenBuffers               = (PFNGLGENBUFFERSPROC)               glGetProcAddressARB("glGenBuffers");
		glBindBuffer               = (PFNGLBINDBUFFERPROC)               glGetProcAddressARB("glBindBuffer");
		glBufferData               = (PFNGLBUFFERDATAPROC)               glGetProcAddressARB("glBufferData");
		glVertexAttribPointer      = (PFNGLVERTEXATTRIBPOINTERPROC)      glGetProcAddressARB("glVertexAttribPointer");
		glEnableVertexAttribArray  = (PFNGLENABLEVERTEXATTRIBARRAYPROC)  glGetProcAddressARB("glEnableVertexAttribArray");
		glCreateShader             = (PFNGLCREATESHADERPROC)             glGetProcAddressARB("glCreateShader");
		glShaderSource             = (PFNGLSHADERSOURCEPROC)             glGetProcAddressARB("glShaderSource");
		glCompileShader            = (PFNGLCOMPILESHADERPROC)            glGetProcAddressARB("glCompileShader");
		glGetShaderiv              = (PFNGLGETSHADERIVPROC)              glGetProcAddressARB("glGetShaderiv");
		glGetShaderInfoLog         = (PFNGLGETSHADERINFOLOGPROC)         glGetProcAddressARB("glGetShaderInfoLog");
		glCreateProgram            = (PFNGLCREATEPROGRAMPROC)            glGetProcAddressARB("glCreateProgram");
		glAttachShader             = (PFNGLATTACHSHADERPROC)             glGetProcAddressARB("glAttachShader");
		glBindAttribLocation       = (PFNGLBINDATTRIBLOCATIONPROC)       glGetProcAddressARB("glBindAttribLocation");
		glLinkProgram              = (PFNGLLINKPROGRAMPROC)              glGetProcAddressARB("glLinkProgram");
		glGetProgramiv             = (PFNGLGETPROGRAMIVPROC)             glGetProcAddressARB("glGetProgramiv");
		glGetProgramInfoLog        = (PFNGLGETPROGRAMINFOLOGPROC)        glGetProcAddressARB("glGetProgramInfoLog");
		glUseProgram               = (PFNGLUSEPROGRAMPROC)               glGetProcAddressARB("glUseProgram");
		glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) glGetProcAddressARB("glDisableVertexAttribArray");
		glDetachShader             = (PFNGLDETACHSHADERPROC)             glGetProcAddressARB("glDetachShader");
		glDeleteProgram            = (PFNGLDELETEPROGRAMPROC)            glGetProcAddressARB("glDeleteProgram");
		glDeleteShader             = (PFNGLDELETESHADERPROC)             glGetProcAddressARB("glDeleteShader");
		glDeleteBuffers            = (PFNGLDELETEBUFFERSPROC)            glGetProcAddressARB("glDeleteBuffers");
		glDeleteVertexArrays       = (PFNGLDELETEVERTEXARRAYSPROC)       glGetProcAddressARB("glDeleteVertexArrays");
		glGetStringi               = (PFNGLGETSTRINGIPROC)               glGetProcAddressARB("glGetStringi");
		glGetAttribLocation        = (PFNGLGETATTRIBLOCATIONPROC)        glGetProcAddressARB("glGetAttribLocation");
		glGetUniformLocation       = (PFNGLGETUNIFORMLOCATIONPROC)       glGetProcAddressARB("glGetUniformLocation");
		glUniform1i                = (PFNGLUNIFORM1IPROC)                glGetProcAddressARB("glUniform1i");
		glUniform4fv               = (PFNGLUNIFORM4FVPROC)               glGetProcAddressARB("glUniform4fv");
		#endif
	}
}
