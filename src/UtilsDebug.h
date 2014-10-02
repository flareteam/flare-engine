/*
Copyright © 2012 David Bariod

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

#ifndef UTILS_DEBUG_H
#define UTILS_DEBUG_H

extern std::ostream & operator<< (std::ostream &, const SDL_Event &);
#if SDL_VERSION_ATLEAST(2,0,0)
extern std::ostream & operator<< (std::ostream &, const SDL_WindowEvent &);
#else
extern std::ostream & operator<< (std::ostream &, const SDL_ActiveEvent &);
#endif
extern std::ostream & operator<< (std::ostream &, const SDL_KeyboardEvent &);
#if SDL_VERSION_ATLEAST(2,0,0)
extern std::ostream & operator<< (std::ostream &, const SDL_Keysym &);
#else
extern std::ostream & operator<< (std::ostream &, const SDL_keysym &);
#endif
extern std::ostream & operator<< (std::ostream &, const SDL_MouseMotionEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_MouseButtonEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyAxisEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyBallEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyHatEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyButtonEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_QuitEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_SysWMEvent &);
#if SDL_VERSION_ATLEAST(2,0,0)
#else
extern std::ostream & operator<< (std::ostream &, const SDL_ResizeEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_ExposeEvent &);
#endif
extern std::ostream & operator<< (std::ostream &, const Rect &);
extern std::ostream & operator<< (std::ostream &, const Point &);

#endif
