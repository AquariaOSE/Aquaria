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

#include "../ExternalLibs/tinyxml.h"
#include "../BBGE/DebugFont.h"
#include "../ExternalLibs/glpng.h"

#include "DSQ.h"
#include "AquariaMenuItem.h"
#include "ScriptedEntity.h"
#include "TileVector.h"
#include "Shot.h"
#include "AquariaProgressBar.h"


class RecipeMenuEntry : public RenderObject
{
public:
	RecipeMenuEntry(Recipe *recipe);
protected:
	void onUpdate(float dt);
	Quad *result, *ing[3];
	Quad *glow;
	BitmapText *description;
	IngredientData *data;

	Recipe *recipe;

	int selected;
};

struct RecipeMenu
{
	RecipeMenu();
	Quad *scroll;
	Quad *scrollEnd;
	BitmapText *header, *page, *description;
	AquariaMenuItem *nextPage, *prevPage;
	

	void toggle(bool on, bool watch=false);
	void createPage(int p);
	void slide(RenderObject *r, bool in, float t);
	void destroyPage();
	void goNextPage();
	void goPrevPage();
	int getNumPages();
	int getNumKnown();
	
	int currentPage;

	bool on;

	std::vector<RecipeMenuEntry*> recipeMenuEntries;
};

class Avatar;
class Gradient;
class CurrentRender;
class SteamRender;
class SongLineRender;
class AutoMap;
class DebugButton;
class WorldMapRender;

const float boxElementZ = -0.1f;
const int MAX_GRID = 2222;

const char CHAR_DOWN		= 'd';
const char CHAR_UP			= 'u';
const char CHAR_LEFT		= 'l';
const char CHAR_RIGHT		= 'r';


const float MIN_SIZE = 0.1;

#ifdef AQUARIA_DEMO
	#undef AQUARIA_BUILD_SCENEEDITOR
#endif

//#include "GridRender.h"
class GridRender;
class MiniMapRender;
class WaterSurfaceRender;
class ToolTip;

#include "Path.h"

#ifdef AQUARIA_BUILD_SCENEEDITOR
struct EntityGroupEntity
{
	std::string name;
	std::string gfx;
};

typedef std::vector<EntityGroupEntity> EntityGroupEntities;

struct EntityGroup
{
	std::string name;
	EntityGroupEntities entities;
};

typedef std::vector<EntityGroup> EntityGroups;

enum EditTypes
{
	ET_NONE			=-1,
	ET_ELEMENTS		=0,
	ET_ENTITIES		=1,
	ET_PATHS		=2,
	ET_SELECTENTITY =4,
	ET_MAX
};
#endif

class ManaBall : public Quad
{
public:
	ManaBall(Vector pos, float a);
	void destroy();
	bool isUsed();
	void use(Entity *entity);
	ParticleEffect healEmitter;
protected:
	float lifeSpan;
	bool used;
	float amount;
	void onUpdate(float dt);
};

class Ingredient : public Entity
{
public:
	Ingredient(const Vector &pos, IngredientData *data, int amount=1);
	void destroy();
	IngredientData *getIngredientData();

	void eat(Entity *e);
	bool hasIET(IngredientEffectType iet);
protected:
	bool isRotKind();
	IngredientData *data;
	bool used, gone;
	float lifeSpan;
	int amount;
	void onUpdate(float dt);
};

typedef std::list<Ingredient*> Ingredients;

class WarpArea
{
public:
	WarpArea()
	{
		w = h = radius = 0;
		generated = false;
	}
	Vector position;
	Vector avatarPosition;
	int radius;
	bool generated;
	int w, h;
	Vector spawnOffset;
	std::string sceneName, warpAreaType;
};

class SongSlot : public AquariaGuiQuad
{
public:
	SongSlot(int songSlot);

	int songSlot, songType;
	bool mbDown;
protected:
	Quad *glow;
	void onUpdate(float dt);
};

class FoodSlot : public AquariaGuiQuad
{
public:
	FoodSlot(int slot);

	void refresh(bool effects);
	int slot;
	void toggle(bool f);
	static int foodSlotIndex;

