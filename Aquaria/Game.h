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
#ifndef GAME_H
#define GAME_H

#include "../BBGE/DebugFont.h"

#include "DSQ.h"
#include "AquariaMenuItem.h"
#include "ScriptedEntity.h"
#include "TileVector.h"
#include "SceneEditor.h"
#include "Tileset.h"

#include <tinyxml2.h>
using namespace tinyxml2;


class Avatar;
class Gradient;
class CurrentRender;
class SteamRender;
class SongLineRender;
class AutoMap;
class DebugButton;
class WorldMapRender;
class Shot;
class InGameMenu;

// FIXME: this should be made dynamic, or at least a power of 2
const int MAX_GRID = 2222;

const char CHAR_DOWN		= 'd';
const char CHAR_UP			= 'u';
const char CHAR_LEFT		= 'l';
const char CHAR_RIGHT		= 'r';


const float MIN_SIZE = 0.1f;



class GridRender;
class MiniMapRender;
class WaterSurfaceRender;
class ToolTip;
class Ingredient;
class ManaBall;
class Beam;

#include "Path.h"


// impl is in Minimap.cpp
struct MinimapIcon
{
	MinimapIcon();
	bool setTexture(std::string);
	void update(float dt);
	CountedPtr<Texture> tex;
	InterpolatedVector color, alpha, size;
	float throbMult;
	bool scaleWithDistance;

	static const Vector defaultSize;
};

typedef std::list<Ingredient*> Ingredients;

class ObsRow
{
public:
	inline ObsRow(unsigned tx, unsigned ty, unsigned len)
		: tx(tx), ty(ty), len(len) {}
	const unsigned tx, ty, len;
};

enum FlagCheckType
{
	NO_TYPE	=-1,
	AND		=0,
	OR		=1
};

class EntityClass
{
public:
	EntityClass(std::string name, bool script=false, int idx=-1, std::string prevGfx="", float prevScale=1)
		: name(name), prevScale(prevScale), prevGfx(prevGfx), script(script), idx(idx) {}
	std::string name;
	float prevScale;
	std::string prevGfx;
	bool script;
	int idx;
};

typedef std::vector<Element*> ElementUpdateList;

struct EntitySaveData
{
public:
	EntitySaveData() : id(0), x(0), y(0), rot(0), idx(0) {}
	EntitySaveData(int id, int x, int y, int rot, int idx, const std::string &name) : name(name), id(id), x(x), y(y), rot(rot), idx(idx) {}
	std::string name;
	int id, x, y, rot, idx;
};

class Game : public StateObject
{
public:
	Game();
	~Game();
	void applyState();
	void removeState();
	void update(float dt);


	Avatar *avatar;
	Entity *li;

	ObsType getGrid(const TileVector &tile) const;
	ObsType getGridRaw(const TileVector &tile) const;
	unsigned char *getGridColumn(int tileX);
	void setGrid(const TileVector &tile, ObsType v);
	void addGrid(const TileVector &tile, ObsType v);
	bool isObstructed(const TileVector &tile, int t = OT_BLOCKING) const;
	bool isObstructedRaw(const TileVector &tile, int t) const;
	void trimGrid();
	void dilateGrid(unsigned int radius, ObsType test, ObsType set, ObsType allowOverwrite);

	void transitionToScene(std::string scene);
	bool loadScene(std::string scene);

	void clearGrid(int v = 0);
	void clearDynamicGrid(unsigned char maskbyte = OT_MASK_BLACK);

	void toggleWorldMap();

	void action(int id, int state, int source, InputDevice device);

	InGameMenu *getInGameMenu() { return themenu; }

	// pass usedIdx == NULL to preload all textures from tileset
	// pass usedIdx != NULL to preload only textures where usedIdx[i] != 0
	bool loadElementTemplates(std::string pack, const unsigned char *usedIdx, size_t usedIdxLen);
	Element* createElement(size_t etidx, Vector position, size_t bgLayer=0, RenderObject *copy=0, ElementTemplate *et=0);

	void updateParticlePause();

	void reconstructGrid(bool force=false);
	void reconstructEntityGrid();

	void registerSporeDrop(const Vector &pos, int t);

	bool collideCircleWithGrid(const Vector& position, float r);

	bool collideHairVsCircle(Entity *a, int num, const Vector &pos2, float radius, float perc=0, int *colSegment=0);

