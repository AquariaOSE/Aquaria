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
#ifndef __dsq__
#define __dsq__

#include "AquariaCompileConfig.h"
#include "../BBGE/Core.h"
#include "../BBGE/Quad.h"
#include "Element.h"
#include "../BBGE/BitmapFont.h"
#include "../BBGE/ScreenTransition.h"
#include "../BBGE/Precacher.h"
#include "../ExternalLibs/tinyxml.h"
#include "AquariaMenuItem.h"
#include "ScriptInterface.h"

#include "PathFinding.h"

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
class ProfRender;

const float FRAME_TIME = 0.04;

const int MAX_INGREDIENT_AMOUNT = 8;

const float MENUSELECTDELAY		= 0.2;

const int VERSION_MAJOR			= 1;
const int VERSION_MINOR			= 1;
const int VERSION_REVISION		= 3;

const int VERSION_BETA			= 5;
const int VERSION_FC			= 3;
const int VERSION_GM			= 6;

enum CursorType
{
	CURSOR_NONE		= -1,
	CURSOR_NORMAL	= 0,
	CURSOR_SWIM		= 1,
	CURSOR_BURST	= 2,
	CURSOR_SING		= 3,
	CURSOR_LOOK		= 4
};

enum AquariaActions
{
	ACTION_PRIMARY					=0,
	ACTION_SECONDARY				=1,
	ACTION_ESC						=2,
	ACTION_TOGGLESCENEEDITOR		=3,
	ACTION_TOGGLEWORLDMAP			=4,

	ACTION_TOGGLEGRID				=5,

	ACTION_MENULEFT					=6,
	ACTION_MENURIGHT				=7,
	ACTION_MENUUP					=8,
	ACTION_MENUDOWN					=9,

	ACTION_PREVPAGE,
	ACTION_NEXTPAGE,
	ACTION_COOKFOOD,
	ACTION_FOODLEFT,
	ACTION_FOODRIGHT,
	ACTION_FOODDROP,


	ACTION_SWIMUP = 100,
	ACTION_SWIMDOWN,
	ACTION_SWIMLEFT,
	ACTION_SWIMRIGHT,

	ACTION_SINGUP,
	ACTION_SINGDOWN,
	ACTION_SINGLEFT,
	ACTION_SINGRIGHT,

	ACTION_SONGSLOT1,
	ACTION_SONGSLOT2,
	ACTION_SONGSLOT3,
	ACTION_SONGSLOT4,
	ACTION_SONGSLOT5,
	ACTION_SONGSLOT6,
	ACTION_SONGSLOT7,
	ACTION_SONGSLOT8,
	ACTION_SONGSLOT9,
	ACTION_SONGSLOT10,
	ACTION_SONGSLOTEND,

	ACTION_ROLL,

	ACTION_SLOW,						// currently unused

	ACTION_ZOOMIN		= 200,
	ACTION_ZOOMOUT,

	ACTION_CAMLEFT,
	ACTION_CAMRIGHT,
	ACTION_CAMUP,
	ACTION_CAMDOWN,

	ACTION_BONELEFT,
	ACTION_BONERIGHT,
	ACTION_BONEUP,
	ACTION_BONEDOWN,

	ACTION_BGLAYER1,
	ACTION_BGLAYER2,
	ACTION_BGLAYER3,
	ACTION_BGLAYER4,
	ACTION_BGLAYER5,
	ACTION_BGLAYER6,
	ACTION_BGLAYER7,
	ACTION_BGLAYER8,
	ACTION_BGLAYER9,
	ACTION_BGLAYER10,
	ACTION_BGLAYER11,
	ACTION_BGLAYER12,
	ACTION_BGLAYER13,
	ACTION_BGLAYER14,
	ACTION_BGLAYER15,
	ACTION_BGLAYER16,
	ACTION_BGLAYEREND,

	ACTION_MULTISELECT				,

	ACTION_TOGGLEWORLDMAPEDITOR		,
	
	ACTION_LOOK						,
	ACTION_TOGGLEHELPSCREEN
};

enum EditorLock
{
	EDITORLOCK_NONE		= 0,
	EDITORLOCK_USER		= 1
};

const EditorLock editorLock = EDITORLOCK_USER;

typedef std::list<Entity*> EntityList;
typedef std::vector<Entity*> EntityContainer;

#define FOR_ENTITIES(i) for (Entity **i = &dsq->entities[0]; *i != 0; i++)


enum MenuPage
{
	MENUPAGE_NONE		= -1,
	MENUPAGE_SONGS		= 0,
	MENUPAGE_FOOD		= 1,
	MENUPAGE_TREASURES	= 2,
	MENUPAGE_PETS		= 3
};

/*
class Title;
class GameOver;
class Logo;
class Entity;
class SCLogo;
class IntroText;
class AnimationEditor;
class Intro;
*/
struct SubLine
{
	SubLine() { timeStamp = 0; }
	float timeStamp;
	std::string line;
};

class StringBank
{
public:
	StringBank();
	void load(const std::string &file);