	IngredientData *getIngredient() { return ingredient; }

	float scaleFactor;

	void eatMe();
	void moveRight();
	void discard();

	bool isCursorIn();

	void setOriginalPosition(const Vector &op);

protected:
	int rmb;
	bool right;
	float doubleClickDelay;
	float grabTime;
	int lastAmount;
	IngredientData *lastIngredient;
	Vector originalPosition;
	void onUpdate(float dt);
	DebugFont *label;
	bool inCookSlot;
	IngredientData *ingredient;
	Quad *lid;
};

class PetSlot : public AquariaGuiQuad
{
public:
	PetSlot(int pet);
	int petFlag;
protected:
	bool wasSlot;
	int petidx;
	bool mouseDown;
	void onUpdate(float dt);
};

class TreasureSlot : public AquariaGuiQuad
{
public:
	TreasureSlot(int treasureFlag);
	void refresh();
protected:
	float doubleClickTimer;
	bool mbd;
	int flag;
	std::string treasureName, treasureDesc;
	int index;
	void onUpdate(float dt);
};

class FoodHolder : public Quad
{
public:
	FoodHolder(int slot, bool trash=false);

	bool isEmpty();
	bool isTrash();
	void setIngredient(IngredientData *i, bool effects=true);
	void dropFood();
	IngredientData *getIngredient();
	void animateLid(bool down, bool longAnim=true);
protected:
	bool trash;
	Quad *wok, *ing;
	bool buttonDown;
	void onUpdate(float dt);

	Quad *lid;

	int slot;
private:
	IngredientData *foodHolderIngredient;
};

class ElementTemplate
{
public:
	ElementTemplate() { alpha = 1; cull = true; w=-1; h=-1; idx=-1; tu1=tu2=tv1=tv2=0; }
	std::string gfx;
	std::vector <TileVector> grid;
	int w,h;
	float tu1, tu2, tv1, tv2;
	void setGrid(Vector position);
	bool cull;
	float alpha;
	int idx;
};

class MiniMapHint
{
public:
	std::string scene;
	std::string warpAreaType;
	void clear()
	{
		debugLog("miniMapHint: CLEAR");
		scene = warpAreaType = "";
	}
};

class WarpAreaRender : public RenderObject
{
public:
protected:
	void onRender();
};

class ObsRow
{
public:
	ObsRow(int tx, int ty, int len);
	int tx, ty, len;
};

enum FlagCheckType
{
	NO_TYPE	=-1,
	AND		=0,
	OR		=1
};

#ifdef AQUARIA_BUILD_SCENEEDITOR
enum EditorStates
{
	ES_SELECTING	=0,
	ES_SCALING		=1,
	ES_ROTATING		=2,
	ES_MOVING		=3,
	ES_MAX
};

enum SelectionType
{
	ST_SINGLE		=0,
	ST_MULTIPLE		,
	ST_MAX
};
#endif

class EntityClass
{
public:
	EntityClass(std::string name, bool script=false, int idx=-1, std::string prevGfx="", float prevScale=1)
		: name(name), script(script), idx(idx), prevGfx(prevGfx), prevScale(prevScale) {}
	std::string name;
	float prevScale;
	std::string prevGfx;
	bool script;
	int idx;
};

#ifdef AQUARIA_BUILD_SCENEEDITOR

class SceneEditorMenuReceiver : public DebugButtonReceiver
{
public:
	void buttonPress(DebugButton *db);
};

struct SelectedEntity
{
	SelectedEntity();
	void clear();
	void setIndex(int idx);
	void setName(const std::string &name, const std::string &prevGfx);
	void setSelectEntity(const SelectedEntity &ent);

	bool nameBased;
	int index, typeListIndex;
	std::string name;
	std::string prevGfx;
	float prevScale;
};

