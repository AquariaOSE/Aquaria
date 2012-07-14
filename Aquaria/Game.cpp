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
#include "../BBGE/Gradient.h"
#include "../BBGE/AfterEffect.h"
#include "../BBGE/MathFunctions.h"
#include "../BBGE/DebugFont.h"
#include "../BBGE/LensFlare.h"
#include "../BBGE/RoundedRect.h"
#include "../BBGE/SimpleIStringStream.h"

#include "Game.h"
#include "GridRender.h"
#include "WaterSurfaceRender.h"
#include "ScriptedEntity.h"
#include "AutoMap.h"
#include "FlockEntity.h"
#include "SchoolFish.h"
#include "Avatar.h"
#include "Shot.h"
#include "Web.h"
#include "StatsAndAchievements.h"

#include "ToolTip.h"

std::vector<std::string> allowedMaps;

const int PATH_ACTIVATION_RANGE		 = 800;

Vector worldLeftCenter(217,250), worldRightCenter(575, 250);
Vector opt_save_original = Vector(350, 350), opt_cancel_original = Vector(450, 350);

const float bgSfxVol	= 1.0;

const float MENUPAGETRANSTIME		= 0.2; // 0.2

const int foodPageSize = 16;
const int treasurePageSize = 16;

int FoodSlot::foodSlotIndex = -1;

int selectedTreasureFlag = -1;

std::vector<FoodHolder*>	foodHolders;
std::vector<PetSlot*>		petSlots;


std::string getSceneFilename(const std::string &scene)
{
	if (dsq->mod.isActive())
		return std::string(dsq->mod.getPath() + "maps/" + scene + ".xml");
	else
		return std::string("data/maps/"+scene+".xml");
	return "";
}

PetSlot::PetSlot(int pet) : AquariaGuiQuad()
{
	PetData *p = dsq->continuity.getPetData(pet);
	if (p)
	{
		std::string fn = "collectibles/egg-" + p->namePart;
		setTexture(fn);
	}
	scale = Vector(0.9, 0.9);
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
		color = Vector(0.5, 0.5, 0.5);

	if (alpha.x < 1) return;

	if ((core->mouse.position - getWorldPosition()).isLength2DIn(32))
	{
		scale.interpolateTo(Vector(1.2, 1.2), 0.1);

		if (core->mouse.buttons.left && !mouseDown)
		{
			mouseDown = true;
		}
		else if (!core->mouse.buttons.left && mouseDown)
		{
			dsq->sound->playSfx("click");

			if (dsq->continuity.getFlag(FLAG_PET_ACTIVE) == petFlag)
			{
				dsq->game->setActivePet(0);
				dsq->sound->playSfx("pet-on");
			}
			else
			{
				dsq->game->setActivePet(FLAG_PET_NAMESTART + petidx);
				dsq->sound->playSfx("pet-off");
			}
			mouseDown = false;
		}
	}
	else
	{
		mouseDown = false;
		scale.interpolateTo(Vector(0.9, 0.9), 0.1);
	}
}

FoodHolder::FoodHolder(int slot, bool trash) : Quad(), slot(slot), trash(trash)
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
	lid->alphaMod = 0.5;
	addChild(lid, PM_POINTER);
}

void FoodHolder::animateLid(bool down, bool longAnim)
{
	float t = 0.2;

	if (!longAnim)
	{
		t = 0.1;
	}

	if (down)
	{
		dsq->sound->playSfx("bubble-lid");
		lid->alpha.interpolateTo(1, t);
		dsq->main(t);
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

		game->enqueuePreviewRecipe();
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
			ing->scale.data->path.addPathNode(Vector(1.25,1.25), 0.2);
			ing->scale.data->path.addPathNode(Vector(1,1),1);
			ing->scale.startPath(0.5);
		}

		game->enqueuePreviewRecipe();
	}
}

void Game::enqueuePreviewRecipe()
{
	enqueuedPreviewRecipe = 1;
}

void Game::updatePreviewRecipe()
{
	const float t = 0.2;

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

void FoodHolder::dropFood()
{
	if (foodHolderIngredient)
	{
		setIngredient(0);
		dsq->game->refreshFoodSlots(true);
	}
}

void FoodHolder::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	if (!dsq->game->recipeMenu.on && foodHolderIngredient)
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
	int offset = game->currentFoodPage*foodPageSize;
	IngredientData *i = dsq->continuity.getIngredientHeldByIndex(offset+slot);
	if (i)
	{
		ingredient = i;

		if (i->amount > 0)
		{
			std::ostringstream os;
			if (i->amount > 1)
				os << i->amount << "/" << MAX_INGREDIENT_AMOUNT;
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

	scale.interpolateTo(Vector(1,1)*scaleFactor,0.001);

	if (ingredient != 0 && (i != lastIngredient || (i && i->amount != lastAmount)))
	{
		if (effects)
		{
			scale.ensureData();
			scale.data->path.clear();
			scale.data->path.addPathNode(Vector(1,1)*scaleFactor,0);
			scale.data->path.addPathNode(Vector(1.5,1.5)*scaleFactor, 0.2);
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
		for (int i = 0; i < foodHolders.size(); i++)
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
			ingredient->amount--;
			dsq->continuity.applyIngredientEffects(ingredient);
			dsq->continuity.removeEmptyIngredients();
			dsq->game->refreshFoodSlots(true);
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
		if (foodHolders[i]->alpha.x > 0 && foodHolders[i]->alphaMod > 0 && foodHolders[i]->isEmpty() && !foodHolders[i]->isTrash())
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
	dsq->game->dropIngrNames.push_back(ingredient->name);
	dsq->continuity.removeEmptyIngredients();
	dsq->game->refreshFoodSlots(true);
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
			//grabTime += dt;
			if (!core->mouse.buttons.left)
			{
				foodSlotIndex = -1;

				//if (ingredient->type < IT_FOOD)

				//if (grabTime > 0.5f)
				if (!dsq->game->recipeMenu.on)
				{
					/*
					dsq->game->removeRenderObject(this);
					dsq->game->addRenderObject(this, LR_MENU);
					*/


					Vector wp = getWorldPosition();
					if ((dsq->game->lips->getWorldPosition() - getWorldPosition()).isLength2DIn(32))
					{
						dsq->menuSelectDelay = 0.5;

						eatMe();
					}
					else if (wp.x < 40 || wp.y < 40 || wp.x > 760 || wp.y > 560)
					{
						discard();
					}
					else
					{
						bool droppedIn = false;
						for (int i = 0; i < foodHolders.size(); i++)
						{
							bool in = (foodHolders[i]->getWorldPosition() - getWorldPosition()).isLength2DIn(32);
							if (in)
							{
								droppedIn = true;

								if (foodHolders[i]->isTrash())
								{
									discard();

									dsq->game->foodLabel->alpha.interpolateTo(0, 2);
									dsq->game->foodDescription->alpha.interpolateTo(0, 2);
									
									break;
									//return;
								}
								else if (foodHolders[i]->isEmpty())
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
								dsq->menuSelectDelay = 0.5;
								doubleClickDelay = 0;
								eatMe();

								//if (!originalPosition.isZero())
								position = originalPosition;

								label->alpha = 1;
								grabTime = 0;

								if (dsq->inputMode == INPUT_JOYSTICK)
								{
									dsq->game->adjustFoodSlotCursor();
								}

								return;
							}
							else
							{
								doubleClickDelay = DOUBLE_CLICK_DELAY;
							}
						}
					}
				}
				/*
				else
				{
					if (ingredient)
						debugLog(splitCamelCase(ingredient->name));
				}
				*/

				//if (!originalPosition.isZero())
				position = originalPosition;

				label->alpha = 1;

				grabTime = 0;
			}
			else
			{
				if (!dsq->game->recipeMenu.on)
				{
					if (dsq->inputMode == INPUT_MOUSE)
					{
						Vector diff = core->mouse.position - getWorldPosition();
						position += diff;
						dsq->game->moveFoodSlotToFront = this;
					}
				}
				//position = parent->getWorldCollidePosition(core->mouse.position);
				//position = core->mouse.position;
			}
		}

		if ((core->mouse.position - getWorldPosition()).isLength2DIn(16))
		//if (isCursorIn())
		{
			dsq->game->foodLabel->setText(ingredient->displayName);
			dsq->game->foodLabel->alpha.interpolateTo(1, 0.2);

			dsq->game->foodDescription->setText(dsq->continuity.getIngredientAffectsString(ingredient));
			dsq->game->foodDescription->alpha.interpolateTo(1, 0.2);

			if (core->mouse.buttons.left && foodSlotIndex == -1)
			{
				grabTime = 0;
				foodSlotIndex = slot;
				label->alpha = 0;

				/*
				dsq->game->removeRenderObject(this);
				dsq->game->addRenderObject(this, LR_HUD);
				*/

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
				if (!game->recipeMenu.on)
					moveRight();
				return;
			}

			/*
			if (core->mouse.buttons.right && !right)
			{
				right = true;
			}
			else if (!core->mouse.buttons.right && right)
			{
				right = false;

				bool dropped = false;
				for (int i = foodHolders.size()-1; i >= 0; i--)
				{
					if (foodHolders[i]->alpha.x > 0 && foodHolders[i]->alphaMod > 0 && foodHolders[i]->isEmpty() && !foodHolders[i]->isTrash())
					{
						foodHolders[i]->setIngredient(ingredient);
						inCookSlot = true;
						refresh();
						dropped = true;
						break;
					}
				}

				if (dropped)
				{
				}
				else
				{
					core->sound->playSfx("denied");
				}
			}
			*/
		}
		else
		{
			if (!dsq->game->foodLabel->alpha.isInterpolating())
				dsq->game->foodLabel->alpha.interpolateTo(0, 2);
			if (!dsq->game->foodDescription->alpha.isInterpolating())
				dsq->game->foodDescription->alpha.interpolateTo(0, 2);
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
	glow->setBlendType(RenderObject::BLEND_ADD);
	glow->alpha = 0;
	addChild(glow, PM_POINTER);

	mbDown = false;

	if (dsq->continuity.isSongTypeForm((SongType)dsq->continuity.getSongTypeBySlot(songSlot)))
		scale = Vector(0.9, 0.9);
	else
		scale = Vector(0.6, 0.6);
}

void SongSlot::onUpdate(float dt)
{
	AquariaGuiQuad::onUpdate(dt);

	if (alpha.x == 1 && alphaMod == 1 && (!parent || parent->alpha.x == 1))
	{
		if ((core->mouse.position - getWorldPosition()).isLength2DIn(24))
		{
			dsq->game->playSongInMenu(songType);
			dsq->game->songLabel->setText(dsq->continuity.getSongNameBySlot(songSlot));
			dsq->game->songLabel->alpha.interpolateTo(1, 0.2);
			const bool anyButton = core->mouse.buttons.left || core->mouse.buttons.right;
			if (!mbDown && anyButton)
			{
				mbDown = true;
			}
			else if (mbDown && !anyButton)
			{
				mbDown = false;

				dsq->game->playSongInMenu(songType, 1);
				if (!dsq->sound->isPlayingVoice())
					dsq->voice(dsq->continuity.getVoxForSongSlot(songSlot));
				//dsq->game->songDescription->setText(dsq->continuity.getDescriptionForSongSlot(songSlot));

			}
			glow->alpha.interpolateTo(0.2, 0.15);
		}
		else
		{
			mbDown = false;
			glow->alpha.interpolateTo(0, 0.2);
			if (!dsq->game->songLabel->alpha.isInterpolating())
			{
				dsq->game->songLabel->alpha.interpolateTo(0, 2);
				/*
				dsq->game->songLabel->alpha.path.addPathNode(dsq->game->songLabel->alpha, 0);
				dsq->game->songLabel->alpha.path.addPathNode(dsq->game->songLabel->alpha, 0.5);
				dsq->game->songLabel->alpha.path.addPathNode(0, 1);
				dsq->game->songLabel->alpha.startPath(3);
				*/
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
			scale.interpolateTo(Vector(1.2, 1.2), 0.1);
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
					
					dsq->runScriptNum("scripts/global/menu-treasures.lua", "useTreasure", flag);
				}
				else
				{
					dsq->sound->playSfx("treasure-select", 0.5);
					dsq->spawnParticleEffect("menu-switch", worldRightCenter, 0, 0, LR_HUD3, 1);

					

					dsq->game->treasureLabel->setText(treasureName);
					dsq->game->treasureLabel->alpha = 1;
					dsq->game->treasureCloseUp->setTexture(dsq->continuity.treasureData[flag].gfx);
					//dsq->game->treasureCloseUp->scale = Vector(dsq->continuity.treasureData[flag].sz, dsq->continuity.treasureData[flag].sz);

					dsq->game->treasureCloseUp->alpha = 1;

					dsq->game->treasureDescription->setText(treasureDesc, Vector(400,450), 400);
					dsq->game->treasureDescription->alpha = 1;

					dsq->game->use->alpha = dsq->continuity.treasureData[flag].use;

					/*
					dsq->game->treasureCloseUp->scale = Vector(0.5,0.5);
					dsq->game->treasureCloseUp->scale.interpolateTo(Vector(1,1), 0.2);
					dsq->game->treasureCloseUp->alpha = 0.1;
					dsq->game->treasureCloseUp->alpha.interpolateTo(1, 0.2);
					*/

					selectedTreasureFlag = flag;

					doubleClickTimer = 0.2;


					std::ostringstream os;
					os << "treasure flag: " << flag << " desc: " << treasureDesc;
					debugLog(os.str());
				}
			}
		}
		else
		{
			mbd = false;
			scale.interpolateTo(Vector(1, 1), 0.1);
		}
	}
	else
	{
		mbd = false;
		scale.interpolateTo(Vector(1, 1), 0.001);
	}
}

void TreasureSlot::refresh()
{
	flag = (game->currentTreasurePage*treasurePageSize) + index + treasureFlagStart;
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

		std::string parse = dsq->continuity.stringBank.get(flag);

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
		//alphaMod = 0;
	}
}

Ingredient *Game::getNearestIngredient(const Vector &pos, int radius)
{
	int closest = -1;
	int r2 = sqr(radius);
	Ingredient *returnIngredient = 0;

	for (Ingredients::iterator i = ingredients.begin(); i != ingredients.end(); i++)
	{
		int len = (pos - (*i)->position).getSquaredLength2D();
		if (len <= r2 && (closest == - 1 || len < closest))
		{
			closest = len;
			returnIngredient = (*i);
		}
	}
	return returnIngredient;
}

Entity *Game::getNearestEntity(const Vector &pos, int radius, Entity *ignore, EntityType et, DamageType dt, int lrStart, int lrEnd)
{
	int sqrRadius = radius*radius;
	Entity *closest = 0;
	int sml=-1;
	int dist = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		dist = (e->position - pos).getSquaredLength2D();
		if (dist <= sqrRadius)
		{
			if (e != ignore && e->isPresent())
			{
				if (lrStart == -1 || lrEnd == -1 || (e->layer >= lrStart && e->layer <= lrEnd))
				{
					if (et == ET_NOTYPE || e->getEntityType() == et)
					{
						if (dt == DT_NONE || e->isDamageTarget(dt))
						{
							if (sml == -1 || dist < sml)
							{
								closest = e;
								sml = dist;
							}
						}
					}
				}
			}
		}
	}
	return closest;
}

/*
class Avatar : public Entity
{
public:
	Vector position, myZoom;
	void clampPosition();
	Entity *convoEntity;
	std::string convoToRun;
	Entity *attachedTo;
	Quad *burstBar;
	int health;
	bool isCharging();
	bool isEntityDead();
	int maxHealth, mana, maxMana;
	bool zoomOverriden;

};
*/

/*
#include "JetStream.h"
#include "Rock.h"
#include "SchoolFish.h"
*/
/*
#include "Game.h"
#include "AfterEffect.h"
#include "Button.h"
#include "TextBox.h"
#include "LightShaft.h"
#include "Item.h"

#include "Elements.h"
#include "WavyWeed.h"
#include "BitmapFont.h"

#include "ParticleEffects.h"



// SPECIAL
*/

Vector menuBgScale;

const int ITEMS_PER_PAGE = 12;

ObsRow::ObsRow(int tx, int ty, int len) : tx(tx), ty(ty), len(len)
{
}

int Game::getNumberOfEntitiesNamed(const std::string &name)
{
	int c = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->life == 1 && (nocasecmp(e->name, name)==0))
			c++;
	}
	return c;
}

void Game::playSongInMenu(int songType, bool override)
{
	if (playingSongInMenu == -1 || override)
	{
		playingSongInMenu = songType;
		currentSongMenuNote = 0;
		songMenuPlayDelay = 0.5;
	}
}

void Game::flipRenderObjectVertical(RenderObject *r, int flipY)
{
	if (r->position.y < flipY)
		r->position.y = flipY + (flipY-r->position.y);
	else
		r->position.y = flipY - (r->position.y-flipY);
}

bool Game::isSceneFlipped()
{
	return sceneFlipped;
}

void Game::flipSceneVertical(int flipY)
{
	sceneFlipped = !sceneFlipped;
	dsq->screenTransition->capture();
	dsq->render();
	dsq->screenTransition->go(1);

	FOR_ENTITIES(itr)
	{
		Entity *e = *itr;
		flipRenderObjectVertical(e, flipY);
	}
	int i = 0;
	int flipTY = (flipY/TILE_SIZE)-1;
	for (i = 0; i < obsRows.size(); i++)
	{
		if (obsRows[i].ty < flipTY)
			obsRows[i].ty = flipTY + (flipTY - obsRows[i].ty);
		else
			obsRows[i].ty = flipTY - (obsRows[i].ty - flipTY);
	}
	for (i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		e->rotation.z = 180-e->rotation.z;
		flipRenderObjectVertical(e, flipY);
	}
	/*
	// DUMBASS: avatar is an entity.. it has already been flipped!
	if (dsq->game->avatar)
	{
		flipRenderObjectVertical(dsq->game->avatar, flipY);
		dsq->game->avatar->clampPosition();
		dsq->game->avatar->update(0.03);
	}
	*/
	reconstructGrid();
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		e->onSceneFlipped();
	}
	reconstructGrid();
	if (cameraFollow)
		warpCameraTo(*cameraFollow);
	dsq->resetTimer();
}

void Game::setMenuDescriptionText(const std::string &text)
{
	menuDescription->setText(text);
}

Game *game = 0;

Ingredient *Game::spawnIngredient(const std::string &ing, const Vector &pos, int times, int out)
{
	std::string use = ing;
	Ingredient *i = 0;
	for (int c = 0; c < times; c++)
	{
		//HACK: 
		if (nocasecmp(ing, "poultice")==0)
			use = "LeafPoultice";

		IngredientData *d = dsq->continuity.getIngredientDataByName(use);
		if (d)
		{
			i = new Ingredient(pos, d);
			ingredients.push_back(i);
			if (out)
			{
				/*
				if (i->velocity.y > i->velocity.x)
				{
					i->velocity.x = i->velocity.y;
					i->velocity.y = -500;
				}
				if (i->velocity.y > 0)
					i->velocity.y *= -1;
				*/

				i->velocity.x = 0;
				i->velocity.y = -500;
			}
			establishEntity(i);
			//addRenderObject(i, LR_ENTITIES);
		}
		else
		{
			debugLog("Could not find ingredient data for [" + use + "]");
		}
	}
	return i;
}

void Game::spawnIngredientFromEntity(Entity *ent, IngredientData *data)
{
	Ingredient *i = new Ingredient(ent->position, data);
	ingredients.push_back(i);
	establishEntity(i);
	//addRenderObject(i, LR_ENTITIES);
}

Game::Game() : StateObject()
{
	applyingState = false;
	blurEffectsCheck = 0;
	ripplesCheck = 0;

	cookDelay = 0;

#if defined(AQUARIA_DEMO)
	allowedMaps.push_back("naijacave");
	allowedMaps.push_back("trainingcave");
	allowedMaps.push_back("mainarea");
	allowedMaps.push_back("vedhacave");
	allowedMaps.push_back("openwater02");
	allowedMaps.push_back("energytemple01");
	allowedMaps.push_back("energytemple02");
	allowedMaps.push_back("energytemple03");
	allowedMaps.push_back("energytemple04");
	allowedMaps.push_back("energytemple05");
	allowedMaps.push_back("energytemple06");
	allowedMaps.push_back("songcave");
	allowedMaps.push_back("songcave02");
	allowedMaps.push_back("title");
	allowedMaps.push_back("energytemplevision");
#endif

	hasPlayedLow = false;

	invincibleOnNested = true;
	activation = false;
	invinciblity = false;

	active = false;

	registerState(this, "Game");

	optionsOnly = false;

	toFlip = -1;
	shuttingDownGameState = false;
	dsq->loops.bg = BBGE_AUDIO_NOCHANNEL;
	dsq->loops.bg2 = BBGE_AUDIO_NOCHANNEL;


	loadingScene = false;
	controlHint_mouseLeft = controlHint_mouseRight = controlHint_mouseMiddle = controlHint_mouseBody = controlHint_bg = 0;
	controlHint_text = 0;

	avatar = 0;
	fromVel = Vector(0,-1);
	currentInventoryPage = 0;
	deathTimer = 0;

	game = this;
	elementWithMenu = 0;
	cameraFollow = 0;

	worldMapRender = 0;

	for (int i = 0; i < PATH_MAX; i++)
		firstPathOfType[i] = 0;

	loadEntityTypeList();


}

Game::~Game()
{
	tileCache.clean();
	game = 0;
}
/*
void Game::doChoiceMenu(Vector position, std::vector<std::string> choices)
{
	dsq->gui.openChoiceMenu(position);
	for (int i = 0; i < choices.size(); i++)
	{
		dsq->gui.choiceMenu.addEntry(choices[i]);
	}
	selectedChoice = "";
	while (selectedChoice.empty())
	{
		dsq->delay(10);
	}
	dsq->gui.closeChoiceMenu();
}
*/

/*
void Game::onAssignMenuScreenItemToSlot0()
{
	if (!dsq->continuity.hudVisible) return;
	if (!selectedMenuScreenItem) return;
	dsq->continuity.setItemSlot(0, selectedMenuScreenItem->getItemIndex());
}

void Game::onAssignMenuScreenItemToSlot1()
{
	if (!dsq->continuity.hudVisible) return;
	if (!selectedMenuScreenItem) return;
	dsq->continuity.setItemSlot(1, selectedMenuScreenItem->getItemIndex());
}

void Game::onAssignMenuScreenItemToSlot2()
{
	if (!dsq->continuity.hudVisible) return;
	if (!selectedMenuScreenItem) return;
	dsq->continuity.setItemSlot(2, selectedMenuScreenItem->getItemIndex());
}

void Game::onAssignMenuScreenItemToSlot3()
{
	if (!dsq->continuity.hudVisible) return;
	if (!selectedMenuScreenItem) return;
	dsq->continuity.setItemSlot(3, selectedMenuScreenItem->getItemIndex());
}
*/

Quad *menu_blackout = 0;

void Game::showInGameMenu(bool ignoreInput, bool optionsOnly, MenuPage menuPage)
{
	if (avatar && core->getNestedMains()==1 && !avatar->isSinging() && (ignoreInput || avatar->isInputEnabled()))
	{
		//dsq->toggleInputGrabPlat(false);
		
		dsq->game->clearControlHint();

		selectedTreasureFlag = -1;
		this->optionsOnly = optionsOnly;

		core->sound->playSfx("Menu-Open");
		dropIngrNames.clear();

		if (avatar->isEntityDead()) return;

		if (dsq->game->autoMap && dsq->game->autoMap->isOn())
			dsq->game->autoMap->toggle(false);

		toggleOptionsMenu(false);
		dsq->overlay->alpha.interpolateTo(0, 0.1);
		float t = 0.3;


		if (!optionsOnly)
		{
			togglePause(true);
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
			addRenderObject(menu_blackout, LR_AFTER_EFFECTS);

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

		int i = 0;


		for (i = 0; i < songSlots.size(); i++)
		{
			if (dsq->continuity.hasSong(dsq->continuity.getSongTypeBySlot(i)))
				songSlots[i]->alpha.interpolateTo(1, t);
			else
				songSlots[i]->alpha = 0;
		}



		/*
		std::ostringstream os;
		os << "Exp: " << dsq->continuity.exp;
		menuEXP->setText(os.str());
		menuEXP->alpha.interpolateTo(1, 0.5);

		std::ostringstream os2;
		os2 << "Money: " << dsq->continuity.money;
		menuMoney->setText(os2.str());
		menuMoney->alpha.interpolateTo(1, 0.5);
		*/

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
		for (int i = 0; i < menu.size(); i++)
			menu[i]->setHidden(false);
		for (int i = 0; i < treasureSlots.size(); i++)
			treasureSlots[i]->setHidden(false);
		treasureDescription->setHidden(false);
		for (int i = 0; i < foodSlots.size(); i++)
			foodSlots[i]->setHidden(false);


		if (dsq->game->miniMapRender)
		{
			dsq->game->miniMapRender->slide(1);
		}

		toggleMainMenu(false);

		dsq->main(t);

		dsq->screenTransition->capture();

		//toggleMiniMapRender(0);

		MenuPage useMenuPage = MENUPAGE_NONE;

		if (!optionsOnly)
		{
			if (menuPage != MENUPAGE_NONE)
			{
				useMenuPage = menuPage;
			}
			else if (dsq->continuity.lastMenuPage != MENUPAGE_NONE)
			{
				//errorLog("setting last menu page");
				useMenuPage = dsq->continuity.lastMenuPage;
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
				float t = 0.1;

				toggleMainMenu(true);

				songBubbles->alpha.interpolateTo(1, t);
				if (menuSongs)
				{
					menuSongs->alpha.interpolateTo(1, t);
				}
				for (i = 0; i < menu.size(); i++)
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
			for (i = 0; i < menu.size(); i++)
			{
				menu[i]->scale.interpolateTo(Vector(1, 1), 0.15);

				menu[i]->alpha.interpolateTo(1, 0.15);
			}
			
			menuIconGlow->alpha.interpolateTo(1, 0.5);
		}

		menuOpenTimer = 0;

		inGameMenu = true;
		


		dsq->routeShoulder = false;


		dsq->screenTransition->transition(MENUPAGETRANSTIME);

		

		if (optionsOnly)
		{
			dsq->main(-1);
		}
	}
}

void Game::pickupIngredientEffects(IngredientData *data)
{
	Quad *q = new Quad("gfx/ingredients/" + data->gfx, Vector(800-20 + core->getVirtualOffX(), (570-2*(100*miniMapRender->scale.y))+ingOffY));
	q->scale = Vector(0.8, 0.8);
	q->followCamera = 1;
	q->alpha.ensureData();
	q->alpha.data->path.addPathNode(0, 0);
	q->alpha.data->path.addPathNode(1.0, 0.1);
	q->alpha.data->path.addPathNode(0, 1.0);
	q->alpha.startPath(2);
	q->setLife(1);
	q->setDecayRate(0.5);
	addRenderObject(q, LR_HELP);
	ingOffY -= 40;
	ingOffYTimer = 2;
}

void Game::hideInGameMenu(bool effects)
{
	if (isCooking) return;
	if (FoodSlot::foodSlotIndex != -1) return;
	if (effects && !this->isInGameMenu()) return;

	if (avatar)
	{
		if (resBox)
			resBox->close();

		//dsq->toggleInputGrabPlat(true);
	
		if (effects)
			core->sound->playSfx("Menu-Close");

		hideInGameMenuExitCheck(false);
		playingSongInMenu = -1;


		float t = 0.3;

		if (!effects)
			t = 0;
		//if (avatar->isEntityDead()) return;

		int i = 0;

		for (i = 0; i < foodHolders.size(); i++)
		{
			foodHolders[i]->dropFood();
		}

		dsq->continuity.lastMenuPage = currentMenuPage;

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

		for (i = 0; i < menu.size(); i++)
		{
			menu[i]->alpha = 0;
			//menu[i]->alpha.interpolateTo(0, t*0.5f);
			//menu[i]->scale.interpolateTo(Vector(0, 0), t);
		}
		for (i = 0; i < spellIcons.size(); i++)
			spellIcons[i]->alpha.interpolateTo(0, t);
		for (i = 0; i < songSlots.size(); i++)
			songSlots[i]->alpha.interpolateTo(0, t);
		songBubbles->alpha.interpolateTo(0, t);

		/*
		menuEXP->alpha.interpolateTo(0, t);
		menuMoney->alpha.interpolateTo(0, t);
		*/
		if (dsq->game->miniMapRender)
			dsq->game->miniMapRender->slide(0);

		menuDescription->alpha.interpolateTo(0, t);
		menuBg->alpha.interpolateTo(0, t);
		menuBg->scale.interpolateTo(menuBg->scale*0.5f, t);
		menuBg2->alpha.interpolateTo(0, t);
		
		

		if (menuSongs)
			menuSongs->alpha.interpolateTo(0, t);

		if (menu_blackout)
		{
			menu_blackout->alpha.interpolateTo(0, t);
		}

		if (showRecipe)
		{
			showRecipe->alpha.interpolateTo(0, t);
		}

		if (effects)
			core->main(t);

		if (menu_blackout)
		{
			menu_blackout->safeKill();
			menu_blackout = 0;
		}
		if (effects)
			togglePause(false);
		inGameMenu = false;
		//toggleMiniMapRender(1);

		for (int i = 0; i < songTips.size(); i++)
			songTips[i]->alpha = 0;
			
		


		for (int i = 0; i < dropIngrNames.size(); i++)
		{
			dsq->game->spawnIngredient(dropIngrNames[i], avatar->position + Vector(0,-96), 1, 1);
		}
		dropIngrNames.clear();
		
		if (effects)
			dsq->quitNestedMain();

		dsq->routeShoulder = true;
	}

	menuBg->setHidden(true);
	opt_save->setHidden(true);
	opt_cancel->setHidden(true);
	options->setHidden(true);
	keyConfigButton->setHidden(true);
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
	for (int i = 0; i < menu.size(); i++)
		menu[i]->setHidden(true);
	for (int i = 0; i < treasureSlots.size(); i++)
		treasureSlots[i]->setHidden(true);
	treasureDescription->setHidden(true);
	for (int i = 0; i < foodSlots.size(); i++)
		foodSlots[i]->setHidden(true);
}

void Game::onLeftMouseButton()
{
	// hud button
	/*
	if (avatar && !avatar->isCharging())
	{

		if ((core->mouse.position - Vector(20, 20)).getSquaredLength2D() < sqr(40))
		{
			if (paused)
				hideInGameMenu();
			else
				showInGameMenu();
		}
	}
	*/
}

/*
void Game::onActivate()
{
	if (!dsq->gui.isInteractionSelectorOpen() && avatar->isInputEnabled())
	{
		Element * e = dsq->getElementAtVector(dsq->cursor->position);
		if (e)
		{
			// open a pop-up menu
			// how?
			// gui system
			// embed menu control in all elements?
				// if in all elements -> stupid, because you can only have on menu at once
			elementWithMenu = e;
			dsq->gui.openInteractionSelectorForElement(e);
		}
	}

	if (dsq->gui.isInteractionSelectorOpen())
	{
		if (dsq->gui.menu.getSelectedEntry() > -1)
		{
			Interaction::Type type = elementWithMenu->interactions[dsq->gui.menu.getSelectedEntry()].getType();
			dsq->gui.closeInteractionSelector();
			// will crash sometime
			elementWithMenu->interact(type, avatar);
			elementWithMenu = 0;


			avatar->enableInput();
			//dsq->gui.menu.clearEntries();
		}
	}
	else if (dsq->gui.isChoiceMenuOpen())
	{
		if (dsq->gui.choiceMenu.getSelectedEntry() > -1)
		{
			selectedChoice = dsq->gui.choiceMenu.getSelectedEntryName();
			dsq->gui.closeChoiceMenu();
			if (core->getNestedMains() > 1)
			{
				core->quitNestedMain();
			}
		}
	}
	else if(core->getNestedMains())
	{
		core->quitNestedMain();
	}
}
*/

void Game::spawnManaBall(Vector pos, float a)
{
	ManaBall *m = new ManaBall(pos, a);
	addRenderObject(m, LR_PARTICLES);
}

void Game::refreshItemSlotIcons()
{
	/*
	for (int i = 0; i < itemSlotIcons.size(); i++)
	{
		if (dsq->continuity.itemSlots[i] != -1)
		{
			itemSlotIcons[i]->setTexture(dsq->getItemTexture(dsq->continuity.itemSlots[i]));
			int n = dsq->continuity.getNumberOf(dsq->continuity.itemSlots[i]);
			if (n > 0)
				itemSlotIcons[i]->alpha = 1;
			else
				itemSlotIcons[i]->alpha = 0;
		}
		else
			itemSlotIcons[i]->alpha = 0;
		itemSlotIcons[i]->setWidthHeight(32, 32);


	}
	*/
	/*
	if (n > 0)
		itemSlotIcons[i]->alpha.interpolateTo(1,0.2);
	else
		itemSlotIcons[i]->alpha.interpolateTo(0,0.2);
	*/
}

void Game::clearPointers()
{
	bg = 0;
	bg2 = 0;
	avatar = 0;
}

void Game::warpPrep()
{
	avatar->onWarp();
	fromVel = avatar->vel;
	fromVel.setLength2D(10);
	fromPosition = avatar->position;
}

void Game::warpToSceneNode(std::string scene, std::string node)
{
	warpPrep();

	sceneToLoad = scene;
	toNode = node;
	stringToLower(sceneToLoad);
	stringToLower(toNode);

	if (avatar->isfh())
		toFlip = 1;


	core->enqueueJumpState("Game");
}

void Game::warpToSceneFromNode( Path *p)
{
	warpPrep();

	sceneToLoad = p->warpMap;

	toNode = "";
	if (!p->warpNode.empty())
	{
		toNode = p->warpNode;
		toFlip = p->toFlip;
		stringToLower(toNode);
	}
	else
	{
		fromScene = sceneName;
		fromWarpType = p->warpType;
	}

	stringToLower(fromScene);
	stringToLower(sceneToLoad);

	core->enqueueJumpState("Game");
}

void Game::transitionToScene(std::string scene)
{
	if (avatar)
	{
		avatar->onWarp();
	}
	sceneToLoad = scene;
	stringToLower(sceneToLoad);
	
	core->enqueueJumpState("Game", false);
}

void Game::transitionToSceneUnder(std::string scene)
{
	if (avatar)
	{
		avatar->onWarp();
	}
	sceneToLoad = scene;
	stringToLower(sceneToLoad);
	core->pushState("Game");
}


ElementTemplate *Game::getElementTemplateByIdx(int idx)
{
	for (int i = 0; i < elementTemplates.size(); i++)
	{
		if (elementTemplates[i].idx == idx)
		{
			return &elementTemplates[i];
		}
	}
	return 0;
}

Element* Game::createElement(int idx, Vector position, int bgLayer, RenderObject *copy, ElementTemplate *et)
{
	if (idx == -1) return 0;

	if (!et)
		et = this->getElementTemplateByIdx(idx);

	Element *element = new Element();
	if (et)
	{
		element->setTexture(et->gfx);
		element->alpha = et->alpha;
	}

	element->position = position;
	element->position.z = -0.05;
	element->templateIdx = idx;

	element->bgLayer = bgLayer;

	if (et)
	{
		if (et->w != -1 && et->h != -1)
			element->setWidthHeight(et->w, et->h);
	}
	if (et)
	{
		if (et->tu1 != 0 || et->tu2 != 0 || et->tv1 != 0 || et->tv2 != 0)
		{
			element->upperLeftTextureCoordinates = Vector(et->tu1, et->tv1);
			element->lowerRightTextureCoordinates = Vector(et->tu2, et->tv2);
		}
	}
	if (copy)
	{
		element->scale = copy->scale;
		if (copy->isfh())
			element->flipHorizontal();
		if (copy->isfv())
			element->flipVertical();
		element->rotation = copy->rotation;
		Quad *q = dynamic_cast<Quad*>(copy);
		if (q)
		{
			element->repeatTextureToFill(q->isRepeatingTextureToFill());
		}
	}
	addRenderObject(element, LR_ELEMENTS1+bgLayer);
	dsq->addElement(element);
	//element->updateCullVariables();

	return element;
}

void Game::addObsRow(int tx, int ty, int len)
{
	ObsRow obsRow(tx, ty, len);
	obsRows.push_back(obsRow);
}

void Game::clearObsRows()
{
	obsRows.clear();
}

void Game::fillGridFromQuad(Quad *q, ObsType obsType, bool trim)
{
#ifdef BBGE_BUILD_OPENGL
	if (q->texture)
	{
		std::vector<TileVector> obs;
		TileVector tpos(q->position);
		int widthscale = q->getWidth()*q->scale.x;
		int heightscale = q->getHeight()*q->scale.y;
		int w2 = widthscale/2;
		int h2 = heightscale/2;
		w2/=TILE_SIZE;
		h2/=TILE_SIZE;
		tpos.x -= w2;
		tpos.y -= h2;

		int w = 0, h = 0;
		unsigned int size = 0;
		unsigned char *data = q->texture->getBufferAndSize(&w, &h, &size);
		if (!data)
		{
			debugLog("Failed to get buffer in Game::fillGridFromQuad()");
			return;
		}

		int szx = TILE_SIZE/q->scale.x;
		int szy = TILE_SIZE/q->scale.y;
		if (szx < 1) szx = 1;
		if (szy < 1) szy = 1;

		for (int tx = 0; tx < widthscale; tx+=TILE_SIZE)
		{
			for (int ty = 0; ty < heightscale; ty+=TILE_SIZE)
			{
				int num = 0;
				for (int x = 0; x < szx; x++)
				{
					for (int y = 0; y < szy; y++)
					{
						// starting position =
						// tx / scale.x
						unsigned int px = int(tx/q->scale.x) + x;
						unsigned int py = int(ty/q->scale.y) + y;
						if (px < unsigned(w) && py < unsigned(h))
						{
							unsigned int p = (py*unsigned(w)*4) + (px*4) + 3; // position of alpha component
							if (p < size && data[p] >= 254)
							{
								num ++;
							}
							else
							{
								break;
							}
						}
					}
				}

				if (num >= int((szx*szy)*0.8f))
				//if (num >= int((szx*szy)))
				{
					// add tile
					//dsq->game->setGrid(TileVector(int(tx/TILE_SIZE)+tpos.x, int(ty/TILE_SIZE)+tpos.y), 1);
					obs.push_back(TileVector(int(tx/TILE_SIZE), int(ty/TILE_SIZE)));
				}
			}
		}

		free(data);

		if (trim)
		{
			std::vector<TileVector> obsCopy;
			obsCopy.swap(obs);
			// obs now empty

			int sides = 0;
			for (int i = 0; i < obsCopy.size(); i++)
			{
				sides = 0;
				for (int j = 0; j < obsCopy.size(); j++)
				{
					if (i != j)
					{
						if (
							(obsCopy[j].x == obsCopy[i].x-1 && obsCopy[j].y == obsCopy[i].y)
						||	(obsCopy[j].x == obsCopy[i].x+1 && obsCopy[j].y == obsCopy[i].y)
						||	(obsCopy[j].y == obsCopy[i].y-1 && obsCopy[j].x == obsCopy[i].x)
						||	(obsCopy[j].y == obsCopy[i].y+1 && obsCopy[j].x == obsCopy[i].x)
						)
						{
							sides++;
						}
						if (sides>=4)
						{
							obs.push_back(obsCopy[i]);
							break;
						}
					}
				}
			}
		}


		glPushMatrix();

		for (int i = 0; i < obs.size(); i++)
		{
			glLoadIdentity();

			glRotatef(q->rotation.z, 0, 0, 1);
			if (q->isfh())
			{
				glRotatef(180, 0, 1, 0);
			}

			//glTranslatef((obs[i].x-w2)*TILE_SIZE+TILE_SIZE/2, (obs[i].y-h2)*TILE_SIZE + TILE_SIZE/2, 0);
			glTranslatef((obs[i].x-w2), (obs[i].y-h2), 0);

			float m[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, m);
			float x = m[12];
			float y = m[13];

			//dsq->game->setGrid(TileVector(tpos.x+(w2*TILE_SIZE)+(x/TILE_SIZE), tpos.y+(h2*TILE_SIZE)+(y/TILE_SIZE)), obsType);
			TileVector tvec(tpos.x+w2+x, tpos.y+h2+y);
			if (dsq->game->getGrid(tvec) == OT_EMPTY)
				dsq->game->setGrid(tvec, obsType);

		}
		glPopMatrix();
	}
#endif
}

std::string Game::getNoteName(int n, const std::string &pre)
{
	std::ostringstream os;
	os << pre << "Note" << n;

	if (n == 6 && bNatural)
	{
		os << "b";
	}
	//debugLog(os.str());
	return os.str();
}

void Game::clearDynamicGrid(unsigned char maskbyte /* = OT_MASK_BLACK */)
{
	// just to be sure in case the grid/type sizes change,
	// otherwise there is a chance to write a few bytes over the end of the buffer -- FG
	compile_assert(sizeof(grid) % sizeof(uint32) == 0);

	signed char *gridstart = &grid[0][0];
	uint32 *gridend = (uint32*)(gridstart + sizeof(grid));
	uint32 *gridptr = (uint32*)gridstart;
	// mask out specific bytes
	// use full uint32 rounds instead of single-bytes to speed things up.
	const uint32 mask = maskbyte | (maskbyte << 8) | (maskbyte << 16) | (maskbyte << 24);
	do
	{
		*gridptr &= mask;
		++gridptr;
	}
	while(gridptr < gridend);
}

void Game::reconstructEntityGrid()
{
	clearDynamicGrid(~OT_INVISIBLEENT);

	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		e->fillGrid();
	}
}

void Game::reconstructGrid(bool force)
{
	if (!force && isSceneEditorActive()) return;

	clearGrid();
	int i = 0;
	for (i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		e->fillGrid();
	}

	ObsRow *o;
	for (i = 0; i < obsRows.size(); i++)
	{
		o = &obsRows[i];
		for (int tx = 0; tx < o->len; tx++)
		{
			setGrid(TileVector(o->tx + tx, o->ty), OT_BLACK);
		}
	}

	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		e->fillGrid();
	}

	trimGrid();

	dsq->pathFinding.generateZones();
}

