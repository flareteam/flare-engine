Project (Flare)
cmake_minimum_required (VERSION 3.5)

Set (PACKAGE "FLARE")
Set (VERSION "1.14")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# can't use add_compile_options here. Seems to have something to do with add_compile_options appending it after the "-O3 -DNDEBUG" flags
if (MINGW AND CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_CXX_FLAGS_RELEASE "-Wl,-subsystem,windows ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# Default definitions
if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
else ()
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-Wno-psabi)
	endif()

	add_compile_options(
		-Wall -Wextra -Wunused -Wshadow -Woverloaded-virtual -Wunreachable-code -Wconversion -Wno-sign-conversion -Wformat-security
		-fno-math-errno -fno-exceptions
		-std=c++98
	)
endif ()

add_compile_options(
	"$<$<CONFIG:RELEASE>:-O2>"
	"$<$<CONFIG:RELEASE>:-g0>"

	"$<$<CONFIG:RELWITHDEBINFO>:-O2>"
	"$<$<CONFIG:RELWITHDEBINFO>:-g>"

	"$<$<CONFIG:MINSIZEREL>:-Os>"
	"$<$<CONFIG:MINSIZEREL>:-g0>"

	"$<$<CONFIG:DEBUG>:-O0>"
	"$<$<CONFIG:DEBUG>:-g3>"
	"$<$<CONFIG:DEBUG>:-pg>"
)

# Not supported by CMake 3.12!
# Travis uses this version so we have to stick to modifying the strings...
# add_link_options(
# 	"$<$<CONFIG:DEBUG>:-pg>"
# )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-pg ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
	set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "-pg ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
	set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "-pg ${CMAKE_MODULE_LINKER_FLAGS_DEBUG}")
endif()

set(BINDIR  "games"             CACHE STRING "Directory from CMAKE_INSTALL_PREFIX where game executable will be installed.")
set(DATADIR "share/games/flare" CACHE STRING "Directory from CMAKE_INSTALL_PREFIX where game data files will be installed.")
set(MANDIR  "share/man"         CACHE STRING "Directory from CMAKE_INSTALL_PREFIX where manual pages will be installed.")

If(NOT IS_ABSOLUTE "${BINDIR}")
	set(FLARE_EXECUTABLE_PATH ${CMAKE_INSTALL_PREFIX}/${BINDIR}/flare)
Else(NOT IS_ABSOLUTE "${BINDIR}")
	set(FLARE_EXECUTABLE_PATH ${BINDIR}/flare)
EndIf(NOT IS_ABSOLUTE "${BINDIR}")

If(NOT IS_ABSOLUTE "${DATADIR}")
	add_definitions(-DDATA_INSTALL_DIR="${CMAKE_INSTALL_PREFIX}/${DATADIR}")
Else(NOT IS_ABSOLUTE "${DATADIR}")
	add_definitions(-DDATA_INSTALL_DIR="${DATADIR}")
EndIf(NOT IS_ABSOLUTE "${DATADIR}")


# desktop file
If(NOT IS_ABSOLUTE "${BINDIR}")
	set(FLARE_EXECUTABLE_PATH ${CMAKE_INSTALL_PREFIX}/${BINDIR}/flare)
Else(NOT IS_ABSOLUTE "${BINDIR}")
	set(FLARE_EXECUTABLE_PATH ${BINDIR}/flare)
EndIf(NOT IS_ABSOLUTE "${BINDIR}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/distribution/flare.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/flare.desktop")


# Detect missing dependencies
# First, search for SDL2 modules

Find_Package(SDL2)
If (NOT SDL2_FOUND)
	Message (FATAL_ERROR "Couldn't find SDL2 development files. On Debian-based systems (such as Ubuntu) you should install the 'libsdl2-dev' package.")
Else (NOT SDL2_FOUND)
	INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR})
EndIf (NOT SDL2_FOUND)

Find_Package(SDL2_image)
If (NOT SDL2IMAGE_FOUND)
	Message (FATAL_ERROR "Couldn't find SDL2-image development files. On Debian-based systems (such as Ubuntu) you should install the 'libsdl2-image-dev' package.")
Else (NOT SDL2IMAGE_FOUND)
	INCLUDE_DIRECTORIES(${SDL2IMAGE_INCLUDE_DIR})
EndIf (NOT SDL2IMAGE_FOUND)

Find_Package(SDL2_mixer)
If (NOT SDL2MIXER_FOUND)
	Message (FATAL_ERROR "Couldn't find SDL2-mixer development files. On Debian-based systems (such as Ubuntu) you should install the 'libsdl2-mixer-dev' package.")
Else (NOT SDL2MIXER_FOUND)
	INCLUDE_DIRECTORIES(${SDL2MIXER_INCLUDE_DIR})
EndIf (NOT SDL2MIXER_FOUND)

