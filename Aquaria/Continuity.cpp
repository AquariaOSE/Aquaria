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
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "ScriptedEntity.h"
#include "GridRender.h"
#include "DeflateCompressor.h"
#include "ttvfs_stdio.h"
#include "ReadXML.h"
#include "Web.h"

#include <tinyxml2.h>
using namespace tinyxml2;

#define MAX_EATS			8

const float webBitTime		= 2;
const float defenseTime		= 15;
const float speedTime		= 30;
const float	biteTime		= 30;
const float fishPoisonTime	= 30;
const float energyTime		= 45;
const float webTime			= 8;
const float petPowerTime	= 30;
const float lightTime		= 60;

Continuity::Continuity()
{
	toggleMoveMode = false;

	poisonBitTime = 1;
	poisonBitTimeAvatar = 2;

	statsAndAchievements = 0;
}

bool Continuity::isIngredientFull(IngredientData *data)
{
	for (size_t i = 0; i < ingredients.size(); i++)
	{
		if (nocasecmp(ingredients[i]->name, data->name)==0)
		{
			if (ingredients[i]->amount >= ingredients[i]->maxAmount)
				return true;
			else
				return false;
		}
	}
	return false;
}

void Continuity::pickupIngredient(IngredientData *d, int amount, bool effects, bool learn)
{
	if(learn)
		learnRecipe(d->name, effects);

	if (!getIngredientHeldByName(d->name))
	{
		ingredients.push_back(d);
	}

	if (d->amount < d->maxAmount - amount)
	{
		d->amount += amount;
	}
	else
	{
		d->amount = d->maxAmount;
	}
}

int Continuity::indexOfIngredientData(const IngredientData* data) const
{
	for (size_t i = 0; i < ingredientData.size(); i++)
	{
		if (ingredientData[i]->name == data->name)
		{
			return i;
		}
	}
	return -1;
}

#define FOR_INGREDIENTDATA(x) for (size_t x = 0; x < ingredientData.size(); x++)

IngredientData *Continuity::getIngredientDataByName(const std::string &name)
{
	FOR_INGREDIENTDATA(i)
	{
		if (nocasecmp(ingredientData[i]->name, name)==0)
			return ingredientData[i];
	}
	return 0;
}

IngredientData *Continuity::getIngredientHeldByName(const std::string &name) const
{
	for (size_t i = 0; i < ingredients.size(); i++) {
		if (nocasecmp(ingredients[i]->name, name)==0)
			return ingredients[i];
	}
	return 0;
}

IngredientType Continuity::getIngredientTypeFromName(const std::string &name) const
{
	if (name == "Meat")
		return IT_MEAT;
	else if (name == "Oil")
		return IT_OIL;
	else if (name == "Egg")
		return IT_EGG;
	else if (name == "Part")
		return IT_PART;
	else if (name == "Bone")
		return IT_BONE;
	else if (name == "Shell")
		return IT_SHELL;
	else if (name == "Tentacle")
		return IT_TENTACLE;
	else if (name == "Berry")
		return IT_BERRY;
	else if (name == "Leaf")
		return IT_LEAF;
	else if (name == "Poultice")
		return IT_POULTICE;
	else if (name == "IceChunk")
		return IT_ICECHUNK;
	else if (name == "Bulb")
		return IT_BULB;
	else if (name == "Roll")
		return IT_ROLL;
	else if (name == "Soup")
		return IT_SOUP;
	else if (name == "Cake")
		return IT_CAKE;
	else if (name == "IceCream")
		return IT_ICECREAM;
	else if (name == "Loaf")
		return IT_LOAF;
	else if (name == "PerogiType")
		return IT_PEROGI;
	else if (name == "Mushroom")
		return IT_MUSHROOM;
	else if (name == "Anything")
		return IT_ANYTHING;
	else if (name.length() && isdigit(name[0]))
		return (IngredientType)atoi(name.c_str());

	return IT_NONE;
}

std::string Continuity::getIngredientDisplayName(const std::string& name) const
{
	IngredientNameMap::const_iterator it = ingredientDisplayNames.find(name);
	if (it != ingredientDisplayNames.end())
		return it->second;

	return splitCamelCase(name);
}

IngredientData *Continuity::getIngredientHeldByIndex(size_t idx) const
{
	if (idx >= ingredients.size()) return 0;
	return ingredients[idx];
}

IngredientData *Continuity::getIngredientDataByIndex(size_t idx)
{
	if (idx >= ingredientData.size()) return 0;
	return ingredientData[idx];
}

int Continuity::getIngredientDataSize() const
{
	return (int)ingredientData.size();
}

int Continuity::getIngredientHeldSize() const
{
	return (int)ingredients.size();
}

void Continuity::initFoodSort()
{
	// move to init
	sortByType.clear();

	sortByType.push_back(FoodSortOrder(IT_POULTICE));
	sortByType.push_back(FoodSortOrder(IT_ROLL));
	sortByType.push_back(FoodSortOrder(IT_CAKE));
	sortByType.push_back(FoodSortOrder(IT_SOUP));
	sortByType.push_back(FoodSortOrder(IT_LOAF));
	sortByType.push_back(FoodSortOrder(IT_PEROGI));
	sortByType.push_back(FoodSortOrder(IT_LEAF));
	sortByType.push_back(FoodSortOrder(IT_MEAT));
	sortByType.push_back(FoodSortOrder(IT_OIL));
	sortByType.push_back(FoodSortOrder(IT_ICECREAM));
	sortByType.push_back(FoodSortOrder(IT_BERRY));
	sortByType.push_back(FoodSortOrder(IT_MUSHROOM));
	sortByType.push_back(FoodSortOrder(IT_BULB));
	sortByType.push_back(FoodSortOrder(IT_EGG));
	sortByType.push_back(FoodSortOrder(IT_SHELL));
	sortByType.push_back(FoodSortOrder(IT_PART));
	sortByType.push_back(FoodSortOrder(IT_TENTACLE));
	sortByType.push_back(FoodSortOrder(IT_ICECHUNK));
	sortByType.push_back(FoodSortOrder(IT_BONE));
	sortByType.push_back(FoodSortOrder(IT_FOOD));

	sortByHeal.clear();
	sortByHeal.push_back(FoodSortOrder(IT_NONE, IET_MAXHP));
	for (int i = 10; i >= -10; i--)
	{
		if (i != 0)
			sortByHeal.push_back(FoodSortOrder(IT_NONE, IET_HP, "", i));
	}
	sortByHeal.push_back(FoodSortOrder(IT_NONE, IET_DEFENSE));
	sortByHeal.push_back(FoodSortOrder(IT_NONE, IET_SPEED));

	sortByIngredients.clear();
	for (int i = 0; i < IT_INGREDIENTSEND; i++)
	{
		sortByIngredients.push_back(FoodSortOrder((IngredientType)i));
	}
}

void Continuity::sortFood()
{
	std::vector<FoodSortOrder> sortOrder;

	bool doSort = true;

	switch (dsq->continuity.foodSortType)
	{

	case FOODSORT_BYTYPE:
		sortOrder = sortByType;
	break;
	case FOODSORT_BYHEAL:
		sortOrder = sortByHeal;
	break;
	case FOODSORT_BYINGREDIENT:
		sortOrder = sortByIngredients;
	break;

	}



	if (doSort)
	{


		std::vector<IngredientData*> sort;

		for (size_t i = 0; i < dsq->continuity.ingredients.size(); i++)
		{
			dsq->continuity.ingredients[i]->sorted = false;
		}

		for (size_t j = 0; j < sortOrder.size(); j++)
		{
			for (size_t i = 0; i < dsq->continuity.ingredients.size(); i++)
			{
				IngredientData *data = dsq->continuity.ingredients[i];
				if (!data->sorted)
				{
					if (sortOrder[j].type == IT_NONE || sortOrder[j].type == data->type)
					{
						if (!sortOrder[j].name.empty())
						{
							if (sortOrder[j].name == data->name)
							{
								data->sorted = true;
								sort.push_back(data);
							}
						}
						else if (sortOrder[j].effectType != IET_NONE)
						{
							for (size_t c = 0; c < data->effects.size(); c++)
							{
								if (data->effects[c].type == sortOrder[j].effectType)
								{
									if (sortOrder[j].effectAmount == 0 || data->effects[c].magnitude == sortOrder[j].effectAmount)
									{
										data->sorted = true;
										sort.push_back(data);
									}
								}
							}
						}
						else
						{
							data->sorted = true;
							sort.push_back(data);
						}
					}
				}
			}
		}

		for (size_t i = 0; i < dsq->continuity.ingredients.size(); i++)
		{
			IngredientData *data = dsq->continuity.ingredients[i];
			if (!data->sorted)
			{
				data->sorted = true;
				sort.push_back(data);
			}
		}

		ingredients.clear();
		for (size_t i = 0; i < sort.size(); i++) {
			ingredients.push_back(sort[i]);
		}
		sort.clear();

	}



}

void Continuity::setRegen(float t)
{
	regenTimer.start(t);
}

void Continuity::setTrip(float t)
{
	tripTimer.start(t);
	game->avatar->applyTripEffects();
}

void Continuity::setInvincible(float t)
{
	invincibleTimer.start(t);
}

void Continuity::setSpeedMultiplier(float s, float t)
{
	speedMultTimer.start(t);
	speedMult = s;
}

void Continuity::setEnergy(float m, float t)
{
	energyTimer.start(t);
	energyMult = m;
}

void Continuity::setLiPower(float m, float t)
{
	liPowerTimer.start(t);
	liPower = m;
}

void Continuity::setPetPower(float m, float t)
{
	petPower = m;
	petPowerTimer.start(t);
}

void Continuity::setLight(float m, float t)
{
	light = m;
	lightTimer.start(t);
}

void Continuity::setWeb(float t)
{
	webTimer.start(t);

	webBitTimer.start(webBitTime);

	if (game->avatar)
	{
		if (!game->avatar->web)
		{
			game->avatar->createWeb();
		}
	}
}

void Continuity::setPoison(float m, float t)
{
	poisonTimer.start(t);
	poison = m;

	if (poison)
	{
		poisonBitTimer.start(poisonBitTime);
	}
}

void Continuity::cureAllStatus()
{
	setPoison(0,0);

	if (game->avatar)
	{
		game->avatar->setBlind(0);
	}
}

void Continuity::setDefenseMultiplier(float s, float t)
{
	defenseMultTimer.start(t);
	defenseMult = s;
}

void Continuity::setBiteMultiplier(float m, float t)
{
	biteMultTimer.start(t);
	biteMult = m;
}

void Continuity::setFishPoison(float m, float t)
{
	fishPoisonTimer.start(t);
	fishPoison = m;
}