void Game::trimGrid()
{
	// Prevent the left- and rightmost column of black tiles
	// from beeing drawn. (The maps were designed with this mind...)
	for (int x = 0; x < MAX_GRID; x++)
	{
		const signed char *curCol   = dsq->game->getGridColumn(x);
		const signed char *leftCol  = dsq->game->getGridColumn(x-1);
		const signed char *rightCol = dsq->game->getGridColumn(x+1);
		for (int y = 0; y < MAX_GRID; y++)
		{
			if (curCol[y] & OT_MASK_BLACK)
			{
				if (!(leftCol[y] & OT_MASK_BLACK) || !(rightCol[y] & OT_MASK_BLACK))
					setGrid(TileVector(x, y), OT_BLACKINVIS);
			}
		}
	}
}

float Game::getCoverage(Vector pos, int sampleArea)
{
	TileVector t(pos);
	int total = 0, covered = 0;
	for (int x = t.x-sampleArea; x <= t.x+sampleArea; x++)
	{
		for (int y = t.y-sampleArea; y <= t.y+sampleArea; y++)
		{
			if (x == t.x && y == t.y) continue;
			TileVector ct(x,y);
			Vector vt = ct.worldVector();
			if (isObstructed(ct))
			{
				covered++;
			}
			total++;
		}
	}
	return float(covered)/float(total);
}

float Game::getPercObsInArea(Vector pos, int sampleArea, int obs)
{
	int sz = sampleArea * sampleArea;
	int c = 0;
	TileVector t(pos);

	for (int x = t.x-sampleArea; x <= t.x+sampleArea; x++)
	{
		for (int y = t.y-sampleArea; y <= t.y+sampleArea; y++)
		{
			if (isObstructed(TileVector(x, y), obs))
			{
				c++;
			}
		}
	}
	return float(c)/float(sz);
}

Vector Game::getWallNormal(Vector pos, int sampleArea, float *dist, int obs)
{
	TileVector t(pos);
	//Vector p = t.worldVector();
	Vector avg;
	int c = 0;
	//float maxLen = -1;
	std::vector<Vector> vs;
	if (dist != NULL)
		*dist = -1;
	for (int x = t.x-sampleArea; x <= t.x+sampleArea; x++)
	{
		for (int y = t.y-sampleArea; y <= t.y+sampleArea; y++)
		{
			if (x == t.x && y == t.y) continue;
			TileVector ct(x,y);
			Vector vt = ct.worldVector();
			if (isObstructed(ct, obs))
			{
				/*
				int xDiff = abs(t.x - x);
				int yDiff = abs(t.y - y);
				float xv = float(sampleArea-xDiff)/float(sampleArea);
				if (x < t.x)
					xv = -xv;
				float yv = float(sampleArea-yDiff)/float(sampleArea);
				if (y < t.y)
					yv = -yv;
				Vector v(-xv, -yv);
				*/
				/*
				int xDiff = t.x-x;
				int yDiff = t.y-y;
				*/

				int xDiff = pos.x-vt.x;
				int yDiff = pos.y-vt.y;
				/*
				float xEffect = (sampleArea*TILE_SIZE - abs(xDiff))*1.0f;
				float yEffect = (sampleArea*TILE_SIZE - abs(yDiff))*1.0f;
				*/
				//Vector v(xDiff*xEffect, yDiff*yEffect);
				Vector v(xDiff, yDiff);
				vs.push_back (v);

				if (dist!=NULL)
				{
					float d = (vt-pos).getLength2D();
					if (*dist == -1 || d < *dist)
					{
						*dist = d;
					}
				}
				/*
				float len = v.getLength2D();
				if (len > maxLen || maxLen == -1)
					maxLen = len;
				*/
				//Vector v(xDiff, yDiff);
				//avg += v;
				//c++;
			}
		}
	}
	int sz = (TILE_SIZE*(sampleArea-1));
	for (int i = 0; i < vs.size(); i++)
	{
		float len = vs[i].getLength2D();
		if (len < sz)
		{
			vs[i].setLength2D(sz - len);
			c++;
			avg += vs[i];
		}
	}
	if (avg != 0)
	{
		avg /= c;
		if (avg.x != 0 || avg.y != 0)
		{
			avg.normalize2D();
			avg.z = 0;
		}
	}
	else
	{
		avg.x = avg.y = 0;
	}

	/*
	avg.x = -avg.x;
	avg.y = -avg.y;
	*/
	return avg;
}

Entity *Game::getEntityAtCursor()
{
	int minDist = -1;
	Entity *selected = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		int dist = (e->position - dsq->getGameCursorPosition()).getSquaredLength2D();
		if (dist < sqr(64) && (minDist == -1 || dist < minDist))
		{
			selected = e;
			dist = minDist;
		}
	}
	return selected;
}

// WARNING: will remove from save file, if present!
bool Game::removeEntityAtCursor()
{
	Entity *selected = getEntityAtCursor();
	if (selected)
	{
		return removeEntity(selected);
	}
	return false;
}

bool Game::removeEntity(Entity *selected)
{
	selected->setState(Entity::STATE_DEAD);
	selected->safeKill();
	TiXmlElement *e = this->saveFile->FirstChildElement("Enemy");
	while (e)
	{
		int x = atoi(e->Attribute("x"));
		int y = atoi(e->Attribute("y"));
		if (int(selected->startPos.x) == x && int(selected->startPos.y) == y)
		{
			this->saveFile->RemoveChild(e);
			//delete e;
			return true;
		}

		e = e->NextSiblingElement("Enemy");
	}

	for (int i = 0; i < entitySaveData.size(); i++)
	{
		if (entitySaveData[i].x == int(selected->startPos.x) && entitySaveData[i].y == int(selected->startPos.y))
		{
			std::vector<EntitySaveData> copy = entitySaveData;
			entitySaveData.clear();
			for (int j = 0; j < copy.size(); j++)
			{
				if (j != i)
					entitySaveData.push_back(copy[j]);
			}
			return true;
		}
	}
	return false;
}

void Game::removeIngredient(Ingredient *i)
{
	ingredients.remove(i);
}

void Game::bindIngredients()
{
	for (Ingredients::iterator i = ingredients.begin(); i != ingredients.end(); ++ i)
	{
		Vector v = avatar->position - (*i)->position;
		if (!v.isLength2DIn(16))
		{
			v.setLength2D(500);
			(*i)->vel += v;
		}
	}
}

/*
void Game::getEntityTypeName(int entityType)
{
	switch(entityType)
	{
	case ET_NAUTILUS:		return "Nautilus";
	case ET_CENTIPEDE:		return "Centipede";
	case ET_WATERBUG:		return "WaterBug";
	case ET_CRAWLER:		return "Crawler";
	case ET_HELLBEASTHEAD:	return "HellBeastHead";
	case ET_SPINNER:		return "Spinner";
	case ET_SQUID:			return "Squid";
	case ET_JELLYFISH:		return "JellyFish";
	case ET_ROCKHEAD:		return "RockHead";
	case ET_SPIKE:			return "Spike";
	case ET_MEGASHRIMP:		return "MegaShrimp";
	case ET_ZUNNA:			return "Zunna";
	case ET_PICKUPITEM:		return "PickupItem";
	case ET_HERETICSKULL:	return "HereticSkull";
	case ET_RATTLEOYSTER:	return "RattleOyster";
	case ET_LEACH:			return "Leach";
	case ET_QUEENHYDRA:		return "QueenHydra";
	case ET_HYDRALARVA:		return "Lumite";
	case ET_BARRIER:		return "Barrier";
	}
}
*/

void Game::loadEntityTypeList()
// and group list!
{
	entityTypeList.clear();
	InStream in("scripts/entities/entities.txt");
	std::string line;
	if(!in)
	{
		core->messageBox(dsq->continuity.stringBank.get(2008), dsq->continuity.stringBank.get(2016));
		exit(1);
	}
	while (std::getline(in, line))
	{
		std::string name, prevGfx;
		int idx;
		float scale;
		std::istringstream is(line);
		is >> idx >> name >> prevGfx >> scale;
		//errorLog(line);
		/*
		std::ostringstream os;
		os << "adding entity [" << name << "] idx[" << idx << "] prevGfx [" << prevGfx << "] scale [" << scale << "]";
		debugLog(os.str());
		*/
		entityTypeList.push_back(EntityClass(name, 1, idx, prevGfx, scale));
	}
	in.close();

#ifdef AQUARIA_BUILD_SCENEEDITOR
	entityGroups.clear();

	std::string fn = "scripts/entities/entitygroups.txt";
	if (dsq->mod.isActive())
	{
		fn = dsq->mod.getPath() + "entitygroups.txt";
	}

	InStream in2(fn.c_str());

	int curGroup=0;
	while (std::getline(in2, line))
	{
		if (line.find("GROUP:")!=std::string::npos)
		{
			line = line.substr(6, line.size());
			//debugLog("****** NEWGROUP: " + line);
			EntityGroup newGroup;
			newGroup.name = line;
			//entityGroups[line] = newGroup;
			//
			entityGroups.push_back(newGroup);
			curGroup = entityGroups.size()-1;
        }
		else if (!line.empty())
		{
			EntityGroupEntity ent;
			std::istringstream is(line);
			std::string addLine, graphic;
			is >> ent.name >> ent.gfx;
			stringToLower(ent.name);
			//debugLog("**adding: " + addLine);
			entityGroups[curGroup].entities.push_back(ent);
		}
	}
	in2.close();

	game->sceneEditor.entityPageNum = 0;
	//game->sceneEditor.page = entityGroups.begin();
#endif
}

EntityClass *Game::getEntityClassForEntityType(const std::string &type)
{
	for (int i = 0; i < entityTypeList.size(); i++)
	{
	/*
		std::ostringstream os;
		os << "Comparing entityTypeList [" << entityTypeList[i].name << "] with type [" << type << "]";
		debugLog(os.str());
		*/
		if (nocasecmp(entityTypeList[i].name, type)==0)
			return &entityTypeList[i];
	}
	return 0;
}

int Game::getIdxForEntityType(std::string type)
{
	//if (!type.empty() && type[0] == '@')	return -1;

	for (int i = 0; i < entityTypeList.size(); i++)
	{
		if (nocasecmp(entityTypeList[i].name, type)==0)
			return entityTypeList[i].idx;
	}
	return -1;
}

Entity *Game::createEntity(int idx, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType et, Entity::NodeGroups *nodeGroups, int gid, bool doPostInit)
{
	std::string type;
	for (int i = 0; i < dsq->game->entityTypeList.size(); i++)
	{
		EntityClass *ec = &dsq->game->entityTypeList[i];
		if (ec->idx == idx)
		{
			type = ec->name;
			return createEntity(type, id, position, rot, createSaveData, name, et, nodeGroups, gid, doPostInit);
		}
	}
	return 0;
}

// ensure a limit of entity types in the current level
// older entities with be culled if state is set to 0
// otherwise, the older entities will have the state set
void Game::ensureLimit(Entity *e, int num, int state)
{
	int idx = e->entityTypeIdx;
	int c = 0;
	std::list<Entity*> entityList;
	FOR_ENTITIES(i)
	{
		if ((*i)->entityTypeIdx == idx && (state == 0 || (*i)->getState() != state))
		{
			entityList.push_back(*i);
			c++;
		}
	}

	int numDelete = c-(num+1);
	if (numDelete >= 0)
	{
		for (std::list<Entity*>::iterator i = entityList.begin(); i != entityList.end(); i++)
		{
			if (state == 0)
				(*i)->safeKill();
			else
				(*i)->setState(state);
			numDelete--;
			if (numDelete <= 0)
				break;
		}
	}
}

Entity* Game::establishEntity(Entity *e, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType et, Entity::NodeGroups *nodeGroups, int gid, bool doPostInit)
{
	// e->layer must be set BEFORE calling this function!

	std::string type = e->name;
	stringToLower(type);



	// i'm thinking this *should* hold up for new files
	// it will mess up if you're importing an old file
	// the logic being that you're not going to load in an non-ID-specified entity in new files
	// so assignUniqueID should never be called
	// so what i'm going to do is have it bitch

	// note that when not loading a scene, it is valid to call assignUniqueID here
	if (id != 0)
	{
		e->setID(id);
	}
	else
	{
		if (loadingScene)
		{
			std::ostringstream os;
			os << "ERROR: Assigning Unique ID to a loaded Entity... if this is called from loadScene then Entity IDs may be invalid";
			os << "\nEntityName: " << e->name;
			errorLog(os.str());
		}
		else
		{
			e->assignUniqueID();
		}
	}

	// get node groups before calling init
	if (nodeGroups)
	{
		e->nodeGroups = (*nodeGroups);
	}

	// NOTE: init cannot be called after "addRenderObject" for some unknown reason
	e->init();

	Vector usePos = position;
	e->startPos = usePos;
	if (!name.empty())
		e->name = name;

	e->setGroupID(gid);

	e->rotation.z = rot;

	int idx = getIdxForEntityType(type);
	e->entityTypeIdx = idx;


	if (createSaveData)
	{
		int idx = dsq->game->getIdxForEntityType(type);
		entitySaveData.push_back(EntitySaveData(e, idx, usePos.x, usePos.y, rot, e->getGroupID(), e->getID(), e->name));
	}

	addRenderObject(e, e->layer);

	if (doPostInit)
	{
		e->postInit();
	}

	return e;
}

Entity *Game::createEntity(const std::string &t, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType et, Entity::NodeGroups *nodeGroups, int gid, bool doPostInit)
{
	std::string type = t;
	stringToLower(type);

	ScriptedEntity *e;

	e = new ScriptedEntity(type, position, et);


	return establishEntity(e, id, position, rot, createSaveData, name, et, nodeGroups, gid, doPostInit);
}

void Game::initEntities()
{
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e)
		{
			e->init();
		}
	}
}

void Game::assignEntitiesUniqueIDs()
{
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e && e->entityID == 0)
		{
			e->assignUniqueID();
		}
	}

}

EntitySaveData *Game::getEntitySaveDataForEntity(Entity *e, Vector pos)
{

	for (int i = 0; i < entitySaveData.size(); i++)
	{
		if (entitySaveData[i].e == e)
		{
			return &entitySaveData[i];
		}
		/*
		if (entitySaveData[i].x == int(e->startPos.x) && entitySaveData[i].y == int(e->startPos.y))
		{
			if (entitySaveData[i].idx == e->entityTypeIdx)
			{
				debugLog("found entity");
				return &entitySaveData[i];
			}
		}
		*/
	}
	return 0;
}

void Game::spawnSporeChildren()
{
	creatingSporeChildren = true;
	SporeChildData *scd;
	int sz = dsq->continuity.sporeChildData.size();
	for (int i=0; i < sz; i++)
	{
		scd = &dsq->continuity.sporeChildData[i];
		scd->entity = 0;
	}
	int c = 0;
	for (int i=0; i < sz; i++)
	{
		scd = &dsq->continuity.sporeChildData[i];
		Entity *e = dsq->game->createEntity("SporeChild", 0, avatar->position+Vector(0,2+c*2), 0, 0, "");
		if (e)
		{
			e->setState(scd->state);
			if (scd->health < 1)
				scd->health = 1;
			e->health = scd->health;
			scd->entity = e;
		}
		c++;
	}
	creatingSporeChildren = false;
}

void Game::setTimerTextAlpha(float a, float t)
{
	timerText->alpha.interpolateTo(a, t);
}

void Game::setTimerText(float time)
{
	std::ostringstream os;
	int mins = int(time/60);
	int secs = time - (mins*60);
	os << mins;
	if (getTimer() > 0.5f)
		os << ":";
	else
		os << ".";
	if (secs < 10)
		os << "0";
	os << secs;
	timerText->setText(os.str());
}

void Game::generateCollisionMask(Quad *q, int overrideCollideRadius)
{
#ifdef BBGE_BUILD_OPENGL
	if (q->texture)
	{
		q->collidePosition = Vector(0,0,0);
		if (overrideCollideRadius)
			q->collideRadius = overrideCollideRadius;
		else
			q->collideRadius = TILE_SIZE/2;
		q->collisionMask.clear();
		TileVector tpos(q->position);
		int widthscale = q->getWidth()*q->scale.x;
		int heightscale = q->getHeight()*q->scale.y;
		int w2 = widthscale/2;
		int h2 = heightscale/2;
		w2/=TILE_SIZE;
		h2/=TILE_SIZE;
		tpos.x -= w2;
		tpos.y -= h2;

		int w = 0, h = 0;
		unsigned int size = 0;
		unsigned char *data = q->texture->getBufferAndSize(&w, &h, &size);
		if (!data)
		{
			debugLog("Failed to get buffer in Game::generateCollisionMask()");
			return;
		}

		q->collisionMaskRadius = 0;

		Vector collisionMaskHalfVector = Vector(q->getWidth()/2, q->getHeight()/2);

		int szx = TILE_SIZE/q->scale.x;
		int szy = TILE_SIZE/q->scale.y;
		if (szx < 1) szx = 1;
		if (szy < 1) szy = 1;

		for (int tx = 0; tx < widthscale; tx+=TILE_SIZE)
		{
			for (int ty = 0; ty < heightscale; ty+=TILE_SIZE)
			{
				int num = 0;

				for (int x = 0; x < szx; x++)
				{
					for (int y = 0; y < szy; y++)
					{
						// starting position =
						// tx / scale.x
						unsigned int px = int(tx/q->scale.x) + x;
						unsigned int py = int(ty/q->scale.y) + y;
						if (px < unsigned(w) && py < unsigned(h))
						{
							unsigned int p = (py*unsigned(w)*4) + (px*4) + 3; // position of alpha component
							if (p < size && data[p] >= 250)
							{
								num ++;
							}
						}
					}
				}
				if (num >= int((szx*szy)*0.25f))
				{
					TileVector tile(int((tx+TILE_SIZE/2)/TILE_SIZE), int((ty+TILE_SIZE/2)/TILE_SIZE));
					// + Vector(0,TILE_SIZE)
					q->collisionMask.push_back(tile.worldVector() - collisionMaskHalfVector);
				}
			}
		}

		q->collisionMaskRadius = 512;

		free(data);


		/*
		for (int i = 0; i < q->collisionMask.size(); i++)
		{
			float xsz = q->collisionMask[i].x;
			float ysz = q->collisionMask[i].y;
			if (xsz < 0)	xsz = -xsz;
			if (ysz < 0)	ysz = -ysz;
			if (xsz > q->collisionMaskRadius)
				q->collisionMaskRadius = xsz;
			if (ysz > q->collisionMaskRadius)
				q->collisionMaskRadius = ysz;
		}
		q->collisionMaskRadius += TILE_SIZE/2;
		*/
		/*
		if (w2 > h2)
			q->collisionMaskRadius = w2*2;
		else
			q->collisionMaskRadius = h2*2;
		*/
		//q->collisionMaskRadius = sqrtf(sqr(w2)+sqr(h2));

		/*
		int rot = rotation.z;
		while (rot > 360)
			rot -= 360;
		while (rot < 0)
			rot += 360;
		*/
	}
#endif
}

void Game::addPath(Path *p)
{
	paths.push_back(p);
	if (p->pathType >= 0 && p->pathType < PATH_MAX)
	{
		p->nextOfType = firstPathOfType[p->pathType];
		firstPathOfType[p->pathType] = p;
	}
}

void Game::removePath(int idx)
{
	if (idx >= 0 && idx < paths.size()) paths[idx]->destroy();
	std::vector<Path*> copy = this->paths;
	clearPaths();
	for (int i = 0; i < copy.size(); i++)
	{
		if (i != idx)
			addPath(copy[i]);
	}
}

void Game::clearPaths()
{
	paths.clear();
	for (int i = 0; i < PATH_MAX; i++)
		firstPathOfType[i] = 0;
}

int Game::getIndexOfPath(Path *p)
{
	for (int i = 0; i < paths.size(); i++)
	{
		if (paths[i] == p)
			return i;
	}
	return -1;
}

Path *Game::getPathAtCursor()
{
	return getNearestPath(dsq->getGameCursorPosition(), "");
	/*
	int range = 128;
	int sz = paths.size();
	for (int i = 0; i < sz; i++)
	{
		Path *p = &(paths[i]);
		if (!p->nodes.empty())
		{
			int dist = (p->nodes[0].position - dsq->getGameCursorPosition()).getSquaredLength2D();
			if (dist < sqr(range))
			{
				return p;
			}
		}
	}
	return 0;
	*/
}

Path *Game::getScriptedPathAtCursor(bool withAct)
{
	//int range = 64;
	int sz = paths.size();
	for (int i = 0; i < sz; i++)
	{
		Path *p = (paths[i]);
		if (!p->nodes.empty() && p->hasScript())
		{
			if (!withAct || p->cursorActivation)
			{
				if (p->isCoordinateInside(dsq->getGameCursorPosition()))
				{
					return p;
				}
			}
			/*
			int dist = (p->nodes[0].position - dsq->getGameCursorPosition()).getSquaredLength2D();
			if (dist < sqr(range))
			{
				return p;
			}
			*/
		}
	}
	return 0;
}

Path *Game::getNearestPath(const Vector &pos, const std::string &s, const Path *ignore)
{
	Path *closest = 0;
	float smallestDist = HUGE_VALF;
	std::string st = s;
	stringToLower(st);
	for (int i = 0; i < dsq->game->paths.size(); i++)
	{
		Path *cp = dsq->game->paths[i];
		if (cp != ignore && !cp->nodes.empty() && (st.empty() || st == cp->label))
		{
			const Vector v = cp->nodes[0].position - pos;
			const float dist = v.getSquaredLength2D();
			if (dist < smallestDist)
			{
				smallestDist = dist;
				closest = cp;
			}
		}
	}
	return closest;
}

Path *Game::getNearestPath(const Vector &pos, PathType pathType)
{
	Path *closest = 0;
	float smallestDist = HUGE_VALF;
	for (Path *cp = dsq->game->getFirstPathOfType(pathType); cp; cp = cp->nextOfType)
	{
		if (!cp->nodes.empty())
		{
			const Vector v = cp->nodes[0].position - pos;
			const float dist = v.getSquaredLength2D();
			if (dist < smallestDist)
			{
				smallestDist = dist;
				closest = cp;
			}
		}
	}
	return closest;
}

Path *Game::getNearestPath(Path *p, std::string s)
{
	if (p->nodes.empty()) return 0;

	return getNearestPath(p->nodes[0].position, s);
}

Path *Game::getPathByName(std::string name)
{
	stringToLowerUserData(name);
	for (int i = 0; i < paths.size(); i++)
	{
		if (paths[i]->label == name)
			return paths[i];
	}
	return 0;
}


void Game::addKeyConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int y, int l1, int l2, int l3)
{
	//DebugFont *lb = new DebugFont(6, label);
	TTFText *lb = new TTFText(&dsq->fontArialSmallest);
	lb->setText(label);
	lb->position = Vector(140,y);
	group->addChild(lb, PM_POINTER);

	AquariaKeyConfig *k1 = new AquariaKeyConfig(actionInputName, INPUTSET_KEY, 0);
	k1->position = Vector(350,y);
	k1->setLock(l1);
	group->addChild(k1, PM_POINTER);

	AquariaKeyConfig *k2 = new AquariaKeyConfig(actionInputName, INPUTSET_KEY, 1);
	k2->position = Vector(475,y);
	k2->setLock(l2);
	group->addChild(k2, PM_POINTER);

	AquariaKeyConfig *j1 = new AquariaKeyConfig(actionInputName, INPUTSET_JOY, 0);
	j1->position = Vector(600,y);
	j1->setLock(l3);
	group->addChild(j1, PM_POINTER);

	k1->setDirMove(DIR_RIGHT, k2);
	k2->setDirMove(DIR_RIGHT, j1);

	j1->setDirMove(DIR_LEFT, k2);
	k2->setDirMove(DIR_LEFT, k1);
}

AquariaKeyConfig *Game::addAxesConfigLine(RenderObject *group, const std::string &label, const std::string &actionInputName, int y, int offx)
{
	/// DebugFont *lb = new DebugFont(6, label);
	TTFText *lb = new TTFText(&dsq->fontArialSmallest);
	lb->setText(label);
	lb->position = Vector(140+offx, y);
	group->addChild(lb, PM_POINTER);

	AquariaKeyConfig *i1 = new AquariaKeyConfig(actionInputName, INPUTSET_OTHER, 0);
	i1->position = Vector(140+80+offx,y);
	//i1->setLock(l1);
	group->addChild(i1, PM_POINTER);

	i1->setDirMove(DIR_RIGHT, 0);
	i1->setDirMove(DIR_LEFT, 0);

	return i1;
}

bool Game::doFlagCheck(const std::string &flagCheck, FlagCheckType type, bool lastTruth)
{
	if (!flagCheck.empty())
	{
		std::string flagName, comparison, next;
		int value=0;
		std::istringstream is(flagCheck);
		is >> flagName >> comparison >> value >> next;

		bool truth=false;
		if (comparison == "==")
		{
			if (dsq->continuity.getFlag(flagName) == value) truth = true;
		}
		else if (comparison == "<")
		{
			if (dsq->continuity.getFlag(flagName) < value)	truth = true;
		}
		else if (comparison == ">")
		{
			if (dsq->continuity.getFlag(flagName) > value)	truth = true;
		}
		else if (comparison == "<=")
		{
			if (dsq->continuity.getFlag(flagName) <= value)	truth = true;
		}
		else if (comparison == ">=")
		{
			if (dsq->continuity.getFlag(flagName) >= value)	truth = true;
		}

		if (type == AND)
		{
			truth = (lastTruth && truth);
		}
		else if (type == OR)
		{
			truth = (lastTruth || truth);
		}

		if (next == "AND")
		{
			std::string restOfIt;
			std::getline(is, restOfIt);
			return doFlagCheck(restOfIt, AND, truth);
		}
		else if (next == "OR")
		{
			std::string restOfIt;
			std::getline(is, restOfIt);
			return doFlagCheck(restOfIt, OR, truth);
		}

		return truth;
	}
	return true;
}

