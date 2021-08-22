/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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

/**
 * class FogOfWar
 *
 * Contains logic and rendering routines for fog of war.
 */

#include "Avatar.h"
#include "EngineSettings.h"
#include "FogOfWar.h"
#include "FontEngine.h"
#include "MapRenderer.h"
#include "Menu.h"
#include "MenuManager.h"
#include "MenuMiniMap.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"

const unsigned short FogOfWar::CIRCLE_MASK[NUM_FOW_RADII][FOW_MAX_RADIUS_LENGTH * FOW_MAX_RADIUS_LENGTH] = {
	// radius 3
	{
		fow_all,fow_all,fow_CNW,fow_NNN,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_SSS,fow_CSE,fow_all,fow_all,
	},
	// radius 4
	{
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
	},
	// radius 5
	{
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
	},
	// radius 6
	{
		fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,
	},
	// radius 7
	{
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,
	},
	// radius 8
	{
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
	},
	// radius 9
	{
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,
		fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
	},
		// radius 10
	{
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_NNN,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CNW,fow_NNN,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_NNN,fow_CNE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,fow_all,
		fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,
		fow_all,fow_CNW,fow_JNW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JNE,fow_CNE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,
		fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,
		fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,
		fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_WWW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_EEE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_CSW,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_CSE,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_JSW,fow_non,fow_non,fow_non,fow_JSE,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_CSW,fow_SSS,fow_SSS,fow_SSS,fow_CSE,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
		fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,fow_all,
	}
};

FogOfWar::FogOfWar()
	: dark_layer_id(0)
	, fog_layer_id(0)
	, tileset_dark("tilesetdefs/tileset_fow_dark.txt")
	, tileset_fog("tilesetdefs/tileset_fow_fog.txt")
	, bounds(0,0,0,0)
	, color_sight(255,255,255)
	, color_visited(128,128,128)
	, color_hidden(0,0,0)
	, update_minimap(true) {

	for (int i=0; i<512; i++) {
		std::stringstream ss;
		ss << i;

		Image* tile_num = render_device->createImage(64,32);
		font->render(ss.str(), 16, 5, FontEngine::JUSTIFY_LEFT, tile_num, 0, Color(255,0,0,255));
		Sprite* tile_spr = tile_num->createSprite();

		tile_numbers.resize(tile_numbers.size()+1, tile_spr);
	}
}

int FogOfWar::load() {
	tset_dark.load(tileset_dark);
	tset_fog.load(tileset_fog);
	return 0;
}

void FogOfWar::logic() {
	updateTiles();
	if (update_minimap) {
		calcMiniBoundaries();
		menu->mini->update(&mapr->collider, &bounds);
		update_minimap = false;
	}
}

void FogOfWar::handleIntramapTeleport() {
	calcBoundaries();
	
	for (int x = bounds.x; x <= bounds.w; x++) {
		for (int y = bounds.y; y <= bounds.h; y++) {
			if (x>=0 && y>=0 && x < mapr->w && y < mapr->h) {
				mapr->layers[fog_layer_id][x][y] = TILE_HIDDEN;
			}
		}
	}
}

Color FogOfWar::getTileColorMod(const int_fast16_t x, const int_fast16_t y) {
	if (mapr->layers[dark_layer_id][x][y] == 0 && mapr->layers[fog_layer_id][x][y] > 0)
		return color_visited;
	else if (mapr->layers[dark_layer_id][x][y] > 0)
		return color_hidden;
	else 
		return color_sight;
}

void FogOfWar::calcBoundaries() {
	bounds.x = static_cast<short>(pc->stats.pos.x)-pc->sight;
	bounds.y = static_cast<short>(pc->stats.pos.y)-pc->sight;
	bounds.w = static_cast<short>(pc->stats.pos.x)+pc->sight;
	bounds.h = static_cast<short>(pc->stats.pos.y)+pc->sight;
}

void FogOfWar::calcMiniBoundaries() {
	bounds.x = static_cast<short>(pc->stats.pos.x)-pc->sight;
	bounds.y = static_cast<short>(pc->stats.pos.y)-pc->sight;
	bounds.w = static_cast<short>(pc->stats.pos.x)+pc->sight;
	bounds.h = static_cast<short>(pc->stats.pos.y)+pc->sight;

	if (bounds.x < 0) bounds.x = 0;
	if (bounds.y < 0) bounds.y = 0;
	if (bounds.w > mapr->w) bounds.w = mapr->w;
	if (bounds.h > mapr->h) bounds.h = mapr->h;
}

void FogOfWar::updateTiles() {
	calcBoundaries();
	int radius = pc->sight;
	const unsigned short * mask = &CIRCLE_MASK[radius - FOW_RADIUS_MIN][0];

	for (int x = bounds.x; x <= bounds.w; x++) {
		for (int y = bounds.y; y <= bounds.h; y++) {
			if (x>=0 && y>=0 && x < mapr->w && y < mapr->h) {
				unsigned short prev_tile = mapr->layers[dark_layer_id][x][y];
				mapr->layers[dark_layer_id][x][y] &= *mask;
				mapr->layers[fog_layer_id][x][y] = *mask;
				if ((prev_tile == TILE_HIDDEN) && prev_tile != mapr->layers[dark_layer_id][x][y]) {
					update_minimap = true;
				}
			}
			mask++;
		}
	}
}

FogOfWar::~FogOfWar() {
}