std::string Continuity::getIEString(IngredientData *data, size_t i)
{
	if (i >= data->effects.size()) return "";

	IngredientEffect fx = data->effects[i];
	IngredientEffectType useType = fx.type;

	std::ostringstream os;

	switch(useType)
	{
	case IET_HP:
		if (fx.magnitude > 0)
		{
			std::ostringstream os;
			os << stringbank.get(200) << " ";
			os << stringbank.get(100) << " ";
			os << fx.magnitude;
			return os.str();
		}
		else
		{
			std::ostringstream os;
			os << stringbank.get(200) << " ";
			os << stringbank.get(101) << " ";
			os << fabsf(fx.magnitude);
			return os.str();
		}
	// break;
	case IET_MAXHP:
		return stringbank.get(201);
	// break;
	case IET_DEFENSE:
		os << stringbank.get(202);
		os << " " << fx.magnitude << " " << stringbank.get(205) << " " << defenseTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_SPEED:
		os << stringbank.get(204) << " " << fx.magnitude;
		os << " " << stringbank.get(205) << " " << speedTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_REGEN:
		os << stringbank.get(206) << " " << fx.magnitude;
		return os.str();
	// break;
	case IET_TRIP:
		return stringbank.get(207);
	// break;
	case IET_EAT:
		return stringbank.get(208);
	// break;
	case IET_BITE:
		os << stringbank.get(209);
		os << " " << stringbank.get(205) << " " << biteTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_FISHPOISON:
		os << stringbank.get(217);
		os << " " << stringbank.get(205) << " " << fishPoisonTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_INVINCIBLE:
		os << stringbank.get(210);
		os << " " << stringbank.get(205) << " " << (fx.magnitude*5) << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_ENERGY:
		os << stringbank.get(211) << " " << fx.magnitude;
		os << " " << stringbank.get(205) << " " << energyTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_BLIND:
		return stringbank.get(212);
	// break;
	case IET_POISON:
		if (fx.magnitude < 0)
			return stringbank.get(213);
		else
			return stringbank.get(214);
	// break;
	case IET_YUM:
		return stringbank.get(215);
	// break;
	case IET_WEB:
		os << stringbank.get(219);
		os << " " << stringbank.get(205) << " " << webTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_ALLSTATUS:
		return stringbank.get(218);
	// break;
	case IET_PETPOWER:
		os << stringbank.get(216);
		os << " " << stringbank.get(205) << " " << petPowerTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_LIGHT:
		os << stringbank.get(220);
		os << " " << stringbank.get(205) << " " << lightTime << " " << stringbank.get(203);
		return os.str();
	// break;
	case IET_LI:
		return stringbank.get(227);
	// break;
	case IET_SCRIPT:
		if(game->cookingScript)
		{
			std::string ret = "";
			game->cookingScript->call("getIngredientEffectString", data->name.c_str(), &ret);
			return ret;
		}
	break;
	case IET_NONE:
	case IET_RANDOM:
	case IET_MAX:
		break;
	}

	return "";
}

std::string Continuity::getAllIEString(IngredientData *data)
{
	std::ostringstream os;

	for (size_t i = 0; i < data->effects.size(); i++)
	{
		os << getIEString(data, i) << "\n";
	}

	return os.str();
}

// returns true if eaten
bool Continuity::applyIngredientEffects(IngredientData *data)
{
	bool eaten = true;
	float y =0;
	for (size_t i = 0; i < data->effects.size(); i++)
	{
		y = 300 + i * 40;
		IngredientEffect fx = data->effects[i];
		IngredientEffectType useType = fx.type;
		if (fx.type == IET_RANDOM)
		{
		}
		switch(useType)
		{
		case IET_HP:
		{
			game->avatar->heal(fx.magnitude);
			debugLog("ingredient effect: hp");
			if (fx.magnitude > 0)
			{
				dsq->centerMessage(getIEString(data, i), y);

				core->sound->playSfx("CollectMana");

				dsq->overlay2->color = Vector(0.5, 0.5, 1);
				dsq->overlay2->alpha.ensureData();
				dsq->overlay2->alpha.data->path.clear();
				dsq->overlay2->alpha.data->path.addPathNode(0, 0);
				dsq->overlay2->alpha.data->path.addPathNode(0.5, 0.5);
				dsq->overlay2->alpha.data->path.addPathNode(0, 1);
				dsq->overlay2->alpha.startPath(1);
			}
			else
			{
				dsq->centerMessage(getIEString(data, i), y, 1);

				game->avatar->playHitSound();
			}
		}
		break;
		case IET_MAXHP:
		{
			game->avatar->heal(game->avatar->maxHealth);
			debugLog("ingredient effect: maxhp");
			core->sound->playSfx("CollectMana");

			dsq->overlay2->color = Vector(0.5, 0.5, 1);
			dsq->overlay2->alpha.ensureData();
			dsq->overlay2->alpha.data->path.clear();
			dsq->overlay2->alpha.data->path.addPathNode(0, 0);
			dsq->overlay2->alpha.data->path.addPathNode(0.5, 0.5);
			dsq->overlay2->alpha.data->path.addPathNode(0, 1);
			dsq->overlay2->alpha.startPath(2);

			dsq->centerMessage(getIEString(data, i), y);
		}
		break;
		case IET_DEFENSE:
		{

			debugLog("ingredient effect: defense");

			if (fx.magnitude <= 1)
				dsq->continuity.setDefenseMultiplier(0.75f, defenseTime);
			else if (fx.magnitude == 2)
				dsq->continuity.setDefenseMultiplier(0.5f, defenseTime);
			else if (fx.magnitude == 3)
				dsq->continuity.setDefenseMultiplier(0.3f, defenseTime);
			else
				debugLog("unsupported magnitude for defense");

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("defense");
		}
		break;
		case IET_SPEED:
		{

			dsq->continuity.setSpeedMultiplier(1.0f + fx.magnitude*0.5f, speedTime);
			debugLog("ingredient effect: speed");

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("speedup");
		}
		break;
		case IET_REGEN:
		{
			dsq->continuity.setRegen(fx.magnitude*5);
			debugLog("ingredient effect: regen");

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("regen");
		}
		break;
		case IET_TRIP:
		{
			dsq->continuity.setTrip(fx.magnitude*30);
			debugLog("ingredient effect: trip");

			dsq->centerMessage(getIEString(data, i), y);
		}
		break;
		case IET_EAT:
		{
			EatData *getter = dsq->continuity.getEatData(fx.string);
			if (getter)
			{
				EatData setter = *getter;
				dsq->continuity.eatBeast(setter);
				debugLog("ate: " + setter.name);
			}
			debugLog("ingredient effect: eat");

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("gulp");
		}
		break;
		case IET_BITE:
		{
			dsq->continuity.setBiteMultiplier(1.0f + fx.magnitude, biteTime);
			debugLog("ingredient effect: bite");

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("bite");
		}
		break;
		case IET_FISHPOISON:
		{
			dsq->continuity.setFishPoison(1.0f * fx.magnitude, fishPoisonTime);
			debugLog("ingredient effect: fishPoison");

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("poison");
		}
		break;
		case IET_INVINCIBLE:
		{
			dsq->continuity.setInvincible(fx.magnitude*5);

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("invincible");
		}
		break;
		case IET_ENERGY:
		{
			dsq->continuity.setEnergy(fx.magnitude, energyTime);

			dsq->centerMessage(getIEString(data, i), y);

			dsq->sound->playSfx("energy");
		}
		break;
		case IET_BLIND:
		{
			if (fx.magnitude < 0)
			{
				game->avatar->setBlind(0);
				dsq->centerMessage(getIEString(data, i), y);
				dsq->sound->playSfx("regen");
			}
		}
		break;
		case IET_POISON:
		{
			if (fx.magnitude < 0)
			{
				dsq->continuity.setPoison(0,0);
				dsq->centerMessage(getIEString(data, i), y);
				dsq->sound->playSfx("regen");
			}
			else
			{
				dsq->sound->playSfx("poison");
				float t = 30;
				dsq->continuity.setPoison(fx.magnitude,t);
				dsq->centerMessage(getIEString(data, i), y, 1);
			}
		}
		break;
		case IET_YUM:
		{
			dsq->centerMessage(getIEString(data, i), y);
			dsq->sound->playSfx("naijayum");
		}
		break;
		case IET_WEB:
		{
			dsq->sound->playSfx("spiderweb");

			dsq->centerMessage(getIEString(data, i), y);

			dsq->continuity.setWeb(webTime);

		}
		break;
		case IET_ALLSTATUS:
		{
			dsq->sound->playSfx("regen");

			dsq->continuity.cureAllStatus();
			dsq->centerMessage(getIEString(data, i), y);
		}
		break;
		case IET_PETPOWER:
		{
			dsq->sound->playSfx("nautilus");

			dsq->continuity.setPetPower(fx.magnitude, petPowerTime);

			dsq->centerMessage(getIEString(data, i), y);
		}
		break;
		case IET_LIGHT:
		{
			dsq->sound->playSfx("sunform");

			dsq->continuity.setLight(fx.magnitude, lightTime);

			dsq->centerMessage(getIEString(data, i), y);
		}
		break;
		case IET_LI:
		{
			// this should do nothing, its just here to catch the ingredient effect so it doesn't
			// give the "default:" error message
			// this item should only affect li if naija drops it and li eats it.
		}
		break;
		case IET_SCRIPT:
		{
			// If this fails, it will still be eaten
			if(game->cookingScript)
				game->cookingScript->call("useIngredient", data->name.c_str(), &eaten);
		}
		break;
		default:
		{
			char str[256];
			sprintf((char*)&str, "ingredient effect not defined, index[%d]", int(useType));
			errorLog(str);
			eaten = false;
		}
		break;
		}
	}
	return eaten;
}

std::string Continuity::getIngredientAffectsString(IngredientData *data)
{
	return getAllIEString(data);
}

void Continuity::loadTreasureData()
{
	treasureData.clear();

	std::string line, gfx, file;
	int num, use;
	float sz;
	bool found = false;
	if (dsq->mod.isActive())
	{
		file = dsq->mod.getPath() + "treasures.txt";
		if(exists(file))
			found = true;
	}

	if(!found)
		file = "data/treasures.txt";

	debugLog("Load treasures: " + file);
	InStream in2(file.c_str());
	while (std::getline(in2, line))
	{
		std::istringstream is(line);
		is >> num >> gfx >> sz >> use;
		if (sz == 0)
			sz = 1;
		TreasureDataEntry d;
		d.gfx = gfx;
		d.sz = sz;
		d.use = use;
		treasureData[num] = d;
	}
	in2.close();
}

void Continuity::clearIngredientData()
{
	for (IngredientDatas::iterator i = ingredientData.begin(); i != ingredientData.end(); ++ i)
	{
		delete *i;
	}
	ingredientData.clear();
}

void Continuity::loadIngredientData()
{
	if(ingredients.size())
	{
		debugLog("Can't reload ingredient data, inventory is not empty");
		return; // ... because otherwise there would be dangling pointers and it would crash.
	}

	clearIngredientData();
	ingredientDisplayNames.clear();
	recipes.clear();

	loadIngredientDisplayNames("data/ingredientnames.txt");

	std::string fname = localisePath("data/ingredientnames.txt");
	loadIngredientDisplayNames(fname);

	if(dsq->mod.isActive())
	{
		fname = localisePath(dsq->mod.getPath() + "ingredientnames.txt", dsq->mod.getPath());
		loadIngredientDisplayNames(fname);
	}

	if(dsq->mod.isActive())
	{
		//load mod ingredients
		loadIngredientData(dsq->mod.getPath() + "ingredients.txt");
	}

	//load ingredients for the main game
	if(ingredientData.empty() && recipes.empty())
	{
		loadIngredientData("data/ingredients.txt");
	}
}