void Game::switchToSongMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	toggleFoodMenu(false);
	togglePetMenu(false);
	toggleTreasureMenu(false);

	toggleMainMenu(true);

	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void Game::switchToFoodMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	togglePetMenu(false);
	toggleMainMenu(false);
	toggleTreasureMenu(false);

	toggleFoodMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void Game::switchToPetMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	toggleFoodMenu(false);
	toggleMainMenu(false);
	toggleTreasureMenu(false);

	togglePetMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void Game::switchToTreasureMenu()
{
	dsq->screenTransition->capture();

	toggleOptionsMenu(false);
	toggleFoodMenu(false);
	toggleMainMenu(false);
	togglePetMenu(false);

	toggleTreasureMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

/*
IET_INVINCIBLE
IET_HP
IET_MAXHP
IET_DEFENSE
IET_SPEED
IET_REGEN
IET_ENERGY
IET_BLIND
IET_LIGHT
IET_PETPOWER
IET_WEB
IET_LI
IET_FISHPOISON
IET_BITE
IET_EAT
IET_YUM
IET_TRIP
IET_RANDOM
IET_POISON
IET_ALLSTATUS
*/

typedef std::vector<IngredientData> IngVec;

void Game::sortFood()
{
	/*
	if (dsq->continuity.foodSortType == FOODSORT_UNSORTED)
	{
		if (dsq->continuity.sortByUnsort.empty())
			for (int i = 0; i < dsq->continuity.ingredients.size(); i++)
				dsq->continuity.sortByUnsort.push_back(FoodSortOrder(IT_NONE, IET_NONE, dsq->continuity.ingredients[i].name));
	}
	*/
	
	std::vector<std::string> foodHolderNames;
	foodHolderNames.resize(foodHolders.size());
	
	for (int i = 0; i < foodHolders.size(); i++) {
		IngredientData *ing = foodHolders[i]->getIngredient();
		if (ing) {
			foodHolderNames[i] = ing->name;
			//errorLog(foodHolderNames[i]);
			//foodHolders[i]->setIngredient(0);
		}
	}

	dsq->continuity.foodSortType++;
	if (dsq->continuity.foodSortType >= MAX_FOODSORT)
		dsq->continuity.foodSortType = 0;
	
	dsq->continuity.sortFood();
	
	// rebuild the page
	
	refreshFoodSlots(false);
	
	/*
	toggleFoodMenu(false);
	toggleFoodMenu(true);
	*/
	
	dsq->sound->playSfx("shuffle");
	dsq->sound->playSfx("menu-switch", 0.5);
	dsq->spawnParticleEffect("menu-switch", worldLeftCenter, 0, 0, LR_HUD3, 1);
	
	for (int i = 0; i < foodHolders.size(); i++) {
		if (!foodHolderNames[i].empty()) {
			IngredientData *ing = dsq->continuity.getIngredientHeldByName(foodHolderNames[i]);
			foodHolders[i]->setIngredient(ing, false);

			//foodHolders[i]->setIngredient(dsq->continuity.getIngredientByName(foodHolderNames[i]));
			/*
			if (!foodHolders[i]->foodHolderIngredient) {
				errorLog("not found");
			}
			else {
				std::ostringstream os;
				os << "get: " << foodHolders[i]->foodHolderIngredient->name;
				errorLog(os.str());
			}
			*/
		}
	}
}

void Game::createInGameMenu()
{
	float menuz = 4;
	int i = 0;


	menuBg = new Quad;
	menuBg->setTexture("menu");
	//menuBg->setWidthHeight(800);
	//menuBg->scale = Vector(800.0f/1024.0f, 800.0f/1024.0f);
	menuBgScale = Vector(800.0f/1024.0f, 800.0f/1024.0f);
	menuBg->position = Vector(400,300,menuz);
	menuBg->followCamera = 1;
	//menuBg->shareAlphaWithChildren=true;
	addRenderObject(menuBg, LR_MENU);


	menuBg2 = new Quad;
	menuBg2->setTexture("menu2");
	menuBg2->position = Vector(0, 240);
	menuBg->addChild(menuBg2, PM_POINTER);

	float scale = menuBg->scale.x;
	/*
	songDescription = new BitmapText(&dsq->font);
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

	resBox = new AquariaComboBox();
	resBox->position = Vector(196, 285);
	for (i = 0; i < core->screenModes.size(); i++)
	{
		ostringstream os;
		os << core->screenModes[i].x << "x" << core->screenModes[i].y;
		resBox->addItem(os.str());
		if (core->screenModes[i].x == dsq->user.video.resx && core->screenModes[i].y == dsq->user.video.resy)
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

	micInputCheck = 0;

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
	opt_save->event.set(MakeFunctionEvent(Game, onOptionsSave));
	opt_save->position = opt_save_original;
	opt_save->alpha = 0;
	addRenderObject(opt_save, LR_MENU);

	opt_cancel = new AquariaMenuItem;
	opt_cancel->useQuad("gui/Cancel");
	opt_cancel->useGlow("particles/glow", 100, 50);
	opt_cancel->event.set(MakeFunctionEvent(Game, onOptionsCancel));
	opt_cancel->position = opt_cancel_original;
	opt_cancel->alpha = 0;
	addRenderObject(opt_cancel, LR_MENU);

	options->shareAlphaWithChildren = 1;
	options->alpha = 0;
	options->followCamera = 1;
	addRenderObject(options, LR_MENU);

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
	

	songLabel = new BitmapText(&dsq->smallFont);
	{
		songLabel->alpha = 0;
		songLabel->setAlign(ALIGN_CENTER);
		songLabel->followCamera = 1;
		songLabel->setFontSize(20);
		songLabel->position = Vector(-center.x, center.y) + Vector(0, -15); //+ Vector(10, -10);
		songLabel->scale = Vector(1.2, 1.2);
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
	tip->setText(dsq->continuity.stringBank.get(0), Vector(200,450), 350);
	addRenderObject(tip, LR_HUD);
	foodTips.push_back(tip);


	tip = new ToolTip;
	tip->alpha = 0;
	tip->setCircularAreaFromCenter(worldRightCenter, 240);
	tip->setText(dsq->continuity.stringBank.get(1), Vector(600,450), 350);
	addRenderObject(tip, LR_HUD);
	foodTips.push_back(tip);



	tip = new ToolTip;
	tip->alpha = 0;
	tip->setCircularAreaFromCenter(worldLeftCenter, 240);
	tip->setText(dsq->continuity.stringBank.get(14), Vector(200,450), 350);
	addRenderObject(tip, LR_HUD);
	songTips.push_back(tip);


	/*
	tip = new ToolTip;
	tip->alpha = 0;
	tip->setAreaFromCenter(Vector(400,300), 800, 600);
	tip->setText(dsq->continuity.stringBank.get(16), Vector(400,300), 400);
	addRenderObject(tip, LR_HUD);
	petTips.push_back(tip);
	*/

	tip = new ToolTip;
	tip->alpha = 0;
	tip->setCircularAreaFromCenter(worldLeftCenter, 240);
	tip->setText(dsq->continuity.stringBank.get(17), Vector(200,450), 350);
	addRenderObject(tip, LR_HUD);
	petTips.push_back(tip);

	tip = new ToolTip;
	tip->alpha = 0;
	tip->setAreaFromCenter(Vector(400,350), 150, 50);
	tip->setText(dsq->continuity.stringBank.get(15), Vector(400,450), 450);
	addRenderObject(tip, LR_HUD);
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
	keyConfigButton->scale = Vector(0.8, 0.8);
	keyConfigButton->event.set(MakeFunctionEvent(Game, onKeyConfig));
	//keyConfigButton->setCanDirMove(false);
	addRenderObject(keyConfigButton, LR_MENU);



	group_keyConfig = new RenderObject;

	/*
	Quad *kbg = new Quad("gui/keyconfig-menu", Vector(400,300));
	kbg->setWidthHeight(800, 800);
	group_keyConfig->addChild(kbg);
	*/

	//Quad *kcb = new Quad;
	RoundedRect *kcb = new RoundedRect();
	//kcb->color = 0;
	//kcb->alphaMod = 0.75;
	kcb->position = Vector(400,276 - 10);
	kcb->setWidthHeight(580, 455, 10);
	group_keyConfig->addChild(kcb, PM_POINTER);

	int offy = -20;

#define SB(x) dsq->continuity.stringBank.get(x)
	
	TTFText *header_action = new TTFText(&dsq->fontArialSmall);
	header_action->setText(SB(2101));
	header_action->position = Vector(140, 80+offy);
	group_keyConfig->addChild(header_action, PM_POINTER);
	
	TTFText *header_key1 = new TTFText(&dsq->fontArialSmall);
	header_key1->setText(SB(2102));
	header_key1->position = Vector(350, 80+offy);
	header_key1->setAlign(ALIGN_CENTER);
	group_keyConfig->addChild(header_key1, PM_POINTER);
	
	TTFText *header_key2 = new TTFText(&dsq->fontArialSmall);
	header_key2->setText(SB(2103));
	header_key2->position = Vector(475, 80+offy);
	header_key2->setAlign(ALIGN_CENTER);
	group_keyConfig->addChild(header_key2, PM_POINTER);
	
	TTFText *header_joy = new TTFText(&dsq->fontArialSmall);
	header_joy->setText(SB(2104));
	header_joy->position = Vector(600, 80+offy);
	header_joy->setAlign(ALIGN_CENTER);
	group_keyConfig->addChild(header_joy, PM_POINTER);

	addKeyConfigLine(group_keyConfig, SB(2105), "lmb",					100+offy, 0, 0, 0);
	addKeyConfigLine(group_keyConfig, SB(2106), "rmb",					120+offy, 0, 0, 0);
	addKeyConfigLine(group_keyConfig, SB(2107), "PrimaryAction",		140+offy);
	addKeyConfigLine(group_keyConfig, SB(2108), "SecondaryAction",		160+offy);
	addKeyConfigLine(group_keyConfig, SB(2109), "SwimUp",				180+offy);
	addKeyConfigLine(group_keyConfig, SB(2110), "SwimDown",				200+offy);
	addKeyConfigLine(group_keyConfig, SB(2111), "SwimLeft",				220+offy);
	addKeyConfigLine(group_keyConfig, SB(2112), "SwimRight",			240+offy);
	addKeyConfigLine(group_keyConfig, SB(2113), "Roll",					260+offy);
	addKeyConfigLine(group_keyConfig, SB(2114), "Revert",				280+offy);
	addKeyConfigLine(group_keyConfig, SB(2115), "WorldMap",				300+offy);
	addKeyConfigLine(group_keyConfig, SB(2116), "Escape",				320+offy, 1, 0, 0);

	AquariaKeyConfig* s1x = addAxesConfigLine(group_keyConfig, SB(2117), "s1ax", 340+offy, 0);
	AquariaKeyConfig* s1y = addAxesConfigLine(group_keyConfig, SB(2118), "s1ay", 340+offy, 130);
	AquariaKeyConfig* s2x = addAxesConfigLine(group_keyConfig, SB(2119), "s2ax", 340+offy, 260);
	AquariaKeyConfig* s2y = addAxesConfigLine(group_keyConfig, SB(2120), "s2ay", 340+offy, 380);
	
	s1x->setDirMove(DIR_LEFT, s1x);
	s1x->setDirMove(DIR_RIGHT, s1y);

	s1y->setDirMove(DIR_LEFT, s1x);
	s1y->setDirMove(DIR_RIGHT, s2x);

	s2x->setDirMove(DIR_LEFT, s1y);
	s2x->setDirMove(DIR_RIGHT, s2y);

	s2y->setDirMove(DIR_LEFT, s2x);
	s2y->setDirMove(DIR_RIGHT, s2y);

	offy += 20;
	
	addKeyConfigLine(group_keyConfig, SB(2121), "PrevPage",		340+offy);
	addKeyConfigLine(group_keyConfig, SB(2122), "NextPage",		360+offy);
	addKeyConfigLine(group_keyConfig, SB(2123), "CookFood",		380+offy);
	addKeyConfigLine(group_keyConfig, SB(2124), "FoodLeft",		400+offy);
	addKeyConfigLine(group_keyConfig, SB(2125), "FoodRight",	420+offy);
	addKeyConfigLine(group_keyConfig, SB(2126), "FoodDrop",		440+offy);

	addKeyConfigLine(group_keyConfig, SB(2127), "Look",			460+offy);
	
	addKeyConfigLine(group_keyConfig, SB(2128), "ToggleHelp",	480+offy);

#undef SB

	group_keyConfig->shareAlphaWithChildren = 1;
	group_keyConfig->followCamera = 1;
	group_keyConfig->alpha = 0;
	group_keyConfig->setHidden(true);

	group_keyConfig->position = Vector(0, -40);

	addRenderObject(group_keyConfig, LR_OVERLAY);
	

	cook = new AquariaMenuItem;
	cook->useQuad("Gui/cook-button");
	cook->useGlow("particles/glow", 128, 40);
	cook->position = worldRightCenter + Vector(0, -120);
	cook->alpha = 0;
	cook->scale = Vector(0.8, 0.8);
	cook->event.set(MakeFunctionEvent(Game, onCook));
	cook->setCanDirMove(false);
	addRenderObject(cook, LR_MENU);

	foodSort = new AquariaMenuItem;
	foodSort->useQuad("gui/sort");
	foodSort->useSound("click");
	foodSort->useGlow("particles/glow", 32,32);
	foodSort->position = worldLeftCenter + Vector(-100, -100);
	foodSort->event.set(MakeFunctionEvent(Game, sortFood));
	foodSort->alpha = 0;
	addRenderObject(foodSort, LR_MENU);

	recipes = new AquariaMenuItem;
	recipes->useQuad("Gui/recipes-button");
	recipes->useGlow("particles/glow", 128, 32);
	recipes->position = worldLeftCenter + Vector(-40, 140);
	recipes->alpha = 0;
	recipes->scale = Vector(0.8, 0.8);
	recipes->event.set(MakeFunctionEvent(Game, onRecipes));
	addRenderObject(recipes, LR_MENU);

	use = new AquariaMenuItem;
	use->useQuad("Gui/use-button");
	use->useGlow("particles/glow", 128, 64);
	use->position = worldRightCenter + Vector(0, -120);
	use->alpha = 0;
	use->scale = Vector(0.8, 0.8);
	use->event.set(MakeFunctionEvent(Game, onUseTreasure));
	addRenderObject(use, LR_MENU);

	prevFood = new AquariaMenuItem;
	prevFood->useQuad("Gui/arrow-left");
	prevFood->useSound("click");
	prevFood->useGlow("particles/glow", 64, 32);
	prevFood->position = worldLeftCenter + Vector(-50, -130);
	prevFood->alpha = 0;
	prevFood->event.set(MakeFunctionEvent(Game, onPrevFoodPage));
	prevFood->scale = Vector(0.6, 0.6);
	prevFood->setCanDirMove(false);
	addRenderObject(prevFood, LR_MENU);

	nextFood = new AquariaMenuItem;
	nextFood->useQuad("Gui/arrow-right");
	nextFood->useSound("click");
	nextFood->useGlow("particles/glow", 64, 32);
	nextFood->position = worldLeftCenter + Vector(50, -130);
	nextFood->alpha = 0;
	nextFood->setCanDirMove(false);
	nextFood->event.set(MakeFunctionEvent(Game, onNextFoodPage));
	nextFood->scale = Vector(0.6, 0.6);
	addRenderObject(nextFood, LR_MENU);

	prevTreasure = new AquariaMenuItem;
	prevTreasure->useQuad("Gui/arrow-left");
	prevTreasure->useSound("click");
	prevTreasure->useGlow("particles/glow", 64, 32);
	prevTreasure->position = worldLeftCenter + Vector(-50, -130);
	prevTreasure->alpha = 0;
	prevTreasure->setCanDirMove(false);
	prevTreasure->scale = Vector(0.6, 0.6);
	prevTreasure->event.set(MakeFunctionEvent(Game, onPrevTreasurePage));
	prevTreasure->setCanDirMove(false);
	addRenderObject(prevTreasure, LR_MENU);

	nextTreasure = new AquariaMenuItem;
	nextTreasure->useQuad("Gui/arrow-right");
	nextTreasure->useSound("click");
	nextTreasure->useGlow("particles/glow", 64, 32);
	nextTreasure->position = worldLeftCenter + Vector(50, -130);
	nextTreasure->alpha = 0;
	nextTreasure->scale = Vector(0.6, 0.6);
	nextTreasure->event.set(MakeFunctionEvent(Game, onNextTreasurePage));
	nextTreasure->setCanDirMove(false);
	addRenderObject(nextTreasure, LR_MENU);

	circlePageNum = new BitmapText(&dsq->smallFont);
	circlePageNum->color = Vector(0,0,0);
	circlePageNum->position = worldLeftCenter + Vector(0, -142);
	circlePageNum->alpha = 0;
	circlePageNum->followCamera = 1;
	addRenderObject(circlePageNum, LR_MENU);

	foodHolders.resize(3);
	int holders=0;
	for (i = 0; i < foodHolders.size(); i++)
	{
		foodHolders[i] = new FoodHolder(i);
		foodHolders[i]->alpha = 0;

		float angle = (float(holders)/float(foodHolders.size()))*PI*2;
		foodHolders[i]->position = rightCenter + Vector(sinf(angle), cosf(angle))*radius;
		holders ++;

		menuBg->addChild(foodHolders[i], PM_POINTER);
	}

	previewRecipe = new Quad;
	previewRecipe->alphaMod = 0.75;
	previewRecipe->followCamera = 1;
	previewRecipe->alpha = 0;
	previewRecipe->scale = Vector(0.7, 0.7);
	previewRecipe->scale.interpolateTo(Vector(0.9, 0.9), 0.5, -1, 1, 1);
	previewRecipe->position = worldRightCenter;
	addRenderObject(previewRecipe, LR_MENU);

	showRecipe = new Quad();
	showRecipe->followCamera = 1;
	showRecipe->position = Vector(575,250);
	addRenderObject(showRecipe, LR_MENU);
	
	float scrollx = 555;
	recipeMenu.scroll = new Quad("gui/recipe-scroll", Vector(scrollx, 200));
	recipeMenu.scroll->followCamera = 1;
	recipeMenu.scroll->alpha = 0;
	addRenderObject(recipeMenu.scroll, LR_RECIPES); // LR_HUD3

	recipeMenu.scrollEnd = new Quad("gui/recipe-scroll-end", Vector(scrollx, 400));
	recipeMenu.scrollEnd->followCamera = 1;
	recipeMenu.scrollEnd->alpha = 0;
	addRenderObject(recipeMenu.scrollEnd, LR_RECIPES);

	recipeMenu.header = new BitmapText(&dsq->font);
	recipeMenu.header->color = 0;
	recipeMenu.header->followCamera = 1;
	recipeMenu.header->setText(dsq->continuity.stringBank.get(2007));
	recipeMenu.header->alpha = 0;
	recipeMenu.header->position = Vector(scrollx, 5); //10
	addRenderObject(recipeMenu.header, LR_RECIPES);

	recipeMenu.page = new BitmapText(&dsq->smallFont);
	recipeMenu.page->color = 0;
	recipeMenu.page->followCamera = 1;
	recipeMenu.page->position = Vector(scrollx, 400);
	recipeMenu.page->setText(dsq->continuity.stringBank.get(2006));
	recipeMenu.page->alpha = 0;
	addRenderObject(recipeMenu.page, LR_RECIPES);
	
	recipeMenu.prevPage = new AquariaMenuItem;
	recipeMenu.prevPage->useQuad("Gui/arrow-left");
	recipeMenu.prevPage->useSound("click");
	recipeMenu.prevPage->useGlow("particles/glow", 64, 32);
	recipeMenu.prevPage->position = Vector(scrollx - 150, 410);
	recipeMenu.prevPage->alpha = 0;
	recipeMenu.prevPage->event.set(MakeFunctionEvent(Game, onPrevRecipePage));
	recipeMenu.prevPage->scale = Vector(0.8, 0.8);
	addRenderObject(recipeMenu.prevPage, LR_RECIPES);

	recipeMenu.nextPage = new AquariaMenuItem;
	recipeMenu.nextPage->useQuad("Gui/arrow-right");
	recipeMenu.nextPage->useSound("click");
	recipeMenu.nextPage->useGlow("particles/glow", 64, 32);
	recipeMenu.nextPage->position = Vector(scrollx + 150, 410);
	recipeMenu.nextPage->alpha = 0;
	recipeMenu.nextPage->event.set(MakeFunctionEvent(Game, onNextRecipePage));
	recipeMenu.nextPage->scale = Vector(0.8, 0.8);
	addRenderObject(recipeMenu.nextPage, LR_RECIPES);


	petSlots.resize(dsq->continuity.petData.size());
	for (i = 0; i < petSlots.size(); i++)
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
	foodHolders[3]->alpha = 0;
	foodHolders[3]->position = rightCenter + Vector(96, 150);
	menuBg->addChild(foodHolders[3], PM_POINTER);





	int outer = 0;
	int inner = 0;
	for (i = 0; i < songSlots.size(); i++)
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
	menuSongs = 0;

	menuMoney = menuEXP = 0;

	menuDescription = new BitmapText(&dsq->smallFont);
	menuDescription->setFontSize(14);
	menuDescription->position = Vector(400, 450);
	menuDescription->setAlign(ALIGN_CENTER);
	menuDescription->setWidth(400);
	menuDescription->followCamera = 1;
	menuDescription->alpha = 0;
	addRenderObject(menuDescription, LR_MENU);

	currentInventoryPage = 0;

	int areYouShim = -25;
	eAre = new Quad;
	eAre->position = Vector(400,448+areYouShim);
	eAre->setTexture("AreYouSure");
	eAre->alpha = 0;
	eAre->followCamera = 1;
	addRenderObject(eAre, LR_MENU);

	eYes = new AquariaMenuItem;
	eYes->position = Vector(400-100,516+areYouShim);
	eYes->useQuad("Yes");
	eYes->useGlow("particles/glow", 100, 32);
	eYes->event.set(MakeFunctionEvent(Game, onExitCheckYes));
	eYes->alpha = 0;
	eYes->shareAlpha = 1;
	addRenderObject(eYes, LR_MENU);

	eNo = new AquariaMenuItem;
	eNo->position = Vector(400+100,516+areYouShim);
	eNo->useQuad("No");
	eNo->useGlow("particles/glow", 100, 32);
	eNo->event.set(MakeFunctionEvent(Game, onExitCheckNo));
	eNo->alpha = 0;
	eNo->shareAlpha = 1;
	addRenderObject(eNo, LR_MENU);

	eNo->setDirMove(DIR_LEFT, eYes);
	eYes->setDirMove(DIR_RIGHT, eNo);



	menu.resize(10);
	for (i = 0; i < menu.size(); i++)
		menu[i] = new AquariaMenuItem;

	int ty = 530;
	//menu[0]->setLabel("Continue");
	menu[0]->event.set(MakeFunctionEvent(Game, onInGameMenuContinue));
	menu[0]->useGlow("particles/glow", 200, 100);
	//menu[0]->position = Vector(150, 550);
	menu[0]->position = Vector(150-30, ty-10);

	//menu[1]->setLabel("Exit");
	menu[1]->useGlow("particles/glow", 200, 100);
	menu[1]->event.set(MakeFunctionEvent(Game, onInGameMenuExit));
	//menu[1]->position = Vector(800-150, 550);
	//menu[1]->position = Vector(800-150+30, ty);
	menu[1]->position = Vector(800-150+20, ty-10);

	menu[2]->setLabel("DebugSave");
	menu[2]->event.set(MakeFunctionEvent(Game, onDebugSave));
	menu[2]->position = Vector(400,ty+60);
	if (!dsq->isDeveloperKeys())
		menu[2]->position = Vector(400, 12000);
	menu[2]->setCanDirMove(false);

	menu[3]->event.set(MakeFunctionEvent(Game, onLips));
	menu[3]->useGlow("particles/glow", 64, 64);
	//menu[0]->position = Vector(150, 550);
	menu[3]->position = Vector(400, 195);
	menu[3]->setCanDirMove(false);

	lips = menu[3];

	// options
	menu[4]->event.set(MakeFunctionEvent(Game, onOptionsMenu));
	menu[4]->useGlow("particles/glow", 200, 32);
	menu[4]->position = Vector(400,ty+10);

	int gs = 40;
	
	menu[5]->event.set(MakeFunctionEvent(Game, switchToSongMenu));
	menu[5]->useQuad("gui/icon-songs");
	menu[5]->useGlow("particles/glow", gs, gs);
	menu[5]->useSound("Click");
	menu[5]->position = Vector(400-60, 350);
	
	menuIconGlow = new Quad("particles/glow", menu[5]->position);
	menuIconGlow->alphaMod = 0.4;
	menuIconGlow->alpha = 0;
	menuIconGlow->setWidthHeight(80, 80);
	menuIconGlow->setBlendType(RenderObject::BLEND_ADD);
	menuIconGlow->followCamera = 1;
	addRenderObject(menuIconGlow, LR_MENU);

	menu[6]->event.set(MakeFunctionEvent(Game, switchToFoodMenu));
	menu[6]->useQuad("gui/icon-food");
	menu[6]->useGlow("particles/glow", gs, gs);
	menu[6]->useSound("Click");
	menu[6]->position = Vector(400-20, 350);

	menu[7]->event.set(MakeFunctionEvent(Game, switchToPetMenu));
	menu[7]->useQuad("gui/icon-pets");
	menu[7]->useGlow("particles/glow", gs, gs);
	menu[7]->useSound("Click");
	menu[7]->position = Vector(400+20, 350);

	menu[8]->event.set(MakeFunctionEvent(Game, switchToTreasureMenu));
	menu[8]->useQuad("gui/icon-treasures");
	menu[8]->useGlow("particles/glow", gs, gs);
	menu[8]->useSound("Click");
	menu[8]->position = Vector(400+60, 350);

	menu[9]->event.set(MakeFunctionEvent(Game, onToggleHelpScreen));
	menu[9]->useQuad("gui/icon-help");
	menu[9]->useGlow("particles/glow", gs, gs);
	menu[9]->useSound("Click");
	menu[9]->position = Vector(400+60*3, 410);

	/*
	menu[9]->event.set(MakeFunctionEvent(Game, sortFood));
	menu[9]->setLabel("sort food");
	menu[9]->position = Vector(100,100);
	*/

	for (i = 0; i < menu.size(); i++)
	{
		addRenderObject(menu[i], LR_MENU);
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
	for (i = 0; i < foodSlots.size(); i++)
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
		addRenderObject(foodSlots[i], LR_HUD2);
	}


	foodLabel = new BitmapText(&dsq->smallFont);
	{
		foodLabel->alpha = 0;
		foodLabel->setAlign(ALIGN_CENTER);
		foodLabel->followCamera = 1;
		foodLabel->setFontSize(20);
		foodLabel->position = center - Vector(0, 16) + Vector(0,-32);
		foodLabel->scale = Vector(1, 1);
	}
	menuBg->addChild(foodLabel, PM_POINTER);

	foodDescription = new BitmapText(&dsq->smallFont);
	{
		foodDescription->alpha = 0;
		foodDescription->setAlign(ALIGN_CENTER);
		foodDescription->followCamera = 1;
		foodDescription->position = center + Vector(0, 8) + Vector(0,-32);
		foodDescription->scale = Vector(0.8, 0.8);

		foodDescription->setWidth(240);
	}
	menuBg->addChild(foodDescription, PM_POINTER);


// ---------- TREASURES

	
	int treasureSlotRadius = 96;

	treasureSlots.resize(treasurePageSize);

	for (i = 0; i < treasureSlots.size(); i++)
	{
		treasureSlots[i] = new TreasureSlot(i);


		float angle = (float(i)/float(treasureSlots.size()))*PI*2;
		treasureSlots[i]->position = worldCenter + Vector(sinf(angle), cosf(angle))*treasureSlotRadius;

		treasureSlots[i]->alphaMod = 0;

		treasureSlots[i]->followCamera = 1;

		//treasureSlots[i]->scaleFactor = 0.75;

		addRenderObject(treasureSlots[i], LR_MENU);
	}

	treasureLabel = new BitmapText(&dsq->smallFont);
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
	addRenderObject(treasureDescription, LR_HUD);

	foodTips.push_back(tip);

	treasureCloseUp = new Quad();
		treasureCloseUp->position = rightCenter;
		treasureCloseUp->alpha = 0;
	menuBg->addChild(treasureCloseUp, PM_POINTER);



	menuBg->alpha = 0;
}

void Game::onNextRecipePage()
{
	game->recipeMenu.goNextPage();
}

void Game::onPrevRecipePage()
{
	game->recipeMenu.goPrevPage();
}

void Game::toggleOverrideZoom(bool on)
{
	if (avatar)
	{
		if (on )
		{
			if (avatar->isEntityDead())
				return;
		}
		if (!on && avatar->zoomOverriden == true)
		{
			dsq->globalScale.stop();
			dsq->game->avatar->myZoom = dsq->globalScale;
			//dsq->game->avatar->myZoom.interpolateTo(Vector(1,1), 1.0);
		}
		avatar->zoomOverriden = on;
	}
}

void Game::addProgress()
{
	if (progressBar)
	{
		progressBar->progress();
	}
}

void Game::endProgress()
{
	if (progressBar)
	{
		progressBar->setLife(1);
		progressBar->setDecayRate(1.0f/0.5f);
		progressBar->fadeAlphaWithLife = 1;
		progressBar = 0;
	}
}

bool Game::loadSceneXML(std::string scene)
{
	bgSfxLoop = "";
	airSfxLoop = "";
	elementTemplatePack = "Main";
	entitySaveData.clear();
	std::string fn = getSceneFilename(scene);
	if (!exists(fn))
	{
		//errorLog("Could not find [" + fn + "]");
		//msg("Could not find map [" + fn + "]");
		debugLog("Could not find map [" + fn + "]");
		return false;
	}
	TiXmlDocument doc;
	doc.LoadFile(fn);
	if (saveFile)
	{
		delete saveFile;
		saveFile = 0;
	}
	if (!saveFile)
	{
		saveFile = new TiXmlDocument();
	}

	addProgress();

	clearObsRows();
	warpAreas.clear();
	TiXmlElement *lensFlare = doc.FirstChildElement("LensFlare");
	while (lensFlare)
	{
		LensFlare *l = new LensFlare;
		SimpleIStringStream is(lensFlare->Attribute("tex"));
		int w = -1, h=-1;
		w = atoi(lensFlare->Attribute("w"));
		h = atoi(lensFlare->Attribute("h"));

		std::string tex;
		while (is >> tex)
		{
			if (!tex.empty())
				l->addFlare(tex, Vector(1,1,1), w, h);
		}
		SimpleIStringStream is2(lensFlare->Attribute("inc"));
		is2 >> l->inc;
		l->maxLen = atoi(lensFlare->Attribute("maxLen"));
		/*
		l->addFlare("flares/flare0", Vector(1,1,0.5));
		l->addFlare("flares/flare1", Vector(1,1,1));
		l->addFlare("flares/flare2", Vector(0.5,1,1));
		l->addFlare("flares/flare2", Vector(1,1,1));
		*/
		l->position = Vector(atoi(lensFlare->Attribute("x")),atoi(lensFlare->Attribute("y")));
		addRenderObject(l, LR_LIGHTING);

		TiXmlElement lSF("LensFlare");
		lSF.SetAttribute("inc", lensFlare->Attribute("inc"));
		lSF.SetAttribute("x", lensFlare->Attribute("x"));
		lSF.SetAttribute("y", lensFlare->Attribute("y"));
		lSF.SetAttribute("tex", lensFlare->Attribute("tex"));
		lSF.SetAttribute("w", lensFlare->Attribute("w"));
		lSF.SetAttribute("h", lensFlare->Attribute("h"));
		lSF.SetAttribute("maxLen", lensFlare->Attribute("maxLen"));
		saveFile->InsertEndChild(lSF);

		lensFlare = lensFlare->NextSiblingElement("LensFlare");
	}
	TiXmlElement *level = doc.FirstChildElement("Level");
	if (level)
	{
		TiXmlElement levelSF("Level");
		if (level->Attribute("tileset"))
		{
			elementTemplatePack = level->Attribute("tileset");
			loadElementTemplates(elementTemplatePack);
			levelSF.SetAttribute("tileset", elementTemplatePack);
		}
		else if (level->Attribute("elementTemplatePack"))
		{
			elementTemplatePack = level->Attribute("elementTemplatePack");
			loadElementTemplates(elementTemplatePack);
			levelSF.SetAttribute("tileset", elementTemplatePack);
		}
		else
			return false;

		if (level->Attribute("waterLevel"))
		{
			useWaterLevel = true;
			waterLevel = atoi(level->Attribute("waterLevel"));
			saveWaterLevel = atoi(level->Attribute("waterLevel"));
			levelSF.SetAttribute("waterLevel", waterLevel.x);
		}
		if (level->Attribute("worldMapIndex"))
		{
			worldMapIndex = atoi(level->Attribute("worldMapIndex"));
			levelSF.SetAttribute("worldMapIndex", worldMapIndex);
		}

		if (level->Attribute("bgSfxLoop"))
		{
			bgSfxLoop = level->Attribute("bgSfxLoop");
			levelSF.SetAttribute("bgSfxLoop", bgSfxLoop);
		}
		if (level->Attribute("airSfxLoop"))
		{
			airSfxLoop = level->Attribute("airSfxLoop");
			levelSF.SetAttribute("airSfxLoop", airSfxLoop);
		}
		if (level->Attribute("bnat"))
		{
			bNatural = atoi(level->Attribute("bnat"));
			levelSF.SetAttribute("bnat", 1);
		}
		else
		{
			bNatural = false;
		}

		/*
		if (level->Attribute("darkLayer"))
		{
			int v = (atoi(level->Attribute("darkLayer")));

			levelSF.SetAttribute("darkLayer", v);
		}
		*/
		dsq->darkLayer.toggle(true);

		if (level->Attribute("bgRepeat"))
		{
			SimpleIStringStream is(level->Attribute("bgRepeat"));
			is >> backgroundImageRepeat;
			levelSF.SetAttribute("bgRepeat", level->Attribute("bgRepeat"));
		}
		if (level->Attribute("cameraConstrained"))
		{
			SimpleIStringStream is(level->Attribute("cameraConstrained"));
			is >> cameraConstrained;
			levelSF.SetAttribute("cameraConstrained", cameraConstrained);
			std::ostringstream os;
			os << "cameraConstrained: " << cameraConstrained;
			debugLog(os.str());
		}
		if (level->Attribute("maxZoom"))
		{
			maxZoom = atof(level->Attribute("maxZoom"));
			std::ostringstream os;
			os << maxZoom;
			levelSF.SetAttribute("maxZoom", os.str());
		}
		if (level->Attribute("natureForm"))
		{
			sceneNatureForm = level->Attribute("natureForm");
			levelSF.SetAttribute("natureForm", sceneNatureForm);
		}
		if (level->Attribute("bg"))
		{
			std::string tex = std::string(level->Attribute("bg"));
			if (!tex.empty())
			{
				/*
				if (tex.find('.') == std::string::npos)
					bg->setTexture(tex+"");
				else
					bg->setTexture(tex);
				*/

				bg->setTexture(tex);
				bg->setWidthHeight(900,600);
				levelSF.SetAttribute("bg", tex);
			}
			else
			{
				bg->alpha = 0;
			}
		}
		else
		{
			bg->alpha = 0;
			//grad->alpha =0;
		}
		gradTop = gradBtm = Vector(0,0,0);
		if (level->Attribute("gradient"))
		{
			if (level->Attribute("gradTop"))
			{
				SimpleIStringStream is(level->Attribute("gradTop"));
				is >> gradTop.x >> gradTop.y >> gradTop.z;
				levelSF.SetAttribute("gradTop", level->Attribute("gradTop"));
			}
			if (level->Attribute("gradBtm"))
			{
				SimpleIStringStream is(level->Attribute("gradBtm"));
				is >> gradBtm.x >> gradBtm.y >> gradBtm.z;
				levelSF.SetAttribute("gradBtm", level->Attribute("gradBtm"));
			}
			createGradient();
			levelSF.SetAttribute("gradient", 1);
		}

		if (level->Attribute("parallax"))
		{
			SimpleIStringStream is(level->Attribute("parallax"));
			float x,y,z,r,g,b;
			is >> x >> y >> z >> r >> g >> b;
			RenderObjectLayer *l = 0;
			l = &dsq->renderObjectLayers[LR_ELEMENTS10];
			l->followCamera = x;
			l = &dsq->renderObjectLayers[LR_ELEMENTS11];
			l->followCamera = y;
			l = &dsq->renderObjectLayers[LR_ENTITIES_MINUS4_PLACEHOLDER];
			l->followCamera = y;
			l = &dsq->renderObjectLayers[LR_ENTITIES_MINUS4];
			l->followCamera = y;
			l = &dsq->renderObjectLayers[LR_ELEMENTS12];
			l->followCamera = z;
			l = &dsq->renderObjectLayers[LR_ELEMENTS14];
			l->followCamera = r;
			l = &dsq->renderObjectLayers[LR_ELEMENTS15];
			l->followCamera = g;
			l = &dsq->renderObjectLayers[LR_ELEMENTS16];
			l->followCamera = b;
			levelSF.SetAttribute("parallax", level->Attribute("parallax"));
		}

		if (level->Attribute("parallaxLock"))
		{
			int x, y, z, r, g, b;
			SimpleIStringStream is(level->Attribute("parallaxLock"));
			is >> x >> y >> z >> r >> g >> b;

			RenderObjectLayer *l = 0;
			l = &dsq->renderObjectLayers[LR_ELEMENTS10];
			l->followCameraLock = x;
			l = &dsq->renderObjectLayers[LR_ELEMENTS11];
			l->followCameraLock = y;
			l = &dsq->renderObjectLayers[LR_ELEMENTS12];
			l->followCameraLock = z;
			l = &dsq->renderObjectLayers[LR_ELEMENTS14];
			l->followCameraLock = r;
			l = &dsq->renderObjectLayers[LR_ELEMENTS15];
			l->followCameraLock = g;
			l = &dsq->renderObjectLayers[LR_ELEMENTS16];
			l->followCameraLock = b;

			levelSF.SetAttribute("parallaxLock", level->Attribute("parallaxLock"));
		}

		if (level->Attribute("bg2"))
		{

			std::string tex = std::string(level->Attribute("bg2"));
			if (!tex.empty())
			{
				/*
				if (tex.find('.') == std::string::npos)
					bg2->setTexture(tex+"");
				else
					bg2->setTexture(tex);
				*/
				bg2->setTexture(tex);
				bg2->setWidthHeight(900,600);
				levelSF.SetAttribute("bg2", tex);

			}
			else
				bg2->alpha = 0;
			//createGradient();

			bg2->alpha = 0;
			bg->alpha = 0;
		}
		else
		{
			bg2->alpha = 0;
			//grad->alpha =0;
		}

		if (level->Attribute("backdrop"))
		{
			std::string backdrop = level->Attribute("backdrop");
			backdropQuad = new Quad;
			backdropQuad->setTexture(backdrop);
			backdropQuad->blendEnabled = false;

			if (level->Attribute("bd-x") && level->Attribute("bd-y"))
			{
				int x = atoi(level->Attribute("bd-x"));
				int y = atoi(level->Attribute("bd-y"));
				backdropQuad->position = Vector(x,y);
				levelSF.SetAttribute("bd-x", x);
				levelSF.SetAttribute("bd-y", y);
			}
			if (level->Attribute("bd-w") && level->Attribute("bd-h"))
			{
				int w = atoi(level->Attribute("bd-w"));
				int h = atoi(level->Attribute("bd-h"));
				backdropQuad->setWidthHeight(w, h);
				levelSF.SetAttribute("bd-w", w);
				levelSF.SetAttribute("bd-h", h);
			}
			backdropQuad->toggleCull(false);
			//backdropQuad->followCamera = 1;
			addRenderObject(backdropQuad, LR_SCENEBACKGROUNDIMAGE);

			// upper left justify
			backdropQuad->offset =
				Vector((backdropQuad->getWidth()*backdropQuad->scale.x)/2.0f,
				(backdropQuad->getHeight()*backdropQuad->scale.y)/2.0f);
			// save
			levelSF.SetAttribute("backdrop", backdrop.c_str());
			//backdrop="cavebg" bd-w="2400" bd-h="2400"
		}
		musicToPlay = "";
		if (level->Attribute("music"))
		{
			setMusicToPlay(level->Attribute("music"));
			saveMusic = level->Attribute("music");
			levelSF.SetAttribute("music", level->Attribute("music"));
			/*
			// if using SDL_Mixer
			if (!core->sound->isPlayingMusic(musicToPlay))
			{
				core->sound->fadeMusic(SFT_OUT, 1);
			}
			*/
		}
		if (level->Attribute("sceneColor"))
		{
			SimpleIStringStream in(level->Attribute("sceneColor"));
			in >> sceneColor.x >> sceneColor.y >> sceneColor.z;
			levelSF.SetAttribute("sceneColor", level->Attribute("sceneColor"));
		}

		saveFile->InsertEndChild(levelSF);
	}
	else
		return false;

	TiXmlElement *obs = doc.FirstChildElement("Obs");
	if (obs)
	{
		int tx, ty, len;
		SimpleIStringStream is(obs->Attribute("d"));
		while (is >> tx)
		{
			is >> ty >> len;
			addObsRow(tx, ty, len);
		}
		addProgress();
	}

	TiXmlElement *pathXml = doc.FirstChildElement("Path");
	while (pathXml)
	{
		Path *path = new Path;
		path->name = pathXml->Attribute("name");
		stringToLower(path->name);
		/*
		if (pathXml->Attribute("active"))
		{
			path.active = atoi(pathXml->Attribute("active"));
		}
		*/
		TiXmlElement *nodeXml = pathXml->FirstChildElement("Node");
		while (nodeXml)
		{
			PathNode node;
			SimpleIStringStream is(nodeXml->Attribute("pos"));
			is >> node.position.x >> node.position.y;

			if (nodeXml->Attribute("ms"))
			{
				node.maxSpeed = atoi(nodeXml->Attribute("ms"));
			}

			if (nodeXml->Attribute("rect"))
			{
				SimpleIStringStream is(nodeXml->Attribute("rect"));
				int w,h;
				is >> w >> h;
				path->rect.setWidth(w);
				path->rect.setHeight(h);
			}

			if (nodeXml->Attribute("shape"))
			{
				path->pathShape = (PathShape)atoi(nodeXml->Attribute("shape"));
			}

			path->nodes.push_back(node);
			nodeXml = nodeXml->NextSiblingElement("Node");
		}
		path->refreshScript();
		addPath(path);
		addProgress();
		pathXml = pathXml->NextSiblingElement("Path");
	}

	TiXmlElement *quad = doc.FirstChildElement("Quad");
	while (quad)
	{
		TiXmlElement qSF("Quad");
		int x=0, y=0, z=0;
		int w=0,h=0;
		bool cull=true;
		bool solid = false;
		std::string justify;
		std::string tex;
		qSF.SetAttribute("x", x = atoi(quad->Attribute("x")));
		qSF.SetAttribute("y", y = atoi(quad->Attribute("y")));
		//qSF.SetAttribute("z", z = atoi(quad->Attribute("z")));
		qSF.SetAttribute("w", w = atoi(quad->Attribute("w")));
		qSF.SetAttribute("h", h = atoi(quad->Attribute("h")));
		qSF.SetAttribute("tex", tex = (quad->Attribute("tex")));
		qSF.SetAttribute("cull", cull = atoi(quad->Attribute("cull")));
		qSF.SetAttribute("justify", justify = (quad->Attribute("justify")));

		if (quad->Attribute("solid"))
			qSF.SetAttribute("solid", solid = atoi(quad->Attribute("solid")));

		Quad *q = new Quad;
		q->position = Vector(x,y,z);
		/*
		if (solid)
			Texture::pngLoadHaloFix = false;
		*/
		q->setTexture(tex);
		/*
		if (solid)
			Texture::pngLoadHaloFix = true;
		*/
		q->toggleCull(cull);
		q->setWidthHeight(w, h);

		if (justify == "upperLeft")
		{
			q->offset = Vector((q->getWidth()*q->scale.x)/2.0f, (q->getHeight()*q->scale.y)/2.0f);
		}
		addRenderObject(q, LR_BACKGROUND);

		saveFile->InsertEndChild(qSF);

		quad = quad->NextSiblingElement("Quad");
	}

	TiXmlElement *floater = doc.FirstChildElement("Floater");
	while(floater)
	{
		TiXmlElement nSF("Floater");
		if (!floater->Attribute("boxW") || !floater->Attribute("boxH"))
		{
			errorLog ("no boxW/boxH");
			break;
		}
		int boxW, boxH, x, y, fx, fy;
		std::string tex;
		nSF.SetAttribute("boxW", boxW = atoi(floater->Attribute("boxW")));
		nSF.SetAttribute("boxH", boxH = atoi(floater->Attribute("boxH")));
		tex = floater->Attribute("tex");
		nSF.SetAttribute("tex", tex);
		nSF.SetAttribute("x", x = atoi(floater->Attribute("x")));
		nSF.SetAttribute("y", y = atoi(floater->Attribute("y")));
		nSF.SetAttribute("fx", fx = atoi(floater->Attribute("fx")));
		nSF.SetAttribute("fy", fy = atoi(floater->Attribute("fy")));

		/*
		Floater *f = new Floater(Vector(x,y), Vector(fx, fy), boxW, boxH, tex);
		{
		}
		addRenderObject(f, LR_BACKGROUND);
		saveFile->InsertEndChild(nSF);
		*/
		floater = floater->NextSiblingElement("Floater");

	}

	/*
	TiXmlElement *breakable = doc.FirstChildElement("Breakable");
	while(breakable)
	{
		TiXmlElement nSF("Breakable");
		if (!breakable->Attribute("boxW") || !breakable->Attribute("boxH"))
		{
			errorLog ("Breakable error.. no boxW/boxH");
			break;
		}
		int boxW, boxH;
		std::string tex;
		nSF.SetAttribute("boxW", boxW = atoi(breakable->Attribute("boxW")));
		nSF.SetAttribute("boxH", boxH = atoi(breakable->Attribute("boxH")));
		tex = breakable->Attribute("tex");
		nSF.SetAttribute("tex", tex);
		Breakable *n = new Breakable(boxW, boxH, tex);
		{
			nSF.SetAttribute("x", n->position.x = atoi(breakable->Attribute("x")));
			nSF.SetAttribute("y", n->position.y = atoi(breakable->Attribute("y")));
			int w=0, h=0;
			if (breakable->Attribute("w"))
				nSF.SetAttribute("w", w = atoi(breakable->Attribute("w")));
			if (breakable->Attribute("h"))
				nSF.SetAttribute("h", h= atoi(breakable->Attribute("h")));
			if (w != 0 && h != 0)
			{
				n->setWidthHeight(w, h);
			}
		}
		addRenderObject(n, LR_BACKGROUND);
		saveFile->InsertEndChild(nSF);
		breakable = breakable->NextSiblingElement("Breakable");
	}
	*/

	TiXmlElement *warpArea = doc.FirstChildElement("WarpArea");
	while(warpArea)
	{
		TiXmlElement waSF("WarpArea");
		WarpArea a;
		waSF.SetAttribute("x", a.position.x = atoi(warpArea->Attribute("x")));
		waSF.SetAttribute("y", a.position.y = atoi(warpArea->Attribute("y")));
		if (warpArea->Attribute("radius"))
			waSF.SetAttribute("radius", a.radius = atoi(warpArea->Attribute("radius")));
		bool isRect = false;
		if (warpArea->Attribute("w"))
		{
			isRect = true;
			waSF.SetAttribute("w", a.w = atoi(warpArea->Attribute("w")));
			waSF.SetAttribute("h", a.h = atoi(warpArea->Attribute("h")));
		}
		if (warpArea->Attribute("g"))
		{
			waSF.SetAttribute("g", a.generated = atoi(warpArea->Attribute("g")));
		}
		std::string sceneString = warpArea->Attribute("scene");
		waSF.SetAttribute("scene", sceneString.c_str());
		/*
		waSF.SetAttribute("ax", a.avatarPosition.x = atoi(warpArea->Attribute("ax")));
		waSF.SetAttribute("ay", a.avatarPosition.y = atoi(warpArea->Attribute("ay")));
		*/

		SimpleIStringStream is(sceneString);
		std::string sceneName, warpAreaType, side;
		is >> sceneName >> warpAreaType >> a.spawnOffset.x >> a.spawnOffset.y;
		a.spawnOffset.normalize2D();
		a.sceneName = sceneName;
		a.warpAreaType = warpAreaType;
		//a.side = side;
		// saveFile->InsertEndChild(waSF);

		bool add = true;
		std::string flagCheck;
		if (warpArea->Attribute("flagCheck"))
		{
			flagCheck = warpArea->Attribute("flagCheck");
			add = doFlagCheck(flagCheck);
		}
		if (add)
			warpAreas.push_back(a);

		if (a.generated)
		{
			setWarpAreaSceneName(a);
		}

		warpArea = warpArea->NextSiblingElement("WarpArea");
	}

	TiXmlElement *schoolFish = doc.FirstChildElement("SchoolFish");
	while(schoolFish)
	{
		int num = atoi(schoolFish->Attribute("num"));
		int x, y;
		int id;
		x = atoi(schoolFish->Attribute("x"));
		y = atoi(schoolFish->Attribute("y"));
		id = atoi(schoolFish->Attribute("id"));
		std::string gfx, texture="flock-0001";
		if (schoolFish->Attribute("gfx"))
		{
			gfx = schoolFish->Attribute("gfx");
			texture = gfx;
			/*
			std::ostringstream os;
			os << "flock-" << gfx << "";
			texture = os.str();
			*/
		}
		int layer = 0;
		if (schoolFish->Attribute("layer"))
		{
			layer = atoi(schoolFish->Attribute("layer"));
		}

		float size = 1;
		if (schoolFish->Attribute("size"))
		{
			SimpleIStringStream is(schoolFish->Attribute("size"));
			is >> size;
		}

		int maxSpeed = 0;
		if (schoolFish->Attribute("maxSpeed"))
			maxSpeed = atoi(schoolFish->Attribute("maxSpeed"));

		int range = 0;
		if (schoolFish->Attribute("range"))
			range = atoi(schoolFish->Attribute("range"));

		for (int i = 0; i < num; i++)
		{
			SchoolFish *s = new SchoolFish(texture);
			{
				s->position = Vector(x+i*5,y+i*5);
				s->startPos = s->position;
				s->addToFlock(id);
				if (range != 0)
					s->range = range;
				if (maxSpeed != 0)
					s->setMaxSpeed(maxSpeed);

				std::ostringstream os;
				os << "adding schoolfish (" << s->position.x << ", " << s->position.y << ")";
				debugLog(os.str());
			}
			if (layer == -3)
			{
				addRenderObject(s, LR_ELEMENTS11);
			}
			else
			{
				if (chance(50))
					addRenderObject(s, LR_ENTITIES2);
				else
					addRenderObject(s, LR_ENTITIES);
			}

			/*if (layer == 1)
			{
				addRenderObject(s, LR_ENTITIES);
			}
			else
			{
				// school fish layer hack
				// because we want all fish on top dammit
				//addRenderObject(s, LR_ENTITIES2);

				// or...  not?
				//hrm.. why not?
				if (chance(50))
					addRenderObject(s, LR_ENTITIES2);
				else
					addRenderObject(s, LR_ENTITIES);
				//s->setOverrideRenderPass(4);
			}
			*/
			s->applyLayer(layer);

			s->scale *= size;
			//s->update(0.033);

		}

		schoolFish = schoolFish->NextSiblingElement("SchoolFish");

		TiXmlElement newSF("SchoolFish");
		newSF.SetAttribute("x", x);
		newSF.SetAttribute("y", y);
		newSF.SetAttribute("id", id);
		newSF.SetAttribute("num", num);

		if (range != 0)
			newSF.SetAttribute("range", range);
		if (maxSpeed != 0)
			newSF.SetAttribute("maxSpeed", maxSpeed);
		if (layer != 0)
			newSF.SetAttribute("layer", layer);
		if (!gfx.empty())
			newSF.SetAttribute("gfx", gfx.c_str());
		if (size != 1)
			newSF.SetAttribute("size", size);

		saveFile->InsertEndChild(newSF);
	}
	/*
	TiXmlElement *boxElement = doc.FirstChildElement("BoxElement");
	while (boxElement)
	{
		BoxElement *b = new BoxElement(atoi(boxElement->Attribute("w")), atoi(boxElement->Attribute("h")));
		b->position = Vector(atoi(boxElement->Attribute("x")), atoi(boxElement->Attribute("y")));
		addRenderObject(b, LR_BLACKGROUND);
		b->position.z = boxElementZ;
		dsq->addElement(b);
		boxElement = boxElement->NextSiblingElement("BoxElement");
	}
	*/
	TiXmlElement *simpleElements = doc.FirstChildElement("SE");
	while (simpleElements)
	{
		int idx, x, y, rot;
		float sz,sz2;
		if (simpleElements->Attribute("d"))
		{
			SimpleIStringStream is(simpleElements->Attribute("d"));
			while (is >> idx)
			{
				is >> x >> y >> rot;
				Element *e = createElement(idx, Vector(x,y), 4);
				e->rotation.z = rot;
			}
		}
		if (simpleElements->Attribute("e"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("e"));
			int l = atoi(simpleElements->Attribute("l"));
			while(is2 >> idx)
			{
				is2 >> x >> y >> rot;
				Element *e = createElement(idx, Vector(x,y), l);
				e->rotation.z = rot;
			}
		}
		if (simpleElements->Attribute("f"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("f"));
			int l = atoi(simpleElements->Attribute("l"));
			while(is2 >> idx)
			{
				is2 >> x >> y >> rot >> sz;
				Element *e = createElement(idx, Vector(x,y), l);
				e->scale = Vector(sz,sz);
				e->rotation.z = rot;
			}
		}
		if (simpleElements->Attribute("g"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("g"));
			int l = atoi(simpleElements->Attribute("l"));
			while(is2 >> idx)
			{
				int fh, fv;
				is2 >> x >> y >> rot >> sz >> fh >> fv;
				Element *e = createElement(idx, Vector(x,y), l);
				if (fh)
					e->flipHorizontal();
				if (fv)
					e->flipVertical();
				e->scale = Vector(sz,sz);
				e->rotation.z = rot;
			}
		}
		if (simpleElements->Attribute("h"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("h"));
			int l = atoi(simpleElements->Attribute("l"));
			while(is2 >> idx)
			{
				int fh, fv;
				int flags;
				is2 >> x >> y >> rot >> sz >> fh >> fv >> flags;
				Element *e = createElement(idx, Vector(x,y), l);
				e->elementFlag = (ElementFlag)flags;
				if (e->elementFlag >= EF_MAX || e->elementFlag < EF_NONE)
					e->elementFlag = EF_NONE;
				if (fh)
					e->flipHorizontal();
				if (fv)
					e->flipVertical();
				e->scale = Vector(sz,sz);
				e->rotation.z = rot;
			}
		}
		if (simpleElements->Attribute("i"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("i"));
			int l = atoi(simpleElements->Attribute("l"));
			while(is2 >> idx)
			{
				int fh, fv;
				int flags;
				int efxIdx;
				is2 >> x >> y >> rot >> sz >> fh >> fv >> flags >> efxIdx;
				if (sz < MIN_SIZE)
					sz = MIN_SIZE;
				Element *e = createElement(idx, Vector(x,y), l);
				e->elementFlag = (ElementFlag)flags;
				if (fh)
					e->flipHorizontal();
				if (fv)
					e->flipVertical();

				e->scale = Vector(sz,sz);
				e->rotation.z = rot;
				e->setElementEffectByIndex(efxIdx);
			}
		}
		if (simpleElements->Attribute("j"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("j"));
			int l = atoi(simpleElements->Attribute("l"));
			while(is2 >> idx)
			{
				int fh, fv;
				int flags;
				int efxIdx;
				int repeat;
				is2 >> x >> y >> rot >> sz >> fh >> fv >> flags >> efxIdx >> repeat;
				if (sz < MIN_SIZE)
					sz = MIN_SIZE;
				Element *e = createElement(idx, Vector(x,y), l);
				e->elementFlag = (ElementFlag)flags;
				if (fh)
					e->flipHorizontal();
				if (fv)
					e->flipVertical();

				e->scale = Vector(sz,sz);
				e->rotation.z = rot;
				e->setElementEffectByIndex(efxIdx);
				if (repeat)
					e->repeatTextureToFill(true);
			}
		}
		if (simpleElements->Attribute("k"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("k"));
			int l = atoi(simpleElements->Attribute("l"));
			int c = 0;
			while(is2 >> idx)
			{
				int fh, fv;
				int flags;
				int efxIdx;
				int repeat;
				is2 >> x >> y >> rot >> sz >> sz2 >> fh >> fv >> flags >> efxIdx >> repeat;
				if (sz < MIN_SIZE)
					sz = MIN_SIZE;
				if (sz2 < MIN_SIZE)
					sz2 = MIN_SIZE;
				Element *e = createElement(idx, Vector(x,y), l);
				e->elementFlag = (ElementFlag)flags;
				if (fh)
					e->flipHorizontal();
				if (fv)
					e->flipVertical();

				e->scale = Vector(sz,sz2);
				e->rotation.z = rot;
				e->setElementEffectByIndex(efxIdx);
				if (repeat)
					e->repeatTextureToFill(true);

				c++;
				if (c> 100)
				{
					c=0;
					addProgress();
				}
			}
		}
		simpleElements = simpleElements->NextSiblingElement("SE");
	}

	TiXmlElement *element = doc.FirstChildElement("Element");
	while (element)
	{
		if (element->Attribute("idx"))
		{
			int x = atoi(element->Attribute("x"));
			int y = atoi(element->Attribute("y"));
			int idx = atoi(element->Attribute("idx"));
			int layer=LR_ELEMENTS5;
			float rot =0;
			bool flipH = false, flipV = false;
			if (element->Attribute("flipH"))
				flipH = atoi(element->Attribute("flipH"));
			if (element->Attribute("flipV"))
				flipV = atoi(element->Attribute("flipV"));

			if (element->Attribute("rot"))
				rot = atof(element->Attribute("rot"));

			if (element->Attribute("lyr"))
				layer = atoi(element->Attribute("lyr"));


			if (idx != -1)
			{
				Element *e = createElement(idx, Vector(x,y), layer);
				e->rotation.z = rot;
				if (flipH)
					e->flipHorizontal();
				if (flipV)
					e->flipVertical();

				if (element->Attribute("sz"))
				{
					SimpleIStringStream is(element->Attribute("sz"));
					is >> e->scale.x >> e->scale.y;
				}
			}


		}
		element = element->NextSiblingElement("Element");
	}

	this->reconstructGrid(true);

	/*
	TiXmlElement *enemyNode = doc.FirstChildElement("Enemy");
	while(enemyNode)
	{
		Vector pos;
		pos.x = atoi(enemyNode->Attribute("x"));
		pos.y = atoi(enemyNode->Attribute("y"));

		std::string type = enemyNode->Attribute("type");

		std::string flagCheck;
		if (enemyNode->Attribute("flagCheck"))
		{
			flagCheck = enemyNode->Attribute("flagCheck");
		}
		if (doFlagCheck(flagCheck))
			Entity *e = createEnemy(type, pos, true, enemyNode, flagCheck, -1);

		enemyNode = enemyNode->NextSiblingElement("Enemy");
	}
	*/
	TiXmlElement *entitiesNode = doc.FirstChildElement("Entities");
	while(entitiesNode)
	{
		if (entitiesNode->Attribute("d"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("d"));
			int idx, x, y;
			while (is >> idx)
			{
				is >> x >> y;
				dsq->game->createEntity(idx, 0, Vector(x,y), 0, true, "");
			}
		}
		if (entitiesNode->Attribute("e"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("e"));
			int idx, x, y, rot;
			while (is >> idx)
			{
				is >> x >> y >> rot;
				if (idx == 32)
				{
					std::ostringstream os;
					os << "read in rot as: " << rot;
					debugLog(os.str());
				}
				dsq->game->createEntity(idx, 0, Vector(x,y), rot, true, "");
			}
		}
		if (entitiesNode->Attribute("f"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("f"));
			int idx, x, y, rot, group;
			while (is >> idx)
			{
				is >> x >> y >> rot >> group;
				Entity *e = dsq->game->createEntity(idx, 0, Vector(x,y), rot, true, "");
				e->setGroupID(group);
			}
		}
		if (entitiesNode->Attribute("g"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("g"));
			int idx, x, y, rot, group, id;
			while (is >> idx)
			{
				is >> x >> y >> rot >> group >> id;
				Entity *e = dsq->game->createEntity(idx, id, Vector(x,y), rot, true, "");
				e->setGroupID(group);
			}
		}
		if (entitiesNode->Attribute("h"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("h"));
			int idx, x, y, rot, groupID, id;
			Entity::NodeGroups *ng;
			Entity::NodeGroups nodeGroups;
			while (is >> idx)
			{
				int numNodeGroups = 0;
				is >> x >> y >> rot >> groupID >> id;
				is >> numNodeGroups;

				ng = 0;
				nodeGroups.clear();
				if (numNodeGroups > 0)
				{
					ng = &nodeGroups;
					for (int i = 0; i < numNodeGroups; i++)
					{
						int sz;
						is >> sz;
						for (int j = 0; j < sz; j++)
						{
							int idx;
							is >> idx;
							if (idx >= 0 && idx < getNumPaths())
							{
								nodeGroups[i].push_back(getPath(idx));
							}
						}
					}
				}

				dsq->game->createEntity(idx, id, Vector(x,y), rot, true, "", ET_ENEMY, ng, groupID);
				// setting group ID
			}
		}
		if (entitiesNode->Attribute("i"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("i"));
			int idx, x, y, rot, groupID, id;
			Entity::NodeGroups nodeGroups;
			while (is >> idx)
			{
				is >> x >> y >> rot >> groupID >> id;

				dsq->game->createEntity(idx, id, Vector(x,y), rot, true, "", ET_ENEMY, 0, groupID);
				// setting group ID
			}
		}
		if (entitiesNode->Attribute("j"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("j"));
			int idx, x, y, rot, groupID, id;
			std::string name;
			Entity::NodeGroups nodeGroups;
			while (is >> idx)
			{
				name="";
				if (idx == -1)
					is >> name;
				is >> x >> y >> rot >> groupID >> id;

				if (!name.empty())
					dsq->game->createEntity(name, id, Vector(x,y), rot, true, "", ET_ENEMY, 0, groupID);
				else
					dsq->game->createEntity(idx, id, Vector(x,y), rot, true, "", ET_ENEMY, 0, groupID);
				// setting group ID
			}
		}
		entitiesNode = entitiesNode->NextSiblingElement("Entities");
	}
	//assignEntitiesUniqueIDs();
	//initEntities();
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		e->onSceneFlipped();
	}
	this->reconstructGrid(true);
	rebuildElementUpdateList();
	setElementLayerFlags();

	// HACK: Don't try to optimize the barrier layer in Mithalas Cathedral
	// since elements are turned off dynamically.
	if (nocasecmp(scene, "cathedral02") == 0)
		dsq->getRenderObjectLayer(LR_ELEMENTS3)->setOptimizeStatic(false);

	findMaxCameraValues();

	endProgress();

	return true;
}