	std::string get(int idx);
protected:

	typedef std::map<int, std::string> StringMap;
	StringMap stringMap;
};

class SubtitlePlayer
{
public:
	SubtitlePlayer();
	void go(const std::string &subs);
	void update(float dt);
	void end();
	
	void hide(float t = 0);
	void show(float t = 0);

	bool isVisible();

	typedef std::vector<SubLine> SubLines;
	SubLines subLines;

	int curLine;
protected:
	bool vis, hidden;
};

struct ModEntry
{
	std::string path;
};

class Mod
{
public:
	Mod();
	void clear();
	void loadModXML(TiXmlDocument *d, std::string modName);
	void setActive(bool v);
	void start();
	void stop();
	void load(const std::string &path);
	
	void update(float dt);

	void recache();
	
	std::string getBaseModPath();

	bool isActive();
	bool isDebugMenu();

	std::string getPath();
	std::string getName();
	
	void shutdown();
	bool isShuttingDown();
protected:
	bool shuttingDown;
	bool active;
	int doRecache;
	int debugMenu;
	int enqueueModStart;
	void applyStart();

	std::string name;
	std::string path;
};

class ModSelector : public AquariaGuiQuad
{
public:
	ModSelector();
	void refreshTexture();
protected:
	bool refreshing;
	BitmapText *label;
	void onUpdate(float dt);
	bool mouseDown;
};

class AquariaScreenTransition : public ScreenTransition
{
public:
	void capture();
};

typedef std::vector<int> SongNotes;

struct Song
{
	Song() { index=0; script=0; }
	int index;
	SongNotes notes;
	bool script;
};

const int MAX_FLAGS				= 1024;

enum AuraType
{
	AURA_NONE				= -1,
	AURA_SHIELD				= 0,
	AURA_THING				= 1,
	AURA_HEAL				= 2
};

enum SongType
{
	SONG_NONE				= -1,
	SONG_HEAL				= 0,
	SONG_ENERGYFORM			= 1,
	SONG_SONGDOOR1			= 2,
	SONG_SPIRITFORM			= 3,
	SONG_BIND				= 4,
	SONG_NATUREFORM			= 5,
	SONG_BEASTFORM			= 6,
	SONG_SHIELDAURA			= 7,
	SONG_SONGDOOR2			= 8,
	SONG_DUALFORM			= 9,
	SONG_FISHFORM			= 10,
	SONG_SUNFORM			= 11,
	SONG_LI					= 12,
	SONG_TIME				= 13,
	SONG_LANCE				= 14,
	SONG_MAP				= 15,
	SONG_ANIMA				= 16,
	SONG_MAX
};

const int numForms			= 7;

enum FormType
{
	FORM_NONE			= -1,
	FORM_NORMAL			= 0,
	FORM_ENERGY			,
	FORM_BEAST			,
	FORM_NATURE			,
	FORM_SPIRIT			,
	FORM_DUAL			,
	FORM_FISH			,
	FORM_SUN			,
	FORM_MAX
};

enum FormUpgradeType
{
	FORMUPGRADE_ENERGY1		=0,
	FORMUPGRADE_ENERGY2		,
	FORMUPGRADE_BEAST		,
	FORMUPGRADE_MAX
};

// defined by windows includes
#undef INPUT_MOUSE
#undef INPUT_KEYBOARD

enum InputMode
{
	INPUT_MOUSE		= 0,
	INPUT_JOYSTICK	= 1,
	INPUT_KEYBOARD	= 2
};

enum EFXType
{
	EFX_NONE	=-1,
	EFX_SEGS	=0,
	EFX_ALPHA	,
	EFX_WAVY	,
	EFX_MAX
};

struct ElementEffect
{
public:
	int type;
	int segsx, segsy;
	float segs_dgox, segs_dgoy, segs_dgmx, segs_dgmy, segs_dgtm, segs_dgo;
	float wavy_radius, wavy_min, wavy_max;
	bool wavy_flip;
	InterpolatedVector alpha;
	InterpolatedVector color;
	int blendType;
};

struct EmoteData
{
	EmoteData()
	{
		index = -1; variations = 0;
	}
	int index;
	std::string name;
	int variations;
};

enum EmoteType
{
	EMOTE_NAIJAEVILLAUGH	= 0,
	EMOTE_NAIJAGIGGLE		= 1,
	EMOTE_NAIJALAUGH		= 2,
	EMOTE_NAIJASADSIGH		= 3,
	EMOTE_NAIJASIGH			= 4,
	EMOTE_NAIJAWOW			= 5,
	EMOTE_NAIJAUGH			= 6,
	EMOTE_NAIJALOW			= 7,
	EMOTE_NAIJALI			= 8
};

class Emote
{
public:
	Emote();
	void load(const std::string &file);
	void playSfx(int index);
	void update(float dt);

	float emoteTimer;
	int lastVariation;

	typedef std::vector<EmoteData> Emotes;
	Emotes emotes;
};

enum WorldType
{
	WT_NONE		= -1,
	WT_NORMAL	= 0,
	WT_SPIRIT	= 1
};

