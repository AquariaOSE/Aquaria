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
#include "WorldMapRender.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "GridRender.h"
#include "Gradient.h"
#include "TTFFont.h"
#include "RoundedRect.h"
#include "RenderGrid.h"

#define GEM_GRAB			 10

namespace WorldMapRenderNamespace
{
	const float WORLDMAP_UNDERLAY_ALPHA = 0.8f;

	const float baseMapSegAlpha		= 0.0f;
	const float visibleMapSegAlpha	= 0.8f;

	const float blinkPeriod		= 0.2f;

	// Fraction of the screen width and height we consider "visited".
	// (We don't mark the entire screen "visited" because the player may
	// overlook things on the edge of the screen while moving.)
	const float visitedFraction	= 0.8f;

	float xMin, yMin, xMax, yMax;

	const float zoomMin = 0.2f;
	const float zoomMax = 3.0f;

	// Where does this come from?
	// This has been always there, the map data were made with it, and it needs to be there
	// to make everything look right.
	// The scale factor recorded in the world map data is just off by a factor of 4,
	// this fixes it and also documents the places where this factor needs to be used.
	const float tileScaleFactor = 0.25f;

	bool editorActive=false;
}

using namespace WorldMapRenderNamespace;

class GemMover;

GemMover *mover=0;

const float beaconSpawnBitTime = 0.05f;


class WorldMapBoundQuad : public Quad
{
public:
	WorldMapBoundQuad(const Vector &posToUse)
	{
		position = posToUse;
		truePosition = posToUse;
		followCamera = 1;
	}

protected:
	Vector truePosition;
	void onUpdate(float dt) OVERRIDE
	{
		Quad::onUpdate(dt);

		Vector wp = parent->getWorldCollidePosition(truePosition);
		Vector diff = wp - core->center;

		float w2 = core->getVirtualWidth()/2;
		float h2 = core->getVirtualHeight()/2;

		if (diff.x < -w2)
			wp.x = core->center.x - w2;
		if (diff.x > w2)
			wp.x = core->center.x + w2;
		if (diff.y < -h2)
			wp.y = core->center.y - h2;
		if (diff.y > h2)
			wp.y = core->center.y + h2;

		Vector move = wp - getWorldPosition();
		// If we don't scale the move vector properly, the dots overshoot at high zoom or don't go far
		// enough at low zoom and we can end up with a weird "disco" effect -- see icculus bug 4542.
		position += move / parent->getRealScale().x;
	}
};

class BeaconRender : public Quad
{
public:
	BeaconRender(BeaconData *beaconData) : Quad(), beaconData(beaconData)
	{
		renderQuad = false;
		setTexture("gui/minimap/ripple");
		position = beaconData->pos;
		truePosition = beaconData->pos;
		followCamera = 1;

		alpha = 0.5;
		color = beaconData->color;


		spawnBitTimer = 0;
	}


	Vector truePosition;

	ParticleEffect *pe;

	float spawnBitTimer;


protected:
	BeaconData *beaconData;

	void onUpdate(float dt)
	{
		Quad::onUpdate(dt);



		if (!game->worldMapRender->isOn()) return;

		const int lenRange = 125;
		const float pscale = 0.7f;

		float leftOver = dt;

		while (leftOver)
		{
			spawnBitTimer -= leftOver;
			if (spawnBitTimer <= 0)
			{
				leftOver = -spawnBitTimer;
				spawnBitTimer = beaconSpawnBitTime;

				float r = (rand()%100)/100.0f;
				float radius = r * 2*PI;
				float len = (rand()%lenRange);
				int x = sinf(radius)*len;
				int y = cosf(radius)*len;


				float t = 0.75;
				WorldMapBoundQuad *q = new WorldMapBoundQuad(Vector(x, y, 0));
				q->setTexture("particles/glow");
				q->alpha.ensureData();
				q->alpha.data->path.addPathNode(0.0, 0.0);
				q->alpha.data->path.addPathNode(1.0, 0.5);
				q->alpha.data->path.addPathNode(0.0, 1.0);
				q->alpha.startPath(0.5);
				q->alphaMod = 0.5;
				q->color = color;

				q->scale = Vector(pscale, pscale);

				q->setLife(1);
				q->setDecayRate(1.0f/t);

				q->setBlendType(BLEND_ADD);
				addChild(q, PM_POINTER);



			}
			else
			{
				leftOver = 0;
			}
		}



	}

};

class GemMover : public Quad
{
public:
	GemMover(GemData *gemData) : Quad(), gemData(gemData)
	{
		followCamera = 1;
		blinkTimer = 0;
		alphaMod = 0.66f;
		clickDelay = 0;
		lmbdown = false;
		isEnteringText = false;

		text = new TTFText(&dsq->fontArialSmall);
		text->offset = Vector(0, 4);
		text->setAlign(ALIGN_CENTER);

		textBG = new RoundedRect(100, 25, 10);
		textBG->alpha = 0;
		textBG->followCamera = 1;
		game->addRenderObject(textBG, LR_WORLDMAPHUD);

		textBG->addChild(text, PM_POINTER);

		refresh();
	}

