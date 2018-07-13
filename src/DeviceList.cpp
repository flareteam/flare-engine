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

#include <stdio.h>
#include <string>

#include "DeviceList.h"

#include "MessageEngine.h"
#include "RenderDevice.h"

#include "SDLSoftwareRenderDevice.h"
#include "SDLHardwareRenderDevice.h"

#include "SDLFontEngine.h"
#include "SDLSoundManager.h"
#include "SDLInputState.h"

RenderDevice* getRenderDevice(const std::string& name) {
	// "sdl" is the default
	if (name != "") {
		if (name == "sdl") return new SDLSoftwareRenderDevice();
		else if (name == "sdl_hardware") return new SDLHardwareRenderDevice();
		else {
			Utils::logError("DeviceList: Render device '%s' not found. Falling back to the default.", name.c_str());
			return new SDLHardwareRenderDevice();
		}
	}
	else {
		return new SDLHardwareRenderDevice();
	}
}

void createRenderDeviceList(MessageEngine* msg, std::vector<std::string> &rd_name, std::vector<std::string> &rd_desc) {
	rd_name.clear();
	rd_desc.clear();

	rd_name.resize(2);
	rd_desc.resize(2);

	rd_name[0] = "sdl";
	rd_desc[0] = msg->get("SDL software renderer\n\nOften slower, but less likely to have issues.");

	rd_name[1] = "sdl_hardware";
	rd_desc[1] = msg->get("SDL hardware renderer\n\nThe default renderer that is often faster than the SDL software renderer.");
}

FontEngine* getFontEngine() {
	return new SDLFontEngine();
}

SoundManager* getSoundManager() {
	return new SDLSoundManager();
}

InputState* getInputManager() {
	return new SDLInputState();
}