	bool collideCircleVsCircle(Entity *a, Entity *b);
	Bone *collideSkeletalVsCircle(Entity *skeletal, CollideQuad *circle);
	Bone *collideSkeletalVsLine(Entity *skeletal, Vector start, Vector end, float radius);
	bool collideCircleVsLine(CollideQuad *r, Vector start, Vector end, float radius);
	bool collideCircleVsLineAngle(CollideQuad *r, float angle, float startLen, float endLen, float radius, Vector basePos);
	Bone *collideSkeletalVsCircle(Entity *skeletal, Vector pos, float radius);
	void handleShotCollisions(Entity *e, bool hasShield=false);
	void handleShotCollisionsSkeletal(Entity *e);
	void handleShotCollisionsHair(Entity *e, int num = 0, float perc = 0);

	Tileset tileset;
	std::string sceneName, sceneDisplayName;

	ElementTemplate *getElementTemplateByIdx(size_t idx);

	bool saveScene(std::string scene);

	void postInitEntities();
	EntityClass *getEntityClassForEntityType(const std::string &type);

	InterpolatedVector sceneColor, sceneColor2, sceneColor3;

	Vector getCameraPositionFor(const Vector &vec);

	bool isActive();

	bool isPaused();
	void togglePause(bool v);

	Ingredient* spawnIngredient(const std::string &ing, const Vector &pos, int times=1, int out=0);
	void spawnIngredientFromEntity(Entity *ent, IngredientData *data);

	Ingredient *getNearestIngredient(const Vector &pos, float radius);
	Entity *getNearestEntity(const Vector &pos, float radius, Entity *ignore = 0, EntityType et=ET_NOTYPE, DamageType dt=DT_NONE, int lrStart=-1, int lrEnd=-1);

	Script *cookingScript;

	void spawnManaBall(Vector pos, float a);
	bool updateMusic();
	std::string overrideMusic;
	void resetFromTitle();

	float maxLookDistance;

	XMLDocument *saveFile;

	Vector positionToAvatar;

	Vector getWallNormal(Vector pos, int sampleArea = 5, int obs = -1);

	EntitySaveData *getEntitySaveDataForEntity(Entity *e);
	Entity *createEntityOnMap(const EntitySaveData& sav); // when loading from save (saved to map). Caller must postInit().
	Entity *createEntityOnMap(const char *type, Vector pos); // when spawning in the editor (saved to map). Caller must postInit().
	Entity *createEntityTemp(const char *type, Vector pos, bool doPostInit); // when spawning via script (not saved to map). Never does postInit() if we're currently loading a map.
	void establishEntity(Entity *e, int id, Vector startPos);
	Entity *getEntityByID(int id) const;
	int findUnusedEntityID(bool temporary) const; // pass temporary=true for script-spawned entities, false for entities that are spawned on the map
	void setCameraFollow(RenderObject *r);
	void setCameraFollowEntity(Entity *e);

	bool removeEntityAtCursor();
	void toggleOverrideZoom(bool on);

	bool useWaterLevel;
	InterpolatedVector waterLevel;
	int saveWaterLevel;
	void warpCameraTo(RenderObject *r);

	void addObsRow(unsigned tx, unsigned ty, unsigned len);
	void clearObsRows();
	Entity *getEntityAtCursor();
	Vector cameraMin, cameraMax;
	bool removeEntity(Entity *e);
	void removeIngredient(Ingredient *i);
	void bindIngredients();

protected:
	std::vector<Path*> paths;
	Path *firstPathOfType[PATH_MAX];
public:
	void addPath(Path *p);
	void removePath(size_t idx);
	void clearPaths();
	size_t getNumPaths() const {return paths.size();}
	Path *getPath(size_t idx) const {return paths[idx];}
	Path *getFirstPathOfType(PathType type) const {return firstPathOfType[type];}
	Path *getPathByName(std::string name);
	size_t getIndexOfPath(Path *p);
	Path *getPathAtCursor();
	Path *getScriptedPathAtCursor(bool withAct=false);
	Path *getNearestPath(const Vector &pos, const std::string &name="", const Path *ignore=0);
	Path *getNearestPath(const Vector &pos, PathType pathType=PATH_NONE);
	Path *getNearestPath(Path *p, std::string name);