	void refresh()
	{
		setTexture("gems/" + gemData->name);

		WorldMapTileContainer *tc = dynamic_cast<WorldMapTileContainer*>(parent);
		position = tc ? tc->worldPosToTilePos(gemData->pos) : gemData->pos;
		text->setText(gemData->userString);
		textBG->setWidthHeight(text->getActualWidth() + 20, 25, 10);
	}

	void destroy()
	{
		if (textBG)
		{
			textBG->safeKill();
			textBG = 0;
		}
		Quad::destroy();
	}

	GemData *getGemData() { return gemData; }

	inline bool canMove() const { return gemData->canMove; }
	inline bool canRemove() const { return canMove() && !isEnteringText; }
protected:

	float blinkTimer;
	float clickDelay;
	bool lmbdown;
	bool isEnteringText;
	GemData *gemData;

	TTFText *text;
	RoundedRect *textBG;
	void onUpdate(float dt)
	{
		Quad::onUpdate(dt);

		// Make sure ALL gems always have the same size no matter the container's scale.
		// This is a bit nasty because some gems have the world map surface as parent (where gemData->global),
		// while the map-local gems have their map tile as parent.
		// So for scaling them properly, make sure the tile scale factor cancels out while normalizing the scale.

		float invparent = gemData->global ? 1.0f : 1.0f / parent->scale.x; // factor used to cancel out parent tile scaling, if the parent is a tile.
		float sz = parent->getRealScale().x;

		sz *= invparent;

		if (sz < zoomMin)
			sz = zoomMin;
		if (sz > zoomMax)
			sz = zoomMax;

		sz = sz < 1.0f ? 1.0f : 1.0f / sz;

		sz *= invparent;

		scale.x = sz;
		scale.y = sz;


		Vector wp = getWorldPosition();

		if (gemData->blink)
		{
			blinkTimer += dt;
			if (blinkTimer > blinkPeriod)
			{
				if (alphaMod == 0)
					alphaMod = 1;
				else
					alphaMod = 0;

				blinkTimer = 0;
			}
		}

		clickDelay = std::max(clickDelay - dt, 0.0f);

		if (canMove())
		{
			if (mover == 0)
			{
				if (core->mouse.buttons.left && (core->mouse.position - wp).isLength2DIn(GEM_GRAB))
				{
					core->sound->playSfx("Gem-Move");
					mover = this;
					if(!lmbdown)
					{
						lmbdown = true;
						if(clickDelay > 0 && !isEnteringText)
						{
							// double click!
							isEnteringText = true;
							std::string useString = dsq->getUserInputString(stringbank.get(860), gemData->userString, true);
							if (!useString.empty())
							{
								gemData->userString = useString;
								refresh();
							}
							isEnteringText = false;
						}
						else
							clickDelay = 0.5f;

					}
				}
			}
			else if (mover == this)
			{

				position += (core->mouse.position - wp)/parent->scale.x;
				if (!core->mouse.buttons.left)
				{
					mover = 0;
					core->sound->playSfx("Gem-Place");
					gemData->pos = position;
					lmbdown = false;
				}
			}
		}


		if (textBG)
		{
			textBG->position = getWorldPosition() + Vector(0, -20);

			if ((core->mouse.position - wp).isLength2DIn(GEM_GRAB))
			{
				if (!gemData->userString.empty())
					textBG->show();
			}
			else
			{
				if (textBG->alpha == 1)
					textBG->hide();
			}
		}
	}
};

class HintGemQuad : public Quad
{
public:
	HintGemQuad(const std::string& gemtex, const Vector& pos)
		: Quad("gems/" + gemtex, pos), gemTex(gemtex)
	{
	}
	virtual ~HintGemQuad() {}
	inline const std::string& getGemTex() const { return gemTex; }
	const std::string gemTex;
};

typedef std::vector <GemMover*> GemMovers;
GemMovers gemMovers;

typedef std::vector<BeaconRender*> BeaconRenders;
BeaconRenders beaconRenders;

void WorldMapRender::setProperTileColor(WorldMapTileContainer& wt)
{
	const WorldMapTile& t = wt.tile;
	float amod;
	if(selectedTile != &wt)
	{
		amod = (t.revealed || t.prerevealed) ? 0.5f : 0.0f;

		if (selectedTile && t.layer != selectedTile->tile.layer)
			amod *= 0.5f;

		wt.q.color = Vector(0.7f, 0.8f, 1);
	}
	else
	{
		wt.q.color = Vector(1,1,1);
		amod = 1;
	}

	if(!t.revealed)
		amod *= float(WORLDMAP_REVEALED_BUT_UNEXPLORED_ALPHA) / float(0xff);

	wt.q.alphaMod = amod;
}

static HintGemQuad *addHintGem(const char *tex)
{
	HintGemQuad *q = new HintGemQuad(tex, Vector(0,0));
	q->followCamera = 1;
	q->alpha = 0;
	game->addRenderObject(q, LR_WORLDMAPHUD);
	return q;
}