void Game::setMusicToPlay(const std::string &m)
{
	musicToPlay = m;
	stringToLower(musicToPlay);
}

void Game::findMaxCameraValues()
{
	cameraMin.x = 20;
	cameraMin.y = 20;
	cameraMax.x = -1;
	cameraMax.y = -1;
	int i = 0;
	for (i = 0; i < obsRows.size(); i++)
	{
		ObsRow *r = &obsRows[i];
		TileVector t(r->tx + r->len, r->ty);
		Vector v = t.worldVector();
		if (v.x > cameraMax.x)
		{
			cameraMax.x = v.x;
		}
		if (v.y > cameraMax.y)
		{
			cameraMax.y = v.y;
		}
	}
	/*
	for (i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		if (e->position.x > cameraMax.x)
			cameraMax.x = e->position.x;
		if (e->position.y > cameraMax.y)
			cameraMax.y = e->position.y;
	}
	*/
	if (backdropQuad)
	{
		if (backdropQuad->getWidth() > cameraMax.x)
		{
			cameraMax.x = backdropQuad->getWidth();
		}
		if (backdropQuad->getHeight() > cameraMax.y)
		{
			cameraMax.y = backdropQuad->getHeight();
		}
	}
}

void Game::setWarpAreaSceneName(WarpArea &warpArea)
{
	InStream in("data/warpAreas.txt");
	std::string color, area1, dir1, area2, dir2;
	std::string line;
	while (std::getline(in, line))
	{

		std::istringstream is(line);
		is >> color >> area1 >> dir1 >> area2 >> dir2;
		/*
		errorLog (color + " : " + area1 + " : " + dir1 + " : " + area2 + " : " + dir2);
		*/
		if (area2 == dsq->game->sceneName && warpArea.warpAreaType == color)
		{
			area2 = area1;
			dir2 = dir1;
			area1 = dsq->game->sceneName;
		}
		if (area1 == dsq->game->sceneName && warpArea.warpAreaType == color)
		{
			if (dir2=="Left")
				warpArea.spawnOffset = Vector(-1,0);
			else if (dir2=="Right")
				warpArea.spawnOffset = Vector(1,0);
			else if (dir2=="Up")
				warpArea.spawnOffset = Vector(0,-1);
			else if (dir2=="Down")
				warpArea.spawnOffset = Vector(0,1);
			warpArea.sceneName = area2;
			break;
		}
	}
	if (warpArea.sceneName.empty())
	{
		errorLog(warpArea.warpAreaType + " WarpArea for " + dsq->game->sceneName + " not found");
	}
}

Entity *Game::getEntityInGroup(int gid, int iter)
{
	int c = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->getGroupID() == gid)
		{
			if (iter == c)
				return e;
			c++;
		}
	}
	return 0;
}

bool Game::loadScene(std::string scene)
{
	stringToLower(scene);

	sceneName = scene;
	if (scene.empty())
	{
		return false;
	}

#ifdef AQUARIA_DEMO
	int i = 0;
	for (; i < allowedMaps.size(); i++)
	{
		if (allowedMaps[i] == scene)
			break;
	}
	if (i == allowedMaps.size())
	{
		exit(-1);
	}
#endif


	loadingScene = true;
	bool ret = loadSceneXML(scene);
	loadingScene = false;

	return ret;

	/*
	std::string fn = ("data/maps/"+scene+".xml");
	if (!exists(fn))
	{
		loadSceneDAT(scene);
		return;
	}
	loadSceneXML(scene);
	*/


	/*

	*/
}

void Game::saveScene(std::string scene)
{
	if (!this->saveFile)
		return;

	std::string fn = getSceneFilename(scene);

	TiXmlDocument saveFile(*this->saveFile);
	//this->saveFile->CopyTo(&saveFile);
	TiXmlElement *level = saveFile.FirstChildElement("Level");

	TiXmlElement levelLocal("Level");
	bool addIt = false;
	if (!level)
	{
		level = &levelLocal;
		addIt = true;
	}

	if (level)
	{
		level->SetAttribute("waterLevel", dsq->game->saveWaterLevel);

		if (grad)
		{
			level->SetAttribute("gradient", 1);

			std::ostringstream os;
			os << gradTop.x << " " << gradTop.y << " " << gradTop.z;
			level->SetAttribute("gradTop", os.str());

			std::ostringstream os2;
			os2 << gradBtm.x << " " << gradBtm.y << " " << gradBtm.z;
			level->SetAttribute("gradBtm", os2.str());

		}

		if (!saveMusic.empty())
		{
			level->SetAttribute("music", saveMusic);
		}
	}

	if (addIt)
	{
		saveFile.InsertEndChild(levelLocal);
	}

	/*
	TiXmlElement level("Level");
	level.SetAttribute("elementTemplatePack", elementTemplatePack);
	if (bg)
	{
		int pos = bg->texture->name.find_last_of('/')+1;
		int pos2 = bg->texture->name.find_last_of('.');
		level.SetAttribute("bg", bg->texture->name.substr(pos, pos2-pos));
		std::ostringstream os;
		os << sceneColor.x << " " << sceneColor.y << " " << sceneColor.z;
		level.SetAttribute("sceneColor", os.str());
	}
	saveFile->InsertEndChild(level);
	*/

	std::ostringstream obs, obsBinary;
	int i = 0;
	for (i = 0; i < obsRows.size(); i++)
	{
		obs << obsRows[i].tx << " " << obsRows[i].ty << " " << obsRows[i].len << " ";
	}
	TiXmlElement obsXml("Obs");
	obsXml.SetAttribute("d", obs.str());
	saveFile.InsertEndChild(obsXml);


	for (i = 0; i < dsq->game->getNumPaths(); i++)
	{
		TiXmlElement pathXml("Path");
		Path *p = dsq->game->getPath(i);
		pathXml.SetAttribute("name", p->name);
		//pathXml.SetAttribute("active", p->active);
		for (int n = 0; n < p->nodes.size(); n++)
		{
			TiXmlElement nodeXml("Node");
			std::ostringstream os;
			os << int(p->nodes[n].position.x) << " " << int(p->nodes[n].position.y);
			nodeXml.SetAttribute("pos", os.str().c_str());
			std::ostringstream os2;
			os2 << p->rect.getWidth() << " " << p->rect.getHeight();
			nodeXml.SetAttribute("rect", os2.str().c_str());
			nodeXml.SetAttribute("shape", (int)p->pathShape);
			if (p->nodes[n].maxSpeed != -1)
			{
				nodeXml.SetAttribute("ms", p->nodes[n].maxSpeed);
			}
			pathXml.InsertEndChild(nodeXml);
		}
		saveFile.InsertEndChild(pathXml);
	}

	for (i = 0; i < dsq->game->warpAreas.size(); i++)
	{
		WarpArea a = dsq->game->warpAreas[i];
		TiXmlElement waSF("WarpArea");
		waSF.SetAttribute("x", a.position.x);
		waSF.SetAttribute("y", a.position.y);
		if (a.radius > 0)
			waSF.SetAttribute("radius", a.radius);
		else if (a.w > 0 && a.h > 0)
		{
			waSF.SetAttribute("w", a.w);
			waSF.SetAttribute("h", a.h);
		}
		if (a.generated)
		{
			waSF.SetAttribute("g", 1);
		}
		std::ostringstream os;
		os << a.sceneName << " " << a.warpAreaType << " " << a.spawnOffset.x << " " << a.spawnOffset.y;
		waSF.SetAttribute("scene", os.str().c_str());

		saveFile.InsertEndChild(waSF);
	}

	std::ostringstream simpleElements[LR_MAX];


	for (i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		simpleElements[e->bgLayer] << e->templateIdx << " " << int(e->position.x) << " " << int(e->position.y) << " " << int(e->rotation.z) << " " << e->scale.x << " " << e->scale.y << " " << int(e->isfh()) << " " << int(e->isfv()) << " " << e->elementFlag << " " << e->getElementEffectIndex() << " " << e->isRepeatingTextureToFill() << " ";
	}

	if (dsq->game->entitySaveData.size() > 0)
	{
		TiXmlElement entitiesNode("Entities");

		std::ostringstream os;
		for (int i = 0; i < dsq->game->entitySaveData.size(); i++)
		{
			EntitySaveData *e = &dsq->game->entitySaveData[i];
			os << e->idx << " ";

			if (e->idx == -1)
			{
				if (!e->name.empty())
					os << e->name << " ";
				else
					os << "INVALID" << " ";
			}

			os << e->x << " " << e->y << " " << e->rot << " " << e->group << " " << e->id << " ";
		}
		entitiesNode.SetAttribute("j", os.str());
		saveFile.InsertEndChild(entitiesNode);
	}

	for (i = 0; i < LR_MAX; i++)
	{
		std::string s = simpleElements[i].str();
		if (!s.empty())
		{
			TiXmlElement simpleElementsXML("SE");
			simpleElementsXML.SetAttribute("k", s.c_str());
			simpleElementsXML.SetAttribute("l", i);
			saveFile.InsertEndChild(simpleElementsXML);
		}
	}

	// HACK: fix this later (won't save light shafts)
	/*
	for (Core::RenderObjects::iterator i = core->renderObjects.begin(); i != core->renderObjects.end(); i++)
	{
		LightShaft *l = dynamic_cast<LightShaft*>(*i);
		if (l)
		{
			TiXmlElement lightShaft("LightShaft");
			lightShaft.SetAttribute("x", l->position.x);
			lightShaft.SetAttribute("y", l->position.y);
			std::ostringstream os;
			os << l->getDir().x;
			lightShaft.SetAttribute("dirx", os.str());
			std::ostringstream os2;
			os2 << l->getDir().y;
			lightShaft.SetAttribute("diry", os2.str());
			std::ostringstream os3;
			os3 << l->shaftWidth;
			lightShaft.SetAttribute("w", os3.str());

			//lightShaft.SetAttribute("dirx", int(l->getDir().x*1000));
			//lightShaft.SetAttribute("diry", int(l->getDir().y*1000));
			saveFile.InsertEndChild(lightShaft);
		}
	}
	*/

	saveFile.SaveFile(fn);
}