class SceneEditor : public ActionMapper
{
public:
	SceneEditor();
	void init();
	void shutdown();
	void toggle();
	void toggle(bool on);
	void update(float dt);
	void prevElement();
	void nextElement();
	void doPrevElement();
	Element *cycleElementNext(Element *e);
	Element *cycleElementPrev(Element *e);
	void selectZero();
	void selectEnd();
	void placeElement();
	void flipElementHorz();
	void flipElementVert();
	void deleteSelectedElement();
	void deleteElement(int selectedIdx);
	void action(int id, int state);
	void scaleElementUp();
	void scaleElementDown();
	void scaleElement1();
	void placeAvatar();

	void executeButtonID(int bid);

	void openMainMenu();
	void closeMainMenu();

	void setBackgroundGradient();
	void addSpringPlant();

	bool isOn();

	void generateLevel();
	void skinLevel(pngRawInfo *png, int minX, int minY, int maxX, int maxY);
	void skinLevel();

	void regenLevel();


	void startDrawingWarpArea(char c);
	void endDrawingWarpArea(char c);

	void updateSaveFileEnemyPosition(Entity *ent);
	void startMoveEntity();
	void endMoveEntity();

	void down();
	void up();

	void exitMoveState();

	void alignHorz();
	void alignVert();

	EditTypes editType;
	EditorStates state;
	Quad *target;



	Element *getElementAtCursor();
	Entity *getEntityAtCursor();

	void mouseButtonLeftUp();
	void mouseButtonRightUp();
	void moveToBack();
	void moveToFront();
	int bgLayer;
	Element *editingElement;
	Entity *editingEntity;
	Path *editingPath;
	SelectionType selectionType;

	void toggleWarpAreaRender();
	int selectedIdx;
	int selectedNode;
	Path *getSelectedPath();
	void changeDepth();
	void updateEntitySaveData(Entity *editingEntity);
	void moveLayer();
	void moveElementToLayer(Element *e, int bgLayer);
	void toggleElementRepeat();
	void setGroup();
	bool multiSelecting;
	Vector multiSelectPoint;
	std::vector <Element*> selectedElements;
	void fixEntityIDs();
	void bindNodeToEntity();

	Vector groupCenter;
	Vector getSelectedElementsCenter();

	Quad dummy;

	void updateSelectedElementPosition(Vector position);
	int selectedEntityType;
	//int curEntity;
	SelectedEntity selectedEntity;
	//EntityGroups::iterator page;
	int entityPageNum;

	void checkForRebuild();
	void createAquarian();
	void dumpObs();

	DebugButton *btnMenu;
protected:

	void reversePath();

	void updateEntityPlacer();
	void updateMultiSelect();
	float autoSaveTimer;
	int autoSaveFile;

	void enterName();
	void changeShape();
	int skinMinX, skinMinY, skinMaxX, skinMaxY;

	void setGridPattern(int gi);
	void setGridPattern0();
	void setGridPattern1();
	void setGridPattern2();
	void setGridPattern3();
	void setGridPattern4();
	void setGridPattern5();
	void setGridPattern6();
	void setGridPattern7();
	void setGridPattern8();
	void setGridPattern9();
	void toggleElementSolid();
	void toggleElementHurt();
	void editModeElements();
	void editModeEntities();
	void editModePaths();
	int selectedType, possibleSelectedType;

	void deleteSelected();
	void cloneSelectedElement();
	void cloneSelectedElementInput();
	void enterScaleState();
	void enterRotateState();
	void enterMoveState();

	Vector oldPosition, oldRotation, oldScale, cursorOffset;

	RenderObject *getSelectedRenderObject();

	Entity *movingEntity;
	void updateDrawingWarpArea(char c, int k);
	char drawingWarpArea;

	void nextEntityType();
	void prevEntityType();

	void removeEntity();

	void selectEntityFromGroups();


	WarpAreaRender *warpAreaRender;
	Vector zoom;

	Vector boxPos;
	Quad *boxPromo;
	bool drawingBox;
	void rotateElement();
	void rotateElement2();
	void updateText();

	void saveScene();
	void loadScene();

	SceneEditorMenuReceiver menuReceiver;

	void addMainMenuItem(const std::string &label, int bid);

	void loadSceneByName();
	void reloadScene();

	void mouseButtonLeft();
	void mouseButtonRight();

