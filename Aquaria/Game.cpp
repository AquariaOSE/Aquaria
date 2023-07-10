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

#include "ttvfs_stdio.h"
#include "ReadXML.h"
#include "RenderBase.h"

#include "Game.h"
#include "GridRender.h"
#include "WaterSurfaceRender.h"
#include "ScriptedEntity.h"
#include "FlockEntity.h"
#include "SchoolFish.h"
#include "Avatar.h"
#include "Shot.h"
#include "Web.h"
#include "StatsAndAchievements.h"
#include "InGameMenu.h"
#include "ManaBall.h"
#include "Spore.h"
#include "Ingredient.h"
#include "Beam.h"
#include "Hair.h"

#ifdef BBGE_USE_GLM
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#endif


static const float MENUPAGETRANSTIME		= 0.2f;

const float bgSfxVol	= 1.0f;

unsigned char Game::grid[MAX_GRID][MAX_GRID];


static std::string getSceneFilename(const std::string &scene)
{
	if (dsq->mod.isActive())
		return std::string(dsq->mod.getPath() + "maps/" + scene + ".xml");
	else
		return std::string("data/maps/"+scene+".xml");
	return "";
}

Ingredient *Game::getNearestIngredient(const Vector &pos, float radius)
{
	int closest = -1;
	float r2 = sqr(radius);
	Ingredient *returnIngredient = 0;

	for (Ingredients::iterator i = ingredients.begin(); i != ingredients.end(); i++)
	{
		float len = (pos - (*i)->position).getSquaredLength2D();
		if (len <= r2 && (closest == - 1 || len < closest))
		{
			closest = len;
			returnIngredient = (*i);
		}
	}
	return returnIngredient;
}

Entity *Game::getNearestEntity(const Vector &pos, float radius, Entity *ignore, EntityType et, DamageType dt, int lrStart, int lrEnd)
{
	int sqrRadius = radius*radius;
	Entity *closest = 0;
	float sml=-1;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		const float dist = (e->position - pos).getSquaredLength2D();
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
			establishEntity(i, findUnusedEntityID(true), pos);
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
	establishEntity(i, findUnusedEntityID(true), ent->position);
	//addRenderObject(i, LR_ENTITIES);
}

Game::Game() : StateObject()
{
	themenu = new InGameMenu;

	applyingState = false;

	hasPlayedLow = false;

	invincibleOnNested = true;
	activation = false;
	invinciblity = false;

	active = false;

	registerState(this, "Game");

	toFlip = -1;
	shuttingDownGameState = false;
	dsq->loops.bg = BBGE_AUDIO_NOCHANNEL;
	dsq->loops.bg2 = BBGE_AUDIO_NOCHANNEL;


	loadingScene = false;
	controlHint_mouseLeft = controlHint_mouseRight = controlHint_mouseMiddle = controlHint_mouseBody = controlHint_bg = 0;
	controlHint_text = 0;

	avatar = 0;
	deathTimer = 0;

	game = this;
	cameraFollow = 0;

	worldMapRender = 0;

	for (int i = 0; i < PATH_MAX; i++)
		firstPathOfType[i] = 0;

	loadEntityTypeList();

	worldPaused = false;

	cookingScript = 0;
	doScreenTrans = false;
	noSceneTransitionFadeout = false;
	fullTilesetReload = false;
}

Game::~Game()
{
	delete themenu;
	game = 0;
}

void Game::pickupIngredientEffects(IngredientData *data)
{
	Quad *q = new Quad("gfx/ingredients/" + data->gfx, Vector(800-20 + core->getVirtualOffX(), (570-2*(100*miniMapRender->scale.y))+ingOffY));
	q->scale = Vector(0.8f, 0.8f);
	q->followCamera = 1;
	q->alpha.ensureData();
	q->alpha.data->path.addPathNode(0, 0);
	q->alpha.data->path.addPathNode(1.0f, 0.1f);
	q->alpha.data->path.addPathNode(0, 1.0f);
	q->alpha.startPath(2);
	q->setLife(1);
	q->setDecayRate(0.5);
	addRenderObject(q, LR_HELP);
	ingOffY -= 40;
	ingOffYTimer = 2;
}