void Game::warpToArea(WarpArea *area)
{
	if (this->miniMapHint.scene == area->sceneName && this->miniMapHint.warpAreaType == area->warpAreaType)
	{
		miniMapHint.clear();
	}
	//positionToAvatar = area->avatarPosition;
	dsq->game->warpAreaType = area->warpAreaType;
	dsq->game->spawnOffset = area->spawnOffset;
	//dsq->game->warpAreaSide = area->;
	dsq->game->transitionToScene(area->sceneName);
}

void Game::createGradient()
{
	if (grad)
	{
		grad->safeKill();
		grad = 0;
	}
	if (!grad)
	{
		grad = new Gradient;
		{
			//grad->makeVertical(Vector(0.6, 0.75, 0.65), Vector(0.4, 0.6, 0.5));
			//grad->makeVertical(Vector(0.6, 0.8, 0.65), Vector(0.1, 0.2, 0.4));
			grad->makeVertical(gradTop, gradBtm);
			grad->autoWidth = AUTO_VIRTUALWIDTH;
			grad->autoHeight = AUTO_VIRTUALHEIGHT;
			//grad->scale = Vector(core->getVirtualWidth(), core->getVirtualHeight());
			grad->position = Vector(400,300,-4);
			grad->followCamera = 1;
			grad->alpha = 1;
			grad->toggleCull(false);
		}
		addRenderObject(grad, LR_BACKDROP);
		if (bg)
			bg->blendEnabled = true;
		if (bg2)
			bg2->blendEnabled = true;
	}
}

bool Game::isInGameMenu()
{
	return inGameMenu;
}

bool Game::isValidTarget(Entity *e, Entity *me)
{
	//(e->layer == LR_ENTITIES0 || e->layer == LR_ENTITIES || e->layer == LR_ENTITIES2)
	//&& true
	return (e != me && e->isNormalLayer() && e->isPresent() && e->getEntityType() == ET_ENEMY && e->isAvatarAttackTarget());
}

void Game::updateMiniMapHintPosition()
{
	miniMapHintPosition = Vector(0,0,0);
	for (int i = 0; i < warpAreas.size(); i++)
	{
		if (this->sceneName == miniMapHint.scene)
		{
			if (warpAreas[i].warpAreaType == miniMapHint.warpAreaType)
			{
				miniMapHintPosition = warpAreas[i].position;
			}
		}
		else
		{
			if (warpAreas[i].sceneName == miniMapHint.scene)
			{
				miniMapHintPosition = warpAreas[i].position;
			}
		}
	}
}

void Game::createPets()
{
	setActivePet(dsq->continuity.getFlag(FLAG_PET_ACTIVE));
}

Entity* Game::setActivePet(int flag)
{
	if (currentPet)
	{
		currentPet->safeKill();
		currentPet = 0;
	}

	dsq->continuity.setFlag(FLAG_PET_ACTIVE, flag);

	if (flag != 0)
	{

		int petv = flag - FLAG_PET_NAMESTART;

		PetData *p = dsq->continuity.getPetData(petv);
		if (p)
		{
			std::string name = p->namePart;

			Entity *e = createEntity("Pet_" + name, -1, avatar->position, 0, false, "");
			if (e)
			{
				currentPet = e;
				e->setState(Entity::STATE_FOLLOW, -1, true);
				e->postInit();
			}
		}
	}

	return currentPet;
}

void Game::createLi()
{
	int liFlag = dsq->continuity.getFlag(FLAG_LI);
	std::ostringstream os;
	os << "liFlag: " << liFlag;
	debugLog(os.str());

	if (liFlag == 100)
	{
		debugLog("Creating Li");
		li = createEntity("Li", 0, Vector(0,0), 0, false, "");
		//li->skeletalSprite.animate("idle");
	}
}

void Game::colorTest()
{
	// test element coloring
	// possibly useful for darker maps
	/*
	std::vector<QuadLight> quadLights;
	quadLights.push_back(QuadLight(Vector(400, 300), Vector(1, 0, 0), 2000));
	for (int i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		//e->color = Vector(rand()%100, rand()%100, rand()%100);
		for (int i = 0; i < quadLights.size(); i++)
		{
			QuadLight *q = &quadLights[i];
			Vector dist = e->position - q->position;
			if (dist.isLength2DIn(q->dist))
			{
				float fract = float(dist.getLength2D())/float(quadLights[i].dist);
				float amb = fract;
				fract = 1.0f - fract;
				e->color = Vector(1,1,1)*amb + q->color*fract;
			}
			else
			{
				e->color = Vector(1,1,0);
			}
		}
		//e->color.normalize2D();
	}
	*/
}

void Game::showImage(const std::string &gfx)
{
	if (!image)
	{
		//float t = lua_tonumber(L, 2);

		dsq->overlay->color = Vector(1,1,1);
		dsq->fade(1, 0.5);
		dsq->watch(0.5);

		image = new Quad;
		image->setTexture(gfx);
		image->position = Vector(400,300);
		image->setWidthHeight(800, 800);
		image->offset = Vector(0,100);
		image->alpha = 0;
		image->followCamera = 1;
		core->addRenderObject(image, LR_HUD);

		image->scale = Vector(1,1);
		image->scale.interpolateTo(Vector(1.1, 1.1), 12);

		image->alpha = 1;
		dsq->fade(0, 0.5);
	}
}

void Game::hideImage()
{
	if (image)
	{
		image->setLife(1);
		image->setDecayRate(1.0f/2.0f);
		image->fadeAlphaWithLife = 1;
	}

	image = 0;
	dsq->overlay->color = 0;
}

void Game::switchBgLoop(int v)
{
	if (v != lastBgSfxLoop)
	{
		if (dsq->loops.bg != BBGE_AUDIO_NOCHANNEL)
		{
			core->sound->fadeSfx(dsq->loops.bg, SFT_OUT, 0.5);
			dsq->loops.bg = BBGE_AUDIO_NOCHANNEL;
		}

		switch(v)
		{
		case 0:
			if (!bgSfxLoop.empty())
			{
			    PlaySfx sfx;
			    sfx.name = bgSfxLoop;
			    sfx.vol = bgSfxVol;
			    sfx.loops = -1;
				sfx.priority = 0.8;
				dsq->loops.bg = core->sound->playSfx(sfx);
			}
		break;
		case 1:
			if (!airSfxLoop.empty())
			{
			    PlaySfx sfx;
			    sfx.name = airSfxLoop;
			    sfx.vol = bgSfxVol;
			    sfx.loops = -1;
				sfx.priority = 0.8;
				dsq->loops.bg = core->sound->playSfx(sfx);
			}
		break;
		}
		lastBgSfxLoop = v;
	}
}

void Game::entityDied(Entity *eDead)
{
	Entity *e = 0;
	FOR_ENTITIES(i)
	{
		e = *i;
		if (e != eDead && e->isv(EV_ENTITYDIED,1))
		{
			e->entityDied(eDead);
		}
	}

	dsq->continuity.entityDied(eDead);
}

void Game::postInitEntities()
{
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e)
		{
			e->postInit();
		}
	}
	core->resetTimer();
}

void Game::updateParticlePause()
{
	if (this->isPaused())
	{
		core->particlesPaused = 2;
	}
	else if (dsq->continuity.getWorldType() == WT_SPIRIT)
	{
		core->particlesPaused = 1;
	}
	else
	{
		core->particlesPaused = 0;
	}
}

void Game::warpKey1()
{
	if (core->getCtrlState())
	{
		dsq->game->avatar->heal(1000);
		dsq->game->avatar->fhTo(true);
		warpToSceneNode("OPENWATER02", "WARPKEY");
	}
}

void Game::warpKey2()
{
	if (core->getCtrlState())
	{
		dsq->game->avatar->heal(1000);
		dsq->game->avatar->fhTo(true);
		warpToSceneNode("VEIL01", "WARPKEY");
	}
}

void Game::warpKey3()
{
	if (core->getCtrlState())
	{
		dsq->game->avatar->heal(1000);
		dsq->game->avatar->fhTo(true);
		warpToSceneNode("FOREST03", "WARPKEY");
	}
}

void Game::warpKey4()
{
	if (core->getCtrlState())
	{
		dsq->game->avatar->heal(1000);
		dsq->game->avatar->fhTo(true);
		warpToSceneNode("ABYSS01", "WARPKEY");
	}
}

int game_collideParticle(Vector pos)
{
	bool aboveWaterLine = (pos.y <= dsq->game->waterLevel.x+20);
	bool inWaterBubble = false;
	if (!aboveWaterLine)
	{
		Path *p = dsq->game->getNearestPath(pos, PATH_WATERBUBBLE);
		if (p)
		{
			if (p->isCoordinateInside(pos))
			{
				inWaterBubble = true;
			}
		}
	}
	if (!inWaterBubble && aboveWaterLine)
	{
		return 1;
	}

	TileVector t(pos);
	return dsq->game->isObstructed(t);
}

void game_wibbleParticle(Particle *p)
{
	/*
	if (dsq->game->avatar->getNotesOpen() > 0 && (p->position - dsq->game->avatar->position).isLength2DIn(256))
	{
		if (!p->offset.isInterpolating())
		{
			p->offset.interpolateTo(Vector(10,0), (8-dsq->game->avatar->getLastNote())/10.0f, -1, 1);

			//if (dsq->game->avatar->isSinging())
			//{
			//	p->influenceVariable = 1;
			//	p->color.interpolateTo(dsq->getNoteColor(dsq->game->avatar->getLastNote()), 1.0);
			//}

		}
		else
		{
		}
	}
	else
	{
		p->offset.stop();

		//if (p->influenceVariable)
		//	p->color.interpolateTo(Vector(1,1,1), 1);

	}
	*/
}

void Game::rebuildElementUpdateList()
{
	for (int i = LR_ELEMENTS1; i <= LR_ELEMENTS8; i++)
		dsq->getRenderObjectLayer(i)->update = false;

	elementUpdateList.clear();
	for (int i = 0; i < dsq->getNumElements(); i++)
	//for (int i = LR_ELEMENTS1; i <= LR_ELEMENTS8; i++)
	{
		//RenderObjectLayer *rl = dsq->getRenderObjectLayer(i);
		Element *e = dsq->getElement(i);
		if (e && e->layer >= LR_ELEMENTS1 && e->layer <= LR_ELEMENTS8)
		{
			if (e->getElementEffectIndex() != -1)
			{
				elementUpdateList.push_back(e);
			}
		}
	}
}

void Game::setElementLayerFlags()
{
	for (int i = LR_ELEMENTS1; i <= LR_ELEMENTS16; i++)
	{
		// FIXME: Background SchoolFish get added to ELEMENTS11, so
		// we can't optimize that layer.  (Maybe create a new layer?)
		if (i == LR_ELEMENTS11)
			continue;

		dsq->getRenderObjectLayer(i)->setOptimizeStatic(!isSceneEditorActive() && dsq->user.video.displaylists);
	}
}

float Game::getTimer(float mod)
{
	return timer*mod;
}

float Game::getHalf2WayTimer(float mod)
{
	float t=timer;
	if (t > 0.5f)
		t = 1 - t;
	return timer*2*mod;
}

float Game::getHalfTimer(float mod)
{
	return halfTimer*mod;
}

void Game::adjustFoodSlotCursor()
{
	// using visible slots now, don't need this atm
	return;
	/*
	for (int i = 0; i < foodSlots.size(); i++)
	{
		if (foodSlots[i]->isCursorIn())
		{
			if (!foodSlots[i]->getIngredient() || foodSlots[i]->getIngredient()->amount <= 0)
			{
				foodSlots[i]->setFocus(false);
				i--;
				while (i >= 0)
				{
					if (foodSlots[i]->getIngredient() && foodSlots[i]->getIngredient()->amount > 0)
					{
						//cursor->position = foodSlots[i]->getWorldPosition();
						foodSlots[i]->setFocus(true);
						break;
					}
					i--;
				}
				if (i <= -1)
				{
					menu[5]->setFocus(true);
					//cursor->position = menu[5]->getWorldPosition();
				}
			}
			break;
		}
	}
	*/
}