enum VisualEffectsType
{
	VFX_NONE		= -1,
	VFX_SHOCK		= 0,
	VFX_RIPPLE		= 1,
	VFX_SHOCKHIT	= 2,
	VFX_MAX			= 3
};

enum Layers
{
	// GAME WILL CLEAR THESE
	LR_ZERO						= 0,
	LR_BACKDROP					,
	LR_BACKGROUND				,
	LR_SCENEBACKGROUNDIMAGE		,
	LR_BACKDROP_ELEMENTS1	,
	LR_BACKDROP_ELEMENTS2	,
	LR_ENTITIES_MINUS4_PLACEHOLDER	,
	LR_BACKDROP_ELEMENTS3	,
	LR_BACKDROP_ELEMENTS4	,
	LR_BACKDROP_ELEMENTS5	,
	LR_BACKDROP_ELEMENTS6	,
	LR_BACKGROUND_ELEMENTS1	,
	LR_BACKGROUND_ELEMENTS2	,
	LR_ENTITIES_MINUS3_PLACEHOLDER	,
	LR_BACKGROUND_ELEMENTS3	,
	LR_ENTITIES_MINUS2_PLACEHOLDER	,
	LR_BLACKGROUND			,
	LR_UPDATE_ELEMENTS_BG	,
	LR_ELEMENTS1			,
	LR_ELEMENTS2			,
	LR_ELEMENTS3			,
	LR_ELEMENTS4			,
	LR_ELEMENTS5			,
	LR_ELEMENTS6			,
	LR_ELEMENTS7			,
	LR_ELEMENTS8			,
	LR_ELEMENTS9			,
	LR_ELEMENTS10			,
	LR_ELEMENTS11			,
	LR_ELEMENTS12			,
	LR_ELEMENTS13			,
	LR_ELEMENTS14			,
	LR_ELEMENTS15			,
	LR_ELEMENTS16			,
	LR_UPDATE_ELEMENTS_FG	,
	LR_ENTITIES_MINUS4		,
	LR_ENTITIES_MINUS3		,
	LR_ENTITIES_MINUS2		,
	LR_ENTITIES00			,
	LR_ENTITIES0			,
	LR_ENTITIES				,
	LR_ENTITIES2			,
	LR_WATERSURFACE			,
	LR_WATERSURFACE2		,
	LR_DARK_LAYER			,
	LR_PROJECTILES			,
	LR_LIGHTING				,
	LR_PARTICLES			,
	LR_PARTICLES2			,
	LR_FOREGROUND_ELEMENTS1	,
	LR_FOREGROUND_ELEMENTS2	,
	LR_PARTICLES_TOP		,
	LR_AFTER_EFFECTS		,
	LR_SCENE_COLOR			,
	LR_MENU					,
	LR_MENU2				,
	LR_HUD					,
	LR_HUD2					,
	LR_HUD3					,
	LR_HUDUNDERLAY			,
	LR_MINIMAP				,
	LR_RECIPES				,
	LR_WORLDMAP				,
	LR_WORLDMAPHUD			,
	LR_REGISTER_TEXT		,
	LR_DAMAGESPRITE			,
	LR_HELP					,
	LR_TRANSITION			,
	LR_OVERLAY				,
	LR_FILEMENU				,
	LR_CONFIRM				,
	LR_CURSOR				,
	LR_SUBTITLES			,
	LR_PROGRESS				,
	LR_DEBUG_TEXT			,
	LR_BLACKBARS			,
	LR_MAX
};

class Avatar;

/*
class GardenHoleData
{
public:
	GardenHoleData();
	float timePlanted, timeLastChecked, lastTimeGrown;
	int plantedItem;
	unsigned int state;
	int slotsUsed;
};

class GardenData
{
public:
	GardenHoleData holeData[256];
};
*/

#define MAPVIS_SUBDIV 64

struct WorldMapTile
{
	WorldMapTile();
	~WorldMapTile();

	void markVisited(int left, int top, int right, int bottom);
	void dataToString(std::ostringstream &os);
	void stringToData(std::istringstream &is);
	const unsigned char *getData() const {return data;}

	std::string name;
	Vector gridPos;
	float scale, scale2;
	bool revealed, prerevealed;
	int layer, index;
	int stringIndex;

	Quad *q;

protected:
	unsigned char *data;
};

struct WorldMap
{
	WorldMap();
	void load(const std::string &file);
	void save(const std::string &file);
	void hideMap();
	void revealMap(const std::string &name);
	WorldMapTile *getWorldMapTile(const std::string &name);
	int getNumWorldMapTiles();
	WorldMapTile *getWorldMapTile(int index);

	WorldMapTile *getWorldMapTileByIndex(int index);
	void revealMapIndex(int index);

	int gw, gh;
	typedef std::vector<WorldMapTile> WorldMapTiles;
	WorldMapTiles worldMapTiles;
};

class Path;

struct GemData
{
	GemData() { canMove=false; }
	std::string name;
	std::string userString;
	bool canMove;
	Vector pos;
};

