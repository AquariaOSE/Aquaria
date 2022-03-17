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
	fromVel = Vector(0,-1);
	deathTimer = 0;

	game = this;
	cameraFollow = 0;

	worldMapRender = 0;

	for (int i = 0; i < PATH_MAX; i++)
		firstPathOfType[i] = 0;

	loadEntityTypeList();

	lastCollideMaskIndex = -1;
	worldPaused = false;

	cookingScript = 0;
}

Game::~Game()
{
	delete themenu;
	tileCache.clean();
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

ElementTemplate *Game::getElementTemplateByIdx(size_t idx)
{
	for (size_t i = 0; i < elementTemplates.size(); i++)
	{
		if (elementTemplates[i].idx == idx)
		{
			return &elementTemplates[i];
		}
	}
	return 0;
}

Element* Game::createElement(size_t idx, Vector position, size_t bgLayer, RenderObject *copy, ElementTemplate *et)
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

			//dsq->game->setGrid(TileVector(tpos.x+(w2*TILE_SIZE)+(x/TILE_SIZE), tpos.y+(h2*TILE_SIZE)+(y/TILE_SIZE)), obsType);
			TileVector tvec(tpos.x+w2+x, tpos.y+h2+y);
			if (!dsq->game->isObstructed(tvec))
				dsq->game->addGrid(tvec, obsType);

		}
		glPopMatrix();
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
}