void Game::action(int id, int state)
{
	for (int i = 0; i < paths.size(); i++)
	{
		if (paths[i]->catchActions)
		{
			if (!paths[i]->action(id, state))
				break;
		}
	}

	if (id == ACTION_TOGGLEHELPSCREEN && !state)
	{
		onToggleHelpScreen();
		//toggleHelpScreen(!inHelpScreen);
	}
	if (id == ACTION_ESC && !state)					onPressEscape();
	if (id == ACTION_PRIMARY && !state)				onLeftMouseButton();
	if (id == ACTION_TOGGLEWORLDMAP && !state)
	{
		if (foodMenu)
		{
			recipes->setFocus(true);
			recipeMenu.toggle(!recipeMenu.on, true);
		}
		else if (!core->isStateJumpPending())
		{
			toggleWorldMap();
		}
	}

#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (id == ACTION_TOGGLESCENEEDITOR && !state)		toggleSceneEditor();
#endif

	if (dsq->isDeveloperKeys() || isSceneEditorActive())
	{
		if (id == ACTION_TOGGLEGRID && !state)			toggleGridRender();
	}

	if (isInGameMenu())
	{
		if (treasureMenu)
		{
			if (!state && !dsq->isNested())
			{
				if (dsq->menuSelectDelay == 0)
				{
					if (id == ACTION_PREVPAGE)
					{
						dsq->menuSelectDelay = MENUSELECTDELAY;
						onPrevTreasurePage();
						//menu[5]->setFocus(true);
					}
					if (id == ACTION_NEXTPAGE)
					{
						dsq->menuSelectDelay = MENUSELECTDELAY;
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
				if (dsq->menuSelectDelay == 0)
				{
					if (id == ACTION_PREVPAGE)
					{
						dsq->menuSelectDelay = MENUSELECTDELAY;
						if (recipeMenu.on)
							recipeMenu.goPrevPage();
						else
							onPrevFoodPage();
					}
					if (id == ACTION_NEXTPAGE)
					{
						dsq->menuSelectDelay = MENUSELECTDELAY;
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
						for (int i = 0; i < foodHolders.size(); i++)
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
						for (int i = 0; i < foodSlots.size(); i++)
						{
							if (foodSlots[i]->isCursorIn() && foodSlots[i]->getIngredient())
							{
								foodSlots[i]->moveRight();
								adjustFoodSlotCursor();
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
						for (int i = 0; i < foodHolders.size(); i++)
						{
							if (foodHolders[i]->alpha.x > 0 && foodHolders[i]->alphaMod > 0 && foodHolders[i]->isTrash())
							{
								trashIndex = i;
								break;
							}
						}
						if (trashIndex >= 0)
						{
							int ingrIndex = -1;
							for (int i = 0; i < foodSlots.size(); i++)
							{
								if (foodSlots[i]->isCursorIn() && foodSlots[i]->getIngredient())
								{
									ingrIndex = i;
									break;
								}
							}
							if (ingrIndex >= 0)
							{
								foodSlots[ingrIndex]->discard();
								adjustFoodSlotCursor();
							}
						}
					}
				}
			}
		}
	}
}

void Game::toggleWorldMap()
{
	if (worldMapRender)
	{
		worldMapRender->toggle(!worldMapRender->isOn());
	}
}

void Game::applyState()
{
	bool verbose = true;
	applyingState = true;	

	helpText = 0;
	helpUp = helpDown = 0;
	inHelpScreen = false;
	helpBG = 0;
	helpBG2 = 0;
	
	dsq->returnToScene = "";
	
	// new place where mods get stopped!
	// this lets recaching work
	// (presumably because there has been time for the garbage to be cleared)
	if (sceneToLoad == "title" && dsq->mod.isShuttingDown())
	{
		if (dsq->mod.isActive())
		{
			dsq->mod.stop();
			dsq->continuity.reset();
		}
	}

	dsq->collectScriptGarbage();

	isCooking = false;
	enqueuedPreviewRecipe = 0;

	dsq->toggleBlackBars(false);
	
	dsq->setTexturePointers();


	moveFoodSlotToFront = 0;

	cameraOffBounds = false;


	ingOffY = 0;
	ingOffYTimer = 0;

	AquariaGuiElement::canDirMoveGlobal = true;

	cookDelay = 0;

	dsq->toggleVersionLabel(false);

	activation = true;

	active = true;

	hasPlayedLow = false;

	firstSchoolFish = true;
	invincibleOnNested = true;


	controlHintNotes.clear();

	worldMapIndex = -1;

	particleManager->setNumSuckPositions(10);

	dropIngrNames.clear();

	foodMenu = optionsMenu = petMenu = treasureMenu = false;

	currentPet = 0;

	bgSfxLoopPlaying2 = "";
	lastBgSfxLoop = -1;
	saveMusic = "";

	timer = 0;
	halfTimer = 0;

	cameraFollowObject = 0;
	cameraFollowEntity = 0;

	shuttingDownGameState = false;
	core->particlesPaused = false;
	bNatural = false;
	songLineRender = 0;
	image = 0;

	core->particleManager->collideFunction = game_collideParticle;
	core->particleManager->specialFunction = game_wibbleParticle;

	controlHint_ignoreClear = false;
	inGameMenuExitState = 0;
	int i = 0;
	debugLog("Entering Game::applyState");
	dsq->overlay->alpha = 1;
	dsq->overlay->color = 0;


	for (i = LR_ELEMENTS1; i <= LR_ELEMENTS12; i++) // LR_ELEMENTS13 is darkness, stop before that
	{
		dsq->game->setElementLayerVisible(i-LR_ELEMENTS1, true);
	}
	
	dsq->applyParallaxUserSettings();
	
	controlHintTimer = 0;
	cameraConstrained = true;
	// reset parallax
	RenderObjectLayer *l = 0;
	for (i = LR_ELEMENTS10; i <= LR_ELEMENTS16; i++)
	{
		l = &dsq->renderObjectLayers[i];
		l->followCamera = 0;
		l->followCameraLock = 0;
	}

	cameraLerpDelay = 0;
	playingSongInMenu = -1;
	sceneColor2 = Vector(1,1,1);
	sceneColor3 = Vector(1,1,1);
	if (core->afterEffectManager)
	{
		core->afterEffectManager->clear();
		//core->afterEffectManager->addEffect(new RippleEffect());
	}
	Shot::shots.clear();
	backdropQuad = 0;
	clearObsRows();
	inGameMenu = false;
	sceneFlipped = false;
	useWaterLevel = false;
	waterLevel = saveWaterLevel = 0;
	//miniMapHintPosition = Vector(8900, 14520);
	currentInventoryPage = 0;

	dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = false;

	//dsq->getRenderObjectLayer(LR_ELEMENTS5)->update = false;

	backgroundImageRepeat = 1;
	grad = 0;
	maxZoom = -1;
	saveFile = 0;
	deathTimer = 0.9;
	runGameOverScript = false;
	paused = false;
	//sceneColor = Vector(0.75, 0.75, 0.8);
	sceneColor = Vector(1,1,1);
	sceneName = "";
	elementTemplatePack ="";
	clearGrid();
	clearPointers();
	SkeletalSprite::clearCache();


	StateObject::applyState();
	//core->enable2D(800);

	dsq->clearEntities();
	dsq->clearElements();
	elementWithMenu = 0;
	//dsq->gui.menu.clearEntries();


	progressBar = 0;

	/*
	progressBar = new AquariaProgressBar();
	{
		progressBar->position = Vector(400,300);
	}
	addRenderObject(progressBar, LR_PROGRESS);
	*/

	damageSprite = new Quad;
	{
		damageSprite->setTexture("damage");
		damageSprite->alpha = 0;
		damageSprite->autoWidth = AUTO_VIRTUALWIDTH;
		damageSprite->autoHeight = AUTO_VIRTUALHEIGHT;
		damageSprite->position = Vector(400,300);
		damageSprite->followCamera = true;
		damageSprite->scale.interpolateTo(Vector(1.1, 1.1), 0.75, -1, 1, 1);
	}
	addRenderObject(damageSprite, LR_DAMAGESPRITE);

	bg2 = new Quad;
	{
		bg2->position = Vector(400, 300, -3/*-0.09f*/);
		//bg2->color = Vector(0.9, 0.9, 0.9);
		bg2->setTexture("missingImage");
		bg2->setWidthHeight(900,600);
		//bg2->blendEnabled = false;
		bg2->followCamera =1;
		bg2->alpha = 0.8;
	}
	addRenderObject(bg2, LR_BACKGROUND);

	bg = new Quad;
	{
		bg->blendEnabled = false;
		bg->position = Vector(400, 300, -2/*-0.09f*/);
		//bg->color = Vector(0.9, 0.9, 0.9);
		bg->setTexture("missingImage");
		bg->setWidthHeight(900,600);
		//bg->blendEnabled = true;
		bg->followCamera =1;
		bg->alpha = 1;
	}
	addRenderObject(bg, LR_BACKGROUND);


	Vector mousePos(400,490);

	controlHint_bg = new Quad;
	{
		controlHint_bg->followCamera = 1;
		controlHint_bg->position = Vector(400,500);
		controlHint_bg->color = 0;
		controlHint_bg->alphaMod = 0.7;
		//controlHint_bg->setTexture("HintBox");
		controlHint_bg->setWidthHeight(core->getVirtualWidth(), 100);
		controlHint_bg->autoWidth = AUTO_VIRTUALWIDTH;
		controlHint_bg->alpha = 0;
	}
	addRenderObject(controlHint_bg, LR_HELP);

	controlHint_text = new BitmapText(&dsq->smallFont);
	{
		controlHint_text->alpha = 0;
		controlHint_text->setWidth(700);

		controlHint_text->setAlign(ALIGN_LEFT);
		controlHint_text->followCamera = 1;
		controlHint_text->scale = Vector(0.9, 0.9);
		//controlHint_text->setFontSize(14);
	}
	addRenderObject(controlHint_text, LR_HELP);

	controlHint_image = new Quad;
	{
		controlHint_image->followCamera = 1;
		controlHint_image->position = mousePos;
		controlHint_image->alpha = 0;
	}
	addRenderObject(controlHint_image, LR_HELP);

	controlHint_mouseLeft = new Quad;
	{
		controlHint_mouseLeft->followCamera = 1;
		controlHint_mouseLeft->setTexture("Mouse-LeftButton");
		controlHint_mouseLeft->position = mousePos;
		controlHint_mouseLeft->alpha = 0;
	}
	addRenderObject(controlHint_mouseLeft, LR_HELP);


	controlHint_mouseRight = new Quad;
	{
		controlHint_mouseRight->followCamera = 1;
		controlHint_mouseRight->setTexture("Mouse-RightButton");
		controlHint_mouseRight->position = mousePos;
		controlHint_mouseRight->alpha = 0;
	}
	addRenderObject(controlHint_mouseRight, LR_HELP);

	controlHint_mouseMiddle = new Quad;
	{
		controlHint_mouseMiddle->followCamera = 1;
		controlHint_mouseMiddle->setTexture("Mouse-MiddleButton");
		controlHint_mouseMiddle->position = mousePos;
		controlHint_mouseMiddle->alpha = 0;
	}
	addRenderObject(controlHint_mouseMiddle, LR_HELP);

	controlHint_mouseBody = new Quad;
	{
		controlHint_mouseBody->followCamera = 1;
		controlHint_mouseBody->setTexture("Mouse-Body");
		controlHint_mouseBody->position = mousePos;
		controlHint_mouseBody->alpha = 0;
	}
	addRenderObject(controlHint_mouseBody, LR_HELP);


	controlHint_shine = new Quad;
	{
		//controlHint_shine->setTexture("spiralglow");
		controlHint_shine->color = Vector(1,1,1);
		controlHint_shine->followCamera = 1;
		controlHint_shine->position = Vector(400,500);
		controlHint_shine->alphaMod = 0.3;
		controlHint_shine->setWidthHeight(core->getVirtualWidth(), 100);
		controlHint_shine->alpha = 0;
		controlHint_shine->setBlendType(RenderObject::BLEND_ADD);
	}
	addRenderObject(controlHint_shine, LR_HELP);

	li = 0;


#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (dsq->canOpenEditor())
	{
		sceneEditor.init();
	}
#endif

/*
	if (liFlag == 100)
	*/

	if (verbose) debugLog("Creating Avatar");
	avatar = new Avatar();
	if (verbose) debugLog("Done new Avatar");

	if (warpAreaType.empty())
	{
		if (positionToAvatar.x == 0 && positionToAvatar.y == 0)
			avatar->position = Vector(dsq->avStart.x,dsq->avStart.y);
		else
			avatar->position = positionToAvatar;
		positionToAvatar = Vector(0,0);
	}
	if (verbose) debugLog("Done warp");

	if (verbose) debugLog("Create Li");
	createLi();
	if (verbose) debugLog("Done");




	if (toFlip == 1)
	{
		dsq->game->avatar->flipHorizontal();
		toFlip = -1;
	}

	



	// li
	//if (true)


	if (verbose) debugLog("WarpKeys");



	if (verbose) debugLog("Done WarpKeys");

	bindInput();

	shapeDebug = 0;

	/*
	shapeDebug = new Quad;
	shapeDebug->setWidthHeight(80, 80);
	shapeDebug->color = Vector(1,1,0);
	addRenderObject(shapeDebug, LR_ENTITIES);
	*/



	if (verbose) debugLog("Loading Scene");
	if (!loadScene(sceneToLoad))
	{
		loadElementTemplates(elementTemplatePack);
	}
	if (verbose) debugLog("...Done");
	backupSceneColor = sceneColor;

	dsq->continuity.worldMap.revealMap(sceneName);

	colorTest();

	if (!warpAreaType.empty())
	{
		for (int i = 0; i < warpAreas.size(); i++)
		{
			WarpArea *a = &warpAreas[i];
			if (a->warpAreaType == warpAreaType)
			{
				int extra=96;
				if (a->radius)
					avatar->position = a->position + (spawnOffset*(a->radius+extra));
				else
				{
					Vector s(spawnOffset.x*(a->w+extra), spawnOffset.y*(a->h+extra));
					avatar->position = a->position + s;
				}
				break;
			}
		}
		warpAreaType = "";
	}


	if (verbose) debugLog("Adding Avatar");
	addRenderObject(avatar, LR_ENTITIES);
	//cameraFollow = &avatar->position;
	setCameraFollowEntity(avatar);
	if (verbose) debugLog("...Done");


	currentRender = new CurrentRender();
	addRenderObject(currentRender, LR_ELEMENTS3);

	steamRender = new SteamRender();
	addRenderObject(steamRender, LR_ELEMENTS9);

	songLineRender = new SongLineRender();
	addRenderObject(songLineRender, LR_HUD);

	gridRender = new GridRender(OT_INVISIBLE);
	addRenderObject(gridRender, LR_DEBUG_TEXT);
	gridRender->alpha = 0;

	gridRender2 = new GridRender(OT_HURT);
	addRenderObject(gridRender2, LR_DEBUG_TEXT);
	gridRender2->alpha = 0;

	gridRender3 = new GridRender(OT_INVISIBLEIN);
	addRenderObject(gridRender3, LR_DEBUG_TEXT);
	gridRender3->alpha = 0;

	edgeRender = new GridRender(OT_BLACKINVIS);
	addRenderObject(edgeRender, LR_DEBUG_TEXT);
	edgeRender->alpha = 0;

	gridRenderEnt = new GridRender(OT_INVISIBLEENT);
	addRenderObject(gridRenderEnt, LR_DEBUG_TEXT);
	gridRenderEnt->alpha = 0;

	waterSurfaceRender = new WaterSurfaceRender();
	//waterSurfaceRender->setRenderPass(-1);
	addRenderObject(waterSurfaceRender, LR_WATERSURFACE);

	GridRender *blackRender = new GridRender(OT_BLACK);
	//blackRender->alpha = 0;
	blackRender->blendEnabled = false;
	addRenderObject(blackRender, LR_ELEMENTS4);


	hudUnderlay = new Quad;
	hudUnderlay->color = 0;
	hudUnderlay->position = Vector(400,300);
	//hudUnderlay->scale = Vector(800, 600);
	hudUnderlay->autoWidth = AUTO_VIRTUALWIDTH;
	hudUnderlay->autoHeight = AUTO_VIRTUALHEIGHT;
	hudUnderlay->alpha = 0;
	hudUnderlay->followCamera = 1;
	addRenderObject(hudUnderlay, LR_HUDUNDERLAY);

	autoMap = 0;
	/*
	autoMap = new AutoMap;
	addRenderObject(autoMap, LR_MESSAGEBOX);
	*/

	miniMapRender = 0;

	//miniMapRender->position = Vector(400,300);

	miniMapRender = new MiniMapRender;

	//miniMapRender->position = Vector(740,540);
	// position = (vw,vh) - (scale*100)
	// set in minimaprender::onupdate
	miniMapRender->scale = Vector(0.55, 0.55);


	/*
	miniMapRender->position = Vector(750,550);
	miniMapRender->scale = Vector(0.5, 0.5);
	*/

	//miniMapRender->scale = Vector(8,8);
	addRenderObject(miniMapRender, LR_MINIMAP);

	timerText = new BitmapText(&dsq->smallFont);
	timerText->position = Vector(745, 550);
	timerText->alpha = 0;
	timerText->followCamera = 1;
	addRenderObject(timerText, LR_MINIMAP);

	worldMapRender = 0;

	worldMapRender = new WorldMapRender;
	addRenderObject(worldMapRender, LR_WORLDMAP);
	// to hide minimap
	//miniMapRender->position += Vector(800,0);

	sceneToLoad="";

	if (!fromScene.empty())
	{
		stringToLower(fromScene);
		debugLog("fromScene: " + fromScene + " fromWarpType: " + fromWarpType);
		float smallestDist = HUGE_VALF;
		Path *closest = 0;
		Vector closestPushOut;
		bool doFlip = false;
		for (int i = 0; i < dsq->game->getNumPaths(); i++)
		{
			Path *p = dsq->game->getPath(i);
			Vector pos = p->nodes[0].position;
			if (p && (nocasecmp(p->warpMap, fromScene)==0))
			{
				float dist = -1;
				bool go = false;
				Vector pushOut;
				switch(fromWarpType)
				{
				case CHAR_RIGHT:
					go = (p->warpType == CHAR_LEFT);
					pushOut = Vector(1,0);
					dist = fabsf(fromPosition.y - pos.y);
					doFlip = true;
				break;
				case CHAR_LEFT:
					go = (p->warpType == CHAR_RIGHT);
					pushOut = Vector(-1,0);
					dist = fabsf(fromPosition.y - pos.y);
				break;
				case CHAR_UP:
					go = (p->warpType == CHAR_DOWN);
					pushOut = Vector(0, -1);
					dist = fabsf(fromPosition.x - pos.x);
				break;
				case CHAR_DOWN:
					go = (p->warpType == CHAR_UP);
					pushOut = Vector(0, 1);
					dist = fabsf(fromPosition.x - pos.x);
				break;
				}
				if (go)
				{
					if (dist == -1)
					{
						debugLog(p->warpMap + ": warpType is wonky");
					}
					else if (dist < smallestDist)
					{
						smallestDist = dist;
						closest = p;
						closestPushOut = pushOut;
					}
				}
			}
		}
		if (closest)
		{
			debugLog("warping avatar to node: "  + closest->name);
			// this value of 8 is just nothing really
			// it short work with default value 1
			// just gives the player some room to move without heading straight back
			// into the warp
			// LOL to the above!!! :DDDDD
			avatar->position = closest->getEnterPosition(50);
			if (doFlip)
				avatar->flipHorizontal();
			/*
			avatar->position = closest->nodes[0].position;
			avatar->position += closestPushOut * 80;
			*/

			/*
			fromVel = Vector(0,1);
			avatar->rotateToVec(fromVel, 0.001);
			*/
		}
		else
		{
			debugLog("ERROR: Could not find a node to warp the player to!");
		}
		fromScene = "";
	}
	else if (!toNode.empty())
	{
		Path *p = dsq->game->getPathByName(toNode);
		if (p)
		{
			avatar->position = p->nodes[0].position;
		}
		toNode = "";
	}
	/*
	if (!fromVel.isZero())
	{
		//avatar->vel = fromVel;
		avatar->rotateToVec(fromVel, 0.001);
	}
	*/

	avatar->setWasUnderWater();
	if (!avatar->isUnderWater())
	{
		avatar->setMaxSpeed(dsq->v.maxOutOfWaterSpeed);
		avatar->currentMaxSpeed = dsq->v.maxOutOfWaterSpeed;
	}

	if (avatar->position.isZero() || avatar->position == Vector(1,1))
	{
		Path *p = 0;
		if ((p = getPathByName("NAIJASTART")) != 0 || (p = getPathByName("NAIJASTART L")) != 0)
		{
			avatar->position = p->nodes[0].position;
		}
		else if ((p = getPathByName("NAIJASTART R")) != 0)
		{
			avatar->position = p->nodes[0].position;
			avatar->flipHorizontal();
		}
	}


	//positionLi
	if (li)
	{
		li->position = avatar->position + Vector(8,8);
	}

	toNode = "";


	spawnSporeChildren();

	createInGameMenu();
	hideInGameMenu(false);

	core->cacheRender();

	core->cameraPos.stop();
	cameraInterp.stop();

	core->globalScale = dsq->continuity.zoom;
	avatar->myZoom = dsq->continuity.zoom;

	cameraInterp = getCameraPositionFor(avatar->position);
	core->cameraPos = getCameraPositionFor(avatar->position);

	core->sort();

	if (dsq->mod.isActive())
		dsq->runScript(dsq->mod.getPath() + "scripts/premap_" + sceneName + ".lua", "init", true);
	else
		dsq->runScript("scripts/maps/premap_"+sceneName+".lua", "init", true);

	std::string musicToPlay = this->musicToPlay;
	if (!overrideMusic.empty())
	{
		musicToPlay = overrideMusic;
	}

	//INFO: this used to be here to start fading out the music
	// before the level had begun
	/*
	if (dsq->sound->isPlayingMusic())
	{
		if (dsq->sound->currentMusic.find(musicToPlay) != std::string::npos)
		{
		}
		else
		{
			dsq->sound->fadeMusic(SFT_CROSS, 1);
		}
	}
	*/

	/*
	// HACK: to get the player on the map if there's an error with the warp coords
	while(dsq->game->collideCircleWithGrid(avatar->position, 32))
	{
		if (avatar->position.y < 200)
			avatar->position.y += 200;
		avatar->position += Vector(40,0);
		avatar->clampPosition();
	}
	*/

	updateMiniMapHintPosition();

	createPets();


	postInitEntities();


	/*
	core->sound->musicVolume(1.0, 0.5);
	core->sound->sfxVolume(1.0, 0.5);
	*/

	bool musicchanged = updateMusic();



	dsq->loops.bg = BBGE_AUDIO_NOCHANNEL;

	if (!bgSfxLoop.empty())
	{
		core->sound->loadLocalSound(bgSfxLoop);
	}

	if (!airSfxLoop.empty())
	{
		core->sound->loadLocalSound(airSfxLoop);
	}

	if (dsq->continuity.getWorldType() != WT_NORMAL)
		dsq->continuity.applyWorldEffects(dsq->continuity.getWorldType(), 0, musicchanged);


	if (verbose) debugLog("initAvatar");

	dsq->continuity.initAvatar(avatar);

	if (verbose) debugLog("Done initAvatar");


	if (verbose) debugLog("reset timer");
	core->resetTimer();

	if (verbose) debugLog("paths init");
	int pathSz = getNumPaths();
	for (i = 0; i < pathSz; i++)
	{
		getPath(i)->init();
	}

	debugLog("Updating bgSfxLoop");
	updateBgSfxLoop();

	// Must be _before_ the init script, since some init scripts run
	// cutscenes immediately.  --achurch
	dsq->subtitlePlayer.show(0.25);

	if (verbose) debugLog("loading map init script");
	if (dsq->mod.isActive())
		dsq->runScript(dsq->mod.getPath() + "scripts/map_" + sceneName + ".lua", "init", true);
	else
		dsq->runScript("scripts/maps/map_"+sceneName+".lua", "init", true);

	if (!dsq->doScreenTrans && (dsq->overlay->alpha != 0 && !dsq->overlay->alpha.isInterpolating()))
	{
		if (verbose) debugLog("fading in");
		debugLog("FADEIN");
		//dsq->overlay->alpha = 1;
		dsq->overlay->alpha.interpolateTo(0, 1);

		core->resetTimer();
		avatar->disableInput();
		core->main(0.5);
		avatar->enableInput();
		core->resetTimer();
	}

	if (dsq->doScreenTrans)
	{
		debugLog("SCREENTRANS!");
		core->resetTimer();
		dsq->toggleCursor(false, 0);
		dsq->doScreenTrans = false;

		dsq->transitionSaveSlots();
		dsq->overlay->alpha = 0;
		dsq->main(0.5);
		dsq->toggleCursor(true);
		dsq->tfader->alpha.interpolateTo(0, 0.2);
		dsq->main(0.21);
		dsq->clearSaveSlots(false);
	}

	if (verbose) debugLog("reset timer");

	applyingState = false;

	if (!dsq->doScreenTrans)
	{
		dsq->toggleCursor(true, 0.5);
	}
	
	dsq->forceInputGrabOff();

	debugLog("Game::applyState Done");
}

void Game::bindInput()
{
	if (!(this->applyingState || this->isActive())) return;

	ActionMapper::clearActions();
	//ActionMapper::clearCreatedEvents();


#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (dsq->canOpenEditor())
	{
		//addAction(MakeFunctionEvent(Game, toggleSceneEditor), KEY_TAB, 0);
		addAction(ACTION_TOGGLESCENEEDITOR, KEY_TAB);
	}
#endif



	/*
	if (dsq->user.demo.warpKeys)
	{
		addAction(MakeFunctionEvent(Game, warpKey1), KEY_1, 1);
		addAction(MakeFunctionEvent(Game, warpKey2), KEY_2, 1);
		addAction(MakeFunctionEvent(Game, warpKey3), KEY_3, 1);
		addAction(MakeFunctionEvent(Game, warpKey4), KEY_4, 1);
	}
	*/


	dsq->user.control.actionSet.importAction(this, "PrimaryAction", ACTION_PRIMARY);

	dsq->user.control.actionSet.importAction(this, "Escape",		ACTION_ESC);

	dsq->user.control.actionSet.importAction(this, "WorldMap",		ACTION_TOGGLEWORLDMAP);

	dsq->user.control.actionSet.importAction(this, "ToggleHelp",	ACTION_TOGGLEHELPSCREEN);

	// used for scrolling help text
	dsq->user.control.actionSet.importAction(this, "SwimUp",		ACTION_SWIMUP);
	dsq->user.control.actionSet.importAction(this, "SwimDown",		ACTION_SWIMDOWN);
	/*
	dsq->user.control.actionSet.importAction(this, "SwimLeft",		ACTION_SWIMLEFT);
	dsq->user.control.actionSet.importAction(this, "SwimRight",		ACTION_SWIMRIGHT);
	*/


	dsq->user.control.actionSet.importAction(this, "PrevPage",		ACTION_PREVPAGE);
	dsq->user.control.actionSet.importAction(this, "NextPage",		ACTION_NEXTPAGE);
	dsq->user.control.actionSet.importAction(this, "CookFood",		ACTION_COOKFOOD);
	dsq->user.control.actionSet.importAction(this, "FoodLeft",		ACTION_FOODLEFT);
	dsq->user.control.actionSet.importAction(this, "FoodRight",		ACTION_FOODRIGHT);
	dsq->user.control.actionSet.importAction(this, "FoodDrop",		ACTION_FOODDROP);

	if (dsq->canOpenEditor())
	{
		//addAction(MakeFunctionEvent(Game, toggleMiniMapRender), KEY_M, 0);
		addAction(ACTION_TOGGLEGRID, KEY_F9);
	}

	/*
	addAction(ACTION_MENULEFT,	KEY_LEFT);
	addAction(ACTION_MENURIGHT,	KEY_RIGHT);
	addAction(ACTION_MENUUP,	KEY_UP);
	addAction(ACTION_MENUDOWN,	KEY_DOWN);

	dsq->user.control.actionSet.importAction(this, "SwimLeft",		ACTION_MENULEFT);
	dsq->user.control.actionSet.importAction(this, "SwimRight",		ACTION_MENURIGHT);
	dsq->user.control.actionSet.importAction(this, "SwimUp",		ACTION_MENUUP);
	dsq->user.control.actionSet.importAction(this, "SwimDown",		ACTION_MENUDOWN);

	addAction(ACTION_MENULEFT,	JOY1_DPAD_LEFT);
	addAction(ACTION_MENURIGHT,	JOY1_DPAD_RIGHT);
	addAction(ACTION_MENUUP,	JOY1_DPAD_UP);
	addAction(ACTION_MENUDOWN,	JOY1_DPAD_DOWN);
	*/

	addAction(ACTION_MENULEFT,	JOY1_STICK_LEFT);
	addAction(ACTION_MENURIGHT,	JOY1_STICK_RIGHT);
	addAction(ACTION_MENUUP,	JOY1_STICK_UP);
	addAction(ACTION_MENUDOWN,	JOY1_STICK_DOWN);


	if (avatar)
		avatar->bindInput();

	if (worldMapRender)
		worldMapRender->bindInput();
}

bool ingType(const std::vector<IngredientData*> &list, IngredientType type, int amount=1)
{
	int c = 0;
	for (int i = 0; i < list.size(); i++)
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

bool ingName(const std::vector<IngredientData*> &list, const std::string &name, int amount=1)
{
	int c = 0;
	for (int i = 0; i < list.size(); i++)
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

const int numTreasures = 16*2;

void Game::onPrevTreasurePage()
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

void Game::onNextTreasurePage()
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

void Game::onPrevFoodPage()
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

void Game::onNextFoodPage()
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

void Game::onUseTreasure()
{
	debugLog("Use Treasure!");

	if (selectedTreasureFlag != -1)
	{
		dsq->runScriptNum("scripts/global/menu-treasures.lua", "useTreasure", selectedTreasureFlag);
	}
}

Recipe *Game::findRecipe(const std::vector<IngredientData*> &list)
{
	if (list.size() < 2) return 0;

	// there will be a number of types and a number of names
	// the types and names DO NOT overlap
	int rc = 0;
	Recipe *r = 0;
	Recipe *tr = 0;
	int q = 0, q2 = 0;
	for ( rc = 0; rc < dsq->continuity.recipes.size(); rc++)
	{
		for (int i = 0; i < list.size(); i++) list[i]->marked = 0;

		tr = 0;
		r = &dsq->continuity.recipes[rc];
		tr = r;
		q = 0;

		// get the amount of ingredients provided by the player
		int listAmount = list.size();

		// get the amount of ingredients required
		int recipeAmount = 0;

		for (int i = 0; i < r->types.size(); i++)
			recipeAmount += r->types[i].amount;

		for (int i = 0; i < r->names.size(); i++)
			recipeAmount += r->names[i].amount;

		if (listAmount != recipeAmount)
			continue;

		for (int c = 0; c < r->types.size(); c++)
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
			for (int c = 0; c < r->names.size(); c++)
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

	for (int i = 0; i < list.size(); i++) list[i]->marked = 0;

	if (rc == dsq->continuity.recipes.size())
	{
		/*
		data = dsq->continuity.getIngredientByName("SeaLoaf");
		if (data)
		{
			dsq->continuity.pickupIngredient(data);
		}
		*/
	}

	return 0;
}

void Game::updateCookList()
{
	cookList.clear();
	for (int i = 0; i < foodHolders.size(); i++)
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

void Game::onRecipes()
{
	if (foodMenu)
	{
		toggleRecipeList(!recipeMenu.on);
	}
}

void Game::onKeyConfig()
{
	dsq->screenTransition->capture();
	toggleKeyConfigMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

#define DEBUG_COOK

void Game::onCook()
{
	if (recipeMenu.on) return;
	if (cookDelay > 0) return;

	debugLog("Cook!");

	//std::vector<IngredientData*> list;
	updateCookList();

	if (cookList.size() < 2 || recipeMenu.on) return;

	AquariaGuiElement::canDirMoveGlobal = false;

	cookDelay = 0.4;

	bool cooked = false;

	isCooking = true;

	IngredientData *data=0;
	Recipe *r = findRecipe(cookList);

	if (r)
		data = dsq->continuity.getIngredientDataByName(r->result);
	else
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
			if (foodHolders[i]->alpha.x > 0 && !foodHolders[i]->isEmpty() && !foodHolders[i]->isTrash())
				foodHolders[i]->animateLid(true, longAnim);

		//dsq->main(0.2);

		
		if (longAnim)
		{
			float ft = 0.8;
			float nt = 0.1;
			float nt2 = 0.2;
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
			note1.name = getNoteName(0);
			PlaySfx note2;
			note2.name = getNoteName(4);
			PlaySfx note3;
			note3.name = getNoteName(3);

			handle = dsq->sound->playSfx(note1);
			dsq->main(nt2);
			dsq->sound->fadeSfx(handle, SFT_OUT, ft);
			dsq->main(nt);

			handle = dsq->sound->playSfx(note2);
			dsq->main(nt2);
			dsq->sound->fadeSfx(handle, SFT_OUT, ft);
			dsq->main(nt);

			handle = dsq->sound->playSfx(note3);
			dsq->main(nt2);
			dsq->sound->fadeSfx(handle, SFT_OUT, ft);
			dsq->main(nt);
		}

		dsq->sound->playSfx("boil");

		for (int i = 0; i < foodHolders.size(); i++)
		{
			if (!foodHolders[i]->isEmpty())
				dsq->spawnParticleEffect("cook-ingredient", foodHolders[i]->getWorldPosition(), 0, 0, LR_HUD3, 1);
		}

		if (longAnim)
			dsq->main(0.5);
		else
			dsq->main(0.2);

		bool haveLeftovers = true;
		for (int i = 0; i < foodHolders.size(); i++)
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
		for (int i = 0; i < foodHolders.size(); i++)
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

		for (int i = 0; i < foodHolders.size(); i++)
			if (foodHolders[i]->alpha.x > 0 && !foodHolders[i]->isTrash())
				foodHolders[i]->animateLid(false);

		dsq->spawnParticleEffect("cook-food", Vector(575,250), 0, 0, LR_HUD3, 1);

		if (longAnim)
			dsq->main(0.5);
		else
			dsq->main(0.2);

		if (data)
		{
			float t = 3;
			std::string n = "Ingredients/" + data->gfx;
			//Quad *e = new Quad();

			showRecipe->setTexture(n);
			showRecipe->scale = Vector(0.5, 0.5);
			showRecipe->scale.interpolateTo(Vector(1.2, 1.2), t);
			showRecipe->alpha.ensureData();
			showRecipe->alpha.data->path.clear();
			showRecipe->alpha.data->path.addPathNode(0, 0);
			showRecipe->alpha.data->path.addPathNode(1, 0.1);
			showRecipe->alpha.data->path.addPathNode(1, 0.6);
			showRecipe->alpha.data->path.addPathNode(0, 1);
			showRecipe->alpha.startPath(t);
		}

		dsq->continuity.pickupIngredient(data, 1);

		dsq->continuity.removeEmptyIngredients();

		dsq->main(0.5);

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
		dsq->centerMessage(dsq->continuity.stringBank.get(27));
	}
	refreshFoodSlots(true);

	AquariaGuiElement::canDirMoveGlobal = true;

	isCooking = false;
}

void Game::overrideZoom(float sz, float t)
{
	if (sz == 0)
	{
		dsq->game->toggleOverrideZoom(false);
	}
	else
	{
		dsq->game->toggleOverrideZoom(true);
		dsq->globalScale.interpolateTo(Vector(sz, sz), t);
	}
}

FoodSlot* getFoodSlotFromIndex()
{
	for (int i = 0; i < dsq->game->foodSlots.size(); i++)
	{
		if (dsq->game->foodSlots[i]->slot == FoodSlot::foodSlotIndex)
		{
			return dsq->game->foodSlots[i];
		}
	}
	return 0;
}

void Game::onLips()
{
	if (!foodMenu)
	{
		if (dsq->lastVoiceFile.find("NAIJA_SONG_") != std::string::npos)
		{
			dsq->stopVoice();
		}
	}
}

const float hintTransTime = 0.5;

void Game::clearControlHint()
{
	if (!controlHint_ignoreClear)
	{
		controlHintTimer = 0;

		if (controlHint_bg)
		{
			controlHint_mouseLeft->alpha.interpolateTo(0, hintTransTime);
			controlHint_mouseRight->alpha.interpolateTo(0, hintTransTime);
			controlHint_mouseMiddle->alpha.interpolateTo(0, hintTransTime);
			controlHint_mouseBody->alpha.interpolateTo(0, hintTransTime);
			controlHint_text->alpha.interpolateTo(0, hintTransTime);
			controlHint_bg->alpha.interpolateTo(0, hintTransTime);
			controlHint_image->alpha.interpolateTo(0, hintTransTime);
		}

		for (int i = 0; i < controlHintNotes.size(); i++)
		{
			controlHintNotes[i]->alpha.interpolateTo(0, hintTransTime);
		}
		controlHintNotes.clear();
	}
}

float Game::getWaterLevel()
{
	return waterLevel.x;
}

void Game::setControlHint(const std::string &h, bool left, bool right, bool middle, float time, std::string image, bool ignoreClear, int songType, float scale)
{
	if (!h.empty())
		dsq->sound->playSfx("controlhint");

	controlHint_ignoreClear = false;
	clearControlHint();
	std::string hint = h;

	controlHintTimer = time;

	if (core->flipMouseButtons)
	{
		if (h.find("Left")!=std::string::npos && h.find("Right")==std::string::npos)
		{
			std::string sought = "Left";
			std::string replacement = "Right";
			hint.replace(hint.find(sought), sought.size(), replacement);
		}
		else if (h.find("Left")==std::string::npos && h.find("Right")!=std::string::npos)
		{
			std::string sought = "Right";
			std::string replacement = "Left";
			hint.replace(hint.find(sought), sought.size(), replacement);
		}
		std::swap(left, right);
	}

	std::ostringstream os;
	os << "set control hint: (" << hint << ", " << left << ", " << right << ", " << middle << " t: " << time << " )";
	debugLog(os.str());

	if (songType > 0)
	{
		//Song *song = getSongByIndex(num);
		Song *song = dsq->continuity.getSongByIndex(songType);
		controlHintNotes.clear();

		Vector p = controlHint_mouseLeft->position + Vector(-100,0);

		os.seekp(0);
		os << "song/songslot-" << dsq->continuity.getSongSlotByType(songType) << '\0'; // ensure correct string termination across compilers
		Quad *q = new Quad(os.str(), p);
		q->followCamera = 1;
		q->scale = Vector(0.7, 0.7);
		q->alpha = 0;
		addRenderObject(q, controlHint_bg->layer);
		controlHintNotes.push_back(q);

		p += Vector(100, 0);

		for (int i = 0; i < song->notes.size(); i++)
		{
			int note = song->notes[i];

			os.seekp(0);
			os << "song/notebutton-" << note << '\0';
			Quad *q = new Quad(os.str(), p);
			q->color = dsq->getNoteColor(note)*0.5f + Vector(1, 1, 1)*0.5f;
			q->followCamera = 1;
			q->scale = Vector(1.0, 1.0);
			q->alpha = 0;

			if (i % 2)
				q->offset = Vector(0, -10);
			else
				q->offset = Vector(0, 10);

			addRenderObject(q, controlHint_bg->layer);

			controlHintNotes.push_back(q);

			p += Vector(40, 0);
		}
	}

	float alphaOn = 0.8, alphaOff = 0.5;
	controlHint_bg->alpha.interpolateTo(1, hintTransTime);
	controlHint_bg->scale = Vector(1,1);
	//controlHint_bg->scale = Vector(0,1);
	//controlHint_bg->scale.interpolateTo(Vector(1,1), hintTransTime);
	controlHint_text->setText(hint);
	controlHint_text->alpha.interpolateTo(1, hintTransTime);

	if (!image.empty())
	{
		controlHint_image->setTexture(image);
		controlHint_image->alpha.interpolateTo(1, hintTransTime);
		controlHint_image->scale = Vector(scale, scale);
		//controlHint_image->scale = Vector(0.5, 0.5);
	}
	else
	{
		controlHint_image->alpha.interpolateTo(0, hintTransTime);
	}

	
	controlHint_text->position.x = 400 - controlHint_text->getSetWidth()/2 + 25;
		//400 - controlHint_bg->getWidth()/2 + 25;
	controlHint_text->setAlign(ALIGN_LEFT);

	if (!left && !right && !middle)
	{
		controlHint_mouseRight->alpha.interpolateTo(0, hintTransTime);
		controlHint_mouseLeft->alpha.interpolateTo(0, hintTransTime);
		controlHint_mouseMiddle->alpha.interpolateTo(0, hintTransTime);
		if (image.empty() && controlHintNotes.empty())
		{
			controlHint_text->position.y = 470;
		}
		else
		{
			controlHint_text->position.y = 520;
			controlHint_text->position.x = 400;
			controlHint_text->setAlign(ALIGN_CENTER);
		}
	}
	else
	{
		controlHint_text->position.y = 520;
		controlHint_text->position.x = 400;
		controlHint_text->setAlign(ALIGN_CENTER);
		if (left)
			controlHint_mouseLeft->alpha.interpolateTo(alphaOn, hintTransTime);
		else
			controlHint_mouseLeft->alpha.interpolateTo(alphaOff, hintTransTime);

		if (right)
			controlHint_mouseRight->alpha.interpolateTo(alphaOn, hintTransTime);
		else
			controlHint_mouseRight->alpha.interpolateTo(alphaOff, hintTransTime);

		if (middle)
			controlHint_mouseMiddle->alpha.interpolateTo(alphaOn, hintTransTime);
		else
			controlHint_mouseMiddle->alpha.interpolateTo(alphaOff, hintTransTime);
		controlHint_mouseBody->alpha.interpolateTo(0.5, hintTransTime);
	}

	for (int i = 0; i < controlHintNotes.size(); i++)
	{
		controlHintNotes[i]->alpha.interpolateTo(alphaOn, hintTransTime);
	}

	controlHint_ignoreClear = ignoreClear;


	controlHint_shine->alpha.ensureData();
	controlHint_shine->alpha.data->path.clear();
	controlHint_shine->alpha.data->path.addPathNode(0.001, 0.0);
	controlHint_shine->alpha.data->path.addPathNode(1.000, 0.3);
	controlHint_shine->alpha.data->path.addPathNode(0.001, 1.0);
	controlHint_shine->alpha.startPath(0.4);
}

void Game::onFlipTest()
{
	flipSceneVertical(14960);
}

void appendFileToString(std::string &string, const std::string &file)
{
	InStream inf(file.c_str());

	if (inf.is_open())
	{
		while (!inf.eof())
		{
			std::string read;
			std::getline(inf, read);
#if BBGE_BUILD_UNIX
			read = stripEndlineForUnix(read);
#endif
			read = dsq->user.control.actionSet.insertInputIntoString(read);
			string += read + "\n";
		}
	}

	inf.close();
}

void Game::onToggleHelpScreen()
{
	if (inHelpScreen)
		toggleHelpScreen(false);
	else if (core->isStateJumpPending())
		return;
	else
	{
		if (worldMapRender->isOn())
		{
			toggleHelpScreen(true, "[World Map]");
		}
		else if (currentMenuPage == MENUPAGE_FOOD)
		{
			toggleHelpScreen(true, "[Food]");
		}
		else if (currentMenuPage == MENUPAGE_TREASURES)
		{
			toggleHelpScreen(true, "[Treasures]");
		}
		/*
		else if (currentMenuPage == MENUPAGE_SONGS)
		{
			toggleHelpScreen(true, "[Singing]");
		}
		*/
		else if (currentMenuPage == MENUPAGE_PETS)
		{
			toggleHelpScreen(true, "[Pets]");
		}
		else
		{
			toggleHelpScreen(true);
		}
	}
}

void Game::toggleHelpScreen(bool on, const std::string &label)
{
	if (dsq->game->isSceneEditorActive()) return;

	if (inHelpScreen == on) return;
	if (core->getShiftState()) return;
	if (dsq->screenTransition->isGoing()) return;
	if (dsq->isNested()) return;
	if (dsq->saveSlotMode != SSM_NONE) return;

	if (on)
	{
		AquariaGuiElement::currentGuiInputLevel = 100;
		dsq->screenTransition->capture();

		helpWasPaused = isPaused();
		togglePause(true);
		
		std::string data;

// These say "Mac" but we use them on Linux, too.
#if defined(BBGE_BUILD_UNIX)
		std::string fname = localisePath("data/help_header_mac.txt");
		appendFileToString(data, fname);
#else
		std::string fname = localisePath("data/help_header.txt");
		appendFileToString(data, fname);
#endif
		if (dsq->continuity.hasSong(SONG_BIND)) {
			fname = localisePath("data/help_bindsong.txt");
			appendFileToString(data, fname);
		}
		if (dsq->continuity.hasSong(SONG_ENERGYFORM)) {
			fname = localisePath("data/help_energyform.txt");
			appendFileToString(data, fname);
		}
		fname = localisePath("data/help_start.txt");
		appendFileToString(data, fname);
		
// These say "Mac" but we use them on Linux, too.
#if defined(BBGE_BUILD_UNIX)
		fname = localisePath("data/help_end_mac.txt");
		appendFileToString(data, fname);
#else
		fname = localisePath("data/help_end.txt");
		appendFileToString(data, fname);
#endif

		// !!! FIXME: this is such a hack.
		data += "\n\n" + dsq->continuity.stringBank.get(2032) + "\n\n";
		dsq->continuity.statsAndAchievements->appendStringData(data);

		helpBG = new Quad;
		//helpBG->color = 0;
		helpBG->setTexture("brick");
		helpBG->repeatTextureToFill(true);
		helpBG->repeatToFillScale = Vector(2, 2);
		//helpBG->alphaMod = 0.75;
		helpBG->autoWidth = AUTO_VIRTUALWIDTH;
		helpBG->autoHeight = AUTO_VIRTUALHEIGHT;
		helpBG->position = Vector(400,300);
		helpBG->followCamera = 1;
		addRenderObject(helpBG, LR_HELP);

		helpBG2 = new Quad;
		helpBG2->color = 0;
		helpBG2->alphaMod = 0.5;
		helpBG2->setWidth(620);
		helpBG2->autoHeight = AUTO_VIRTUALHEIGHT;
		helpBG2->position = Vector(400,300);
		helpBG2->followCamera = 1;
		addRenderObject(helpBG2, LR_HELP);

		helpText = new TTFText(&dsq->fontArialSmall);
		//test->setAlign(ALIGN_CENTER);
		helpText->setWidth(600);
		helpText->setText(data);
		//test->setAlign(ALIGN_CENTER);
		helpText->cull = false;
		helpText->followCamera = 1;
		helpText->position = Vector(100, 20);
		if (!label.empty())
		{
			int line = helpText->findLine(label);
			helpText->offset.interpolateTo(Vector(0, -helpText->getLineHeight()*line), -1200);
		}
		
		//helpText->offset.interpolateTo(Vector(0, -400), 4, -1, 1);
		//test->position = Vector(400,300);
		addRenderObject(helpText, LR_HELP);

		helpUp = new AquariaMenuItem;
		helpUp->useQuad("Gui/arrow-left");
		helpUp->useSound("click");
		helpUp->useGlow("particles/glow", 64, 32);
		helpUp->position = Vector(50, 40);
		helpUp->followCamera = 1;
		helpUp->rotation.z = 90;
		helpUp->event.set(MakeFunctionEvent(Game, onHelpUp));
		helpUp->scale = Vector(0.6, 0.6);
		helpUp->guiInputLevel = 100;
		addRenderObject(helpUp, LR_HELP);

		helpDown = new AquariaMenuItem;
		helpDown->useQuad("Gui/arrow-right");
		helpDown->useSound("click");
		helpDown->useGlow("particles/glow", 64, 32);
		helpDown->position = Vector(50, 600-40);
		helpDown->followCamera = 1;
		helpDown->rotation.z = 90;
		helpDown->event.set(MakeFunctionEvent(Game, onHelpDown));
		helpDown->scale = Vector(0.6, 0.6);
		helpDown->guiInputLevel = 100;
		addRenderObject(helpDown, LR_HELP);

		helpCancel = new AquariaMenuItem;
		helpCancel->useQuad("Gui/cancel");
		helpCancel->useSound("click");
		helpCancel->useGlow("particles/glow", 128, 40);
		helpCancel->position = Vector(750, 600-20);
		helpCancel->followCamera = 1;
		//helpCancel->rotation.z = 90;
		helpCancel->event.set(MakeFunctionEvent(Game, onToggleHelpScreen));
		helpCancel->scale = Vector(0.9, 0.9);
		helpCancel->guiInputLevel = 100;
		addRenderObject(helpCancel, LR_HELP);

		for (int i = 0; i < LR_HELP; i++)
		{
			core->getRenderObjectLayer(i)->visible = false;
		}
		
		core->resetTimer();

		dsq->screenTransition->transition(MENUPAGETRANSTIME);
	}
	else
	{
		if (!helpWasPaused)
			togglePause(false);
		dsq->screenTransition->capture();
		if (helpText)
		{
			helpText->alpha = 0;
			helpText->setLife(1);
			helpText->setDecayRate(1000);
			helpText = 0;
		}
		if (helpBG)
		{
			helpBG->alpha = 0;
			helpBG->setLife(1);
			helpBG->setDecayRate(1000);
			helpBG = 0;
		}
		if (helpBG2)
		{
			helpBG2->alpha = 0;
			helpBG2->setLife(1);
			helpBG2->setDecayRate(1000);
			helpBG2 = 0;
		}
		if (helpUp)
		{
			helpUp->alpha = 0;
			helpUp->setLife(1);
			helpUp->setDecayRate(1000);
			helpUp = 0;
		}
		if (helpDown)
		{
			helpDown->alpha = 0;
			helpDown->setLife(1);
			helpDown->setDecayRate(1000);
			helpDown = 0;
		}
		if (helpCancel)
		{
			helpCancel->alpha = 0;
			helpCancel->setLife(1);
			helpCancel->setDecayRate(1000);
			helpCancel = 0;
		}

		for (int i = 0; i < LR_HELP; i++)
		{
			core->getRenderObjectLayer(i)->visible = true;
		}
		dsq->applyParallaxUserSettings();

		dsq->screenTransition->transition(MENUPAGETRANSTIME);


		AquariaGuiElement::currentGuiInputLevel = 0;
	}

	inHelpScreen = on;
}
bool Game::updateMusic()
{
	std::string musicToPlay = this->musicToPlay;
	if (!overrideMusic.empty())
	{
		musicToPlay = overrideMusic;
	}

	if (!musicToPlay.empty())
	{
		if (musicToPlay == "none")
			core->sound->fadeMusic(SFT_OUT, 1);
		else
		{
			bool play = true;
			/*
			std::string originalFile = musicToPlay;
			std::string file = originalFile;

			if (!exists(file, false) && file.find('.') == std::string::npos)
			{
				file = originalFile + ".ogg";
				if (!exists(file, false))
				{
					file = originalFile + ".mp3";
					if (!exists(file, false))
					{
						errorLog ("music file not found [" + originalFile + "]");
						play = false;
					}
				}
			}
			*/
			if (play)
			{
				/*
				if (core->sound->isStreamingMusic())
					core->sound->crossfadeOutMusic(1);
				*/
				return core->sound->playMusic(musicToPlay, SLT_LOOP, SFT_CROSS, 1, SCT_ISNOTPLAYING);
			}
		}
	}
	else
	{
		core->sound->fadeMusic(SFT_OUT, 1);
	}
	return false;
}

void Game::onPressEscape()
{
	if (dsq->isInCutscene())
	{
		// do nothing (moved to dsq::
	}
	else
	{
		if (inHelpScreen)
		{
			toggleHelpScreen(false);
			return;
		}
		if (dsq->game->worldMapRender->isOn() && !dsq->isNested())
		{
			dsq->game->worldMapRender->toggle(false);
			return;
		}
		if (dsq->game->isInGameMenu())
		{
			if (!AquariaKeyConfig::waitingForInput)
			{
				if (dsq->game->menuOpenTimer > 0.5f)
				{
					if (optionsMenu || keyConfigMenu)
						onOptionsCancel();
					else
						hideInGameMenu();
				}
			}
			return;
		}

		if (!paused)
		{
			if (core->getNestedMains() == 1 && !core->isStateJumpPending())
				showInGameMenu();
		}
		else
		{

			if (autoMap)
			{
				if (paused && autoMap->isOn())
					autoMap->toggle(false);
			}
		}


		if ((dsq->saveSlotMode != SSM_NONE || dsq->inModSelector) && core->isNested())
		{
			dsq->selectedSaveSlot = 0;
			core->quitNestedMain();
		}
	}
}

void Game::onDebugSave()
{
	hideInGameMenu();
	clearControlHint();
	core->main(0.5);
	dsq->game->togglePause(true);
	dsq->doSaveSlotMenu(SSM_SAVE);
	dsq->game->togglePause(false);
	//dsq->continuity.saveFile(0);
}

void Game::onInGameMenuInventory()
{
}

void Game::onInGameMenuSpellBook()
{
}

void Game::onInGameMenuContinue()
{
	hideInGameMenu();
}

void Game::onInGameMenuOptions()
{
}

void Game::onInGameMenuSave()
{
}

void Game::onExitCheckYes()
{
	dsq->sound->stopAllVoice();
	dsq->toggleCursor(0, 0.25);
	dsq->title();
}

void Game::onExitCheckNo()
{
	hideInGameMenuExitCheck(true);
}

void Game::showInGameMenuExitCheck()
{
	recipeMenu.toggle(false);
	inGameMenuExitState = 1;
	eYes->alpha.interpolateTo(1, 0.2);
	eNo->alpha.interpolateTo(1, 0.2);
	eAre->alpha.interpolateTo(1, 0.2);

	eNo->setFocus(true);
}

void Game::hideInGameMenuExitCheck(bool refocus)
{
	inGameMenuExitState = 0;
	eYes->alpha.interpolateTo(0, 0.2);
	eNo->alpha.interpolateTo(0, 0.2);
	eAre->alpha.interpolateTo(0, 0.2);

	if (refocus)
		((AquariaMenuItem*)menu[1])->setFocus(true);
}

void Game::onInGameMenuExit()
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

void Game::toggleDamageSprite(bool on)
{
	if (on)
	{
		damageSprite->alphaMod = 1;
	}
	else
		damageSprite->alphaMod = 0;
}

void Game::togglePause(bool v)
{
	paused = v;
	if (paused)
	{
		dsq->cursorGlow->alpha		= 0;
		dsq->cursorBlinker->alpha	= 0;
		//dsq->overlay->alpha.interpolateTo(0.5, 0.5);
	}
	//core->particlesPaused = v;
	/*
	else
		dsq->overlay->alpha.interpolateTo(0, 0.5);
		*/
}

bool Game::isPaused()
{
	return paused;
}

void Game::playBurstSound(bool wallJump)
{
	int freqBase = 950;
	if (wallJump)
		freqBase += 100;
	sound->playSfx("Burst", 1, 0);//, (freqBase+rand()%25)/1000.0f);
	if (chance(50))
	{
		switch (dsq->continuity.form)
		{
		case FORM_BEAST:
			sound->playSfx("BeastBurst", (128+rand()%64)/256.0f, 0);//, (freqBase+rand()%25)/1000.0f);
		break;
		}
	}
}

bool Game::collideCircleVsCircle(Entity *a, Entity *b)
{
	return (a->position - b->position).isLength2DIn(a->collideRadius + b->collideRadius);
}

bool Game::collideHairVsCircle(Entity *a, int num, const Vector &pos2, int radius, float perc)
{
	if (perc == 0)
		perc = 1;
	bool c = false;
	if (a->hair)
	{
		if (a)
		{
			if (num == 0)
				num = a->hair->hairNodes.size();
			// HACK: minus 2
			for (int i = 0; i < num; i++)
			{
				// + a->hair->position
				c = ((a->hair->hairNodes[i].position) - pos2).isLength2DIn(a->hair->hairWidth*perc + radius);
				if (c)
				{
					return true;
				}
			}
		}
	}
	return c;
}

// NOTE THIS FUNCTION ASSUMES THAT IF A BONE ISN'T AT FULL ALPHA (1.0) IT IS DISABLED
Bone *Game::collideSkeletalVsCircle(Entity *skeletal, Entity *circle)
{
	Vector pos = circle->position;

	return collideSkeletalVsCircle(skeletal, pos, circle->collideRadius);
}

Bone *Game::collideSkeletalVsLine(Entity *skeletal, Vector start, Vector end, float radius)
{
	//float smallestDist = HUGE_VALF;
	Bone *closest = 0;
	for (int i = 0; i < skeletal->skeletalSprite.bones.size(); i++)
	{
		Bone *b = skeletal->skeletalSprite.bones[i];
		/*
		int checkRadius = sqr(radius+b->collisionMaskRadius);
		Vector bonePos = b->getWorldCollidePosition();
		float dist = (bonePos - pos).getSquaredLength2D();
		*/

		// MULTIPLE CIRCLES METHOD
		if (!b->collisionMask.empty() && b->alpha.x == 1 && b->renderQuad)
		{
			for (int i = 0; i < b->transformedCollisionMask.size(); i++)
			{
				if (isTouchingLine(start, end, b->transformedCollisionMask[i], radius+b->collideRadius))
				{
					closest = b;
					//smallestDist = dist;
					break;
				}
			}
		}
		if (closest != 0)
		{
			break;
		}
		/*
		// ONE CIRCLE PER BONE METHOD
		else if (b->collideRadius && b->alpha.x == 1)
		{
			if (dist < checkRadius)
			{
				if (dist < smallestDist)
				{
					closest = b;
					smallestDist = dist;
				}
			}
		}
		*/
	}
	return closest;
}

bool Game::collideCircleVsLine(RenderObject *r, Vector start, Vector end, float radius)
{
	bool collision = false;
	if (isTouchingLine(start, end, r->position, radius+r->collideRadius, &lastCollidePosition))
	{
		collision = true;
	}
	return collision;
}

bool Game::collideCircleVsLineAngle(RenderObject *r, float angle, float startLen, float endLen, float radius, Vector basePos)
{
	bool collision = false;
	float rads = MathFunctions::toRadians(angle);
	float sinv = sinf(rads);
	float cosv = cosf(rads);
	Vector start=Vector(sinv,cosv)*startLen + basePos;
	Vector end=Vector(sinv,cosv)*endLen + basePos;
	if (isTouchingLine(start, end, r->position, radius+r->collideRadius, &lastCollidePosition))
	{
		collision = true;
	}
	if (shapeDebug)
	{
		shapeDebug->position = end;
	}
	return collision;
}


Bone *Game::collideSkeletalVsCircle(Entity *skeletal, Vector pos, float radius)
{
	float smallestDist = HUGE_VALF;
	Bone *closest = 0;
	if (!(pos - skeletal->position).isLength2DIn(2000)) return 0;
	for (int i = 0; i < skeletal->skeletalSprite.bones.size(); i++)
	{
		Bone *b = skeletal->skeletalSprite.bones[i];

		if (b->alpha.x == 1 && b->renderQuad)
		{
			float checkRadius = sqr(radius+b->collisionMaskRadius);
			Vector bonePos = b->getWorldCollidePosition();
			float dist = (bonePos - pos).getSquaredLength2D();
			// BOUND RECT METHOD
			if (!b->collisionRects.empty())
			{
				for (int i = 0; i < b->collisionRects.size(); i++)
				{
					b->collisionRects[i].isCoordinateInside(pos, radius);
				}
			}
			// MULTIPLE CIRCLES METHOD
			else if (!b->collisionMask.empty())
			{
				if (dist < checkRadius)
				{
					for (int i = 0; i < b->transformedCollisionMask.size(); i++)
					{
						if ((b->transformedCollisionMask[i] - pos).isLength2DIn(radius+b->collideRadius*skeletal->scale.x))
						{
							closest = b;
							smallestDist = dist;
							lastCollideMaskIndex = i;
							break;
						}
					}
				}
			}
			// ONE CIRCLE PER BONE METHOD
			else if (b->collideRadius)
			{
				if (dist < sqr(radius+b->collideRadius))
				{
					if (dist < smallestDist)
					{
						closest = b;
						smallestDist = dist;
					}
				}
			}
		}
	}
	return closest;
}

void Game::preLocalWarp(LocalWarpType localWarpType)
{
	// won't work if you start the map inside a local warp area... but that doesn't happen much for collecting gems
	if (localWarpType == LOCALWARP_IN)
	{
		dsq->game->avatar->warpInLocal = dsq->game->avatar->position;
	}
	else if (localWarpType == LOCALWARP_OUT)
	{
		dsq->game->avatar->warpInLocal = Vector(0,0,0);
	}

	dsq->game->avatar->warpIn = !dsq->game->avatar->warpIn;
	
	dsq->screenTransition->capture();
	core->resetTimer();
}

void Game::postLocalWarp()
{
	if (dsq->game->li && dsq->continuity.hasLi())
		dsq->game->li->position = dsq->game->avatar->position;
	if (dsq->game->avatar->pullTarget)
		dsq->game->avatar->pullTarget->position = dsq->game->avatar->position;
	dsq->game->snapCam();
	dsq->screenTransition->transition(0.6);

}

void Game::registerSporeDrop(const Vector &pos, int t)
{
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if ((e->position - pos).isLength2DIn(1024))
		{
			e->sporesDropped(pos, t);
		}
	}
}

bool Game::isEntityCollideWithShot(Entity *e, Shot *shot)
{
	if (!shot->isHitEnts())
	{
		return false;
	}
	if (shot->shotData && shot->shotData->checkDamageTarget)
	{
		if (!e->isDamageTarget(shot->getDamageType()))
			return false;
	}
	if (e->getEntityType() == ET_ENEMY)
	{
		if (shot->firer != e)
		{
			if (shot->getDamageType() == DT_AVATAR_BITE)
			{
				Avatar::BittenEntities::iterator i;
				for (i = avatar->bittenEntities.begin(); i != avatar->bittenEntities.end(); i++)
				{
					if (e == (*i))
					{
						return false;
					}
				}
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	else if (e->getEntityType() == ET_AVATAR)
	{
		// this used to be stuff != ET_AVATAR.. but what else would do that
		return !dsq->game->isDamageTypeAvatar(shot->getDamageType()) && (!shot->firer || shot->firer->getEntityType() == ET_ENEMY);
	}
	else if (e->getEntityType() == ET_PET)
	{
		bool go = shot->firer != e;
		if (shot->firer && shot->firer->getEntityType() != ET_ENEMY)
			go = false;
		return go;
	}

	return true;
}

void Game::handleShotCollisions(Entity *e, bool hasShield)
{
	BBGE_PROF(Game_handleShotCollisions);
	bool isRegValid=true;
	for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); i++)
	{
		Shot *shot = *i;
		if (isEntityCollideWithShot(e, shot) && (!hasShield || (!shot->shotData || !shot->shotData->ignoreShield)))
		{
			Vector collidePoint = e->position+e->offset;
			if (e->getNumTargetPoints()>0)
			{
				collidePoint = e->getTargetPoint(0);
			}
			if ((collidePoint - shot->position).isLength2DIn(shot->collideRadius + e->collideRadius))
			{
				lastCollidePosition = shot->position;
				shot->hitEntity(e,0,isRegValid);
			}
		}
	}
}

bool Game::isDamageTypeAvatar(DamageType dt)
{
	return (dt >= DT_AVATAR && dt < DT_TOUCH);
}

bool Game::isDamageTypeEnemy(DamageType dt)
{
	return (dt >= DT_ENEMY && dt < DT_AVATAR);
}

void Game::handleShotCollisionsSkeletal(Entity *e)
{
	BBGE_PROF(Game_HSSKELETAL);
	for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); i++)
	{
		Shot *shot = *i;
		if (isEntityCollideWithShot(e, shot))
		{
			Bone *b = collideSkeletalVsCircle(e, shot->position, shot->collideRadius);
			if (b)
			{
				lastCollidePosition = shot->position;
				shot->hitEntity(e, b);
			}
		}
	}
}

void Game::handleShotCollisionsHair(Entity *e, int num)
{
	for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); i++)
	{
		Shot *shot = *i;
		if (isEntityCollideWithShot(e, shot))
		{
			bool b = collideHairVsCircle(e, num, shot->position, 8);
			if (b)
			{
				lastCollidePosition = shot->position;
				shot->hitEntity(e, 0);
			}
		}
	}
}

#ifdef AQUARIA_BUILD_SCENEEDITOR
void Game::toggleSceneEditor()
{
	if (!core->getAltState())
	{
		sceneEditor.toggle();
		setElementLayerFlags();
	}
}
#endif

void Game::toggleMiniMapRender()
{
	if (miniMapRender)
	{
		if (miniMapRender->alpha == 0)
			miniMapRender->alpha.interpolateTo(1, 0.1f);
		else if (!miniMapRender->alpha.isInterpolating())
			miniMapRender->alpha.interpolateTo(0, 0.1f);
	}
}


void Game::toggleMiniMapRender(int v)
{
	if (miniMapRender)
	{
		if (v == 0)
			miniMapRender->alpha.interpolateTo(0, 0.1f);
		else
			miniMapRender->alpha.interpolateTo(1, 0.1f);
	}
}

void Game::toggleGridRender()
{
	float t = 0;
	if (gridRender->alpha == 0)
	{
		gridRender->alpha.interpolateTo(0.5, t);
		gridRender2->alpha.interpolateTo(0.5, t);
		gridRender3->alpha.interpolateTo(0.5, t);
		edgeRender->alpha.interpolateTo(0.5, t);
		gridRenderEnt->alpha.interpolateTo(0.5, t);
	}
	else if (gridRender->alpha == 0.5)
	{
		gridRender->alpha.interpolateTo(0, t);
		gridRender2->alpha.interpolateTo(0, t);
		gridRender3->alpha.interpolateTo(0, t);
		edgeRender->alpha.interpolateTo(0, t);
		gridRenderEnt->alpha.interpolateTo(0, t);
	}
}

Vector Game::getCameraPositionFor(const Vector &pos)
{
	Vector dest = pos;

	Vector v;
	dest += v + Vector(-400/core->globalScale.x,-300/core->globalScale.y);
	dest.z = 0;

	return dest;
}

void Game::setParallaxTextureCoordinates(Quad *q, float speed)
{
	//int backgroundImageRepeat = 1.2;
	q->followCamera = 1;
	q->texture->repeat = true;

	float camx = (core->cameraPos.x/800.0f)*speed;
	float camy = -(core->cameraPos.y/600.0f)*speed;

	float camx1 = camx - float(backgroundImageRepeat)/2.0f;
	float camx2 = camx + float(backgroundImageRepeat)/2.0f;
	float camy1 = camy - float(backgroundImageRepeat)/2.0f;
	float camy2 = camy + float(backgroundImageRepeat)/2.0f;

	q->upperLeftTextureCoordinates = Vector(camx1*backgroundImageRepeat, camy1*backgroundImageRepeat);
	q->lowerRightTextureCoordinates = Vector(camx2*backgroundImageRepeat, camy2*backgroundImageRepeat);
}

void Game::setCameraFollow(Vector *position)
{
	cameraFollow = position;
	cameraFollowObject = 0;
	cameraFollowEntity = 0;
}

void Game::setCameraFollow(RenderObject *r)
{
	cameraFollow = &r->position;
	cameraFollowObject = r;
	cameraFollowEntity = 0;
}

void Game::setCameraFollowEntity(Entity *e)
{
	cameraFollow = &e->position;
	cameraFollowObject = 0;
	cameraFollowEntity = e;
}

void Game::updateCurrentVisuals(float dt)
{
	/*
	static float delay = 0;
	delay += dt;
	if (delay > 0.2f)
	{
		for (Path *p = dsq->game->getFirstPathOfType(PATH_CURRENT); p; p = p->nextOfType)
		{
			if (p->active)
			{
				for (int n = 1; n < p->nodes.size(); n++)
				{
					PathNode *node2 = &p->nodes[n];
					PathNode *node1 = &p->nodes[n-1];
					Vector dir = node2->position - node1->position;

					dir.setLength2D(p->currentMod);
					dsq->spawnBubble(node1->position, dir);
				}
			}

		}
		delay = 0;
	}
	*/

	/*
    if (dsq->game->getPath(i)->name == "CURRENT" && dsq->game->getPath(i)->nodes.size() >= 2)
	{
		Vector dir = dsq->game->getPath(i)->nodes[1].position - dsq->game->getPath(i)->nodes[0].position;
		dir.setLength2D(800);
		dsq->spawnBubble(dsq->game->getPath(i)->nodes[0].position, dir);
	}
	*/
}

void Game::onOptionsMenu()
{
	dsq->screenTransition->capture();
	toggleOptionsMenu(true);
	dsq->screenTransition->transition(MENUPAGETRANSTIME);
}

void Game::onOptionsSave()
{
	dsq->user.apply();

	if (dsq->user.video.resx != dsq->user_backup.video.resx
		|| dsq->user.video.resy != dsq->user_backup.video.resy
		|| dsq->user.video.bits != dsq->user_backup.video.bits
		|| dsq->user.video.full != dsq->user_backup.video.full
		|| dsq->user.video.vsync != dsq->user_backup.video.vsync)
	{
		dsq->resetGraphics(dsq->user.video.resx, dsq->user.video.resy, dsq->user.video.full);
		if (dsq->confirm("", "graphics", false, 10)) {
		} else {
			dsq->user.video.resx = dsq->user_backup.video.resx;
			dsq->user.video.resy = dsq->user_backup.video.resy;
			dsq->user.video.bits = dsq->user_backup.video.bits;
			dsq->user.video.full = dsq->user_backup.video.full;
			dsq->user.video.vsync = dsq->user_backup.video.vsync;

			dsq->user.apply();

			dsq->resetGraphics(dsq->user.video.resx, dsq->user.video.resy, dsq->user.video.full);
		}
	}

	/*
	if (dsq->user.video.ripples != dsq->user_backup.video.ripples)
	{
		if (dsq->user.video.ripples)
		{
			if (core->frameBuffer.isInited())
			{
				if (!core->afterEffectManager)
				{
					core->afterEffectManager = new AfterEffectManager(vars->afterEffectsXDivs, vars->afterEffectsYDivs);
					core->afterEffectManager->update(0.0);
					core->afterEffectManager->capture();
				}

				dsq->useFrameBuffer = 1;
			}
			else
			{
				dsq->useFrameBuffer = 0;
			}
		}
		else
		{
			if (core->afterEffectManager)
			{
				delete core->afterEffectManager;
				core->afterEffectManager = 0;
			}
		}
	}
	*/

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
			hideInGameMenu();
		}
		else
		{
			dsq->screenTransition->capture();
			toggleOptionsMenu(false);
			dsq->screenTransition->transition(MENUPAGETRANSTIME);
		}
	}
}

void Game::onOptionsCancel()
{
	if (!keyConfigMenu)
	{
		dsq->user = dsq->user_backup;
	}
	else
	{
		dsq->user.control.actionSet = dsq->user_bcontrol.control.actionSet;
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
			hideInGameMenu();
		}
		else
		{
			dsq->screenTransition->capture();
			toggleOptionsMenu(false);
			dsq->screenTransition->transition(MENUPAGETRANSTIME);
		}
	}
}

void Game::refreshFoodSlots(bool effects)
{
	for (int i = 0; i < foodSlots.size(); i++)
	{
		foodSlots[i]->refresh(effects);
	}
	adjustFoodSlotCursor();
}

void Game::refreshTreasureSlots()
{
	for (int i = 0; i < treasureSlots.size(); i++)
	{
		treasureSlots[i]->refresh();
	}
}

void Game::togglePetMenu(bool f)
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
		for (int i = 0; i < petSlots.size(); i++)
		{
			petSlots[i]->alpha = 1;
			bool has = dsq->continuity.getFlag(petSlots[i]->petFlag);
			if (has)
			{
				hasPet = true;
				/*
				for (int j = 0; j < petSlots.size(); j++)
				{
					if (j != i)
					{
						bool has = dsq->continuity.getFlag(petSlots[j]->petFlag);
						if (has)
						{
							if (i == 0 && j == 1)
							{

							}
						}
					}
				}
				*/
			}
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


		for (int i = 0; i < petTips.size(); i++)
		{
			/*
			if (hasPet && i == 0)
			{
				petTips[i]->alpha = 0;
			}
			else if (!hasPet && i == 1)
				petTips[i]->alpha = 0;
			else
				petTips[i]->alpha = 1;
			*/
			petTips[i]->alpha = 1;
		}

		liCrystal->alpha = 1;


		menu[7]->setFocus(true);


		doMenuSectionHighlight(2);

	}
	else if (!f && petMenu)
	{
		for (int i = 0; i < petSlots.size(); i++)
		{
			petSlots[i]->alpha = 0;
		}

		for (int i = 0; i < petTips.size(); i++)
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

void Game::toggleTreasureMenu(bool f)
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

		for (int i = 0; i < treasureTips.size(); i++)
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

		for (int i = 0; i < treasureTips.size(); i++)
			treasureTips[i]->alpha = 0;

		menu[0]->setDirMove(DIR_UP, 0);
		menu[5]->setDirMove(DIR_LEFT, 0);

		liCrystal->alpha = 0;

		circlePageNum->alpha = 0;
	}

	for (int i = 0; i < treasureSlots.size(); i++)
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

void Game::toggleRecipeList(bool on)
{
	recipeMenu.toggle(on, true);
}

void Game::toggleFoodMenu(bool f)
{
	if (optionsMenu)
		toggleOptionsMenu(false);
	if (petMenu)
		togglePetMenu(false);
	if (treasureMenu)
		toggleTreasureMenu(false);


	for (int i = 0; i < foodHolders.size(); i++)
	{
		if (f)
			foodHolders[i]->alpha = 1;
		else
			foodHolders[i]->alpha = 0;
	}

	if (f)
	{
		if (dsq->game->avatar)
		{
			Path *p=0;
			if (dsq->continuity.getFlag(FLAG_UPGRADE_WOK) > 0 
				|| ((p=dsq->game->getNearestPath(dsq->game->avatar->position, PATH_COOK))
				&& p->isCoordinateInside(dsq->game->avatar->position)))
			{
				//cook->alpha = 1;
				foodHolders[0]->alpha = 1;
			}
			else
			{
				foodHolders[0]->alpha = 0;
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

		for (int i = 0; i < foodTips.size(); i++)
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

		for (int i = 0; i < foodTips.size(); i++)
			foodTips[i]->alpha = 0;

		menu[5]->setDirMove(DIR_LEFT, 0);

		circlePageNum->alpha = 0;

		previewRecipe->alpha = 0;
	}

	for (int i = 0; i < foodSlots.size(); i++)
	{
		foodSlots[i]->toggle(f);
	}
}

void Game::doMenuSectionHighlight(int section)
{
	for (int i = 0; i < 4; i++)
		((AquariaMenuItem*)menu[(5+i)])->quad->alphaMod = 0.8;
		//menu[(5+i)]->offset = Vector(0,-2);
	((AquariaMenuItem*)menu[(5+section)])->quad->alphaMod = 1.0;
	menuIconGlow->position = menu[5+section]->position;
	/*
	for (int i = 0; i < 4; i++)
		menu[5+i]->color = Vector(0.5, 0.5, 0.5);

	menu[5+section]->color = Vector(1,1,1);
	*/
}

void Game::toggleMainMenu(bool f)
{
	const float t = 0;
	if (f)
	{
		currentMenuPage = MENUPAGE_SONGS;
		for (int i = 0; i < songSlots.size(); i++)
		{
			//songSlots[i]->alpha.interpolateTo(1, t);
			songSlots[i]->alphaMod = 1;
		}
		songBubbles->alpha.interpolateTo(1,t);
		energyIdol->alpha.interpolateTo(1,t);
		liCrystal->alpha.interpolateTo(1, t);
		for (int i = 0; i < songTips.size(); i++)
			songTips[i]->alpha = 1;
		menuBg2->alpha.interpolateTo(1, t);


		int sm=-900;
		SongSlot *ss=0;
		for (int i = 0; i < songSlots.size(); i++)
		{
			if (dsq->continuity.hasSong(dsq->continuity.getSongTypeBySlot(i)))
			{
				//if (songSlots[i]->alpha == 1 && songSlots[i]->renderQuad && songSlots[i]->alphaMod == 1)
				{
					Vector p = songSlots[i]->getWorldPosition();
					if (p.x > sm)
					{
						sm = p.x;
						ss = songSlots[i];
					}
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

		for (int i = 0; i < songSlots.size(); i++)
		{
			songSlots[i]->alphaMod = 0;
		}

		for (int i = 0; i < songTips.size(); i++)
			songTips[i]->alpha = 0;

		songBubbles->alpha.interpolateTo(0, t);
		energyIdol->alpha.interpolateTo(0,t);
		liCrystal->alpha.interpolateTo(0, t);
	}
}

void Game::toggleKeyConfigMenu(bool f)
{
	const float t = 0;
	playingSongInMenu = -1;
	

	if (f && !keyConfigMenu)
	{
		//dsq->screenTransition->capture();

		toggleOptionsMenu(false, false, true);

		// Prevent int i from "leaking out" due to Microsoft extension to for-scope
		{
			for (int i = 0; i <= 1; i++)
				menu[i]->alpha.interpolateTo(0, t);
			for (int i = 4; i <= 8; i++)
				menu[i]->alpha.interpolateTo(0, t);
		}

		toggleMainMenu(false);

		menuBg2->alpha.interpolateTo(0, t);

		keyConfigMenu = true;

		group_keyConfig->setHidden(false);
		group_keyConfig->alpha = 1;

		dsq->user_bcontrol = dsq->user;

		//group_keyConfig->children[group_keyConfig->children.size()-3]

		RenderObjectList::reverse_iterator i = group_keyConfig->children.rbegin();
		AquariaKeyConfig *upright0 = (AquariaKeyConfig*)(*i);
		i++;
		AquariaKeyConfig *upright = (AquariaKeyConfig*)(*i);
		i++; //i++;
		AquariaKeyConfig *upleft = (AquariaKeyConfig*)(*i);

		opt_cancel->setDirMove(DIR_UP, upright);
		upright->setDirMove(DIR_DOWN, opt_cancel);
		upright0->setDirMove(DIR_DOWN, opt_cancel);

		opt_save->setDirMove(DIR_UP, upleft);
		upleft->setDirMove(DIR_DOWN, opt_save);


		opt_cancel->alpha = 1;
		opt_save->alpha = 1;
		

		opt_save->position = opt_save_original + Vector(0, 120);
		opt_cancel->position = opt_cancel_original + Vector(0, 120);

		opt_cancel->setFocus(true);
		
		menuIconGlow->alpha = 0;

		//dsq->screenTransition->transition(MENUPAGETRANSTIME);
	}
	else if (!f)
	{
		keyConfigMenu = false;

		group_keyConfig->alpha = 0;
		group_keyConfig->setHidden(true);

		opt_cancel->alpha = 0;
		opt_save->alpha = 0;

		opt_save->position = opt_save_original;
		opt_cancel->position = opt_cancel_original;
		
		menuIconGlow->alpha = 1;
	}
}

void Game::toggleOptionsMenu(bool f, bool skipBackup, bool isKeyConfig)
{
	const float t = 0;
	playingSongInMenu = -1;

	if (f && !optionsMenu)
	{
		//menuBg->setTexture("gui/options-menu");
		//menuBg->setWidthHeight(1024, 1024);

		if (!isKeyConfig && !optionsOnly)
		{
			dsq->continuity.lastOptionsMenuPage = currentMenuPage;
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

		if (micInputCheck)
			micInputCheck->setValue(dsq->user.audio.micOn);

		if (resBox)
		{
			std::ostringstream os;
			os << core->width << "x" << core->height;
			if (!resBox->setSelectedItem(os.str()))
			{
				resBox->addItem(os.str());
				resBox->setSelectedItem(os.str());
			}
		}

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

		/*
		for (int i = 0; i < menu.size(); i++)
			menu[i]->alpha.interpolateTo(0, 0.2);
		*/
	}
	else if (!f && optionsMenu)
	{
		lips->alpha = 0;

		keyConfigButton->alpha = 0;
		
		options->alpha.interpolateTo(0, t);

		opt_cancel->alpha = 0;
		opt_save->alpha = 0;

		liCrystal->alpha = 0;

		/*
		// what does this do?
		if (optionsMenu)
			((AquariaMenuItem*)menu[4])->setFocus(true);
		*/
			//((AquariaMenuItem*)menu[5])->setFocus(true);

		optionsMenu = false;


	
		if (!optionsOnly)
		{
			for (int i = 0; i <= 1; i++)
				menu[i]->alpha.interpolateTo(1, t);
			for (int i = 4; i <= 9; i++)
			{
				menu[i]->alpha.interpolateTo(1, t);
			}

			//menu[9]->alpha = 1;

			if (!isKeyConfig)
			{
				switch(dsq->continuity.lastOptionsMenuPage)
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

			//((AquariaMenuItem*)menu[4])->setFocus(true);

			
			menuBg2->alpha.interpolateTo(1, t);
		}
		
		menuIconGlow->alpha = 1;
		
	}
}

float optsfxdly = 0;
void Game::updateOptionsMenu(float dt)
{
	if (optionsMenu)
	{
		dsq->user.audio.voxvol				= voxslider->getValue();
		dsq->user.audio.sfxvol				= sfxslider->getValue();
		dsq->user.audio.musvol				= musslider->getValue();

		if (micInputCheck)
			dsq->user.audio.micOn			= micInputCheck->getValue();

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

		if (resBox)
		{
			std::string s = resBox->getSelectedItemString();
			if (!s.empty())
			{
				int pos = s.find('x');
				std::istringstream is1(s.substr(0, pos));
				is1 >> dsq->user.video.resx;
				std::istringstream is2(s.substr(pos+1, s.size()-(pos+1)));
				is2 >> dsq->user.video.resy;
			}

		}

		/*
		dsq->user.audio.sfxvol = sfxslider->getValue();
		dsq->user.audio.musvol = musslider->getValue();
		*/

		optsfxdly += dt;
		if (sfxslider->hadInput())
		{
			dsq->sound->playSfx("denied");
		}
		else if (voxslider->hadInput())
		{
			if (!dsq->sound->isPlayingVoice())
				dsq->voice("naija_somethingfamiliar");
		}
		else if (optsfxdly > 0.6f)
		{
			optsfxdly = 0;
			if (sfxslider->isGrabbed())
			{
				dsq->sound->playSfx("denied");
				dsq->loops.updateVolume();
				if (dsq->game->avatar)
					dsq->game->avatar->updateHeartbeatSfx();
			}
			if (voxslider->isGrabbed())
			{
				if (!dsq->sound->isPlayingVoice())
				{
					dsq->voice("naija_somethingfamiliar");
				}
			}
		}

		/*
		std::ostringstream os;
		os << "musvol: " << dsq->user.audio.musvol;
		debugLog(os.str());
		*/

		dsq->user.apply();
	}
}

void Game::updateInGameMenu(float dt)
{
	if (isInGameMenu())
	{
		menuOpenTimer += dt;
		if (dt > 10)
			dt = 10;

		if (foodMenu)
		{
			if (dsq->inputMode == INPUT_JOYSTICK)
			{
				//debugLog("food menu, joystick");


				/*
				*/
			}

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
						/*
						std::ostringstream os;
						os << "MenuNote" << s[currentSongMenuNote];
						*/
						sound->playSfx(dsq->game->getNoteName(s.notes[currentSongMenuNote], "Menu"));

						float a = (s.notes[currentSongMenuNote]*2*PI)/8.0f;
						int sz = 110*menuBg->scale.x;
						Vector notePos(sinf(a)*sz,cosf(a)*sz);

						float t = 0.5;
						Quad *q = new Quad("particles/glow", Vector(400+237*menuBg->scale.x,300-52*menuBg->scale.x)+notePos);
						q->setBlendType(RenderObject::BLEND_ADD);
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
					else
					{

						/*
						if (playedDudNote)
							playingSongInMenu = -1;
						else
							playedDudNote = true;
						*/
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
	}
}


void Game::updateCursor(float dt)
{
	bool rotate = false;

	if (dsq->inputMode == INPUT_MOUSE)
	{
		dsq->cursor->offset.stop();
		dsq->cursor->offset = Vector(0,0);
		//debugLog("offset lerp stop in mouse!");
	}
	else if (dsq->inputMode == INPUT_JOYSTICK)
	{
		if (!dsq->game->isPaused() || dsq->game->isInGameMenu() || !dsq->game->avatar->isInputEnabled())
		{
			int offy = -60;
			if (dsq->game->isInGameMenu() || !dsq->game->avatar->isInputEnabled())
			{
				//cursor->setTexture("");
				offy = 0;
			}
			if (!dsq->cursor->offset.isInterpolating())
			{
				//debugLog("offset lerp!");
				dsq->cursor->offset = Vector(0, offy);
				dsq->cursor->offset.interpolateTo(Vector(0, offy-20), 0.4, -1, 1, 1);
			}
		}
		else
		{
			//debugLog("offset lerp stop in joystick!");
			dsq->cursor->offset.stop();
			dsq->cursor->offset = Vector(0,0);
		}
	}

	if (isSceneEditorActive() || dsq->game->isPaused() || (!avatar || !avatar->isInputEnabled()) ||
		(dsq->game->miniMapRender && dsq->game->miniMapRender->isCursorIn())
		)
	{
		dsq->setCursor(CURSOR_NORMAL);
		// Don't show the cursor in keyboard/joystick mode if it's not
		// already visible (this keeps the cursor from appearing for an
		// instant during map fadeout).
		if (dsq->inputMode == INPUT_MOUSE || isSceneEditorActive() || dsq->game->isPaused())
			dsq->cursor->alphaMod = 0.5;
		
		/*
		dsq->cursor->offset.stop();
		dsq->cursor->offset = Vector(0,0);
		*/
	}
	else if (avatar)
	{
		//Vector v = avatar->getVectorToCursorFromScreenCentre();
		if (dsq->inputMode == INPUT_JOYSTICK)// && !avatar->isSinging() && !dsq->game->isInGameMenu() && !dsq->game->isPaused())
		{
			dsq->cursor->alphaMod = 0;
			if (!avatar->isSinging())
				core->setMousePosition(core->center);
			if (!dsq->game->isPaused())
			{
				/*

				*/
			}

		}
		else
		{
			dsq->cursor->offset.stop();
			dsq->cursor->offset = Vector(0,0);

			dsq->cursor->alphaMod = 0.5;
		}
		Vector v = avatar->getVectorToCursor();
		if (avatar->looking)
		{
			dsq->setCursor(CURSOR_LOOK);
		}
		else if (avatar->isSinging())
		{
			dsq->setCursor(CURSOR_SING);
		}
		else if (dsq->game->isInGameMenu() || v.isLength2DIn(avatar->getStopDistance()) || (avatar->entityToActivate || avatar->pathToActivate))
		{
			dsq->setCursor(CURSOR_NORMAL);
		}
		else if (!v.isLength2DIn(avatar->getBurstDistance()) /*|| avatar->state.lockedToWall*/ /*|| avatar->bursting*/)
		{
			dsq->setCursor(CURSOR_BURST);
			rotate = true;
		}
		else
		{
			dsq->setCursor(CURSOR_SWIM);
			rotate = true;
		}
	}
	if (rotate)
	{
		if (avatar)
		{
			Vector vec = dsq->getGameCursorPosition() - avatar->position;
			float angle=0;
			MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), vec, angle);
			angle = 180-(360-angle);
			angle += 90;
			dsq->cursor->rotation.z = angle;
		}
	}
	else
	{
		dsq->cursor->rotation.z = 0;
	}
}

void Game::constrainCamera()
{
	cameraOffBounds = 0;
	if (cameraConstrained)
	{
		int vw2 = core->getVirtualOffX()*core->invGlobalScale;
		int vh2 = core->getVirtualOffY()*core->invGlobalScale;

		if (dsq->cameraPos.x - vw2 < (cameraMin.x+1))
		{
			dsq->cameraPos.x = (cameraMin.x+1) + vw2;
			cameraOffBounds = 1;
		}

		if (dsq->cameraPos.y <= (cameraMin.y+1))
		{
			dsq->cameraPos.y = (cameraMin.y+1) + vh2;
			cameraOffBounds = 1;
		}

		int scrw, scrh;
		scrw = core->getVirtualWidth()*core->invGlobalScale;
		scrh = 600*core->invGlobalScale;

		if (cameraMax.x != -1 && dsq->cameraPos.x + scrw >= cameraMax.x)
		{
			dsq->cameraPos.x = cameraMax.x - scrw;
			cameraOffBounds = 1;
		}

		if (cameraMax.y != -1 && dsq->cameraPos.y + scrh >= cameraMax.y)
		{
			dsq->cameraPos.y = cameraMax.y - scrh;
			cameraOffBounds = 1;
		}
	}
}

bool areEntitiesUnique()
{
	bool unique = true;
	int c = 0;

	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		FOR_ENTITIES(j)
		{
			Entity *e2 = *j;
			if (e != e2)
			{
				if (e->getID() == e2->getID())
				{
					std::ostringstream os;
					os << "found non-unique entity: " << e->getID() << " names, " << e->name << " and " << e2->name;
					debugLog(os.str());
					unique = false;
					c++;
				}
			}
		}
	}
	if (unique)
	{
		debugLog("Entity IDs are unique");
	}
	else
	{
		std::ostringstream os;
		os << c << " Entity IDs are NOT unique";
		debugLog(os.str());
	}
	return unique;
}

bool Game::isControlHint()
{
	return controlHint_bg->alpha.x != 0;
}

bool Game::trace(Vector start, Vector target)
{
	/*
	TileVector tstart(start);
	TileVector ttarget(target);
	*/
	int i = 0;
	Vector mov(target-start);
	Vector pos = start;
	//mov.normalize2D();
	//mov |= g;
	//mov |= 0.5;
	mov.setLength2D(TILE_SIZE*1);
	int c = 0;
	// 1024
	while (c < 2048*10)
	{
		pos += mov;


		if (dsq->game->isObstructed(TileVector(pos)))
			return false;


		//Vector diff(tstart.x - ttarget.x, tstart.y - ttarget.y);
		Vector diff = target - pos;
		if (diff.getSquaredLength2D() <= sqr(TILE_SIZE*2))
			//close enough!
			return true;

		Vector pl = mov.getPerpendicularLeft();
		Vector pr = mov.getPerpendicularRight();
		i = 1;
		for (i = 1; i <= 6; i++)
		{
			TileVector tl(pos + pl*i);//(start.x + pl.x*i, start.y + pl.y*i);
			TileVector tr(pos + pr*i);//(start.x + pr.x*i, start.y + pr.y*i);
			if (dsq->game->isObstructed(tl) || dsq->game->isObstructed(tr))
				return false;
		}

		c++;
	}
	return false;
}

const float bgLoopFadeTime = 1;
void Game::updateBgSfxLoop()
{
	if (!avatar) return;

	Path *p = getNearestPath(dsq->game->avatar->position, PATH_BGSFXLOOP);
	if (p && p->isCoordinateInside(dsq->game->avatar->position) && !p->content.empty())
	{
		if (bgSfxLoopPlaying2 != p->content)
		{
			if (dsq->loops.bg2 != BBGE_AUDIO_NOCHANNEL)
			{
				core->sound->fadeSfx(dsq->loops.bg2, SFT_OUT, bgLoopFadeTime);
				dsq->loops.bg2 = BBGE_AUDIO_NOCHANNEL;
				bgSfxLoopPlaying2 = "";
			}
			PlaySfx play;
			play.name = p->content;
			play.time = bgLoopFadeTime;
			play.fade = SFT_IN;
			play.vol = 1;
			play.loops = -1;
			play.priority = 0.7;
			dsq->loops.bg2 = core->sound->playSfx(play);
			bgSfxLoopPlaying2 = p->content;
		}
	}
	else
	{
		if (dsq->loops.bg2 != BBGE_AUDIO_NOCHANNEL)
		{
			core->sound->fadeSfx(dsq->loops.bg2, SFT_OUT, bgLoopFadeTime);
			dsq->loops.bg2 = BBGE_AUDIO_NOCHANNEL;
			bgSfxLoopPlaying2 = "";
		}
	}

	if (avatar->isUnderWater(avatar->getHeadPosition()))
	{
		dsq->game->switchBgLoop(0);
	}
	else
		dsq->game->switchBgLoop(1);
}

const float helpTextScrollSpeed = 800.0f;
const float helpTextScrollClickAmount = 340.0f;
const float helpTextScrollClickTime = -helpTextScrollSpeed;
void Game::onHelpDown()
{	
	float to = helpText->offset.y - helpTextScrollClickAmount;
	if (to < -helpText->getFullHeight() + core->getVirtualHeight())
	{
		to = -helpText->getFullHeight() + core->getVirtualHeight();
	}
	helpText->offset.interpolateTo(Vector(0, to), helpTextScrollClickTime);
}

void Game::onHelpUp()
{
	float to = helpText->offset.y + helpTextScrollClickAmount;
	if (to > 0)
	{
		to = 0;
	}
	helpText->offset.interpolateTo(Vector(0, to), helpTextScrollClickTime);
}

void Game::update(float dt)
{
	particleManager->clearInfluences();

	if (inHelpScreen)
	{
		const float helpTextScrollSpeed = 400.0f;
		if (isActing(ACTION_SWIMDOWN))
		{
			helpText->offset.stop();
			helpText->offset.y -= helpTextScrollSpeed * dt;
			if (helpText->offset.y < -helpText->getFullHeight() + core->getVirtualHeight())
			{
				helpText->offset.y = -helpText->getFullHeight() + core->getVirtualHeight();
			}
		}
		if (isActing(ACTION_SWIMUP))
		{
			helpText->offset.stop();
			helpText->offset.y += helpTextScrollSpeed * dt;
			if (helpText->offset.y > 0)
			{
				helpText->offset.y = 0;
			}
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

	if (ingOffYTimer > 0)
	{
		ingOffYTimer -= dt;
		if (ingOffYTimer < 0)
		{
			ingOffYTimer = 0;
			ingOffY = 0;
		}
	}

	if (cookDelay > 0)
	{
		cookDelay -= dt;
		if (cookDelay < 0)
			cookDelay = 0;
	}

	if (avatar)
	{
		tintColor.update(dt);
		if (core->afterEffectManager)
		{
			if (tintColor.isInterpolating())
				core->afterEffectManager->setActiveShader(AS_GLOW);
			else
				core->afterEffectManager->setActiveShader(AS_NONE);

			core->afterEffectManager->glowShader.setValue(tintColor.x, tintColor.y, tintColor.z, 1);
		}

		if (avatar->isRolling())
			particleManager->addInfluence(ParticleInfluence(avatar->position, 300, 800, true));
		else if (avatar->isCharging())
			particleManager->addInfluence(ParticleInfluence(avatar->position, 100, 600, true));
		else if (avatar->bursting)
			particleManager->addInfluence(ParticleInfluence(avatar->position, 400, 200, true));
		else
			particleManager->addInfluence(ParticleInfluence(avatar->position, avatar->vel.getLength2D(), 24, false));


		particleManager->setSuckPosition(0, avatar->position);
		particleManager->setSuckPosition(1, avatar->position + avatar->vel + avatar->vel2);

		//avatar->vel.getLength2D()
		/*
		for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); i++)
		{
			Emitter::addInfluence(ParticleInfluence((*i)->position, 400, 128, true));
		}
		*/
		/*
		FOR_ENTITIES (i)
		{
			Entity *e = *i;
			if (e->getEntityType() != ET_AVATAR && e->collideRadius > 0)
			{
				Emitter::addInfluence(ParticleInfluence(e->position, 200, e->collideRadius, false));
			}
		}
		*/
	}
	updateParticlePause();
	//areEntitiesUnique();
	if (controlHintTimer > 0)
	{
		controlHintTimer -= dt;
		if (controlHintTimer < 0)
		{
			controlHint_ignoreClear = false;
			clearControlHint();
		}
	}
	dsq->continuity.update(dt);

	StateObject::update(dt);


	for (ElementUpdateList::iterator e = elementUpdateList.begin(); e != elementUpdateList.end(); e++)
	{
		(*e)->update(dt);
	}


	int i = 0;
	for (i = 0; i < dsq->game->getNumPaths(); i++)
	{
		dsq->game->getPath(i)->update(dt);
	}

	FOR_ENTITIES(j)
	{
		(*j)->postUpdate(dt);
	}

	FlockEntity::updateFlockData();

	updateCurrentVisuals(dt);
	updateCursor(dt);

	updateBgSfxLoop();
	/*
	for (int i = 0; i < 2; i++)
		dsq->spawnBubble(Vector(rand()%800 + core->cameraPos.x, rand()%600 + core->cameraPos.y, -0.05f), Vector(0, 0,0));
	*/

	// spawn bubbles
	/*
	const float spawnTime = 0.008;
	static float bubbleTimer = 0;
	bubbleTimer += dt;
	int sx1, sx2, sy1, sy2;
	int extra = 300;
	sx1 = core->screenCullX1 - extra;
	sx2 = core->screenCullX2 + extra;
	sy1 = core->screenCullY1 - extra;
	sy2 = core->screenCullY2 + extra;

	while (bubbleTimer >= spawnTime)
	{
		Vector p(sx1 + rand()%(sx2-sx1),sy1 + rand()%(sy2-sy1));
		Quad *q = new Quad;
		q->scale = Vector(0.25,0.25);
		q->position = p;
		q->setTexture("bubble");
		q->setLife(2);
		q->setDecayRate(1);
		q->alpha = 0;
		q->alpha.interpolateTo(0.4, 1, 1, 1, 1);
		addRenderObject(q, LR_ELEMENTS3);
		q->velocity = Vector(((rand()%200)-100)/100.0f, ((rand()%200)-100)/100.0f);
		q->velocity *= 32;
		bubbleTimer -= spawnTime;
	}
	*/

	sceneColor.update(dt);
	sceneColor2.update(dt);
	sceneColor3.update(dt);
	dsq->sceneColorOverlay->color = sceneColor * sceneColor2 * sceneColor3;
	if (bg)
	{
		setParallaxTextureCoordinates(bg, 0.3);
	}
	if (bg2)
	{
		setParallaxTextureCoordinates(bg2, 0.1);
	}
	updateInGameMenu(dt);
	if (avatar && grad && bg && bg2)
	{
		//float d = avatar->position.y / float(40000.0);

		/*
		Vector top1(0.6, 0.8, 0.65);
		Vector top2(0.1, 0.2, 0.4);
		Vector btm1(0.1, 0.2, 0.4);
		Vector btm2(0, 0, 0.1);
		*/
		/*
		Vector top1(0.5, 0.65, 7);
		Vector top2(0.2, 0.25, 0.3);
		Vector btm1(0.2, 0.25, 0.3);
		Vector btm2(0, 0, 0.1);
		*/

		/*
		// dynamic gradient
		Vector top1(99/256.0f, 166/256.0f, 170/256.0f);
		Vector top2(86/256.0f, 150/256.0f, 154/256.0f);
		Vector btm1(86/256.0f, 150/256.0f, 154/256.0f);
		Vector btm2(66/256.0f, 109/256.0f, 122/256.0f);
		btm2 *= 0.75;


		Vector newtop1(105/256.0f, 190/256.0f, 200/256.0f);

		grad->makeVertical(newtop1, btm2);
		*/



		//grad->makeVertical(top1*(1.0-d) + top2*(d), btm1*(1.0-d) + btm2*(d));
		/*
		float range = 0.5f;
		float left = 1.0f - range;
		float v = ((range-(d*range)) + left);
		bg->color = Vector(v,v,v);
		bg2->color = Vector(v,v,v);
		*/

	}

#ifdef AQUARIA_BUILD_SCENEEDITOR
	{
		sceneEditor.update(dt);
	}
#endif

	dsq->emote.update(dt);

	if (!isPaused())
	{
		timer += dt;
		while (timer > 1.0f)
			timer -= 1.0f;

		halfTimer += dt*0.5f;
		while (halfTimer > 1.0f)
			halfTimer -= 1.0f;
	}


	if (avatar && (avatar->isEntityDead() || avatar->health <= 0) && core->getNestedMains()==1 && !isPaused())
	{
		dsq->stopVoice();
		/*
		if (runGameOverScript && dsq->runScript("scripts/maps/" + sceneName + ".lua", "gameOver"))
		{
			// no game over for you!
		}
		else
		*/
		if (deathTimer > 0)
		{
			deathTimer -= dt;
			if (deathTimer <= 0)
			{
				// run game over script

				//errorLog("here");
				core->enqueueJumpState("GameOver");
			}
		}
	}
	if (avatar && avatar->isSinging() && avatar->songInterfaceTimer > 0.5f)
	{
		avatar->entityToActivate = 0;
		avatar->pathToActivate = 0;
	}

	if (avatar && core->getNestedMains() == 1 && !isPaused() && !avatar->isSinging() && activation)
		// && dsq->continuity.form == FORM_NORMAL
	{
		dsq->continuity.refreshAvatarData(avatar);

		Vector bigGlow(3,3);
		float bigGlowTime = 0.4;
		bool hadThingToActivate = (avatar->entityToActivate!=0 || avatar->pathToActivate!=0);
		avatar->entityToActivate = 0;

		if (avatar->canActivateStuff())
		{
			FOR_ENTITIES(i)
			{
				Entity *e = *i;
				int sqrLen = (dsq->getGameCursorPosition() - e->position).getSquaredLength2D();
				if (sqrLen < sqr(e->activationRadius)
					&& (avatar->position-e->position).getSquaredLength2D() < sqr(e->activationRange)
					&& e->activationType == Entity::ACT_CLICK
					&& !e->position.isInterpolating()
					)
				{
					//if (trace(avatar->position, e->position))
					{
						avatar->entityToActivate = e;
						dsq->cursorGlow->alpha.interpolateTo(1, 0.2);
						dsq->cursorBlinker->alpha.interpolateTo(1.0,0.1);
						if (!hadThingToActivate)
						{
							dsq->cursorGlow->scale = Vector(1,1);
							dsq->cursorGlow->scale.interpolateTo(bigGlow,bigGlowTime,1, -1, 1);
						}
					}
					break;
				}
			}
		}

		avatar->pathToActivate = 0;
		//if (!avatar->entityToActivate && !avatar->state.lockedToWall)


		// make sure you also disable entityToActivate
		if (dsq->game && dsq->game->avatar->canActivateStuff())
		{
			Path* p = dsq->game->getScriptedPathAtCursor(true);
			if (p && p->cursorActivation)
			{
				Vector diff = p->nodes[0].position - dsq->game->avatar->position;

				if (p->isCoordinateInside(dsq->game->avatar->position) || diff.getSquaredLength2D() < sqr(PATH_ACTIVATION_RANGE))
				{
					//if (trace(avatar->position, p->nodes[0].position))
					{
						avatar->pathToActivate = p;
						dsq->cursorGlow->alpha.interpolateTo(1,0.2);
						dsq->cursorBlinker->alpha.interpolateTo(1,0.2);
						if (!hadThingToActivate)
						{
							dsq->cursorGlow->scale = Vector(1,1);
							dsq->cursorGlow->scale.interpolateTo(bigGlow,bigGlowTime,1, -1, 1);
						}
					}
				}
			}
		}

		/*
		if (!hadThingToActivate && (avatar->entityToActivate || avatar->pathToActivate))
		{
			debugLog("Spawning cursor particles");
			dsq->spawnParticleEffect("CursorBurst", dsq->getGameCursorPosition());
		}
		*/
	}

	if (!activation)
	{
		avatar->entityToActivate = 0;
		avatar->pathToActivate = 0;
	}

	if (!avatar->entityToActivate && !avatar->pathToActivate)
	{
		dsq->cursorGlow->alpha.interpolateTo(0, 0.2);
		dsq->cursorBlinker->alpha.interpolateTo(0, 0.1);
	}

	if (!isSceneEditorActive())
	{
		if (!isPaused())
			waterLevel.update(dt);
		cameraInterp.update(dt);
		if (cameraFollow)
		{
			Vector dest = getCameraPositionFor(*cameraFollow);
			
			if (avatar)
			{
				if (avatar->looking && !dsq->game->isPaused()) {
					Vector diff = avatar->getAim();//dsq->getGameCursorPosition() - avatar->position;
					diff.capLength2D(600);
					dest += diff;
				}
				else {
					avatar->looking = 0;
				}
			}

			/*
			if (avatar)
			{
				if (!dsq->game->isPaused() && core->mouse.buttons.middle && !dsq->game->avatar->isSinging() && dsq->game->avatar->isInputEnabled())
				{
					Vector diff = avatar->getAim();//dsq->getGameCursorPosition() - avatar->position;
					diff.capLength2D(600);

					avatar->looking = 1;
					dest += diff;
				}
				else
				{
					avatar->looking = 0;
				}
			}
			*/
			

			if (cameraLerpDelay==0)
			{
				//cameraLerpDelay = 0.15;
				cameraLerpDelay = vars->defaultCameraLerpDelay;
			}
			cameraInterp.interpolateTo(dest, cameraLerpDelay);

			dsq->cameraPos.x = cameraInterp.x;
			dsq->cameraPos.y = cameraInterp.y;

			// constrainCamera
			constrainCamera();
			/*
			if (cam_region)
				ConstrainToRegion(&ek->cameraPos, cam_region, core->getVirtualWidth()*(core->globalScale.x), core->getVirtualHeight()*(core->globalScale.y));
				*/
		}

	}

}

void Game::setElementLayerVisible(int bgLayer, bool v)
{
	core->getRenderObjectLayer(LR_ELEMENTS1+bgLayer)->visible = v;
}

bool Game::isElementLayerVisible(int bgLayer)
{
	return core->getRenderObjectLayer(LR_ELEMENTS1+bgLayer)->visible;
}

/*
void Game::cameraPanToNode(Path *p, int speed)
{

	cameraFollow = &p->nodes[0].position;
}

void Game::cameraRestore()
{
	setCameraFollow(avatar);
}
*/

Shot *Game::fireShot(const std::string &bankShot, Entity *firer, Entity *target, const Vector &pos, const Vector &aim, bool playSfx)
{
	Shot *s = 0;
	if (firer)
	{
		s = new Shot;
		s->firer = firer;

		if (pos.isZero())
			s->position = firer->position;
		else
			s->position = pos;

		if (target)
			s->setTarget(target);

		Shot::loadBankShot(bankShot, s);

		if (!aim.isZero())
			s->setAimVector(aim);
		else
		{
			if (target && firer)
				s->setAimVector(target->position - firer->position);
			else if (firer)
				s->setAimVector(firer->getNormal());
			else
				s->setAimVector(Vector(0,1));
		}

		s->updatePosition();
		s->fire(playSfx);


		core->getTopStateData()->addRenderObject(s, LR_PROJECTILES);
	}

	return s;
}

Shot* Game::fireShot(Entity *firer, const std::string &particleEffect, Vector position, bool big, Vector dir, Entity *target, int homing, int velLenOverride, int targetPt)
{
	//sound->playSfx("BasicShot", 255, 0, rand()%100 + 1000);
	/*
	DamageType dt;
	if (firer->getEntityType() == ET_ENEMY)
		dt = DT_ENEMY_ENERGYBLAST;
	else if (firer->getEntityType() == ET_AVATAR)
		dt = DT_AVATAR_ENERGYBLAST;
	else
		debugLog("UNDEFINED DAMAGE TYPE!");
	int velLen = 900;
	if (big)
		velLen += 200;
	if (velLenOverride != 0)
		velLen = velLenOverride;
	Shot *shot;
	if (big && target)
		shot = new Shot(dt, firer, position, target, "energyBlast", homing, velLen, 8, 0.1, 4, 2);
	else
	{
		if (homing > 0)
			shot = new Shot(dt, firer, position, target, "energyBlast", homing, velLen, 5, 0.1, 4, 1);
		else
			shot = new Shot(dt, firer, position, 0, "energyBlast", 0, velLen, 5, 0.1, 4, 1);
	}
	shot->setParticleEffect(particleEffect);
	if (big)
		shot->scale = Vector(1.5, 1.5);
	//shot->velocity = dsq->getGameCursorPosition() - position;
	if (dir.x == 0 && dir.y == 0 && target)
	{
		shot->velocity = target->position - firer->position;
	}
	else
	{
		shot->velocity = dir;
	}
	if (velLen != 0 && !shot->velocity.isZero())
		shot->velocity.setLength2D(velLen);

	if (firer->getEntityType() == ET_AVATAR && homing && target)
	{
		//std::ostringstream os;
		//os << "targetvel(" << target->vel.x << ", " << target->vel.y << ")";
		//debugLog(os.str());
		if (!target->vel.isZero() && !target->vel.isNan())
			shot->velocity += target->vel;
	}
	shot->targetPt = targetPt;
	//shot->velocity += firer->vel;

	core->getTopStateData()->addRenderObject(shot, LR_PROJECTILES);
	return shot;
	*/
	debugLog("Old version of Game::fireShot is obsolete");
	return 0;
}

void Game::warpCameraTo(RenderObject *r)
{
	warpCameraTo(r->position);
}

void Game::warpCameraTo(Vector position)
{
	cameraInterp.stop();
	cameraInterp = getCameraPositionFor(position);
	dsq->cameraPos.x = cameraInterp.x;
	dsq->cameraPos.y = cameraInterp.y;
}

void Game::snapCam()
{
	if (cameraFollow)
	{
		Vector p = getCameraPositionFor(*cameraFollow);
		cameraInterp.interpolateTo(p,0);
		cameraInterp = p;
		core->cameraPos = p;
	}
}

ElementTemplate Game::getElementTemplateForLetter(int i)
{
	float cell = 64.0f/512.0f;
	//for (int i = 0; i < 27; i++)
	ElementTemplate t;
	t.idx = 1024+i;
	t.gfx = "Aquarian";
	int x = i,y=0;
	while (x >= 6)
	{
		x -= 6;
		y++;
	}

	t.tu1 = x*cell;
	t.tv1 = y*cell;
	t.tu2 = t.tu1 + cell;
	t.tv2 = t.tv1 + cell;

	t.tv2 = 1 - t.tv2;
	t.tv1 = 1 - t.tv1;
	std::swap(t.tv1,t.tv2);

	t.w = 512*cell;
	t.h = 512*cell;
	//elementTemplates.push_back(t);
	return t;
}

void Game::loadElementTemplates(std::string pack)
{

    stringToLower(pack);
	/*
	std::string fn = ("data/"+pack+".xml");
	if (!exists(fn))
	{
		loadElementTemplatesDAT(pack);
		return;
	}
	loadElementTemplatesXML(pack);
	*/

	elementTemplates.clear();

	// HACK: need to uncache things! causes memory leak currently
	bool doPrecache=false;
	std::string fn;

	if (dsq->mod.isActive())
		fn = dsq->mod.getPath() + "tilesets/" + pack + ".txt";
	else
		fn = "data/tilesets/" + pack + ".txt";


	if (lastTileset == fn)
	{
		doPrecache=false;
	}

	lastTileset = fn;
	if (!exists(fn))
	{
		errorLog ("Could not open element template pack [" + fn + "]");
		return;
	}

	if (doPrecache)
	{
		tileCache.clean();
	}

	InStream in(fn.c_str());
	std::string line;
	while (std::getline(in, line))
	{
		int idx=-1, w=-1, h=-1;
		std::string gfx;
		std::istringstream is(line);
		is >> idx >> gfx >> w >> h;
		ElementTemplate t;
		t.idx = idx;
		t.gfx = gfx;
		if (w==0) w=-1;
		if (h==0) h=-1;
		t.w = w;
		t.h = h;
		elementTemplates.push_back(t);
		if (doPrecache)
			tileCache.precacheTex(gfx);
	}
	in.close();

	for (int i = 0; i < elementTemplates.size(); i++)
	{
		for (int j = i; j < elementTemplates.size(); j++)
		{
			if (elementTemplates[i].idx > elementTemplates[j].idx)
			{
				std::swap(elementTemplates[i], elementTemplates[j]);
			}
		}
	}
	for (int i = 0; i < 27; i++)
	{
		elementTemplates.push_back(getElementTemplateForLetter(i));
	}
}

void Game::clearGrid(int v)
{
	// ensure that grid is really a byte-array
	compile_assert(sizeof(grid) == MAX_GRID * MAX_GRID);

	memset(grid, v, sizeof(grid));
}

void Game::resetFromTitle()
{
	overrideMusic = "";
}

void Game::setGrid(ElementTemplate *et, Vector position, float rot360)
{
	for (int i = 0; i < et->grid.size(); i++)
	{
		TileVector t(position);
		/*
		std::ostringstream os;
		os << "opos(" << position.x << ", " << position.y << ") centre(" << t.x << ", " << t.y << ")";
		debugLog(os.str());
		*/
		int x = et->grid[i].x;
		int y = et->grid[i].y;
		if (rot360 >= 0 && rot360 < 90)
		{
		}
		else if (rot360 >= 90 && rot360 < 180)
		{
			int swap = y;
			y = x;
			x = swap;
			x = -x;
		}
		else if (rot360 >= 180 && rot360 < 270)
		{
			x = -x;
			y = -y;
		}
		else if (rot360 >= 270 && rot360 < 360)
		{
			int swap = y;
			y = x;
			x = swap;
			y = -y;
		}
		TileVector s(t.x+x, t.y+y);
		//Vector v = Vector(position.x+et->grid[i].x*TILE_SIZE, position.y+et->grid[i].y*TILE_SIZE);
		setGrid(s, 1);
	}
}

void Game::removeState()
{
	const float fadeTime = 0.25;

	dsq->toggleVersionLabel(false);
	
	dsq->subtitlePlayer.hide(fadeTime);

	dropIngrNames.clear();

	debugLog("Entering Game::removeState");
	shuttingDownGameState = true;
	debugLog("avatar->endOfGameState()");
	if (avatar)
	{
		avatar->endOfGameState();
	}

#ifdef AQUARIA_BUILD_SCENEEDITOR
	debugLog("toggle sceneEditor");
	if (sceneEditor.isOn())
		sceneEditor.toggle(false);
#endif

	debugLog("gameSpeed");
	dsq->gameSpeed.interpolateTo(1, 0);

	debugLog("bgSfxLoop");

	dsq->loops.stopAll();

	debugLog("toggleCursor");

	dsq->toggleCursor(0, fadeTime);

	if (!isInGameMenu())
		avatar->disableInput();

	debugLog("control hint");

	controlHint_ignoreClear = false;
	clearControlHint();
	dsq->overlay->color = 0;

	//dsq->overlay->alpha = 0;
	dsq->overlay->alpha.interpolateTo(1, fadeTime);
	dsq->main(fadeTime);

	/*
	// to block on voice overs
	while (dsq->isStreamingVoice())
	{
		dsq->main(FRAME_TIME);
	}
	*/
	// AFTER TRANSITION:

	dsq->rumble(0,0,0);

	dsq->sound->clearFadingSfx();


	ingredients.clear();

	core->particlesPaused = false;

	elementUpdateList.clear();

	if (core->afterEffectManager)
	{
		//core->afterEffectManager->blurShader.setMode(0);
		core->afterEffectManager->setActiveShader(AS_NONE);
	}
	dsq->setCursor(CURSOR_NORMAL);
	dsq->darkLayer.toggle(0);
	dsq->shakeCamera(0,0);
	if (core->afterEffectManager)
		core->afterEffectManager->clear();

	dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = true;

	//core->sound->fadeOut(1);
	if (saveFile)
	{
		delete saveFile;
		saveFile = 0;
	}
	/*
	if (core->getEnqueuedJumpState() != "Game")
	{
		this->overrideMusic = "";
	}
	*/
	dsq->continuity.zoom = core->globalScale;

	dsq->game->toggleOverrideZoom(false);
	dsq->game->avatar->myZoom.stop();
	dsq->globalScale.stop();

	dsq->game->avatar->myZoom = Vector(1,1);
	dsq->globalScale = Vector(1,1);

	for (int i = 0; i < getNumPaths(); i++)
	{
		Path *p = getPath(i);
		p->destroy();
		delete p;
	}
	clearPaths();

	StateObject::removeState();
	dsq->clearElements();
	dsq->clearEntities();
	avatar = 0;
	//items.clear();
#ifdef AQUARIA_BUILD_SCENEEDITOR
	sceneEditor.shutdown();
#endif

	cameraFollow = 0;
	core->cameraPos = Vector(0,0);
	sceneColor.stop();

	controlHint_mouseLeft = controlHint_mouseRight = controlHint_mouseMiddle = controlHint_mouseBody = controlHint_bg = controlHint_image = 0;
	controlHint_text = 0;

	miniMapRender = 0;
	gridRender = 0;
	worldMapRender = 0;
	//core->sound->stopStreamingOgg();

	// optimize:: clear layers
	/*
	int c = 0;
	for (DSQ::RenderObjectLayers::iterator i = core->renderObjectLayers.begin(); i != core->renderObjectLayers.end(); i++)
	{
		DSQ::RenderObjects *r = &(*i);
		//if (c <= LR_HUD)
		if (c<=LR_PARTICLES)
			r->clear();
		c++;
	}
	*/

	clearObsRows();


	debugLog("killAllShots");
	Shot::killAllShots();
	debugLog("killAllBeams");
	Beam::killAllBeams();
	debugLog("killAllWebs");
	Web::killAllWebs();
	debugLog("killAllSpores");
	Spore::killAllSpores();

	debugLog("clear Local Sounds");
	core->sound->clearLocalSounds();

	active = false;


	dsq->routeShoulder = true;



	debugLog("Game::removeState Done");
	//core->sound->stopAllSounds();
}

bool Game::isActive()
{
	return active;
}

bool isBoxIn(Vector pos1, Vector sz1, Vector pos2, Vector sz2)
{
	if ((pos1.x - sz1.x > pos2.x-sz2.x) && (pos1.x - sz1.x < pos2.x+sz2.x))
	{
		if ((pos1.y - sz1.y > pos2.y-sz2.y) && (pos1.y - sz1.y < pos2.y+sz2.y))
			return true;
		else if ((pos1.y + sz1.y > pos2.y-sz2.y) && (pos1.y + sz1.y < pos2.y+sz2.y))
			return true;
	}
	else if ((pos1.x + sz1.x > pos2.x-sz2.x) && (pos1.x + sz1.x < pos2.x+sz2.x))
	{
		if ((pos1.y - sz1.y > pos2.y-sz2.y) && (pos1.y - sz1.y < pos2.y+sz2.y))
			return true;
		else if ((pos1.y + sz1.y > pos2.y-sz2.y) && (pos1.y + sz1.y < pos2.y+sz2.y))
			return true;
	}
	return false;
}

Vector Game::getClosestPointOnTriangle(Vector a, Vector b, Vector c, Vector p)
{
   Vector  Rab = getClosestPointOnLine(a, b, p);
   Vector  Rbc = getClosestPointOnLine(b, c, p);
   Vector  Rca = getClosestPointOnLine(c, a, p);
   int RabDist = Rab.getSquaredLength2D();
   int RbcDist = Rab.getSquaredLength2D();
   int RcaDist = Rca.getSquaredLength2D();
   if (RabDist < RbcDist && RabDist < RcaDist)
   {
	   return Rab;
   }
   if (RbcDist < RabDist && RbcDist < RcaDist)
	   return Rbc;
	return Rca;
}

Vector Game::getClosestPointOnLine(Vector a, Vector b, Vector p)
{
   // Determine t (the length of the vector from a to p)
	Vector c = p - a;
   Vector V = b-a;
   V.normalize2D();
   float d = (a-b).getLength2D();
   float t = V.dot(c);

   // Check to see if t is beyond the extents of the line segment

   if (t < 0) return a;
   if (t > d) return b;

   // Return the point between a and b

   //set length of V to t;
   V.setLength2D(t);
   return a + V;
}

bool Game::collideCircleWithGrid(const Vector& position, int r)
{
	Vector tile = position;
	TileVector t(tile);
	tile.x = t.x;
	tile.y = t.y;

	float hsz = TILE_SIZE/2;
	int xrange=1,yrange=1;
	xrange = (r/TILE_SIZE)+1;
	yrange = (r/TILE_SIZE)+1;

	for (int x = tile.x-xrange; x <= tile.x+xrange; x++)
	{
		for (int y = tile.y-yrange; y <= tile.y+yrange; y++)
		{
			int v = this->getGrid(TileVector(x, y));
			if (v != 0)
			{
				//if (tile.x == x && tile.y == y) return true;
				TileVector t(x, y);
				lastCollidePosition = t.worldVector();
				//if (tile.x == x && tile.y == y) return true;
				float rx = (x*TILE_SIZE)+TILE_SIZE/2;
				float ry = (y*TILE_SIZE)+TILE_SIZE/2;

				float rSqr;
				lastCollideTileType = (ObsType)v;

				rSqr = sqr(position.x - (rx+hsz)) + sqr(position.y - (ry+hsz));
				if (rSqr < sqr(r))	return true;

				rSqr = sqr(position.x - (rx-hsz)) + sqr(position.y - (ry+hsz));
				if (rSqr < sqr(r))	return true;

				rSqr = sqr(position.x - (rx-hsz)) + sqr(position.y - (ry-hsz));
				if (rSqr < sqr(r))	return true;

				rSqr = sqr(position.x - (rx+hsz)) + sqr(position.y - (ry-hsz));
				if (rSqr < sqr(r))	return true;


				if (position.x > rx-hsz && position.x < rx+hsz)
				{
					if (fabsf(ry - position.y) < r+hsz)
					{
						return true;
					}
				}


				if (position.y > ry-hsz && position.y < ry+hsz)
				{
					if (fabsf(rx - position.x) < r+hsz)
					{
						return true;
					}
				}
			}
		}
	}
	lastCollideTileType = OT_EMPTY;
	return false;
}

bool Game::collideBoxWithGrid(const Vector& position, int hw, int hh)
{
	Vector tile = position;
	TileVector t(tile);
	tile.x = t.x;
	tile.y = t.y;

	float hsz = TILE_SIZE/2;
	int xrange=1,yrange=1;
	xrange = (hw/TILE_SIZE)+1;
	yrange = (hh/TILE_SIZE)+1;
	for (int x = tile.x-xrange; x <= tile.x+xrange; x++)
	{
		for (int y = tile.y-yrange; y <= tile.y+yrange; y++)
		{
			int v = this->getGrid(TileVector(x, y));
			if (v == 1)
			{
				if (tile.x == x && tile.y == y) return true;
				float rx = (x*TILE_SIZE)+TILE_SIZE/2;
				float ry = (y*TILE_SIZE)+TILE_SIZE/2;


				if (isBoxIn(position, Vector(hw, hh), Vector(rx, ry), Vector(hsz, hsz)))
				{
					return true;
				}
				if (isBoxIn(Vector(rx, ry), Vector(hsz, hsz), position, Vector(hw, hh)))
				{
					return true;
				}
			}
		}
	}
	return false;
}


void Game::learnedRecipe(Recipe *r, bool effects)
{
	if (nocasecmp(dsq->getTopStateData()->name,"Game")==0 && !applyingState)
	{
		std::ostringstream os;
		os << dsq->continuity.stringBank.get(23) << " "  << r->resultDisplayName << " " << dsq->continuity.stringBank.get(24);
		IngredientData *data = dsq->continuity.getIngredientDataByName(r->result);
		if (data)
		{
			if (effects)
			{
				dsq->game->setControlHint(os.str(), 0, 0, 0, 3, std::string("gfx/ingredients/") + data->gfx);
			}
		}

		/*
		errorLog(os.str());
		*/
	}
}


