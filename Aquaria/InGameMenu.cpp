#include "InGameMenu.h"

#include "DSQ.h"
#include "Game.h"
#include "AquariaMenuItem.h"
#include "Quad.h"
#include "ToolTip.h"
#include "DSQ.h"
#include "Avatar.h"
#include "GridRender.h"
#include "DebugFont.h"
#include "ActionSet.h"

static InGameMenu *themenu = 0;

// defined below
class FoodHolder;
class PetSlot;

static std::vector<FoodHolder*>	foodHolders;
static std::vector<PetSlot*>		petSlots;

// ---------------- Constants ----------------------------

static const float MENUPAGETRANSTIME		= 0.2f;
const Vector menuBgScale(800.0f/1024.0f, 800.0f/1024.0f);
const size_t foodPageSize = 16;
const size_t treasurePageSize = 16;
const size_t numTreasures = 16*2;

const Vector worldLeftCenter(217,250), worldRightCenter(575, 250);
const Vector opt_save_original = Vector(350, 350), opt_cancel_original = Vector(450, 350);

static const int KEYCONFIG_FIRST_COL_DISTANCE = 170;
static const int KEYCONFIG_COL_DISTANCE = 105;


// --------- Private class defs, not used outside ---------------

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

int FoodSlot::foodSlotIndex = -1;


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
	FoodHolder(size_t slot, bool trash=false);

	bool isValid() const;
	void toggleValid(bool b);
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

	size_t slot;
private:
	IngredientData *foodHolderIngredient;
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

// ------------- Private class impls -------------------


PetSlot::PetSlot(int pet) : AquariaGuiQuad()
{
	PetData *p = dsq->continuity.getPetData(pet);
	if (p)
	{
		std::string fn = "collectibles/egg-" + p->namePart;
		setTexture(fn);
	}
	scale = Vector(0.9f, 0.9f);
	petidx = pet;
	mouseDown = false;
	petFlag = FLAG_PET_NAMESTART + petidx;
	wasSlot = false;
}

void PetSlot::onUpdate(float dt)
{
	AquariaGuiQuad::onUpdate(dt);

	if (!dsq->continuity.getFlag(petFlag))
	{
		if (!wasSlot)
		{
			setTexture("gui/wok");
			setWidthHeight(80);
			wasSlot = true;
		}
		//alphaMod = 0;
		return;
	}
	else
	{
		alphaMod = 1;

		if (wasSlot)
		{
			PetData *p = dsq->continuity.getPetData(petidx);
			if (p)
			{
				std::string fn = "collectibles/egg-" + p->namePart;
				setTexture(fn);
			}
			wasSlot = false;
		}
	}

	if (dsq->continuity.getFlag(FLAG_PET_ACTIVE) == petFlag)
	{
		color = Vector(1,1,1);
	}
	else
		color = Vector(0.5f, 0.5f, 0.5f);

	if (alpha.x < 1) return;

	if ((core->mouse.position - getWorldPosition()).isLength2DIn(32))
	{
		scale.interpolateTo(Vector(1.2f, 1.2f), 0.1f);

		if (core->mouse.buttons.left && !mouseDown)
		{
			mouseDown = true;
		}
		else if (!core->mouse.buttons.left && mouseDown)
		{
			dsq->sound->playSfx("click");

			if (dsq->continuity.getFlag(FLAG_PET_ACTIVE) == petFlag)
			{
				game->setActivePet(0);
				dsq->sound->playSfx("pet-on");
			}
			else
			{
				game->setActivePet(FLAG_PET_NAMESTART + petidx);
				dsq->sound->playSfx("pet-off");
			}
			mouseDown = false;
		}
	}
	else
	{
		mouseDown = false;
		scale.interpolateTo(Vector(0.9f, 0.9f), 0.1f);
	}
}

FoodHolder::FoodHolder(size_t slot, bool trash) : Quad(), trash(trash), slot(slot)
{
	foodHolderIngredient = 0;
	buttonDown = false;

	//setTexture("Gui/wok");
	renderQuad = false;

	wok = new Quad;
	if (trash)
		wok->setTexture("gui/wok-drop");
	else
		wok->setTexture("gui/wok");
	addChild(wok, PM_POINTER, RBP_ON);

	ing = new Quad;
	ing->renderQuad = false;
	addChild(ing, PM_POINTER);

	lid = new Quad("gui/wok-lid", Vector(0,0));
	lid->alpha = 0;
	lid->alphaMod = 0.5f;
	addChild(lid, PM_POINTER);
}

bool FoodHolder::isValid() const
{
	return alpha.x > 0 && alphaMod > 0;
}

void FoodHolder::toggleValid(bool v)
{
	alpha = alphaMod = (float) v;
}

void FoodHolder::animateLid(bool down, bool longAnim)
{
	float t = 0.2f;

	if (!longAnim)
	{
		t = 0.1f;
	}

	if (down)
	{
		dsq->sound->playSfx("bubble-lid");
		lid->alpha.interpolateTo(1, t);
		dsq->run(t);
	}
	else
	{
		lid->alpha.interpolateTo(0, t);
	}
}

bool FoodHolder::isTrash()
{
	return trash;
}

bool FoodHolder::isEmpty()
{
	return (foodHolderIngredient == 0);
}

IngredientData *FoodHolder::getIngredient()
{
	return foodHolderIngredient;
}

void FoodHolder::setIngredient(IngredientData *i, bool effects)
{
	IngredientData *oldi = foodHolderIngredient;
	foodHolderIngredient = i;

	if (oldi) {
		if (oldi->held > 0)
			oldi->held --;
		oldi->amount ++;
	}

	if (!i)
	{
		//ing->scale.interpolateTo(Vector(0,0), 0.1);
		ing->renderQuad = false;
		//setTexture("Gui/wok");
		if (oldi && effects)
		{
			core->sound->playSfx("Drop");
		}

		themenu->enqueuePreviewRecipe();
	}
	else
	{
		i->held ++;
		if (i->amount > 0)
			i->amount --;

		ing->setTexture("Ingredients/" + i->gfx);
		ing->renderQuad = true;
		if (effects)
		{
			core->sound->playSfx("Wok");

			ing->scale.ensureData();
			ing->scale.data->path.clear();
			ing->scale.data->path.addPathNode(Vector(1,1),0);
			ing->scale.data->path.addPathNode(Vector(1.25f,1.25f), 0.2f);
			ing->scale.data->path.addPathNode(Vector(1,1),1);
			ing->scale.startPath(0.5);
		}

		themenu->enqueuePreviewRecipe();
	}
}


void FoodHolder::dropFood()
{
	if (foodHolderIngredient)
	{
		setIngredient(0);
		themenu->refreshFoodSlots(true);
	}
}

void FoodHolder::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	if (!themenu->recipeMenu.on && foodHolderIngredient)
	{
		if ((core->mouse.position - getWorldPosition()).isLength2DIn(20))
		{
			if (!buttonDown && core->mouse.buttons.left)
			{
				dropFood();
				buttonDown = true;
			}
		}

		if (!buttonDown && core->mouse.buttons.left)
			buttonDown = true;
		if (buttonDown && !core->mouse.buttons.left)
			buttonDown = false;
	}
}

FoodSlot::FoodSlot(int slot) : AquariaGuiQuad(), slot(slot)
{
	doubleClickDelay = 0;

	right = false;

	renderQuad = false;

	label = new DebugFont(8, "");
	label->position = Vector(-2, 9);
	addChild(label, PM_POINTER);

	inCookSlot = false;

	ingredient = 0;

	lastIngredient = 0;
	lastAmount = 0;

	grabTime = 0;

	foodSlotIndex = -1;
	scaleFactor = 1;

	shareAlphaWithChildren = 1;

	rmb = 0;

}

void FoodSlot::setOriginalPosition(const Vector &op)
{
	originalPosition = op;
}

void FoodSlot::toggle(bool f)
{
	if (f)
	{
		alpha = 1;
		alphaMod = 1;
		label->alpha = 1;
	}
	else
	{
		alpha = 0;
		alphaMod = 0;
		label->alpha = 0;
	}
}

void FoodSlot::refresh(bool effects)
{
	int offset = themenu->currentFoodPage*foodPageSize;
	IngredientData *i = dsq->continuity.getIngredientHeldByIndex(offset+slot);
	if (i)
	{
		ingredient = i;

		if (i->amount > 0)
		{
			std::ostringstream os;
			if (i->amount > 1)
				os << i->amount << "/" << i->maxAmount;
			label->setText(os.str());
			setTexture("Ingredients/" + i->gfx);
			renderQuad = true;
		}
		else
		{
			label->setText("");
			renderQuad = true;

			setTexture("gui/wok");
			setWidthHeight(64);
		}
	}
	else
	{
		ingredient = 0;

		label->setText("");
		renderQuad = true;
		setTexture("gui/wok");
		setWidthHeight(64);
	}

	scale.interpolateTo(Vector(1,1)*scaleFactor,0.001f);

	if (ingredient != 0 && (i != lastIngredient || (i && i->amount != lastAmount)))
	{
		if (effects)
		{
			scale.ensureData();
			scale.data->path.clear();
			scale.data->path.addPathNode(Vector(1,1)*scaleFactor,0);
			scale.data->path.addPathNode(Vector(1.5f,1.5f)*scaleFactor, 0.2f);
			scale.data->path.addPathNode(Vector(1,1)*scaleFactor,1);
			scale.startPath(0.5);
		}
	}

	lastIngredient = i;
	if (i)
		lastAmount = i->amount;
	else
		lastAmount = 0;
}

void FoodSlot::eatMe()
{
	if (ingredient && !dsq->isNested())
	{
		for (size_t i = 0; i < foodHolders.size(); i++)
		{
			if (!foodHolders[i]->isTrash() && !foodHolders[i]->isEmpty())
			{
				dsq->sound->playSfx("denied");
				foodHolders[i]->dropFood();
				return;
			}
		}

		if (!ingredient->effects.empty())
		{
			bool eaten = dsq->continuity.applyIngredientEffects(ingredient);
			if(eaten)
			{
				ingredient->amount--;
				dsq->continuity.removeEmptyIngredients();
				themenu->refreshFoodSlots(true);
			}
		}
		else
		{
			dsq->sound->playSfx("denied");
			/// don't
		}
	}
}

void FoodSlot::moveRight()
{
	if (!ingredient) return;
	if (ingredient->amount <= 0) return;

	for (int i = foodHolders.size()-1; i >= 0; i--)
	{
		if (foodHolders[i]->isValid() && foodHolders[i]->isEmpty() && !foodHolders[i]->isTrash())
		{
			foodHolders[i]->setIngredient(ingredient);
			inCookSlot = true;
			refresh(true);
			break;
		}
	}
}

void FoodSlot::discard()
{
	if (!ingredient) return;
	if (ingredient->amount <= 0) return;

	ingredient->amount--;
	themenu->dropIngrNames.push_back(ingredient->name);
	dsq->continuity.removeEmptyIngredients();
	themenu->refreshFoodSlots(true);
}

bool FoodSlot::isCursorIn()
{
	return (core->mouse.position - getWorldPosition()).isLength2DIn(32);
}

void FoodSlot::onUpdate(float dt)
{
	AquariaGuiQuad::onUpdate(dt);

	if (doubleClickDelay > 0)
	{
		doubleClickDelay -= dt;
		if (doubleClickDelay < 0) doubleClickDelay = 0;
	}

	if (alphaMod==1 && ingredient && ingredient->amount > 0)
	{
		if (foodSlotIndex == slot)
		{
			if (!core->mouse.buttons.left)
			{
				foodSlotIndex = -1;
				if (!themenu->recipeMenu.on)
				{
					Vector wp = getWorldPosition();
					if ((themenu->lips->getWorldPosition() - wp).isLength2DIn(32))
					{
						themenu->menuSelectDelay = 0.5f;

						eatMe();
					}
					else if (wp.x < 40 || wp.y < 40 || wp.x > 760 || wp.y > 560)
					{
						discard();
					}
					else
					{
						bool droppedIn = false;
						for (size_t i = 0; i < foodHolders.size(); i++)
						{
							bool in = (foodHolders[i]->getWorldPosition() - wp).isLength2DIn(32);
							if (in)
							{
								droppedIn = true;

								if (foodHolders[i]->isTrash())
								{
									discard();

									themenu->foodLabel->alpha.interpolateTo(0, 2);
									themenu->foodDescription->alpha.interpolateTo(0, 2);

									break;
								}
								else if (foodHolders[i]->isEmpty() && foodHolders[i]->isValid())
								{
									foodHolders[i]->setIngredient(ingredient);
									inCookSlot = true;
									refresh(true);
									break;
								}
							}
						}

						if (!droppedIn)
						{
							if (doubleClickDelay > 0)
							{
								themenu->menuSelectDelay = 0.5f;
								doubleClickDelay = 0;
								eatMe();

								position = originalPosition;

								label->alpha = 1;
								grabTime = 0;

								return;
							}
							else
							{
								doubleClickDelay = DOUBLE_CLICK_DELAY;
							}
						}
					}
				}
				position = originalPosition;

				label->alpha = 1;

				grabTime = 0;
			}
			else
			{
				if (!themenu->recipeMenu.on)
				{
					if (dsq->getInputMode() == INPUT_MOUSE)
					{
						Vector diff = core->mouse.position - getWorldPosition();
						position += diff;
						themenu->moveFoodSlotToFront = this;
					}
				}
			}
		}

		if ((core->mouse.position - getWorldPosition()).isLength2DIn(16))
		{
			themenu->foodLabel->setText(ingredient->displayName);
			themenu->foodLabel->alpha.interpolateTo(1, 0.2f);

			themenu->foodDescription->setText(dsq->continuity.getIngredientAffectsString(ingredient));
			themenu->foodDescription->alpha.interpolateTo(1, 0.2f);

			if (core->mouse.buttons.left && foodSlotIndex == -1)
			{
				grabTime = 0;
				foodSlotIndex = slot;
				label->alpha = 0;

				if (!inCookSlot)
				{
					originalPosition = position;
				}
			}

			if (core->mouse.buttons.right && !rmb)
			{
				rmb = 1;
			}
			else if (!core->mouse.buttons.right && rmb)
			{
				rmb = 0;
				if (!themenu->recipeMenu.on)
					moveRight();
				return;
			}
		}
		else
		{
			if (!themenu->foodLabel->alpha.isInterpolating())
				themenu->foodLabel->alpha.interpolateTo(0, 2);
			if (!themenu->foodDescription->alpha.isInterpolating())
				themenu->foodDescription->alpha.interpolateTo(0, 2);
			rmb = 0;
		}
	}
	else
	{
		rmb = 0;
	}
}

SongSlot::SongSlot(int songSlot) : AquariaGuiQuad(), songSlot(songSlot)
{
	songType = dsq->continuity.getSongTypeBySlot(songSlot);
	std::ostringstream os;
	os << "Song/SongSlot-" << songSlot;
	setTexture(os.str());

	glow = new Quad("particles/glow", Vector(0,0));
	glow->setWidthHeight(128, 128);
	glow->setBlendType(BLEND_ADD);
	glow->alpha = 0;
	addChild(glow, PM_POINTER);

	mbDown = false;

	if (dsq->continuity.isSongTypeForm((SongType)dsq->continuity.getSongTypeBySlot(songSlot)))
		scale = Vector(0.9f, 0.9f);
	else
		scale = Vector(0.6f, 0.6f);
}

