/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef DSQ_H
#define DSQ_H

#include "AquariaCompileConfig.h"
#include "Core.h"
#include "Quad.h"
#include "TileMgr.h"
#include "BitmapFont.h"
#include "ScreenTransition.h"
#include "Precacher.h"
#include "ScriptInterface.h"
#include "GameEnums.h"
#include "Mod.h"
#include "GameStructs.h"
#include "Continuity.h"
#include "SubtitlePlayer.h"
#include "StringBank.h"

#include "TTFFont.h"

#define AQUARIA_BUILD_MAPVIS

// Define this to save map visited data in a base64-encoded raw format.
// This can take much less space than the standard text format (as little
// as 10%), but WILL BE INCOMPATIBLE with previous builds of Aquaria --
// the visited data will be lost if the file is loaded into such a build.
// (Current builds will load either format regardless of whether or not
// this is defined.)
//#define AQUARIA_SAVE_MAPVIS_RAW

class Game;
class DebugFont;
class Avatar;
class Path;
class AquariaSaveSlot;
class AquariaMenuItem;

const float FRAME_TIME = 0.04f;

const int MAX_INGREDIENT_AMOUNT = 8;

const float MENUSELECTDELAY		= 0.2f;

const int VERSION_MAJOR			= 1;
const int VERSION_MINOR			= 1;
const int VERSION_REVISION		= 3;

const int VERSION_BETA			= 5;
const int VERSION_FC			= 3;
const int VERSION_GM			= 6;

// last entry is always NULL. added if is a little hack to ensure the scope of the iterator variable
#define FOR_ENTITIES(i) for (size_t i##_i = 0; dsq->entities[i##_i] != 0; ++i##_i) if (Entity **i = &dsq->entities[i##_i])


class AquariaScreenTransition : public ScreenTransition
{
public:
	void capture();
};

struct SFXLoops
{
	SFXLoops();
	void updateVolume();
	void stopAll();

	void *bg;
	void *bg2;
	void *roll;
	void *charge;
	void *shield;
	void *current;
	void *trip;
};

class GameplayVariables
{
public:
	int maxSlowSwimSpeed, maxSwimSpeed, maxBurstSpeed, maxDodgeSpeed, maxWallJumpSpeed, maxWallJumpBurstSpeed;
	int maxDreamWorldSpeed;
	int autoSaveTime, autoSaveFiles;
	int afterEffectsXDivs, afterEffectsYDivs;
	int frictionForce, maxSpringSpeed;
	int grabSpringPlantVelCap;
	float springTime;
	float zoomStop, zoomMove, zoomNaija;
	float jumpVelocityMod;
	float dodgeTime;
	float initialId, initialEgo, initialSuperEgo;
	int unusedFPSSmoothing;
	float defaultCameraLerpDelay;
	int maxOutOfWaterSpeed;
	float entityDamageTime, pushTime, avatarDamageTime;
	void load();
};

enum SaveSlotMode
{
	SSM_NONE = -1,
	SSM_SAVE = 0,
	SSM_LOAD = 1
};

extern GameplayVariables *vars;

#include "UserSettings.h"

enum NagType
{
	NAG_TOTITLE		= 0,
	NAG_QUIT		= 1
};

class DSQ : public Core
{
public:
	DSQ(const std::string& fileSystem, const std::string& extraDataDir);
	~DSQ();

	void init();
	void shutdown();

	void toggleBlackBars(bool on, float t=0);

	void setCursor(CursorType type);

	Quad *cursor, *cursorGlow, *cursorBlinker;
	Quad *overlay, *tfader, *overlay2, *overlay3, *overlayRed;
	Quad *sceneColorOverlay;
	Quad *bar_left, *bar_right, *bar_up, *bar_down;
	Quad *barFade_left, *barFade_right;

	CountedPtr<Texture> texCursor, texCursorSwim, texCursorBurst, texCursorSing, texCursorLook;

	void setBlackBarsColor(Vector color);

	void toggleFullscreen();

	void setTexturePointers();

	void fade(float alpha, float time);

	void nag(NagType type);

	void action(int id, int state, int source, InputDevice device);

	void title(bool fadeMusic);

	void cutsceneEffects(bool on);

