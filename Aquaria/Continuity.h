#ifndef CONTINUITY_H
#define CONTINUITY_H

#include <map>
#include <vector>
#include <string>
#include "StatsAndAchievements.h"
#include "GameEnums.h"
#include "WorldMap.h"

namespace tinyxml2 { class XMLDocument; }

class Avatar;
class IngredientData;
class Path;

typedef std::map<int, TreasureDataEntry> TreasureData;


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

	void shortenSong(Song &song, size_t size);
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
	bool loadFileData(int slot, tinyxml2::XMLDocument &doc);
	bool loadFile(int slot);

	void castSong(int num);

	bool hasLi();

	std::string getDescriptionForSongSlot(int songSlot);
	std::string getVoxForSongSlot(int songSlot);

	std::string getIEString(IngredientData *data, size_t i);
	std::string getAllIEString(IngredientData *data);

	std::string getInternalFormName();

	std::string getSaveFileName(int slot, const std::string &pfix);

	float maxHealth;
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

	float getSpeedType(size_t speedType);

	FormType form;

	void learnFormUpgrade(FormUpgradeType form);
	bool hasFormUpgrade(FormUpgradeType form);

	typedef std::map<FormUpgradeType, bool> FormUpgrades;
	FormUpgrades formUpgrades;

	void loadSongBank();
	void loadIntoSongBank(const std::string &file);
	int checkSong(const Song &song);
	int checkSongAssisted(const Song &song);
	typedef std::map<int, Song> SongMap;
	SongMap songBank;

	Song *getSongByIndex(int idx);


	bool hasSong(int song);
	int getSongTypeBySlot(size_t slot);
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

	EatData *getEatData(const std::string &name);
	void loadEatBank();

	bool isSongTypeForm(SongType s);

	std::string getSongNameBySlot(int slot);
	void toggleLiCombat(bool t);

	void pickupIngredient(IngredientData *i, int amount, bool effects=true, bool learn=true);
	int indexOfIngredientData(const IngredientData* data) const;
	IngredientData *getIngredientHeldByName(const std::string &name) const; // an ingredient that the player actually has; in the ingredients list
	IngredientData *getIngredientDataByName(const std::string &name); // an ingredient in the general data list; ingredientData

	IngredientData *getIngredientHeldByIndex(size_t idx) const;
	IngredientData *getIngredientDataByIndex(size_t idx);

	int getIngredientDataSize() const;
	int getIngredientHeldSize() const;

	bool applyIngredientEffects(IngredientData *data);

	void loadIngredientData();
	void loadIngredientData(const std::string &file);
	void loadIngredientDisplayNames(const std::string& file);
	bool hasIngredients() const { return !ingredients.empty(); }
	IngredientDatas::size_type ingredientCount() const { return ingredients.size(); }
	IngredientType getIngredientTypeFromName(const std::string &name) const;
	std::string getIngredientDisplayName(const std::string& name) const;

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

	float speedMult2;

	void eatBeast(const EatData &eatData);
	void removeNaijaEat(size_t idx);
	void removeLastNaijaEat();
	EatData *getLastNaijaEat();
	bool isNaijaEatsEmpty();

	void loadPetData();
	PetData *getPetData(size_t idx);

	std::vector<EatData> naijaEats;

	std::vector<PetData> petData;

	std::string getIngredientAffectsString(IngredientData *data);

	WorldMap worldMap;

	TreasureData treasureData;

	void loadTreasureData();

	void learnRecipe(Recipe *r, bool effects=true);
	void learnRecipe(const std::string &result, bool effects=true);

	float poisonBitTime, poisonBitTimeAvatar;

	enum { DUALFORM_NAIJA = 0, DUALFORM_LI = 1 };
	int dualFormMode, dualFormCharge;

	BeaconData *getBeaconByIndex(int index);
	void setBeacon(int index, bool v, Vector pos=Vector(0,0,0), Vector color=Vector(1,1,1));

	int foodSortType;
	std::vector<FoodSortOrder> sortByType, sortByHeal, sortByIngredients, sortByUnsort;

	StatsAndAchievements *statsAndAchievements;

protected:
	std::vector<EatData> eats;
	std::vector<float> speedTypes;
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

	typedef std::map<std::string,std::string> IngredientNameMap;
	IngredientNameMap ingredientDisplayNames;
};


#endif