void SongSlot::onUpdate(float dt)
{
	AquariaGuiQuad::onUpdate(dt);

	if (alpha.x == 1 && alphaMod == 1 && (!parent || parent->alpha.x == 1))
	{
		if ((core->mouse.position - getWorldPosition()).isLength2DIn(24))
		{
			themenu->playSongInMenu(songType);
			themenu->songLabel->setText(dsq->continuity.getSongNameBySlot(songSlot));
			themenu->songLabel->alpha.interpolateTo(1, 0.2f);
			const bool anyButton = core->mouse.buttons.left || core->mouse.buttons.right;
			if (!mbDown && anyButton)
			{
				mbDown = true;
			}
			else if (mbDown && !anyButton)
			{
				mbDown = false;

				themenu->playSongInMenu(songType, 1);
				if (!dsq->sound->isPlayingVoice())
					dsq->voice(dsq->continuity.getVoxForSongSlot(songSlot));
			}
			glow->alpha.interpolateTo(0.2f, 0.15f);
		}
		else
		{
			mbDown = false;
			glow->alpha.interpolateTo(0, 0.2f);
			if (!themenu->songLabel->alpha.isInterpolating())
			{
				themenu->songLabel->alpha.interpolateTo(0, 2);
				}
		}
	}
}


const int treasureFlagStart			= 500;

TreasureSlot::TreasureSlot(int index) : AquariaGuiQuad()
{
	this->index = index;
	mbd = false;
	flag = 0;
	doubleClickTimer = 0;
}

void TreasureSlot::onUpdate(float dt)
{
	AquariaGuiQuad::onUpdate(dt);

	doubleClickTimer -= dt;
	if (doubleClickTimer < 0)
		doubleClickTimer = 0;

	if (alphaMod == 1 && alpha.x == 1 && flag != 0)
	{
		if ((core->mouse.position - getWorldPosition()).isLength2DIn(18))
		{
			scale.interpolateTo(Vector(1.2f, 1.2f), 0.1f);
			if (core->mouse.buttons.left && !mbd)
			{
				mbd = true;
			}
			else if (!core->mouse.buttons.left && mbd)
			{
				mbd = false;

				if (doubleClickTimer > 0)
				{
					doubleClickTimer = 0;

					themenu->onUseTreasure(flag);
				}
				else
				{
					dsq->sound->playSfx("treasure-select", 0.5);
					dsq->spawnParticleEffect("menu-switch", worldRightCenter, 0, 0, LR_HUD3, 1);



					themenu->treasureLabel->setText(treasureName);
					themenu->treasureLabel->alpha = 1;
					themenu->treasureCloseUp->setTexture(dsq->continuity.treasureData[flag].gfx);

					themenu->treasureCloseUp->alpha = 1;

					themenu->treasureDescription->setText(treasureDesc, Vector(400,450), 400);
					themenu->treasureDescription->alpha = 1;

					themenu->use->alpha = dsq->continuity.treasureData[flag].use;

					themenu->selectedTreasureFlag = flag;

					doubleClickTimer = 0.2f;


					std::ostringstream os;
					os << "treasure flag: " << flag << " desc: " << treasureDesc;
					debugLog(os.str());
				}
			}
		}
		else
		{
			mbd = false;
			scale.interpolateTo(Vector(1, 1), 0.1f);
		}
	}
	else
	{
		mbd = false;
		scale.interpolateTo(Vector(1, 1), 0.001f);
	}
}

void TreasureSlot::refresh()
{
	flag = (themenu->currentTreasurePage*treasurePageSize) + index + treasureFlagStart;
	if (flag >= FLAG_COLLECTIBLE_START && flag < FLAG_COLLECTIBLE_END && dsq->continuity.getFlag(flag)>0)
	{
		// get treasure image somehow
		setTexture(dsq->continuity.treasureData[flag].gfx);
		float scl = dsq->continuity.treasureData[flag].sz;

		float w = width;
		float h = height;
		float sz = 50;
		if (w > h)
		{
			w = sz;
			h = (height*sz)/width;
		}
		else
		{
			h = sz;
			w = (width*sz)/height;
		}

		setWidthHeight(w*scl, h*scl);

		std::string parse = stringbank.get(flag);

		int p1 = parse.find_first_of('[');
		if (p1 != std::string::npos)
		{
			p1++;
			int p2 = parse.find_first_of(']');
			treasureName = parse.substr(p1,p2-p1);

			p1 = parse.find_last_of('[');
			if (p1 != std::string::npos)
			{
				p1++;
				p2 = parse.find_last_of(']');
				treasureDesc = parse.substr(p1,p2-p1);
			}
		}

		alphaMod = 1;
	}
	else
	{
		flag = 0;
		alphaMod = 1;

		setTexture("gui/wok");
		setWidthHeight(48);
	}
}




// ------------------ Menu code -------------------------

InGameMenu::InGameMenu()
{
	themenu = this;
	inGameMenu = false;
	optionsOnly = false;
	cookDelay = 0;
	lastMenuPage = MENUPAGE_NONE;
	lastOptionsMenuPage = MENUPAGE_NONE;
	optsfxdly = 0;
	blurEffectsCheck = 0;
	ripplesCheck = 0;
	menu_blackout = 0;
	menuSelectDelay = 0;
	selectedActionSetIdx = 0;
}

InGameMenu::~InGameMenu()
{
	themenu = 0;
}

void InGameMenu::bindInput()
{
	addAction(ACTION_ESC, KEY_ESCAPE, -1);

	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
		const ActionSet& as = dsq->user.control.actionSets[i];
		int sourceID = (int)i;

		as.importAction(this, "PrimaryAction", ACTION_PRIMARY, sourceID);
		as.importAction(this, "SecondaryAction", ACTION_SECONDARY, sourceID);

		as.importAction(this, "Escape",		ACTION_ESC, sourceID);
		as.importAction(this, "WorldMap",		ACTION_TOGGLEWORLDMAP, sourceID);
		as.importAction(this, "ToggleHelp",	ACTION_TOGGLEHELPSCREEN, sourceID);

		as.importAction(this, "PrevPage",		ACTION_PREVPAGE, sourceID);
		as.importAction(this, "NextPage",		ACTION_NEXTPAGE, sourceID);
		as.importAction(this, "CookFood",		ACTION_COOKFOOD, sourceID);
		as.importAction(this, "FoodLeft",		ACTION_FOODLEFT, sourceID);
		as.importAction(this, "FoodRight",		ACTION_FOODRIGHT, sourceID);
		as.importAction(this, "FoodDrop",		ACTION_FOODDROP, sourceID);
	}
}

void InGameMenu::reset()
{
	isCooking = false;
	enqueuedPreviewRecipe = 0;
	moveFoodSlotToFront = 0;
	cookDelay = 0;
	foodMenu = optionsMenu = petMenu = treasureMenu = false;
	inGameMenuExitState = 0;
	optsfxdly = 0;
	playingSongInMenu = -1;
	menuSelectDelay = 0;

	dropIngrNames.clear();

	create();
	hide(false);
}

void InGameMenu::onContinuityReset()
{
	currentMenuPage = MENUPAGE_NONE;
	currentFoodPage = 0;
	currentTreasurePage = 0;
	recipeMenu.currentPage = 0;
	lastMenuPage = MENUPAGE_NONE;
	lastOptionsMenuPage = MENUPAGE_NONE;
}

