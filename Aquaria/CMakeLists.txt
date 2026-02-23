# Main game source code for Aquaria, minus engine and other middleware...
SET(AQUARIA_SRCS
    AnimationEditor.cpp
    AnimationEditor.h
    AquariaComboBox.cpp
    AquariaCompileConfig.h
    AquariaMenuItem.cpp
    AquariaMenuItem.h
    AquariaSaveSlot.cpp
    Avatar.cpp
    Avatar.h
    Beam.cpp
    Beam.h
    BitBlotLogo.cpp
    CollideEntity.cpp
    CollideEntity.h
    Continuity.cpp
    Continuity.h
    Credits.cpp
    CurrentRender.cpp
    custom-fields.h
    Damage.h
    DSQ.cpp
    DSQ.h
    Element.cpp
    Element.h
    Elements.h
    Emote.cpp
    Entity.cpp
    Entity.h
    FlockEntity.cpp
    FlockEntity.h
    Game.cpp
    Game.h
    GameEnums.cpp
    GameEnums.h
    GameplayVariables.cpp
    GameStructs.cpp
    GameStructs.h
    GasCloud.cpp
    GasCloud.h
    GridRender.cpp
    GridRender.h
    Hair.cpp
    Hair.h
    InGameMenu.cpp
    InGameMenu.h
    Ingredient.cpp
    Ingredient.h
    Intro.cpp
    Intro.h
    Main.cpp
    ManaBall.cpp
    ManaBall.h
    MiniMapRender.cpp
    Mod.cpp
    Mod.h
    ModDownloader.cpp
    ModDownloader.h
    ModSelector.cpp
    ModSelector.h
    Network.cpp
    Network.h
    ParticleEditor.cpp
    Path.cpp
    Path.h
    PathFinding.cpp
    PathFinding.h
    PathRender.cpp
    RecipeMenuEntry.cpp
    RecipeMenuEntry.h
    SceneEditor.cpp
    SceneEditor.h
    SchoolFish.cpp
    SchoolFish.h
    Scriptable.cpp
    Scriptable.h
    ScriptedEntity.cpp
    ScriptedEntity.h
    ScriptInterface.cpp
    ScriptInterface.h
    Segmented.cpp
    Segmented.h
    SFXLoops.cpp
    Shot.cpp
    Shot.h
    Spore.cpp
    Spore.h
    States.cpp
    States.h
    StatsAndAchievements.cpp
    StatsAndAchievements.h
    SteamRender.cpp
    Strand.cpp
    StringBank_gen.h
    SubtitlePlayer.cpp
    SubtitlePlayer.h
    TileVector.h
    ToolTip.cpp
    ToolTip.h
    UserSettings.cpp
    UserSettings.h
    WaterSurfaceRender.cpp
    WaterSurfaceRender.h
    Web.cpp
    Web.h
    WorldMap.h
    WorldMapRender.cpp
    WorldMapTiles.cpp
)

set(EXETYPE)

option(AQUARIA_CONSOLE_WINDOW "Enable console output (always on in debug builds)" TRUE)
if(AQUARIA_CONSOLE_WINDOW OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    add_definitions(-DAQUARIA_ENABLE_CONSOLE_LOG)
endif()

IF(WIN32)
    if(NOT AQUARIA_CONSOLE_WINDOW)
        SET(EXETYPE WIN32)
    endif()
    SET(AQUARIA_SRCS ${AQUARIA_SRCS} aquaria.rc)
ENDIF()

ADD_EXECUTABLE(aquaria ${EXETYPE}
    ${AQUARIA_SRCS}
)

target_link_libraries(aquaria BBGE lua51)

if(APPLE)
    # Stage macOS binary to /build/macOS
    set(AQUARIA_RUNTIME_STAGE_DIR "${CMAKE_BINARY_DIR}/macOS")
    add_custom_command(TARGET aquaria POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${AQUARIA_RUNTIME_STAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:aquaria>" "${AQUARIA_RUNTIME_STAGE_DIR}/aquaria"
        VERBATIM
    )

    # If SDL2 is being used, stage the SDL2 dylib alongside the binary
    if(AQUARIA_USE_SDL2 AND SDL2_FOUND)
        set(AQUARIA_SDL2_DYLIB "")
        foreach(_aquaria_sdl2_item IN LISTS SDL2_LIBRARY)
            if(_aquaria_sdl2_item MATCHES "libSDL2[^/]*\\.dylib$")
                set(AQUARIA_SDL2_DYLIB "${_aquaria_sdl2_item}")
                break()
            endif()
        endforeach()

        if(NOT AQUARIA_SDL2_DYLIB)
            message(WARNING "SDL2 dylib was not detected from SDL2_LIBRARY, staging will skip SDL2 copy.")
        else()
            get_filename_component(AQUARIA_SDL2_DYLIB_REAL "${AQUARIA_SDL2_DYLIB}" REALPATH)
            get_filename_component(AQUARIA_SDL2_DYLIB_NAME "${AQUARIA_SDL2_DYLIB_REAL}" NAME)

            add_custom_command(TARGET aquaria POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${AQUARIA_SDL2_DYLIB_REAL}" "${AQUARIA_RUNTIME_STAGE_DIR}/${AQUARIA_SDL2_DYLIB_NAME}"
                COMMAND /bin/sh -c "set -e; old_path=`otool -L \"${AQUARIA_RUNTIME_STAGE_DIR}/aquaria\" | awk '/SDL2.*dylib/{print \\$1; exit}'`; [ -n \"$old_path\" ] && install_name_tool -change \"$old_path\" \"@executable_path/../Frameworks/${AQUARIA_SDL2_DYLIB_NAME}\" \"${AQUARIA_RUNTIME_STAGE_DIR}/aquaria\""
                VERBATIM
            )
        endif()
    endif()
endif()

IF(WIN32)
    SET(RC_DEFINES "" FORCE)
    SET(RC_INCLUDES "" FORCE)
    SET(RC_FLAGS "" FORCE)
endif(WIN32)