void Continuity::loadIngredientData(const std::string &file)
{
	debugLog("Load ingredient data: " + file);

	std::string line, name, gfx, type, effects;

	clearIngredientData();
	recipes.clear();

	InStream in(file.c_str());

	bool recipes = false;
	bool extradata = false;
	while (std::getline(in, line))
	{
		std::istringstream inLine(line);

		inLine >> name;

		if (name == "==Recipes==")
		{
			recipes = true;
			break;
		}
		else if(name == "==Extra==")
		{
			extradata = true;
			break;
		}
		inLine >> gfx >> type;

		std::getline(inLine, effects);

		IngredientData *data = new IngredientData(name, gfx, getIngredientTypeFromName(type));

		if (!effects.empty())
		{
			size_t p1 = effects.find("(");
			size_t p2 = effects.find(")");
			if (p1 != std::string::npos && p2 != std::string::npos)
			{
				effects = effects.substr(p1+1, p2-(p1+1));
				std::istringstream fxLine(effects);
				std::string bit;
				while (fxLine >> bit)
				{
					IngredientEffect fx;

					if (bit.find("eat:") != std::string::npos)
					{
						int pos = bit.find(':')+1;
						fx.string = bit.substr(pos, bit.size()-pos);
						fx.type = IET_EAT;
					}
					else if (bit.find("yum") != std::string::npos)
					{
						fx.type = IET_YUM;
					}
					else if (bit.find("petpower") != std::string::npos)
					{
						fx.type = IET_PETPOWER;
					}
					else if (bit.find("web") != std::string::npos)
					{
						fx.type = IET_WEB;
					}
					else if (bit.find("energy") != std::string::npos)
					{
						fx.type = IET_ENERGY;
					}
					else if (bit.find("poison") != std::string::npos)
					{
						fx.type = IET_POISON;
					}
					else if (bit.find("blind") != std::string::npos)
					{
						fx.type = IET_BLIND;
					}
					else if (bit.find("allstatus") != std::string::npos)
					{
						fx.type = IET_ALLSTATUS;
					}
					else if (bit.find("maxhp") != std::string::npos)
					{
						fx.type = IET_MAXHP;
					}
					else if (bit.find("invincible") != std::string::npos)
					{
						fx.type = IET_INVINCIBLE;
					}
					else if (bit.find("trip") != std::string::npos)
					{
						fx.type = IET_TRIP;
					}
					else if (bit.find("defense") != std::string::npos)
					{
						fx.type = IET_DEFENSE;
					}
					else if (bit.find("speed") != std::string::npos)
					{
						fx.type = IET_SPEED;
					}
					else if (bit.find("random") != std::string::npos)
					{
 						fx.type = IET_RANDOM;
					}
					else if (bit.find("bite") != std::string::npos)
					{
						fx.type = IET_BITE;
					}
					else if (bit.find("fishPoison") != std::string::npos)
					{
						fx.type = IET_FISHPOISON;
					}
					else if (bit.find("regen") != std::string::npos)
					{
						fx.type = IET_REGEN;
					}
					else if (bit.find("light") != std::string::npos)
					{
						fx.type = IET_LIGHT;
					}
					else if (bit.find("hp") != std::string::npos)
					{
						fx.type = IET_HP;
					}
					else if (bit.find("li") != std::string::npos)
					{
						fx.type = IET_LI;
					}
					else if (bit.find("script") != std::string::npos)
					{
						fx.type = IET_SCRIPT;
					}

					size_t c = 0;
					while (c < bit.size())
					{
						if (bit[c] == '+')
							fx.magnitude += 1;
						else if (bit[c] == '-')
							fx.magnitude -= 1;
						else if (bit[c] == '~')
							fx.magnitude += 0.1f;
						c++;
					}
					data->effects.push_back(fx);
				}
			}
		}

		ingredientData.push_back(data);
	}

	if(extradata)
	{
		while (std::getline(in, line))
		{
			SimpleIStringStream inLine(line.c_str(), SimpleIStringStream::REUSE);
			int maxAmount = MAX_INGREDIENT_AMOUNT;
			int rotKind = 1;
			inLine >> name >> maxAmount >> rotKind;
			if (name == "==Recipes==")
			{
				recipes = true;
				break;
			}
			IngredientData *data = getIngredientDataByName(name);
			if(!data)
			{
				errorLog("Specifying data for undefined ingredient: " + name);
				continue;
			}

			data->maxAmount = maxAmount;
			data->rotKind = rotKind;
		}
	}

	if (recipes)
	{
		bool quitNext = false;

		int index=0;
		Recipe r;
		while (in >> name)
		{
			if (name == "+")
			{
				continue;
			}
			else if (name == "=")
			{
				quitNext = true;
				continue;
			}
			else
			{
				if (quitNext)
				{
					r.result = name;
					r.resultDisplayName = getIngredientDisplayName(name);
				}
				else
				{
					IngredientType it = getIngredientTypeFromName(name);
					if (it == IT_NONE)
					{
						r.addName(name);
					}
					else
					{
						r.addType(it, name);
					}
				}
			}

			if (quitNext)
			{
				r.index = index;
				this->recipes.push_back(r);
				r.clear();
				quitNext = false;
				index++;
			}
		}
	}
	in.close();
}

void Continuity::loadIngredientDisplayNames(const std::string& file)
{
	debugLog("Load ingredient display names: " + file);
	InStream in(file);
	if (!in)
		return;

	std::string line, name, text;
	while (std::getline(in, line))
	{
		size_t pos = line.find(' ');
		if (pos == std::string::npos)
			continue;
		name = line.substr(0, pos);
		text = line.substr(pos + 1);
		ingredientDisplayNames[name] = text;
	}
}

void Continuity::learnFormUpgrade(FormUpgradeType form)
{
	formUpgrades[form] = true;
}

bool Continuity::hasFormUpgrade(FormUpgradeType form)
{
	return formUpgrades[form];
}

std::string Continuity::getInternalFormName()
{
	switch(form)
	{
	case FORM_NORMAL:
		return "normal";
	case FORM_ENERGY:
		return "energy";
	case FORM_NATURE:
		return "nature";
	case FORM_BEAST:
		return "beast";
	case FORM_FISH:
		return "fish";
	case FORM_SPIRIT:
		return "spirit";
	case FORM_SUN:
		return "sun";
	case FORM_DUAL:
		return "dual";
	case FORM_NONE:
	case FORM_MAX:
		break;
	}
	return "";
}

void Continuity::loadIntoSongBank(const std::string &file)
{
	debugLog("Load songbank: " + file);
	if(!exists(file))
		return;

	XMLDocument doc;
	XMLError err = readXML(file, doc);
	if(err == XML_ERROR_EMPTY_DOCUMENT)
		return;
	if(err != XML_SUCCESS)
	{
		errorLog("Failed to load song bank: Malformed XML");
		return;
	}

	XMLElement *song = doc.FirstChildElement("Song");
	while (song)
	{
		Song s;

		if (song->Attribute("notes"))
		{
			std::string strng = song->Attribute("notes");
			std::istringstream is(strng);
			int note = 0;
			while (is >> note)
			{
				s.notes.push_back(note);
			}
		}

		if (song->Attribute("script"))
		{
			s.script = atoi(song->Attribute("script"));
		}

		int slot = -1;
		if (song->Attribute("slot"))
		{
			slot = atoi(song->Attribute("slot"));
			if (slot != -1)
			{
				if (song->Attribute("description"))
				{
					songSlotDescriptions[slot] = song->Attribute("description");
				}
				if (song->Attribute("vox"))
				{
					songSlotVox[slot] = song->Attribute("vox");
				}
			}
		}
		int idx = atoi(song->Attribute("idx"));
		songBank[idx] = s;
		if (slot != -1)
		{
			songSlotsToType[slot] = idx;
			songTypesToSlot[idx] = slot;
		}
		if (song->Attribute("name"))
		{
			songSlotNames[slot] = song->Attribute("name");
		}
		song = song->NextSiblingElement("Song");
	}
}

void Continuity::loadSongBank()
{
	songSlotDescriptions.clear();
	songSlotVox.clear();
	songSlotsToType.clear();
	songTypesToSlot.clear();
	songSlotNames.clear();
	songBank.clear();

	loadIntoSongBank(localisePath("data/songs.xml"));

	if (dsq->mod.isActive())
	{
		loadIntoSongBank(localisePath(dsq->mod.getPath() + "scripts/songs.xml", dsq->mod.getPath()));
	}
}

int Continuity::getSongTypeBySlot(size_t slot)
{
	return songSlotsToType[slot];
}

int Continuity::getSongSlotByType(int type)
{
	return songTypesToSlot[type];
}

std::string Continuity::getDescriptionForSongSlot(int songSlot)
{
	return songSlotDescriptions[songSlot];
}

std::string Continuity::getVoxForSongSlot(int songSlot)
{
	return songSlotVox[songSlot];
}

EatData *Continuity::getEatData(const std::string &name)
{
	for (size_t i = 0; i < eats.size(); i++)
	{
		if (eats[i].name == name)
			return &eats[i];
	}
	return 0;
}

void Continuity::loadEatBank()
{
	eats.clear();

	std::string file;
	bool found = false;
	if (dsq->mod.isActive())
	{
		file = dsq->mod.getPath() + "eats.txt";
		if(exists(file))
			found = true;
	}

	if(!found)
		file = "data/eats.txt";

	debugLog("Load eats: " + file);
	InStream inf(file.c_str());

	EatData curData;
	std::string read;
	while (inf >> read)
	{
		if (read.find(':')!=std::string::npos)
		{
			if (!curData.name.empty())
			{
				eats.push_back(curData);
				debugLog("added eats: " + curData.name);
			}
			std::string name = read.substr(1, read.length());
			EatData e;
			curData = e;
			curData.name = name;
		}
		else
		{
			if (!read.empty())
			{
				std::string eq, data;
				inf >> eq;
				std::getline(inf, data);
				std::istringstream is(data);
				if (read == "Shot")
				{
					is >> curData.shot;
				}
				else if (read == "AmmoUnitSize")
				{
					is >> curData.ammoUnitSize;
					curData.ammo = curData.ammoUnitSize;
				}
				else if (read == "GetUnits")
				{
					is >> curData.getUnits;
				}
				else if (read == "Health")
				{
					is >> curData.health;
				}
			}
		}
	}
	inf.close();
}

bool Continuity::hasLi()
{
	return (getFlag(FLAG_LI) == 100);
}

std::string Continuity::getSongNameBySlot(int slot)
{
	return songSlotNames[slot];
}

void Continuity::toggleLiCombat(bool t)
{
	if (hasLi())
	{
		setFlag(FLAG_LICOMBAT, (int)t);
		if (game->li)
		{
			game->li->message("c", 0);
		}
	}
}