void InGameMenu::action(int id, int state, int source, InputDevice device)
{
	if(game->isIgnoreAction((AquariaActions)id))
		return;

	if (id == ACTION_TOGGLEMENU)
	{
		if(state)
			show();
		else
			hide();
	}

	if (id == ACTION_TOGGLEWORLDMAP && !state)
	{
		if (foodMenu)
		{
			recipes->setFocus(true);
			recipeMenu.toggle(!recipeMenu.on, true);
		}
	}

	if(id == ACTION_ESC && !state)
	{
		if (isInGameMenu())
		{
			if (!AquariaKeyConfig::waitingForInput)
			{
				if (menuOpenTimer > 0.5f)
				{
					if (optionsMenu || keyConfigMenu)
						onOptionsCancel();
					else
						action(ACTION_TOGGLEMENU, 0, source, device); // hide menu
				}
			}
		}
	}

	if (isInGameMenu())
	{
		if (treasureMenu)
		{
			if (!state && !dsq->isNested())
			{
				if (themenu->menuSelectDelay == 0)
				{
					if (id == ACTION_PREVPAGE)
					{
						themenu->menuSelectDelay = MENUSELECTDELAY;
						onPrevTreasurePage();
						//menu[5]->setFocus(true);
					}
					if (id == ACTION_NEXTPAGE)
					{
						themenu->menuSelectDelay = MENUSELECTDELAY;
						onNextTreasurePage();
						//menu[5]->setFocus(true);
					}
				}
			}
		}
		else if (foodMenu)
		{
			if (!state && !dsq->isNested())
			{
				if (themenu->menuSelectDelay == 0)
				{
					if (id == ACTION_PREVPAGE)
					{
						themenu->menuSelectDelay = MENUSELECTDELAY;
						if (recipeMenu.on)
							recipeMenu.goPrevPage();
						else
							onPrevFoodPage();
					}
					if (id == ACTION_NEXTPAGE)
					{
						themenu->menuSelectDelay = MENUSELECTDELAY;
						if (recipeMenu.on)
							recipeMenu.goNextPage();
						else
							onNextFoodPage();
					}
				}

				if (id == ACTION_COOKFOOD)
				{
					if (!recipeMenu.on)
						onCook();
				}

				if (id == ACTION_FOODLEFT)
				{
					if (recipeMenu.on)
					{
					}
					else
					{
						for (size_t i = 0; i < foodHolders.size(); i++)
						{
							if (!foodHolders[i]->isTrash() && !foodHolders[i]->isEmpty())
							{
								foodHolders[i]->dropFood();
								break;
							}
						}
					}
				}

				if (id == ACTION_FOODRIGHT)
				{
					if (recipeMenu.on)
					{
					}
					else
					{
						for (size_t i = 0; i < foodSlots.size(); i++)
						{
							if (foodSlots[i]->isCursorIn() && foodSlots[i]->getIngredient())
							{
								foodSlots[i]->moveRight();
								break;
							}
						}
					}
				}

				if (id == ACTION_FOODDROP)
				{
					if (recipeMenu.on)
					{
					}
					else
					{
						int trashIndex = -1;
						for (size_t i = 0; i < foodHolders.size(); i++)
						{
							if (foodHolders[i]->isValid() && foodHolders[i]->isTrash())
							{
								trashIndex = i;
								break;
							}
						}
						if (trashIndex >= 0)
						{
							for (size_t i = 0; i < foodSlots.size(); i++)
							{
								if (foodSlots[i]->isCursorIn() && foodSlots[i]->getIngredient())
								{
									foodSlots[i]->discard();
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

void InGameMenu::enqueuePreviewRecipe()
{
	enqueuedPreviewRecipe = 1;
}

void InGameMenu::setMenuDescriptionText(const std::string &text)
{
	menuDescription->setText(text);
}

void InGameMenu::playSongInMenu(int songType, bool override)
{
	if (playingSongInMenu == -1 || override)
	{
		playingSongInMenu = songType;
		currentSongMenuNote = 0;
		songMenuPlayDelay = 0.5;
	}
}

void InGameMenu::updatePreviewRecipe()
{
	const float t = 0.2f;

	updateCookList();

	if (cookList.size() < 2 || recipeMenu.on){
		previewRecipe->alpha.interpolateTo(0, t);
	}
	else{
		Recipe *r = findRecipe(cookList);

		IngredientData *data=0;

		if (r && r->isKnown())
		{
			data = dsq->continuity.getIngredientDataByName(r->result);
			previewRecipe->setTexture("ingredients/"+data->gfx);
		}
		else
		{
			previewRecipe->setTexture("gui/question-mark");
		}

		previewRecipe->alpha.interpolateTo(1, t);

	}
}


void InGameMenu::show(bool ignoreInput, bool optionsOnly, MenuPage menuPage)
{
	if (game->avatar && core->getNestedMains()==1 && !game->avatar->isSinging() && (ignoreInput || game->avatar->isInputEnabled()))
	{
		game->clearControlHint();

		selectedTreasureFlag = -1;
		this->optionsOnly = optionsOnly;

		core->sound->playSfx("Menu-Open");
		dropIngrNames.clear();

		if (game->avatar->isEntityDead()) return;

		toggleOptionsMenu(false);
		dsq->overlay->alpha.interpolateTo(0, 0.1f);
		float t = 0.3f;


		if (!optionsOnly)
		{
			game->togglePause(true);
		}

		if (optionsOnly)
		{
			menu_blackout = new Quad;
			menu_blackout->color = 0;
			menu_blackout->autoWidth = AUTO_VIRTUALWIDTH;
			menu_blackout->autoHeight = AUTO_VIRTUALHEIGHT;
			menu_blackout->followCamera = 1;
			menu_blackout->position = Vector(400,300);
			menu_blackout->alphaMod = 0.75;
			menu_blackout->alpha = 0;
			menu_blackout->alpha.interpolateTo(1, 0.5);
			game->addRenderObject(menu_blackout, LR_AFTER_EFFECTS);

			menuBg2->alpha = 0;
		}
		else
		{
			menuBg2->alpha = 0;
			menuBg2->alpha.interpolateTo(1, t*0.5f);
		}

		if (dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY2))
			energyIdol->alphaMod = 1;
		else
			energyIdol->alphaMod = 0;

		if (dsq->continuity.getFlag(FLAG_LI) >= 100)
			liCrystal->alphaMod = 1;
		else
			liCrystal->alphaMod = 0;

		for (size_t i = 0; i < songSlots.size(); i++)
		{
			if (dsq->continuity.hasSong(dsq->continuity.getSongTypeBySlot(i)))
				songSlots[i]->alpha.interpolateTo(1, t);
			else
				songSlots[i]->alpha = 0;
		}

		menuDescription->setText("");

		menuDescription->alpha.interpolateTo(1, t);

		menuBg->scale = menuBgScale*0.5f;
		menuBg->scale.interpolateTo(menuBgScale, t);
		menuBg->alpha.interpolateTo(1, t*0.5f);
		menuBg->setHidden(false);

		// FIXME: This gets a little verbose because of all the
		// individual non-child objects.  Is there a reason they
		// can't all be children of menuBg?  --achurch
		opt_save->setHidden(false);
		opt_cancel->setHidden(false);
		options->setHidden(false);
		keyConfigButton->setHidden(false);
		keyConfigBg->setHidden(false);
		cook->setHidden(false);
		foodSort->setHidden(false);
		recipes->setHidden(false);
		use->setHidden(false);
		prevFood->setHidden(false);
		nextFood->setHidden(false);
		prevTreasure->setHidden(false);
		nextTreasure->setHidden(false);
		circlePageNum->setHidden(false);
		previewRecipe->setHidden(false);
		showRecipe->setHidden(false);
		recipeMenu.scroll->setHidden(false);
		recipeMenu.scrollEnd->setHidden(false);
		recipeMenu.header->setHidden(false);
		recipeMenu.page->setHidden(false);
		recipeMenu.prevPage->setHidden(false);
		recipeMenu.nextPage->setHidden(false);
		menuDescription->setHidden(false);
		eAre->setHidden(false);
		eYes->setHidden(false);
		eNo->setHidden(false);
		menuIconGlow->setHidden(false);
		for (size_t i = 0; i < menu.size(); i++)
			menu[i]->setHidden(false);
		for (size_t i = 0; i < treasureSlots.size(); i++)
			treasureSlots[i]->setHidden(false);
		treasureDescription->setHidden(false);
		for (size_t i = 0; i < foodSlots.size(); i++)
			foodSlots[i]->setHidden(false);


		if (game->miniMapRender)
		{
			game->miniMapRender->slide(1);
		}

		toggleMainMenu(false);

		dsq->run(t);

		dsq->screenTransition->capture();

		MenuPage useMenuPage = MENUPAGE_NONE;

		if (!optionsOnly)
		{
			if (menuPage != MENUPAGE_NONE)
			{
				useMenuPage = menuPage;
			}
			else if (lastMenuPage != MENUPAGE_NONE)
			{
				useMenuPage = lastMenuPage;
			}
		}

		switch(useMenuPage)
		{
		case MENUPAGE_FOOD:
			toggleFoodMenu(true);
			((AquariaMenuItem*)menu[6])->setFocus(true);
			break;
		case MENUPAGE_TREASURES:
			toggleTreasureMenu(true);
			break;
		case MENUPAGE_PETS:
			togglePetMenu(true);
			break;
		case MENUPAGE_SONGS:
		default:
			{
				if (optionsOnly)
				{
					toggleOptionsMenu(true);
				}
				else
				{
					float t = 0.1f;

					toggleMainMenu(true);

					songBubbles->alpha.interpolateTo(1, t);
					for (size_t i = 0; i < menu.size(); i++)
					{
						menu[i]->scale = Vector(0,0);
						menu[i]->alpha = 0;
					}
					((AquariaMenuItem*)menu[5])->setFocus(true);
				}
			}
		}


		if (!optionsOnly)
		{
			for (size_t i = 0; i < menu.size(); i++)
			{
				menu[i]->scale.interpolateTo(Vector(1, 1), 0.15f);

				menu[i]->alpha.interpolateTo(1, 0.15f);
			}

			menuIconGlow->alpha.interpolateTo(1, 0.5f);
		}

		menuOpenTimer = 0;

		inGameMenu = true;
		dsq->screenTransition->transition(MENUPAGETRANSTIME);

		if (optionsOnly)
			dsq->run(-1);
	}
}


void InGameMenu::hide(bool effects, bool cancel)
{
	if (isCooking) return;
	if (FoodSlot::foodSlotIndex != -1) return;
	if (effects && !isInGameMenu()) return;

	if (game->avatar)
	{
		if (resBox)
			resBox->close();

		if (effects)
			core->sound->playSfx("Menu-Close");

		hideInGameMenuExitCheck(false);
		playingSongInMenu = -1;


		float t = 0.3f;

		if (!effects)
			t = 0;

		for (size_t i = 0; i < foodHolders.size(); i++)
		{
			foodHolders[i]->dropFood();
		}

		lastMenuPage = currentMenuPage;
		if(cancel && (optionsMenu || keyConfigMenu))
			onOptionsCancel();
		else
			toggleOptionsMenu(false);

		if (!optionsOnly)
		{
			toggleFoodMenu(false);
			toggleTreasureMenu(false);
			togglePetMenu(false);
			toggleMainMenu(false);
			toggleKeyConfigMenu(false);
		}

		menuIconGlow->alpha = 0;

		for (size_t i = 0; i < menu.size(); i++)
		{
			menu[i]->alpha = 0;
		}
		for (size_t i = 0; i < songSlots.size(); i++)
			songSlots[i]->alpha.interpolateTo(0, t);
		songBubbles->alpha.interpolateTo(0, t);

		if (game->miniMapRender)
			game->miniMapRender->slide(0);

		menuDescription->alpha.interpolateTo(0, t);
		menuBg->alpha.interpolateTo(0, t);
		menuBg->scale.interpolateTo(menuBg->scale*0.5f, t);
		menuBg2->alpha.interpolateTo(0, t);

		if (menu_blackout)
		{
			menu_blackout->alpha.interpolateTo(0, t);
		}

		if (showRecipe)
		{
			showRecipe->alpha.interpolateTo(0, t);
		}

		if (effects)
			core->run(t);

		if (menu_blackout)
		{
			menu_blackout->safeKill();
			menu_blackout = 0;
		}
		if (effects)
			game->togglePause(false);

		inGameMenu = false;

		for (size_t i = 0; i < songTips.size(); i++)
			songTips[i]->alpha = 0;

		for (size_t i = 0; i < dropIngrNames.size(); i++)
		{
			game->spawnIngredient(dropIngrNames[i], game->avatar->position + Vector(0,-96), 1, 1);
		}
		dropIngrNames.clear();

		if (effects)
			dsq->quitNestedMain();
	}

	menuBg->setHidden(true);
	opt_save->setHidden(true);
	opt_cancel->setHidden(true);
	options->setHidden(true);
	keyConfigButton->setHidden(true);
	keyConfigBg->setHidden(true);
	cook->setHidden(true);
	foodSort->setHidden(true);
	recipes->setHidden(true);
	use->setHidden(true);
	prevFood->setHidden(true);
	nextFood->setHidden(true);
	prevTreasure->setHidden(true);
	nextTreasure->setHidden(true);
	circlePageNum->setHidden(true);
	previewRecipe->setHidden(true);
	showRecipe->setHidden(true);
	recipeMenu.scroll->setHidden(true);
	recipeMenu.scrollEnd->setHidden(true);
	recipeMenu.header->setHidden(true);
	recipeMenu.page->setHidden(true);
	recipeMenu.prevPage->setHidden(true);
	recipeMenu.nextPage->setHidden(true);
	menuDescription->setHidden(true);
	eAre->setHidden(true);
	eYes->setHidden(true);
	eNo->setHidden(true);
	menuIconGlow->setHidden(true);
	for (size_t i = 0; i < menu.size(); i++)
		menu[i]->setHidden(true);
	for (size_t i = 0; i < treasureSlots.size(); i++)
		treasureSlots[i]->setHidden(true);
	treasureDescription->setHidden(true);
	for (size_t i = 0; i < foodSlots.size(); i++)
		foodSlots[i]->setHidden(true);
}


void InGameMenu::addKeyConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int x, int y, bool rejectJoyAxis)
{
	TTFText *lb = new TTFText(&dsq->fontArialSmallest);
	lb->setText(label);
	lb->position = Vector(x,y);
	group->addChild(lb, PM_POINTER);
	x += KEYCONFIG_FIRST_COL_DISTANCE;

	AquariaKeyConfig *m = new AquariaKeyConfig(actionInputName, INPUTSET_MOUSE, 0);
	m->position = Vector(x,y);
	group->addChild(m, PM_POINTER);
	keyConfigs.push_back(m);
	x += KEYCONFIG_COL_DISTANCE;

	AquariaKeyConfig *k1 = new AquariaKeyConfig(actionInputName, INPUTSET_KEY, 0);
	k1->position = Vector(x,y);
	group->addChild(k1, PM_POINTER);
	keyConfigs.push_back(k1);
	x += KEYCONFIG_COL_DISTANCE;

	AquariaKeyConfig *k2 = new AquariaKeyConfig(actionInputName, INPUTSET_KEY, 1);
	k2->position = Vector(x,y);
	group->addChild(k2, PM_POINTER);
	keyConfigs.push_back(k2);
	x += KEYCONFIG_COL_DISTANCE;

	AquariaKeyConfig *j1 = new AquariaKeyConfig(actionInputName, INPUTSET_JOY, 0);
	j1->position = Vector(x,y);
	group->addChild(j1, PM_POINTER);
	keyConfigs.push_back(j1);
	j1->setRejectJoyAxis(rejectJoyAxis);
	x += KEYCONFIG_COL_DISTANCE;

	m->setDirMove(DIR_RIGHT, k1);
	k1->setDirMove(DIR_LEFT, m);

	k1->setDirMove(DIR_RIGHT, k2);
	k2->setDirMove(DIR_RIGHT, j1);

	j1->setDirMove(DIR_LEFT, k2);
	k2->setDirMove(DIR_LEFT, k1);
}

AquariaKeyConfig *InGameMenu::addAxesConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int offx, int y)
{
	TTFText *lb = new TTFText(&dsq->fontArialSmallest);
	lb->setText(label);
	lb->position = Vector(offx, y);
	group->addChild(lb, PM_POINTER);

	AquariaKeyConfig *i1 = new AquariaKeyConfig(actionInputName, INPUTSET_OTHER, 0);
	i1->position = Vector(80+offx,y);
	keyConfigs.push_back(i1);
	group->addChild(i1, PM_POINTER);

	i1->setDirMove(DIR_RIGHT, 0);
	i1->setDirMove(DIR_LEFT, 0);

	return i1;
}


void InGameMenu::switchToSongMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	toggleFoodMenu(false);
	togglePetMenu(false);
	toggleTreasureMenu(false);

	toggleMainMenu(true);

	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void InGameMenu::switchToFoodMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	togglePetMenu(false);
	toggleMainMenu(false);
	toggleTreasureMenu(false);

	toggleFoodMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void InGameMenu::switchToPetMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	toggleFoodMenu(false);
	toggleMainMenu(false);
	toggleTreasureMenu(false);

	togglePetMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void InGameMenu::switchToTreasureMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	toggleFoodMenu(false);
	toggleMainMenu(false);
	togglePetMenu(false);

	toggleTreasureMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}


void InGameMenu::sortFood()
{
	std::vector<std::string> foodHolderNames;
	foodHolderNames.resize(foodHolders.size());

	for (size_t i = 0; i < foodHolders.size(); i++) {
		IngredientData *ing = foodHolders[i]->getIngredient();
		if (ing)
			foodHolderNames[i] = ing->name;
	}

	dsq->continuity.foodSortType++;
	if (dsq->continuity.foodSortType >= MAX_FOODSORT)
		dsq->continuity.foodSortType = 0;

	dsq->continuity.sortFood();

	// rebuild the page

	refreshFoodSlots(false);

	dsq->sound->playSfx("shuffle");
	dsq->sound->playSfx("menu-switch", 0.5);
	dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);

	for (size_t i = 0; i < foodHolders.size(); i++) {
		if (!foodHolderNames[i].empty()) {
			IngredientData *ing = dsq->continuity.getIngredientHeldByName(foodHolderNames[i]);
			foodHolders[i]->setIngredient(ing, false);
		}
	}
}

RenderObject *InGameMenu::createBasicKeyConfig()
{
	RenderObject *keyConfig = new RenderObject;

	keyConfigBg->addChild(keyConfig, PM_POINTER);

	keyConfig->shareAlphaWithChildren = 1;
	keyConfig->followCamera = 1;
	keyConfig->position = Vector(0, -40);

	return keyConfig;
}

static std::string screenModeStr(const ScreenMode& m)
{
	std::ostringstream os;
	if(!m.x && !m.y)
	{
		os << stringbank.get(2142);
	}
	else
	{
		os << m.x << "x" << m.y;
		if(m.hz)
			os << " (" << m.hz << "hz)";
	}
	return os.str();
}

static bool isCurrentScreenMode(const ScreenMode& m)
{
	if(!m.x && !m.y && core->isDesktopResolution())
			return true;
	return m.x == dsq->user.video.resx && m.y == dsq->user.video.resy && (!m.hz || dsq->user.video.hz == m.hz);
}


void InGameMenu::create()
{
	float menuz = 4;

	menuBg = new Quad;
	menuBg->setTexture("menu");
	//menuBg->setWidthHeight(800);
	//menuBg->scale = Vector(800.0f/1024.0f, 800.0f/1024.0f);
	menuBg->position = Vector(400,300,menuz);
	menuBg->followCamera = 1;
	//menuBg->shareAlphaWithChildren=true;
	game->addRenderObject(menuBg, LR_MENU);


	menuBg2 = new Quad;
	menuBg2->setTexture("menu2");
	menuBg2->position = Vector(0, 240);
	menuBg->addChild(menuBg2, PM_POINTER);

	float scale = menuBg->scale.x;
	/*
	songDescription = new BitmapText(dsq->font);
	songDescription->position = Vector(0,100);
	songDescription->parentManagedPointer = 1;
	menuBg->addChild(songDescription);
	*/

	options = new Quad;

	options->renderQuad = false;

	int sliderx = 250, slidery = 160, sliderd = 26;
	int checkx=660, checky = 160, checkd = 26;

	Quad *audio = new Quad("gui/audiovisual", Vector(200, 125));
	options->addChild(audio, PM_POINTER);

	Quad *controls = new Quad("gui/controls", Vector(600, 125));
	options->addChild(controls, PM_POINTER);

	/*
	Quad *visual = new Quad("gui/visual", Vector(170, 300));
	visual->parentManagedPointer = 1;
	options->addChild(visual);
	*/

	/*
	Quad *blurEffectsLabel = new Quad("gui/blurEffectsLabel.png", visual->position + Vector(-20,40));
	blurEffectsLabel->parentManagedPointer = 1;
	options->addChild(blurEffectsLabel);
	*/

	/*
	blurEffectsCheck = new AquariaCheckBox();
	blurEffectsCheck->position = visual->position + Vector(60, 40);
	blurEffectsCheck->parentManagedPointer = 1;
	options->addChild(blurEffectsCheck);
	*/

	Quad *controllabels = new Quad("gui/controllabels", Vector(0,0,0));
	int w = controllabels->getWidth();
	int h = controllabels->getHeight();
	controllabels->position = Vector(checkx-16-w/2.0f, checky + h/2.0f - 14);
	options->addChild(controllabels, PM_POINTER);



	int scheckx=270;
	int schecky=315;
	int sw,sh;
	int voptoffy = 26;

	Quad *subtitleslabel = new Quad("gui/subtitles", Vector(0,0,0));
	sw = subtitleslabel->getWidth();
	sh = subtitleslabel->getHeight();
	subtitleslabel->position = Vector(scheckx-16-sw*0.5f, schecky + sh/2.0f - 14);
	options->addChild(subtitleslabel, PM_POINTER);

	subtitlesCheck = new AquariaCheckBox();
	subtitlesCheck->setValue(dsq->user.audio.subtitles);
	subtitlesCheck->position = Vector(scheckx,schecky);
	options->addChild(subtitlesCheck, PM_POINTER);

	Quad *fullscreenLabel = new Quad("gui/fullscreen", Vector(0,0,0));
	fullscreenLabel->position = Vector(scheckx-16-sw*0.5f, schecky + voptoffy + sh/2.0f - 14);
	options->addChild(fullscreenLabel, PM_POINTER);

	fullscreenCheck = new AquariaCheckBox();
	fullscreenCheck->setValue(dsq->isFullscreen());
	fullscreenCheck->position = Vector(scheckx,schecky + voptoffy);
	options->addChild(fullscreenCheck, PM_POINTER);

	Quad *resolutionLabel = new Quad("gui/resolution", Vector(0,0,0));
	resolutionLabel->position = Vector(160, 260);
	options->addChild(resolutionLabel, PM_POINTER);

	resBox = new AquariaComboBox(Vector(0.7f, 1.0f));
	resBox->position = Vector(196, 285);
	core->enumerateScreenModesIfNecessary();
	for (size_t i = 0; i < core->screenModes.size(); i++)
	{
		const ScreenMode& m = core->screenModes[i];
		resBox->addItem(screenModeStr(m));
		if (isCurrentScreenMode(m))
		{
			resBox->enqueueSelectItem(i);
		}
	}
	options->addChild(resBox, PM_POINTER);

	Quad *audiolabels = new Quad("gui/audiolabels", Vector(0,0,0));
	w = audiolabels->getWidth();
	h = audiolabels->getHeight();
	audiolabels->position = Vector(sliderx-64-w/2.0f, slidery + h/2.0f - 14);
	options->addChild(audiolabels, PM_POINTER);

	musslider = new AquariaSlider();
	musslider->setValue(dsq->user.audio.musvol);
	musslider->position = Vector(sliderx,slidery+1*sliderd);
	options->addChild(musslider, PM_POINTER);

	sfxslider = new AquariaSlider();
	sfxslider->setValue(dsq->user.audio.sfxvol);
	sfxslider->position = Vector(sliderx,slidery);
	options->addChild(sfxslider, PM_POINTER);

	voxslider = new AquariaSlider();
	voxslider->setValue(dsq->user.audio.voxvol);
	voxslider->position = Vector(sliderx,slidery+2*sliderd);
	options->addChild(voxslider, PM_POINTER);


	flipInputButtonsCheck = new AquariaCheckBox();
	flipInputButtonsCheck->setValue(dsq->user.control.flipInputButtons);
	flipInputButtonsCheck->position = Vector(checkx,checky);
	options->addChild(flipInputButtonsCheck, PM_POINTER);

	toolTipsCheck = new AquariaCheckBox();
	toolTipsCheck->setValue(dsq->user.control.toolTipsOn);
	toolTipsCheck->position = Vector(checkx,checky+1*checkd);
	options->addChild(toolTipsCheck, PM_POINTER);

	autoAimCheck = new AquariaCheckBox();
	autoAimCheck->setValue(dsq->user.control.autoAim);
	autoAimCheck->position = Vector(checkx,checky+2*checkd);
	options->addChild(autoAimCheck, PM_POINTER);

	targetingCheck = new AquariaCheckBox();
	targetingCheck->setValue(dsq->user.control.targeting);
	targetingCheck->position = Vector(checkx,checky+3*checkd);
	options->addChild(targetingCheck, PM_POINTER);



	opt_save = new AquariaMenuItem;
	opt_save->useQuad("gui/Apply");
	opt_save->useGlow("particles/glow", 100, 50);
	opt_save->event.set(MakeFunctionEvent(InGameMenu, onOptionsSave));
	opt_save->position = opt_save_original;
	opt_save->alpha = 0;
	game->addRenderObject(opt_save, LR_MENU);

	opt_cancel = new AquariaMenuItem;
	opt_cancel->useQuad("gui/Cancel");
	opt_cancel->useGlow("particles/glow", 100, 50);
	opt_cancel->event.set(MakeFunctionEvent(InGameMenu, onOptionsCancel));
	opt_cancel->position = opt_cancel_original;
	opt_cancel->alpha = 0;
	game->addRenderObject(opt_cancel, LR_MENU);

	options->shareAlphaWithChildren = 1;
	options->alpha = 0;
	options->followCamera = 1;
	game->addRenderObject(options, LR_MENU);

	scale = 1;
	songSlots.clear();
	//songSlots.resize(3);
	songSlots.resize(10);
	// rewrite this: so you can hide / ignore certain songs, etc
	//songSlots.resize(dsq->continuity.getSongBankSize());
	//Vector center(-235, -50);
	Vector center(-230, -50), rightCenter(230, -50);

	energyIdol = new Quad("formupgrades/energyidol-charged", Vector(40,0));
	menuBg->addChild(energyIdol, PM_POINTER);

	liCrystal = new Quad("gui/li-crystal", Vector(0,0));
	menuBg->addChild(liCrystal, PM_POINTER);

	songBubbles = new Quad("gui/SongBubbles", Vector(-center.x, center.y));
	menuBg->addChild(songBubbles, PM_POINTER);


	// Vector(575,250);


	songLabel = new BitmapText(dsq->smallFont);
	{
		songLabel->alpha = 0;
		songLabel->setAlign(ALIGN_CENTER);
		songLabel->followCamera = 1;
		songLabel->setFontSize(20);
		songLabel->position = Vector(-center.x, center.y) + Vector(0, -15); //+ Vector(10, -10);
		songLabel->scale = Vector(1.2f, 1.2f);
	}
	menuBg->addChild(songLabel, PM_POINTER);



	ToolTip *tip = 0;

	foodTips.clear();
	songTips.clear();
	petTips.clear();
	treasureTips.clear();

	tip = new ToolTip;
	tip->alpha = 0;
	tip->setCircularAreaFromCenter(worldLeftCenter, 240);
	tip->setText(stringbank.get(0), Vector(200,450), 350);
	game->addRenderObject(tip, LR_HUD);
	foodTips.push_back(tip);


	tip = new ToolTip;
	tip->alpha = 0;
	tip->setCircularAreaFromCenter(worldRightCenter, 240);
	tip->setText(stringbank.get(1), Vector(600,450), 350);
	game->addRenderObject(tip, LR_HUD);
	foodTips.push_back(tip);



	tip = new ToolTip;
	tip->alpha = 0;
	tip->setCircularAreaFromCenter(worldLeftCenter, 240);
	tip->setText(stringbank.get(14), Vector(200,450), 350);
	game->addRenderObject(tip, LR_HUD);
	songTips.push_back(tip);


	/*
	tip = new ToolTip;
	tip->alpha = 0;
	tip->setAreaFromCenter(Vector(400,300), 800, 600);
	tip->setText(stringbank.get(16), Vector(400,300), 400);
	game->addRenderObject(tip, LR_HUD);
	petTips.push_back(tip);
	*/

	tip = new ToolTip;
	tip->alpha = 0;
	tip->setCircularAreaFromCenter(worldLeftCenter, 240);
	tip->setText(stringbank.get(17), Vector(200,450), 350);
	game->addRenderObject(tip, LR_HUD);
	petTips.push_back(tip);

	tip = new ToolTip;
	tip->alpha = 0;
	tip->setAreaFromCenter(Vector(400,350), 150, 50);
	tip->setText(stringbank.get(15), Vector(400,450), 450);
	game->addRenderObject(tip, LR_HUD);
	songTips.push_back(tip);
	foodTips.push_back(tip);
	petTips.push_back(tip);
	treasureTips.push_back(tip);

	int radius = 118;
	int food = 0;

	keyConfigButton = new AquariaMenuItem;
	keyConfigButton->useQuad("gui/keyconfig-button");
	keyConfigButton->useGlow("particles/glow", 128, 40);
	keyConfigButton->position = worldRightCenter + Vector(0, 80);
	keyConfigButton->alpha = 0;
	keyConfigButton->scale = Vector(0.8f, 0.8f);
	keyConfigButton->event.set(MakeFunctionEvent(InGameMenu, onKeyConfig));
	//keyConfigButton->setCanDirMove(false);
	game->addRenderObject(keyConfigButton, LR_MENU);


#define SB(x) stringbank.get(x)

	keyConfigBg = new RoundedRect();
	keyConfigBg->position = Vector(400,276 - 10 - 40);
	keyConfigBg->setWidthHeight(580, 455, 10);
	keyConfigBg->followCamera = 1;
	keyConfigBg->alpha = 0;
	keyConfigBg->shareAlphaWithChildren = 1;
	keyConfigBg->setHidden(true);
	game->addRenderObject(keyConfigBg, LR_OVERLAY);

	float offy = 30 - keyConfigBg->position.y;
	const float offx = 140 - keyConfigBg->position.x;
	const float yi = 20;

	keyConfigInstructions = new TTFText(&dsq->fontArialSmallest);
	keyConfigInstructions->position = Vector(0, offy + 415);
	keyConfigInstructions->setText(SB(2159));
	keyConfigInstructions->color = Vector(0.6f, 0.7f, 0.8f);
	keyConfigInstructions->alphaMod = 0;
	keyConfigInstructions->setAlign(ALIGN_CENTER);
	keyConfigBg->addChild(keyConfigInstructions, PM_POINTER);

	TTFText *joystickLabel = new TTFText(&dsq->fontArialSmall);
	joystickLabel->position = Vector(offx, offy + 390);
	joystickLabel->setText(SB(2140));
	keyConfigBg->addChild(joystickLabel, PM_POINTER);

	AquariaMenuItem *nextJoystickBtn = new AquariaMenuItem;
	nextJoystickBtn->event.set(MakeFunctionEvent(InGameMenu, nextJoystick));
	nextJoystickBtn->useQuad("gui/sort");
	nextJoystickBtn->useGlow("particles/glow", 40, 40);
	nextJoystickBtn->position = Vector(offx + 100, offy + 390);
	keyConfigBg->addChild(nextJoystickBtn, PM_POINTER);

	joystickNameText = new TTFText(&dsq->fontArialSmallest);
	joystickNameText->position = Vector(offx + 100 + 32, offy + 380);
	keyConfigBg->addChild(joystickNameText, PM_POINTER);

	joystickGUIDText = new TTFText(&dsq->fontArialSmallest);
	joystickGUIDText->position = Vector(offx + 100 + 32, offy + 380 + 15);
	joystickGUIDText->color = Vector(0.5f, 0.5f, 0.5f);
	keyConfigBg->addChild(joystickGUIDText, PM_POINTER);

	joystickButtonsText = new TTFText(&dsq->fontArialSmallest);
	joystickButtonsText->position = Vector(offx + 100 + 32, offy + 380 + 30);
	joystickButtonsText->color = Vector(0.7f, 0.7f, 0.7f);
	keyConfigBg->addChild(joystickButtonsText, PM_POINTER);

	updateJoystickText();

	actionSetBox = NULL;
	actionSetCheck = NULL;
	if(dsq->user.control.actionSets.size() > 1)
	{
		float x = offx;
		TTFText *header_actionset = new TTFText(&dsq->fontArialSmall);
		header_actionset->setText(SB(2133));
		header_actionset->position = Vector(x, offy);
		keyConfigBg->addChild(header_actionset, PM_POINTER);
		x += header_actionset->getActualWidth() + 20 + 100;

		actionSetBox = new AquariaComboBox;
		actionSetBox->position = Vector(x, offy);
		keyConfigBg->addChild(actionSetBox, PM_POINTER);
		updateActionSetComboBox();
		x += 200;

		TTFText *header_enabled = new TTFText(&dsq->fontArialSmall);
		header_enabled->setText(SB(2134));
		header_enabled->position = Vector(x, offy+5);
		keyConfigBg->addChild(header_enabled, PM_POINTER);
		x += header_enabled->getActualWidth() + 20;

		actionSetCheck = new AquariaCheckBox;
		actionSetCheck->position = Vector(x, offy);
		keyConfigBg->addChild(actionSetCheck, PM_POINTER);
	}
	offy += 40;


	Quad *keyConfigPagesBg = new Quad;
	keyConfigPagesBg->alphaMod = 0.6f;
	keyConfigPagesBg->color = Vector(0.2f,0.2f,0.3f);
	keyConfigPagesBg->position = Vector(0, offy);
	keyConfigPagesBg->setWidthHeight(580, 40);
	keyConfigBg->addChild(keyConfigPagesBg, PM_POINTER);
	keyConfigPagesBg->moveToBack();

	TTFText *header_tabs = new TTFText(&dsq->fontArialSmall);
	header_tabs->setText(SB(2130));
	header_tabs->position = Vector(offx, offy);
	keyConfigBg->addChild(header_tabs, PM_POINTER);

	keyCategoryButtons.clear();
	keyConfigs.clear();

	for(int i = 0; i < NUM_KEY_CONFIG_PAGES; ++i)
	{
		const float w = 100;
		const std::string& label = SB(2150+i);
		AquariaButton *b = new AquariaButton("gui/simpleblue", &dsq->fontArialSmall);
		b->scale = Vector(0.8f, 0.8f);
		b->position = Vector(150 + offx + i * (w+10), offy);
		b->setButtonLabel(label);
		const Vector color(0.45f, 0.45f, 0.7f);
		b->inactiveColor = color;
		b->inactiveAlpha = 0.5f;
		b->quad->color = color;
		keyConfigBg->addChild(b, PM_POINTER);
		keyCategoryButtons.push_back(b);
	}
	keyCategoryButtons[0]->event.set(MakeFunctionEvent(InGameMenu, switchToKeyConfigPage1));
	keyCategoryButtons[1]->event.set(MakeFunctionEvent(InGameMenu, switchToKeyConfigPage2));
	keyCategoryButtons[2]->event.set(MakeFunctionEvent(InGameMenu, switchToKeyConfigPage3));

	offy += 2*yi;

	TTFText *header_action = new TTFText(&dsq->fontArialSmall);
	header_action->setText(SB(2101));
	header_action->position = Vector(offx, offy);
	keyConfigBg->addChild(header_action, PM_POINTER);

	TTFText *header_mouse = new TTFText(&dsq->fontArialSmall);
	header_mouse->setText(SB(2131));
	header_mouse->position = Vector(offx+KEYCONFIG_FIRST_COL_DISTANCE, offy);
	header_mouse->setAlign(ALIGN_CENTER);
	keyConfigBg->addChild(header_mouse, PM_POINTER);

	TTFText *header_key1 = new TTFText(&dsq->fontArialSmall);
	header_key1->setText(SB(2102));
	header_key1->position = Vector(offx+KEYCONFIG_FIRST_COL_DISTANCE+KEYCONFIG_COL_DISTANCE, offy);
	header_key1->setAlign(ALIGN_CENTER);
	keyConfigBg->addChild(header_key1, PM_POINTER);

	TTFText *header_key2 = new TTFText(&dsq->fontArialSmall);
	header_key2->setText(SB(2103));
	header_key2->position = Vector(offx+KEYCONFIG_FIRST_COL_DISTANCE+2*KEYCONFIG_COL_DISTANCE, offy);
	header_key2->setAlign(ALIGN_CENTER);
	keyConfigBg->addChild(header_key2, PM_POINTER);

	TTFText *header_joy = new TTFText(&dsq->fontArialSmall);
	header_joy->setText(SB(2104));
	header_joy->position = Vector(offx+KEYCONFIG_FIRST_COL_DISTANCE+3*KEYCONFIG_COL_DISTANCE, offy);
	header_joy->setAlign(ALIGN_CENTER);
	keyConfigBg->addChild(header_joy, PM_POINTER);

	offy += 2*yi+yi/2;

	// PART 1
	{
		RenderObject *kk = createBasicKeyConfig();
		group_keyConfig[0] = kk;

		int y = offy;

		addKeyConfigLine(kk, SB(2107), "PrimaryAction",		offx, y+=yi);
		addKeyConfigLine(kk, SB(2108), "SecondaryAction",		offx, y+=yi);
		addKeyConfigLine(kk, SB(2109), "SwimUp",				offx, y+=yi, true);
		addKeyConfigLine(kk, SB(2110), "SwimDown",				offx, y+=yi, true);
		addKeyConfigLine(kk, SB(2111), "SwimLeft",				offx, y+=yi, true);
		addKeyConfigLine(kk, SB(2112), "SwimRight",			offx, y+=yi, true);
		addKeyConfigLine(kk, SB(2113), "Roll",					offx, y+=yi);
		addKeyConfigLine(kk, SB(2114), "Revert",				offx, y+=yi);
		addKeyConfigLine(kk, SB(2115), "WorldMap",				offx, y+=yi);
		addKeyConfigLine(kk, SB(2127), "Look",				offx, y+=yi);

		y+=yi+yi/2;
		AquariaKeyConfig* s1x = addAxesConfigLine(kk, SB(2117), "s1ax", offx, y);
		AquariaKeyConfig* s1y = addAxesConfigLine(kk, SB(2118), "s1ay", offx+130, y);
		AquariaKeyConfig* s2x = addAxesConfigLine(kk, SB(2119), "s2ax", offx+260, y);
		AquariaKeyConfig* s2y = addAxesConfigLine(kk, SB(2120), "s2ay", offx+380, y);

		s1x->setDirMove(DIR_LEFT, s1x);
		s1x->setDirMove(DIR_RIGHT, s1y);

		s1y->setDirMove(DIR_LEFT, s1x);
		s1y->setDirMove(DIR_RIGHT, s2x);

		s2x->setDirMove(DIR_LEFT, s1y);
		s2x->setDirMove(DIR_RIGHT, s2y);

		s2y->setDirMove(DIR_LEFT, s2x);
		s2y->setDirMove(DIR_RIGHT, s2y);
	}

	// PART 2
	{
		RenderObject *kk = createBasicKeyConfig();
		group_keyConfig[1] = kk;

		int y = offy;

		addKeyConfigLine(kk, SB(2116), "Escape",		offx, y+=yi);
		addKeyConfigLine(kk, SB(2128), "ToggleHelp",	offx, y+=yi);
		addKeyConfigLine(kk, SB(2121), "PrevPage",		offx, y+=yi);
		addKeyConfigLine(kk, SB(2122), "NextPage",		offx, y+=yi);
		addKeyConfigLine(kk, SB(2123), "CookFood",		offx, y+=yi);
		addKeyConfigLine(kk, SB(2124), "FoodLeft",		offx, y+=yi);
		addKeyConfigLine(kk, SB(2125), "FoodRight",	offx, y+=yi);
		addKeyConfigLine(kk, SB(2126), "FoodDrop",		offx, y+=yi);
	}

	// PART 2
	{
		RenderObject *kk = createBasicKeyConfig();
		group_keyConfig[2] = kk;

		int y = offy;
		std::string slotstr = SB(2129);
		for(unsigned i = 1; i <= 10; ++i) // SongSlot starts at 1
		{
			std::ostringstream osname;
			osname << slotstr << ' ' << i;
			std::ostringstream osac;
			osac << "SongSlot" << i;
			addKeyConfigLine(kk, osname.str(), osac.str(), offx, y+=yi);
		}
		addKeyConfigLine(kk, SB(2132), "Screenshot",		offx, y+=yi);
	}

	if(actionSetBox)
		actionSetBox->moveToFront();


#undef SB

	cook = new AquariaMenuItem;
	cook->useQuad("Gui/cook-button");
	cook->useGlow("particles/glow", 128, 40);
	cook->position = worldRightCenter + Vector(0, -120);
	cook->alpha = 0;
	cook->scale = Vector(0.8f, 0.8f);
	cook->event.set(MakeFunctionEvent(InGameMenu, onCook));
	cook->setCanDirMove(false);
	game->addRenderObject(cook, LR_MENU);

	foodSort = new AquariaMenuItem;
	foodSort->useQuad("gui/sort");
	foodSort->useSound("click");
	foodSort->useGlow("particles/glow", 32,32);
	foodSort->position = worldLeftCenter + Vector(-100, -100);
	foodSort->event.set(MakeFunctionEvent(InGameMenu, sortFood));
	foodSort->alpha = 0;
	game->addRenderObject(foodSort, LR_MENU);

	recipes = new AquariaMenuItem;
	recipes->useQuad("Gui/recipes-button");
	recipes->useGlow("particles/glow", 128, 32);
	recipes->position = worldLeftCenter + Vector(-40, 140);
	recipes->alpha = 0;
	recipes->scale = Vector(0.8f, 0.8f);
	recipes->event.set(MakeFunctionEvent(InGameMenu, onRecipes));
	game->addRenderObject(recipes, LR_MENU);

	use = new AquariaMenuItem;
	use->useQuad("Gui/use-button");
	use->useGlow("particles/glow", 128, 64);
	use->position = worldRightCenter + Vector(0, -120);
	use->alpha = 0;
	use->scale = Vector(0.8f, 0.8f);
	use->event.set(MakeFunctionEvent(InGameMenu, onUseTreasure));
	game->addRenderObject(use, LR_MENU);

	prevFood = new AquariaMenuItem;
	prevFood->useQuad("Gui/arrow-left");
	prevFood->useSound("click");
	prevFood->useGlow("particles/glow", 64, 32);
	prevFood->position = worldLeftCenter + Vector(-50, -130);
	prevFood->alpha = 0;
	prevFood->event.set(MakeFunctionEvent(InGameMenu, onPrevFoodPage));
	prevFood->scale = Vector(0.6f, 0.6f);
	prevFood->setCanDirMove(false);
	game->addRenderObject(prevFood, LR_MENU);

	nextFood = new AquariaMenuItem;
	nextFood->useQuad("Gui/arrow-right");
	nextFood->useSound("click");
	nextFood->useGlow("particles/glow", 64, 32);
	nextFood->position = worldLeftCenter + Vector(50, -130);
	nextFood->alpha = 0;
	nextFood->setCanDirMove(false);
	nextFood->event.set(MakeFunctionEvent(InGameMenu, onNextFoodPage));
	nextFood->scale = Vector(0.6f, 0.6f);
	game->addRenderObject(nextFood, LR_MENU);

	prevTreasure = new AquariaMenuItem;
	prevTreasure->useQuad("Gui/arrow-left");
	prevTreasure->useSound("click");
	prevTreasure->useGlow("particles/glow", 64, 32);
	prevTreasure->position = worldLeftCenter + Vector(-50, -130);
	prevTreasure->alpha = 0;
	prevTreasure->setCanDirMove(false);
	prevTreasure->scale = Vector(0.6f, 0.6f);
	prevTreasure->event.set(MakeFunctionEvent(InGameMenu, onPrevTreasurePage));
	prevTreasure->setCanDirMove(false);
	game->addRenderObject(prevTreasure, LR_MENU);

	nextTreasure = new AquariaMenuItem;
	nextTreasure->useQuad("Gui/arrow-right");
	nextTreasure->useSound("click");
	nextTreasure->useGlow("particles/glow", 64, 32);
	nextTreasure->position = worldLeftCenter + Vector(50, -130);
	nextTreasure->alpha = 0;
	nextTreasure->scale = Vector(0.6f, 0.6f);
	nextTreasure->event.set(MakeFunctionEvent(InGameMenu, onNextTreasurePage));
	nextTreasure->setCanDirMove(false);
	game->addRenderObject(nextTreasure, LR_MENU);

	circlePageNum = new BitmapText(dsq->smallFont);
	circlePageNum->color = Vector(0,0,0);
	circlePageNum->position = worldLeftCenter + Vector(0, -142);
	circlePageNum->alpha = 0;
	circlePageNum->followCamera = 1;
	game->addRenderObject(circlePageNum, LR_MENU);

	foodHolders.resize(3);
	int holders=0;
	for (size_t i = 0; i < foodHolders.size(); i++)
	{
		foodHolders[i] = new FoodHolder(i);
		foodHolders[i]->toggleValid(false);

		float angle = (float(holders)/float(foodHolders.size()))*PI*2;
		foodHolders[i]->position = rightCenter + Vector(sinf(angle), cosf(angle))*radius;
		holders ++;

		menuBg->addChild(foodHolders[i], PM_POINTER);
	}

	previewRecipe = new Quad;
	previewRecipe->alphaMod = 0.75;
	previewRecipe->followCamera = 1;
	previewRecipe->alpha = 0;
	previewRecipe->scale = Vector(0.7f, 0.7f);
	previewRecipe->scale.interpolateTo(Vector(0.9f, 0.9f), 0.5f, -1, 1, 1);
	previewRecipe->position = worldRightCenter;
	game->addRenderObject(previewRecipe, LR_MENU);

	showRecipe = new Quad();
	showRecipe->followCamera = 1;
	showRecipe->position = Vector(575,250);
	game->addRenderObject(showRecipe, LR_MENU);

	float scrollx = 555;
	recipeMenu.scroll = new Quad("gui/recipe-scroll", Vector(scrollx, 200));
	recipeMenu.scroll->followCamera = 1;
	recipeMenu.scroll->alpha = 0;
	game->addRenderObject(recipeMenu.scroll, LR_RECIPES); // LR_HUD3

	recipeMenu.scrollEnd = new Quad("gui/recipe-scroll-end", Vector(scrollx, 400));
	recipeMenu.scrollEnd->followCamera = 1;
	recipeMenu.scrollEnd->alpha = 0;
	game->addRenderObject(recipeMenu.scrollEnd, LR_RECIPES);

	recipeMenu.header = new BitmapText(dsq->font);
	recipeMenu.header->color = 0;
	recipeMenu.header->followCamera = 1;
	recipeMenu.header->setText(stringbank.get(2007));
	recipeMenu.header->alpha = 0;
	recipeMenu.header->position = Vector(scrollx, 5); //10
	game->addRenderObject(recipeMenu.header, LR_RECIPES);

	recipeMenu.page = new BitmapText(dsq->smallFont);
	recipeMenu.page->color = 0;
	recipeMenu.page->followCamera = 1;
	recipeMenu.page->position = Vector(scrollx, 400);
	recipeMenu.page->setText(stringbank.get(2006));
	recipeMenu.page->alpha = 0;
	game->addRenderObject(recipeMenu.page, LR_RECIPES);

	recipeMenu.prevPage = new AquariaMenuItem;
	recipeMenu.prevPage->useQuad("Gui/arrow-left");
	recipeMenu.prevPage->useSound("click");
	recipeMenu.prevPage->useGlow("particles/glow", 64, 32);
	recipeMenu.prevPage->position = Vector(scrollx - 150, 410);
	recipeMenu.prevPage->alpha = 0;
	recipeMenu.prevPage->event.set(MakeFunctionEvent(InGameMenu, onPrevRecipePage));
	recipeMenu.prevPage->scale = Vector(0.8f, 0.8f);
	game->addRenderObject(recipeMenu.prevPage, LR_RECIPES);

	recipeMenu.nextPage = new AquariaMenuItem;
	recipeMenu.nextPage->useQuad("Gui/arrow-right");
	recipeMenu.nextPage->useSound("click");
	recipeMenu.nextPage->useGlow("particles/glow", 64, 32);
	recipeMenu.nextPage->position = Vector(scrollx + 150, 410);
	recipeMenu.nextPage->alpha = 0;
	recipeMenu.nextPage->event.set(MakeFunctionEvent(InGameMenu, onNextRecipePage));
	recipeMenu.nextPage->scale = Vector(0.8f, 0.8f);
	game->addRenderObject(recipeMenu.nextPage, LR_RECIPES);


	petSlots.resize(dsq->continuity.petData.size());
	for (size_t i = 0; i < petSlots.size(); i++)
	{
		PetData *p = dsq->continuity.getPetData(i);
		if (p)
		{
			petSlots[i] = new PetSlot(i);
			petSlots[i]->alpha = 0;
			float angle = (float(i)/float(petSlots.size()))*PI*2;
			petSlots[i]->position = center + Vector(sinf(angle), cosf(angle))*(radius*0.9f);
			menuBg->addChild(petSlots[i], PM_POINTER);
		}
	}

	foodHolders.resize(4);
	foodHolders[3] = new FoodHolder(-1, true);
	foodHolders[3]->toggleValid(false);
	foodHolders[3]->position = rightCenter + Vector(96, 150);
	menuBg->addChild(foodHolders[3], PM_POINTER);



	int outer = 0;
	int inner = 0;
	for (size_t i = 0; i < songSlots.size(); i++)
	{
		songSlots[i] = new SongSlot(i);
		float angle = 0;
		SongType s = (SongType)dsq->continuity.getSongTypeBySlot(i);
		if (dsq->continuity.isSongTypeForm(s))
		{
			angle = (float(outer)/float(numForms))*PI*2;
			songSlots[i]->position = center + Vector(sinf(angle), cosf(angle))*radius;
			outer ++;
		}
		else
		{
			angle = (float(inner)/float(songSlots.size()-numForms))*PI*2 + PI;
			songSlots[i]->position = center + Vector(sinf(angle), cosf(angle))*radius*0.4f;
			inner ++;
		}
		menuBg->addChild(songSlots[i], PM_POINTER);
	}

	menuDescription = new BitmapText(dsq->smallFont);
	menuDescription->setFontSize(14);
	menuDescription->position = Vector(400, 450);
	menuDescription->setAlign(ALIGN_CENTER);
	menuDescription->setWidth(400);
	menuDescription->followCamera = 1;
	menuDescription->alpha = 0;
	game->addRenderObject(menuDescription, LR_MENU);

	int areYouShim = -25;
	eAre = new Quad;
	eAre->position = Vector(400,448+areYouShim);
	eAre->setTexture("AreYouSure");
	eAre->alpha = 0;
	eAre->followCamera = 1;
	game->addRenderObject(eAre, LR_MENU);

	eYes = new AquariaMenuItem;
	eYes->position = Vector(400-100,516+areYouShim);
	eYes->useQuad("Yes");
	eYes->useGlow("particles/glow", 100, 32);
	eYes->event.set(MakeFunctionEvent(InGameMenu, onExitCheckYes));
	eYes->alpha = 0;
	eYes->shareAlpha = 1;
	game->addRenderObject(eYes, LR_MENU);

	eNo = new AquariaMenuItem;
	eNo->position = Vector(400+100,516+areYouShim);
	eNo->useQuad("No");
	eNo->useGlow("particles/glow", 100, 32);
	eNo->event.set(MakeFunctionEvent(InGameMenu, onExitCheckNo));
	eNo->alpha = 0;
	eNo->shareAlpha = 1;
	game->addRenderObject(eNo, LR_MENU);

	eNo->setDirMove(DIR_LEFT, eYes);
	eYes->setDirMove(DIR_RIGHT, eNo);



	menu.resize(10);
	for (size_t i = 0; i < menu.size(); i++)
		menu[i] = new AquariaMenuItem;

	int ty = 530;
	//menu[0]->setLabel("Continue");
	menu[0]->event.set(MakeFunctionEvent(InGameMenu, onInGameMenuContinue));
	menu[0]->useGlow("particles/glow", 200, 100);
	//menu[0]->position = Vector(150, 550);
	menu[0]->position = Vector(150-30, ty-10);

	//menu[1]->setLabel("Exit");
	menu[1]->useGlow("particles/glow", 200, 100);
	menu[1]->event.set(MakeFunctionEvent(InGameMenu, onInGameMenuExit));
	//menu[1]->position = Vector(800-150, 550);
	//menu[1]->position = Vector(800-150+30, ty);
	menu[1]->position = Vector(800-150+20, ty-10);

	menu[2]->setLabel("DebugSave");
	menu[2]->event.set(MakeFunctionEvent(InGameMenu, onDebugSave));
	menu[2]->position = Vector(400,ty+60);
	if (!dsq->isDeveloperKeys())
		menu[2]->position = Vector(400, 12000);
	menu[2]->setCanDirMove(false);

	menu[3]->useGlow("particles/glow", 64, 64);
	//menu[0]->position = Vector(150, 550);
	menu[3]->position = Vector(400, 195);
	menu[3]->setCanDirMove(false);

	lips = menu[3];

	// options
	menu[4]->event.set(MakeFunctionEvent(InGameMenu, onOptionsMenu));
	menu[4]->useGlow("particles/glow", 200, 32);
	menu[4]->position = Vector(400,ty+10);

	int gs = 40;

	menu[5]->event.set(MakeFunctionEvent(InGameMenu, switchToSongMenu));
	menu[5]->useQuad("gui/icon-songs");
	menu[5]->useGlow("particles/glow", gs, gs);
	menu[5]->useSound("Click");
	menu[5]->position = Vector(400-60, 350);

	menuIconGlow = new Quad("particles/glow", menu[5]->position);
	menuIconGlow->alphaMod = 0.4f;
	menuIconGlow->alpha = 0;
	menuIconGlow->setWidthHeight(80, 80);
	menuIconGlow->setBlendType(BLEND_ADD);
	menuIconGlow->followCamera = 1;
	game->addRenderObject(menuIconGlow, LR_MENU);

	menu[6]->event.set(MakeFunctionEvent(InGameMenu, switchToFoodMenu));
	menu[6]->useQuad("gui/icon-food");
	menu[6]->useGlow("particles/glow", gs, gs);
	menu[6]->useSound("Click");
	menu[6]->position = Vector(400-20, 350);

	menu[7]->event.set(MakeFunctionEvent(InGameMenu, switchToPetMenu));
	menu[7]->useQuad("gui/icon-pets");
	menu[7]->useGlow("particles/glow", gs, gs);
	menu[7]->useSound("Click");
	menu[7]->position = Vector(400+20, 350);

	menu[8]->event.set(MakeFunctionEvent(InGameMenu, switchToTreasureMenu));
	menu[8]->useQuad("gui/icon-treasures");
	menu[8]->useGlow("particles/glow", gs, gs);
	menu[8]->useSound("Click");
	menu[8]->position = Vector(400+60, 350);

	menu[9]->event.set(MakeFunctionEventPointer( Game, toggleHelpScreen, game));
	menu[9]->useQuad("gui/icon-help");
	menu[9]->useGlow("particles/glow", gs, gs);
	menu[9]->useSound("Click");
	menu[9]->position = Vector(400+60*3, 410);

	/*
	menu[9]->event.set(MakeFunctionEvent(Game, sortFood));
	menu[9]->setLabel("sort food");
	menu[9]->position = Vector(100,100);
	*/

	for (size_t i = 0; i < menu.size(); i++)
	{
		game->addRenderObject(menu[i], LR_MENU);
		menu[i]->alpha = 0;
	}

	((AquariaMenuItem*)menu[5])->setDirMove(DIR_DOWN, ((AquariaMenuItem*)menu[0]));
	((AquariaMenuItem*)menu[6])->setDirMove(DIR_DOWN, ((AquariaMenuItem*)menu[4]));
	((AquariaMenuItem*)menu[7])->setDirMove(DIR_DOWN, ((AquariaMenuItem*)menu[4]));
	((AquariaMenuItem*)menu[8])->setDirMove(DIR_DOWN, ((AquariaMenuItem*)menu[1]));

	((AquariaMenuItem*)menu[0])->setDirMove(DIR_UP, ((AquariaMenuItem*)menu[5]));
	((AquariaMenuItem*)menu[1])->setDirMove(DIR_UP, ((AquariaMenuItem*)menu[8]));

	((AquariaMenuItem*)menu[4])->setDirMove(DIR_UP, ((AquariaMenuItem*)menu[6]));



	// ---------- FOOD MENU

	foodSlots.resize(foodPageSize);

	Vector worldCenter(222, 252);

	int foodSlotRadius = 96;
	for (size_t i = 0; i < foodSlots.size(); i++)
	{
		foodSlots[i] = new FoodSlot(i);

		float angle = (float(food)/float(foodSlots.size()))*PI*2;
		foodSlots[i]->position = worldCenter + Vector(sinf(angle), cosf(angle))*foodSlotRadius;

		foodSlots[i]->setOriginalPosition(foodSlots[i]->position);

		food ++;

		foodSlots[i]->alphaMod = 0;

		foodSlots[i]->followCamera = 1;

		foodSlots[i]->scaleFactor = 0.75;

		//foodSlots[i]->parentManagedPointer = 1;
		//menuBg->addChild(foodSlots[i]);
		//foodSlots[i]->position = menuBg->getWorldCollidePosition(foodSlots[i]->position);
		game->addRenderObject(foodSlots[i], LR_HUD2);
	}


	foodLabel = new BitmapText(dsq->smallFont);
	{
		foodLabel->alpha = 0;
		foodLabel->setAlign(ALIGN_CENTER);
		foodLabel->followCamera = 1;
		foodLabel->setFontSize(20);
		foodLabel->position = center - Vector(0, 16) + Vector(0,-32);
		foodLabel->scale = Vector(1, 1);
	}
	menuBg->addChild(foodLabel, PM_POINTER);

	foodDescription = new BitmapText(dsq->smallFont);
	{
		foodDescription->alpha = 0;
		foodDescription->setAlign(ALIGN_CENTER);
		foodDescription->followCamera = 1;
		foodDescription->position = center + Vector(0, 8) + Vector(0,-32);
		foodDescription->scale = Vector(0.8f, 0.8f);

		foodDescription->setWidth(240);
	}
	menuBg->addChild(foodDescription, PM_POINTER);


	// ---------- TREASURES


	int treasureSlotRadius = 96;

	treasureSlots.resize(treasurePageSize);

	for (size_t i = 0; i < treasureSlots.size(); i++)
	{
		treasureSlots[i] = new TreasureSlot(i);


		float angle = (float(i)/float(treasureSlots.size()))*PI*2;
		treasureSlots[i]->position = worldCenter + Vector(sinf(angle), cosf(angle))*treasureSlotRadius;

		treasureSlots[i]->alphaMod = 0;

		treasureSlots[i]->followCamera = 1;

		//treasureSlots[i]->scaleFactor = 0.75;

		game->addRenderObject(treasureSlots[i], LR_MENU);
	}

	treasureLabel = new BitmapText(dsq->smallFont);
	{
		treasureLabel->alpha = 0;
		treasureLabel->setAlign(ALIGN_CENTER);
		treasureLabel->followCamera = 1;
		treasureLabel->setFontSize(20);
		treasureLabel->position = center - Vector(0, 16);
		treasureLabel->scale = Vector(1, 1);
	}
	menuBg->addChild(treasureLabel, PM_POINTER);

	treasureDescription = new ToolTip();
	treasureDescription->alpha = 0;
	treasureDescription->setAreaFromCenter(Vector(400,200), 800, 400);
	treasureDescription->required = true;
	game->addRenderObject(treasureDescription, LR_HUD);

	foodTips.push_back(tip);

	treasureCloseUp = new Quad();
	treasureCloseUp->position = rightCenter;
	treasureCloseUp->alpha = 0;
	menuBg->addChild(treasureCloseUp, PM_POINTER);



	menuBg->alpha = 0;
}

void InGameMenu::onNextRecipePage()
{
	recipeMenu.goNextPage();
}

void InGameMenu::onPrevRecipePage()
{
	recipeMenu.goPrevPage();
}


void InGameMenu::onPrevTreasurePage()
{
	if (currentTreasurePage > 0)
	{
		dsq->sound->playSfx("menu-switch", 0.5);
		dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);

		currentTreasurePage--;
		refreshTreasureSlots();
	}
	else
	{
		if (numTreasures > 0)
		{
			dsq->sound->playSfx("menu-switch", 0.5);
			dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);

			currentTreasurePage = ((numTreasures-1)/treasurePageSize);
			refreshTreasureSlots();
		}
	}
}

void InGameMenu::onNextTreasurePage()
{
	if ((currentTreasurePage+1)*treasurePageSize < numTreasures)
	{
		dsq->sound->playSfx("menu-switch", 0.5);
		dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);

		currentTreasurePage++;
		refreshTreasureSlots();
	}
	else
	{
		if (currentTreasurePage != 0)
		{
			dsq->sound->playSfx("menu-switch", 0.5);
			dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);

			currentTreasurePage = 0;
			refreshTreasureSlots();
		}
	}
}