WorldMapRender::WorldMapRender(WorldMap& wm) : RenderObject(), ActionMapper()
	, worldmap(wm)
{
	inputDelay = 0;
	editorActive=false;
	mb = false;
	playerTile = NULL;
	selectedTile = NULL;
	areaLabel = 0;

	on = false;
	wasEditorSaveDown = false;
	alpha = 0;

	scale = Vector(1, 1);
	followCamera = 1;
	cull = false;
	position = Vector(400,300);

	lastMousePosition = core->mouse.position;

	shareAlphaWithChildren = 1;

	bindInput();

	tophud = new Quad("gui/worldmap-ui", Vector(400,64));
	tophud->followCamera = 1;
	tophud->alpha = 0;
	game->addRenderObject(tophud, LR_WORLDMAPHUD);


	float aly = 26, aly2 = 18;
	float sz = 0.6f;

	//hover
	areaLabel = new BitmapText(dsq->smallFont);
	areaLabel->scale = Vector(sz,sz);

	areaLabel->setAlign(ALIGN_CENTER);
	areaLabel->followCamera = 1;
	areaLabel->position = Vector(150,aly);
	game->addRenderObject(areaLabel, LR_WORLDMAPHUD);
	areaLabel->alpha = 0;

	//in
	areaLabel2 = new BitmapText(dsq->smallFont);

	areaLabel2->scale = Vector(sz,sz);
	areaLabel2->followCamera = 1;
	areaLabel2->setAlign(ALIGN_CENTER);
	areaLabel2->position = Vector(400,aly2);
	game->addRenderObject(areaLabel2, LR_WORLDMAPHUD);
	areaLabel2->alpha = 0;

	//select
	areaLabel3 = new BitmapText(dsq->smallFont);
	areaLabel3->scale = Vector(sz,sz);

	areaLabel3->followCamera = 1;
	areaLabel3->setAlign(ALIGN_CENTER);
	areaLabel3->position = Vector(650, aly);
	areaLabel3->alpha = 0;
	game->addRenderObject(areaLabel3, LR_WORLDMAPHUD);

	bindInput();

	underlay = new Gradient;

	underlay->makeVertical(Vector(0.25,0.25,0.5), Vector(0,0,0.25));
	underlay->position = Vector(400,300);
	underlay->autoWidth = AUTO_VIRTUALWIDTH;
	underlay->autoHeight = AUTO_VIRTUALHEIGHT;
	underlay->followCamera = 1;
	underlay->alpha = 0;
	game->addRenderObject(underlay, LR_HUDUNDERLAY);

	addHintQuads[0] = addHintGem("pyramidyellow");
	addHintQuads[1] = addHintGem("pyramidpurple");
	addHintQuads[2] = addHintGem("pyramidgreen");
	addHintQuads[3] = addHintGem("pyramidred");
	addHintQuads[4] = addHintGem("pyramidblue");

	helpButton = new AquariaMenuItem;
	helpButton->event.setActionMapperCallback(this, ACTION_TOGGLEHELPSCREEN, 0);
	helpButton->useQuad("gui/icon-help");
	helpButton->useGlow("particles/glow", 40, 40);
	helpButton->useSound("Click");
	helpButton->alpha = 0;

	game->addRenderObject(helpButton, LR_WORLDMAPHUD);
}

WorldMapRender::~WorldMapRender()
{
}

void WorldMapRender::init()
{
	debugLog("WorldMapRender: init...");
	for(size_t i = 0; i < tiles.size(); ++i)
	{
		tiles[i]->destroy();
		delete tiles[i];
	}
	tiles.clear();

	const size_t num = worldmap.worldMapTiles.size();

	std::vector<std::string> textodo(num);
	std::vector<Texture*> texs(num, NULL);
	textodo.reserve(num);
	tiles.reserve(num);
	for (size_t i = 0; i < num; i++)
	{
		WorldMapTile& tile = worldmap.worldMapTiles[i];
		textodo[i] = "gui/worldmap/" + tile.name;
		WorldMapTileContainer *tc = new WorldMapTileContainer(tile);
		tiles.push_back(tc);
		addChild(tc, PM_POINTER);
	}

	if(num)
		dsq->texmgr.loadBatch(&texs[0], &textodo[0], num);

	for (size_t i = 0; i < num; i++)
	{
		WorldMapTileContainer& tc = *tiles[i];
		WorldMapTile& t = tc.tile;

		tc.position = t.gridPos;
		t.originalTex = texs[i];
		tc.setTexturePointer(texs[i]); // to init width, height

		tc.refreshMapTile(); // may or may not set texture to the generated one

		setProperTileColor(tc);
	}
	debugLog("WorldMapRender: init done");
}

void WorldMapRender::onToggleHelpScreen()
{
	game->toggleHelpScreen();
}