struct BeaconData
{
	BeaconData(){ index=-1; on=0; }
	int index;
	Vector pos,color;
	bool on;
};

enum IngredientType
{
	IT_NONE			= -1,
	IT_LEAF			= 0,
	IT_MEAT			,
	IT_EGG			,
	IT_OIL			,
	IT_BERRY		,
	IT_MUSHROOM		,
	IT_BULB			,
	IT_TENTACLE		,
	IT_ICECHUNK		,
	IT_PART			,

	IT_SHELL		,
	IT_BONE			,
	IT_INGREDIENTSEND ,

	IT_FOOD		= 100,
	IT_SOUP		= 101,
	IT_CAKE		= 103,
	IT_ICECREAM	= 105,
	IT_LOAF		= 107,
	IT_PEROGI	= 108,
	IT_POULTICE	= 109,
	IT_ROLL		= 110,

	IT_ANYTHING	= 200,

	IT_MAX
};

enum IngredientEffectType
{
	IET_NONE		= -1,
	IET_HP			= 0,
	IET_DEFENSE		= 1,
	IET_SPEED		= 2,
	IET_RANDOM		= 3,
	IET_MAXHP		= 4,
	IET_INVINCIBLE	= 5,
	IET_TRIP		= 6,
	IET_REGEN		= 7,
	IET_LI			= 8,
	IET_FISHPOISON	= 9,
	IET_BITE		= 10,
	IET_EAT			= 11,
	IET_LIGHT		= 12,
	IET_YUM			= 13,
	IET_PETPOWER	= 14,
	IET_WEB			= 15,
	IET_ENERGY		= 16,
	IET_POISON		= 17,
	IET_BLIND		= 18,
	IET_ALLSTATUS	= 19,
	IET_MAX
};

enum FoodSortType
{
	FOODSORT_BYTYPE			= 0,
	FOODSORT_BYHEAL			= 1,
	FOODSORT_BYINGREDIENT	= 2,
	MAX_FOODSORT
};

//	FOODSORT_UNSORTED		= 0,

struct IngredientEffect
{
	IngredientEffect() : type(IET_NONE), magnitude(0) {}
	float magnitude;
	IngredientEffectType type;
	std::string string;
};

class IngredientData
{
public:
	IngredientData(const std::string &name, const std::string &gfx, IngredientType type)
		: name(name), gfx(gfx), amount(0), held(0), type(type), marked(0), sorted(false) {}
	int getIndex() const;
	const std::string name, gfx;
	const IngredientType type;
	int amount;
	int held;
	int marked;
	bool sorted;
	bool hasIET(IngredientEffectType iet);

	typedef std::vector<IngredientEffect> IngredientEffects;
	IngredientEffects effects;
private:
	// ensure that IngredientData instances are never copied:
	IngredientData(const IngredientData&);
	const IngredientData& operator=(const IngredientData&);
};
typedef std::vector<IngredientData*> IngredientDatas;

class IngredientDescription
{
public:
	std::string text;
};
typedef std::vector<IngredientDescription> IngredientDescriptions;

struct RecipeType
{
	RecipeType(IngredientType type, const std::string &typeName) : type(type), amount(1) { this->typeName = typeName; }
	RecipeType() { amount = 1; type = IT_NONE; }
	IngredientType type;
	int amount;
	std::string typeName;
};

struct RecipeName
{
	RecipeName(const std::string &name) : name(name), amount(1) {}
	RecipeName() : amount(1) {}
	std::string name;
	int amount;
};

class Recipe
{
public:
	Recipe();
	std::vector<RecipeType> types;
	std::vector<RecipeName> names;
	std::string result;

	int index;


	void addName(const std::string &name);
	void addType(IngredientType type, const std::string &typeName);
	void clear();
	void learn();

	bool isKnown() { return known; }
protected:
	bool known;
};

struct PECue
{
	PECue(std::string name, Vector pos, float rot, float t)
		: name(name), pos(pos), rot(rot), t(t) {}
	std::string name;
	Vector pos;
	float rot;
	float t;
};