void Continuity::warpLiToAvatar()
{
	if (hasLi())
	{
		if (game && game->li && game->avatar)
			game->li->position = game->avatar->position - Vector(0,-1);
	}
}

Song *Continuity::getSongByIndex(int idx)
{
	return &songBank[idx];
}

void Continuity::castSong(int num)
{
	if (!dsq->continuity.hasSong((SongType)num)) return;
	Entity *selected = game->avatar;

	Song *song = getSongByIndex(num);
	if (!song)
	{
		std::ostringstream os;
		os << "Could not find song with index [" << num << "]";
		debugLog(os.str());
	}


	float et = 0.5;
	std::ostringstream os;
	os << "Song/SongSlot-" << dsq->continuity.getSongSlotByType(num);
	PauseQuad *effect = new PauseQuad();
	effect->pauseLevel = 1;
	effect->setTexture(os.str());
	effect->position = selected->position + selected->offset;
	effect->scale.interpolateTo(Vector(3,3), et);

	effect->alpha.ensureData();
	effect->alpha.data->path.addPathNode(0, 0);
	effect->alpha.data->path.addPathNode(0.5f, 0.1f);
	effect->alpha.data->path.addPathNode(1, 0.5);
	effect->alpha.data->path.addPathNode(0, 0.9f);
	effect->alpha.data->path.addPathNode(0, 1);
	effect->alpha.startPath(et);
	effect->setLife(et+0.1f);
	effect->setDecayRate(1);
	effect->setPositionSnapTo(&game->avatar->position);
	game->addRenderObject(effect, LR_PARTICLES);


	// song->script == 0: internal handler only
	// song->script == 1: script handler only
	// song->script == 2: both
	if (song->script)
	{
		if (dsq->mod.isActive())
			dsq->runScriptNum(dsq->mod.getPath() + "scripts/songs.lua", "castSong", num);
		else
			dsq->runScriptNum("songs.lua", "castSong", num);
	}

	if (song->script != 1)
	{
		switch((SongType)num)
		{
		case SONG_SHIELDAURA:
			game->avatar->doShieldSong();
		break;
		case SONG_BIND:
			game->avatar->doBindSong();
		break;
		case SONG_ENERGYFORM:
			game->avatar->changeForm(FORM_ENERGY);
		break;
#ifndef AQUARIA_DEMO
		case SONG_HEAL:

			// do heal effects
			sound->playSfx("Heal");
			selected->heal(2);


			selected->skeletalSprite.animate("healSelf", 0, 1);
		break;
		case SONG_TIME:
		{
			float v = 0.3f;
			dsq->gameSpeed.ensureData();
			dsq->gameSpeed.data->path.clear();
			dsq->gameSpeed.data->path.addPathNode(0,0);
			dsq->gameSpeed.data->path.addPathNode(v,0.05f);
			dsq->gameSpeed.data->path.addPathNode(v,0.95f);
			dsq->gameSpeed.data->path.addPathNode(1,1.0f);
			dsq->gameSpeed.startPath(10);
		}
		break;
		case SONG_LANCE:
		{
			Entity *e = game->getNearestEntity(game->avatar->position, 256, game->avatar, ET_ENEMY, DT_AVATAR_LANCEATTACH);
			if (e)
			{
				e->attachLance();
			}
		}
		break;
		case SONG_LI:
			if (!dsq->continuity.hasLi() && dsq->continuity.getFlag(FLAG_LI) > 100)
			{
				dsq->emote.playSfx(EMOTE_NAIJASADSIGH);
			}
			// HACK: when you first get li, the li pointer won't be set
			if (game->li && game->avatar->isUnderWater() && dsq->continuity.hasLi())
			{
				if (!game->avatar->isNearObstruction(2) && !game->avatar->state.lockedToWall && !(game->li->position - game->avatar->position).isLength2DIn(400))
				{

					dsq->overlay->color = Vector(1,1,1);
					dsq->fade(1, 0.3f);
					dsq->run(0.3f);
					warpLiToAvatar();
					dsq->fade(0, 0.3f);
					dsq->run(0.3f);
					dsq->overlay->color = 0;

				}
				else if ((game->li->position - game->avatar->position).isLength2DIn(500))
				{
					if (dsq->continuity.getFlag(FLAG_LICOMBAT) == 1)
						dsq->continuity.setFlag(FLAG_LICOMBAT, 0);
					else
						dsq->continuity.setFlag(FLAG_LICOMBAT, 1);
					game->li->message("c", 0);
				}
				else
				{
					core->sound->playSfx("Denied");
				}

			}
			else
			{
				core->sound->playSfx("Denied");
			}
		break;
		case SONG_SPIRITFORM:
			if (game->avatar->isUnderWater())
			{
				// Don't try to enter spirit form while warping,
				// or we'll get stuck in the spirit world afterward.
				bool inWarp = false;
				const Vector avatarPosition(game->avatar->position);
				for (Path *p = game->getFirstPathOfType(PATH_WARP); p; p = p->nextOfType)
				{
					if (p->isCoordinateInside(avatarPosition))
					{
						inWarp = true;
						break;
					}
				}
				if (inWarp)
					core->sound->playSfx("SongFail");
				else
					game->avatar->changeForm(FORM_SPIRIT);
			}
			else
			{
				core->sound->playSfx("SongFail");
			}
		break;
		case SONG_NATUREFORM:
			game->avatar->changeForm(FORM_NATURE);
		break;
		case SONG_BEASTFORM:
			game->avatar->changeForm(FORM_BEAST);
		break;
		case SONG_DUALFORM:
			game->avatar->changeForm(FORM_DUAL);
		break;
		case SONG_SUNFORM:
			game->avatar->changeForm(FORM_SUN);
		break;
		case SONG_FISHFORM:
			game->avatar->changeForm(FORM_FISH);
		break;
		case SONG_MAP:
		case SONG_SONGDOOR1:
		case SONG_SONGDOOR2:
		case SONG_ANIMA:
		case SONG_NONE:
		case SONG_MAX:
		break;
#endif
		}
	}

	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if ((e->position - game->avatar->position).getSquaredLength2D() < sqr(1000))
		{
			e->song((SongType)num);
		}
	}
	for (size_t i = 0; i < game->getNumPaths(); i++)
	{
		Path *p = game->getPath(i);
		if (p && !p->nodes.empty())
		{
			PathNode *n = &p->nodes[0];
			if ((n->position - game->avatar->position).isLength2DIn(1000))
			{
				p->song((SongType)num);
			}
		}
	}
}

void Continuity::setCostume(const std::string &c)
{
	costume = c;
	game->avatar->changeForm(FORM_NORMAL, false);
}

void Continuity::learnSong(int song)
{
	knowsSong[song] = true;
}

void Continuity::unlearnSong(int song)
{
	knowsSong[song] = false;
}

bool Continuity::isSongTypeForm(SongType s)
{
	return (s == SONG_ENERGYFORM || s == SONG_BEASTFORM || s == SONG_NATUREFORM || s == SONG_SUNFORM || s == SONG_SPIRITFORM || s == SONG_FISHFORM || s== SONG_DUALFORM);
}

void Continuity::shortenSong(Song &song, size_t size)
{
	if (song.notes.size() > size)
	{
		Song copy = song;
		song.notes.clear();
		for (size_t i = copy.notes.size()-size; i < copy.notes.size(); i++)
		{
			song.notes.push_back(copy.notes[i]);
		}
	}
}

struct SongCheck
{
	SongCheck(int idx, Song *s)
	{ rank = 0; pass = false; this->song = s; songIdx = idx; }
	int songIdx;
	int rank;
	bool pass;
	Song *song;
};

const int songTolerance = 4;

int Continuity::checkSongAssisted(const Song &s)
{
	// shorten song
	Song song = s;
	shortenSong(song, 64);

	std::vector<SongCheck> songChecks;
	for (size_t c = 0; c < songBank.size(); c++)
	{
		int i = songSlotsToType[c];
		if (knowsSong[i])
		{
			Song *s = &songBank[i];
			songChecks.push_back(SongCheck(i, s));
		}
	}
	for (size_t i = 0; i < songChecks.size(); i++)
	{
		size_t j = 0;
		int c=0,m=0,last=0,rank=0;
		int ms=songChecks[i].song->notes.size();
		j = 0;

loop:
		rank = 0;
		last = j;
		m = 0;
		for (c = 0; c < ms; c++)
		{
			while (j < song.notes.size() && (*songChecks[i].song).notes[c] != song.notes[j])
			{
				j++;
				if (j >= song.notes.size())
					break;
			}
			if (j < song.notes.size())
			{
				if (m == 0)
					last = j-1;

				int diff = j-last;
				if (diff < 0)
					diff = 1;

				if (diff >= songTolerance)
					break;

				m++;

				rank += diff;
				last=j;
			}
			else
			{
				break;
			}
		}
		if (m == ms)
		{
			// make sure last note is more or less close
			if (song.notes.size()-last < 2)
			{



				songChecks[i].pass = true;
				songChecks[i].rank = rank;
			}
		}
		if (j < song.notes.size())
			goto loop;
	}
	int songIdx = SONG_NONE, lowestRank = -1;
	for (size_t i = 0; i < songChecks.size(); i++)
	{
		if (songChecks[i].pass)
		{
			int checkRank = songChecks[i].rank;
			if (lowestRank == -1 || checkRank < lowestRank)
			{
				lowestRank = songChecks[i].rank;
				songIdx = songChecks[i].songIdx;
			}
		}
	}



	return songIdx;
}

int Continuity::checkSong(const Song &song)
{
	bool knowAllSongs = false;
	// way too long song
	if (song.notes.size() > 64) return SONG_NONE;
	for (size_t c = 0; c < songBank.size(); c++)
	{
		int i = songSlotsToType[c];
		if ((knowAllSongs || knowsSong[i]))
		{
			Song *s = &songBank[i];
			if (s->notes.empty()) continue;
			size_t j = 0;

			{
				bool foundSong = false;
				size_t currentNote = 0;
				for (j = 0; j < song.notes.size(); j++)
				{
					if (currentNote < (*s).notes.size())
					{
						int bankNote = (*s).notes[currentNote];
						int songNote = song.notes[j];
						if (bankNote == songNote)
						{
							currentNote++;
						}
						else
							currentNote = 0;

						if (currentNote == s->notes.size())
						{
							if (j == song.notes.size()-1)
							{
								foundSong = true;
								break;
							}
							else
							{
								currentNote = 0;
							}
						}
					}
				}
				if (j != song.notes.size()-1) foundSong = false;

				if (foundSong)
				{
					return i;
				}
			}
		}
	}
	return -1;
}

void Continuity::getHoursMinutesSeconds(int *hours, int *minutes, int *seconds)
{
	(*hours) = int(this->seconds/(60*60));
	(*minutes) = int((this->seconds/60) - ((*hours)*60));
	(*seconds) = this->seconds - (*minutes)*60 - (*hours)*60*60;
}

bool Continuity::hasSong(int song)
{
	return knowsSong[song];
}

std::string Continuity::getIngredientGfx(const std::string &name)
{
	IngredientData *i = getIngredientDataByName(name);
	if (i)
	{
		return i->gfx;
	}
	return "";
}