	void delay(float dt); // active delay - game continues to run

	void newGame();

	bool isQuitFlag();

	void jiggleCursor();

	SFXLoops loops;
	SubtitlePlayer subtitlePlayer;

	void onPlayedVoice(const std::string &name);

	NagType nagType;

	int getEntityLayerToLayer(int layer);

	void addEntity(Entity *entity);
	void removeEntity(Entity *e);
	void clearEntities();

	std::vector<Entity*> entities;

	bool useFrameBuffer;
	Continuity continuity;
	GameplayVariables v;
	Emote emote;

	void playVisualEffect(int vfx, Vector position, Entity *target=0);
	void playNoEffect();

	AquariaScreenTransition *screenTransition;

	Precacher precacher;

	Entity *getFirstEntity();
	Entity *getNextEntity();

	bool modIsSelected;

	void shakeCamera(float mag, float time);
	Vector avStart;
	Entity *getEntityByName(const std::string &name);
	Entity *getEntityByNameNoCase(std::string name);

	void doSavePoint(const Vector &position);
	std::string getUserInputString(std::string label, std::string t="", bool allowNonLowerCase=false);
	bool onPickedSaveSlot(AquariaSaveSlot *slot);
	void doSaveSlotMenu(SaveSlotMode ssm, const Vector &position = Vector(0,0,0));
	void doModSelect();
	void doLoadMenu();
	void onExitSaveSlotMenu();
	ScriptInterface scriptInterface;
	bool runScript(const std::string &name, const std::string &func="", bool ignoremissing = false);
	bool runScriptNum(const std::string &name, const std::string &func="", float num=0);
	void collectScriptGarbage();

	ParticleEffect *spawnParticleEffect(const std::string &name, Vector position, float rot=0, float t=0, int layer=LR_PARTICLES, float follow=0);
	void spawnAllIngredients(const Vector &position);

	int getEntityTypeIndexByName(std::string s);
	void screenMessage(const std::string &msg);
	void _debugLog(const std::string &s) OVERRIDE;
	void toggleConsole();
	void toggleEffects();
	void debugMenu();

	void takeScreenshotKey();

	void toggleRenderCollisionShapes();

	void voice(const std::string &file, float volMod = -1);
	void voiceOnce(const std::string &file);
	void voiceInterupt(const std::string &file);
	void stopVoice();
	Vector getNoteColor(int note);
	int getRandNote();
	Vector getNoteVector(int note, float mag=1);
	void toggleCursor(bool v, float t = -1);

	bool isDeveloperKeys() const;
	bool canOpenEditor() const;

	void loadTileEffects();
	bool loadTileset(std::string pack, const unsigned char *usedIdx, size_t usedIdxLen);
	TileMgr tilemgr;

	bool playedVoice(const std::string &file);

	bool voiceOversEnabled;
	int recentSaveSlot;

	void playPositionalSfx(const std::string &name, const Vector &position, float freq=1.0f, float fadeOut=0, SoundHolder *holder = 0);

	void playMenuSelectSfx();

	InterpolatedVector gameSpeed;

private:
	InputDevice lastInputMode; // really don't want to expose this one
	std::vector<InputDevice> _inputModes; // index: FIXME ADD INFO
public:
	void setInputMode(InputDevice mode);
	InputDevice getInputMode() const;
	InputDevice getInputMode(int source) const;
	InputDevice getInputModeSafe(int source) const;
	bool useMouseInput() const;
	bool useJoystickInput() const;

	void rumble(float leftMotor, float rightMotor, float time, int source, InputDevice device);
	void vision(std::string folder, int num, bool ignoreMusic = false);

	// returns true if quit early
	bool run(float runTime = -1, bool skipRecurseCheck = false, bool canQuit = false); // same as Core::run() but with recursion check
	void watch(float t);

	UserSettings user, user_backup, user_bcontrol;

	void prepScreen(bool t);
	SaveSlotMode saveSlotMode;
	bool inModSelector;

	void createSaveSlots(SaveSlotMode ssm = SSM_NONE);
	void nextSaveSlotPage();
	void prevSaveSlotPage();
	void createSaveSlotPage();
	void clearSaveSlots(bool trans);
	void hideSaveSlots();
	void transitionSaveSlots();
	void hideSaveSlotCrap();