void InGameMenu::onPrevFoodPage()
{
	int lastFoodPage = currentFoodPage;
	if (currentFoodPage > 0)
	{
		currentFoodPage--;
		refreshFoodSlots(false);
	}
	else
	{
		if (dsq->continuity.hasIngredients())
		{
			currentFoodPage = ((dsq->continuity.ingredientCount()-1)/foodPageSize);
			refreshFoodSlots(false);
		}
	}

	std::ostringstream os;
	os << "food page: " << currentFoodPage;
	debugLog(os.str());

	if (currentFoodPage != lastFoodPage)
	{
		dsq->sound->playSfx("menu-switch", 0.5);
		dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);
	}
}

void InGameMenu::onNextFoodPage()
{
	int lastFoodPage = currentFoodPage;
	if ((currentFoodPage+1)*foodPageSize < dsq->continuity.ingredientCount())
	{
		currentFoodPage++;
		refreshFoodSlots(false);
	}
	else
	{
		if (currentFoodPage != 0)
		{
			currentFoodPage = 0;
			refreshFoodSlots(false);
		}
	}

	if (currentFoodPage != lastFoodPage)
	{
		dsq->sound->playSfx("menu-switch", 0.5);
		dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);
	}
}

void InGameMenu::onUseTreasure()
{
	debugLog("Use Treasure!");

	if (selectedTreasureFlag != -1)
	{
		onUseTreasure(selectedTreasureFlag);
	}
}