	int curElement, selectedVariation, possibleSelectedIdx;

	Quad *placer;
	DebugFont *text;
	bool on;
	InterpolatedVector oldGlobalScale;
};

#endif  // AQUARIA_BUILD_SCENEEDITOR

typedef std::vector<Quad*> QuadList;
typedef std::vector<QuadList> QuadArray;

typedef std::list<Element*> ElementUpdateList;

// Note: although this is a bitmask, only one of these values may be set at a time!
enum ObsType
{
	OT_EMPTY		= 0x00,

	// immutable
	OT_BLACK		= 0x01,
	OT_BLACKINVIS	= 0x02,  // same as OT_BLACK, but not drawn
	OT_MASK_BLACK	= OT_BLACK | OT_BLACKINVIS,

	// set by tiles
	OT_INVISIBLE	= 0x04,
	OT_INVISIBLEIN	= 0x08,
	OT_HURT			= 0x10,

	// set by entities
	OT_INVISIBLEENT = 0x20,
};

struct EntitySaveData
{
public:
	EntitySaveData(Entity *e, int idx, int x, int y, int rot, int group, int id, const std::string &name) : e(e), idx(idx), x(x), y(y), rot(rot), group(group), id(id), name(name) {}
	Entity *e;
	int idx, x, y, rot, group, id;
	std::string name;
};

class Game : public StateObject
{
public:
	Game();
	~Game();
	void applyState();
	void removeState();
	void update(float dt);
	void onLeftMouseButton();
	//std::vector<Item*>items;

	Avatar *avatar;
	Entity *li;

	Element *elementWithMenu;

	FoodSlot *moveFoodSlotToFront;

	//void doChoiceMenu(Vector position, std::vector<std::string> choices);

	std::string getSelectedChoice() { return selectedChoice; }

	int getGrid(const TileVector &tile) const;
	int getGridRaw(unsigned int x, unsigned int y) const;
	const signed char *getGridColumn(int tileX);
	void setGrid(const TileVector &tile, int v);
	bool isObstructed(const TileVector &tile, int t = -1) const;
	void trimGrid();

	void clearPointers();

	void sortFood();
	void updatePreviewRecipe();

	void transitionToScene(std::string scene);
	void transitionToSceneUnder(std::string scene);
	bool loadScene(std::string scene);

	void clearGrid(int v = 0);
	void clearDynamicGrid(unsigned char maskbyte = OT_MASK_BLACK);

	void toggleWorldMap();

	void action(int id, int state);

	void adjustFoodSlotCursor();

	void loadElementTemplates(std::string pack);
	Element* createElement(int etidx, Vector position, int bgLayer=0, RenderObject *copy=0, ElementTemplate *et=0);
	void setGrid(ElementTemplate *et, Vector position, float rot360=0);

	void updateParticlePause();

	void reconstructGrid(bool force=false);
	void reconstructEntityGrid();

	void registerSporeDrop(const Vector &pos, int t);

	bool collideBoxWithGrid(const Vector& position, int w, int h);
	bool collideCircleWithGrid(const Vector& position, int r);

	bool collideHairVsCircle(Entity *a, int num, const Vector &pos2, int radius, float perc=0);

	bool collideCircleVsCircle(Entity *a, Entity *b);
	Bone *collideSkeletalVsCircle(Entity *skeletal, Entity *circle);
	Bone *collideSkeletalVsLine(Entity *skeletal, Vector start, Vector end, float radius);
	bool collideCircleVsLine(RenderObject *r, Vector start, Vector end, float radius);
	bool collideCircleVsLineAngle(RenderObject *r, float angle, float startLen, float endLen, float radius, Vector basePos);
	Bone *collideSkeletalVsCircle(Entity *skeletal, Vector pos, float radius);
	void handleShotCollisions(Entity *e, bool hasShield=false);
	void handleShotCollisionsSkeletal(Entity *e);
	void handleShotCollisionsHair(Entity *e, int num = 0);

	std::vector<ElementTemplate> elementTemplates;
	std::string sceneName;

	ElementTemplate *getElementTemplateByIdx(int idx);