	void createModSelector();
	void clearModSelector();
	bool mountModPackage(const std::string&);
	bool modIsKnown(const std::string& name);
	void unloadMods();
	static void LoadModsCallback(const std::string &filename, void *param);
	static void LoadModPackagesCallback(const std::string &filename, void *param);

	AquariaSaveSlot *selectedSaveSlot;
	void setStory();

	bool disableMiniMapOnNoInput;

	std::string returnToScene;

	DebugFont *fpsText, *cmDebug;
	DebugFont *console;
	BitmapText *versionLabel;

	void setVersionLabelText();

	Mod mod;

	void loadStringBank();
	void loadMods();
	void applyPatches();
	void refreshResourcesForPatch(const std::string& name);
	void activatePatch(const std::string& name);
	void _applyPatch(const std::string& name);
	void disablePatch(const std::string& name);
	bool isPatchActive(const std::string& name);

	std::vector<ModEntry> modEntries;
	std::vector<std::string> activePatches;
	size_t selectedMod;
	ModSelectorScreen *modSelectorScr;

	void startSelectedMod();
	ModEntry* getSelectedModEntry();

	BitmapText *achievement_text;
	Quad *achievement_box;

	BitmapText *subtext;
	Quad *subbox;

	BmpFont font, smallFont, subsFont, goldFont, smallFontRed;
	TTFFont fontArialSmall, fontArialBig, fontArialSmallest;
	unsigned char *arialFontData;
	size_t arialFontDataSize;

	void loadFonts();


	void centerText(const std::string &text);
	void centerMessage(const std::string &text, float y=300, int type=0);

	void toggleVersionLabel(bool on);

	void onConfirmYes();
	void onConfirmNo();

	bool confirm(const std::string &text, const std::string &image="", bool ok=false, float countdown=0.0f);

	std::string particleBank1;
	std::string particleBank2;

	std::string shotBank1;
	std::string shotBank2;

	std::string getSaveDirectory();

	void clickRingEffect(Vector position, int type=0, Vector color=Vector(1,1,1), float ut=0);

	void bindInput();

	void setCutscene(bool on, bool canSkip=false);
	bool isInCutscene();
	bool isCutscenePaused();
	void pauseCutscene(bool on);
	bool canSkipCutscene();
	bool isSkippingCutscene();

	virtual void onBackgroundUpdate();

	void resetLayerPasses();
	bool isMiniMapCursorOkay();

protected:

	Quad *cutscene_bg;
	BitmapText *cutscene_text;
	BitmapText *cutscene_text2;

	bool cutscenePaused;
	bool inCutscene;
	bool _canSkipCutscene;
	bool skippingCutscene;

	std::vector<ActionInput*> almb, armb;

	void recreateBlackBars();

	bool watchQuitFlag, watchForQuit;

	int confirmDone;

	AquariaMenuItem *cancel, *arrowUp, *arrowDown;

	float noEffectTimer;
	void destroyFonts();

	void onReloadResources();

	void unloadDevice();
	void reloadDevice();

	void onSwitchScreenMode();
	virtual void onWindowResize(int w, int h);

	void onPlayVoice();
	void onStopVoice();

	Entity **iter;
	Quad *blackout;
	void updatepecue(float dt);
	std::vector<PECue> pecue;

	void onMouseInput();
	std::vector<std::string> voxQueue;

	std::vector<std::string> consoleLines;

	std::vector <AquariaSaveSlot*> saveSlots;

	void clearMenu(float t = 0.01f);
	std::vector <RenderObject*> menu;
	BitmapText *saveSlotPageCount;

	void updateSaveSlotPageCount();

	float shakeCameraTimer;
	float shakeCameraMag;

	void onUpdate(float dt);
	void onRender();

	void modifyDt(float &dt);

	virtual void onJoystickAdded(int deviceID);
	virtual void onJoystickRemoved(int instanceID);

	virtual void updateActionButtons();

public:
	void fixupJoysticks();
	void initActionButtons();
	void importActionButtons();

	TileRender *tileRenders[MAX_TILE_LAYERS];
};

extern DSQ *dsq;

#endif