void Game::spawnManaBall(Vector pos, float a)
{
	ManaBall *m = new ManaBall(pos, a);
	addRenderObject(m, LR_PARTICLES);
}
void Game::warpPrep()
{
	avatar->onWarp();
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

ElementTemplate *Game::getElementTemplateByIdx(size_t idx)
{
	return tileset.getByIdx(idx);
}

Element* Game::createElement(size_t idx, Vector position, size_t bgLayer, RenderObject *copy, ElementTemplate *et)
{
	if (idx == -1) return 0;

	if (!et)
		et = this->getElementTemplateByIdx(idx);

	Element *element = new Element();
	if (et)
	{
		element->setTexturePointer(et->getTexture());
	}

	element->position = position;
	element->position.z = -0.05f;
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

	return element;
}

void Game::addObsRow(unsigned tx, unsigned ty, unsigned len)
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
		size_t size = 0;
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
					//setGrid(TileVector(int(tx/TILE_SIZE)+tpos.x, int(ty/TILE_SIZE)+tpos.y), 1);
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
			for (size_t i = 0; i < obsCopy.size(); i++)
			{
				sides = 0;
				for (size_t j = 0; j < obsCopy.size(); j++)
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


#ifdef BBGE_USE_GLM
		const float w2f = float(w2);
		const float h2f = float(h2);
		for (size_t i = 0; i < obs.size(); i++)
		{
			glm::mat4 transformMatrix = glm::rotate(q->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			if(q->isfh())
				transformMatrix *= glm::rotate(180.0f, glm::vec3(0.0f, 1.0f, 0.0f));

			transformMatrix *= glm::translate(float(obs[i].x)-w2f, float(obs[i].y)-h2f, 0.0f);
			float x = transformMatrix[3][0];
			float y = transformMatrix[3][1];

			TileVector tvec(tpos.x+w2+x, tpos.y+h2+y);
			if (!isObstructed(tvec))
				addGrid(tvec, obsType);
		}
#else
		glPushMatrix();

		for (size_t i = 0; i < obs.size(); i++)
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

			//setGrid(TileVector(tpos.x+(w2*TILE_SIZE)+(x/TILE_SIZE), tpos.y+(h2*TILE_SIZE)+(y/TILE_SIZE)), obsType);
			TileVector tvec(tpos.x+w2+x, tpos.y+h2+y);
			if (!isObstructed(tvec))
				addGrid(tvec, obsType);

		}
		glPopMatrix();
#endif
	}
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
	compile_assert(sizeof(grid) % sizeof(unsigned) == 0);

	unsigned char *gridstart = &grid[0][0];
	unsigned *gridend = (unsigned*)(gridstart + sizeof(grid));
	unsigned *gridptr = (unsigned*)gridstart;
	// mask out specific bytes
	// use full uint32 rounds instead of single-bytes to speed things up.
	const unsigned mask = maskbyte | (maskbyte << 8) | (maskbyte << 16) | (maskbyte << 24);
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
	for (size_t i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		e->fillGrid();
	}

	ObsRow *o;
	for (size_t i = 0; i < obsRows.size(); i++)
	{
		o = &obsRows[i];
		for (unsigned tx = 0; tx < o->len; tx++)
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
}

void Game::trimGrid()
{
	// Prevent the left- and rightmost column of black tiles
	// from beeing drawn. (The maps were designed with this mind...)
	for (int x = 0; x < MAX_GRID; x++)
	{
		const unsigned char *curCol   = grid[x]; // safe
		const unsigned char *leftCol  = getGridColumn(x-1); // unsafe
		const unsigned char *rightCol = getGridColumn(x+1); // unsafe
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

void Game::dilateGrid(unsigned int radius, ObsType test, ObsType set, ObsType allowOverwrite)
{
	if(!radius)
		return;
	const int lim = MAX_GRID - radius;
	const unsigned int denyOverwrite = ~allowOverwrite;
	// Box dilation is separable, so we do a two-pass by axis
	int dilate = 0;

	// dilate rows
	for (int y = 0; y < MAX_GRID; ++y)
	{
		for (int x = radius; x < lim; ++x)
		{
			if (grid[x][y] & test)
			{
				dilate = 2 * radius;
				goto doDilate1;
			}
			if(dilate)
			{
				--dilate;
				doDilate1:
				if((grid[x - radius][y] & denyOverwrite) == OT_EMPTY)
					grid[x - radius][y] |= set;
			}
		}
		assert(lim + dilate < MAX_GRID);
		for(int x = 0; x < dilate; ++x)
			if(!(grid[x][y - radius] & test))
				grid[x][y - radius] |= set;
	}

	// dilate colums
	dilate = 0;
	for (int x = 0; x < MAX_GRID; ++x)
	{
		unsigned char * const curCol = grid[x];
		for (int y = radius; y < lim; ++y)
		{
			if (curCol[y] & test)
			{
				dilate = 2 * radius;
				goto doDilate2;
			}
			if(dilate)
			{
				--dilate;
				doDilate2:
				if((curCol[y - radius] & denyOverwrite) == OT_EMPTY)
					curCol[y - radius] |= set;
			}
		}
		assert(lim + dilate < MAX_GRID);
		for(int y = 0; y < dilate; ++y)
			if(!(curCol[y - radius] & test))
				curCol[y - radius] |= set;
	}
}

Vector Game::getWallNormal(Vector pos, int sampleArea, int obs)
{
	const TileVector t(pos); // snap to grid
	Vector avg;
	const float szf = (TILE_SIZE*(sampleArea-1));
	for (int x = t.x-sampleArea; x <= t.x+sampleArea; x++)
	{
		for (int y = t.y-sampleArea; y <= t.y+sampleArea; y++)
		{
			if (x == t.x && y == t.y) continue;
			const TileVector ct(x,y);
			if (isObstructed(ct, obs))
			{
				Vector v = pos - ct.worldVector();
				const float d = v.getLength2D();
				if (d < szf)
				{
					v.setLength2D(szf - d);
					avg += v;
				}

			}
		}
	}
	avg.normalize2D();
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
	const int id = selected->getID();

	selected->setState(Entity::STATE_DEAD);
	selected->safeKill();

	for (size_t i = 0; i < entitySaveData.size(); i++)
	{
		if (entitySaveData[i].id == id)
		{
			entitySaveData.erase(entitySaveData.begin() + i);
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

void Game::loadEntityTypeList()
// and group list!
{
	entityTypeList.clear();
	InStream in("scripts/entities/entities.txt");
	std::string line;
	if(!in)
	{
		exit_error(stringbank.get(2008).c_str());
	}
	while (std::getline(in, line))
	{
		std::string name, prevGfx;
		int idx;
		float scale;
		std::istringstream is(line);
		is >> idx >> name >> prevGfx >> scale;
		entityTypeList.push_back(EntityClass(name, 1, idx, prevGfx, scale));
	}
	in.close();

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
}

EntityClass *Game::getEntityClassForEntityType(const std::string &type)
{
	for (size_t i = 0; i < entityTypeList.size(); i++)
	{
		if (nocasecmp(entityTypeList[i].name, type)==0)
			return &entityTypeList[i];
	}
	return 0;
}

int Game::getIdxForEntityType(std::string type)
{
	for (size_t i = 0; i < entityTypeList.size(); i++)
	{
		if (nocasecmp(entityTypeList[i].name, type)==0)
			return entityTypeList[i].idx;
	}
	return -1;
}

// ensure a limit of entity types in the current level
// older entities with be culled if state is set to 0
// otherwise, the older entities will have the state set
void Game::ensureLimit(Entity *me, int num, int state)
{
	int c = 0;
	std::vector<Entity*> entityList;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if ((state == 0 || e->getState() != state) && !nocasecmp(me->name, e->name))
		{
			entityList.push_back(*i);
			c++;
		}
	}

	int numDelete = c-(num+1);
	if (numDelete >= 0)
	{
		for (std::vector<Entity*>::iterator i = entityList.begin(); i != entityList.end(); i++)
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

void Game::establishEntity(Entity *e, int id, Vector startPos)
{
	assert(id); // 0 is invalid/reserved
	assert(!getEntityByID(id)); // must not already exist

	e->setID(id);
	e->position = startPos;
	e->startPos = startPos;

	// most scripts call setupEntity() in init(), which also sets the layer.
	// otherwise the script is expected to set the render layer here if not using the default.
	e->init();

	unsigned layer = e->layer; // layer was either set in ctor, or in init()
	e->layer = LR_NONE; // addRenderObject wants this to be not set
	addRenderObject(e, layer);
}

Entity* Game::getEntityByID(int id) const
{
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->getID() == id)
			return e;
	}
	return NULL;
}

EntitySaveData *Game::getEntitySaveDataForEntity(Entity *e)
{
	const int id = e->getID();
	for (size_t i = 0; i < entitySaveData.size(); i++)
	{
		if (entitySaveData[i].id == id)
		{
			return &entitySaveData[i];
		}
	}
	return 0;
}

int Game::findUnusedEntityID(bool temporary) const
{
	const int inc = temporary ? -1 : 1;
	int id = 0;
retry:
	id += inc;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->getID() == id)
			goto retry;
	}
	return id;
}

// caller must do e->postInit() when all map entities have been created
Entity* Game::createEntityOnMap(const EntitySaveData& sav)
{
	assert(sav.id > 0);
	std::string type = sav.name;

	// legacy entities have no name recorded and instead use the idx specified in scripts/entities.txt
	// newer entities and all those added by mods have idx==-1 and use the name directly
	if(type.empty())
	{
		for (size_t i = 0; i < entityTypeList.size(); i++)
		{
			const EntityClass& ec = entityTypeList[i];
			if (ec.idx == sav.idx)
			{
				type = ec.name;
				break;
			}
		}
		if(type.empty())
		{
			std::ostringstream os;
			os << "Game::createEntityOnMap: Don't know entity type name for idx " << sav.idx;
			errorLog(os.str());
			return NULL;
		}
	}

	if(Entity *e = getEntityByID(sav.id))
	{
		assert(false);
		return NULL; // can't spawn, entity with that ID is already present
	}

	stringToLower(type);

	Vector pos(sav.x, sav.y);
	ScriptedEntity *e = new ScriptedEntity(type, pos, ET_ENEMY);
	e->rotation.z = sav.rot;

	EntitySaveData copy = sav;
	copy.name = type;
	copy.idx = getIdxForEntityType(type);
	entitySaveData.push_back(copy);

	establishEntity(e, sav.id, pos);

	return e;
}

Entity *Game::createEntityOnMap(const char * type, const Vector pos)
{
	int id = findUnusedEntityID(false);
	EntitySaveData data;
	data.id = id;
	data.idx = -1;
	data.name = type;
	data.rot = 0;
	data.x = pos.x;
	data.y = pos.y;
	return createEntityOnMap(data);
}

Entity* Game::createEntityTemp(const char* type, Vector pos, bool doPostInit)
{
	int id = findUnusedEntityID(true);
	ScriptedEntity *e = new ScriptedEntity(type, pos, ET_ENEMY);
	establishEntity(e, id, pos);
	// it's possible that we're loading a map, and an entity spawned via createEntityOnMap()
	// calls createEntity() in its script's init() function. That's when we end up here.
	// Delay postInit() until we're sure that the map has been loaded, then call postInit() for all.
	if(doPostInit && !loadingScene)
		e->postInit(); // if we're already running the map, do postInit() now
	return e;
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

void Game::generateCollisionMask(Bone *q, float overrideCollideRadius /* = 0 */)
{
	if (q->texture)
	{
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
		size_t size = 0;
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
	}
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

void Game::removePath(size_t idx)
{
	if (idx < paths.size()) paths[idx]->destroy();
	std::vector<Path*> copy = this->paths;
	clearPaths();
	for (size_t i = 0; i < copy.size(); i++)
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

size_t Game::getIndexOfPath(Path *p)
{
	for (size_t i = 0; i < paths.size(); i++)
	{
		if (paths[i] == p)
			return i;
	}
	return -1;
}

Path *Game::getPathAtCursor()
{
	return getNearestPath(dsq->getGameCursorPosition(), "");
}

Path *Game::getScriptedPathAtCursor(bool withAct)
{
	const size_t sz = paths.size();
	for (size_t i = 0; i < sz; i++)
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
	for (size_t i = 0; i < paths.size(); i++)
	{
		Path *cp = paths[i];
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
	for (Path *cp = getFirstPathOfType(pathType); cp; cp = cp->nextOfType)
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
	for (size_t i = 0; i < paths.size(); i++)
	{
		if (paths[i]->label == name)
			return paths[i];
	}
	return 0;
}

Path *Game::getWaterbubbleAt(const Vector& pos, float rad) const
{
	for (Path *p = getFirstPathOfType(PATH_WATERBUBBLE); p; p = p->nextOfType)
		if(p->active && p->isCoordinateInside(pos, rad))
			return p;
	return NULL;
}

UnderWaterResult Game::isUnderWater(const Vector& pos, float rad) const
{
	UnderWaterResult ret { false, NULL };
	if (!game->useWaterLevel || game->waterLevel.x == 0
		|| (useWaterLevel && waterLevel.x > 0 && pos.y-rad > waterLevel.x))
	{
		ret.uw = true;
		return ret;
	}

	Path *p = game->getWaterbubbleAt(pos, rad);
	ret.waterbubble = p;
	ret.uw = !!p;
	return ret;
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
			avatar->myZoom = dsq->globalScale;
		}
		avatar->zoomOverriden = on;
	}
}

bool Game::loadSceneXML(std::string scene)
{
	bgSfxLoop = "";
	airSfxLoop = "";
	entitySaveData.clear();
	std::string fn = getSceneFilename(scene);
	if (!exists(fn))
	{
		//errorLog("Could not find [" + fn + "]");
		//msg("Could not find map [" + fn + "]");
		std::string s = "Could not find map [" + fn + "]";
		dsq->screenMessage(s);
		return false;
	}
	XMLDocument doc;
	if(readXML(fn, doc) != XML_SUCCESS)
	{
		dsq->screenMessage("Could not load scene [" + fn + "] - Malformed XML");
		return false;
	}
	if (saveFile)
	{
		delete saveFile;
		saveFile = 0;
	}
	if (!saveFile)
	{
		saveFile = new XMLDocument();
	}
	clearObsRows();

	std::string tilesetToLoad;

	XMLElement *level = doc.FirstChildElement("Level");
	if (level)
	{
		XMLElement *levelSF = saveFile->NewElement("Level");
		const char *tileset = level->Attribute("tileset");
		if(!tileset)
			tileset = level->Attribute("elementTemplatePack"); // legacy, still present in some very old maps
		if (tileset)
		{
			tilesetToLoad = tileset;
			levelSF->SetAttribute("tileset", tileset);
		}
		else
			return false;

		if (level->Attribute("waterLevel"))
		{
			useWaterLevel = true;
			waterLevel = atoi(level->Attribute("waterLevel"));
			saveWaterLevel = atoi(level->Attribute("waterLevel"));
			levelSF->SetAttribute("waterLevel", waterLevel.x);
		}

		if (level->Attribute("bgSfxLoop"))
		{
			bgSfxLoop = level->Attribute("bgSfxLoop");
			levelSF->SetAttribute("bgSfxLoop", bgSfxLoop.c_str());
		}
		if (level->Attribute("airSfxLoop"))
		{
			airSfxLoop = level->Attribute("airSfxLoop");
			levelSF->SetAttribute("airSfxLoop", airSfxLoop.c_str());
		}
		if (level->Attribute("bnat"))

		{
			bNatural = atoi(level->Attribute("bnat"));
			levelSF->SetAttribute("bnat", 1);
		}
		else
		{
			bNatural = false;
		}

		dsq->darkLayer.toggle(true);

		if (level->Attribute("cameraConstrained"))
		{
			SimpleIStringStream is(level->Attribute("cameraConstrained"));
			is >> cameraConstrained;
			levelSF->SetAttribute("cameraConstrained", cameraConstrained);
			std::ostringstream os;
			os << "cameraConstrained: " << cameraConstrained;
			debugLog(os.str());
		}

		gradTop = gradBtm = Vector(0,0,0);
		if (level->Attribute("gradient"))
		{
			if (level->Attribute("gradTop"))
			{
				SimpleIStringStream is(level->Attribute("gradTop"));
				is >> gradTop.x >> gradTop.y >> gradTop.z;
				levelSF->SetAttribute("gradTop", level->Attribute("gradTop"));
			}
			if (level->Attribute("gradBtm"))
			{
				SimpleIStringStream is(level->Attribute("gradBtm"));
				is >> gradBtm.x >> gradBtm.y >> gradBtm.z;
				levelSF->SetAttribute("gradBtm", level->Attribute("gradBtm"));
			}
			createGradient();
			levelSF->SetAttribute("gradient", 1);
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
			levelSF->SetAttribute("parallax", level->Attribute("parallax"));
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

			levelSF->SetAttribute("parallaxLock", level->Attribute("parallaxLock"));
		}

		musicToPlay = "";
		if (level->Attribute("music"))
		{
			setMusicToPlay(level->Attribute("music"));
			saveMusic = level->Attribute("music");
			levelSF->SetAttribute("music", level->Attribute("music"));
		}
		if (level->Attribute("sceneColor"))
		{
			SimpleIStringStream in(level->Attribute("sceneColor"));
			in >> sceneColor.x >> sceneColor.y >> sceneColor.z;
			levelSF->SetAttribute("sceneColor", level->Attribute("sceneColor"));
		}

		saveFile->InsertEndChild(levelSF);
	}
	else
		return false;

	XMLElement *obs = doc.FirstChildElement("Obs");
	if (obs)
	{
		int tx, ty, len;
		SimpleIStringStream is(obs->Attribute("d"));
		while (is >> tx)
		{
			is >> ty >> len;
			addObsRow(tx, ty, len);
		}
	}

	XMLElement *pathXml = doc.FirstChildElement("Path");
	while (pathXml)
	{
		Path *path = new Path;
		path->name = pathXml->Attribute("name");
		stringToLower(path->name);
		XMLElement *nodeXml = pathXml->FirstChildElement("Node");
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
		pathXml = pathXml->NextSiblingElement("Path");
	}

	XMLElement *schoolFish = doc.FirstChildElement("SchoolFish");
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
			s->applyLayer(layer);

			s->scale *= size;
		}

		schoolFish = schoolFish->NextSiblingElement("SchoolFish");

		XMLElement *newSF = saveFile->NewElement("SchoolFish");
		newSF->SetAttribute("x", x);
		newSF->SetAttribute("y", y);
		newSF->SetAttribute("id", id);
		newSF->SetAttribute("num", num);

		if (range != 0)
			newSF->SetAttribute("range", range);
		if (maxSpeed != 0)
			newSF->SetAttribute("maxSpeed", maxSpeed);
		if (layer != 0)
			newSF->SetAttribute("layer", layer);
		if (!gfx.empty())
			newSF->SetAttribute("gfx", gfx.c_str());
		if (size != 1)
			newSF->SetAttribute("size", size);

		saveFile->InsertEndChild(newSF);
	}

	struct ElementDef
	{
		ElementDef(int lr)
			: layer(lr), idx(0), x(0), y(0), rot(0), fh(0), fv(0), flags(0), efxIdx(-1), repeat(0)
			, tag(0), sx(1), sy(1), rsx(1), rsy(1)
		{}

		int layer, idx, x, y, rot, fh, fv, flags, efxIdx, repeat, tag;
		float sx, sy, rsx, rsy;
	};
	std::vector<ElementDef> elemsDefs;
	elemsDefs.reserve(256);

	XMLElement *simpleElements = doc.FirstChildElement("SE");
	while (simpleElements)
	{
		const size_t defsBeginIdx = elemsDefs.size();
		const int layer = atoi(simpleElements->Attribute("l"));

		if (const char *attr = simpleElements->Attribute("d"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(4); // legacy crap
			while (is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot;
				elemsDefs.push_back(d);
			}
		}
		if (const char *attr = simpleElements->Attribute("e"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(layer);
			while(is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot;
				elemsDefs.push_back(d);
			}
		}
		if (const char *attr = simpleElements->Attribute("f"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(layer);
			while(is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot >> d.sx;
				d.sy = d.sx;
				elemsDefs.push_back(d);
			}
		}
		if (const char *attr = simpleElements->Attribute("g"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(layer);
			while(is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot >> d.sx >> d.fh >> d.fv;
				d.sy = d.sx;
				elemsDefs.push_back(d);
			}
		}
		if (const char *attr = simpleElements->Attribute("h"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(layer);
			while(is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot >> d.sx >> d.fh >> d.fv >> d.flags;
				d.sy = d.sx;
				elemsDefs.push_back(d);
			}
		}
		if (const char *attr = simpleElements->Attribute("i"))
		{

			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(layer);
			while(is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot >> d.sx >> d.fh >> d.fv >> d.flags >> d.efxIdx;
				d.sy = d.sx;
				elemsDefs.push_back(d);
			}
		}
		if (const char *attr = simpleElements->Attribute("j"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(layer);
			while(is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot >> d.sx >> d.fh >> d.fv >> d.flags >> d.efxIdx >> d.repeat;
				d.sy = d.sx;
				elemsDefs.push_back(d);
			}
		}
		if (const char *attr = simpleElements->Attribute("k"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			ElementDef d(layer);
			while(is >> d.idx)
			{
				is >> d.x >> d.y >> d.rot >> d.sx >> d.sy >> d.fh >> d.fv >> d.flags >> d.efxIdx >> d.repeat;
				elemsDefs.push_back(d);
			}
		}

		// done loading raw data, now for some possible extensions added later

		if (const char *attr = simpleElements->Attribute("repeatScale"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			for(size_t i = defsBeginIdx; i < elemsDefs.size(); ++i)
			{
				ElementDef& d = elemsDefs[i];
				if(d.repeat)
				{
					if(!(is >> d.rsx >> d.rsy))
						break;
				}
			}
		}
		if (const char *attr = simpleElements->Attribute("tag"))
		{
			SimpleIStringStream is(attr, SimpleIStringStream::REUSE);
			for(size_t i = defsBeginIdx; i < elemsDefs.size(); ++i)
			{
				ElementDef& d = elemsDefs[i];
				if(!(is >> d.tag))
					break;
			}
		}
		simpleElements = simpleElements->NextSiblingElement("SE");
	}

	if(fullTilesetReload)
	{
		fullTilesetReload = false;
		tileset.clear();
		// used by SceneEditor
		// no elements exist right now -> textures will be cleared and reloaded
		dsq->texmgr.clearUnused();
	}

	// figure out which textures in the tileset are used and preload those that are actually used
	{
		std::ostringstream os;
		os << "Scene has " << elemsDefs.size() << " elements";
		debugLog(os.str());

		unsigned char usedIdx[1024] = {0};
		for(size_t i = 0; i < elemsDefs.size(); ++i)
		{
			unsigned idx = elemsDefs[i].idx;
			if(idx < Countof(usedIdx))
				usedIdx[idx] = 1;
		}

		loadElementTemplates(tilesetToLoad, &usedIdx[0], Countof(usedIdx));
	}

	// Now that all SE tags have been processed, spawn them
	for(size_t i = 0; i < elemsDefs.size(); ++i)
	{
		const ElementDef& d = elemsDefs[i];

		Element *e = createElement(d.idx, Vector(d.x,d.y), d.layer);
		e->elementFlag = (ElementFlag)d.flags;
		if (d.fh)
			e->flipHorizontal();
		if (d.fv)
			e->flipVertical();

		e->scale = Vector(d.sx, d.sy);
		e->rotation.z = d.rot;
		e->repeatToFillScale.x = d.rsx;
		e->repeatToFillScale.y = d.rsy;
		e->setElementEffectByIndex(d.efxIdx);
		if (d.repeat)
			e->repeatTextureToFill(true); // also applies repeatToFillScale
		e->setTag(d.tag);

		// HACK: due to a renderer bug in old versions, we need to fix the rotation
		// for horizontally flipped tiles on parallax layers to make maps look correct.
		// See commit 4b52730be253dbfce9bea6f604c772a87da104e3
		// bgLayer IDs (which are NOT LR_* constants):
		// 0..8 are normal layers (keys 1-9)
		// 9,10,11; 13,14,15 are parallax ones, 15 is closest, 9 is furthest away
		// 12 is the dark layer
		if(d.fh && d.layer >= 9 && d.layer <= 15 && d.layer != 12
			&& dsq->renderObjectLayers[e->layer].followCamera != 0)
		{
			e->rotation.z = -e->rotation.z;
		}
	}

	this->reconstructGrid(true);

	std::vector<EntitySaveData> toSpawn;

	XMLElement *entitiesNode = doc.FirstChildElement("Entities");
	while(entitiesNode)
	{
		if (entitiesNode->Attribute("j"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("j"));
			EntitySaveData sav;
			int unusedGroupID;
			while (is >> sav.idx)
			{
				sav.name.clear();
				if (sav.idx == -1)
					is >> sav.name;
				if(is >> sav.x >> sav.y >> sav.rot >> unusedGroupID >> sav.id)
					toSpawn.push_back(sav);
			}
		}
		entitiesNode = entitiesNode->NextSiblingElement("Entities");
	}

	if(toSpawn.size())
		spawnEntities(&toSpawn[0], toSpawn.size());

	this->reconstructGrid(true);
	rebuildElementUpdateList();

	findMaxCameraValues();

	return true;
}

void Game::spawnEntities(const EntitySaveData *sav, size_t n)
{
	std::vector<size_t> conflicting, usable;
	for(size_t i = 0; i < n; ++i)
	{
		const EntitySaveData& es = sav[i];

		// check for ID conflicts
		int id = es.id;
		bool renumber = id <= 0; // entities spawned on map load must have id > 0
		if(!renumber)
		{
			for(size_t k = 0; k < i; ++k)
			{
				if(sav[k].id == id)
				{
					renumber = true;
					break;
				}
			}
		}

		if(!renumber)
			usable.push_back(i);
		else
			conflicting.push_back(i);
	}

	{
		std::ostringstream os;
		os << "Game::spawnEntities: Spawning " << usable.size() << " entities";
		if(conflicting.size())
			os << " without issues, another " << conflicting.size() << " have ID conflicts and need to be renumbered";
		debugLog(os.str());
	}

	size_t failed = 0;

	// create all entities that are possible to spawn without ID conflicts
	for(size_t i = 0; i < usable.size(); ++i)
	{
		const EntitySaveData& es = sav[usable[i]];
		failed += !createEntityOnMap(es);
	}

	// spawn and renumber the rest
	int lastid = 0;
	for(size_t i = 0; i < conflicting.size(); ++i)
	{
		// find an unused ID
		int id = lastid; // assume any ID up to this is already taken...
		bool ok;
		do
		{
			++id; // ... which is why the first thing we do is to increment this
			ok = true;
			for(size_t k = 0; k < n; ++k)
			{
				if(sav[k].id == id)
				{
					ok = false;
					break;
				}
			}
		}
		while(!ok);
		lastid = id;

		EntitySaveData es = sav[conflicting[i]];
		std::ostringstream os;
		os << "Renumbering entity [" << es.idx << ", " << es.name << "] id " << es.id << " to " << id;
		debugLog(os.str());
		es.id = id;
		failed += !createEntityOnMap(es);
	}

	if(failed)
	{
		std::ostringstream os;
		os << "Game::spawnEntities: Failed to spawn " << failed << " entities, on map [" << sceneName << "]";
		errorLog(os.str());
	}
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
	for (size_t i = 0; i < obsRows.size(); i++)
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
}

bool Game::loadScene(std::string scene)
{
	stringToLower(scene);

	sceneName = scene;
	if (scene.empty())
	{
		return false;
	}

	loadingScene = true;
	bool ret = loadSceneXML(scene);
	loadingScene = false;

	return ret;
}

bool Game::saveScene(std::string scene)
{
	if (!this->saveFile)
		return false;

	std::string fn = getSceneFilename(scene);

	XMLDocument saveFile;

	// hackish: Deep-clone XML doc
	{
		XMLPrinter printer;
		this->saveFile->Print(&printer);

		XMLError xmlerr = saveFile.Parse(printer.CStr(), printer.CStrSize());
		if(xmlerr != XML_SUCCESS)
		{
			std::ostringstream os;
			os << "Game::saveScene(): Whoops? Deep cloning level XML failed: Error " << xmlerr;
			errorLog(os.str());
		}
	}

	XMLElement *level = saveFile.FirstChildElement("Level");
	if(!level)
	{
		level = saveFile.NewElement("Level");
		saveFile.InsertFirstChild(level);
	}

	if (level)
	{
		level->SetAttribute("waterLevel", saveWaterLevel);

		if (grad)
		{
			level->SetAttribute("gradient", 1);

			std::ostringstream os;
			os << gradTop.x << " " << gradTop.y << " " << gradTop.z;
			level->SetAttribute("gradTop", os.str().c_str());

			std::ostringstream os2;
			os2 << gradBtm.x << " " << gradBtm.y << " " << gradBtm.z;
			level->SetAttribute("gradBtm", os2.str().c_str());
		}

		if (!saveMusic.empty())
		{
			level->SetAttribute("music", saveMusic.c_str());
		}
	}

	std::ostringstream obs;
	for (size_t i = 0; i < obsRows.size(); i++)
	{
		obs << obsRows[i].tx << " " << obsRows[i].ty << " " << obsRows[i].len << " ";
	}
	XMLElement *obsXml = saveFile.NewElement("Obs");
	obsXml->SetAttribute("d", obs.str().c_str());
	saveFile.InsertEndChild(obsXml);


	for (size_t i = 0; i < getNumPaths(); i++)
	{
		XMLElement *pathXml = saveFile.NewElement("Path");
		Path *p = getPath(i);
		pathXml->SetAttribute("name", p->name.c_str());
		for (size_t n = 0; n < p->nodes.size(); n++)
		{
			XMLElement *nodeXml = saveFile.NewElement("Node");
			std::ostringstream os;
			os << int(p->nodes[n].position.x) << " " << int(p->nodes[n].position.y);
			nodeXml->SetAttribute("pos", os.str().c_str());
			std::ostringstream os2;
			os2 << p->rect.getWidth() << " " << p->rect.getHeight();
			nodeXml->SetAttribute("rect", os2.str().c_str());
			nodeXml->SetAttribute("shape", (int)p->pathShape);
			if (p->nodes[n].maxSpeed != -1)
			{
				nodeXml->SetAttribute("ms", p->nodes[n].maxSpeed);
			}
			pathXml->InsertEndChild(nodeXml);
		}
		saveFile.InsertEndChild(pathXml);
	}

	std::ostringstream simpleElements[LR_MAX];
	std::ostringstream simpleElements_repeatScale[LR_MAX];
	std::ostringstream simpleElements_tag[LR_MAX];
	unsigned tagBitsUsed[LR_MAX] = { 0 };

	for (size_t i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);

		float rot = e->rotation.z;

		// HACK: Intentionally store the wrong rotation for parallax layers,
		// to make loading a scene and the same hack there result in the correct value.
		// This is to ensure compatibility with older game versions that have the renderer bug.
		// See commit 4b52730be253dbfce9bea6f604c772a87da104e3
		if(e->isfh() && e->bgLayer >= 9 && e->bgLayer <= 15 && e->bgLayer != 12
			&& dsq->renderObjectLayers[e->layer].followCamera != 0)
		{
			rot = -rot;
		}

		std::ostringstream& SE = simpleElements[e->bgLayer];
		SE << e->templateIdx << " "
		   << int(e->position.x) << " "
		   << int(e->position.y) << " "
		   << int(rot) << " "
		   << e->scale.x << " "
		   << e->scale.y << " "
		   << int(e->isfh()) << " "
		   << int(e->isfv()) << " "
		   << e->elementFlag << " "
		   << e->getElementEffectIndex()<< " "
		   << e->isRepeatingTextureToFill() << " ";

		if(e->isRepeatingTextureToFill())
		{
			std::ostringstream& SE_rs = simpleElements_repeatScale[e->bgLayer];
			SE_rs << e->repeatToFillScale.x << " "
			      << e->repeatToFillScale.y << " ";
		}

		simpleElements_tag[e->bgLayer] << e->tag << " ";
		tagBitsUsed[e->bgLayer] |= e->tag;
	}

	if (entitySaveData.size() > 0)
	{
		XMLElement *entitiesNode = saveFile.NewElement("Entities");

		std::ostringstream os;
		for (size_t i = 0; i < entitySaveData.size(); i++)
		{
			EntitySaveData *e = &entitySaveData[i];
			os << e->idx << " ";

			if (e->idx == -1)
			{
				if (!e->name.empty())
					os << e->name << " ";
				else
					os << "INVALID" << " ";
			}
			// group ID no longer used
			os << e->x << " " << e->y << " " << e->rot << " " << 0 << " " << e->id << " ";
		}
		entitiesNode->SetAttribute("j", os.str().c_str());
		saveFile.InsertEndChild(entitiesNode);
	}

	for (int i = 0; i < LR_MAX; i++)
	{
		std::string s = simpleElements[i].str();
		if (!s.empty())
		{
			XMLElement *simpleElementsXML = saveFile.NewElement("SE");
			simpleElementsXML->SetAttribute("k", s.c_str());
			simpleElementsXML->SetAttribute("l", i);
			std::string str = simpleElements_repeatScale[i].str();
			if(!str.empty())
				simpleElementsXML->SetAttribute("repeatScale", str.c_str());
			if(tagBitsUsed[i]) // skip writing tags on layers where it's all zero (mainly to avoid putting a long string of 0's for border elements)
			{
				str = simpleElements_tag[i].str();
				if(!str.empty())
					simpleElementsXML->SetAttribute("tag", str.c_str());
			}

			saveFile.InsertEndChild(simpleElementsXML);
		}
	}

	bool result =  saveFile.SaveFile(fn.c_str()) == XML_SUCCESS;
	if (result)
		debugLog("Successfully saved map: " + fn);
	else
		debugLog("Failed to save map: " + fn);

	return result;
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
	}
}

bool Game::isInGameMenu()
{
	return themenu->isInGameMenu();
}

bool Game::isValidTarget(Entity *e, Entity *me)
{
	return (e != me && e->isNormalLayer() && e->isPresent() && e->getEntityType() == ET_ENEMY && e->isAvatarAttackTarget());
}

void Game::createPets()
{
	debugLog("createPets()");
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
			std::string name = "Pet_" + p->namePart;

			Entity *e = createEntityTemp(name.c_str(), avatar->position, false);
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
		li = createEntityTemp("Li", Vector(0,0), false);
		//li->skeletalSprite.animate("idle");
	}
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
		image->scale.interpolateTo(Vector(1.1f, 1.1f), 12);

		image->alpha = 1;
		dsq->fade(0, 0.5f);
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
				sfx.priority = 0.8f;
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
				sfx.priority = 0.8f;
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
	debugLog("postInitEntities()");
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
	else if (this->isWorldPaused())
	{
		core->particlesPaused = 1;
	}
	else
	{
		core->particlesPaused = 0;
	}
}

int game_collideParticle(Vector pos)
{
	bool aboveWaterLine = (pos.y <= game->waterLevel.x+20);
	bool inWaterBubble = false;
	if (!aboveWaterLine)
	{
		inWaterBubble = !!game->getWaterbubbleAt(pos);
	}
	if (!inWaterBubble && aboveWaterLine)
	{
		return 1;
	}

	TileVector t(pos);
	return game->isObstructed(t);
}

void Game::rebuildElementUpdateList()
{
	for (int i = LR_ELEMENTS1; i <= LR_ELEMENTS8; i++)
		dsq->getRenderObjectLayer(i)->update = false;

	elementUpdateList.clear();
	elementInteractionList.clear();
	for (size_t i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		const int eeidx = e->getElementEffectIndex();
		if (eeidx != -1 && e->layer >= LR_ELEMENTS1 && e->layer <= LR_ELEMENTS8)
			elementUpdateList.push_back(e);
		ElementEffect ee = dsq->getElementEffectByIndex(eeidx);
		if(ee.type == EFX_WAVY)
			elementInteractionList.push_back(e);
	}
}

float Game::getTimer(float mod)
{
	return timer*mod;
}

float Game::getHalfTimer(float mod)
{
	return halfTimer*mod;
}

void Game::toggleHelpScreen()
{
	action(ACTION_TOGGLEHELPSCREEN, 0, -1, INPUT_NODEVICE);
}

void Game::action(int id, int state, int source, InputDevice device)
{
	for (size_t i = 0; i < paths.size(); i++)
	{
		if (paths[i]->catchActions)
		{
			if (!paths[i]->action(id, state, source, device))
				break;
		}
	}

	if(isIgnoreAction((AquariaActions)id))
		return;

	// forward menu actions
	if(id == ACTION_TOGGLEMENU)
	{
		themenu->action(id, state, source, device);
	}

	if (id == ACTION_TOGGLEHELPSCREEN && !state)
	{
		onToggleHelpScreen();
	}
	if (id == ACTION_ESC && !state)
		onPressEscape(source, device);

	if (id == ACTION_TOGGLEWORLDMAP && !state)
	{
		if (!core->isStateJumpPending() && !themenu->isFoodMenuOpen())
		{
			toggleWorldMap();
		}
	}

	if (id == ACTION_TOGGLESCENEEDITOR && !state)		toggleSceneEditor();

	if (dsq->isDeveloperKeys() || isSceneEditorActive())
	{
		if (id == ACTION_TOGGLEGRID && !state)			toggleGridRender();
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

	bool versionlabel = false;

	// new place where mods get stopped!
	// this lets recaching work
	// (presumably because there has been time for the garbage to be cleared)
	if (sceneToLoad == "title")
	{
		if (dsq->mod.isActive() && dsq->mod.isShuttingDown())
		{
			dsq->mod.stop();
			dsq->continuity.reset();
		}
		versionlabel = true;
	}

	dsq->collectScriptGarbage();

	dsq->toggleBlackBars(false);

	dsq->setTexturePointers();

	cameraOffBounds = false;


	ingOffY = 0;
	ingOffYTimer = 0;

	AquariaGuiElement::canDirMoveGlobal = true;

	dsq->toggleVersionLabel(versionlabel);

	activation = true;

	active = true;

	hasPlayedLow = false;

	firstSchoolFish = true;
	invincibleOnNested = true;

	controlHintNotes.clear();

	particleManager->setNumSuckPositions(10);

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

	controlHint_ignoreClear = false;
	debugLog("Entering Game::applyState");
	dsq->overlay->alpha = 1;
	dsq->overlay->color = 0;


	for (int i = LR_ELEMENTS1; i <= LR_ELEMENTS12; i++) // LR_ELEMENTS13 is darkness, stop before that
	{
		setElementLayerVisible(i-LR_ELEMENTS1, true);
	}

	controlHintTimer = 0;
	cameraConstrained = true;
	// reset parallax
	RenderObjectLayer *l = 0;
	for (int i = LR_ELEMENTS10; i <= LR_ELEMENTS16; i++)
	{
		l = &dsq->renderObjectLayers[i];
		l->followCamera = 0;
		l->followCameraLock = FCL_NONE;
	}

	dsq->resetLayerPasses();

	ignoredActions.clear();

	cameraLerpDelay = 0;
	sceneColor2 = Vector(1,1,1);
	sceneColor3 = Vector(1,1,1);
	if (core->afterEffectManager)
	{
		core->afterEffectManager->clear();
	}
	Shot::shots.clear(); // the shots were deleted elsewhere, drop any remaining pointers
	Shot::deleteShots.clear();
	clearObsRows();
	useWaterLevel = false;
	waterLevel = saveWaterLevel = 0;

	dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = false;

	grad = 0;
	maxLookDistance = 600;
	saveFile = 0;
	deathTimer = 0.9f;
	runGameOverScript = false;
	paused = false;
	//sceneColor = Vector(0.75, 0.75, 0.8);
	sceneColor = Vector(1,1,1);
	sceneName = "";
	clearGrid();
	avatar = 0;
	SkeletalSprite::clearCache();
	StateObject::applyState();

	dsq->clearEntities();
	dsq->clearElements();

	damageSprite = new Quad;
	{
		damageSprite->setTexture("damage");
		damageSprite->alpha = 0;
		damageSprite->autoWidth = AUTO_VIRTUALWIDTH;
		damageSprite->autoHeight = AUTO_VIRTUALHEIGHT;
		damageSprite->position = Vector(400,300);
		damageSprite->followCamera = true;
		damageSprite->scale.interpolateTo(Vector(1.1f, 1.1f), 0.75f, -1, 1, 1);
	}
	addRenderObject(damageSprite, LR_DAMAGESPRITE);

	Vector mousePos(400,490);

	controlHint_bg = new Quad;
	{
		controlHint_bg->followCamera = 1;
		controlHint_bg->position = Vector(400,500);
		controlHint_bg->color = 0;
		controlHint_bg->alphaMod = 0.7f;
		//controlHint_bg->setTexture("HintBox");
		controlHint_bg->setWidthHeight(core->getVirtualWidth(), 100);
		controlHint_bg->autoWidth = AUTO_VIRTUALWIDTH;
		controlHint_bg->alpha = 0;
	}
	addRenderObject(controlHint_bg, LR_HELP);

	controlHint_text = new BitmapText(dsq->smallFont);
	{
		controlHint_text->alpha = 0;
		controlHint_text->setWidth(700);

		controlHint_text->setAlign(ALIGN_LEFT);
		controlHint_text->followCamera = 1;
		controlHint_text->scale = Vector(0.9f, 0.9f);
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
		controlHint_shine->alphaMod = 0.3f;
		controlHint_shine->setWidthHeight(core->getVirtualWidth(), 100);
		controlHint_shine->alpha = 0;
		controlHint_shine->setBlendType(BLEND_ADD);
	}
	addRenderObject(controlHint_shine, LR_HELP);

	li = 0;

	if (dsq->canOpenEditor())
	{
		sceneEditor.init();
	}

	if (verbose) debugLog("Creating Avatar");
	avatar = new Avatar();
	if (verbose) debugLog("Done new Avatar");

	if (positionToAvatar.x == 0 && positionToAvatar.y == 0)
		avatar->position = Vector(dsq->avStart.x,dsq->avStart.y);
	else
		avatar->position = positionToAvatar;
	positionToAvatar = Vector(0,0);

	if (verbose) debugLog("Done warp");

	if (verbose) debugLog("Create Li");
	createLi();
	if (verbose) debugLog("Done");

	if (toFlip == 1)
	{
		avatar->flipHorizontal();
		toFlip = -1;
	}

	bindInput();

	if (verbose) debugLog("Loading Scene");
	if(!loadScene(sceneToLoad))
	{
		debugLog("Failed to load scene [" + sceneToLoad + "]");
	}
	if (verbose) debugLog("...Done");

	dsq->continuity.worldMap.revealMap(sceneName);

	if (verbose) debugLog("Adding Avatar");
	addRenderObject(avatar, LR_ENTITIES);
	setCameraFollowEntity(avatar);
	if (verbose) debugLog("...Done");


	currentRender = new CurrentRender();
	addRenderObject(currentRender, LR_ELEMENTS3);

	steamRender = new SteamRender();
	addRenderObject(steamRender, LR_ELEMENTS9);

	songLineRender = new SongLineRender();
	addRenderObject(songLineRender, LR_HUD);

	gridRender = new GridRender(OT_INVISIBLE);
	gridRender->color = Vector(1, 0, 0);
	addRenderObject(gridRender, LR_DEBUG_TEXT);
	gridRender->alpha = 0;

	gridRender2 = new GridRender(OT_HURT);
	gridRender2->color = Vector(1, 1, 0);
	addRenderObject(gridRender2, LR_DEBUG_TEXT);
	gridRender2->alpha = 0;

	gridRender3 = new GridRender(OT_INVISIBLEIN);
	gridRender3->color = Vector(1, 0.5f, 0);
	addRenderObject(gridRender3, LR_DEBUG_TEXT);
	gridRender3->alpha = 0;

	edgeRender = new GridRender(OT_BLACKINVIS);
	edgeRender->color = Vector(0.3f, 0, 0.6f);
	addRenderObject(edgeRender, LR_DEBUG_TEXT);
	edgeRender->alpha = 0;

	gridRenderEnt = new GridRender(OT_INVISIBLEENT);
	gridRenderEnt->color = Vector(0, 1, 0.5);
	addRenderObject(gridRenderEnt, LR_DEBUG_TEXT);
	gridRenderEnt->alpha = 0;

	gridRenderUser1 = new GridRender(OT_USER1);
	addRenderObject(gridRenderUser1, LR_DEBUG_TEXT);
	gridRenderUser1->color = Vector(1, 0, 1);
	gridRenderUser1->alpha = 0;

	gridRenderUser2 = new GridRender(OT_USER2);
	addRenderObject(gridRenderUser2, LR_DEBUG_TEXT);
	gridRenderUser2->color = Vector(1, 1, 1);
	gridRenderUser2->alpha = 0;

	waterSurfaceRender = new WaterSurfaceRender();
	//waterSurfaceRender->setRenderPass(-1);
	addRenderObject(waterSurfaceRender, LR_WATERSURFACE);

	GridRender *blackRender = new GridRender(OT_BLACK);
	blackRender->color = Vector(0, 0, 0);
	//blackRender->alpha = 0;
	blackRender->setBlendType(BLEND_DISABLED);
	addRenderObject(blackRender, LR_ELEMENTS4);

	miniMapRender = new MiniMapRender;
	// position is set in minimaprender::onupdate
	miniMapRender->scale = Vector(0.55f, 0.55f);
	addRenderObject(miniMapRender, LR_MINIMAP);

	timerText = new BitmapText(dsq->smallFont);
	timerText->position = Vector(745, 550);
	timerText->alpha = 0;
	timerText->followCamera = 1;
	addRenderObject(timerText, LR_MINIMAP);

	worldMapRender = 0;

	if(dsq->mod.isActive() && dsq->mod.mapRevealMethod != REVEAL_UNSPECIFIED)
		WorldMapRender::setRevealMethod(dsq->mod.mapRevealMethod);
	else
		WorldMapRender::setRevealMethod((WorldMapRevealMethod)dsq->user.video.worldMapRevealMethod);

	worldMapRender = new WorldMapRender;
	addRenderObject(worldMapRender, LR_WORLDMAP);

	sceneToLoad="";

	if (!fromScene.empty())
	{
		stringToLower(fromScene);
		debugLog("fromScene: " + fromScene + " fromWarpType: " + fromWarpType);
		float smallestDist = HUGE_VALF;
		Path *closest = 0;
		Vector closestPushOut;
		bool doFlip = false;
		for (size_t i = 0; i < getNumPaths(); i++)
		{
			Path *p = getPath(i);
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
		}
		else
		{
			debugLog("ERROR: Could not find a node to warp the player to!");
		}
		fromScene = "";
	}
	else if (!toNode.empty())
	{
		Path *p = getPathByName(toNode);
		if (p)
		{
			avatar->position = p->nodes[0].position;
		}
		toNode = "";
	}

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

	themenu->reset();

	core->cacheRender();

	cameraInterp.stop();

	core->globalScale = dsq->continuity.zoom;
	//core->globalScaleChanged();
	avatar->myZoom = dsq->continuity.zoom;

	cameraInterp = avatar->position;
	core->cameraPos = getCameraPositionFor(avatar->position);

	if (dsq->mod.isActive())
		dsq->runScript(dsq->mod.getPath() + "scripts/premap_" + sceneName + ".lua", "init", true);
	else
		dsq->runScript("scripts/maps/premap_"+sceneName+".lua", "init", true);

	std::string musicToPlay = this->musicToPlay;
	if (!overrideMusic.empty())
	{
		musicToPlay = overrideMusic;
	}

	if(cookingScript)
	{
		dsq->scriptInterface.closeScript(cookingScript);
		cookingScript = NULL;
	}

	if (dsq->mod.isActive())
		cookingScript = dsq->scriptInterface.openScript(dsq->mod.getPath() + "scripts/cooking.lua", true);
	else
		cookingScript = dsq->scriptInterface.openScript("scripts/global/cooking.lua", true);

	createPets();

	postInitEntities();

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
	for (int i = 0; i < pathSz; i++)
		getPath(i)->init();

	debugLog("Updating bgSfxLoop");
	updateBgSfxLoop();

	// Must be _before_ the init script, since some init scripts run
	// cutscenes immediately.  --achurch
	dsq->subtitlePlayer.show(0.25);

	// First cleanup -- we're now done loading everything; anything leftover from the prev. map can go
	dsq->texmgr.clearUnused();

	if (verbose) debugLog("loading map init script");
	if (dsq->mod.isActive())
		dsq->runScript(dsq->mod.getPath() + "scripts/map_" + sceneName + ".lua", "init", true);
	else
		dsq->runScript("scripts/maps/map_"+sceneName+".lua", "init", true);

	if (!doScreenTrans && (dsq->overlay->alpha != 0 && !dsq->overlay->alpha.isInterpolating()))
	{
		if (verbose) debugLog("fading in");
		debugLog("FADEIN");
		//dsq->overlay->alpha = 1;
		dsq->overlay->alpha.interpolateTo(0, 1);

		core->resetTimer();
		avatar->disableInput();
		core->run(0.5);
		avatar->enableInput();
		core->resetTimer();
	}

	if (doScreenTrans)
	{
		debugLog("SCREENTRANS!");
		core->resetTimer();
		dsq->toggleCursor(false, 0);
		doScreenTrans = false;

		dsq->transitionSaveSlots();
		dsq->overlay->alpha = 0;
		dsq->run(0.5f);
		dsq->toggleCursor(true);
		dsq->tfader->alpha.interpolateTo(0, 0.2f);
		dsq->run(0.21f);
		dsq->clearSaveSlots(false);
	}

	if (verbose) debugLog("reset timer");

	applyingState = false;

	if (!doScreenTrans)
	{
		dsq->toggleCursor(true, 0.5);
	}

	// Second cleanup -- after the short map init fadeout, some entities may have
	// decided to remove themselves, in which case their gfx are no longer needed
	dsq->texmgr.clearUnused();

	debugLog("Game::applyState Done");
}

void Game::bindInput()
{
	if (!(this->applyingState || this->isActive())) return;

	ActionMapper::clearActions();
	//ActionMapper::clearCreatedEvents();

	addAction(ACTION_ESC, KEY_ESCAPE, -1);


	if (dsq->canOpenEditor())
	{
		addAction(ACTION_TOGGLESCENEEDITOR, KEY_TAB, -1);
	}

	if (dsq->canOpenEditor())
	{
		//addAction(MakeFunctionEvent(Game, toggleMiniMapRender), KEY_M, 0);
		addAction(ACTION_TOGGLEGRID, KEY_F9, -1);
	}

	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
		const ActionSet& as = dsq->user.control.actionSets[i];
		int sourceID = (int)i;

		as.importAction(this, "PrimaryAction", ACTION_PRIMARY, sourceID);
		as.importAction(this, "SecondaryAction", ACTION_SECONDARY, sourceID);

		as.importAction(this, "Escape",		ACTION_ESC, sourceID);
		as.importAction(this, "WorldMap",		ACTION_TOGGLEWORLDMAP, sourceID);
		as.importAction(this, "ToggleHelp",	ACTION_TOGGLEHELPSCREEN, sourceID);

		// used for scrolling help text
		as.importAction(this, "SwimUp",		ACTION_SWIMUP, sourceID);
		as.importAction(this, "SwimDown",		ACTION_SWIMDOWN, sourceID);
		as.importAction(this, "SwimLeft",		ACTION_SWIMLEFT, sourceID);
		as.importAction(this, "SwimRight",		ACTION_SWIMRIGHT, sourceID);

		as.importAction(this, "PrevPage",		ACTION_PREVPAGE, sourceID);
		as.importAction(this, "NextPage",		ACTION_NEXTPAGE, sourceID);
		as.importAction(this, "CookFood",		ACTION_COOKFOOD, sourceID);
		as.importAction(this, "FoodLeft",		ACTION_FOODLEFT, sourceID);
		as.importAction(this, "FoodRight",		ACTION_FOODRIGHT, sourceID);
		as.importAction(this, "FoodDrop",		ACTION_FOODDROP, sourceID);

		// To capture quick song keys via script
		as.importAction(this, "SongSlot1",		ACTION_SONGSLOT1, sourceID);
		as.importAction(this, "SongSlot2",		ACTION_SONGSLOT2, sourceID);
		as.importAction(this, "SongSlot3",		ACTION_SONGSLOT3, sourceID);
		as.importAction(this, "SongSlot4",		ACTION_SONGSLOT4, sourceID);
		as.importAction(this, "SongSlot5",		ACTION_SONGSLOT5, sourceID);
		as.importAction(this, "SongSlot6",		ACTION_SONGSLOT6, sourceID);
		as.importAction(this, "SongSlot7",		ACTION_SONGSLOT7, sourceID);
		as.importAction(this, "SongSlot8",		ACTION_SONGSLOT8, sourceID);
		as.importAction(this, "SongSlot9",		ACTION_SONGSLOT9, sourceID);
		as.importAction(this, "SongSlot10",	ACTION_SONGSLOT10, sourceID);

		as.importAction(this, "Revert",		ACTION_REVERT, sourceID);

		as.importAction(this, "Look",			ACTION_LOOK, sourceID);
		as.importAction(this, "Roll",			ACTION_ROLL, sourceID);

		// menu movement via ACTION_SWIM* alias
		as.importAction(this, "SwimUp",		ACTION_MENUUP, sourceID);
		as.importAction(this, "SwimDown",		ACTION_MENUDOWN, sourceID);
		as.importAction(this, "SwimLeft",		ACTION_MENULEFT, sourceID);
		as.importAction(this, "SwimRight",		ACTION_MENURIGHT, sourceID);

		// menu movement via analog stick
		addAction(ACTION_MENURIGHT, JOY_STICK_RIGHT, sourceID);
		addAction(ACTION_MENULEFT, JOY_STICK_LEFT, sourceID);
		addAction(ACTION_MENUDOWN, JOY_STICK_DOWN, sourceID);
		addAction(ACTION_MENUUP, JOY_STICK_UP, sourceID);
	}

	if (avatar)
		avatar->bindInput();

	if (worldMapRender)
		worldMapRender->bindInput();

	if(themenu)
		themenu->bindInput();
}


void Game::overrideZoom(float sz, float t)
{
	if (sz == 0)
	{
		toggleOverrideZoom(false);
	}
	else
	{
		toggleOverrideZoom(true);
		dsq->globalScale.stop();
		dsq->globalScale.interpolateTo(Vector(sz, sz), t);
		dsq->globalScaleChanged();
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

		for (size_t i = 0; i < controlHintNotes.size(); i++)
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

		char sbuf[32];
		sprintf(sbuf, "song/songslot-%d", dsq->continuity.getSongSlotByType(songType));
		Quad *q = new Quad(sbuf, p);
		q->followCamera = 1;
		q->scale = Vector(0.7f, 0.7f);
		q->alpha = 0;
		addRenderObject(q, controlHint_bg->layer);
		controlHintNotes.push_back(q);

		p += Vector(100, 0);

		for (size_t i = 0; i < song->notes.size(); i++)
		{
			int note = song->notes[i];

			sprintf(sbuf, "song/notebutton-%d", note);
			Quad *q = new Quad(sbuf, p);
			q->color = dsq->getNoteColor(note)*0.5f + Vector(1, 1, 1)*0.5f;
			q->followCamera = 1;
			q->scale = Vector(1.0f, 1.0f);
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

	float alphaOn = 0.8f, alphaOff = 0.5f;
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

	for (size_t i = 0; i < controlHintNotes.size(); i++)
	{
		controlHintNotes[i]->alpha.interpolateTo(alphaOn, hintTransTime);
	}

	controlHint_ignoreClear = ignoreClear;


	controlHint_shine->alpha.ensureData();
	controlHint_shine->alpha.data->path.clear();
	controlHint_shine->alpha.data->path.addPathNode(0.001f, 0.0f);
	controlHint_shine->alpha.data->path.addPathNode(1.000f, 0.3f);
	controlHint_shine->alpha.data->path.addPathNode(0.001f, 1.0f);
	controlHint_shine->alpha.startPath(0.4f);
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
			//read = dsq->user.control.actionSet.insertInputIntoString(read);
			string += read + "\n";
		}
	}

	inf.close();
}

void Game::onToggleHelpScreen()
{
	if (inHelpScreen)
		toggleHelpScreen(false);
	else if (core->isStateJumpPending() || themenu->isInKeyConfigMenu())
		return;
	else
	{
		const MenuPage currentMenuPage = themenu->getCurrentMenuPage();
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
	if (isSceneEditorActive()) return;

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
		data += "\n\n" + stringbank.get(2032) + "\n\n";
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
		helpUp->scale = Vector(0.6f, 0.6f);
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
		helpDown->scale = Vector(0.6f, 0.6f);
		helpDown->guiInputLevel = 100;
		addRenderObject(helpDown, LR_HELP);

		helpCancel = new AquariaMenuItem;
		helpCancel->useQuad("Gui/cancel");
		helpCancel->useSound("click");
		helpCancel->useGlow("particles/glow", 128, 40);
		helpCancel->position = Vector(750, 600-20);
		helpCancel->followCamera = 1;
		//helpCancel->rotation.z = 90;
		helpCancel->event.set(MakeFunctionEvent(Game, toggleHelpScreen));
		helpCancel->scale = Vector(0.9f, 0.9f);
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
			return core->sound->playMusic(musicToPlay, SLT_LOOP, SFT_CROSS, 1, SCT_ISNOTPLAYING);
	}
	else
	{
		core->sound->fadeMusic(SFT_OUT, 1);
	}
	return false;
}

void Game::onPressEscape(int source, InputDevice device)
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
		if (worldMapRender->isOn() && !dsq->isNested())
		{
			worldMapRender->toggle(false);
			return;
		}

		if (!paused)
		{
			if (core->getNestedMains() == 1 && !core->isStateJumpPending())
			{
				action(ACTION_TOGGLEMENU, 1, source, device); // show menu
			}
		}

		if ((dsq->saveSlotMode != SSM_NONE || dsq->inModSelector) && core->isNested())
		{
			dsq->selectedSaveSlot = 0;
			core->quitNestedMain();
		}
	}
}

void Game::toggleDamageSprite(bool on)
{
	damageSprite->alphaMod = (float) on;
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
	sound->playSfx("Burst", 1);
	if (chance(50))
	{
		switch (dsq->continuity.form)
		{
		case FORM_BEAST:
			sound->playSfx("BeastBurst", (128+rand()%64)/256.0f);
		break;
		default: ;
		}
	}
}

bool Game::collideCircleVsCircle(Entity *a, Entity *b)
{
	return (a->position - b->position).isLength2DIn(a->collideRadius + b->collideRadius);
}

bool Game::collideHairVsCircle(Entity *a, int num, const Vector &pos2, float radius, float perc, int *colSegment)
{
	if (perc == 0)
		perc = 1;
	bool c = false;
	if (a && a->hair)
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
				if (colSegment)
					*colSegment = i;
				return true;
			}
		}
	}
	return c;
}

// NOTE THIS FUNCTION ASSUMES THAT IF A BONE ISN'T AT FULL ALPHA (1.0) IT IS DISABLED
Bone *Game::collideSkeletalVsCircle(Entity *skeletal, CollideQuad *circle)
{
	return collideSkeletalVsCircle(skeletal, circle->position, circle->collideRadius);
}

Bone *Game::collideSkeletalVsLine(Entity *skeletal, Vector start, Vector end, float radius)
{
	Bone *closest = 0;
	for (size_t i = 0; i < skeletal->skeletalSprite.bones.size(); i++)
	{
		Bone *b = skeletal->skeletalSprite.bones[i];

		// MULTIPLE CIRCLES METHOD
		if (b->canCollide() && !b->collisionMask.empty())
		{
			for (size_t i = 0; i < b->transformedCollisionMask.size(); i++)
			{
				if (isTouchingLine(start, end, b->transformedCollisionMask[i], radius+b->collideRadius))
				{
					closest = b;
					break;
				}
			}
		}
		if (closest != 0)
		{
			break;
		}
	}
	return closest;
}

bool Game::collideCircleVsLine(CollideQuad *r, Vector start, Vector end, float radius)
{
	bool collision = false;
	if (isTouchingLine(start, end, r->position, radius+r->collideRadius, &lastCollidePosition))
	{
		collision = true;
	}
	return collision;
}

bool Game::collideCircleVsLineAngle(CollideQuad *r, float angle, float startLen, float endLen, float radius, Vector basePos)
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
	return collision;
}

Bone *Game::collideSkeletalVsCircle(Entity *skeletal, Vector pos, float radius)
{
	float smallestDist = HUGE_VALF;
	Bone *closest = 0;
	if (!(pos - skeletal->position).isLength2DIn(2000)) return 0;
	for (size_t i = 0; i < skeletal->skeletalSprite.bones.size(); i++)
	{
		Bone *b = skeletal->skeletalSprite.bones[i];

		if (b->canCollide()) // check this here to avoid calculating getWorldCollidePosition() if not necessary
		{
			float checkRadius = sqr(radius+b->collisionMaskRadius);
			Vector bonePos = b->getWorldCollidePosition();
			float dist = (bonePos - pos).getSquaredLength2D();
			// MULTIPLE CIRCLES METHOD
			if (!b->collisionMask.empty())
			{
				if (dist < checkRadius)
				{
					for (size_t i = 0; i < b->transformedCollisionMask.size(); i++)
					{
						if ((b->transformedCollisionMask[i] - pos).isLength2DIn(radius+b->collideRadius*skeletal->scale.x))
						{
							closest = b;
							smallestDist = dist;
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
		avatar->warpInLocal = avatar->position;
	}
	else if (localWarpType == LOCALWARP_OUT)
	{
		avatar->warpInLocal = Vector(0,0,0);
	}

	dsq->screenTransition->capture();
	core->resetTimer();
}

void Game::postLocalWarp()
{
	if (li && dsq->continuity.hasLi())
		li->position = avatar->position;
	if (avatar->pullTarget)
		avatar->pullTarget->position = avatar->position;
	snapCam();
	dsq->screenTransition->transition(0.6f);

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
	if (!shot->isHitEnts() || shot->firer == e)
	{
		return false;
	}
	if (shot->checkDamageTarget)
	{
		if (!e->isDamageTarget(shot->getDamageType()))
			return false;
	}
	if (e->getEntityType() == ET_ENEMY)
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
	else if (e->getEntityType() == ET_AVATAR)
	{
		// this used to be stuff != ET_AVATAR.. but what else would do that
		return !isDamageTypeAvatar(shot->getDamageType()) && (!shot->firer || shot->firer->getEntityType() == ET_ENEMY);
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
	for (size_t i = 0; i < Shot::shots.size(); ++i)
	{
		Shot *shot = Shot::shots[i];
		if (shot->isActive() && isEntityCollideWithShot(e, shot) && (!hasShield || (!shot->shotData || !shot->shotData->ignoreShield)))
		{
			Vector collidePoint = e->position+e->offset;
			if (e->getNumTargetPoints()>0)
			{
				collidePoint = e->getTargetPoint(0);
			}
			if ((collidePoint - shot->position).isLength2DIn(shot->collideRadius + e->collideRadius))
			{
				lastCollidePosition = shot->position;
				shot->hitEntity(e,0);
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
	for (size_t i = 0; i < Shot::shots.size(); ++i)
	{
		Shot *shot = Shot::shots[i];
		if (shot->isActive() && isEntityCollideWithShot(e, shot))
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

void Game::handleShotCollisionsHair(Entity *e, int num, float perc)
{
	for (size_t i = 0; i < Shot::shots.size(); ++i)
	{
		Shot *shot = Shot::shots[i];
		if (shot->isActive() && isEntityCollideWithShot(e, shot))
		{
			bool b = collideHairVsCircle(e, num, shot->position, 8, perc);
			if (b)
			{
				lastCollidePosition = shot->position;
				shot->hitEntity(e, 0);
			}
		}
	}
}

void Game::toggleSceneEditor()
{
	if (!core->getAltState())
	{
		sceneEditor.toggle();
	}
}

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
	float a = 0;
	if (gridRender->alpha == 0)
		a = 0.5f;

	gridRender->alpha.interpolateTo(a, t);
	gridRender2->alpha.interpolateTo(a, t);
	gridRender3->alpha.interpolateTo(a, t);
	edgeRender->alpha.interpolateTo(a, t);
	gridRenderEnt->alpha.interpolateTo(a, t);
	gridRenderUser1->alpha.interpolateTo(a, t);
	gridRenderUser2->alpha.interpolateTo(a, t);
}

Vector Game::getCameraPositionFor(const Vector &pos)
{
	return Vector(pos.x - 400 * core->invGlobalScale, pos.y - 300 * core->invGlobalScale, 0);
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

void Game::updateCursor(float dt)
{
	bool rotate = false;

	if (dsq->getInputMode() == INPUT_MOUSE)
	{
		dsq->cursor->offset.stop();
		dsq->cursor->offset = Vector(0,0);
		//debugLog("offset lerp stop in mouse!");
	}
	else if (dsq->getInputMode() == INPUT_JOYSTICK)
	{
		if (!isPaused() || isInGameMenu() || !avatar->isInputEnabled())
		{
			int offy = -60;
			if (isInGameMenu() || !avatar->isInputEnabled())
			{
				//cursor->setTexture("");
				offy = 0;
			}
			if (!dsq->cursor->offset.isInterpolating())
			{
				//debugLog("offset lerp!");
				dsq->cursor->offset = Vector(0, offy);
				dsq->cursor->offset.interpolateTo(Vector(0, offy-20), 0.4f, -1, 1, 1);
			}
		}
		else
		{
			//debugLog("offset lerp stop in joystick!");
			dsq->cursor->offset.stop();
			dsq->cursor->offset = Vector(0,0);
		}
	}

	if (isSceneEditorActive() || isPaused() || (!avatar || !avatar->isInputEnabled()) ||
		(miniMapRender && miniMapRender->isCursorIn())
		)
	{
		dsq->setCursor(CURSOR_NORMAL);
		// Don't show the cursor in keyboard/joystick mode if it's not
		// already visible (this keeps the cursor from appearing for an
		// instant during map fadeout).
		if (dsq->getInputMode() == INPUT_MOUSE || isSceneEditorActive() || isPaused())
			dsq->cursor->alphaMod = 0.5;

		/*
		dsq->cursor->offset.stop();
		dsq->cursor->offset = Vector(0,0);
		*/
	}
	else if (avatar)
	{
		//Vector v = avatar->getVectorToCursorFromScreenCentre();
		if (dsq->getInputMode() == INPUT_JOYSTICK)// && !avatar->isSinging() && !isInGameMenu() && !isPaused())
		{
			dsq->cursor->alphaMod = 0;
			if (!avatar->isSinging())
				core->setMousePosition(core->center);
			if (!isPaused())
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
		else if (isInGameMenu() || v.isLength2DIn(avatar->getStopDistance()) || (avatar->entityToActivate || avatar->pathToActivate))
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
		float vw2 = core->getVirtualOffX()*core->invGlobalScale;
		float vh2 = core->getVirtualOffY()*core->invGlobalScale;

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

		// The camera is positioned at (0, 0) screen coordinates, which, on widescreen resolutions,
		// is *not* the upper left corner. Subtract the offset to get the real position.
		// HACK: One column shows through after blackness ends, adding TILE_SIZE fixes this. -- fg
		float scrw = (core->getVirtualWidth()-core->getVirtualOffX()+TILE_SIZE)*core->invGlobalScale;
		float scrh = 600*core->invGlobalScale;

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


		if (isObstructed(TileVector(pos)))
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
			if (isObstructed(tl) || isObstructed(tr))
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

	Path *p = getNearestPath(avatar->position, PATH_BGSFXLOOP);
	if (p && p->isCoordinateInside(avatar->position) && !p->content.empty())
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
			play.priority = 0.7f;
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
		switchBgLoop(0);
	}
	else
		switchBgLoop(1);
}

const float helpTextScrollSpeed = 800.0f;
const float helpTextScrollClickAmount = 340.0f;
const float helpTextScrollClickTime = -helpTextScrollSpeed;
void Game::onHelpDown()
{
	float to = helpText->offset.y - helpTextScrollClickAmount;
	if (to < -helpText->getHeight() + core->getVirtualHeight())
	{
		to = -helpText->getHeight() + core->getVirtualHeight();
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
		if (isActing(ACTION_SWIMDOWN, -1))
		{
			helpText->offset.stop();
			helpText->offset.y -= helpTextScrollSpeed * dt;
			if (helpText->offset.y < -helpText->getHeight() + core->getVirtualHeight())
			{
				helpText->offset.y = -helpText->getHeight() + core->getVirtualHeight();
			}
		}
		if (isActing(ACTION_SWIMUP, -1))
		{
			helpText->offset.stop();
			helpText->offset.y += helpTextScrollSpeed * dt;
			if (helpText->offset.y > 0)
			{
				helpText->offset.y = 0;
			}
		}
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

	if (avatar)
	{
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
	}
	updateParticlePause();
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


	size_t i = 0;
	for (i = 0; i < getNumPaths(); i++)
	{
		getPath(i)->update(dt);
	}

	FOR_ENTITIES(j)
	{
		(*j)->postUpdate(dt);
	}

	FlockEntity::updateFlockData();

	updateCursor(dt);

	updateBgSfxLoop();

	sceneColor.update(dt);
	sceneColor2.update(dt);
	sceneColor3.update(dt);
	dsq->sceneColorOverlay->color = sceneColor * sceneColor2 * sceneColor3;

	themenu->update(dt);

	sceneEditor.update(dt);

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
		if (deathTimer > 0)
		{
			deathTimer -= dt;
			if (deathTimer <= 0)
			{
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
	{
		dsq->continuity.refreshAvatarData(avatar);

		Vector bigGlow(3,3);
		float bigGlowTime = 0.4f;
		bool hadThingToActivate = (avatar->entityToActivate!=0 || avatar->pathToActivate!=0);
		avatar->entityToActivate = 0;

		if (avatar->canActivateStuff())
		{
			FOR_ENTITIES(i)
			{
				Entity *e = *i;
				float sqrLen = (dsq->getGameCursorPosition() - e->position).getSquaredLength2D();
				if (sqrLen < sqr(e->activationRadius)
					&& (avatar->position-e->position).getSquaredLength2D() < sqr(e->activationRange)
					&& e->activationType == Entity::ACT_CLICK
					&& !e->position.isInterpolating()
					)
				{
					//if (trace(avatar->position, e->position))
					{
						avatar->entityToActivate = e;
						dsq->cursorGlow->alpha.interpolateTo(1, 0.2f);
						dsq->cursorBlinker->alpha.interpolateTo(1.0f,0.1f);
						if (!hadThingToActivate)
						{
							dsq->cursorGlow->scale = Vector(1,1);
							dsq->cursorGlow->scale.interpolateTo(bigGlow,bigGlowTime,1, true, true);
						}
					}
					break;
				}
			}
		}

		avatar->pathToActivate = 0;

		// make sure you also disable entityToActivate
		if (avatar->canActivateStuff())
		{
			Path* p = getScriptedPathAtCursor(true);
			if (p && p->cursorActivation)
			{
				Vector diff = p->nodes[0].position - avatar->position;

				if (p->isCoordinateInside(avatar->position) || diff.getSquaredLength2D() < sqr(p->activationRange))
				{
					//if (trace(avatar->position, p->nodes[0].position))
					{
						avatar->pathToActivate = p;
						dsq->cursorGlow->alpha.interpolateTo(1,0.2f);
						dsq->cursorBlinker->alpha.interpolateTo(1,0.2f);
						if (!hadThingToActivate)
						{
							dsq->cursorGlow->scale = Vector(1,1);
							dsq->cursorGlow->scale.interpolateTo(bigGlow,bigGlowTime,1, true, true);
						}
					}
				}
			}
		}
	}

	if (!activation)
	{
		avatar->entityToActivate = 0;
		avatar->pathToActivate = 0;
	}

	if (!avatar->entityToActivate && !avatar->pathToActivate)
	{
		dsq->cursorGlow->alpha.interpolateTo(0, 0.2f);
		dsq->cursorBlinker->alpha.interpolateTo(0, 0.1f);
	}

	if (!isSceneEditorActive())
	{
		if (!isPaused())
			waterLevel.update(dt);

		if (cameraFollow)
		{
			Vector dest = *cameraFollow;

			if (avatar)
			{
				if (avatar->looking && !isPaused()) {
					Vector diff = avatar->getAim();//dsq->getGameCursorPosition() - avatar->position;
					diff.capLength2D(maxLookDistance);
					dest += diff;
				}
				else {
					avatar->looking = 0;
				}
			}

			if (cameraLerpDelay==0)
			{
				//cameraLerpDelay = 0.15;
				cameraLerpDelay = vars->defaultCameraLerpDelay;
			}
			cameraInterp.stop();
			cameraInterp.interpolateTo(dest, cameraLerpDelay);
			dsq->cameraPos = getCameraPositionFor(cameraInterp);
			constrainCamera();
		}

		cameraInterp.update(dt);
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
		s->init();
	}

	return s;
}

void Game::warpCameraTo(RenderObject *r)
{
	warpCameraTo(r->position);
}

void Game::warpCameraTo(Vector position)
{
	cameraInterp.stop();
	cameraInterp = position;
	dsq->cameraPos = getCameraPositionFor(position);
}

void Game::snapCam()
{
	if (cameraFollow)
		warpCameraTo(*cameraFollow);
}

bool Game::loadElementTemplates(std::string pack, const unsigned char *usedIdx, size_t usedIdxLen)
{
	stringToLower(pack);

	std::string fn;
	if (dsq->mod.isActive())
		fn = dsq->mod.getPath() + "tilesets/" + pack + ".txt";
	else
		fn = "data/tilesets/" + pack + ".txt";

	if(!tileset.loadFile(fn.c_str(), usedIdx, usedIdxLen))
	{
		errorLog ("Could not load tileset [" + fn + "]");
		return false;
	}

	// Aquarian alphabet letters
	if(const CountedPtr<Texture> aqtex = dsq->getTexture("aquarian"))
	{
		const float cell = 64.0f/512.0f;
		for (int i = 0; i < 27; i++)
		{
			ElementTemplate t;
			t.idx = 1024+i;
			t.tex = aqtex;
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

			tileset.elementTemplates.push_back(t);
		}
	}

	return true;
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

void Game::removeState()
{
	const float fadeTime = 0.25;

	dsq->toggleVersionLabel(false);

	dsq->subtitlePlayer.hide(fadeTime);

	debugLog("Entering Game::removeState");
	shuttingDownGameState = true;
	debugLog("avatar->endOfGameState()");
	if (avatar)
	{
		avatar->endOfGameState();
	}

	debugLog("toggle sceneEditor");
	if (sceneEditor.isOn())
		sceneEditor.toggle(false);

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
	if(noSceneTransitionFadeout)
		noSceneTransitionFadeout = false;
	else
	{
		dsq->overlay->color = 0;
		dsq->overlay->alpha.interpolateTo(1, fadeTime);
		dsq->run(fadeTime);
	}

	dsq->rumble(0,0,0,-1, INPUT_JOYSTICK);

	dsq->sound->clearFadingSfx();


	ingredients.clear();

	core->particlesPaused = false;

	elementUpdateList.clear();
	elementInteractionList.clear();

	dsq->setCursor(CURSOR_NORMAL);
	dsq->darkLayer.toggle(0);
	dsq->shakeCamera(0,0);
	if (core->afterEffectManager)
		core->afterEffectManager->clear();

	dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = true;

	if (saveFile)
	{
		delete saveFile;
		saveFile = 0;
	}
	dsq->continuity.zoom = core->globalScale;

	toggleOverrideZoom(false);
	avatar->myZoom.stop();
	dsq->globalScale.stop();

	avatar->myZoom = Vector(1,1);
	dsq->globalScale = Vector(1,1);
	core->globalScaleChanged();

	for (size_t i = 0; i < getNumPaths(); i++)
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
	sceneEditor.shutdown();

	cameraFollow = 0;
	core->cameraPos = Vector(0,0);
	sceneColor.stop();

	controlHint_mouseLeft = controlHint_mouseRight = controlHint_mouseMiddle = controlHint_mouseBody = controlHint_bg = controlHint_image = 0;
	controlHint_text = 0;

	miniMapRender = 0;
	gridRender = 0;
	gridRender2 = 0;
	gridRender3 = 0;
	edgeRender = 0;
	gridRenderEnt = 0;
	gridRenderUser1 = 0;
	gridRenderUser2 = 0;
	worldMapRender = 0;

	clearObsRows();


	debugLog("killAllShots");
	Shot::killAllShots();
	Shot::clearShotGarbage(); // make sure there are no pointers left (would lead to a crash on shutdown otherwise)
	debugLog("killAllBeams");
	Beam::killAllBeams();
	debugLog("killAllWebs");
	Web::killAllWebs();
	debugLog("killAllSpores");
	Spore::killAllSpores();

	debugLog("clear Local Sounds");
	core->sound->clearLocalSounds();

	active = false;

	debugLog("Game::removeState Done");
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

bool Game::collideCircleWithGrid(const Vector& position, float r)
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

void Game::learnedRecipe(Recipe *r, bool effects)
{
	if (nocasecmp(dsq->getTopStateData()->name,"Game")==0 && !applyingState)
	{
		std::ostringstream os;
		os << stringbank.get(23) << " "  << r->resultDisplayName << " " << stringbank.get(24);
		IngredientData *data = dsq->continuity.getIngredientDataByName(r->result);
		if (data)
		{
			if (effects)
			{
				setControlHint(os.str(), 0, 0, 0, 3, std::string("gfx/ingredients/") + data->gfx);
			}
		}
	}
}

void Game::setIgnoreAction(AquariaActions ac, bool ignore)
{
	if (ignore)
	{
		if(!isIgnoreAction(ac))
			ignoredActions.push_back(ac);
	}
	else
	{
		for(size_t i = 0; i < ignoredActions.size(); ++i)
		{
			if(ignoredActions[i] == ac)
			{
				ignoredActions[i] = ignoredActions.back();
				ignoredActions.pop_back();
				break;
			}
		}
	}
}

bool Game::isIgnoreAction(AquariaActions ac) const
{
	for(size_t i = 0; i < ignoredActions.size(); ++i)
		if(ignoredActions[i] == ac)
			return true;
	return false;
}

void Game::onContinuityReset()
{
	themenu->onContinuityReset();
}
