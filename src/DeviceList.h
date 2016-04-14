/*
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

#ifndef DEVICELIST_H
#define DEVICELIST_H

#include <string>

#include "RenderDevice.h"

class FontEngine;
class SoundManager;
class InputState;

RenderDevice* getRenderDevice(const std::string& name);

FontEngine* getFontEngine();
SoundManager* getSoundManager();
InputState* getInputManager();

#endif
