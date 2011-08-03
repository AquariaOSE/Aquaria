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
#include "WaterSurfaceRender.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"

/*
{
public:
	WaterSurfaceRender();
	void render();
protected:
	void onRender();
};
*/

namespace WaterSurfaceRenderStuff
{
	Vector baseColor = Vector(0.4,0.9,1.0);
}

using namespace WaterSurfaceRenderStuff;

WaterSurfaceRender::WaterSurfaceRender() : Quad()
{
	color = baseColor;
	cull = false;
	//alpha = 0.98;
	alpha = 0.75;

	if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
	{
		setSegs(4, 32, 0.5, 0.5, -0.005, 0, 5, 1);
	}



	//addChild(qSurface);
	//qSurface->renderBeforeParent = 1;

	/*
	qLine = new Quad("water/water-line", Vector(0,50));
	qLine->repeatTextureToFill(1);
	addChild(qLine);
	*/

	qLine = new Quad("water/water-line", Vector(0,0));
	qLine->repeatTextureToFill(1);
	qLine->cull = false;
	dsq->game->addRenderObject(qLine, LR_WATERSURFACE2);

	qLine2 = 0;
	/*
	qLine2 = new Quad("water/water-line", Vector(0,0));
	qLine2->repeatTextureToFill(1);
	qLine2->cull = false;
	//qLine2->flipVertical();
	dsq->game->addRenderObject(qLine2, LR_ELEMENTS3);
	*/


	//water/water-surface
	qSurface = new Quad("missingimage", Vector(0,0));
	//qSurface->parentManagedPointer = 1;
	qSurface->cull = false;
	qSurface->repeatTextureToFill(1);
	dsq->game->addRenderObject(qSurface, LR_WATERSURFACE2);

	shareColorWithChildren = 0;
	shareAlphaWithChildren = 0;
}

void WaterSurfaceRender::render()
{	
	//if (dsq->frameBuffer.isInited())

	if (dsq->game->waterLevel.x > 0)
	{		

		qLine->alpha = qSurface->alpha = 1;

		//qSurface->alpha = 0;

		position.x = core->screenCenter.x;
		position.y = dsq->game->waterLevel.x;

		width = core->getVirtualWidth()*core->invGlobalScale;
		height = 100;
		

		float dist = (core->screenCenter.y - position.y);
		if (dist > 0)
		{
			if (dist > 400)
				scale.y = 0;
			else
				scale.y = 1.0f-(dist/400.0f);
		}

		offset.y = (height*scale.y);
		//offset.y += 10;
		offset.y -= 40*scale.y;


		qLine->position = position + offset + Vector(0,42)*scale.y;
		qLine->alphaMod = 0.5;
		qLine->setWidth(width);

		/*
		qLine2->position = position + offset;
		qLine2->alphaMod = 0.5;
		qLine2->width = this->width;
		*/

		qSurface->position = position+offset;
		qSurface->scale = scale.y;

		qSurface->setWidthHeight(width, height);

		float bit = core->cameraPos.x/300.0f;
		//qSurface->texOff.x = bit;
		qLine->texOff.x = bit;
		//qSurface->refreshRepeatTextureToFill();
		qLine->refreshRepeatTextureToFill();


		/*
		qLine2->texOff.x = bit;
		qLine2->refreshRepeatTextureToFill();
		*/

		if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
		{
			qSurface->alphaMod = 0.5;
			Quad::render();
			//qLine->alpha = qSurface->alpha = 0;
		}
		else
		{
			//color = baseColor;
			Quad::render();
			//qSurface->alphaMod = 0.6;
			//deleteGrid();
			//color = Vector(1,1,1);
		}

		

		/*
		//core->setupRenderPositionAndScale();
		glClear(GL_DEPTH_BUFFER_BIT);
		core->currentLayerPass = 0;
		dsq->game->avatar->color = Vector(1,0,0);
		dsq->game->avatar->render();
		dsq->game->avatar->color = Vector(1,1,1);
		//glBindTexture(GL_TEXTURE_2D, 0);
		*/
	}
	else
	{
		qLine->alpha = qSurface->alpha = 0;
		
		if (qLine2)
		{
			qLine2->alpha = 0;
		}
	}
}

void WaterSurfaceRender::onRender()
{
#ifdef BBGE_BUILD_OPENGL
	if (dsq->game->waterLevel == 0) return;
	if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
	{
		dsq->frameBuffer.bindTexture();
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
	{
		const float reflectSize = 97;
		const float reflectPos = (dsq->game->waterLevel.x - core->cameraPos.y)
				+ (dsq->game->waterLevel.x - core->screenCenter.y) / 3;
		const float reflectOffset = -0.03f;
		const float coordDiv = 768;
		const float v0 = 1 + reflectOffset - (reflectPos * core->globalScale.x) / coordDiv;
		const float v1 = v0 + (reflectSize * core->globalScale.x) / coordDiv;

		upperLeftTextureCoordinates.y = v0 * core->frameBuffer.getHeightP();
		lowerRightTextureCoordinates.y = v1 * core->frameBuffer.getHeightP();

		upperLeftTextureCoordinates.x = 0;
		lowerRightTextureCoordinates.x = core->frameBuffer.getWidthP();


		Quad::onRender();

		/*
		glTranslatef(0, -height - 20);
		height = 40;


		Quad::onRender();
		*/

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		/*
		upperLeftTextureCoordinates.x = 0;
		lowerRightTextureCoordinates.x = core->frameBuffer.getWidthP();
		*/
		glColor4f(0.4, 0.7, 0.8, 0.2);
		Quad::onRender();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	RenderObject::lastTextureApplied = 0;

	//dsq->game->avatar->setRenderPass(0);

	/*


	glEnable(GL_SCISSOR_TEST);
	float realSz2 = sz2*scale.x;
	float factor = float(core->getWindowWidth()) / 800.0f;
	glScissor(dsq->game->waterLevel.x*factor - realSz2 * factor, 600*factor-(position.y+realSz2)*factor, realSz2*2*factor, realSz2*2*factor);

	*/


	//glDisable(GL_SCISSOR_TEST);
#endif
}