void WorldMapRender::bindInput()
{
	clearActions();
	clearCreatedEvents();

	addAction(ACTION_TOGGLEWORLDMAPEDITOR, KEY_TAB, -1);

	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
		const ActionSet& as = dsq->user.control.actionSets[i];
		int sourceID = (int)i;

		as.importAction(this, "PrimaryAction",		ACTION_PRIMARY, sourceID);
		as.importAction(this, "SecondaryAction",	ACTION_SECONDARY, sourceID);

		as.importAction(this, "SwimLeft",			ACTION_SWIMLEFT, sourceID);
		as.importAction(this, "SwimRight",			ACTION_SWIMRIGHT, sourceID);
		as.importAction(this, "SwimUp",			ACTION_SWIMUP, sourceID);
		as.importAction(this, "SwimDown",			ACTION_SWIMDOWN, sourceID);
	}
}

bool WorldMapRender::isCursorOffHud()
{
	if (helpButton->isCursorInMenuItem())
	{
		return false;
	}
	return true;
}

void WorldMapRender::updateAllTilesColor()
{
	for (size_t i = 0; i < tiles.size(); i++)
		setProperTileColor(*tiles[i]);
}

WorldMapTileContainer* WorldMapRender::getTileByName(const char* name) const
{
	for(size_t i = 0; i < tiles.size(); ++i)
		if(!nocasecmp(tiles[i]->tile.name.c_str(), name))
			return tiles[i];
	return NULL;
}

WorldMapTileContainer *WorldMapRender::setCurrentMap(const char* mapname)
{
	return ((playerTile = getTileByName(mapname)));
}

