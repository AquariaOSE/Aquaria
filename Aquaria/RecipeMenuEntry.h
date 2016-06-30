#ifndef RECIPEMENUENTRY_H
#define RECIPEMENUENTRY_H

#include "RenderObject.h"

class Recipe;
class Quad;
class BitmapText;
class IngredientData;

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


#endif
