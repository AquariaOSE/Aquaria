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
#include "Game.h"

namespace RecipeMenuNamespace
{
	int pageSize = 4;

	std::string processFoodName(std::string name)
	{
		name = splitCamelCase(name);
		int p = name.find(' ');
		if (p != std::string::npos)
		{
			name[p] = '\n';
		}
		return name;
	}
}

using namespace RecipeMenuNamespace;
	
RecipeMenuEntry::RecipeMenuEntry(Recipe *recipe) : RenderObject(), recipe(recipe)
{
	selected = 0;
	//	Quad *result, *i1, *i2, *i3;
	//	Recipe *recipe;

	data = dsq->continuity.getIngredientDataByName(recipe->result);
	if (data)
	{
		glow = new Quad("particles/glow", Vector(-100, 0));
		glow->scale = Vector(4,4);
		glow->setBlendType(BLEND_ADD);
		glow->alphaMod = 0;
		addChild(glow, PM_POINTER);
		
		result = new Quad("ingredients/" + data->gfx, Vector(-100,0));
		result->scale = Vector(0.7, 0.7);
		addChild(result, PM_POINTER);
		
		BitmapText *text = new BitmapText(&dsq->smallFont);
		text->scale = Vector(0.7, 0.7);
		text->color = 0;
		text->position = result->position + Vector(0, 18);
		
		text->setText(processFoodName(data->name));
		addChild(text, PM_POINTER);
	}

	Quad *equals = new Quad("gui/recipe-equals", Vector(-50, 0));
	equals->scale = Vector(0.7, 0.7);
	addChild(equals, PM_POINTER);

	int c = 0;
	//int size = (recipe->names.size() + recipe->types.size())-1;
	int size=0;

	for (int i = 0; i < recipe->names.size(); i++)
		size += recipe->names[i].amount;

	for (int j = 0; j < recipe->types.size(); j++)
		size += recipe->types[j].amount;

	size --;

	
	for (int i = 0; i < recipe->names.size(); i++)
	{
		debugLog("recipe name: " + recipe->names[i].name);
		IngredientData *data = dsq->continuity.getIngredientDataByName(recipe->names[i].name);
		if (data)
		{
			for (int j = 0; j < recipe->names[i].amount; j++)
			{
				ing[c] = new Quad("ingredients/" + data->gfx, Vector(100*c,0));
				ing[c]->scale = Vector(0.7, 0.7);
				addChild(ing[c], PM_POINTER);
				
				BitmapText *text = new BitmapText(&dsq->smallFont);
				text->scale = Vector(0.7, 0.7);
				text->color = 0;
				text->position = ing[c]->position + Vector(0, 18);
				text->setText(processFoodName(data->name));
				addChild(text, PM_POINTER);
				
				if (c < size)
				{
					Quad *plus = new Quad("gui/recipe-plus", Vector(100*c+50, 0));
					plus->scale = Vector(0.7, 0.7);
					addChild(plus, PM_POINTER);
				}
				
				c++;
				if (c > 3)
				{
					errorLog(recipe->result + std::string("'s ingredient count exceeded"));
					break;
				}
			}
		}
	}
	
	for (int i = 0; i < recipe->types.size(); i++)
	{
		//debugLog("recipe type: " + recipe->types[i].typeName);
		for (int j = 0; j < recipe->types[i].amount; j++)
		{
			// any type of whatever...
			BitmapText *text = new BitmapText(&dsq->smallFont);
			text->color = 0;
			text->scale = Vector(0.8, 0.8);
			text->position = Vector(100*c, 0); //-20

			std::string typeName = recipe->types[i].typeName;

			int loc = typeName.find("Type");
			if (loc != std::string::npos)
			{
				typeName = typeName.substr(0, loc) + typeName.substr(loc+4, typeName.size());
			}


			if (typeName != "Anything")
				typeName = std::string("Any\n") + typeName;
			else
				typeName = std::string("\n") + typeName;

			text->setText(typeName);

			addChild(text, PM_POINTER);
			
			if (c < size)
			{
				Quad *plus = new Quad("gui/recipe-plus", Vector(100*c+50, 0));
				plus->scale = Vector(0.7, 0.7);
				addChild(plus, PM_POINTER);
			}
			
			c++;
		}	
	}

	description = 0;

	alpha = 0;
	alpha.interpolateTo(1, 0.2);

	shareAlphaWithChildren = 1;

	followCamera = 1;
}

void RecipeMenuEntry::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (!game->recipeMenu.description)
		return;

	if (alpha.x == 1)
	{
		Vector p = result->getWorldPosition();
		int w2 = 40, h2 = 32, w3 = 300;
		if (core->mouse.position.x > p.x - w2
			&& core->mouse.position.x < p.x + w3
			&& core->mouse.position.y > p.y - h2
			&& core->mouse.position.y < p.y + h2)
		{
			glow->alphaMod = 0.2;

			std::ostringstream ds;
			//ds << data->name << ": ";
			//ds << "* ";
			for (int i = 0; i < data->effects.size(); i++)
			{
				ds << dsq->continuity.getIEString(data, i);
				if (i == data->effects.size()-1)
				{
					// do nothing
					//ds << ".";
				}
				else
				{
					ds << ", ";
				}
			}

			game->recipeMenu.description->setText(ds.str());
			game->recipeMenu.description->offset = Vector(0, -10 * (game->recipeMenu.description->getNumLines()-1));
			// description->setText(ds.str());
			selected = 1;
		}
		else
		{
			glow->alphaMod = 0;
			selected = 0;

			int n=0;

			for (int i = 0; i < game->recipeMenu.recipeMenuEntries.size(); i++)
			{
				if (!game->recipeMenu.recipeMenuEntries[i]->selected)
					n++;
			}

			if (n == game->recipeMenu.recipeMenuEntries.size())
			{
				game->recipeMenu.description->setText("");
			}
		}
	}
}