void WorldMapRender::onUpdate(float dt)
{
	if (AquariaGuiElement::currentGuiInputLevel > 0) return;

	RenderObject::onUpdate(dt);
	ActionMapper::onUpdate(dt);

	areaLabel->alpha.x = this->alpha.x;
	areaLabel2->alpha.x = this->alpha.x;
	areaLabel3->alpha.x = this->alpha.x;
	tophud->alpha.x = this->alpha.x;

	const int offset = 26;
	helpButton->position = Vector(core->getVirtualWidth()-core->getVirtualOffX()-offset, offset);

	for(size_t i = 0; i < tiles.size(); ++i)
		tiles[i]->q.alpha.x = alpha.x;

	if (isOn())
	{
		// minimap marker gem placers
		{
			const float mmWidth  = game->miniMapRender->getMiniMapWidth();
			const float mmHeight = game->miniMapRender->getMiniMapHeight();
			const Vector mmpos = game->miniMapRender->position;

			float yoffs = -mmHeight/2-32;
			addHintQuads[0]->position = mmpos + Vector(-mmWidth*3/22, yoffs);
			addHintQuads[1]->position = mmpos + Vector(mmWidth*3/22, yoffs);

			yoffs = -mmHeight/2-12;
			addHintQuads[2]->position = mmpos + Vector(-mmWidth*6/22, yoffs);
			addHintQuads[3]->position = mmpos + Vector(0, yoffs);
			addHintQuads[4]->position = mmpos + Vector(mmWidth*6/22, yoffs);

			for(size_t i = 0; i < Countof(addHintQuads); ++i)
			{
				float s = addHintQuads[i]->isCoordinateInRadius(core->mouse.position, 10) ? 1.33f : 1.0f;
				addHintQuads[i]->scale.interpolateTo(Vector(s, s), 0.1f);
			}
		}

		if(!selectedTile)
			selectedTile = playerTile;

		if(playerTile && !gemMovers.empty())
		{
			Vector playerPos = game->avatar->getPositionForMap();
			Vector playerTilePos = playerTile->worldPosToTilePos(playerPos);
			for (GemMovers::iterator i = gemMovers.begin(); i != gemMovers.end(); i++)
			{
				GemMover *gm = *i;
				GemData *g = gm->getGemData();
				if(g->isPlayer) // Don't want to call refresh() every frame, so just update position here
				{
					assert(!g->global);
					g->pos = playerPos;
					gm->position = playerTilePos;
				}
			}
		}

		if (inputDelay > 0)
		{
			inputDelay -= dt;
			if (inputDelay < 0) inputDelay = 0;
		}
		else
		{
			WorldMapTileContainer *hoverTile = 0;
			float sd=-1,d=0;
			for (size_t i = 0; i < tiles.size(); i++)
			{
				WorldMapTileContainer *tc = tiles[i];
				if (tc != selectedTile)
				{
					if (tc->tile.revealed || tc->tile.prerevealed)
					{
						if(tc->q.isCoordinateInsideWorld(core->mouse.position))
						{
							d = (tc->getWorldPosition() - core->mouse.position).getSquaredLength2D();

							if (sd < 0 || d < sd)
							{
								sd = d;
								hoverTile = tc;
							}
						}
					}
				}
			}

			if (!editorActive)
			{
				if (hoverTile)
				{
					areaLabel->setText(stringbank.get(hoverTile->tile.stringIndex));

					if (selectedTile && !mover && !dsq->isNested() && isCursorOffHud())
					{
						if (!core->mouse.buttons.left && mb)
						{
							if (selectedTile != hoverTile)
							{
								debugLog("selectedTile: " + hoverTile->tile.name);

								selectedTile = hoverTile;


								dsq->clickRingEffect(hoverTile->getWorldPosition(), 0);

								dsq->sound->playSfx("bubble-lid");
								dsq->sound->playSfx("menuselect");

								updateAllTilesColor();
							}

							mb = false;
						}
						else if (core->mouse.buttons.left && !mb)
						{
							mb = true;
						}
					}
					else
					{
						mb = false;
					}
				}
				else
				{
					areaLabel->setText("");
				}

				if (selectedTile)
				{
					areaLabel3->setText(stringbank.get(selectedTile->tile.stringIndex));
				}
			}
		}

		if (!core->mouse.buttons.left && mb)
			mb = false;

		if (core->mouse.buttons.middle || core->mouse.buttons.right)
		{
			// FIXME: For some reason, not all mouse movement events reach
			// this handler (at least under Linux/SDL), so when moving the
			// mouse quickly, the world map scrolling tends to lag behind.
			// We work around this by keeping our own "last position" vector
			// and calculating the mouse movement from that.  --achurch
			Vector mouseChange = core->mouse.position - lastMousePosition;
			internalOffset += mouseChange / scale.x;
		}

		if(!editorActive)
		{
			if(!dsq->isNested()) // Don't move the worldmap around on key presses if a text input box is open and we're actually just entering text
			{
				float scrollSpeed = 2.0f;
				float amt = (400*dt)/scale.x;
				if (isActing(ACTION_SWIMLEFT, -1))
				{
					internalOffset += Vector(amt, 0);
				}
				if (isActing(ACTION_SWIMRIGHT, -1))
				{
					internalOffset += Vector(-amt, 0);
				}
				if (isActing(ACTION_SWIMDOWN, -1))
				{
					if (core->getShiftState())
					{
						scale.stop();
						scale -= Vector(scrollSpeed*dt, scrollSpeed*dt);
					}
					else
					{
						internalOffset += Vector(0, -amt);
					}
				}
				if (isActing(ACTION_SWIMUP, -1))
				{
					if (core->getShiftState())
					{
						scale.stop();
						scale += Vector(scrollSpeed*dt, scrollSpeed*dt);
					}
					else
					{
						internalOffset += Vector(0, amt);
					}
				}
			}

			if (core->joystickEnabled)
			{
				if (isActing(ACTION_SECONDARY, -1))
				{
					Vector jpos;
					for(size_t i = 0; i < core->getNumJoysticks(); ++i)
						if(Joystick *j = core->getJoystick(i))
							if(j && j->isEnabled())
								if(fabsf(j->position.y) > 0.6f)
								{
									jpos = j->position;
									break;
								}

					if (jpos.y >= 0.6f)
						scale.interpolateTo(scale / 1.2f, 0.1f);
					else if (jpos.y <= -0.6f)
						scale.interpolateTo(scale * 1.2f, 0.1f);
				}
				else
				{
					Vector jpos;
					for(size_t i = 0; i < core->getNumJoysticks(); ++i)
						if(Joystick *j = core->getJoystick(i))
							if(j && j->isEnabled())
								if(!j->position.isZero())
								{
									jpos = j->position;
									break;
								}
					// The negative multiplier is deliberate -- it makes the
					// map scroll as though the joystick was controlling the
					// cursor (which is fixed in the center of the screen).
					internalOffset += jpos * (-400*dt / scale.x);
				}
			}
		}

		float scrollAmount = 0.2f;

		if (core->mouse.scrollWheelChange)
		{
			Vector target = scale;
			int changeLeft = core->mouse.scrollWheelChange;
			for (; changeLeft > 0; changeLeft--)
				target *= 1 + scrollAmount;
			for (; changeLeft < 0; changeLeft++)
				target /= 1 + scrollAmount;
			scale.interpolateTo(target, 0.1f);
		}

		if (scale.x < zoomMin)
		{
			scale.stop();
			scale.x = scale.y = zoomMin;
		}
		if (scale.x > zoomMax)
		{
			scale.stop();
			scale.x = scale.y = zoomMax;
		}

		if (-internalOffset.x < xMin - 300/scale.x)
			internalOffset.x = -(xMin - 300/scale.x);
		else if (-internalOffset.x > xMax + 300/scale.x)
			internalOffset.x = -(xMax + 300/scale.x);
		if (-internalOffset.y < yMin - 225/scale.x)
			internalOffset.y = -(yMin - 225/scale.x);
		else if (-internalOffset.y > yMax + 150/scale.x)
			internalOffset.y = -(yMax + 150/scale.x);

		if (dsq->canOpenEditor())
		{
			if (editorActive)
			{
				if (selectedTile)
				{
					WorldMapTile& t = selectedTile->tile;
					float amt = dt*4;
					float a2 = dt*0.1f;

					if (core->getShiftState())
					{
						if (core->getCtrlState())
							a2 *= 10.0f;
						if (core->getKeyState(KEY_UP))
							t.scale2 += -a2;
						if (core->getKeyState(KEY_DOWN))
							t.scale2 += a2;
					}
					else if (core->getAltState())
					{
						if (core->getCtrlState())
							a2 *= 10.0f;
						if (core->getKeyState(KEY_UP))
							t.scale += -a2;
						if (core->getKeyState(KEY_DOWN))
							t.scale += a2;
					}
					else
					{
						if (core->getCtrlState())
						{
							amt *= 50;
						}
						if (core->getKeyState(KEY_LEFT))
							t.gridPos += Vector(-amt, 0);
						if (core->getKeyState(KEY_RIGHT))
							t.gridPos += Vector(amt, 0);
						if (core->getKeyState(KEY_UP))
							t.gridPos += Vector(0, -amt);
						if (core->getKeyState(KEY_DOWN))
							t.gridPos += Vector(0, amt);
					}

					bool isf2 = core->getKeyState(KEY_F2);
					if (isf2 && !wasEditorSaveDown)
					{
						dsq->continuity.worldMap.save();
					}
					wasEditorSaveDown = isf2;
				}
				updateEditor();
			}
		}
	}
	else
	{
		if (!dsq->isInCutscene() && game->avatar && playerTile && !game->sceneEditor.isOn())
		{
			const float screenWidth  = core->getVirtualWidth()  * core->invGlobalScale;
			const float screenHeight = core->getVirtualHeight() * core->invGlobalScale;
			Vector camera = core->cameraPos;
			camera.x += screenWidth/2;
			camera.y += screenHeight/2;
			const float visWidth  = screenWidth  * visitedFraction;
			const float visHeight = screenHeight * visitedFraction;
			Vector tl, br;
			tl.x = (camera.x - visWidth/2 ) / game->cameraMax.x;
			tl.y = (camera.y - visHeight/2) / game->cameraMax.y;
			br.x = (camera.x + visWidth/2 ) / game->cameraMax.x;
			br.y = (camera.y + visHeight/2) / game->cameraMax.y;
			const int x0 = int(tl.x * MAPVIS_SUBDIV);
			const int y0 = int(tl.y * MAPVIS_SUBDIV);
			const int x1 = int(br.x * MAPVIS_SUBDIV);
			const int y1 = int(br.y * MAPVIS_SUBDIV);
			playerTile->tile.markVisited(x0, y0, x1, y1);
		}
	}

	lastMousePosition = core->mouse.position;
}

