#include "Base.h"
#include "GameStructs.h"


Recipe::Recipe()
{
	known = false;
	index = -1;
}

void Recipe::clear()
{
	types.clear();
	names.clear();
	result = "";
	resultDisplayName = "";
	known = false;
}

void Recipe::learn()
{
	known = true;
}

void Recipe::addName(const std::string &name)
{
	size_t i = 0;
	for (; i < names.size(); i++)
	{
		if (names[i].name == name)
		{
			names[i].amount++;
			break;
		}
	}
	if (i == names.size())
		names.push_back(RecipeName(name));
}

void Recipe::addType(IngredientType type, const std::string &typeName)
{
	size_t i = 0;
	for (; i < types.size(); i++)
	{
		if (types[i].type == type)
		{
			types[i].amount++;
			break;
		}
	}
	if (i == types.size())
		types.push_back(RecipeType(type, typeName));
}

