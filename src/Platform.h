/*
Copyright © 2016 Justin Jacobs

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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

#define CONFIG_MENU_TYPE_BASE 0
#define CONFIG_MENU_TYPE_DESKTOP 1

struct PlatformOptions_t {
	bool has_exit_button;
	bool is_mobile_device;
	unsigned char config_menu_type;
	std::string default_renderer;
};

extern struct PlatformOptions_t PlatformOptions;

void PlatformInit(struct PlatformOptions_t *options);
void PlatformSetPaths();
void PlatformSetExitEventFilter();
bool PlatformDirCreate(const std::string& path);
bool PlatformDirRemove(const std::string& path);

#endif
