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
    NotEntities.h
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

IF(WIN32)
    SET(RC_DEFINES "" FORCE)
    SET(RC_INCLUDES "" FORCE)
    SET(RC_FLAGS "" FORCE)
endif(WIN32)