struct EatData
{
	EatData() { ammoUnitSize=getUnits=1; health=0; ammo=1;}
	std::string name, shot;
	int ammoUnitSize, getUnits, ammo;
	float health;
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

enum CMStat
{
	CM_ID	=0,
	CM_EGO,
	CM_SEGO
};

class WordColoring
{
public:
	std::string word;
	Vector color;
};

struct SporeChildData
{
	SporeChildData() : state(0), entity(0), health(0) {}
	int state;
	int health;
	Entity *entity;
};

const int FLAG_LI = 1000, FLAG_LICOMBAT = 1001;

const int FLAG_COOKS			= 21;

const int FLAG_PET_ACTIVE		= 600;
const int FLAG_PET_NAMESTART	= 601;

const int FLAG_UPGRADE_WOK		= 620;

const int FLAG_SEALOAFANNOYANCE = 801;

const int FLAG_SECRET01 = 234;
const int FLAG_SECRET02	= 235;
const int FLAG_SECRET03 = 236;

const int FLAG_COLLECTIBLE_START = 500;
const int FLAG_COLLECTIBLE_END = 600;

/*
const int FLAG_PET_NAUTILUS		= 601;
const int FLAG_PET_DUMBO		= 602;
const int FLAG_PET_BLASTER		= 603;
const int FLAG_PET_PIRANHA		= 604;
*/

struct PetData
{
	std::string namePart;
};

struct TreasureDataEntry
{
	TreasureDataEntry() { sz = 1; use = 0;}
	std::string gfx;
	float sz;
	int use;
};

struct FoodSortOrder
{
	FoodSortOrder(IngredientType t, IngredientEffectType et = IET_NONE, std::string name="", int effectAmount=0)
	{ type = t; effectType = et; this->name = name; this->effectAmount=effectAmount;}
	FoodSortOrder() { type = IT_NONE; effectType = IET_NONE; }
	std::string name;
	IngredientType type;
	IngredientEffectType effectType;
	int effectAmount;
};

typedef std::map<int, TreasureDataEntry> TreasureData;

#include "StatsAndAchievements.h"

class Continuity
{
public:
	Continuity();
	~Continuity() { clearIngredientData(); }
	void init();
	void shutdown();
	void initAvatar(Avatar *a);
	void refreshAvatarData(Avatar *a);
	void reset();
	bool hasItem(int type);
	void pickup(int type, int amount=1);
	void drop(int type);

	void entityDied(Entity *eDead);
	
	void achieve(const std::string &achievement);

	void initFoodSort();
	void sortFood();

	bool isIngredientFull(IngredientData *data);

	void setCostume(const std::string &c);

	void shortenSong(Song &song, int size);
	void warpLiToAvatar();

	void flingMonkey(Entity *e);

	void upgradeHealth();

	int  getFlag(std::string flag);
	void setFlag(std::string flag, int v);

	int getFlag(int flag);
	void setFlag(int flag, int v);

	int getEntityFlag(const std::string &sceneName, int id);
	void setEntityFlag(const std::string &sceneName, int id, int v);

	void setPathFlag(Path *p, int v);
	int getPathFlag(Path *p);

	std::string getStringFlag(std::string flag);
	void		setStringFlag(std::string flag, std::string v);

	void saveFile(int slot, Vector position=Vector(0,0,0), unsigned char *scrShotData=0, int scrShotWidth=0, int scrShotHeight=0);
	void loadFileData(int slot, TiXmlDocument &doc);
	void loadFile(int slot);

	void castSong(int num);

	bool hasLi();

	std::string getDescriptionForSongSlot(int songSlot);
	std::string getVoxForSongSlot(int songSlot);

	std::string getIEString(IngredientData *data, int i);
	std::string getAllIEString(IngredientData *data);

	std::string getInternalFormName();

	std::string getSaveFileName(int slot, const std::string &pfix);

	int maxHealth;
	float health;
	bool hudVisible;
	unsigned int exp;

	void clearTempFlags();
	void getHoursMinutesSeconds(int *hours, int *minutes, int *seconds);
	float seconds;

	void update(float dt);

	void setItemSlot(int slot, int itemType);

	std::vector<int> itemSlots;

	bool isItemPlantable(int item);

	float getCurrentTime(){return seconds;}

	//GardenData gardenData;
	Vector zoom;

	std::string getIngredientGfx(const std::string &name);


	WorldType getWorldType() { return worldType; }
	void shiftWorlds();
	void applyWorldEffects(WorldType type, bool transition, bool affectMusic);


	//void setActivePet(int flag);

	bool isStory(float v);
	float getStory();
	void setStory(float v);

	int getSpeedType(int speedType);
	void setNaijaModel(std::string model);


	std::string naijaModel;

	std::vector<WordColoring> wordColoring;

	FormType form;

	void learnFormUpgrade(FormUpgradeType form);
	bool hasFormUpgrade(FormUpgradeType form);

	typedef std::map<FormUpgradeType, bool> FormUpgrades;
	FormUpgrades formUpgrades;

	void loadSongBank();
	int getSongBankSize();
	void loadIntoSongBank(const std::string &file);
	int checkSong(const Song &song);
	int checkSongAssisted(const Song &song);
	typedef std::map<int, Song> SongMap;
	SongMap songBank;

	Song *getSongByIndex(int idx);

	
	bool hasSong(int song);
	int getSongTypeBySlot(int slot);
	int getSongSlotByType(int type);
	void learnSong(int song);
	void unlearnSong(int song);
	std::map<int, bool> knowsSong;
	std::map<int, int> songSlotsToType;
	std::map<int, int> songTypesToSlot;

	std::map<int, std::string> songSlotDescriptions;
	std::map<int, std::string> songSlotNames;
	std::map<int, std::string> songSlotVox;

	typedef std::map<std::string, int> EntityFlags;
	EntityFlags entityFlags;

	bool toggleMoveMode;

	typedef std::list<GemData> Gems;
	Gems gems;
	