RecipeMenu::RecipeMenu()
{
	on = false;
	currentPage = 0;
	description = 0;
}

void RecipeMenu::slide(RenderObject *r, bool in, float t)
{
	int oy=-640;
	if (in)
	{
		r->alpha = 1;
		//r->alpha.interpolateTo(1, 0.2);
		r->offset = Vector(0,oy);
		r->offset.interpolateTo(Vector(0,0), t, 0, 0, 1);
	}
	else
	{
		r->alpha.interpolateTo(0, 1);
		r->offset.interpolateTo(Vector(0,oy), t, 0, 0, 1);
	}
}

int RecipeMenu::getNumKnown()
{
	int num = 0;
	for (int i = 0; i < dsq->continuity.recipes.size(); i++)
	{
		if (dsq->continuity.recipes[i].isKnown())
		{
			num++;
		}
	}
	return num;
}

int RecipeMenu::getNumPages()
{
	int numKnown = dsq->continuity.recipes.size();//getNumKnown();
	int numPages = (numKnown/pageSize);

	/*
	for (int i = 0; i < dsq->continuity.recipes.size(); i++)
	{
		debugLog("r: " + dsq->continuity.recipes[i].result);
	}
	*/
	
	std::ostringstream os;
	os << "numKnown: " << numKnown << " pagesSize: " << pageSize << " numPages: " << numPages;
	debugLog(os.str());
	
	return numPages;
}

void RecipeMenu::goPrevPage()
{
	if (getNumPages() <= 1) return;

	dsq->sound->playSfx("recipemenu-pageturn");

	destroyPage();
	
	currentPage--;
	
	if (currentPage < 0)
		currentPage = getNumPages();
		
	createPage(currentPage);
}

void RecipeMenu::goNextPage()
{
	if (getNumPages() <= 1) return;

	dsq->sound->playSfx("recipemenu-pageturn");

	destroyPage();
	
	currentPage++;
	
	int pages = getNumPages();
	if (currentPage > pages)
	{
		currentPage = 0;
	}
	
	createPage(currentPage);
}

void RecipeMenu::toggle(bool on, bool watch)
{
	if (dsq->isNested()) return;

	static bool toggling = false;

	if (toggling) return;

	debugLog("RecipeMenu::toggle");

	toggling = true;
	
	float t = 0.6;

	if (on)
	{
		slide(scroll, 1, t);
		slide(scrollEnd, 1, t);
		slide(header, 1, t);
		slide(page, 1, t);

		dsq->sound->playSfx("recipemenu-open");

		if (watch)
			dsq->main(t);

		if (!dsq->game->isInGameMenu())
		{
			slide(scroll, 0, 0);
			slide(scrollEnd, 0, 0);
			slide(header, 0, 0);
			slide(page, 0, 0);
			toggling = false;
			return;
		}

		createPage(currentPage);
		
		nextPage->alpha.interpolateTo(1, 0.2);
		prevPage->alpha.interpolateTo(1, 0.2);
	}
	else
	{
		destroyPage();

		slide(scroll, 0, t);
		slide(scrollEnd, 0, t);
		slide(header, 0, t);
		slide(page, 0, t);

		if (this->on)
			dsq->sound->playSfx("recipemenu-close");
		
		nextPage->alpha = 0;
		prevPage->alpha = 0;

		if (watch)
			dsq->main(t);
	}

	this->on = on;

	AquariaGuiElement::canDirMoveGlobal = !this->on;

	toggling = false;
}

void RecipeMenu::createPage(int p)
{
	int num = 0;
	int startNum = p*pageSize;
	int checkNum = 0;
	for (int i = dsq->continuity.recipes.size()-1; i >=0 ; i--)
	{
		if (dsq->continuity.recipes[i].isKnown())
		{
			if (checkNum >= startNum)
			{
				if (num < pageSize)
				{
					RecipeMenuEntry *r = new RecipeMenuEntry(&dsq->continuity.recipes[i]);
					recipeMenuEntries.push_back(r);
					//80 + num * 70
					r->position = Vector(500-5, 65 + num * 70);
					dsq->game->addRenderObject(r, scroll->layer);
					num++;
				}
				else
					break;
			}
			else
			{
				checkNum++;
			}
		}
	}
	
	if (num == 0)
	{
		
	}

	description = new BitmapText(&dsq->smallFont);
	description->followCamera = 1;
	description->scale = Vector(0.7, 0.7);
	description->setAlign(ALIGN_LEFT);
	description->position = Vector(364, 334); //most recent: (364, 334) //348, 328 
	description->color = Vector(0,0,0);//Vector(0.7,0,0);
	description->setText("");
	description->setWidth(500); // 1000??
	
	dsq->game->addRenderObject(description, scroll->layer);

	std::ostringstream os2;
	os2 << "Page " << currentPage+1 << "/" << getNumPages()+1;
	page->setText(os2.str());

	debugLog("done: " + os2.str());
}

void RecipeMenu::destroyPage()
{
	for (int i = 0; i < recipeMenuEntries.size(); i++)
	{
		recipeMenuEntries[i]->setLife(1);
		recipeMenuEntries[i]->setDecayRate(20);
		recipeMenuEntries[i]->fadeAlphaWithLife = 1;
	}
	recipeMenuEntries.clear();

	if (description)
	{
		description->setLife(1);
		description->setDecayRate(4);
		description->fadeAlphaWithLife = 1;
		description = 0;
	}
}