	bool saveScene(std::string scene);
	typedef std::vector<WarpArea> WarpAreas;
	WarpAreas warpAreas;

	void postInitEntities();
	Entity *getEntityInGroup(int gid, int iter);
	EntityClass *getEntityClassForEntityType(const std::string &type);

	void warpToArea(WarpArea *area);

	InterpolatedVector sceneColor, sceneColor2, sceneColor3;
	Vector backupSceneColor;

	Vector getCameraPositionFor(const Vector &vec);

	bool isActive();

	bool isPaused();
	void togglePause(bool v);

	Ingredient* spawnIngredient(const std::string &ing, const Vector &pos, int times=1, int out=0);
	void spawnIngredientFromEntity(Entity *ent, IngredientData *data);

	Ingredient *getNearestIngredient(const Vector &pos, int radius);
	Entity *getNearestEntity(const Vector &pos, int radius, Entity *ignore = 0, EntityType et=ET_NOTYPE, DamageType dt=DT_NONE, int lrStart=-1, int lrEnd=-1);

	void spawnManaBall(Vector pos, float a);
	bool updateMusic();
	std::string overrideMusic;
	void resetFromTitle();

	float maxZoom;

	void setParallaxTextureCoordinates(Quad *q, float speed);

	TiXmlDocument *saveFile;

	Vector positionToAvatar;
	float getCoverage(Vector pos, int sampleArea = 5);

	float getPercObsInArea(Vector position, int range, int obs=-1);
	Vector getWallNormal(Vector pos, int sampleArea = 5, float *dist=0, int obs = -1);

	// HACK:: clean up these vars
	std::string warpAreaType, warpAreaSide;
	Vector spawnOffset;
	Vector miniMapHintPosition;
	MiniMapHint miniMapHint;
	void updateMiniMapHintPosition();
	EntitySaveData *getEntitySaveDataForEntity(Entity *e, Vector pos);
	Entity *createEntity(int idx, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType = ET_ENEMY, Entity::NodeGroups *nodeGroups=0, int groupID=0, bool doPostInit=false);
	Entity *createEntity(const std::string &type, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType = ET_ENEMY, Entity::NodeGroups *nodeGroups=0, int groupID=0, bool doPostInit=false);
	Entity *establishEntity(Entity *e, int id=0, Vector position=Vector(0,0), int rot=0, bool createSaveData=false, std::string name="", EntityType = ET_ENEMY, Entity::NodeGroups *nodeGroups=0, int groupID=0, bool doPostInit=false);
	void setCameraFollow(RenderObject *r);
	void setCameraFollowEntity(Entity *e);
	void setMenuDescriptionText(const std::string &text);

	bool removeEntityAtCursor();
	void toggleOverrideZoom(bool on);
	bool doFlagCheck(const std::string &flagCheck, FlagCheckType type=NO_TYPE, bool lastTruth=false);

	bool useWaterLevel;
	InterpolatedVector waterLevel;
	int saveWaterLevel;
	void flipSceneVertical(int flipY);
	void warpCameraTo(RenderObject *r);
	bool isSceneFlipped();
	void refreshItemSlotIcons();

	void addObsRow(int tx, int ty, int len);
	void clearObsRows();
	void setWarpAreaSceneName(WarpArea &warpArea);
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
	void removePath(int idx);
	void clearPaths();
	int getNumPaths() const {return paths.size();}
	Path *getPath(int idx) const {return paths[idx];}
	Path *getFirstPathOfType(PathType type) const {return firstPathOfType[type];}
	Path *getPathByName(std::string name);
	int getIndexOfPath(Path *p);
	Path *getPathAtCursor();
	Path *getScriptedPathAtCursor(bool withAct=false);
	Path *getNearestPath(const Vector &pos, const std::string &name="", const Path *ignore=0);
	Path *getNearestPath(const Vector &pos, PathType pathType=PATH_NONE);
	Path *getNearestPath(Path *p, std::string name);

#ifdef AQUARIA_BUILD_SCENEEDITOR
	SceneEditor sceneEditor;
	bool isSceneEditorActive() {return sceneEditor.isOn();}
#else
	bool isSceneEditorActive() const {return false;}
#endif