	Path *getWaterbubbleAt(const Vector& pos, float rad = 0) const;
	UnderWaterResult isUnderWater(const Vector& pos, float rad = 0) const;

	SceneEditor sceneEditor;
	bool isSceneEditorActive() {return sceneEditor.isOn();}

	bool isInGameMenu();

	typedef std::vector<EntityClass> EntityTypeList;
	EntityTypeList entityTypeList;
	void loadEntityTypeList();
	std::vector<EntitySaveData> entitySaveData;
	int getIdxForEntityType(std::string type);

	void setCameraFollow(Vector *position);
	Shot *fireShot(const std::string &bankShot, Entity *firer, Entity *target=0, const Vector &pos=Vector(0,0,0), const Vector &aim=Vector(0,0,0), bool playSfx=true);
	void playBurstSound(bool wallJump=false);
	void toggleMiniMapRender();
	void toggleMiniMapRender(int v);
	bool runGameOverScript;


	void setTimerTextAlpha(float a, float t);
	void setTimerText(float time);

	void generateCollisionMask(Bone *q, float overrideCollideRadius=0);
	std::string fromScene, toNode;
	int toFlip;
	char fromWarpType;
	void warpToSceneFromNode(Path *p);
	Vector fromPosition;

	Quad *damageSprite;

	void toggleDamageSprite(bool on);

	ObsType lastCollideTileType;

	void fillGridFromQuad(Quad *q, ObsType ot=OT_INVISIBLEIN, bool trim=true);

	bool isDamageTypeAvatar(DamageType dt);
	bool isDamageTypeEnemy(DamageType dt);
	bool isEntityCollideWithShot(Entity *e, Shot *shot);
	void setControlHint(const std::string &hint, bool left, bool right, bool middle, float time, std::string image="", bool ignoreClear=false, int songType=0, float scale=1);
	void clearControlHint();

	bool trace(Vector start, Vector target);

	BitmapText *timerText;

	float cameraLerpDelay;
	Vector gradTop, gradBtm;
	bool isValidTarget(Entity *e, Entity *me);
	Gradient *grad;
	std::string bgSfxLoop, airSfxLoop;
	void constrainCamera();
	void setElementLayerVisible(int bgLayer, bool v);
	bool isElementLayerVisible(int bgLayer);

	bool isControlHint();

	int getNumberOfEntitiesNamed(const std::string &name);
	MiniMapRender *miniMapRender;
	WorldMapRender *worldMapRender;

	bool loadingScene;
	bool doScreenTrans;
	bool noSceneTransitionFadeout;
	bool fullTilesetReload;

	WaterSurfaceRender *waterSurfaceRender;

	EntityGroups entityGroups;

	std::string getNoteName(int n, const std::string &pre="");

	InterpolatedVector cameraInterp;

	float getWaterLevel();
	void setMusicToPlay(const std::string &musicToPlay);
	Vector lastCollidePosition;
	void switchBgLoop(int v);
	CurrentRender *currentRender;
	SteamRender *steamRender;
	SongLineRender *songLineRender;

	void showImage(const std::string &image);
	void hideImage();

	bool bNatural;
	std::string sceneToLoad;
	void snapCam();
	void updateBgSfxLoop();
	void preLocalWarp(LocalWarpType localWarpType);
	void postLocalWarp();
	void entityDied(Entity *e);

	bool isShuttingDownGameState() { return shuttingDownGameState; }
	void warpToSceneNode(std::string scene, std::string node);

	void ensureLimit(Entity *e, int num, int state=0);

	void rebuildElementUpdateList();

	float getTimer(float mod=1);
	float getHalfTimer(float mod=1);

	std::string bgSfxLoopPlaying2;


	void createInGameMenu();

	Entity *currentPet;
	Entity* setActivePet(int flag);

	void spawnAllIngredients(const Vector &position);
	void createGradient();

	std::string saveMusic;
	GridRender *gridRender, *gridRender2, *gridRender3, *edgeRender, *gridRenderEnt, *gridRenderUser1, *gridRenderUser2;
	void toggleGridRender();
	ElementUpdateList elementUpdateList;
	ElementUpdateList elementInteractionList;

	bool invinciblity;

	bool isApplyingState() { return applyingState; }
	bool activation;
	void overrideZoom(float z, float t=1);

