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
#include "GridRender.h"
#include "Game.h"
#include "Avatar.h"

namespace MiniMapRenderSpace
{
	typedef std::vector<Quad*> Buttons;
	Buttons buttons;

	const int BUTTON_RADIUS = 15;

	// Total minimap size in virtual pixels
	const float miniMapSize = 200;
	// View area radius in virtual pixels
	const float miniMapRadius = 80;
	// Minimap scale (actual distance / displayed distance)
	const float miniMapScale = 40;
	// View area radius in world tiles
	const float miniMapTileRadius = miniMapRadius * miniMapScale / TILE_SIZE;
	// 1/2 size (width/height) of minimap GUI
	const float miniMapGuiSize = miniMapRadius * 1.5f;
	// Base radius of texture (texWaterBit) used to indicate open areas
	const float waterBitSize = 10;
	// Distance in tiles between adjacent water bits
	const int tileStep = 12;
	// Base size of warp/save icons
	const float iconBaseSize = 14;
	// Additional radius added (or subtracted) by "throb" effect
	const float iconThrobSize = 6;
	// Size of cooking icon (fixed)
	const float iconCookSize = 16;
	// Maximum offset of warp/save/cooking icons from center of minimap
	const float iconMaxOffset = miniMapRadius * miniMapScale * (7.0f/8.0f);
	// Distance at which the icon decreases to minimum size
	const float iconMaxDistance = iconMaxOffset * 3;
	// Scale of the icon at minimum size
	const float iconMinScale = 0.6;
	// Radius of the health bar circle
	const int healthBarRadius = miniMapRadius + 4;
	// Number of steps around health bar at which to draw bits
	const int healthSteps = 64;
	// 1/2 size (width/height) used for drawing health bar bits
	const int healthBitSizeLarge = 32;
	const int healthBitSizeSmall = 10;
	// 1/2 size (width/height) used for drawing the maximum health marker
	const int healthMarkerSize = 20;


	Texture *texCook = 0;
	Texture *texWaterBit = 0;
	Texture *texMinimapBtm = 0;
	Texture *texMinimapTop = 0;
	Texture *texRipple = 0;
	Texture *texNaija = 0;
	Texture *texHealthBar = 0;
	Texture *texMarker = 0;

	float waterSin = 0;

	int jumpOff = 0;
	float jumpTimer = 0.5;
	const float jumpTime = 1.5;
	float incr = 0;

	int *heightLookup;
	const int heightLookupLimit = miniMapTileRadius + tileStep;
	float *bitSizeLookup;
	const int bitSizeLookupPeriod = 256;
	float *healthLookupAngle, *healthLookupX, *healthLookupY;
}

using namespace MiniMapRenderSpace;

MiniMapRender::MiniMapRender() : RenderObject()
{
	toggleOn = 1;

	radarHide = false;

	doubleClickDelay = 0;
	mouseDown = false;
	_isCursorIn = false;
	lastCursorIn = false;
	followCamera = 1;
	doRender = true;
	float shade = 0.75;
	color = Vector(shade, shade, shade);
	cull = false;
	lightLevel = 1.0;

	texCook				= core->addTexture("GUI/ICON-FOOD");
	texWaterBit			= core->addTexture("GUI/MINIMAP/WATERBIT");
	texMinimapBtm		= core->addTexture("GUI/MINIMAP/BTM");
	texMinimapTop		= core->addTexture("GUI/MINIMAP/TOP");
	texRipple			= core->addTexture("GUI/MINIMAP/RIPPLE");
	texNaija			= core->addTexture("GEMS/NAIJA-TOKEN");
	texHealthBar		= core->addTexture("PARTICLES/glow-masked"); 
	texMarker			= core->addTexture("gui/minimap/marker");

	buttons.clear();

	Quad *q = 0;

	q = new Quad();
	q->setTexture("gui/open-menu");
	q->scale = Vector(1.5, 1.5);
	buttons.push_back(q);

	q->position = Vector(miniMapRadius, miniMapRadius);

	addChild(q, PM_POINTER, RBP_OFF);

	heightLookup = new int[heightLookupLimit];
	for (int i = 0; i < heightLookupLimit; i++)
	{
		if (i < miniMapTileRadius)
		{
			const float heightFrac = cosf(float(i) / miniMapTileRadius * (PI/2));
			heightLookup[i] = int(ceilf(miniMapTileRadius * heightFrac));
		}
		else
		{
			heightLookup[i] = 0;
		}
	}

	bitSizeLookup = new float[bitSizeLookupPeriod];
	for (int i = 0; i < bitSizeLookupPeriod; i++)
		bitSizeLookup[i] = (1+fabsf(sinf((i*(2*PI)) / bitSizeLookupPeriod))) * waterBitSize;

	healthLookupAngle = new float[healthSteps+1];
	healthLookupX = new float[healthSteps+1];
	healthLookupY = new float[healthSteps+1];
	for (int i = 0; i <= healthSteps; i++)
	{
		const float angle = -PI + ((float(i)/healthSteps) * (2*PI));
		healthLookupAngle[i] = angle;
		healthLookupX[i] = cosf(angle)*healthBarRadius+2;
		healthLookupY[i] = -sinf(angle)*healthBarRadius;
	}
}