	bool isInGameMenu();

	typedef std::vector<EntityClass> EntityTypeList;
	EntityTypeList entityTypeList;
	void loadEntityTypeList();
	std::vector<EntitySaveData> entitySaveData;
	int getIdxForEntityType(std::string type);
	void hideInGameMenu(bool effects=true);
	void showInGameMenu(bool force=false, bool optionsOnly=false, MenuPage menuPage = MENUPAGE_NONE);
	bool optionsOnly;

	MenuPage currentMenuPage;
	int currentFoodPage, currentTreasurePage;

	Precacher tileCache;

	//void cameraPanToNode(Path *p, int speed=500);
	//void cameraRestore();
	void setCameraFollow(Vector *position);
	Shot *fireShot(Entity *firer, const std::string &particleEffect, Vector position, bool big, Vector direction, Entity *target, int homing=0, int velLenOverride=0, int targetPt=-1);
	Shot *fireShot(const std::string &bankShot, Entity *firer, Entity *target=0, const Vector &pos=Vector(0,0,0), const Vector &aim=Vector(0,0,0), bool playSfx=true);
	void playBurstSound(bool wallJump=false);
	void toggleMiniMapRender();
	void toggleMiniMapRender(int v);
	bool runGameOverScript;


	void setTimerTextAlpha(float a, float t);
	void setTimerText(float time);

	void generateCollisionMask(Quad *q, int overrideCollideRadius=0);
	std::string sceneNatureForm;
	std::string fromScene, toNode;
	int toFlip;
	char fromWarpType;
	Vector fromVel;
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

	void colorTest();

	void playSongInMenu(int songType, bool override=false);

	bool trace(Vector start, Vector target);

	Quad *menuSongs;
	std::vector<SongSlot*> songSlots;
	std::vector<FoodSlot*> foodSlots;
	std::vector<TreasureSlot*> treasureSlots;
	BitmapText* songDescription;

	BitmapText *timerText;

	float cameraLerpDelay;
	Vector gradTop, gradBtm;
	bool isValidTarget(Entity *e, Entity *me);
	Gradient *grad;
	std::string bgSfxLoop, airSfxLoop;
	void constrainCamera();
	void setElementLayerVisible(int bgLayer, bool v);
	bool isElementLayerVisible(int bgLayer);

	void showInGameMenuExitCheck();
	void hideInGameMenuExitCheck(bool refocus);
	bool isControlHint();

	int getNumberOfEntitiesNamed(const std::string &name);
	MiniMapRender *miniMapRender;
	WorldMapRender *worldMapRender;
	AutoMap *autoMap;

	Quad *hudUnderlay;

	int worldMapIndex;

	void spawnSporeChildren();

	bool creatingSporeChildren;
	bool loadingScene;

	WaterSurfaceRender *waterSurfaceRender;
	Quad *shapeDebug;

#ifdef AQUARIA_BUILD_SCENEEDITOR
	EntityGroups entityGroups;
#endif

	std::string getNoteName(int n, const std::string &pre="");

	void selectEntityFromGroups();
	InterpolatedVector cameraInterp, tintColor;
	float getWaterLevel();
	void setMusicToPlay(const std::string &musicToPlay);
	Vector lastCollidePosition;
	void switchBgLoop(int v);
	ElementTemplate getElementTemplateForLetter(int i);
	CurrentRender *currentRender;
	SteamRender *steamRender;
	SongLineRender *songLineRender;

	void showImage(const std::string &image);
	void hideImage();

	bool bNatural;
	void onLips();
	std::string sceneToLoad;
	void snapCam();

	void updateOptionsMenu(float dt);
	BitmapText *songLabel, *foodLabel, *foodDescription, *treasureLabel;
	ToolTip *treasureDescription;
	Quad *treasureCloseUp;
	void updateBgSfxLoop();
	void preLocalWarp(LocalWarpType localWarpType);
	void postLocalWarp();
	void entityDied(Entity *e);