	typedef std::list<BeaconData> Beacons;
	Beacons beacons;

	GemData *pickupGem(std::string name, bool effects = true);
	void removeGemData(GemData *gemData);


	typedef std::vector<std::string> VoiceOversPlayed;
	VoiceOversPlayed voiceOversPlayed;

	std::string costume;

	AuraType auraType;
	float auraTimer;

	SporeChildData *getSporeChildDataForEntity(Entity *e);
	void registerSporeChildData(Entity *e);

	std::vector<SporeChildData> sporeChildData;


	EatData *getEatData(const std::string &name);
	void loadEatBank();

	bool isSongTypeForm(SongType s);

	std::string getSongNameBySlot(int slot);
	void toggleLiCombat(bool t);

	void pickupIngredient(IngredientData *i, int amount, bool effects=true);
	int indexOfIngredientData(const IngredientData* data) const;
	IngredientData *getIngredientHeldByName(const std::string &name) const; // an ingredient that the player actually has; in the ingredients list
	IngredientData *getIngredientDataByName(const std::string &name); // an ingredient in the general data list; ingredientData

	IngredientData *getIngredientHeldByIndex(int idx) const;
	IngredientData *getIngredientDataByIndex(int idx);

	void applyIngredientEffects(IngredientData *data);

	void loadIngredientData();
	bool hasIngredients() const { return !ingredients.empty(); }
	IngredientDatas::size_type ingredientCount() const { return ingredients.size(); }
	IngredientType getIngredientTypeFromName(const std::string &name) const;

	void removeEmptyIngredients();
	void spawnAllIngredients(const Vector &position);
	
	std::vector<std::string> unsortedOrder;

	typedef std::vector<Recipe> Recipes;
	Recipes recipes;

	void setSpeedMultiplier(float s, float t);
	void setBiteMultiplier(float m, float t);
	void setFishPoison(float m, float t);
	void setDefenseMultiplier(float s, float t);
	void setRegen(float t);
	void setTrip(float t);
	void setInvincible(float t);
	void setEnergy(float m, float t);
	void setPoison(float m, float t);
	void setWeb(float t);
	void setLight(float m, float t);
	void setPetPower(float m, float t);
	void setLiPower(float m, float t);

	void cureAllStatus();

	float speedMult, biteMult, fishPoison, defenseMult, energyMult, poison, light, petPower, liPower;
	Timer speedMultTimer, biteMultTimer, fishPoisonTimer, defenseMultTimer, liPowerTimer;
	Timer invincibleTimer;
	Timer regenTimer, tripTimer;
	Timer energyTimer, poisonTimer, poisonBitTimer;
	Timer webTimer, webBitTimer, lightTimer, petPowerTimer;

	void eatBeast(const EatData &eatData);
	void removeNaijaEat(int idx);
	void removeLastNaijaEat();
	EatData *getLastNaijaEat();
	bool isNaijaEatsEmpty();

	void loadPetData();
	PetData *getPetData(int idx);

	std::vector<EatData> naijaEats;

	std::vector<PetData> petData;

	IngredientDescriptions ingredientDescriptions;

	std::string getIngredientAffectsString(IngredientData *data);
	std::string getIngredientDescription(IngredientEffectType type);

	WorldMap worldMap;

	StringBank stringBank;

	TreasureData treasureData;

	void loadTreasureData();

	void learnRecipe(Recipe *r, bool effects=true);
	void learnRecipe(const std::string &result, bool effects=true);

	float poisonBitTime, poisonBitTimeAvatar;

	MenuPage lastMenuPage, lastOptionsMenuPage;

	enum { DUALFORM_NAIJA = 0, DUALFORM_LI = 1 };
	int dualFormMode, dualFormCharge;
	
	BeaconData *getBeaconByIndex(int index);
	void setBeacon(int index, bool v, Vector pos=Vector(0,0,0), Vector color=Vector(1,1,1));
	
	int foodSortType;
	std::vector<FoodSortOrder> sortByType, sortByHeal, sortByIngredients, sortByUnsort;

	StatsAndAchievements *statsAndAchievements;
protected:
	std::vector<EatData> eats;
	std::vector<int> speedTypes;
	float story;
	WorldType worldType;

	std::vector<int> items;
	std::vector<int> spells;
	typedef std::map<std::string,int> Flags;
	Flags flags;

	int intFlags[MAX_FLAGS];
	typedef std::map<std::string,std::string> StringFlags;
	StringFlags stringFlags;
private:
	void clearIngredientData();

	IngredientDatas ingredients; // held ingredients
	IngredientDatas ingredientData; // all possible ingredients
};

class Profile
{
public:
	Profile();
	std::string name;
};

enum SaveSlotMode
{
	SSM_NONE = -1,
	SSM_SAVE = 0,
	SSM_LOAD = 1
};

extern GameplayVariables *vars;

#include "UserSettings.h"

struct DemoFrame
{
	float t;
	Vector avatarPos, vel, vel2;
	Mouse mouse;
	float rot;
};

class Demo
{
public:
	enum {
		DEMOMODE_NONE		= -1,
		DEMOMODE_RECORD		= 0,
		DEMOMODE_PLAYBACK	= 1
	};
	Demo();
	void toggleRecord(bool on);
	void togglePlayback(bool on);
	void renderFramesToDisk();
	void clearRecordedFrames();