	void learnedRecipe(Recipe *r, bool effects=true);


	bool firstSchoolFish;
	bool invincibleOnNested;
	bool hasPlayedLow;

	void pickupIngredientEffects(IngredientData *data);

	void bindInput();

	bool cameraOffBounds;

	void toggleHelpScreen();

	void setWorldPaused(bool b) { worldPaused = b; }
	bool isWorldPaused() const { return worldPaused; }

	void setIgnoreAction(AquariaActions ac, bool ignore);
	bool isIgnoreAction(AquariaActions ac) const;

	void onContinuityReset();

protected:

	void toggleHelpScreen(bool on, const std::string &label="");
	void onToggleHelpScreen();

	void onHelpUp();
	void onHelpDown();
	bool helpWasPaused;
	Quad *helpBG, *helpBG2;
	AquariaMenuItem *helpUp, *helpDown, *helpCancel;
	TTFText *helpText;
	bool inHelpScreen;

	float ingOffY;
	float ingOffYTimer;

	std::vector<RenderObject*> controlHintNotes;


	bool active;
	bool applyingState;
	int lastBgSfxLoop;
	float timer, halfTimer;


	void warpPrep();
	bool shuttingDownGameState;
	Quad *image;

	float controlHintTimer;
	bool cameraConstrained;

	void updateCursor(float dt);

	Quad *controlHint_mouseLeft, *controlHint_mouseRight, *controlHint_mouseBody, *controlHint_mouseMiddle, *controlHint_bg, *controlHint_image;
	Quad *controlHint_shine;
	bool controlHint_ignoreClear;
	BitmapText *controlHint_text;

	void createLi();
	void createPets();
	void findMaxCameraValues();
	std::vector<ObsRow> obsRows;


	std::string musicToPlay;

	float deathTimer;


	void onPressEscape(int source, InputDevice device);

	bool paused;
	bool worldPaused;

	Vector getClosestPointOnTriangle(Vector a, Vector b, Vector c, Vector p);
	Vector getClosestPointOnLine(Vector a, Vector b, Vector p);

	Vector *cameraFollow;
	RenderObject *cameraFollowObject;
	Entity *cameraFollowEntity;
	bool loadSceneXML(std::string scene);
	void spawnEntities(const EntitySaveData *sav, size_t n);

	void toggleSceneEditor();

	void warpCameraTo(Vector position);

	std::vector<int> ignoredActions;

private:
	Ingredients ingredients;
	InGameMenu *themenu;
	static unsigned char grid[MAX_GRID][MAX_GRID];
};

extern Game *game;

// INLINE FUNCTIONS

inline
ObsType Game::getGridRaw(const TileVector &tile) const
{
	return (unsigned(tile.x) < unsigned(MAX_GRID) && unsigned(tile.y) < unsigned(MAX_GRID))
		? ObsType(grid[tile.x][tile.y])
		: OT_OUTOFBOUNDS;
}

inline
ObsType Game::getGrid(const TileVector &tile) const
{
	return (unsigned(tile.x) < unsigned(MAX_GRID) && unsigned(tile.y) < unsigned(MAX_GRID))
		? ObsType(grid[tile.x][tile.y] & OT_BLOCKING)
		: OT_INVISIBLE;
}

inline
unsigned char *Game::getGridColumn(int tileX)
{
	if (tileX < 0)
		return grid[0];
	else if (tileX >= MAX_GRID)
		return grid[MAX_GRID-1];
	else
		return grid[tileX];
}

inline
void Game::setGrid(const TileVector &tile, ObsType v)
{
	if (unsigned(tile.x) < unsigned(MAX_GRID) && unsigned(tile.y) < unsigned(MAX_GRID))
		grid[tile.x][tile.y] = v;
}

inline
void Game::addGrid(const TileVector &tile, ObsType v)
{
	if (unsigned(tile.x) < unsigned(MAX_GRID) && unsigned(tile.y) < unsigned(MAX_GRID))
		grid[tile.x][tile.y] |= v;
}

inline
bool Game::isObstructed(const TileVector &tile, int t /* = OT_BLOCKING */) const
{
	return (getGrid(tile) & t);
}

inline
bool Game::isObstructedRaw(const TileVector &tile, int t) const
{
	return (getGridRaw(tile) & t);
}

#endif