Find_Package(SDL2_ttf)
If (NOT SDL2TTF_FOUND)
	Message (FATAL_ERROR "Couldn't find SDL2-ttf development files. On Debian-based systems (such as Ubuntu) you should install the 'libsdl2-ttf-dev' package.")
Else (NOT SDL2TTF_FOUND)
	INCLUDE_DIRECTORIES(${SDL2TTF_INCLUDE_DIR})
EndIf (NOT SDL2TTF_FOUND)


# Sources

Set (FLARE_SOURCES
	./src/Animation.cpp
	./src/AnimationMedia.cpp
	./src/AnimationManager.cpp
	./src/AnimationSet.cpp
	./src/AStarContainer.cpp
	./src/AStarNode.cpp
	./src/Avatar.cpp
	./src/Camera.cpp
	./src/CampaignManager.cpp
	./src/CombatText.cpp
	./src/CursorManager.cpp
	./src/DeviceList.cpp
	./src/EffectManager.cpp
	./src/EnemyGroupManager.cpp
	./src/EngineSettings.cpp
	./src/Entity.cpp
	./src/EntityBehavior.cpp
	./src/EntityManager.cpp
	./src/EventManager.cpp
	./src/FileParser.cpp
	./src/FogOfWar.cpp
	./src/FontEngine.cpp
	./src/GameSlotPreview.cpp
	./src/GameState.cpp
	./src/GameStateConfig.cpp
	./src/GameStateCutscene.cpp
	./src/GameStateTitle.cpp
	./src/GameStateLoad.cpp
	./src/GameStatePlay.cpp
	./src/GameStateNew.cpp
	./src/GameSwitcher.cpp
	./src/GetText.cpp
	./src/Hazard.cpp
	./src/HazardManager.cpp
	./src/IconManager.cpp
	./src/InputState.cpp
	./src/ItemManager.cpp
	./src/ItemStorage.cpp
	./src/Loot.cpp
	./src/LootManager.cpp
	./src/Map.cpp
	./src/MapCollision.cpp
	./src/MapParallax.cpp
	./src/MapRenderer.cpp
	./src/MapSaver.cpp
	./src/Menu.cpp
	./src/MenuActionBar.cpp
	./src/MenuActiveEffects.cpp
	./src/MenuBook.cpp
	./src/MenuCharacter.cpp
	./src/MenuConfig.cpp
	./src/MenuConfirm.cpp
	./src/MenuDevConsole.cpp
	./src/MenuEnemy.cpp
	./src/MenuExit.cpp
	./src/MenuGameOver.cpp
	./src/MenuHUDLog.cpp
	./src/MenuInventory.cpp
	./src/MenuItemStorage.cpp
	./src/MenuLog.cpp
	./src/MenuManager.cpp
	./src/MenuMiniMap.cpp
	./src/MenuNumPicker.cpp
	./src/MenuPowers.cpp
	./src/MenuStash.cpp
	./src/MenuStatBar.cpp
	./src/MenuTalker.cpp
	./src/MenuTouchControls.cpp
	./src/MenuVendor.cpp
	./src/MessageEngine.cpp
	./src/ModManager.cpp
	./src/NPC.cpp
	./src/NPCManager.cpp
	./src/PowerManager.cpp
	./src/QuestLog.cpp
	./src/RenderDevice.cpp
	./src/SaveLoad.cpp
	./src/SDLInputState.cpp
	./src/SDLSoftwareRenderDevice.cpp
	./src/SDLSoundManager.cpp
	./src/SDLHardwareRenderDevice.cpp
	./src/SDLFontEngine.cpp
	./src/Settings.cpp
	./src/SharedGameResources.cpp
	./src/SharedResources.cpp
	./src/SoundManager.cpp
	./src/StatBlock.cpp
	./src/Stats.cpp
	./src/Subtitles.cpp
	./src/TileSet.cpp
	./src/TooltipData.cpp
	./src/TooltipManager.cpp
	./src/Utils.cpp
	./src/UtilsDebug.cpp
	./src/UtilsFileSystem.cpp
	./src/UtilsParsing.cpp
	./src/Version.cpp
	./src/Widget.cpp
	./src/WidgetCheckBox.cpp
	./src/WidgetButton.cpp
	./src/WidgetHorizontalList.cpp
	./src/WidgetInput.cpp
	./src/WidgetLabel.cpp
	./src/WidgetListBox.cpp
	./src/WidgetLog.cpp
	./src/WidgetScrollBar.cpp
	./src/WidgetScrollBox.cpp
	./src/WidgetSlider.cpp
	./src/WidgetSlot.cpp
	./src/WidgetTabControl.cpp
	./src/WidgetTooltip.cpp
	./src/XPScaling.cpp
	./src/main.cpp
)