void Game::trimGrid()
{
	// Prevent the left- and rightmost column of black tiles
	// from beeing drawn. (The maps were designed with this mind...)
	for (int x = 0; x < MAX_GRID; x++)
	{
		const unsigned char *curCol   = grid[x]; // safe
		const unsigned char *leftCol  = dsq->game->getGridColumn(x-1); // unsafe
		const unsigned char *rightCol = dsq->game->getGridColumn(x+1); // unsafe
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
				int xDiff = pos.x-vt.x;
				int yDiff = pos.y-vt.y;
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
			}
		}
	}
	const int sz = (TILE_SIZE*(sampleArea-1));
	for (size_t i = 0; i < vs.size(); i++)
	{
		float len = vs[i].getLength2D();
		if (len < sz)
		{
			vs[i].setLength2D(sz - len);
			c++;
			avg += vs[i];
		}
	}
	if (c)
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
	XMLElement *e = this->saveFile->FirstChildElement("Enemy");
	while (e)
	{
		int x = atoi(e->Attribute("x"));
		int y = atoi(e->Attribute("y"));
		if (int(selected->startPos.x) == x && int(selected->startPos.y) == y)
		{
			this->saveFile->DeleteChild(e);
			//delete e;
			return true;
		}

		e = e->NextSiblingElement("Enemy");
	}

	for (size_t i = 0; i < entitySaveData.size(); i++)
	{
		if (entitySaveData[i].x == int(selected->startPos.x) && entitySaveData[i].y == int(selected->startPos.y))
		{
			std::vector<EntitySaveData> copy = entitySaveData;
			entitySaveData.clear();
			for (size_t j = 0; j < copy.size(); j++)
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

Entity *Game::createEntity(int idx, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType et, bool doPostInit)
{
	std::string type;
	for (size_t i = 0; i < dsq->game->entityTypeList.size(); i++)
	{
		EntityClass *ec = &dsq->game->entityTypeList[i];
		if (ec->idx == idx)
		{
			type = ec->name;
			return createEntity(type, id, position, rot, createSaveData, name, et, doPostInit);
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

Entity* Game::establishEntity(Entity *e, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType et, bool doPostInit)
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
			e->assignUniqueID(!createSaveData); // when entity is placed on map, give positive ID; otherwise, if script-spawned, give negative ID
		}
	}

	// NOTE: init cannot be called after "addRenderObject" for some unknown reason
	e->init();

	Vector usePos = position;
	e->startPos = usePos;
	if (!name.empty())
		e->name = name;

	e->rotation.z = rot;

	int idx = getIdxForEntityType(type);
	e->entityTypeIdx = idx;


	if (createSaveData)
	{
		int idx = dsq->game->getIdxForEntityType(type);
		entitySaveData.push_back(EntitySaveData(e, idx, usePos.x, usePos.y, rot, e->getID(), e->name));
	}

	addRenderObject(e, e->layer);

	if (doPostInit)
	{
		e->postInit();
	}

	return e;
}

Entity *Game::createEntity(const std::string &t, int id, Vector position, int rot, bool createSaveData, std::string name, EntityType et, bool doPostInit)
{
	std::string type = t;
	stringToLower(type);

	ScriptedEntity *e;

	e = new ScriptedEntity(type, position, et);


	return establishEntity(e, id, position, rot, createSaveData, name, et, doPostInit);
}

EntitySaveData *Game::getEntitySaveDataForEntity(Entity *e, Vector pos)
{

	for (size_t i = 0; i < entitySaveData.size(); i++)
	{
		if (entitySaveData[i].e == e)
		{
			return &entitySaveData[i];
		}
	}
	return 0;
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

void Game::generateCollisionMask(Quad *q, float overrideCollideRadius /* = 0 */)
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
	for (size_t i = 0; i < dsq->game->paths.size(); i++)
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
	for (size_t i = 0; i < paths.size(); i++)
	{
		if (paths[i]->label == name)
			return paths[i];
	}
	return 0;
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
	elementTemplatePack = "Main";
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

	XMLElement *level = doc.FirstChildElement("Level");
	if (level)
	{
		XMLElement *levelSF = saveFile->NewElement("Level");
		if (level->Attribute("tileset"))
		{
			elementTemplatePack = level->Attribute("tileset");
			loadElementTemplates(elementTemplatePack);
			levelSF->SetAttribute("tileset", elementTemplatePack.c_str());
		}
		else if (level->Attribute("elementTemplatePack"))
		{
			elementTemplatePack = level->Attribute("elementTemplatePack");
			loadElementTemplates(elementTemplatePack);
			levelSF->SetAttribute("tileset", elementTemplatePack.c_str());
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
		if (level->Attribute("worldMapIndex"))
		{
			worldMapIndex = atoi(level->Attribute("worldMapIndex"));
			levelSF->SetAttribute("worldMapIndex", worldMapIndex);
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

		if (level->Attribute("bgRepeat"))
		{
			SimpleIStringStream is(level->Attribute("bgRepeat"));
			is >> backgroundImageRepeat;
			levelSF->SetAttribute("bgRepeat", level->Attribute("bgRepeat"));
		}
		if (level->Attribute("cameraConstrained"))
		{
			SimpleIStringStream is(level->Attribute("cameraConstrained"));
			is >> cameraConstrained;
			levelSF->SetAttribute("cameraConstrained", cameraConstrained);
			std::ostringstream os;
			os << "cameraConstrained: " << cameraConstrained;
			debugLog(os.str());
		}
		if (level->Attribute("maxZoom"))
		{
			maxZoom = atof(level->Attribute("maxZoom"));
			std::ostringstream os;
			os << maxZoom;
			levelSF->SetAttribute("maxZoom", os.str().c_str());
		}
		if (level->Attribute("bg"))
		{
			std::string tex = std::string(level->Attribute("bg"));
			if (!tex.empty())
			{
				bg->setTexture(tex);
				bg->setWidthHeight(900,600);
				levelSF->SetAttribute("bg", tex.c_str());
			}
			else
			{
				bg->alpha = 0;
			}
		}
		else
		{
			bg->alpha = 0;
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

		if (level->Attribute("bg2"))
		{

			std::string tex = std::string(level->Attribute("bg2"));
			if (!tex.empty())
			{
				bg2->setTexture(tex);
				bg2->setWidthHeight(900,600);
				levelSF->SetAttribute("bg2", tex.c_str());

			}
			else
				bg2->alpha = 0;

			bg2->alpha = 0;
			bg->alpha = 0;
		}
		else
		{
			bg2->alpha = 0;
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
				levelSF->SetAttribute("bd-x", x);
				levelSF->SetAttribute("bd-y", y);
			}
			if (level->Attribute("bd-w") && level->Attribute("bd-h"))
			{
				int w = atoi(level->Attribute("bd-w"));
				int h = atoi(level->Attribute("bd-h"));
				backdropQuad->setWidthHeight(w, h);
				levelSF->SetAttribute("bd-w", w);
				levelSF->SetAttribute("bd-h", h);
			}
			backdropQuad->toggleCull(false);
			addRenderObject(backdropQuad, LR_SCENEBACKGROUNDIMAGE);

			// upper left justify
			backdropQuad->offset =
				Vector((backdropQuad->getWidth()*backdropQuad->scale.x)/2.0f,
				(backdropQuad->getHeight()*backdropQuad->scale.y)/2.0f);
			// save
			levelSF->SetAttribute("backdrop", backdrop.c_str());
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

	XMLElement *quad = doc.FirstChildElement("Quad");
	while (quad)
	{
		XMLElement *qSF = saveFile->NewElement("Quad");
		int x=0, y=0, z=0;
		int w=0,h=0;
		bool cull=true;
		bool solid = false;
		std::string justify;
		std::string tex;
		qSF->SetAttribute("x", x = atoi(quad->Attribute("x")));
		qSF->SetAttribute("y", y = atoi(quad->Attribute("y")));
		qSF->SetAttribute("w", w = atoi(quad->Attribute("w")));
		qSF->SetAttribute("h", h = atoi(quad->Attribute("h")));
		qSF->SetAttribute("tex", (tex = (quad->Attribute("tex"))).c_str());
		qSF->SetAttribute("cull", cull = atoi(quad->Attribute("cull")));
		qSF->SetAttribute("justify", (justify = (quad->Attribute("justify"))).c_str());

		if (quad->Attribute("solid"))
			qSF->SetAttribute("solid", solid = atoi(quad->Attribute("solid")));

		Quad *q = new Quad;
		q->position = Vector(x,y,z);
		q->setTexture(tex);
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

	std::vector<Element*> loadedElements;
	loadedElements.reserve(200);
	XMLElement *simpleElements = doc.FirstChildElement("SE");
	while (simpleElements)
	{
		int idx, x, y, rot;
		float sz,sz2;
		loadedElements.clear();
		if (simpleElements->Attribute("d"))
		{
			SimpleIStringStream is(simpleElements->Attribute("d"));
			while (is >> idx)
			{
				is >> x >> y >> rot;
				Element *e = createElement(idx, Vector(x,y), 4);
				e->rotation.z = rot;
				loadedElements.push_back(e);
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
				loadedElements.push_back(e);
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
				loadedElements.push_back(e);
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
				loadedElements.push_back(e);
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
				loadedElements.push_back(e);
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
				loadedElements.push_back(e);
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
				loadedElements.push_back(e);
			}
		}
		if (simpleElements->Attribute("k"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("k"));
			int l = atoi(simpleElements->Attribute("l"));
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

				loadedElements.push_back(e);
			}
		}
		if (simpleElements->Attribute("repeatScale"))
		{
			SimpleIStringStream is2(simpleElements->Attribute("repeatScale"));
			for(size_t i = 0; i < loadedElements.size(); ++i)
			{
				Element *e = loadedElements[i];
				if(e->isRepeatingTextureToFill())
				{
					float repeatScaleX = 1, repeatScaleY = 1;
					if(!(is2 >> repeatScaleX >> repeatScaleY))
						break;
					e->repeatToFillScale.x = repeatScaleX;
					e->repeatToFillScale.y = repeatScaleY;
					e->refreshRepeatTextureToFill();
				}
			}
		}
		simpleElements = simpleElements->NextSiblingElement("SE");
	}

	XMLElement *element = doc.FirstChildElement("Element");
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

	XMLElement *entitiesNode = doc.FirstChildElement("Entities");
	while(entitiesNode)
	{
		if (entitiesNode->Attribute("j"))
		{
			SimpleIStringStream is(entitiesNode->Attribute("j"));
			int idx, x, y, rot, groupID, id;
			std::string name;
			while (is >> idx)
			{
				name="";
				if (idx == -1)
					is >> name;
				is >> x >> y >> rot >> groupID >> id;

				if (!name.empty())
					dsq->game->createEntity(name, id, Vector(x,y), rot, true, "", ET_ENEMY);
				else
					dsq->game->createEntity(idx, id, Vector(x,y), rot, true, "", ET_ENEMY);
			}
		}
		entitiesNode = entitiesNode->NextSiblingElement("Entities");
	}

	this->reconstructGrid(true);
	rebuildElementUpdateList();
	setElementLayerFlags();

	// HACK: Don't try to optimize the barrier layer in Mithalas Cathedral
	// since elements are turned off dynamically.
	if (nocasecmp(scene, "cathedral02") == 0)
		dsq->getRenderObjectLayer(LR_ELEMENTS3)->setOptimizeStatic(false);

	findMaxCameraValues();

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
		level->SetAttribute("waterLevel", dsq->game->saveWaterLevel);

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


	for (size_t i = 0; i < dsq->game->getNumPaths(); i++)
	{
		XMLElement *pathXml = saveFile.NewElement("Path");
		Path *p = dsq->game->getPath(i);
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

	for (size_t i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		std::ostringstream& SE = simpleElements[e->bgLayer];
		SE << e->templateIdx << " "
		   << int(e->position.x) << " "
		   << int(e->position.y) << " "
		   << int(e->rotation.z) << " "
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
	}

	if (dsq->game->entitySaveData.size() > 0)
	{
		XMLElement *entitiesNode = saveFile.NewElement("Entities");

		std::ostringstream os;
		for (size_t i = 0; i < dsq->game->entitySaveData.size(); i++)
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
			std::string repeatScaleStr = simpleElements_repeatScale[i].str();
			if(!repeatScaleStr.empty())
				simpleElementsXML->SetAttribute("repeatScale", repeatScaleStr.c_str());
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
		if (bg)
			bg->blendEnabled = true;
		if (bg2)
			bg2->blendEnabled = true;
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
	bool aboveWaterLine = (pos.y <= dsq->game->waterLevel.x+20);
	bool inWaterBubble = false;
	if (!aboveWaterLine)
	{
		Path *p = dsq->game->getNearestPath(pos, PATH_WATERBUBBLE);
		if (p && p->active)
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

#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (id == ACTION_TOGGLESCENEEDITOR && !state)		toggleSceneEditor();
#endif

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

	dsq->toggleBlackBars(false);

	dsq->setTexturePointers();

	cameraOffBounds = false;


	ingOffY = 0;
	ingOffYTimer = 0;

	AquariaGuiElement::canDirMoveGlobal = true;

	dsq->toggleVersionLabel(false);

	activation = true;

	active = true;

	hasPlayedLow = false;

	firstSchoolFish = true;
	invincibleOnNested = true;

	controlHintNotes.clear();

	worldMapIndex = -1;

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
		dsq->game->setElementLayerVisible(i-LR_ELEMENTS1, true);
	}

	dsq->applyParallaxUserSettings();

	controlHintTimer = 0;
	cameraConstrained = true;
	// reset parallax
	RenderObjectLayer *l = 0;
	for (int i = LR_ELEMENTS10; i <= LR_ELEMENTS16; i++)
	{
		l = &dsq->renderObjectLayers[i];
		l->followCamera = 0;
		l->followCameraLock = 0;
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
	backdropQuad = 0;
	clearObsRows();
	useWaterLevel = false;
	waterLevel = saveWaterLevel = 0;

	dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = false;

	backgroundImageRepeat = 1;
	grad = 0;
	maxZoom = -1;
	maxLookDistance = 600;
	saveFile = 0;
	deathTimer = 0.9f;
	runGameOverScript = false;
	paused = false;
	//sceneColor = Vector(0.75, 0.75, 0.8);
	sceneColor = Vector(1,1,1);
	sceneName = "";
	elementTemplatePack ="";
	clearGrid();
	bg = 0;
	bg2 = 0;
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

	bg2 = new Quad;
	{
		bg2->position = Vector(400, 300, -3/*-0.09f*/);
		//bg2->color = Vector(0.9, 0.9, 0.9);
		bg2->setTexture("missingImage");
		bg2->setWidthHeight(900,600);
		//bg2->blendEnabled = false;
		bg2->followCamera =1;
		bg2->alpha = 0.8f;
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
		controlHint_bg->alphaMod = 0.7f;
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
		dsq->game->avatar->flipHorizontal();
		toFlip = -1;
	}

	bindInput();

	if (verbose) debugLog("Loading Scene");
	if (!loadScene(sceneToLoad))
	{
		loadElementTemplates(elementTemplatePack);
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
	blackRender->blendEnabled = false;
	addRenderObject(blackRender, LR_ELEMENTS4);

	miniMapRender = new MiniMapRender;
	// position is set in minimaprender::onupdate
	miniMapRender->scale = Vector(0.55f, 0.55f);
	addRenderObject(miniMapRender, LR_MINIMAP);

	timerText = new BitmapText(&dsq->smallFont);
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
		for (size_t i = 0; i < dsq->game->getNumPaths(); i++)
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
		core->run(0.5);
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
		dsq->run(0.5f);
		dsq->toggleCursor(true);
		dsq->tfader->alpha.interpolateTo(0, 0.2f);
		dsq->run(0.21f);
		dsq->clearSaveSlots(false);
	}

	if (verbose) debugLog("reset timer");

	applyingState = false;

	if (!dsq->doScreenTrans)
	{
		dsq->toggleCursor(true, 0.5);
	}

	debugLog("Game::applyState Done");
}

void Game::bindInput()
{
	if (!(this->applyingState || this->isActive())) return;

	ActionMapper::clearActions();
	//ActionMapper::clearCreatedEvents();

	addAction(ACTION_ESC, KEY_ESCAPE, -1);


#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (dsq->canOpenEditor())
	{
		addAction(ACTION_TOGGLESCENEEDITOR, KEY_TAB, -1);
	}
#endif

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
		dsq->game->toggleOverrideZoom(false);
	}
	else
	{
		dsq->game->toggleOverrideZoom(true);
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
		if (dsq->game->worldMapRender->isOn() && !dsq->isNested())
		{
			dsq->game->worldMapRender->toggle(false);
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
Bone *Game::collideSkeletalVsCircle(Entity *skeletal, RenderObject *circle)
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
		if (!b->collisionMask.empty() && b->alpha.x == 1 && b->renderQuad)
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

		if (b->alpha.x == 1 && b->renderQuad &&
			(!b->collisionMask.empty() || b->collideRadius) // check this here to avoid calculating getWorldCollidePosition() if not necessary
		)
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

	if (isSceneEditorActive() || dsq->game->isPaused() || (!avatar || !avatar->isInputEnabled()) ||
		(dsq->game->miniMapRender && dsq->game->miniMapRender->isCursorIn())
		)
	{
		dsq->setCursor(CURSOR_NORMAL);
		// Don't show the cursor in keyboard/joystick mode if it's not
		// already visible (this keeps the cursor from appearing for an
		// instant during map fadeout).
		if (dsq->getInputMode() == INPUT_MOUSE || isSceneEditorActive() || dsq->game->isPaused())
			dsq->cursor->alphaMod = 0.5;

		/*
		dsq->cursor->offset.stop();
		dsq->cursor->offset = Vector(0,0);
		*/
	}
	else if (avatar)
	{
		//Vector v = avatar->getVectorToCursorFromScreenCentre();
		if (dsq->getInputMode() == INPUT_JOYSTICK)// && !avatar->isSinging() && !dsq->game->isInGameMenu() && !dsq->game->isPaused())
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
	for (i = 0; i < dsq->game->getNumPaths(); i++)
	{
		dsq->game->getPath(i)->update(dt);
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
	if (bg)
	{
		setParallaxTextureCoordinates(bg, 0.3f);
	}
	if (bg2)
	{
		setParallaxTextureCoordinates(bg2, 0.1f);
	}
	themenu->update(dt);

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
		if (dsq->game && dsq->game->avatar->canActivateStuff())
		{
			Path* p = dsq->game->getScriptedPathAtCursor(true);
			if (p && p->cursorActivation)
			{
				Vector diff = p->nodes[0].position - dsq->game->avatar->position;

				if (p->isCoordinateInside(dsq->game->avatar->position) || diff.getSquaredLength2D() < sqr(p->activationRange))
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
				if (avatar->looking && !dsq->game->isPaused()) {
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

	for (size_t i = 0; i < elementTemplates.size(); i++)
	{
		for (size_t j = i; j < elementTemplates.size(); j++)
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
	for (size_t i = 0; i < et->grid.size(); i++)
	{
		TileVector t(position);
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
		setGrid(s, OT_INVISIBLE);
	}
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

	dsq->overlay->alpha.interpolateTo(1, fadeTime);
	dsq->run(fadeTime);

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

	dsq->game->toggleOverrideZoom(false);
	dsq->game->avatar->myZoom.stop();
	dsq->globalScale.stop();

	dsq->game->avatar->myZoom = Vector(1,1);
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
				dsq->game->setControlHint(os.str(), 0, 0, 0, 3, std::string("gfx/ingredients/") + data->gfx);
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
