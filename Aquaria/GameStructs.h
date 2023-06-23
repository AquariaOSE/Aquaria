#ifndef GAMESTRUCTS_H
#define GAMESTRUCTS_H

#include <vector>
#include <string>
#include "Vector.h"
#include "GameEnums.h"

class Path;

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
	BlendType blendType;
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

struct GemData
{
	GemData() { canMove=false; blink = false; }
	std::string name;
	std::string userString;
	std::string mapName;
	Vector pos;
	bool canMove;
	bool blink; // not saved
};

struct BeaconData
{
	BeaconData(){ index=-1; on=0; }
	int index;
	Vector pos,color;
	bool on;
};


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

struct EatData
{
	EatData() { ammoUnitSize=getUnits=1; health=0; ammo=1;}
	std::string name, shot;
	int ammoUnitSize, getUnits, ammo;
	float health;
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
	std::string resultDisplayName;

	int index;


	void addName(const std::string &name);
	void addType(IngredientType type, const std::string &typeName);
	void clear();
	void learn();

	bool isKnown() { return known; }
protected:
	bool known;
};


typedef std::vector<int> SongNotes;

struct Song
{
	Song() { index=0; script=0; }
	int index;
	SongNotes notes;
	int script;
};

class Emote
{
public:
	Emote();
	void load(const std::string &file);
	void playSfx(size_t index);
	void update(float dt);

	float emoteTimer;
	int lastVariation;

	typedef std::vector<EmoteData> Emotes;
	Emotes emotes;
};

struct IngredientEffect
{
	IngredientEffect() : magnitude(0), type(IET_NONE) {}
	float magnitude;
	IngredientEffectType type;
	std::string string;
};

class IngredientData
{
public:
	IngredientData(const std::string &name, const std::string &gfx, IngredientType type);
	int getIndex() const;
	const std::string name, gfx;
	std::string displayName;
	const IngredientType type;
	int amount;
	int maxAmount;
	int held;
	int marked;
	bool sorted;
	bool rotKind;
	bool hasIET(IngredientEffectType iet);

	typedef std::vector<IngredientEffect> IngredientEffects;
	IngredientEffects effects;
private:
	// ensure that IngredientData instances are never copied:
	IngredientData(const IngredientData&);
	const IngredientData& operator=(const IngredientData&);
};
typedef std::vector<IngredientData*> IngredientDatas;


struct UnderWaterResult
{
	bool uw;
	Path *waterbubble;
};


#endif