void MiniMapRender::destroy()
{
	RenderObject::destroy();

	UNREFTEX(texCook);
	UNREFTEX(texWaterBit);
	UNREFTEX(texMinimapBtm);
	UNREFTEX(texMinimapTop);
	UNREFTEX(texRipple);
	UNREFTEX(texNaija);
	UNREFTEX(texHealthBar);
	UNREFTEX(texMarker);

	delete[] heightLookup;
	heightLookup = 0;
	delete[] bitSizeLookup;
	bitSizeLookup = 0;
	delete[] healthLookupAngle;
	healthLookupAngle = 0;
	delete[] healthLookupX;
	healthLookupX = 0;
	delete[] healthLookupY;
	healthLookupY = 0;
}

bool MiniMapRender::isCursorIn()
{
	return _isCursorIn || lastCursorIn;
}

void MiniMapRender::slide(int slide)
{
	switch(slide)
	{
	case 0:
		offset.interpolateTo(Vector(0, 0), 0.28, 0, 0, 1);
	break;
	case 1:
		offset.interpolateTo(Vector(0, getMiniMapHeight()+5-600), 0.28, 0, 0, 1);
	break;
	}
}



bool MiniMapRender::isCursorInButtons()
{
	for (Buttons::iterator i = buttons.begin(); i != buttons.end(); i++)
	{
		if ((core->mouse.position - (*i)->getWorldPosition()).isLength2DIn(BUTTON_RADIUS))
		{
			return true;
		}
	}

	return ((core->mouse.position - position).isLength2DIn(50));
}

void MiniMapRender::clickEffect(int type)
{
	dsq->clickRingEffect(getWorldPosition(), type);
}

void MiniMapRender::toggle(int t)
{
	toggleOn = t;
}

float MiniMapRender::getMiniMapWidth() const
{
    return scale.x * miniMapSize;
}

float MiniMapRender::getMiniMapHeight() const
{
    return scale.y * miniMapSize;
}