	bool isShuttingDownGameState() { return shuttingDownGameState; }
	void warpToSceneNode(std::string scene, std::string node);

	AquariaProgressBar *progressBar;
	void addProgress();
	void endProgress();

	void refreshFoodSlots(bool effects);
	void refreshTreasureSlots();

	Recipe *findRecipe(const std::vector<IngredientData*> &list);
	void onCook();
	void onRecipes();
	void updateCookList();
	void onUseTreasure();

	void onPrevFoodPage();
	void onNextFoodPage();

	void onPrevTreasurePage();
	void onNextTreasurePage();

	std::vector<std::string> dropIngrNames;

	AquariaMenuItem *lips;

	int lastCollideMaskIndex;

	void ensureLimit(Entity *e, int num, int state=0);

	void rebuildElementUpdateList();
	void setElementLayerFlags();

	float getTimer(float mod=1);
	float getHalfTimer(float mod=1);
	float getHalf2WayTimer(float mod=1);

	std::string bgSfxLoopPlaying2;


	void createInGameMenu();

	Entity *currentPet;
	Entity* setActivePet(int flag);

	void spawnAllIngredients(const Vector &position);
	void createGradient();

	std::string saveMusic;
	GridRender *gridRender, *gridRender2, *gridRender3, *edgeRender, *gridRenderEnt;
	void toggleGridRender();
	ElementUpdateList elementUpdateList;

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
	RecipeMenu recipeMenu;

	AquariaMenuItem *eYes, *eNo, *cook, *recipes, *nextFood, *prevFood, *nextTreasure, *prevTreasure, *use, *keyConfigButton;
	AquariaMenuItem *opt_cancel, *opt_save, *foodSort;

	bool cameraOffBounds;

	void enqueuePreviewRecipe();

	void toggleHelpScreen(bool on, const std::string &label="");
	void onToggleHelpScreen();

protected:

	void onHelpUp();
	void onHelpDown();
	bool helpWasPaused;
	Quad *helpBG, *helpBG2;
	AquariaMenuItem *helpUp, *helpDown, *helpCancel;
	TTFText *helpText;
	bool inHelpScreen;

	int enqueuedPreviewRecipe;

	Quad *previewRecipe;
	Quad *menuIconGlow;
	Quad *showRecipe;

	bool isCooking;

	void doMenuSectionHighlight(int sect);
	
	float cookDelay;

	float ingOffY;
	float ingOffYTimer;

	std::vector<RenderObject*> controlHintNotes;

	void onPrevRecipePage();
	void onNextRecipePage();

	

	typedef std::vector<IngredientData*> CookList;
	CookList cookList;

	bool active;
	bool applyingState;
	int lastBgSfxLoop;
	float timer, halfTimer;


	void warpPrep();
	void warpKey1();
	void warpKey2();
	void warpKey3();
	void warpKey4();
	bool shuttingDownGameState;
	void onOptionsMenu();
	bool optionsMenu, foodMenu, petMenu, treasureMenu, keyConfigMenu;
	void toggleOptionsMenu(bool f, bool skipBackup=false, bool isKeyConfig=false);
	void toggleFoodMenu(bool f);
	void toggleMainMenu(bool f);
	void togglePetMenu(bool f);
	void toggleTreasureMenu(bool f);
	void toggleRecipeList(bool on);
	void toggleKeyConfigMenu(bool f);

	void switchToSongMenu();
	void switchToFoodMenu();
	void switchToPetMenu();
	void switchToTreasureMenu();

	void onKeyConfig();