bool WorldMapRender::isOn()
{
	return this->on;
}

GemMover *WorldMapRender::addGem(GemData *gemData)
{
	GemMover *g = new GemMover(gemData);
	WorldMapTileContainer *tc = NULL;
	if(!gemData->global && !gemData->mapName.empty())
		tc = getTileByName(gemData->mapName.c_str());

	// tile-local gems also get added as a child to their tile
	if(tc)
		tc->addGem(g);
	else
		addChild(g, PM_POINTER);
	gemMovers.push_back(g);
	g->refresh();
	return g;
}

void WorldMapRender::refreshGem(const GemData* gemData)
{
	GemMover *m = getGem(gemData);
	if(!m)
		return;

	if(!gemData->global)
	{
		WorldMapTileContainer *src = getTileWithGem(gemData);
		if(src)
		{
			if(nocasecmp(gemData->mapName, src->tile.name))
			{
				WorldMapTileContainer *dst = getTileByName(gemData->mapName.c_str());
				if(dst)
				{
					GemMover *gm = src->removeGem(gemData);
					if(gm)
						dst->addGem(gm);
				}
			}
		}
	}

	m->refresh();
}

void WorldMapRender::removeGem(GemMover *gem)
{
	dsq->continuity.removeGemData(gem->getGemData());
	for(size_t i = 0; i < gemMovers.size(); ++i)
		if(gemMovers[i] == gem)
		{
			gemMovers[i] = gemMovers.back();
			gemMovers.pop_back();
			break;
		}
	WorldMapTileContainer *tc = getTileWithGem(gem->getGemData());
	if(tc)
		tc->removeGem(gem);
	gem->safeKill();
}

void WorldMapRender::removeGem(const GemData* gemData)
{
	if(GemMover *gm = getGem(gemData))
		removeGem(gm);
}

GemMover* WorldMapRender::getGem(const GemData* gemData) const
{
	for(size_t i = 0; i < gemMovers.size(); ++i)
		if(gemMovers[i]->getGemData() == gemData)
			return gemMovers[i];
	return NULL;
}

WorldMapTileContainer* WorldMapRender::getTileWithGem(const GemData* gemData) const
{
	WorldMapTileContainer *src = NULL;
	for(size_t i = 0; i < tiles.size(); ++i)
	{
		WorldMapTileContainer *tc = tiles[i];
		if(tc->getGem(gemData))
			return tc;
	}
	return NULL;
}