	void update(float dt);

	bool getQuitKey();

	void save(const std::string &name);
	void load(const std::string &name);

	int frame;
	float time;
	float timeDiff;
	std::vector <DemoFrame> frames;
	int mode;
};

enum NagType
{
	NAG_TOTITLE		= 0,
	NAG_QUIT		= 1
};

class DSQ : public Core
{
public:
	DSQ(std::string fileSystem);
	~DSQ();

	void init();
	void shutdown();
	
	void toggleInputGrabPlat(bool on);

	void toggleBlackBars(bool on, float t=0);

	void setCursor(CursorType type);

	Quad *cursor, *cursorGlow, *cursorBlinker;
	Quad *overlay, *tfader, *overlay2, *overlay3, *overlayRed;
	Quad *sceneColorOverlay;
	Quad *bar_left, *bar_right, *bar_up, *bar_down;
	Quad *barFade_left, *barFade_right;

	Texture *texCursor, *texCursorSwim, *texCursorBurst, *texCursorSing, *texCursorLook;

	void setBlackBarsColor(Vector color);
	
	void toggleFullscreen();

	void setTexturePointers();

	void doScript(const std::string &script);

	void fade(float alpha, float time);
	void print(int x, int y, const std::string &text);

	void applyParallaxUserSettings();

	void lockMouse();

	void nag(NagType type);

	void action(int id, int state);

	void title(bool fadeMusic=true);

	void cutsceneEffects(bool on);

	bool isScriptRunning();
	void delay(float dt); // active delay - game continues to run

	void newGame();

	Game *game;

	bool isQuitFlag();
	
	void jiggleCursor();

	SFXLoops loops;
	SubtitlePlayer subtitlePlayer;

	void onPlayedVoice(const std::string &name);

	NagType nagType;

	int getEntityLayerToLayer(int layer);

	void addElement(Element *e);
	int getNumElements() const {return elements.size();}
	Element *getElement(int idx) const {return elements[idx];}
	Element *getFirstElementOnLayer(int layer) const {return layer<0 || layer>15 ? 0 : firstElementOnLayer[layer];}
	Element *getElementWithType(Element::Type type);
	void clearElements();
	// Used only by scene editor:
	void removeElement(int idx);
	void removeElement(Element *e);
	ElementContainer getElementsCopy() const {return elements;}

protected:  // These should never be accessed from outside (use the functions above).
	ElementContainer elements;
	Element *firstElementOnLayer[16];
public:

	void addEntity(Entity *entity);
	void removeEntity(Entity *e);
	void clearEntities();

	EntityContainer entities;

	bool useFrameBuffer;
	Continuity continuity;
	GameplayVariables v;
	Emote emote;

	void playVisualEffect(int vfx, Vector position, Entity *target=0);
	void playNoEffect();

	typedef std::vector<std::string> StringList;
	StringList profiles;

	Profile currentProfile;

	AquariaScreenTransition *screenTransition;

	Precacher precacher;

	Entity *getFirstEntity();
	Entity *getNextEntity();

	std::string initScene;

	bool modIsSelected;

	void toggleMuffleSound(bool toggle);
	void toggleInputMode();
	void shakeCamera(float mag, float time);
	Vector avStart;
	Entity *getEntityByName(std::string name);
	Entity *getEntityByNameNoCase(std::string name);

	void doSavePoint(const Vector &position);
	std::string getEntityFlagName(Entity *e);
	std::string getUserInputString(std::string label, std::string t="", bool allowNonLowerCase=false);
	Vector getUserInputDirection(std::string label);
	Vector getCameraCenter();
	bool onPickedSaveSlot(AquariaSaveSlot *slot);
	void doSaveSlotMenu(SaveSlotMode ssm, const Vector &position = Vector(0,0,0));
	void doModSelect();
	void doLoadMenu();
	void onExitSaveSlotMenu();
	ScriptInterface scriptInterface;
	bool runScript(const std::string &name, const std::string &func="");
	bool runScriptNum(const std::string &name, const std::string &func="", float num=0);
	void collectScriptGarbage();

	void spawnParticleEffect(const std::string &name, Vector position, float rot=0, float t=0, int layer=LR_PARTICLES, float follow=0);
	void spawnAllIngredients(const Vector &position);

	std::string getDialogueFilename(const std::string &f);

	bool isShakingCamera();
	Element *getSolidElementNear(Vector pos, int rad);

	std::string languagePack;

	int getEntityTypeIndexByName(std::string s);
	void screenMessage(const std::string &msg);
#ifdef AQUARIA_BUILD_CONSOLE  // No need to override it otherwise.
	void debugLog(const std::string &s);
#endif
	void toggleConsole();
	void toggleEffects();
	void debugMenu();

	std::string dialogueFile;