	void addKeyConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int y, int l1=0, int l2=0, int l3=0);

	AquariaKeyConfig *addAxesConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int y, int offx);

	void onOptionsSave();
	void onOptionsCancel();
	AquariaSlider *sfxslider, *musslider, *voxslider;
	AquariaCheckBox *autoAimCheck, *targetingCheck, *toolTipsCheck, *flipInputButtonsCheck, *micInputCheck, *blurEffectsCheck;
	AquariaCheckBox *subtitlesCheck, *fullscreenCheck, *ripplesCheck;
	AquariaComboBox *resBox;
	Quad *songBubbles, *energyIdol, *liCrystal;

	RenderObject *group_keyConfig;

	Quad *options;

	Quad *image;
	void assignEntitiesUniqueIDs();
	void initEntities();


	void onExitCheckNo();
	void onExitCheckYes();

	BitmapText *circlePageNum;

	std::vector<ToolTip*> foodTips, songTips, petTips, treasureTips;



	Quad *eAre;
	int inGameMenuExitState;
	float controlHintTimer;
	bool cameraConstrained;

	void updateCursor(float dt);
	void updateInGameMenu(float dt);
	float songMenuPlayDelay;
	int currentSongMenuNote;
	int playingSongInMenu;
	Quad *controlHint_mouseLeft, *controlHint_mouseRight, *controlHint_mouseBody, *controlHint_mouseMiddle, *controlHint_bg, *controlHint_image;
	Quad *controlHint_shine;
	bool controlHint_ignoreClear;
	BitmapText *controlHint_text;



	void updateCurrentVisuals(float dt);
	std::string lastTileset;


	void createLi();
	void createPets();
	Quad *backdropQuad;
	void findMaxCameraValues();
	std::vector<ObsRow> obsRows;

	bool sceneFlipped;


	void flipRenderObjectVertical(RenderObject *r, int flipY);
	void onFlipTest();

	void onDebugSave();

	BitmapText *menuDescription;
	BitmapText *menuEXP, *menuMoney;

	std::vector<Quad*> spellIcons;
	int currentInventoryPage;
	float backgroundImageRepeat;

	std::string musicToPlay;

	float deathTimer;

	/*
	void onAssignMenuScreenItemToSlot0();
	void onAssignMenuScreenItemToSlot1();
	void onAssignMenuScreenItemToSlot2();
	void onAssignMenuScreenItemToSlot3();
	*/

	void onInGameMenuInventory();
	void onInGameMenuSpellBook();
	void onInGameMenuContinue();
	void onInGameMenuOptions();
	void onInGameMenuSave();
	void onInGameMenuExit();

	void onPressEscape();



	std::vector<AquariaMenuItem*> menu;
	Quad *menuBg, *menuBg2;
	bool paused;

	Vector getClosestPointOnTriangle(Vector a, Vector b, Vector c, Vector p);
	Vector getClosestPointOnLine(Vector a, Vector b, Vector p);

	std::string elementTemplatePack;


	Vector *cameraFollow;
	RenderObject *cameraFollowObject;
	Entity *cameraFollowEntity;
	bool loadSceneXML(std::string scene);

#ifdef AQUARIA_BUILD_SCENEEDITOR
	void toggleSceneEditor();
#endif




	signed char grid[MAX_GRID][MAX_GRID];


	Quad *bg, *bg2;

	bool inGameMenu;
	float menuOpenTimer;

	std::vector <Quad*> itemSlotIcons, itemSlotEmptyIcons;


	std::string selectedChoice;



	void warpCameraTo(Vector position);

private:
	Ingredients ingredients;
};

extern Game *game;

// INLINE FUNCTIONS

inline
int Game::getGridRaw(unsigned int x, unsigned int y) const
{
	return grid[x][y];
}

inline
int Game::getGrid(const TileVector &tile) const
{
	if (tile.x < 0 || tile.x >= MAX_GRID || tile.y < 0 || tile.y >= MAX_GRID) return OT_INVISIBLE;
	return grid[tile.x][tile.y];
}

inline
const signed char *Game::getGridColumn(int tileX)
{
	if (tileX < 0)
		return grid[0];
	else if (tileX >= MAX_GRID)
		return grid[MAX_GRID-1];
	else
		return grid[tileX];
}

inline
void Game::setGrid(const TileVector &tile, int v)
{
	if (tile.x < 0 || tile.x >= MAX_GRID || tile.y < 0 || tile.y >= MAX_GRID) return;
	grid[tile.x][tile.y] = v;
}

inline
bool Game::isObstructed(const TileVector &tile, int t /* = -1 */) const
{
	return (getGrid(tile) & t);
}

#endif