void WorldMapRender::toggle(bool turnON)
{
	if (AquariaGuiElement::currentGuiInputLevel > 0) return;
	if (game->miniMapRender->isRadarHide()) return;
	if (alpha.isInterpolating()) return;

	if (dsq->mod.isActive() && !dsq->mod.hasWorldMap()) return;

	if (dsq->isNested()) return;

	if (!game->avatar) return;

	if (turnON && game->avatar->isSinging()) return;

	if (game->isInGameMenu()) return;

	if (!game->isActive()) return;

	if (turnON && game->isPaused()) return;

	if (!this->on && !game->avatar->isInputEnabled()) return;

	const SeeMapMode mapmode = game->avatar->getSeeMapMode();

	if (mapmode == SEE_MAP_NEVER
		|| (mapmode == SEE_MAP_DEFAULT && game->avatar->isInDarkness() && dsq->continuity.form != FORM_SUN))
	{
		core->sound->playSfx("denied");
		return;
	}

	mb = false;
	this->on = turnON;
	if (on)
	{
		game->togglePause(true);

		core->sound->playSfx("menu-open");

		if (playerTile)
		{
			selectedTile = playerTile;

			areaLabel2->setText(stringbank.get(playerTile->tile.stringIndex)); // FIXME: and if playerTile == NULL?

			internalOffset = -playerTile->tile.gridPos;
			if (playerTile->layer == 1)
				scale = Vector(1.5,1.5);
			else
				scale = Vector(1,1);
		}

		// Opening the map. Some tiles may need refreshing in case some new visited areas were uncovered in the meantime
		for(size_t i = 0; i < tiles.size(); ++i)
			tiles[i]->refreshMapTile();

		xMin = xMax = -internalOffset.x;
		yMin = yMax = -internalOffset.y;
		for (size_t i = 0; i < tiles.size(); ++i)
		{
			WorldMapTileContainer *tc = tiles[i];
			const WorldMapTile& t = tc->tile;
			if (t.revealed || t.prerevealed)
			{
				Quad& q = tc->q;
				const float width = q.getWidth() * q.scale.x;
				const float height = q.getHeight() * q.scale.y;
				const float tx = t.gridPos.x;
				const float ty = t.gridPos.y;
				const float w2 = width * 0.5f;
				const float h2 = height * 0.5f;
				if (xMin > tx - w2)
					xMin = tx - w2;
				if (xMax < tx + w2)
					xMax = tx + w2;
				if (yMin > ty - h2)
					yMin = ty - h2;
				if (yMax < ty + h2)
					yMax = ty + h2;
			}
		}

		updateAllTilesColor();

		alpha.interpolateTo(1, 0.2f);


		underlay->alpha.interpolateTo(WORLDMAP_UNDERLAY_ALPHA, 0.2f);
		helpButton->alpha.interpolateTo(1.0f, 0.2f);

		for(size_t i = 0; i < Countof(addHintQuads); ++i)
			addHintQuads[i]->alpha.interpolateTo(1.0f, 0.2f);

		assert(gemMovers.empty());

		// I'm not sure why this is all backwards but i'm thinking that it's because the naija gem should always be on top,
		// and since it's the very first one in the list that adds it last, ie. on top of everything else.
		for (Continuity::Gems::reverse_iterator i = dsq->continuity.gems.rbegin(); i != dsq->continuity.gems.rend(); i++)
			addGem(&(*i));

		for (Continuity::Beacons::reverse_iterator i = dsq->continuity.beacons.rbegin(); i != dsq->continuity.beacons.rend(); i++)
		{
			if ((*i).on)
			{
				BeaconRender *b = new BeaconRender(&(*i));


				addChild(b, PM_POINTER);
				beaconRenders.push_back(b);
			}
		}

		inputDelay = 0.5;
	}
	else if (!on)
	{
		inputDelay = 0.5;

		for (size_t i = 0; i < tiles.size(); i++)
			tiles[i]->gems.clear();

		core->sound->playSfx("Menu-Close");

		alpha.interpolateTo(0, 0.2f);

		game->togglePause(false);

		underlay->alpha.interpolateTo(0, 0.2f);
		helpButton->alpha.interpolateTo(0, 0.2f);

		for(size_t i = 0; i < Countof(addHintQuads); ++i)
			addHintQuads[i]->alpha.interpolateTo(0, 0.2f);


		for (GemMovers::iterator i = gemMovers.begin(); i != gemMovers.end(); i++)
		{

			(*i)->safeKill();
		}
		gemMovers.clear();

		for (BeaconRenders::iterator i = beaconRenders.begin(); i != beaconRenders.end(); i++)
		{

			(*i)->safeKill();
		}
		beaconRenders.clear();
	}
}

void WorldMapRender::createGemHint(const std::string &gfx)
{
	if(!playerTile)
	{
		sound->playSfx("denied");
		debugLog("WorldMapRender::createGemHint can't create gem, no player tile");
		return;
	}
	std::string useString = dsq->getUserInputString(stringbank.get(860), "", true);
	if (!useString.empty())
	{
		GemData *g = dsq->continuity.pickupGem(gfx, false);
		if(g)
		{
			g->canMove = 1;
			g->global = true;
			g->pos = playerTile->worldPosToMapPos(game->avatar->getPositionForMap());
			g->userString = useString;
			addGem(g);
		}
	}
}

