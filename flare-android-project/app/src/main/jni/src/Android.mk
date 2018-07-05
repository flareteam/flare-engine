LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
					$(LOCAL_PATH)/../SDL2_image \
					$(LOCAL_PATH)/../SDL2_mixer \
					$(LOCAL_PATH)/../SDL2_ttf

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	../../../../../../src/main.cpp \
	../../../../../../src/BehaviorAlly.cpp \
	../../../../../../src/Entity.cpp \
	../../../../../../src/Animation.cpp \
	../../../../../../src/AnimationManager.cpp \
	../../../../../../src/AnimationSet.cpp \
	../../../../../../src/AStarContainer.cpp \
	../../../../../../src/AStarNode.cpp \
	../../../../../../src/Avatar.cpp \
	../../../../../../src/BehaviorStandard.cpp \
	../../../../../../src/CampaignManager.cpp \
	../../../../../../src/CombatText.cpp \
	../../../../../../src/CursorManager.cpp \
	../../../../../../src/DeviceList.cpp \
	../../../../../../src/EffectManager.cpp \
	../../../../../../src/Enemy.cpp \
	../../../../../../src/EnemyBehavior.cpp \
	../../../../../../src/EnemyGroupManager.cpp \
	../../../../../../src/EnemyManager.cpp \
	../../../../../../src/EventManager.cpp \
	../../../../../../src/FileParser.cpp \
	../../../../../../src/FontEngine.cpp \
	../../../../../../src/GameSlotPreview.cpp \
	../../../../../../src/GameState.cpp \
	../../../../../../src/GameStateConfigBase.cpp \
	../../../../../../src/GameStateConfigDesktop.cpp \
	../../../../../../src/GameStateCutscene.cpp \
	../../../../../../src/GameStateTitle.cpp \
	../../../../../../src/GameStateLoad.cpp \
	../../../../../../src/GameStatePlay.cpp \
	../../../../../../src/GameStateNew.cpp \
	../../../../../../src/GameSwitcher.cpp \
	../../../../../../src/GetText.cpp \
	../../../../../../src/Hazard.cpp \
	../../../../../../src/HazardManager.cpp \
	../../../../../../src/IconManager.cpp \
	../../../../../../src/InputState.cpp \
	../../../../../../src/ItemManager.cpp \
	../../../../../../src/ItemStorage.cpp \
	../../../../../../src/Loot.cpp \
	../../../../../../src/LootManager.cpp \
	../../../../../../src/Map.cpp \
	../../../../../../src/MapParallax.cpp \
	../../../../../../src/MapCollision.cpp \
	../../../../../../src/MapRenderer.cpp \
	../../../../../../src/Menu.cpp \
	../../../../../../src/MenuActionBar.cpp \
	../../../../../../src/MenuActiveEffects.cpp \
	../../../../../../src/MenuBook.cpp \
	../../../../../../src/MenuCharacter.cpp \
	../../../../../../src/MenuConfirm.cpp \
	../../../../../../src/MenuDevConsole.cpp \
	../../../../../../src/MenuEnemy.cpp \
	../../../../../../src/MenuExit.cpp \
	../../../../../../src/MenuHUDLog.cpp \
	../../../../../../src/MenuInventory.cpp \
	../../../../../../src/MenuItemStorage.cpp \
	../../../../../../src/MenuLog.cpp \
	../../../../../../src/MenuManager.cpp \
	../../../../../../src/MenuMiniMap.cpp \
	../../../../../../src/MenuNPCActions.cpp \
	../../../../../../src/MenuNumPicker.cpp \
	../../../../../../src/MenuPowers.cpp \
	../../../../../../src/MenuStash.cpp \
	../../../../../../src/MenuStatBar.cpp \
	../../../../../../src/MenuTalker.cpp \
	../../../../../../src/MenuTouchControls.cpp \
	../../../../../../src/MenuVendor.cpp \
	../../../../../../src/MessageEngine.cpp \
	../../../../../../src/ModManager.cpp \
	../../../../../../src/NPC.cpp \
	../../../../../../src/NPCManager.cpp \
	../../../../../../src/PowerManager.cpp \
	../../../../../../src/QuestLog.cpp \
	../../../../../../src/RenderDevice.cpp \
	../../../../../../src/SaveLoad.cpp \
	../../../../../../src/SDLInputState.cpp \
	../../../../../../src/SDLHardwareRenderDevice.cpp \
	../../../../../../src/SDLSoftwareRenderDevice.cpp \
	../../../../../../src/SDLSoundManager.cpp \
	../../../../../../src/SDLFontEngine.cpp \
	../../../../../../src/Settings.cpp \
	../../../../../../src/SharedGameResources.cpp \
	../../../../../../src/SharedResources.cpp \
	../../../../../../src/StatBlock.cpp \
	../../../../../../src/Stats.cpp \
	../../../../../../src/Subtitles.cpp \
	../../../../../../src/TileSet.cpp \
	../../../../../../src/TooltipData.cpp \
	../../../../../../src/TooltipManager.cpp \
	../../../../../../src/Utils.cpp \
	../../../../../../src/UtilsDebug.cpp \
	../../../../../../src/UtilsFileSystem.cpp \
	../../../../../../src/UtilsParsing.cpp \
	../../../../../../src/Version.cpp \
	../../../../../../src/Widget.cpp \
	../../../../../../src/WidgetCheckBox.cpp \
	../../../../../../src/WidgetButton.cpp \
	../../../../../../src/WidgetInput.cpp \
 	../../../../../../src/WidgetLabel.cpp \
 	../../../../../../src/WidgetListBox.cpp \
 	../../../../../../src/WidgetLog.cpp \
	../../../../../../src/WidgetScrollBar.cpp \
	../../../../../../src/WidgetScrollBox.cpp \
	../../../../../../src/WidgetSlider.cpp \
	../../../../../../src/WidgetSlot.cpp \
 	../../../../../../src/WidgetTabControl.cpp \
	../../../../../../src/WidgetTooltip.cpp

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_ttf

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