void MiniMapRender::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);	

	position.x = core->getVirtualWidth() - core->getVirtualOffX() - getMiniMapWidth()/2;
	position.y = core->getVirtualHeight() - getMiniMapHeight()/2;
	position.z = 2.9;

	waterSin += dt * (bitSizeLookupPeriod / (2*PI));
	waterSin = fmodf(waterSin, bitSizeLookupPeriod);

	if (doubleClickDelay > 0)
	{
		doubleClickDelay -= dt;
	}

	radarHide = false;

	if (dsq->darkLayer.isUsed() && dsq->game->avatar)
	{
		if (dsq->continuity.form != FORM_SUN && dsq->game->avatar->isInDarkness())
		{
			radarHide = true;
		}
		else
		{
			for (Path *p = dsq->game->getFirstPathOfType(PATH_RADARHIDE); p; p = p->nextOfType)
			{
				if (p->isCoordinateInside(dsq->game->avatar->position))
				{
					radarHide = true;
					break;
				}
			}
		}
		float t = dt*2;
		if (radarHide)
		{
			lightLevel -= t;
			if (lightLevel < 0)
				lightLevel = 0;
		}
		else
		{
			lightLevel += t;
			if (lightLevel > 1)
				lightLevel = 1;
		}

	}
	else
	{
		lightLevel = 1;
	}

	if (dsq->game->avatar && dsq->game->avatar->isInputEnabled())
	{
		float v = dsq->game->avatar->health/5.0f;
		if (v < 0)
			v = 0;
		if (!lerp.isInterpolating() && lerp.x != v)
			lerp.interpolateTo(v, 0.1);
		lerp.update(dt);


		jumpTimer += dt*0.5f;
		if (jumpTimer > jumpTime)
		{
			jumpTimer = 0.5;
		}
		incr += dt*2;
		if (incr > PI)
			incr -= PI;
	}

	_isCursorIn = false;
	if (alpha.x == 1)
	{		
		if (!dsq->game->isInGameMenu() && (!dsq->game->isPaused() || (dsq->game->isPaused() && dsq->game->worldMapRender->isOn())))
		{
			if (isCursorInButtons())
			{
				if (!core->mouse.buttons.left || mouseDown)
					_isCursorIn = true;
			}

			if (_isCursorIn || lastCursorIn)
			{

				if (core->mouse.buttons.left && !mouseDown)
				{
					mouseDown = true;
				}
				else if (!core->mouse.buttons.left && mouseDown)
				{
					mouseDown = false;

					bool btn=false;

					if (!dsq->game->worldMapRender->isOn())
					{
						for (int i = 0; i < buttons.size(); i++)
						{
							if ((buttons[i]->getWorldPosition() - core->mouse.position).isLength2DIn(BUTTON_RADIUS))
							{
								switch(i)
								{
								case 0:
								{
									doubleClickDelay = 0;
									if (!core->isStateJumpPending())
										dsq->game->showInGameMenu();
									btn = true;
								}
								break;
								}
							}
							if (btn) break;
						}
					}

					if (!btn && !dsq->mod.isActive() && !radarHide)
					{
						if (dsq->game->worldMapRender->isOn())
						{
							dsq->game->worldMapRender->toggle(false);
							clickEffect(1);
						}
						else
						{
							if (doubleClickDelay > 0 && !core->isStateJumpPending())
							{
								
								if (dsq->continuity.gems.empty())
									dsq->continuity.pickupGem("Naija-Token");

								dsq->game->worldMapRender->toggle(true);

								clickEffect(0);

								doubleClickDelay = 0;
							}
							else
							{
								doubleClickDelay = DOUBLE_CLICK_DELAY;

								clickEffect(0);
							}
						}
					}
				}

				if (isCursorInButtons())
				{
					if (mouseDown)
					{
						_isCursorIn = true;
					}
				}
			}
			else
			{
				mouseDown = false;
			}
			lastCursorIn = _isCursorIn;
		}
	}

	core->getRenderObjectLayer(LR_MINIMAP)->visible =
		toggleOn && dsq->game->avatar && dsq->game->avatar->getState() != Entity::STATE_TITLE && !(dsq->disableMiniMapOnNoInput && !dsq->game->avatar->isInputEnabled());
}