void WorldMapRender::updateEditor()
{
	std::ostringstream os;
	os << "EDITING...";
	if(selectedTile)
	{
		const WorldMapTile& t = selectedTile->tile;
		os << " (" << t.name << ")" << std::endl;
		os << "x=" << t.gridPos.x << "; y=" << t.gridPos.y << std::endl;
		os << "scale=" << t.scale << "; scale2=" << t.scale2;
	}
	areaLabel->setText(os.str());
}

void WorldMapRender::action (int id, int state, int source, InputDevice device)
{
	if (isOn())
	{
		if (id == ACTION_TOGGLEHELPSCREEN && !state)
		{
			onToggleHelpScreen();
		}
		if (id == ACTION_TOGGLEWORLDMAPEDITOR && !state)
		{
			if (dsq->canOpenEditor())
			{
				editorActive = !editorActive;

				if (editorActive)
				{
					updateEditor();
				}

				for(size_t i = 0; i < tiles.size(); ++i)
					tiles[i]->q.renderBorder = editorActive;
			}
		}

		if (id == ACTION_PRIMARY && state)
		{
			for(size_t i = 0; i < Countof(addHintQuads); ++i)
				if(addHintQuads[i]->isCoordinateInRadius(core->mouse.position, 10))
					createGemHint(addHintQuads[i]->getGemTex());
		}

		if (id == ACTION_SECONDARY && !state)
		{
			if (!mover)
			{
				for (GemMovers::iterator i = gemMovers.begin(); i != gemMovers.end(); i++)
				{
					if ((*i)->canRemove() && (core->mouse.position - (*i)->getWorldPosition()).isLength2DIn(GEM_GRAB))
					{
						removeGem(*i);
						break;
					}
				}
			}
		}
	}
}




WorldMapTileContainer::WorldMapTileContainer(WorldMapTile& tile)
	: tile(tile)
{
	addChild(&q, PM_STATIC);
	q.borderAlpha = 0.7f;
	q.renderBorderColor = Vector(1, 0.5f, 0.5f);
}

WorldMapTileContainer::~WorldMapTileContainer()
{
}

void WorldMapTileContainer::refreshMapTile()
{
	Texture *usetex = tile.originalTex.content();
	if(tile.revealed)
	{
		if(tile.dirty)
		{
			tile.updateDiscoveredTex();
			tile.dirty = false; // keep it dirty when undiscovered
		}
		if(tile.generatedTex)
			usetex = tile.generatedTex.content();
	}
	q.setTexturePointer(usetex); // updates width, height
}

void WorldMapTileContainer::removeGems()
{
	for(size_t i = 0; i < gems.size(); ++i)
		removeChild(gems[i]);
	gems.clear();
}

void WorldMapTileContainer::addGem(GemMover* gem)
{
	addChild(gem, PM_POINTER);
	gems.push_back(gem);
}

bool WorldMapTileContainer::removeGem(GemMover* gem)
{
	for(size_t i = 0; i < gems.size(); ++i)
		if(gems[i] == gem)
		{
			gems[i] = gems.back();
			gems.pop_back();
			removeChild(gem);
			return true;
		}
	return false;
}

GemMover *WorldMapTileContainer::removeGem(const GemData* gem)
{
	for(size_t i = 0; i < gems.size(); ++i)
	{
		GemMover *gm = gems[i];
		if(gm->getGemData() == gem)
		{
			gems[i] = gems.back();
			gems.pop_back();
			removeChild(gm);
			return gm;
		}
	}
	return NULL;
}

GemMover* WorldMapTileContainer::getGem(const GemData* gemData) const
{
	for(size_t i = 0; i < gems.size(); ++i)
		if(gems[i]->getGemData() == gemData)
			return gems[i];
	return NULL;
}

Vector WorldMapTileContainer::worldPosToTilePos(const Vector& position) const
{
	// Notes: TILE_SIZE in the world is 1 pixel in the map template png.
	// scale is intended to adjust for a texture with a lower resolution than the map template,
	// eg. if a 1024x1024 png was resized to 512x512 the scale is used to compensate;
	// and scale2 is used to scale the entirety of the map tile with gems and everything on it
	const float sizew = (float)q.texture->width;
	const float sizeh = (float)q.texture->height;
	const float invsz = 1.0f / TILE_SIZE;

	const float f = tileScaleFactor * tile.scale2; // scale2 is off the same way as scale
	const float g = f * tile.scale * 0.5f; // half the texture size
	Vector p = position * invsz * f;
	p -= Vector(sizew*g, sizeh*g);
	return p;
}

Vector WorldMapTileContainer::worldPosToMapPos(const Vector& p) const
{
	return worldPosToTilePos(p) + tile.gridPos;
}

void WorldMapTileContainer::onUpdate(float dt)
{
	position = tile.gridPos;
	scale.x =  tile.scale2;
	scale.y =  tile.scale2;

	q.scale.x = tile.scale * tileScaleFactor;
	q.scale.y = tile.scale * tileScaleFactor;
	q.renderQuad = tile.revealed || tile.prerevealed;

	RenderObject::onUpdate(dt);
}