void Continuity::update(float dt)
{
	if (game->isActive())
		seconds += dt;

	if (statsAndAchievements) {
		statsAndAchievements->update(dt);
		statsAndAchievements->RunFrame();
	}

	if (game->isActive() && !game->isPaused() )
	{

		if (liPowerTimer.updateCheck(dt))
		{
			liPower = 0;
		}

		if (speedMultTimer.updateCheck(dt))
		{
			speedMult = 1;
		}

		if (lightTimer.updateCheck(dt))
		{
			light = 0;
		}

		if (petPowerTimer.updateCheck(dt))
		{
			petPower = 0;
		}

		if (game->avatar && game->avatar->isInputEnabled())
		{
			if (poisonTimer.updateCheck(dt))
			{
				poison = 0;
			}

			if (poison)
			{
				if (poisonBitTimer.updateCheck(dt))
				{
					poisonBitTimer.start(poisonBitTimeAvatar);
					if (game->avatar)
					{
						core->sound->playSfx("poison");

						DamageData d;
						d.damage = poison * 0.2f;
						d.useTimer = 0;
						d.damageType = DT_ENEMY_ACTIVEPOISON;
						game->avatar->damage(d);

						dsq->spawnParticleEffect("PoisonBubbles", game->avatar->position);
					}
				}
			}

			if (webTimer.updateCheck(dt))
			{
				game->avatar->clearWeb();
			}

			if (game->avatar->web)
			{
				if (webBitTimer.updateCheck(dt))
				{
					webBitTimer.start(webBitTime);

					game->avatar->web->addPoint(game->avatar->position);
				}
			}
		}

		if (energyTimer.updateCheck(dt))
		{
			energyMult = 0;
		}

		if (defenseMultTimer.updateCheck(dt))
		{
			defenseMult = 1;
		}

		if (biteMultTimer.updateCheck(dt))
		{
			biteMult = 1;
		}

		if (fishPoisonTimer.updateCheck(dt))
		{
			fishPoison = 1;
		}

		if (tripTimer.updateCheck(dt))
		{
			game->avatar->removeTripEffects();
		}

		if (regenTimer.updateCheck(dt))
		{
		}

		if (invincibleTimer.updateCheck(dt))
		{
		}

		if (regenTimer.isActive())
		{

			{
				Avatar *a = game->avatar;
				if (a)
				{
					a->heal(dt*0.5f);
				}

			}
		}
	}


}

void Continuity::shiftWorlds()
{
	WorldType lastWorld = worldType;
	if (worldType == WT_NORMAL)
	{
		worldType = WT_SPIRIT;
		game->setWorldPaused(true);
	}
	else if (worldType == WT_SPIRIT)
	{
		worldType = WT_NORMAL;
		game->setWorldPaused(false);
	}
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		e->shiftWorlds(lastWorld, worldType);
	}
	applyWorldEffects(worldType, 1, 1);
	if (worldType == WT_SPIRIT)
		core->sound->playSfx("Spirit-Enter");
	else if (worldType == WT_NORMAL)
		core->sound->playSfx("Spirit-Return");
}

BeaconData *Continuity::getBeaconByIndex(int index)
{
	for (Beacons::iterator i = beacons.begin(); i != beacons.end(); i++)
	{
		if ((*i).index == index)
		{
			return &(*i); // stupidity
		}
	}
	return 0;
}

void Continuity::setBeacon(int index, bool on, Vector pos, Vector color)
{
	if (on)
	{
		BeaconData *b = getBeaconByIndex(index);
		if (!b)
		{
			BeaconData newb;
			newb.index = index;
			beacons.push_back(newb);
			b = getBeaconByIndex(index);
		}
		b->on = true;
		b->pos = pos;
		b->color = color;
	}
	else
	{
		BeaconData *b = getBeaconByIndex(index);
		if (b)
		{
			b->on = false;
		}
	}
}

void Continuity::applyWorldEffects(WorldType type, bool transition, bool affectMusic)
{
	float time = 1;
	if (!transition) time = 0;
	if (type == WT_SPIRIT)
	{
		game->avatar->canWarp = false;

		game->sceneColor.interpolateTo(Vector(0.4f, 0.8f, 0.9f), time);
		game->avatar->applyWorldEffects(type);
	}
	else
	{
		game->avatar->canWarp = true;

		game->sceneColor.interpolateTo(Vector(1,1,1), time);
		game->avatar->applyWorldEffects(type);
	}
	if (time > 0)
	{

	}
	worldType = type;
}

void Continuity::eatBeast(const EatData &eatData)
{
	if (!eatData.name.empty())
	{
		for (int i = 0; i < eatData.getUnits; i++)
		{
			if (naijaEats.size() < MAX_EATS)
			{
				if (!eatData.shot.empty())
					naijaEats.push_back(eatData);
			}
		}

		if (eatData.health > 0)
		{
			game->avatar->heal(eatData.health);
		}
	}
}

void Continuity::removeNaijaEat(size_t idx)
{
	std::vector<EatData> copy = naijaEats;
	naijaEats.clear();
	for (size_t i = 0; i < copy.size(); i++)
	{
		if (i != idx)
			naijaEats.push_back(copy[i]);
	}
}

void Continuity::removeLastNaijaEat()
{
	removeNaijaEat(naijaEats.size()-1);
}

EatData *Continuity::getLastNaijaEat()
{
	if (naijaEats.empty())
		return 0;
	return &naijaEats[naijaEats.size()-1];
}

bool Continuity::isNaijaEatsEmpty()
{
	return naijaEats.empty();
}

void Continuity::init()
{
	statsAndAchievements = new StatsAndAchievements;
}

void Continuity::shutdown()
{
	if (statsAndAchievements)
	{
		delete statsAndAchievements;
		statsAndAchievements = 0;
	}
}

void Continuity::initAvatar(Avatar *a)
{
	debugLog("in initAvatar");
	a->maxHealth = maxHealth;
	a->health = 0;
	a->heal(health);

	// block spirit form in case of bug
	if (form == FORM_SPIRIT)
		form = FORM_NORMAL;

	debugLog("changeForm...");
	a->changeForm(form, false, true, FORM_NORMAL);
	debugLog("done");

	debugLog("auraType...");
	if (auraType != AURA_NONE && auraTimer > 0)
	{
		a->activateAura(auraType);
		a->auraTimer = auraTimer;
	}

	debugLog("trip");
	if (tripTimer.isActive())
	{
		a->applyTripEffects();
	}

	debugLog("web");
	if (webTimer.isActive())
	{
		setWeb(webTimer.getValue());
	}
	debugLog("done initAvatar");

	// HACK-ish
	a->skeletalSprite.stopAllAnimations();
	a->skeletalSprite.animate(a->getIdleAnimName(), -1, 0);
}

void Continuity::spawnAllIngredients(const Vector &position)
{
	for (size_t i = 0; i < ingredientData.size(); i++)
	{
		game->spawnIngredient(ingredientData[i]->name, position, 4, 0);
	}
}

void Continuity::removeEmptyIngredients()
{
	for (IngredientDatas::iterator i = ingredients.begin(); i != ingredients.end();)
	{
		IngredientData *data = *i;
		if (data->amount == 0 && data->held <= 0)
		{
			i = ingredients.erase(i);
		}
		else
		{
			++ i;
		}
	}
}

void Continuity::refreshAvatarData(Avatar *a)
{
	maxHealth = a->maxHealth;
	health = a->health;
	auraType = a->activeAura;
	auraTimer = a->auraTimer;
}

int Continuity::getFlag(std::string flag)
{
	if (flag == "story")
		errorLog("Hey! Use the new fancy story functions!");
	return flags[flag];
}

void Continuity::setFlag(std::string flag, int v)
{
	flags[flag] = v;
}



void Continuity::loadPetData()
{
	debugLog("Load pet data...");
	petData.clear();
	InStream in("data/pets.txt");
	std::string read;
	while (std::getline(in, read))
	{
		int num=0;
		PetData p;
		std::istringstream is(read);
		is >> num >> p.namePart;
		petData.push_back(p);
	}
	in.close();
}

PetData *Continuity::getPetData(size_t idx)
{
	if (idx >= petData.size())
	{
		std::ostringstream os;
		os << "getPetData(" << idx << ") index out of range";
		debugLog(os.str());
		return 0;

	}

	return &petData[idx];
}

bool Continuity::isStory(float v)
{
	return (story == v);
}

float Continuity::getStory()
{
	return story;
}

void Continuity::setStory(float v)
{
	story = v;
}

std::string Continuity::getStringFlag(std::string flag)
{
	return stringFlags[flag];
}

void Continuity::setStringFlag(std::string flag, std::string v)
{
	stringFlags[flag] = v;
}

void Continuity::clearTempFlags()
{
	for (Flags::iterator i = flags.begin(); i != flags.end(); i++)
	{
		if ((*i).first.find("CHOICE_")!=std::string::npos)
		{
			(*i).second = 0;
		}
	}
}

void Continuity::upgradeHealth()
{
	Avatar *a = game->avatar;
	maxHealth = a->maxHealth+1;
	a->maxHealth = maxHealth;
	a->heal(maxHealth - a->health);
}

