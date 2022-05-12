#ifndef IN_GAME_MENU_H
#define IN_GAME_MENU_H

#include <string>
#include <vector>

#include "ActionMapper.h"

class BitmapText;
class ToolTip;
class Quad;
class AquariaMenuItem;
class RenderObject;
class SongSlot;
class FoodSlot;
class TreasureSlot;
class IngredientData;
class AquariaKeyConfig;
class AquariaCheckBox;
class AquariaSlider;
class AquariaComboBox;
class RecipeMenuEntry;
class Recipe;
class RoundedRect;
class DebugButton;
class TTFText;
class AquariaButton;


enum MenuPage
{
	MENUPAGE_NONE		= -1,
	MENUPAGE_SONGS		= 0,
	MENUPAGE_FOOD		= 1,
	MENUPAGE_TREASURES	= 2,
	MENUPAGE_PETS		= 3
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
	size_t getNumPages();
	size_t getNumKnown();

	int currentPage;

	bool on;

	std::vector<RecipeMenuEntry*> recipeMenuEntries;
};


class InGameMenu : public ActionMapper
{
	// These are defined in the cpp file and are exclusively used in menu code
	friend class FoodSlot;
	friend class PetSlot;
	friend class TreasureSlot;
	friend class SongSlot;
	friend class FoodHolder;
	friend class KeyConfigMenuReceiver;
	enum { NUM_KEY_CONFIG_PAGES = 3 };

public:
	InGameMenu();
	~InGameMenu();

	void create();
	void update(float dt);
	void hide(bool effects=true, bool cancel=false);
	void show(bool force=false, bool optionsOnly=false, MenuPage menuPage = MENUPAGE_NONE);
	bool isInGameMenu() const { return inGameMenu; }
	bool isFoodMenuOpen() const { return foodMenu; }
	MenuPage getCurrentMenuPage() const { return currentMenuPage; }
	void reset();
	void onContinuityReset();

	void bindInput();
	virtual void action(int actionID, int state, int source, InputDevice device);

	void refreshFoodSlots(bool effects);
	
	RecipeMenu recipeMenu;

	float menuSelectDelay;

	bool isInKeyConfigMenu() const { return keyConfigMenu; }

private:
	void updateOptionsMenu(float dt);
	void updateKeyConfigMenu(float dt);

	void sortFood();
	void updatePreviewRecipe();
	void adjustFoodSlotCursor();
	void playSongInMenu(int songType, bool override=false);

	void refreshTreasureSlots();

	Recipe *findRecipe(const std::vector<IngredientData*> &list);
	void onCook();
	void onRecipes();
	void updateCookList();
	void onUseTreasure();
	void onUseTreasure(int flag);

	void onPrevFoodPage();
	void onNextFoodPage();

	void onPrevTreasurePage();
	void onNextTreasurePage();

	void showInGameMenuExitCheck();
	void hideInGameMenuExitCheck(bool refocus);

	void setMenuDescriptionText(const std::string &text);
	FoodSlot* getFoodSlotFromIndex();

	BitmapText *songLabel, *foodLabel, *foodDescription, *treasureLabel;
	ToolTip *treasureDescription;
	Quad *treasureCloseUp;

	Quad *menuBg, *menuBg2;

	AquariaMenuItem *eYes, *eNo, *cook, *recipes, *nextFood, *prevFood, *nextTreasure, *prevTreasure, *use, *keyConfigButton;
	AquariaMenuItem *opt_cancel, *opt_save, *foodSort;

	Quad *menu_blackout;

	std::vector<SongSlot*> songSlots;
	std::vector<FoodSlot*> foodSlots;
	std::vector<TreasureSlot*> treasureSlots;
	BitmapText* songDescription;

	int selectedTreasureFlag;

	bool optionsOnly;

	float cookDelay;

	MenuPage currentMenuPage;
	int currentFoodPage, currentTreasurePage;

	int enqueuedPreviewRecipe;

	Quad *previewRecipe;
	Quad *menuIconGlow;
	Quad *showRecipe;

	bool isCooking;
	float optsfxdly;

	void doMenuSectionHighlight(int sect);

	void onPrevRecipePage();
	void onNextRecipePage();
	void onDebugSave();

	typedef std::vector<IngredientData*> CookList;
	CookList cookList;
	std::vector<std::string> dropIngrNames;

	float songMenuPlayDelay;
	size_t currentSongMenuNote;
	int playingSongInMenu;

	void onOptionsMenu();
	bool optionsMenu, foodMenu, petMenu, treasureMenu, keyConfigMenu;
	void toggleOptionsMenu(bool f, bool skipBackup=false, bool isKeyConfig=false);
	void toggleFoodMenu(bool f);
	void toggleMainMenu(bool f);
	void togglePetMenu(bool f);
	void toggleTreasureMenu(bool f);
	void toggleRecipeList(bool on);
	void toggleKeyConfigMenu(bool f);

	void enqueuePreviewRecipe();

	void switchToSongMenu();
	void switchToFoodMenu();
	void switchToPetMenu();
	void switchToTreasureMenu();

	void onKeyConfig();

	void addKeyConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int x, int y, bool rejectJoyAxis = false);

	AquariaKeyConfig *addAxesConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int offx, int y);

	void onOptionsSave();
	void onOptionsCancel();
	AquariaSlider *sfxslider, *musslider, *voxslider;
	AquariaCheckBox *autoAimCheck, *targetingCheck, *toolTipsCheck, *flipInputButtonsCheck, *blurEffectsCheck;
	AquariaCheckBox *subtitlesCheck, *fullscreenCheck, *ripplesCheck;
	AquariaComboBox *resBox;
	Quad *songBubbles, *energyIdol, *liCrystal;

	RenderObject *group_keyConfig[NUM_KEY_CONFIG_PAGES];
	RoundedRect *keyConfigBg;
	std::vector<AquariaButton*> keyCategoryButtons;
	std::vector<AquariaKeyConfig*> keyConfigs;
	RenderObject *createBasicKeyConfig();
	void switchToKeyConfigPage(int page);
	void switchToKeyConfigPage1();
	void switchToKeyConfigPage2();
	void switchToKeyConfigPage3();
	Quad *options;
	AquariaComboBox *actionSetBox;
	AquariaCheckBox *actionSetCheck;
	int selectedActionSetIdx;
	TTFText *joystickNameText, *joystickGUIDText, *joystickButtonsText;
	TTFText *keyConfigInstructions;
	void updateActionSetComboBox();
	void switchToActionSet(int idx);
	void nextJoystick();
	void updateJoystickText();

	void onExitCheckNo();
	void onExitCheckYes();
	void onInGameMenuContinue();
	void onInGameMenuOptions();
	void onInGameMenuExit();

	BitmapText *circlePageNum;

	std::vector<ToolTip*> foodTips, songTips, petTips, treasureTips;

	Quad *eAre;
	int inGameMenuExitState;

	BitmapText *menuDescription;

	std::vector<AquariaMenuItem*> menu;

	bool inGameMenu;
	float menuOpenTimer;

	FoodSlot *moveFoodSlotToFront;
	AquariaMenuItem *lips;

	MenuPage lastMenuPage, lastOptionsMenuPage;
};


#endif
