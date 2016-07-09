#ifndef INGREDIENT_H
#define INGREDIENT_H

#include "Entity.h"

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


#endif