	void takeScreenshot();
	void takeScreenshotKey();

	void jumpToSection(std::ifstream &inFile, const std::string &section);

	PathFinding pathFinding;
	void runGesture(const std::string &line);
	void generateCollisionMask(RenderObject *r);
	void toggleRenderCollisionShapes();

	void voice(const std::string &file, float volMod = -1);
	void voiceOnce(const std::string &file);
	void voiceInterupt(const std::string &file);
	void stopVoice();
	Vector getNoteColor(int note);
	int getRandNote();
	Vector getNoteVector(int note, float mag=1);
	void toggleCursor(bool v, float t = -1);

	bool isDeveloperKeys();

	void loadElementEffects();
	ElementEffect getElementEffectByIndex(int e);
	typedef std::vector<ElementEffect> ElementEffects;
	ElementEffects elementEffects;

	bool playedVoice(const std::string &file);

	bool voiceOversEnabled;
	int recentSaveSlot;

	void playPositionalSfx(const std::string &name, const Vector &position, float freq=1.0, float fadeOut=0);

	void playMenuSelectSfx();

	InterpolatedVector gameSpeed;

	InputMode inputMode;
	void setInputMode(InputMode mode);

	void rumble(float leftMotor, float rightMotor, float time);
	void vision(std::string folder, int num, bool ignoreMusic = false);

	bool useMic, autoSingMenuOpen;
	void watch(float t, int canQuit = 0);

	std::string lastVoiceFile;

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

	bool doScreenTrans;

	AquariaSaveSlot *selectedSaveSlot;
	void setStory();

	bool disableMiniMapOnNoInput;
	
	std::string returnToScene;

	Demo demo;

	DebugFont *fpsText, *cmDebug;
#ifdef AQUARIA_BUILD_CONSOLE
	DebugFont *console;
#endif
	BitmapText *versionLabel;
	ProfRender *profRender;
	
	void setVersionLabelText();

	float menuSelectDelay;
	float timer;

	Mod mod;

	void loadMods();

	std::vector<ModEntry> modEntries;
	int selectedMod;
	ModSelector *modSelector;

	void startSelectedMod();
	void selectNextMod();
	void selectPrevMod();
	ModEntry* getSelectedModEntry();

#ifdef BBGE_BUILD_ACHIEVEMENTS_INTERNAL
	BitmapText *achievement_text;
	Quad *achievement_box;
#endif

	BitmapText *subtext;
	Quad *subbox;

	BmpFont font, smallFont, subsFont, goldFont, smallFontRed;
	TTFFont fontArialSmall, fontArialBig, fontArialSmallest;
	unsigned char *arialFontData;
	unsigned long arialFontDataSize;

	void loadFonts();

	
	void instantQuit();

	void centerText(const std::string &text);
	void centerMessage(const std::string &text, float y=300, int type=0);

	bool routeShoulder;

	void toggleVersionLabel(bool on);

	void onConfirmYes();
	void onConfirmNo();

	bool confirm(const std::string &text, const std::string &image="", bool ok=false, float countdown=0.0f);

	std::string particleBank1;
	std::string particleBank2;

	std::string shotBank1;
	std::string shotBank2;


	int dsq_filter;
	void setFilter(int dsqFilterCode);


	enum Difficulty
	{
		DIFF_NORMAL		= 0,
		DIFF_EASY		= 1
	};

	Difficulty difficulty;
	
	std::string getSaveDirectory();

	void clickRingEffect(Vector position, int type=0, Vector color=Vector(1,1,1), float ut=0);
	
	void bindInput();
	
	void forceInputGrabOff();

	int weird;

	void setCutscene(bool on, bool canSkip=false);
	bool isInCutscene();
	bool isCutscenePaused();
	void pauseCutscene(bool on);
	bool canSkipCutscene();
	bool isSkippingCutscene();
protected:

	Quad *cutscene_bg;
	BitmapText *cutscene_text;
	BitmapText *cutscene_text2;

	bool cutscenePaused;
	bool inCutscene;
	bool _canSkipCutscene;
	bool skippingCutscene;

	ActionInput *almb, *armb;

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
	void onAltTab();

	void onPlayVoice();
	void onStopVoice();

	Entity **iter;
	Quad *blackout;
	void updatepecue(float dt);
	std::vector<PECue> pecue;

	bool developerKeys;
	void onMouseInput();
	std::vector<std::string> voxQueue;

#ifdef AQUARIA_BUILD_CONSOLE
	std::vector<std::string> consoleLines;
#endif

	std::vector <AquariaSaveSlot*> saveSlots;

	BitmapText *expText, *moneyText;
	TiXmlDocument *xmlDoc;

	void clearMenu(float t = 0.01);
	std::vector <RenderObject*> menu;
	BitmapText *saveSlotPageCount;

	void updateSaveSlotPageCount();

	float shakeCameraTimer;
	float shakeCameraMag;
	std::string currentPortrait;

	void onUpdate(float dt);
	void onRender();

	void modifyDt(float &dt);
};

extern DSQ *dsq;

#endif