void MiniMapRender::onRender()
{
#ifdef BBGE_BUILD_OPENGL

	glBindTexture(GL_TEXTURE_2D, 0);
	RenderObject::lastTextureApplied = 0;
	const float alphaValue = alpha.x;

	const TileVector centerTile(dsq->game->avatar->position);

	if (alphaValue > 0)
	{
		texMinimapBtm->apply();

		glBegin(GL_QUADS);
			glColor4f(lightLevel, lightLevel, lightLevel, 1);
			glTexCoord2f(0, 1);
			glVertex2f(-miniMapGuiSize, miniMapGuiSize);
			glTexCoord2f(1, 1);
			glVertex2f(miniMapGuiSize, miniMapGuiSize);
			glTexCoord2f(1, 0);
			glVertex2f(miniMapGuiSize, -miniMapGuiSize);
			glTexCoord2f(0, 0);
			glVertex2f(-miniMapGuiSize, -miniMapGuiSize);
		glEnd();

		texMinimapBtm->unbind();


		if (lightLevel > 0)
		{
			texWaterBit->apply();

			glBlendFunc(GL_SRC_ALPHA,GL_ONE);
			glColor4f(0.1, 0.2, 0.9, 0.4f*lightLevel);
			bool curColorIsWater = true;

			const int xmin = int(ceilf(dsq->game->cameraMin.x / TILE_SIZE));
			const int ymin = int(ceilf(dsq->game->cameraMin.y / TILE_SIZE));
			const int xmax = int(floorf(dsq->game->cameraMax.x / TILE_SIZE));
			const int ymax = int(floorf(dsq->game->cameraMax.y / TILE_SIZE));

			int x1 = centerTile.x - miniMapTileRadius;
			int x2 = centerTile.x + miniMapTileRadius;
			// Round all coordinates to a multiple of tileStep, so
			// the minimap doesn't change as you scroll.
			x1 = (x1 / tileStep) * tileStep;
			x2 = ((x2 + tileStep-1) / tileStep) * tileStep;
			for (int x = x1; x <= x2; x += tileStep)
			{
				if (x < xmin) continue;
				if (x > xmax) break;

				int dx = x - centerTile.x;
				if (dx < 0)
					dx = -dx;
				const int halfTileHeight = heightLookup[dx];

				int y1 = centerTile.y - halfTileHeight;
				int y2 = centerTile.y + halfTileHeight;
				y1 = (y1 / tileStep) * tileStep;
				y2 = ((y2 + tileStep-1) / tileStep) * tileStep;
				for (int y = y1; y <= y2; y += tileStep)
				{
					if (y < ymin) continue;
					if (y > ymax) break;

					TileVector tile(x, y);
					if (!dsq->game->getGrid(tile))
					{
						const Vector tilePos(tile.worldVector());
						if (tilePos.y < dsq->game->waterLevel.x)
						{
							if (curColorIsWater)
							{
								glColor4f(0.1, 0.2, 0.5, 0.2f*lightLevel);
								curColorIsWater = false;
							}
						}
						else
						{
							if (!curColorIsWater)
							{
								glColor4f(0.1, 0.2, 0.9, 0.4f*lightLevel);
								curColorIsWater = true;
							}
						}

						const Vector miniMapPos = Vector(tilePos - dsq->game->avatar->position) * (1.0f / miniMapScale);

						glTranslatef(miniMapPos.x, miniMapPos.y, 0);

						const float indexMult = bitSizeLookupPeriod / (2*PI);
						const float v = waterSin
							+ (tilePos.x + tilePos.y*miniMapTileRadius) * (indexMult/1000)
							+ sqr(tilePos.x+tilePos.y) * (indexMult/100000);
						const unsigned int sizeIndex = (unsigned int)(v) % bitSizeLookupPeriod;
						const float bitSize = bitSizeLookup[sizeIndex];

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-bitSize, bitSize);
							glTexCoord2f(1, 1);
							glVertex2f(bitSize, bitSize);
							glTexCoord2f(1, 0);
							glVertex2f(bitSize, -bitSize);
							glTexCoord2f(0, 0);
							glVertex2f(-bitSize, -bitSize);
						glEnd();

						glTranslatef(-miniMapPos.x, -miniMapPos.y, 0);
					}
				}
			}
			texWaterBit->unbind();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, 0);

		}
	}

	if (!radarHide)
	{
		const float factor = sinf(game->getTimer()*PI);
		const float iconSize = iconBaseSize + factor*iconThrobSize;
		texRipple->apply();
		// FIXME: use getFirstPathOfType?
		for (int i = 0; i < dsq->game->getNumPaths(); i++)
		{
			Path *p = dsq->game->getPath(i);
			if (!p->nodes.empty() && (p->pathType==PATH_COOK || p->pathType==PATH_SAVEPOINT || p->pathType==PATH_WARP))
			{
				bool render = true;
				Path *p2 = dsq->game->getNearestPath(p->nodes[0].position, PATH_RADARHIDE);
				if (p2 && p2->isCoordinateInside(p->nodes[0].position))
				{
					if (!p2->isCoordinateInside(dsq->game->avatar->position))
					{
						render = false;
					}
				}

				if (render)
				{
					Vector pt(p->nodes[0].position);
					Vector d = pt - dsq->game->avatar->position;
					const float len = d.getLength2D();
					float iconScale;
					if (len < iconMaxOffset)
					{
						iconScale = 1;
					}
					else
					{
						d *= iconMaxOffset / len;
						float k;
						if (len < iconMaxDistance)
							k = ((iconMaxDistance - len) / (iconMaxDistance - iconMaxOffset));
						else
							k = 0;
						iconScale = iconMinScale + k*(1-iconMinScale);
					}
					const Vector miniMapPos = Vector(d)*Vector(1.0f/miniMapScale, 1.0f/miniMapScale);

					switch(p->pathType)
					{
					case PATH_COOK:
					{
						glColor4f(1, 1, 1, 1);

						glTranslatef(miniMapPos.x, miniMapPos.y, 0);
						const float sz = iconCookSize * iconScale;

						texCook->apply();

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-sz, sz);
							glTexCoord2f(1, 1);
							glVertex2f(sz, sz);
							glTexCoord2f(1, 0);
							glVertex2f(sz, -sz);
							glTexCoord2f(0, 0);
							glVertex2f(-sz, -sz);
						glEnd();

						glTranslatef(-miniMapPos.x, -miniMapPos.y, 0);
						texRipple->apply();
						render = false;  // Skip common rendering code
					}
					break;
					case PATH_SAVEPOINT:
					{
						glColor4f(1.0, 0, 0, alphaValue*0.75f);
					}
					break;
					case PATH_WARP:
					{
						if (p->naijaHome)
						{
							glColor4f(1.0, 0.9, 0.2, alphaValue*0.75f);	
						}
						else
						{
							glColor4f(1.0, 1.0, 1.0, alphaValue*0.75f);
						}
					}
					break;
					}

					if (render)
					{
						glTranslatef(miniMapPos.x, miniMapPos.y, 0);
						const float sz = iconSize * iconScale;

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-sz, sz);
							glTexCoord2f(1, 1);
							glVertex2f(sz, sz);
							glTexCoord2f(1, 0);
							glVertex2f(sz, -sz);
							glTexCoord2f(0, 0);
							glVertex2f(-sz, -sz);
						glEnd();

						glTranslatef(-miniMapPos.x, -miniMapPos.y, 0);
					}
				}
			}
		}
		texRipple->unbind();
	}

	glColor4f(1,1,1, alphaValue);

	const int hsz = 20;
	texNaija->apply();

	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(-hsz, hsz);
		glTexCoord2f(1, 1);
		glVertex2f(hsz, hsz);
		glTexCoord2f(1, 0);
		glVertex2f(hsz, -hsz);
		glTexCoord2f(0, 0);
		glVertex2f(-hsz, -hsz);
	glEnd();

	texNaija->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	glColor4f(1,1,1,1);

	texMinimapTop->apply();
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(-miniMapGuiSize, miniMapGuiSize);
		glTexCoord2f(1, 1);
		glVertex2f(miniMapGuiSize, miniMapGuiSize);
		glTexCoord2f(1, 0);
		glVertex2f(miniMapGuiSize, -miniMapGuiSize);
		glTexCoord2f(0, 0);
		glVertex2f(-miniMapGuiSize, -miniMapGuiSize);
	glEnd();
	texMinimapTop->unbind();

	glBindTexture(GL_TEXTURE_2D, 0);


	const int curHealthSteps = int((lerp.x/2) * healthSteps);
	const int maxHealthSteps = int((dsq->game->avatar->maxHealth/10.0f) * healthSteps);

	Vector healthBarColor;
	if (lerp.x >= 1)
	{
		healthBarColor = Vector(0, 1, 0.5f);
	}
	else
	{
		healthBarColor = Vector(1-lerp.x, lerp.x*1, lerp.x*0.5f);
		healthBarColor.normalize2D();
	}

	texHealthBar->apply();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(healthBarColor.x, healthBarColor.y, healthBarColor.z, 0.6);

	glBegin(GL_QUADS);
	for (int step = 0; step <= curHealthSteps; step++)
	{
		const float x = healthLookupX[step];
		const float y = healthLookupY[step];

		glTexCoord2f(0, 1);
		glVertex2f(x-healthBitSizeSmall, y+healthBitSizeSmall);
		glTexCoord2f(1, 1);
		glVertex2f(x+healthBitSizeSmall, y+healthBitSizeSmall);
		glTexCoord2f(1, 0);
		glVertex2f(x+healthBitSizeSmall, y-healthBitSizeSmall);
		glTexCoord2f(0, 0);
		glVertex2f(x-healthBitSizeSmall, y-healthBitSizeSmall);
	}
	glEnd();


	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	int jump = 0;

	glBegin(GL_QUADS);
	for (int step = 0; step <= curHealthSteps; step++)
	{
		if (jump == 0)
		{
			const float angle = healthLookupAngle[step];
			const float x = healthLookupX[step];
			const float y = healthLookupY[step];

			glColor4f(healthBarColor.x, healthBarColor.y, healthBarColor.z, fabsf(cosf(angle-incr))*0.3f + 0.2f);

			glTexCoord2f(0, 1);
			glVertex2f(x-healthBitSizeLarge, y+healthBitSizeLarge);
			glTexCoord2f(1, 1);
			glVertex2f(x+healthBitSizeLarge, y+healthBitSizeLarge);
			glTexCoord2f(1, 0);
			glVertex2f(x+healthBitSizeLarge, y-healthBitSizeLarge);
			glTexCoord2f(0, 0);
			glVertex2f(x-healthBitSizeLarge, y-healthBitSizeLarge);
		}

		jump++;
		if (jump > 3)
			jump = 0;
	}
	glEnd();

	texHealthBar->unbind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);

	texMarker->apply();

	const float x = healthLookupX[maxHealthSteps];
	const float y = healthLookupY[maxHealthSteps];

	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(x-healthMarkerSize, y+healthMarkerSize);
		glTexCoord2f(1, 1);
		glVertex2f(x+healthMarkerSize, y+healthMarkerSize);
		glTexCoord2f(1, 0);
		glVertex2f(x+healthMarkerSize, y-healthMarkerSize);
		glTexCoord2f(0, 0);
		glVertex2f(x-healthMarkerSize, y-healthMarkerSize);
	glEnd();

	texMarker->unbind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(1,1,1,1);

	glBindTexture(GL_TEXTURE_2D, 0);

#endif
}