void InGameMenu::onUseTreasure(int flag)
{
	if(dsq->mod.isActive())
		dsq->runScriptNum(dsq->mod.getPath() + "scripts/menu-treasures.lua", "useTreasure", flag);
	else
		dsq->runScriptNum("scripts/global/menu-treasures.lua", "useTreasure", flag);
}


static bool ingType(const std::vector<IngredientData*> &list, IngredientType type, int amount=1)
{
	int c = 0;
	for (size_t i = 0; i < list.size(); i++)
	{
		IngredientData *data = list[i];
		if ((data->marked < data->held) && (data->type == type || type == IT_ANYTHING))
		{
			if (type != IT_ANYTHING)
				data->marked++;
			c++;
			if (c == amount)
				return true;
		}
	}
	return false;
}

static bool ingName(const std::vector<IngredientData*> &list, const std::string &name, int amount=1)
{
	int c = 0;
	for (size_t i = 0; i < list.size(); i++)
	{
		IngredientData *data = list[i];
		if ((data->marked < data->held) && (nocasecmp(data->name, name)==0))//data->name == name)
		{
			data->marked++;
			c++;
			if (c == amount)
				return true;
		}
	}
	return false;
}


Recipe *InGameMenu::findRecipe(const std::vector<IngredientData*> &list)
{
	if (list.size() < 2) return 0;

	// there will be a number of types and a number of names
	Recipe *r = 0;
	Recipe *tr = 0;
	size_t q = 0, q2 = 0;
	for (size_t rc = 0; rc < dsq->continuity.recipes.size(); rc++)
	{
		for (size_t i = 0; i < list.size(); i++)
			list[i]->marked = 0;

		tr = 0;
		r = &dsq->continuity.recipes[rc];
		tr = r;
		q = 0;

		// get the amount of ingredients provided by the player
		size_t listAmount = list.size();

		// get the amount of ingredients required
		size_t recipeAmount = 0;

		for (size_t i = 0; i < r->types.size(); i++)
			recipeAmount += r->types[i].amount;

		for (size_t i = 0; i < r->names.size(); i++)
			recipeAmount += r->names[i].amount;

		if (listAmount != recipeAmount)
			continue;

		for (size_t c = 0; c < r->types.size(); c++)
		{
			RecipeType *t = &r->types[c];
			if (ingType(list, t->type, t->amount))
				q++;
			else
				break;
		}

		/*
		// if all the types are checked
		// AND there are no names to check
		// then you found it!
		if (q == r->types.size() && q > 0 && r->names.empty())
		{
		return tr;
		}
		*/

		// this check is _kinda_ unnecessary... but we'll see
		if (q == r->types.size())
		{
			q2 = 0;
			for (size_t c = 0; c < r->names.size(); c++)
			{
				RecipeName *n = &r->names[c];
				if (ingName(list, n->name, n->amount))
					q2++;
				else
					break;
			}
			if (q2 == r->names.size())
			{
				return r;
			}
			/*
			// if there were actually types to check
			// and they were checked successfully
			// (being in this section of code implies that there were no types OR there was a successful full check)
			else if (q>0 && tr)
			{
			// return the ingredient we found in types

			// but this is kind of silly.
			// would make more sense to return earlier
			return tr;
			}
			*/
		}
	}

	for (size_t i = 0; i < list.size(); i++)
		list[i]->marked = 0;

	return 0;
}

