# Main game source code for Aquaria, minus engine and other middleware...
SET(AQUARIA_SRCS
    AnimationEditor.cpp
    AquariaComboBox.cpp
    AquariaMenuItem.cpp
    AquariaSaveSlot.cpp
    Avatar.cpp
    Beam.cpp
    BitBlotLogo.cpp
    CollideEntity.cpp
    Continuity.cpp
    Credits.cpp
    CurrentRender.cpp
    Demo.cpp
    DSQ.cpp
    Element.cpp
    Emote.cpp
    Entity.cpp
    FlockEntity.cpp
    Game.cpp
    GameStructs.cpp
    GameplayVariables.cpp
    GasCloud.cpp
    GridRender.cpp
    Hair.cpp
    Ingredient.cpp
    InGameMenu.cpp
    Intro.cpp
    Logo.cpp
    Main.cpp
    ManaBall.cpp
    MiniMapRender.cpp
    Mod.cpp
    ModSelector.cpp
    ModDownloader.cpp
    Network.cpp
    ParticleEditor.cpp
    Path.cpp
    PathFinding.cpp
    PathRender.cpp
    RecipeMenuEntry.cpp
    SceneEditor.cpp
    SchoolFish.cpp
    Scriptable.cpp
    ScriptedEntity.cpp
    ScriptInterface.cpp
    Segmented.cpp
    SFXLoops.cpp
    Shot.cpp
    Spore.cpp
    States.cpp
    StatsAndAchievements.cpp
    SteamRender.cpp
    Strand.cpp
    StringBank_gen.h
    SubtitlePlayer.cpp
    ToolTip.cpp
    UserSettings.cpp
    WaterSurfaceRender.cpp
    Web.cpp
    WorldMapRender.cpp
    WorldMapTiles.cpp
)

set(EXETYPE)

IF(WIN32)
    SET(EXETYPE WIN32)
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