void Continuity::saveFile(int slot, Vector position, unsigned char *scrShotData, int scrShotWidth, int scrShotHeight)
{
	refreshAvatarData(game->avatar);

	if (position.isZero())
	{
		position = game->avatar->position;
	}

	dsq->user.save();

	XMLDocument doc;

	XMLElement *version = doc.NewElement("Version");
	{
		version->SetAttribute("major",		VERSION_MAJOR);
		version->SetAttribute("minor",		VERSION_MINOR);
		version->SetAttribute("revision",	VERSION_REVISION);
	}
	doc.InsertEndChild(version);

	for (Flags::iterator i = flags.begin(); i != flags.end(); i++)
	{
		if ((*i).first.find("CHOICE_")!=std::string::npos) continue;
		if ((*i).first.find("TEMP_")!=std::string::npos) continue;
		XMLElement *flag = doc.NewElement("Flag");
		flag->SetAttribute("name", (*i).first.c_str());
		flag->SetAttribute("value", (*i).second);
		doc.InsertEndChild(flag);
	}

	XMLElement *efx = doc.NewElement("EFX");
	{
		std::ostringstream os;
		for (EntityFlags::iterator i = entityFlags.begin(); i != entityFlags.end(); i++)
		{
			os << (*i).first << " " << (*i).second << " ";
		}
		efx->SetAttribute("a", os.str().c_str());
	}
	doc.InsertEndChild(efx);

	XMLElement *gems = doc.NewElement("Gems");
	{
		// old gems format; used in 1.1.x <= 1.1.3
		{
			std::ostringstream os;
			bool hasUserString = false;
			os << this->gems.size() << " ";
			for (Gems::iterator i = this->gems.begin(); i != this->gems.end(); i++)
			{
				os << (*i).name << " " << (*i).pos.x << " " << (*i).pos.y << " ";
				os << (*i).canMove << " ";

				hasUserString = !(*i).userString.empty();

				os << hasUserString << " ";

				if (hasUserString)
					os << spacesToUnderscores((*i).userString) << " ";
			}
			gems->SetAttribute("c", os.str().c_str());
		}

		// This is the format used in the android version.
		{
			std::ostringstream os;
			os << this->gems.size() << " ";
			for (Gems::iterator i = this->gems.begin(); i != this->gems.end(); i++)
			{
				os << (*i).name << " ";
				bool hasMapName = !(*i).mapName.empty();
				os << hasMapName << " ";
				if(hasMapName)
					os << (*i).mapName << " "; // warning: this will fail to load if the map name contains whitespace

				os << (*i).pos.x << " " << (*i).pos.y << " ";
				os << (*i).canMove << " ";

				bool hasUserString = !(*i).userString.empty();
				os << hasUserString << " ";
				if (hasUserString)
					os << spacesToUnderscores((*i).userString) << " ";
			}
			gems->SetAttribute("d", os.str().c_str());
		}

		// newest format; is aware if tile-relative position
		{

		}

	}
	doc.InsertEndChild(gems);

	XMLElement *worldMap = doc.NewElement("WorldMap");
	{
		std::ostringstream os;
		for (size_t i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
		{
			WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
			if (tile->revealed)
			{
				os << tile->index << " ";
			}
		}
		worldMap->SetAttribute("b", os.str().c_str());

		if (game->worldMapRender)
		{
			std::ostringstream os;
			for (size_t i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
			{
				WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
				os << tile->index << " ";
				tile->dataToString(os);
				os << " ";
			}
			worldMap->SetAttribute("va", os.str().c_str());
		}
	}
	doc.InsertEndChild(worldMap);

	XMLElement *vox = doc.NewElement("VO");
	{
		std::ostringstream os;
		for (size_t i = 0; i < dsq->continuity.voiceOversPlayed.size(); i++)
		{

			os << dsq->continuity.voiceOversPlayed[i] << " ";
		}
		vox->SetAttribute("v", os.str().c_str());
	}
	doc.InsertEndChild(vox);

	XMLElement *eats = doc.NewElement("eats");
	{
		std::ostringstream os;
		int num = naijaEats.size();
		os << num << " ";
		for (int i = 0; i < num; i++)
		{
			EatData *eat = &naijaEats[i];
			os << eat->name << " "  << eat->shot << " " << eat->ammo << " " << eat->ammoUnitSize << " ";
		}
		eats->SetAttribute("a", os.str().c_str());
	}
	doc.InsertEndChild(eats);

	XMLElement *bcn = doc.NewElement("bcn");
	{
		std::ostringstream os;
		for (Beacons::iterator i = beacons.begin(); i != beacons.end(); i++)
		{
			BeaconData *data = &(*i);
			os << data->index << " " << data->on << " ";
			os << data->color.x << " " << data->color.y << " " << data->color.z << " ";
			os << data->pos.x << " " << data->pos.y << " " << data->pos.z << " ";
		}
		bcn->SetAttribute("a", os.str().c_str());
	}
	doc.InsertEndChild(bcn);

	XMLElement *s = doc.NewElement("Story");
	{
		std::ostringstream os;
		os << story;
		s->SetAttribute("v", os.str().c_str());
		doc.InsertEndChild(s);
	}

	for (StringFlags::iterator i = stringFlags.begin(); i != stringFlags.end(); i++)
	{
		if ((*i).first.find("TEMP_")!=std::string::npos) continue;
		XMLElement *stringFlag = doc.NewElement("StringFlag");
		stringFlag->SetAttribute("name", (*i).first.c_str());
		stringFlag->SetAttribute("value", (*i).second.c_str());
		doc.InsertEndChild(stringFlag);
	}

	XMLElement *startData = doc.NewElement("StartData");
	startData->SetAttribute("x", int(position.x));
	startData->SetAttribute("y", int(position.y));
	startData->SetAttribute("scene", game->sceneName.c_str());
	startData->SetAttribute("sceneDisplayName", game->sceneDisplayName.c_str());
	startData->SetAttribute("exp", dsq->continuity.exp);
	startData->SetAttribute("h", dsq->continuity.maxHealth);
	startData->SetAttribute("ch", dsq->continuity.health);
	startData->SetAttribute("costume", dsq->continuity.costume.c_str());
	startData->SetAttribute("form", dsq->continuity.form);
	if (dsq->mod.isActive())
		startData->SetAttribute("mod", dsq->mod.getName().c_str());
	std::ostringstream secondsOS;
	secondsOS << dsq->continuity.seconds;
	startData->SetAttribute("seconds", secondsOS.str().c_str());
	std::ostringstream os2;
	for (int i = 0; i < SONG_MAX; i++)
	{
		if (knowsSong[i])
		{
			os2 << i << " ";
		}
	}
	startData->SetAttribute("songs", os2.str().c_str());

	// new format as used by android version
	std::ostringstream ingrNames;
	for (size_t i = 0; i < ingredients.size(); i++)
	{
		IngredientData *data = ingredients[i];
		ingrNames << data->name << " " << data->amount << " ";
	}
	startData->SetAttribute("ingrNames", ingrNames.str().c_str());

	// for compatibility with older versions
	std::ostringstream ingrOs;
	for (size_t i = 0; i < ingredients.size(); i++)
	{
		IngredientData *data = ingredients[i];
		ingrOs << data->getIndex() << " " << data->amount << " ";
	}
	startData->SetAttribute("ingr", ingrOs.str().c_str());

	std::ostringstream recOs;
	for (size_t i = 0; i < recipes.size(); i++)
	{
		recOs << recipes[i].isKnown() << " ";
	}
	startData->SetAttribute("rec", recOs.str().c_str());

	std::ostringstream os3;
	for (int i = 0; i < FORMUPGRADE_MAX; i++)
	{
		if (hasFormUpgrade((FormUpgradeType)i))
		{
			os3 << i << " ";
		}
	}
	startData->SetAttribute("formUpgrades", os3.str().c_str());

	std::ostringstream fos;
	fos << MAX_FLAGS << " ";
	for (int i = 0; i < MAX_FLAGS; i++)
	{
		fos << intFlags[i] << " ";
	}
	startData->SetAttribute("intFlags", fos.str().c_str());

	// Additional data for the android version

#define SINGLE_FLOAT_ATTR(name, cond, val) \
	do { if((cond) && (val)) { \
		std::ostringstream osf; \
		osf << (val); \
		startData->SetAttribute(name, osf.str().c_str()); \
	}} while(0)

	SINGLE_FLOAT_ATTR("blind", game->avatar->state.blind, game->avatar->state.blindTimer.getValue());
	SINGLE_FLOAT_ATTR("invincible", invincibleTimer.isActive(), invincibleTimer.getValue());
	SINGLE_FLOAT_ATTR("regen", regenTimer.isActive(), regenTimer.getValue());
	SINGLE_FLOAT_ATTR("trip", tripTimer.isActive(), tripTimer.getValue());
	SINGLE_FLOAT_ATTR("shieldPoints", true, game->avatar->shieldPoints);
	SINGLE_FLOAT_ATTR("webTimer", webTimer.isActive(), webTimer.getValue()); // Extension; not present in the android version

#undef SINGLE_FLOAT_ATTR

#define TIMER_AND_VALUE_ATTR(name, timer, val) \
	do { if(((timer).isActive()) && (val)) { \
		std::ostringstream osf; \
		osf << (val) << " " << ((timer).getValue()); \
		startData->SetAttribute((name), osf.str().c_str()); \
	}} while(0)

	TIMER_AND_VALUE_ATTR("biteMult", biteMultTimer, biteMult);
	TIMER_AND_VALUE_ATTR("speedMult", speedMultTimer, speedMult);
	TIMER_AND_VALUE_ATTR("defenseMult", defenseMultTimer, defenseMult);
	TIMER_AND_VALUE_ATTR("energyMult", energyTimer, energyMult);
	TIMER_AND_VALUE_ATTR("petPower", petPowerTimer, petPower);
	TIMER_AND_VALUE_ATTR("liPower", liPowerTimer, liPower);
	TIMER_AND_VALUE_ATTR("light", lightTimer, light);

#undef TIMER_AND_VALUE_ATTR

	if(poisonTimer.isActive())
	{
		std::ostringstream osp;
		osp << poison << " " << poisonTimer.getValue() << " " << poisonBitTimer.getValue();
		startData->SetAttribute("poison", osp.str().c_str());
	}

	if(game->avatar->activeAura != AURA_NONE)
	{
		std::ostringstream osa;
		osa << game->avatar->activeAura << " " << game->avatar->auraTimer;
		startData->SetAttribute("aura", osa.str().c_str());
	}

	// FIXME: Web is a bit weird. There are 2 webBitTimer variables in use, one in Continuity, one in Avatar.
	// Because the avatar one ticks every 0.5 seconds, it will be hardly noticeable if that timer is off.
	// So we just use the Continuty timers and hope for the best. -- FG
	if(webTimer.isActive() && game->avatar->web)
	{
		Web *w = game->avatar->web;
		const int nump = w->getNumPoints();
		std::ostringstream osw;
		osw << webBitTimer.getValue() << " " << nump << " ";
		for(int i = 0; i < nump; ++i)
		{
			Vector v = w->getPoint(i);
			osw << v.x << " " << v.y << " ";
		}
		startData->SetAttribute("web", osw.str().c_str());
	}

	// end extra android data

	doc.InsertEndChild(startData);


	std::string fn = adjustFilenameCase(getSaveFileName(slot, "aqs"));
	FILE *fh = fopen(fn.c_str(), "wb");
	if(!fh)
	{
		debugLog("FAILED TO SAVE GAME");
		return;
	}

	XMLPrinter printer;
	doc.Accept( &printer );
	const char* xmlstr = printer.CStr();
	ZlibCompressor z;
	z.init((void*)xmlstr, printer.CStrSize(), ZlibCompressor::REUSE);
	z.SetForceCompression(true);
	z.Compress(3);
	std::ostringstream os;
	os << "Writing " << z.size() << " bytes to save file " << fn;
	debugLog(os.str());
	size_t written = fwrite(z.contents(), 1, z.size(), fh);
	if (written != z.size())
	{
		debugLog("FAILED TO WRITE SAVE FILE COMPLETELY");
	}
	fclose(fh);
}

std::string Continuity::getSaveFileName(int slot, const std::string &pfix)
{
	std::ostringstream os;
	os << dsq->getSaveDirectory() << "/save-" << numToZeroString(slot, 4) << "." << pfix;
	return os.str();
}

bool Continuity::loadFileData(int slot, XMLDocument &doc)
{
	std::string teh_file = dsq->continuity.getSaveFileName(slot, "aqs");
	if(!exists(teh_file))
		teh_file = dsq->continuity.getSaveFileName(slot, "bin");

	std::string err = "file not exist";

	if (exists(teh_file))
	{
		size_t size = 0;
		char *buf = readCompressedFile(teh_file.c_str(), &size);
		if (!buf)
		{
			errorLog("Failed to decompress save file: " + teh_file);
			return false;
		}
		bool good = doc.Parse(buf, size) == XML_SUCCESS;
		delete [] buf;
		if (good)
			return true;
		err =  doc.GetErrorStr1();
	}
	else
	{
		teh_file = dsq->continuity.getSaveFileName(slot, "xml");
		if (exists(teh_file))
		{
			if (readXML(teh_file, doc) == XML_SUCCESS)
				return true;
			err =  doc.GetErrorStr1();
		}
		else // No save for this slot - no error
			return false;
	}

	debugLog("Failed to load save data: " + teh_file + " -- Error: " + err);
	return false;
}

bool Continuity::loadFile(int slot)
{
	dsq->user.save();

	XMLDocument doc;
	if(!loadFileData(slot, doc))
		return false;

	XMLElement *startData = doc.FirstChildElement("StartData");
	if (startData)
	{
		if (startData->Attribute("mod"))
		{
#ifdef AQUARIA_DEMO
			exit_error("The demo version does not support loading savegames from mods, sorry.");
#else
			if(!dsq->mod.loadSavedGame(startData->Attribute("mod")))
				return false;
#endif
		}
	}

	this->reset();

	XMLElement *e = doc.FirstChildElement("Flag");
	while (e)
	{
		dsq->continuity.setFlag(e->Attribute("name"), atoi(e->Attribute("value")));
		e = e->NextSiblingElement("Flag");
	}

	XMLElement *efx = doc.FirstChildElement("EFX");
	if (efx)
	{
		if (efx->Attribute("a"))
		{
			std::istringstream is(efx->Attribute("a"));
			std::string name;
			while (is >> name)
			{
				is >> entityFlags[name];
			}
		}
	}

	XMLElement *eats = doc.FirstChildElement("eats");
	if (eats)
	{
		if (eats->Attribute("a"))
		{
			std::istringstream is(eats->Attribute("a"));
			int num = 0;
			naijaEats.clear();
			is >> num;
			for (int i = 0; i < num; i++)
			{
				EatData eat;
				is >> eat.name >> eat.shot >> eat.ammo >> eat.ammoUnitSize;
				naijaEats.push_back(eat);
			}
		}
	}

	XMLElement *bcn = doc.FirstChildElement("bcn");
	if (bcn)
	{
		if (bcn->Attribute("a"))
		{
			beacons.clear();

			std::istringstream is(bcn->Attribute("a"));
			int idx=0;
			while (is >> idx)
			{
				BeaconData data;

				data.index = idx;
				is >> data.on;
				is >> data.color.x >> data.color.y >> data.color.z;
				is >> data.pos.x >> data.pos.y >> data.pos.z;

				beacons.push_back(data);
			}
		}
	}

	XMLElement *vox = doc.FirstChildElement("VO");
	if (vox)
	{
		std::string s = vox->Attribute("v");
		std::istringstream is(s);
		std::string v;
		while (is >> v)
		{
			dsq->continuity.voiceOversPlayed.push_back(v);
		}
	}

	XMLElement *gems = doc.FirstChildElement("Gems");
	if (gems)
	{
		if (gems->Attribute("a"))
		{
			std::string s = gems->Attribute("a");
			std::istringstream is(s);
			GemData g;
			while (is >> g.name)
			{
				is >> g.pos.x >> g.pos.y;
				this->gems.push_back(g);
			}
		}
		else if (gems->Attribute("b"))
		{
			std::string s = gems->Attribute("b");
			std::istringstream is(s);
			GemData g;
			bool hasUserString = false;
			while (is >> g.name)
			{
				hasUserString=false;

				is >> g.pos.x >> g.pos.y;
				is >> g.canMove;
				is >> hasUserString;

				if (hasUserString)
					is >> g.userString;

				std::ostringstream os;
				os << "Loading a Gem called [" << g.name << "] with userString [" << g.userString << "] pos (" << g.pos.x << ", " << g.pos.y << ")\n";
				debugLog(os.str());

				g.userString = underscoresToSpaces(g.userString);
				this->gems.push_back(g);
			}
		}
		// num [name mapX mapY canMove hasUserString (userString)]
		else if (gems->Attribute("c"))
		{
			std::string s = gems->Attribute("c");
			std::istringstream is(s);

			int num = 0;
			is >> num;

			bool hasUserString = false;
			GemData g;

			std::ostringstream os;
			os << "continuity num: [" << num << "]" << std::endl;
			os << "data: [" << s << "]" << std::endl;
			debugLog(os.str());

			for (int i = 0; i < num; i++)
			{
				g.pos = Vector(0,0,0);
				g.canMove = false;
				g.userString = "";

				hasUserString=false;

				is >> g.name;
				is >> g.pos.x >> g.pos.y;
				is >> g.canMove;
				is >> hasUserString;

				if (hasUserString)
					is >> g.userString;
				else
					g.userString = "";

				g.userString = underscoresToSpaces(g.userString);
				this->gems.push_back(g);

				std::ostringstream os;
				os << "Loading a Gem called [" << g.name << "] with userString [" << g.userString << "] pos (" << g.pos.x << ", " << g.pos.y << ")\n";
				debugLog(os.str());
			}
		}
		// num [name hasMapName (mapName) mapX mapY canMove hasUserString (userString)]
		else if (gems->Attribute("d"))
		{
			std::string s = gems->Attribute("d");
			std::istringstream is(s);

			int num = 0;
			is >> num;

			bool hasUserString = false;
			bool hasMapName = false;
			GemData g;

			std::ostringstream os;
			os << "continuity num: [" << num << "]" << std::endl;
			os << "data: [" << s << "]" << std::endl;
			debugLog(os.str());

			for (int i = 0; i < num; i++)
			{
				g.pos = Vector(0,0,0);
				g.canMove = false;
				g.userString = "";
				g.mapName = "";

				hasUserString=false;
				hasMapName = false;

				is >> g.name;
				is >> hasMapName;
				if(hasMapName)
					is >> g.mapName;

				is >> g.pos.x >> g.pos.y;
				is >> g.canMove;
				is >> hasUserString;

				if (hasUserString)
					is >> g.userString;

				g.userString = underscoresToSpaces(g.userString);
				this->gems.push_back(g);

				std::ostringstream os;
				os << "Loading a Gem called [" << g.name << "] with userString [" << g.userString << "] pos (" << g.pos.x << ", " << g.pos.y << ")\n";
				debugLog(os.str());
			}
		}
	}

	XMLElement *worldMap = doc.FirstChildElement("WorldMap");
	if (worldMap)
	{
		if (worldMap->Attribute("b"))
		{
			std::string s = worldMap->Attribute("b");
			std::istringstream is(s);
			int idx;
			while (is >> idx)
			{
				dsq->continuity.worldMap.revealMapIndex(idx);
			}
		}


		if (worldMap->Attribute("va") && dsq->continuity.worldMap.getNumWorldMapTiles())
		{
			std::istringstream is(worldMap->Attribute("va"));

			WorldMapTile dummy;

			int idx;



			while (is >> idx)
			{
				WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(idx);

				if (!tile)
				{
					std::ostringstream os;
					os << "tile dummy: dropping data for worldmap tile index " << idx;
					debugLog(os.str());
					tile = &dummy;
				}

				tile->stringToData(is);
			}
		}
	}


	XMLElement *s = doc.FirstChildElement("Story");
	if (s)
	{
		std::istringstream is(s->Attribute("v"));
		is >> story;
	}

	XMLElement *e2 = doc.FirstChildElement("StringFlag");
	while (e2)
	{
		dsq->continuity.setStringFlag(e2->Attribute("name"), e2->Attribute("value"));
		e2 = e2->NextSiblingElement("StringFlag");
	}

	if (startData)
	{
		int x = atoi(startData->Attribute("x"));
		int y = atoi(startData->Attribute("y"));
		game->positionToAvatar = Vector(x,y);
		if (startData->Attribute("exp"))
			exp = atoi(startData->Attribute("exp"));

		if (startData->Attribute("naijaModel"))
		{
			//dsq->continuity.naijaModel = startData->Attribute("naijaModel");
		}

		if (startData->Attribute("form"))
		{
			dsq->continuity.form = FormType(atoi(startData->Attribute("form")));
		}

		if (startData->Attribute("ingrNames"))
		{
			std::istringstream is(startData->Attribute("ingrNames"));
			std::string name;
			while (is >> name)
			{
				int amount=0;
				is >> amount;
				IngredientData *data = getIngredientDataByName(name);
				if (data)
				{
					data->amount = 0;
					pickupIngredient(data, amount, false);
				}
			}
		}
		else if (startData->Attribute("ingr")) // use this only if ingrNames does not exist.
		{
			std::istringstream is(startData->Attribute("ingr"));
			int idx;
			while (is >> idx)
			{
				int amount=0;
				is >> amount;
				IngredientData *data = getIngredientDataByIndex(idx);
				if (data)
				{
					data->amount = 0;
					pickupIngredient(data, amount, false);
				}
			}
		}

		if (startData->Attribute("rec"))
		{
			std::istringstream is(startData->Attribute("rec"));

			for (size_t i = 0; i < recipes.size(); i++)
			{
				bool known = false;
				is >> known;
				if (known)
					recipes[i].learn();
			}
		}

		if (startData->Attribute("songs"))
		{
			std::istringstream is(std::string(startData->Attribute("songs")));
			int v=0;
			while (is >> v)
			{
				knowsSong[v] = true;
			}
		}

		if (startData->Attribute("formUpgrades"))
		{
			std::istringstream is(std::string(startData->Attribute("formUpgrades")));
			int v = 0;
			while (is >> v)
			{
				learnFormUpgrade(FormUpgradeType(v));
			}
		}

		if (startData->Attribute("intFlags"))
		{
			std::istringstream is(std::string(startData->Attribute("intFlags")));
			int numFlags;
			is >> numFlags;
			if (numFlags > MAX_FLAGS)
				numFlags = MAX_FLAGS;
			for (int i = 0; i < numFlags; i++)
			{
				is >> intFlags[i];
			}
		}

		if (startData->Attribute("h"))
		{
			float read = strtof(startData->Attribute("h"), NULL);
			maxHealth = read;
			health = read;
			std::ostringstream os;
			os << "MaxHealth read as: " << maxHealth;
			debugLog(os.str());

			if (game->avatar)
			{
				game->avatar->maxHealth = maxHealth;
				game->avatar->health = maxHealth;
			}
		}

		if (startData->Attribute("ch"))
		{
			float h = strtof(startData->Attribute("ch"), NULL);
			health = h;
			std::ostringstream os;
			os << "CurHealth read as: " << health;
			debugLog(os.str());

			if (game->avatar)
			{
				game->avatar->health = h;
			}
		}

		if (startData->Attribute("seconds"))
		{
			std::istringstream is(startData->Attribute("seconds"));
			is >> dsq->continuity.seconds;
		}
		if (startData->Attribute("costume"))
		{
			dsq->continuity.costume = startData->Attribute("costume");
		}

		game->sceneToLoad = startData->Attribute("scene");

		// Additional data introduced in the android version

		if(startData->Attribute("blind"))
		{
			float timer = strtof(startData->Attribute("blind"), NULL);
			if(game->avatar)
				game->avatar->setBlind(timer);
		}

		if(startData->Attribute("invincible"))
		{
			float timer = strtof(startData->Attribute("invincible"), NULL);
			setInvincible(timer);
		}

		if(startData->Attribute("regen"))
		{
			float timer = strtof(startData->Attribute("regen"), NULL);
			setRegen(timer);
		}

		if(startData->Attribute("trip"))
		{
			float timer = strtof(startData->Attribute("trip"), NULL);
			setTrip(timer);
		}

		if(startData->Attribute("aura"))
		{
			std::istringstream is(startData->Attribute("aura"));
			int type = AURA_NONE;
			float timer = 0.0f;
			is >> type >> timer;
			auraTimer = timer;
			auraType = (AuraType)type;
			if(game->avatar)
			{
				game->avatar->activateAura((AuraType)type);
				game->avatar->auraTimer = timer;
			}
		}

		if(startData->Attribute("shieldPoints"))
		{
			float sp = strtof(startData->Attribute("shieldPoints"), NULL);
			if(game->avatar)
				game->avatar->shieldPoints = sp;
		}

#define LOAD_MULTI_SIMPLE(attr, mth) \
		do { if(startData->Attribute(attr)) \
		{ \
			std::istringstream is(startData->Attribute(attr)); \
			float value = 0.0f, timer = 0.0f; \
			is >> value >> timer; \
			this->mth(value, timer); \
		}} while(0)

		LOAD_MULTI_SIMPLE("biteMult", setBiteMultiplier);
		LOAD_MULTI_SIMPLE("speedMult", setSpeedMultiplier);
		LOAD_MULTI_SIMPLE("defenseMult", setDefenseMultiplier);
		LOAD_MULTI_SIMPLE("energyMult", setEnergy);
		LOAD_MULTI_SIMPLE("petPower", setPetPower);
		LOAD_MULTI_SIMPLE("liPower", setLiPower);
		LOAD_MULTI_SIMPLE("light", setLight);

#undef LOAD_MULTI_SIMPLE

		if(startData->Attribute("poison"))
		{
			std::istringstream is(startData->Attribute("poison"));
			float p = 0.0f, pt = 0.0f, pbit = 0.0f;
			is >> p >> pt >> pbit;
			setPoison(p, pt);
			poisonBitTimer.start(pbit);
		}

		// FIXME: the total web time is seemingly not saved in the file.
		// Not sure if the calculation of the remaining time is correct.
		// Especially because there are two webBitTimer variables in use (in Continuity and Avatar),
		// and both of them access the avatar web. It's thus likely that more points were added than intended. -- FG
		if(startData->Attribute("web"))
		{
			std::istringstream is(startData->Attribute("web"));
			float wbit = 0.0f;
			int nump = 0;
			is >> wbit >> nump;
			// 2 web points are added in setWeb() by default, so we exclude them from the calculation
			float remainTime = webTime - (0.5 * (nump - 2)); // Avatar::webBitTimer ticks every 0.5 secs
			if(nump > 1 && remainTime > 0 && game->avatar)
			{
				if(!game->avatar->web)
					game->avatar->createWeb();
				Web *w = game->avatar->web;
				for(int i = 0; i < nump; ++i)
				{
					Vector v;
					is >> v.x >> v.y;
					if(i < w->getNumPoints())
						w->setPoint(i, v);
					else
						w->addPoint(v);
				}
				webBitTimer.start(wbit);
				webTimer.start(remainTime);
			}
		}

		// This is AFAIK not in the android version, but let's add this for completeness
		// and to avoid the mess described above.
		if(startData->Attribute("webTimer"))
		{
			float timer = strtof(startData->Attribute("webTimer"), NULL);
			webTimer.start(timer);
		}
	}
	return true;
}

int Continuity::getFlag(int flag)
{
	if (flag == 0)
	{
		errorLog("Flag 0 not allowed");
	}

	return intFlags[flag];
}

void Continuity::setFlag(int flag, int v)
{
	if (flag == 0)
	{
		errorLog("Flag 0 not allowed");
	}
	std::ostringstream os;
	os << "setting flag [" << flag << "] to " << v;
	debugLog(os.str());
	intFlags[flag] = v;
}

int Continuity::getEntityFlag(const std::string &sceneName, int id)
{
	std::ostringstream os;
	os << sceneName << ":" << id;

	std::ostringstream os2;
	os2 << hash(os.str());

	return entityFlags[os2.str()];
}

void Continuity::setEntityFlag(const std::string &sceneName, int id, int v)
{
	std::ostringstream os;
	os << sceneName << ":" << id;

	std::ostringstream os2;
	os2 << hash(os.str());

	entityFlags[os2.str()] = v;
}

void Continuity::setPathFlag(Path *p, int v)
{
	std::ostringstream os;
	os << "p:" << game->sceneName << ":" << p->nodes[0].position.x << ":" << p->nodes[0].position.y << ":" << removeSpaces(p->name);

	std::ostringstream os2;
	os2 << hash(os.str());

	entityFlags[os2.str()] = v;
}

int Continuity::getPathFlag(Path *p)
{
	std::ostringstream os;
	os << "p:" << game->sceneName << ":" << p->nodes[0].position.x << ":" << p->nodes[0].position.y << ":" << removeSpaces(p->name);
	std::ostringstream os2;
	os2 << hash(os.str());
	return entityFlags[os2.str()];
}

class GemGet : public Quad
{
public:
	GemGet(const std::string &gem)
	{
		timeScale = 3;

		timer = 0;



		setTexture("Gems/" + gem);

		followCamera = 1;

		scale = Vector(0, 0);
		scale.ensureData();
		scale.data->path.addPathNode(Vector(0,0), 0);
		scale.data->path.addPathNode(Vector(1,1), 0.3f);
		scale.data->path.addPathNode(Vector(1,1), 0.6f);
		scale.data->path.addPathNode(Vector(0.5f,0.5f), 0.9f);
		scale.data->path.addPathNode(Vector(0.1f,0.1f), 1);
		scale.startPath(timeScale);

		position = Vector(400,400);

		setLife(timeScale+0.1f);
		setDecayRate(1);


	}
protected:
	float timer;
	float timeScale;
	Vector startPos;
	void onUpdate(float dt)
	{
		Quad::onUpdate(dt);

		timer += dt;
		if (timer > 0.6f*timeScale)
		{
			if (startPos.isZero())
			{
				startPos = position;
			}
			else
			{
				float p = (timer - (0.6f*timeScale)) / (0.4f*timeScale);
				position = ((game->miniMapRender->position + game->miniMapRender->offset) - startPos)*p + startPos;
			}
		}
	}
};

// WARNING: invalidates pointers to gems!
// BUT: maybe not
void Continuity::removeGemData(GemData *gemData)
{
	for (Gems::iterator i = gems.begin(); i != gems.end(); i++)
	{
		if (&(*i) == gemData)
		{
			gems.erase(i);
			return; // safety first
		}
	}
}

GemData *Continuity::pickupGem(std::string name, bool effects)
{
	GemData g;
	g.name = name;
	g.mapName = game->sceneName;
	int sz = gems.size();

	//HACK: (hacky) using effects to determine the starting position of the gem
	if (!effects)
	{
		g.pos = game->worldMapRender->getAvatarWorldMapPosition() + Vector(sz*16-64, -64);
	}
	else
	{
		if (!gems.empty())
			g.pos = game->worldMapRender->getAvatarWorldMapPosition();
		else
			g.pos = Vector(0,0);
	}

	gems.push_back(g);

	if (effects && game->isActive())
	{
		core->sound->playSfx("Gem-Collect");

		GemGet *gg = new GemGet(g.name);
		game->addRenderObject(gg, LR_MINIMAP);



		if (!getFlag("tokenHint"))
		{
			setFlag("tokenHint", 1);
			game->setControlHint(stringbank.get(4), false, false, false, 8);
		}


	}

	// return the last one
	return &gems.back();
}

void Continuity::entityDied(Entity *eDead)
{
	if (statsAndAchievements)
	{
		statsAndAchievements->entityDied(eDead);
	}
}

void Continuity::learnRecipe(Recipe *r, bool effects)
{
	bool k = r->isKnown();
	r->learn();

	std::ostringstream os;
	os << "learned recipe: " << r->result << " @ idx: " << r->index;
	debugLog(os.str());

	if (!k)
		game->learnedRecipe(r, effects);
}

void Continuity::learnRecipe(const std::string &result, bool effects)
{
	for (size_t i = 0; i < recipes.size(); i++)
	{
		if (nocasecmp(recipes[i].result, result)==0)
		{
			learnRecipe(&recipes[i], effects);
			return;
		}
	}
}

void Continuity::reset()
{
	dualFormMode = DUALFORM_LI;
	dualFormCharge = 0;

	if (game)
		game->onContinuityReset();



	speedMult = biteMult = fishPoison = defenseMult = 1;
	speedMult2 = 1;
	poison = 0;
	energyMult = 0;
	light = petPower = 0;
	liPower = 0;

	speedMultTimer.stop();
	biteMultTimer.stop();
	fishPoisonTimer.stop();
	defenseMultTimer.stop();
	invincibleTimer.stop();
	regenTimer.stop();
	tripTimer.stop();
	energyTimer.stop();
	poisonTimer.stop();
	poisonBitTimer.stop();
	webTimer.stop();
	webBitTimer.stop();
	lightTimer.stop();
	petPowerTimer.stop();

	loadTreasureData();

	dsq->loadStringBank();

	gems.clear();
	beacons.clear();

	worldMap.load();

	naijaEats.clear();
	foodSortType = 0;
	ingredients.clear();

	loadIngredientData(); // must be after clearing ingredients

	loadPetData();

	formUpgrades.clear();

	auraType = AURA_NONE;
	for (int i = 0; i < MAX_FLAGS; i++)
	{
		intFlags[i] = 0;
	}
	voiceOversPlayed.clear();

	entityFlags.clear();
	knowsSong.clear();
	loadSongBank();
	loadEatBank();
	dsq->loadElementEffects();
	form = FORM_NORMAL;
	costume = "";
	dsq->emote.load("data/naijaemote.txt");



	worldType = WT_NORMAL;

	zoom = Vector(1,1,0);
	itemSlots.clear();

	seconds = 0;
	exp = 0;
	hudVisible = true;

	flags.clear();

	stringFlags.clear();
	story = 0;

	maxHealth = 5;
	health = maxHealth;

	speedTypes.clear();
	debugLog("Load speedtypes...");
	InStream inFile("data/speedtypes.txt");
	int n;
	float spd;
	while (inFile >> n)
	{
		inFile >> spd;
		speedTypes.push_back(spd);
	}


	if (!dsq->mod.isActive())
	{
		learnSong(SONG_SHIELDAURA);
	}

	initFoodSort();

	core->resetTimer();
}

float Continuity::getSpeedType(size_t speedType)
{
	if (speedType >= speedTypes.size())
	{
		std::ostringstream os;
		os << "speedType: " << speedType << " out of range";
		debugLog(os.str());
		return 0;
	}
	return speedTypes[speedType];
}

void Continuity::achieve(const std::string &achievement)
{
}

void Continuity::flingMonkey(Entity *e)
{
	statsAndAchievements->flingMonkey(e);
}