void InGameMenu::updateCookList()
{
	cookList.clear();
	for (size_t i = 0; i < foodHolders.size(); i++)
	{
		IngredientData *ing = foodHolders[i]->getIngredient();
		if (!foodHolders[i]->isTrash() && ing)
		{
			std::ostringstream os;
			os << "cooklist: " << ing->name;
			debugLog(os.str());
			cookList.push_back(ing);
		}
	}
}

void InGameMenu::onRecipes()
{
	if (foodMenu)
	{
		toggleRecipeList(!recipeMenu.on);
	}
}

void InGameMenu::onKeyConfig()
{
	dsq->screenTransition->capture();
	toggleKeyConfigMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

#define DEBUG_COOK

void InGameMenu::onCook()
{
	if (recipeMenu.on) return;
	if (cookDelay > 0) return;

	debugLog("Cook!");

	//std::vector<IngredientData*> list;
	updateCookList();

	if (cookList.size() < 2 || recipeMenu.on) return;

	AquariaGuiElement::canDirMoveGlobal = false;

	cookDelay = 0.4f;

	bool cooked = false;

	isCooking = true;

	IngredientData *data=0;
	Recipe *r = findRecipe(cookList);

	if (r)
		data = dsq->continuity.getIngredientDataByName(r->result);
	else if(game->cookingScript)
	{
		const char *p1 = cookList[0]->name.c_str();
		const char *p2 = cookList[1]->name.c_str();
		const char *p3 = cookList.size() >= 3 ? cookList[2]->name.c_str() : "";
		std::string ingname;
		if(game->cookingScript->call("cookFailure", p1, p2, p3, &ingname))
		{
			if(ingname.length())
				data = dsq->continuity.getIngredientDataByName(ingname);
			if(!data)
				goto endcook;
		}
	}

	if(!data)
	{
		dsq->sound->playSfx("Denied");
		data = dsq->continuity.getIngredientDataByName("SeaLoaf");

		bool tooMany = data && dsq->continuity.isIngredientFull(data);

		if (!tooMany)
		{
			int f = dsq->continuity.getFlag(FLAG_SEALOAFANNOYANCE);
			f++;
			if (f >= 3)
			{
				dsq->voiceInterupt("naija_sealoaf");
				f = 0;
			}
			dsq->continuity.setFlag(FLAG_SEALOAFANNOYANCE, f);
		}
	}

	if (data)
	{
		cooked = !dsq->continuity.isIngredientFull(data);
	}

	if (cooked)
	{
		debugLog("Cooked something!");

		// do animationy stuff.

		core->mouse.buttonsEnabled = false;

		bool longAnim = true;
		int cooks = dsq->continuity.getFlag(FLAG_COOKS);

		if (cooks >= 4)
			longAnim = false;

		for (int i = foodHolders.size()-1; i >= 0; i--)
			if (foodHolders[i]->isValid() && !foodHolders[i]->isEmpty() && !foodHolders[i]->isTrash())
				foodHolders[i]->animateLid(true, longAnim);

		//dsq->main(0.2);


		if (longAnim)
		{
			float ft = 0.8f;
			float nt = 0.1f;
			float nt2 = 0.2f;
			void *handle = NULL;

			/*
			if (!longAnim)
			{
			float factor = 0.3;
			ft *= factor;
			nt *= factor;
			nt2 *= factor;
			}
			*/

			PlaySfx note1;
			note1.name = game->getNoteName(0);
			PlaySfx note2;
			note2.name = game->getNoteName(4);
			PlaySfx note3;
			note3.name = game->getNoteName(3);

			handle = dsq->sound->playSfx(note1);
			dsq->run(nt2);
			dsq->sound->fadeSfx(handle, SFT_OUT, ft);
			dsq->run(nt);

			handle = dsq->sound->playSfx(note2);
			dsq->run(nt2);
			dsq->sound->fadeSfx(handle, SFT_OUT, ft);
			dsq->run(nt);

			handle = dsq->sound->playSfx(note3);
			dsq->run(nt2);
			dsq->sound->fadeSfx(handle, SFT_OUT, ft);
			dsq->run(nt);
		}

		dsq->sound->playSfx("boil");

		for (size_t i = 0; i < foodHolders.size(); i++)
		{
			if (!foodHolders[i]->isEmpty())
				dsq->spawnParticleEffect("cook-ingredient", foodHolders[i]->getWorldPosition(), 0, 0, LR_HUD3, 1);
		}

		if (longAnim)
			dsq->run(0.5f);
		else
			dsq->run(0.2f);

		bool haveLeftovers = true;
		for (size_t i = 0; i < foodHolders.size(); i++)
		{
			if (!foodHolders[i]->isEmpty()) {
				IngredientData *ing = foodHolders[i]->getIngredient();
				if (!ing || ing->amount < ing->held)
				{
					haveLeftovers = false;
					break;
				}
			}
		}
		for (size_t i = 0; i < foodHolders.size(); i++)
		{
			IngredientData *ing = foodHolders[i]->getIngredient();
			if (ing)
			{
				ing->amount--;
			}

			if (!haveLeftovers)
			{
				foodHolders[i]->setIngredient(0, false);
			}
		}

		dsq->sound->playSfx("Cook");

		for (size_t i = 0; i < foodHolders.size(); i++)
			if (foodHolders[i]->isValid() && !foodHolders[i]->isTrash())
				foodHolders[i]->animateLid(false);

		dsq->spawnParticleEffect("cook-food", Vector(575,250), 0, 0, LR_HUD3, 1);

		if (longAnim)
			dsq->run(0.5f);
		else
			dsq->run(0.2f);

		if (data)
		{
			float t = 3;
			std::string n = "Ingredients/" + data->gfx;
			//Quad *e = new Quad();

			showRecipe->setTexture(n);
			showRecipe->scale = Vector(0.5f, 0.5f);
			showRecipe->scale.interpolateTo(Vector(1.2f, 1.2f), t);
			showRecipe->alpha.ensureData();
			showRecipe->alpha.data->path.clear();
			showRecipe->alpha.data->path.addPathNode(0, 0);
			showRecipe->alpha.data->path.addPathNode(1, 0.1f);
			showRecipe->alpha.data->path.addPathNode(1, 0.6f);
			showRecipe->alpha.data->path.addPathNode(0, 1);
			showRecipe->alpha.startPath(t);
		}

		dsq->continuity.pickupIngredient(data, 1);

		dsq->continuity.removeEmptyIngredients();

		dsq->run(0.5);

		dsq->continuity.setFlag(FLAG_COOKS, dsq->continuity.getFlag(FLAG_COOKS)+1);

		if (r)
		{
			dsq->continuity.learnRecipe(r);
			if (haveLeftovers)
				updatePreviewRecipe();
		}

		core->mouse.buttonsEnabled = true;
	}
	else
	{
		dsq->sound->playSfx("Denied");
		dsq->centerMessage(stringbank.get(27));
	}
	refreshFoodSlots(true);

endcook:

	AquariaGuiElement::canDirMoveGlobal = true;

	isCooking = false;
}


FoodSlot* InGameMenu::getFoodSlotFromIndex()
{
	for (size_t i = 0; i < foodSlots.size(); i++)
	{
		if (foodSlots[i]->slot == FoodSlot::foodSlotIndex)
		{
			return foodSlots[i];
		}
	}
	return 0;
}

void InGameMenu::onExitCheckYes()
{
	dsq->sound->stopAllVoice();
	dsq->toggleCursor(0, 0.25);
	dsq->title(true);
}

void InGameMenu::onExitCheckNo()
{
	hideInGameMenuExitCheck(true);
}

void InGameMenu::showInGameMenuExitCheck()
{
	recipeMenu.toggle(false);
	inGameMenuExitState = 1;
	eYes->alpha.interpolateTo(1, 0.2f);
	eNo->alpha.interpolateTo(1, 0.2f);
	eAre->alpha.interpolateTo(1, 0.2f);

	eNo->setFocus(true);
}

void InGameMenu::hideInGameMenuExitCheck(bool refocus)
{
	inGameMenuExitState = 0;
	eYes->alpha.interpolateTo(0, 0.2f);
	eNo->alpha.interpolateTo(0, 0.2f);
	eAre->alpha.interpolateTo(0, 0.2f);

	if (refocus)
		((AquariaMenuItem*)menu[1])->setFocus(true);
}

void InGameMenu::onInGameMenuExit()
{
	if (!dsq->user.demo.warpKeys || (core->getCtrlState() && core->getAltState()))
	{
		if (inGameMenuExitState == 0)
		{
			// show yes/no
			showInGameMenuExitCheck();
		}
	}
}

void InGameMenu::onInGameMenuContinue()
{
	hide();
}


void InGameMenu::onOptionsMenu()
{
	dsq->screenTransition->capture();
	toggleOptionsMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void InGameMenu::onOptionsSave()
{
	if (resBox)
	{
		size_t itm = resBox->getSelectedItem();
		if(itm < core->screenModes.size()) // Required because the menu appends another element if it can't select one in the list
		{
			const ScreenMode& m = core->screenModes[itm];
			dsq->user.video.resx = m.x;
			dsq->user.video.resy = m.y;
			dsq->user.video.hz = m.hz;
		}
	}

	dsq->user.apply();

	const UserSettings::Video& bv = dsq->user_backup.video;
	UserSettings::Video& v = dsq->user.video;

	if (v.resx != bv.resx
		|| v.resy != bv.resy
		|| v.bits != bv.bits
		|| v.full != bv.full
		|| v.vsync != bv.vsync
		|| v.hz != bv.hz
	)
	{
		dsq->resizeWindow(v.resx, v.resy, v.full,  v.vsync, v.bits, -1, v.hz);
		if (dsq->confirm("", "graphics", false, 10)) {
		} else {
			v.resx = bv.resx;
			v.resy = bv.resy;
			v.bits = bv.bits;
			v.full = bv.full;
			v.vsync = bv.vsync;
			v.hz = bv.hz;

			dsq->user.apply();

			dsq->resizeWindow(v.resx, v.resy, v.full, -1, -1, -1, -1);
		}
	}

	if (!keyConfigMenu)
		dsq->user.save();

	if (keyConfigMenu)
	{
		AquariaKeyConfig::waitingForInput = 0;
		dsq->screenTransition->capture();
		toggleKeyConfigMenu(false);
		toggleOptionsMenu(true, false, true);
		dsq->screenTransition->transition(MENUPAGETRANSTIME);
	}
	else
	{
		if (optionsOnly)
		{
			hide();
		}
		else
		{
			dsq->screenTransition->capture();
			toggleOptionsMenu(false);
			dsq->screenTransition->transition(MENUPAGETRANSTIME);
		}
	}
}

void InGameMenu::onOptionsCancel()
{
	if (!keyConfigMenu)
	{
		dsq->user = dsq->user_backup;
	}
	else
	{
		dsq->user.control.actionSets = dsq->user_bcontrol.control.actionSets;
	}

	dsq->user.apply();

	if (keyConfigMenu)
	{
		AquariaKeyConfig::waitingForInput = 0;
		dsq->screenTransition->capture();
		toggleKeyConfigMenu(false);
		toggleOptionsMenu(true, true, true);
		dsq->screenTransition->transition(MENUPAGETRANSTIME);
	}
	else
	{
		if (optionsOnly)
		{
			hide();
		}
		else
		{
			dsq->screenTransition->capture();
			toggleOptionsMenu(false);
			dsq->screenTransition->transition(MENUPAGETRANSTIME);
		}
	}
}

void InGameMenu::refreshFoodSlots(bool effects)
{
	for (size_t i = 0; i < foodSlots.size(); i++)
	{
		foodSlots[i]->refresh(effects);
	}
}

void InGameMenu::refreshTreasureSlots()
{
	for (size_t i = 0; i < treasureSlots.size(); i++)
	{
		treasureSlots[i]->refresh();
	}
}

void InGameMenu::togglePetMenu(bool f)
{
	if (optionsMenu)
	{
		toggleOptionsMenu(false);
	}

	if (foodMenu)
		toggleFoodMenu(false);
	if (treasureMenu)
		toggleTreasureMenu(false);

	if (f && !petMenu)
	{
		currentMenuPage = MENUPAGE_PETS;

		toggleMainMenu(false);

		bool hasPet = false;
		for (size_t i = 0; i < petSlots.size(); i++)
		{
			petSlots[i]->alpha = 1;
			hasPet = dsq->continuity.getFlag(petSlots[i]->petFlag) || hasPet;
		}
		// act as if they're all active for now...
		if (petSlots.size() == 4)
		{
			petSlots[0]->setDirMove(DIR_RIGHT, petSlots[1]);
			petSlots[0]->setDirMove(DIR_UP, petSlots[2]);
			petSlots[0]->setDirMove(DIR_LEFT, petSlots[3]);
			petSlots[0]->setDirMove(DIR_DOWN, menu[0]);

			menu[0]->setDirMove(DIR_UP, petSlots[0]);

			petSlots[1]->setDirMove(DIR_LEFT, petSlots[3]);
			petSlots[1]->setDirMove(DIR_UP, petSlots[2]);
			petSlots[1]->setDirMove(DIR_DOWN, petSlots[0]);

			petSlots[1]->setDirMove(DIR_RIGHT, menu[5]);
			menu[5]->setDirMove(DIR_LEFT, petSlots[1]);
			menu[5]->setDirMove(DIR_UP, petSlots[1]);

			petSlots[2]->setDirMove(DIR_RIGHT, petSlots[1]);
			petSlots[2]->setDirMove(DIR_DOWN, petSlots[0]);
			petSlots[2]->setDirMove(DIR_LEFT, petSlots[3]);

			petSlots[3]->setDirMove(DIR_UP, petSlots[2]);
			petSlots[3]->setDirMove(DIR_RIGHT, petSlots[1]);
			petSlots[3]->setDirMove(DIR_DOWN, petSlots[0]);
		}


		for (size_t i = 0; i < petTips.size(); i++)
			petTips[i]->alpha = 1;

		liCrystal->alpha = 1;


		menu[7]->setFocus(true);


		doMenuSectionHighlight(2);

	}
	else if (!f && petMenu)
	{
		for (size_t i = 0; i < petSlots.size(); i++)
		{
			petSlots[i]->alpha = 0;
		}

		for (size_t i = 0; i < petTips.size(); i++)
		{
			petTips[i]->alpha = 0;
		}

		liCrystal->alpha = 0;


		menu[5]->setDirMove(DIR_LEFT, 0);
		menu[5]->setDirMove(DIR_UP, 0);
		menu[0]->setDirMove(DIR_UP, 0);
	}

	petMenu = f;
}

void InGameMenu::toggleTreasureMenu(bool f)
{
	//debugLog("toggle treasure menu!");

	if (optionsMenu)
		toggleOptionsMenu(false);

	if (foodMenu)
		toggleFoodMenu(false);
	if (petMenu)
		togglePetMenu(false);

	if (f && !treasureMenu)
	{
		currentMenuPage = MENUPAGE_TREASURES;

		treasureMenu = true;
		toggleMainMenu(false);

		refreshTreasureSlots();

		for (size_t i = 0; i < treasureTips.size(); i++)
			treasureTips[i]->alpha = 1;

		if (treasureSlots.size() > 8)
		{
			treasureSlots[0]->setDirMove(DIR_DOWN, menu[0]);
			menu[0]->setDirMove(DIR_UP, treasureSlots[0]);

			treasureSlots[2]->setDirMove(DIR_RIGHT, menu[5]);
			menu[5]->setDirMove(DIR_LEFT, treasureSlots[2]);

			treasureSlots[3]->setDirMove(DIR_RIGHT, menu[5]);
		}

		menu[8]->setFocus(true);

		doMenuSectionHighlight(3);

		liCrystal->alpha = 1;

		circlePageNum->alpha = 1;
	}
	else if (!f && treasureMenu)
	{
		treasureMenu = false;

		for (size_t i = 0; i < treasureTips.size(); i++)
			treasureTips[i]->alpha = 0;

		menu[0]->setDirMove(DIR_UP, 0);
		menu[5]->setDirMove(DIR_LEFT, 0);

		liCrystal->alpha = 0;

		circlePageNum->alpha = 0;
	}

	for (size_t i = 0; i < treasureSlots.size(); i++)
	{
		if (f)
			treasureSlots[i]->alpha = 1;
		else
			treasureSlots[i]->alpha = 0;
	}

	if (f)
	{
		nextTreasure->alpha = 1;
		prevTreasure->alpha = 1;
		use->alpha = 0;

		treasureLabel->alpha = 0;
		treasureDescription->alpha = 0;
		treasureCloseUp->alpha = 0;
	}
	else
	{
		nextTreasure->alpha = 0;
		prevTreasure->alpha = 0;
		use->alpha = 0;

		treasureLabel->alpha = 0;
		treasureDescription->alpha = 0;
		treasureCloseUp->alpha = 0;
	}
}

void InGameMenu::toggleRecipeList(bool on)
{
	recipeMenu.toggle(on, true);
}

void InGameMenu::toggleFoodMenu(bool f)
{
	if (optionsMenu)
		toggleOptionsMenu(false);
	if (petMenu)
		togglePetMenu(false);
	if (treasureMenu)
		toggleTreasureMenu(false);


	for (size_t i = 0; i < foodHolders.size(); i++)
		foodHolders[i]->toggleValid(f);

	if (f)
	{
		if (game->avatar)
		{
			Path *p=0;
			if (dsq->continuity.getFlag(FLAG_UPGRADE_WOK) > 0
				|| ((p=game->getNearestPath(game->avatar->position, PATH_COOK))
				&& p->isCoordinateInside(game->avatar->position)))
			{
				foodHolders[0]->toggleValid(true);
			}
			else
			{
				foodHolders[0]->toggleValid(false);
			}
		}
	}

	if (f && !foodMenu)
	{
		currentMenuPage = MENUPAGE_FOOD;

		foodMenu = true;

		toggleMainMenu(false);

		refreshFoodSlots(false);

		cook->alpha = 1;
		recipes->alpha = 1;

		prevFood->alpha = 1;
		nextFood->alpha = 1;
		foodLabel->alphaMod = 1;
		foodDescription->alphaMod = 1;

		foodSort->alpha = 1;

		for (size_t i = 0; i < foodTips.size(); i++)
			foodTips[i]->alpha = 1;

		if (foodSlots.size() >= 16)
		{
			foodSlots[2]->setDirMove(DIR_RIGHT, menu[5]);
			foodSlots[3]->setDirMove(DIR_RIGHT, menu[5]);
			menu[5]->setDirMove(DIR_LEFT, foodSlots[2]);

			treasureSlots[3]->setDirMove(DIR_RIGHT, menu[5]);

			recipes->setDirMove(DIR_UP, foodSlots[15]);
			foodSlots[15]->setDirMove(DIR_DOWN, recipes);
			foodSlots[14]->setDirMove(DIR_DOWN, recipes);
			foodSlots[0]->setDirMove(DIR_DOWN, recipes);

			foodSlots[0]->setDirMove(DIR_LEFT, foodSlots[15]);
			foodSlots[15]->setDirMove(DIR_RIGHT, foodSlots[0]);

			foodSlots[15]->setDirMove(DIR_LEFT, foodSlots[14]);

			recipes->setDirMove(DIR_RIGHT, menu[5]);
		}

		menu[6]->setFocus(true);

		doMenuSectionHighlight(1);

		liCrystal->alpha = 1;

		circlePageNum->alpha = 1;

		previewRecipe->alpha = 0;
		updatePreviewRecipe();
	}
	else if (!f && foodMenu)
	{
		recipeMenu.toggle(false);
		foodMenu = false;

		cook->alpha = 0;
		recipes->alpha = 0;
		prevFood->alpha = 0;
		nextFood->alpha = 0;
		foodLabel->alphaMod = 0;
		foodLabel->alpha = 0;
		foodDescription->alpha = 0;
		foodSort->alpha = 0;
		showRecipe->alpha = 0;

		liCrystal->alpha = 0;

		for (size_t i = 0; i < foodTips.size(); i++)
			foodTips[i]->alpha = 0;

		menu[5]->setDirMove(DIR_LEFT, 0);

		circlePageNum->alpha = 0;

		previewRecipe->alpha = 0;
	}

	for (size_t i = 0; i < foodSlots.size(); i++)
	{
		foodSlots[i]->toggle(f);
	}
}

void InGameMenu::doMenuSectionHighlight(int section)
{
	for (int i = 0; i < 4; i++)
		((AquariaMenuItem*)menu[(5+i)])->quad->alphaMod = 0.8f;
	((AquariaMenuItem*)menu[(5+section)])->quad->alphaMod = 1.0f;
	menuIconGlow->position = menu[5+section]->position;
}

void InGameMenu::toggleMainMenu(bool f)
{
	const float t = 0;
	if (f)
	{
		currentMenuPage = MENUPAGE_SONGS;
		for (size_t i = 0; i < songSlots.size(); i++)
		{
			songSlots[i]->alphaMod = 1;
		}
		songBubbles->alpha.interpolateTo(1,t);
		energyIdol->alpha.interpolateTo(1,t);
		liCrystal->alpha.interpolateTo(1, t);
		for (size_t i = 0; i < songTips.size(); i++)
			songTips[i]->alpha = 1;
		menuBg2->alpha.interpolateTo(1, t);


		int sm=-900;
		SongSlot *ss=0;
		for (size_t i = 0; i < songSlots.size(); i++)
		{
			if (dsq->continuity.hasSong(dsq->continuity.getSongTypeBySlot(i)))
			{
				Vector p = songSlots[i]->getWorldPosition();
				if (p.x > sm)
				{
					sm = p.x;
					ss = songSlots[i];
				}
			}
		}

		if (ss)
		{
			ss->setDirMove(DIR_RIGHT, (AquariaMenuItem*)menu[5]);
		}
		((AquariaMenuItem*)menu[5])->setDirMove(DIR_LEFT, ss);

		doMenuSectionHighlight(0);
	}
	else
	{
		((AquariaMenuItem*)menu[5])->setDirMove(DIR_LEFT, 0);

		for (size_t i = 0; i < songSlots.size(); i++)
		{
			songSlots[i]->alphaMod = 0;
		}

		for (size_t i = 0; i < songTips.size(); i++)
			songTips[i]->alpha = 0;

		songBubbles->alpha.interpolateTo(0, t);
		energyIdol->alpha.interpolateTo(0,t);
		liCrystal->alpha.interpolateTo(0, t);
	}
}

void InGameMenu::toggleKeyConfigMenu(bool f)
{
	const float t = 0;
	playingSongInMenu = -1;


	if (f && !keyConfigMenu)
	{
		toggleOptionsMenu(false, false, true);

		for (int i = 0; i <= 1; i++)
			menu[i]->alpha.interpolateTo(0, t);
		for (int i = 4; i <= 8; i++)
			menu[i]->alpha.interpolateTo(0, t);

		toggleMainMenu(false);

		menuBg2->alpha.interpolateTo(0, t);

		keyConfigMenu = true;

		keyConfigBg->setHidden(false);
		keyConfigBg->alpha = 1;

		switchToKeyConfigPage(0);

		dsq->user_bcontrol = dsq->user;

		opt_cancel->alpha = 1;
		opt_save->alpha = 1;


		opt_save->position = opt_save_original + Vector(0, 120);
		opt_cancel->position = opt_cancel_original + Vector(0, 120);

		opt_cancel->setDirMove(DIR_UP, NULL);
		opt_save->setDirMove(DIR_UP, NULL);
		opt_cancel->setFocus(true);

		menuIconGlow->alpha = 0;
	}
	else if (!f)
	{
		keyConfigMenu = false;

		keyConfigBg->setHidden(true);
		keyConfigBg->alpha = 0;

		opt_cancel->alpha = 0;
		opt_save->alpha = 0;

		opt_save->position = opt_save_original;
		opt_cancel->position = opt_cancel_original;

		menuIconGlow->alpha = 1;
	}
}

void InGameMenu::switchToKeyConfigPage(int page)
{
	assert(page < NUM_KEY_CONFIG_PAGES);
	for(unsigned i = 0; i < NUM_KEY_CONFIG_PAGES; ++i)
	{
		group_keyConfig[i]->setHidden(true);
		group_keyConfig[i]->alpha = 0;
		keyCategoryButtons[i]->goUp();
	}
	keyCategoryButtons[page]->goDown();
	group_keyConfig[page]->setHidden(false);
	group_keyConfig[page]->alpha = 1;
}

void InGameMenu::switchToKeyConfigPage1()
{
	switchToKeyConfigPage(0);
}
void InGameMenu::switchToKeyConfigPage2()
{
	switchToKeyConfigPage(1);
}
void InGameMenu::switchToKeyConfigPage3()
{
	switchToKeyConfigPage(2);
}

void InGameMenu::toggleOptionsMenu(bool f, bool skipBackup, bool isKeyConfig)
{
	const float t = 0;
	playingSongInMenu = -1;

	if (f && !optionsMenu)
	{
		if (!isKeyConfig && !optionsOnly)
		{
			lastOptionsMenuPage = currentMenuPage;
		}

		toggleFoodMenu(false);
		optionsMenu = true;
		voxslider->setValue(dsq->user.audio.voxvol);
		musslider->setValue(dsq->user.audio.musvol);
		sfxslider->setValue(dsq->user.audio.sfxvol);

		if (blurEffectsCheck)
			blurEffectsCheck->setValue(dsq->user.video.blur);

		flipInputButtonsCheck->setValue(dsq->user.control.flipInputButtons);
		toolTipsCheck->setValue(dsq->user.control.toolTipsOn);
		autoAimCheck->setValue(dsq->user.control.autoAim);
		targetingCheck->setValue(dsq->user.control.targeting);

		subtitlesCheck->setValue(dsq->user.audio.subtitles);
		fullscreenCheck->setValue(dsq->isFullscreen());

		if (ripplesCheck)
			ripplesCheck->setValue(core->afterEffectManager!=0);

		if (resBox)
		{
			// Note: This adds one past the original list (core->screenModes)
			ScreenMode m = core->isDesktopResolution() ? ScreenMode(0,0,0) : ScreenMode(core->width, core->height, core->getRefreshRate());
			std::string mstr = screenModeStr(m);
			if (!resBox->setSelectedItem(mstr))
			{
				resBox->addItem(mstr);
				resBox->setSelectedItem(mstr);
			}
		}

		switchToActionSet(selectedActionSetIdx);

		opt_cancel->setDirMove(DIR_UP, targetingCheck);
		targetingCheck->setDirMove(DIR_DOWN, opt_cancel);


		opt_save->setDirMove(DIR_UP, voxslider);
		voxslider->setDirMove(DIR_DOWN, opt_save);

		keyConfigButton->setDirMove(DIR_UP, targetingCheck);

		if (!skipBackup)
			dsq->user_backup = dsq->user;


		options->alpha.interpolateTo(1, t);

		for (int i = 0; i <= 1; i++)
			menu[i]->alpha.interpolateTo(0, t);
		for (int i = 4; i <= 9; i++)
			menu[i]->alpha.interpolateTo(0, t);

		toggleMainMenu(false);

		keyConfigButton->alpha = 1;

		menuBg2->alpha.interpolateTo(0, t);

		opt_cancel->alpha = 1;
		opt_save->alpha = 1;
		opt_cancel->setFocus(true);

		lips->alpha = 0;

		liCrystal->alpha = 1;

		optionsMenu = true;

		menuIconGlow->alpha = 0;
	}
	else if (!f && optionsMenu)
	{
		AquariaMenuItem::currentGuiInputLevel = 0;

		lips->alpha = 0;

		keyConfigButton->alpha = 0;

		options->alpha.interpolateTo(0, t);

		opt_cancel->alpha = 0;
		opt_save->alpha = 0;

		liCrystal->alpha = 0;

		optionsMenu = false;

		if (!optionsOnly)
		{
			for (int i = 0; i <= 1; i++)
				menu[i]->alpha.interpolateTo(1, t);
			for (int i = 4; i <= 8; i++)
			{
				menu[i]->alpha.interpolateTo(1, t);
			}

			if (!isKeyConfig)
			{
				menu[9]->alpha.interpolateTo(1, t);

				switch(lastOptionsMenuPage)
				{
				case MENUPAGE_FOOD:
					toggleFoodMenu(true);
					((AquariaMenuItem*)menu[6])->setFocus(true);
					break;
				case MENUPAGE_TREASURES:
					toggleTreasureMenu(true);
					((AquariaMenuItem*)menu[8])->setFocus(true);
					break;
				case MENUPAGE_PETS:
					togglePetMenu(true);
					((AquariaMenuItem*)menu[7])->setFocus(true);
					break;
				case MENUPAGE_SONGS:
				default:
					toggleMainMenu(true);
					((AquariaMenuItem*)menu[5])->setFocus(true);
					break;
				}
			}

			menuBg2->alpha.interpolateTo(1, t);
		}

		menuIconGlow->alpha = 1;
	}
}

void InGameMenu::updateKeyConfigMenu(float dt)
{
	if(!keyConfigMenu)
		return;

	bool isopen = actionSetBox && actionSetBox->isOpen();
	AquariaMenuItem::currentGuiInputLevel = isopen ? 50 : 0;
	float a = isopen ? 0.0f : 1.0f;
	// HACK: debug buttons ignore input at < 1 alpha
	for(size_t i = 0; i < keyCategoryButtons.size(); ++i)
		keyCategoryButtons[i]->alpha = a;

	if(actionSetBox)
	{
		const int curAS = actionSetBox->getSelectedItem();
		if(selectedActionSetIdx != curAS)
			switchToActionSet(curAS);

		dsq->user.control.actionSets[selectedActionSetIdx].enabled
			= actionSetCheck->getValue();
	}

	keyConfigInstructions->alphaMod = AquariaKeyConfig::waitingForInput ? 1.0f : 0.0f;

	updateJoystickText();
}

void InGameMenu::updateOptionsMenu(float dt)
{
	if (!optionsMenu)
		return;

	dsq->user.audio.voxvol				= voxslider->getValue();
	dsq->user.audio.sfxvol				= sfxslider->getValue();
	dsq->user.audio.musvol				= musslider->getValue();

	dsq->user.control.flipInputButtons	= flipInputButtonsCheck->getValue();
	dsq->user.control.toolTipsOn		= toolTipsCheck->getValue();
	dsq->user.control.autoAim			= autoAimCheck->getValue();
	dsq->user.control.targeting			= targetingCheck->getValue();

	dsq->user.audio.subtitles			= subtitlesCheck->getValue();
	dsq->user.video.full				= fullscreenCheck->getValue();

	if (ripplesCheck)
		dsq->user.video.fbuffer			= ripplesCheck->getValue();

	if (blurEffectsCheck)
		dsq->user.video.blur			= blurEffectsCheck->getValue();

	bool apply = false;
	optsfxdly += dt;
	if (sfxslider->hadInput())
	{
		dsq->sound->playSfx("denied");
		apply = true;
	}
	else if (voxslider->hadInput())
	{
		if (!dsq->sound->isPlayingVoice())
			dsq->voice("naija_somethingfamiliar");
		apply = true;
	}
	else if(musslider->hadInput() || musslider->isGrabbed())
	{
		apply = true;
	}
	else if (optsfxdly > 0.6f)
	{
		optsfxdly = 0;
		if (sfxslider->isGrabbed())
		{
			dsq->sound->playSfx("denied");
			dsq->loops.updateVolume();
			apply = true;
		}
		if (voxslider->isGrabbed())
		{
			if (!dsq->sound->isPlayingVoice())
			{
				dsq->voice("naija_somethingfamiliar");
			}
			apply = true;
		}
	}

	if(apply)
		dsq->user.apply();
}

void InGameMenu::update(float dt)
{
	ActionMapper::onUpdate(dt);

	if (menuSelectDelay > 0)
	{
		menuSelectDelay -= dt;
		if (menuSelectDelay <= 0)
		{
			menuSelectDelay = 0;
		}
	}

	if (enqueuedPreviewRecipe)
	{
		updatePreviewRecipe();
		enqueuedPreviewRecipe = 0;
	}

	if (moveFoodSlotToFront)
	{
		moveFoodSlotToFront->moveToFront();
		moveFoodSlotToFront = 0;
	}

	if (cookDelay > 0)
	{
		cookDelay -= dt;
		if (cookDelay < 0)
			cookDelay = 0;
	}

	if (isInGameMenu())
	{
		menuOpenTimer += dt;
		if (dt > 10)
			dt = 10;

		if (foodMenu)
		{
			if (dsq->continuity.hasIngredients())
			{
				int pageNum = (currentFoodPage+1);
				int numPages = ((dsq->continuity.ingredientCount()-1)/foodPageSize)+1;

				std::ostringstream os;
				os << pageNum << "/" << numPages;
				circlePageNum->setText(os.str());

				if (pageNum > numPages && pageNum > 1)
				{
					onPrevFoodPage();
				}
			}
			else
			{
				circlePageNum->setText("1/1");
			}
		}
		if (treasureMenu)
		{
			std::ostringstream os;
			os << (currentTreasurePage+1) << "/" << (numTreasures/treasurePageSize);
			circlePageNum->setText(os.str());
		}
		// HACK: move this later
		updateOptionsMenu(dt);
		if (playingSongInMenu != -1)
		{
			songMenuPlayDelay += dt;

			Song s = dsq->continuity.songBank[playingSongInMenu];

			if (currentSongMenuNote < s.notes.size())
			{
				if (songMenuPlayDelay >= 0.5f)
				{
					songMenuPlayDelay = 0;


					if (currentSongMenuNote >= 0 && currentSongMenuNote < s.notes.size())
					{
						sound->playSfx(game->getNoteName(s.notes[currentSongMenuNote], "Menu"));

						float a = (s.notes[currentSongMenuNote]*2*PI)/8.0f;
						int sz = 110*menuBg->scale.x;
						Vector notePos(sinf(a)*sz,cosf(a)*sz);

						float t = 0.5;
						Quad *q = new Quad("particles/glow", Vector(400+237*menuBg->scale.x,300-52*menuBg->scale.x)+notePos);
						q->setBlendType(BLEND_ADD);
						q->scale = Vector(5,5);
						q->alpha.ensureData();
						q->alpha.data->path.addPathNode(0, 0);
						q->alpha.data->path.addPathNode(0.75, 0.5);
						q->alpha.data->path.addPathNode(0.75, 0.5);
						q->alpha.data->path.addPathNode(0, 1);
						q->alpha.startPath(t);
						q->followCamera = 1;
						q->setLife(t);
						q->setDecayRate(1);

						game->addRenderObject(q, LR_HUD);

						currentSongMenuNote++;
					}
				}
			}
			else
			{
				if (songMenuPlayDelay >= 1.0f)
				{
					playingSongInMenu = -1;
				}
			}
		}

		updateKeyConfigMenu(dt);
	}
}

void InGameMenu::onDebugSave()
{
	themenu->hide();
	game->clearControlHint();
	core->run(0.5);
	game->togglePause(true);
	dsq->doSaveSlotMenu(SSM_SAVE);
	game->togglePause(false);
}

void InGameMenu::switchToActionSet(int idx)
{
	selectedActionSetIdx = idx;
	if(actionSetBox)
	{
		actionSetBox->setSelectedItem(idx);
		actionSetCheck->setValue(dsq->user.control.actionSets[idx].enabled);
	}
	for(size_t i = 0; i < keyConfigs.size(); ++i)
		keyConfigs[i]->setActionSetIndex(idx);
}

void InGameMenu::updateActionSetComboBox()
{
	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
		std::ostringstream os;
		os << '#' << (i+1);
		const std::string& name = dsq->user.control.actionSets[i].name;
		if(name.length())
			os << ": " << name;
		actionSetBox->addItem(os.str());
	}
}

void InGameMenu::nextJoystick()
{
	ActionSet& as = dsq->user.control.actionSets[selectedActionSetIdx];
	int i = as.joystickID;
	Joystick *j = NULL;
	const int N = core->getNumJoysticks();
	do
	{
		++i;
		if(i >= N)
		{
			i = -1;
			break;
		}
		j = core->getJoystick(i);
	}
	while (!j);

	if(j)
		j->rumble(0.3f, 0.3f, 0.2f);

	as.assignJoystickIdx(i, true);
	if(as.joystickID < 0)
		as.joystickName = "NONE";
	updateJoystickText();
}

void InGameMenu::updateJoystickText()
{
	ActionSet& as = dsq->user.control.actionSets[selectedActionSetIdx];
	Joystick *j = core->getJoystick(as.joystickID);
	std::ostringstream jbt;

	if(j)
	{
		joystickNameText->setText(j->getName());
		int numb = j->getNumButtons();
		for(int i = 0; i < numb; ++i)
			if(j->getButton(i))
				jbt << i << "  ";
	}
	else if(as.joystickID == ACTIONSET_REASSIGN_JOYSTICK)
	{
		std::string s = "(";
		s += stringbank.get(2141);
		s += " ";
		s += as.joystickName;
		s += ")";
		joystickNameText->setText(s);
	}
	else
		joystickNameText->setText(stringbank.get(2139));

	joystickButtonsText->setText(jbt.str());

	if(j && as.joystickID >= 0)
		joystickGUIDText->setText(as.joystickGUID);
	else
		joystickGUIDText->setText("");
}