Set (FLARE_HEADERS
	./src/Animation.h
	./src/AnimationMedia.h
	./src/AnimationManager.h
	./src/AnimationSet.h
	./src/AStarContainer.h
	./src/AStarNode.h
	./src/Avatar.h
	./src/Camera.h
	./src/CampaignManager.h
	./src/CombatText.h
	./src/CommonIncludes.h
	./src/CursorManager.h
	./src/DeviceList.h
	./src/EffectManager.h
	./src/EnemyGroupManager.h
	./src/EngineSettings.h
	./src/Entity.h
	./src/EntityBehavior.h
	./src/EntityManager.h
	./src/EventManager.h
	./src/FileParser.h
	./src/FogOfWar.h
	./src/FontEngine.h
	./src/GameSlotPreview.h
	./src/GameState.h
	./src/GameStateConfig.h
	./src/GameStateCutscene.h
	./src/GameStateTitle.h
	./src/GameStateLoad.h
	./src/GameStatePlay.h
	./src/GameStateNew.h
	./src/GameSwitcher.h
	./src/GetText.h
	./src/Hazard.h
	./src/HazardManager.h
	./src/IconManager.h
	./src/InputState.h
	./src/ItemManager.h
	./src/ItemStorage.h
	./src/Loot.h
	./src/LootManager.h
	./src/Map.h
	./src/MapCollision.h
	./src/MapParallax.h
	./src/MapRenderer.h
	./src/MapSaver.h
	./src/Menu.h
	./src/MenuActionBar.h
	./src/MenuActiveEffects.h
	./src/MenuBook.h
	./src/MenuCharacter.h
	./src/MenuConfig.h
	./src/MenuConfirm.h
	./src/MenuDevConsole.h
	./src/MenuEnemy.h
	./src/MenuExit.h
	./src/MenuGameOver.h
	./src/MenuHUDLog.h
	./src/MenuInventory.h
	./src/MenuItemStorage.h
	./src/MenuLog.h
	./src/MenuManager.h
	./src/MenuMiniMap.h
	./src/MenuNumPicker.h
	./src/MenuPowers.h
	./src/MenuStash.h
	./src/MenuStatBar.h
	./src/MenuTalker.h
	./src/MenuTouchControls.h
	./src/MenuVendor.h
	./src/MessageEngine.h
	./src/ModManager.h
	./src/NPC.h
	./src/NPCManager.h
	./src/PowerManager.h
	./src/QuestLog.h
	./src/RenderDevice.h
	./src/SDLInputState.h
	./src/SDLSoftwareRenderDevice.h
	./src/SDLSoundManager.h
	./src/SDLHardwareRenderDevice.h
	./src/SDLFontEngine.h
	./src/Settings.h
	./src/SharedGameResources.h
	./src/SharedResources.h
	./src/StatBlock.h
	./src/Stats.h
	./src/SoundManager.h
	./src/Subtitles.h
	./src/TileSet.h
	./src/TooltipData.h
	./src/TooltipManager.h
	./src/Utils.h
	./src/UtilsDebug.h
	./src/UtilsFileSystem.h
	./src/UtilsMath.h
	./src/UtilsParsing.h
	./src/Version.h
	./src/Widget.h
	./src/WidgetCheckBox.h
	./src/WidgetButton.h
	./src/WidgetHorizontalList.h
	./src/WidgetInput.h
	./src/WidgetLabel.h
	./src/WidgetListBox.h
	./src/WidgetLog.h
	./src/WidgetScrollBar.h
	./src/WidgetScrollBox.h
	./src/WidgetSlider.h
	./src/WidgetSlot.h
	./src/WidgetTabControl.h
	./src/WidgetTooltip.h
	./src/XPScaling.h
)

# Add icon and file info to executable for Windows systems
IF (WIN32)
	SET(FLARE_SOURCES
	${FLARE_SOURCES}
	./src/Flare.rc
	)
ENDIF (WIN32)

Add_Executable (flare ${FLARE_SOURCES} ${FLARE_HEADERS})

# libSDLMain comes with libSDL if needed on certain platforms
If (NOT SDL2MAIN_LIBRARY)
	Set (SDL2MAIN_LIBRARY "")
EndIf (NOT SDL2MAIN_LIBRARY)

Target_Link_Libraries (flare ${CMAKE_LD_FLAGS} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARY} ${SDL2MIXER_LIBRARY} ${SDL2TTF_LIBRARY} ${SDL2MAIN_LIBRARY})


# installing to the proper places
install(PROGRAMS
	${CMAKE_CURRENT_BINARY_DIR}/flare
	DESTINATION ${BINDIR})
install(DIRECTORY
	"${CMAKE_CURRENT_SOURCE_DIR}/mods"
	DESTINATION ${DATADIR})
install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/distribution/flare.man"
	DESTINATION ${MANDIR}/man6
	RENAME flare.6)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/flare.desktop"
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/distribution/flare_logo_icon.svg"
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps
	RENAME flare.svg)
