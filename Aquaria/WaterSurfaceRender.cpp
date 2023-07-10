
#include "WaterSurfaceRender.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "RenderBase.h"


namespace WaterSurfaceRenderStuff
{
	Vector baseColor = Vector(0.4f,0.9f,1.0f);
}

using namespace WaterSurfaceRenderStuff;

WaterSurfaceRender::WaterSurfaceRender() : Quad()
{
	color = baseColor;
	cull = false;

	alpha = 0.75;

	if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
	{
		setSegs(4, 32, 0.5f, 0.5f, -0.005f, 0, 5, 1);
	}



	qLine = new Quad("water/water-line", Vector(0,0));
	qLine->repeatTextureToFill(1);
	qLine->cull = false;
	game->addRenderObject(qLine, LR_WATERSURFACE2);

	qLine2 = 0;



	qSurface = new Quad("missingimage", Vector(0,0));

	qSurface->cull = false;
	qSurface->repeatTextureToFill(1);
	game->addRenderObject(qSurface, LR_WATERSURFACE2);

	shareColorWithChildren = 0;
	shareAlphaWithChildren = 0;
}

void WaterSurfaceRender::onUpdate(float dt)
{
	if (game->waterLevel.x > 0)
	{
		Quad::onUpdate(dt);

		qLine->alpha = qSurface->alpha = 1;



		position.x = core->screenCenter.x;
		position.y = game->waterLevel.x;

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

		offset.y -= 40*scale.y;


		qLine->position = position + offset + Vector(0,42)*scale.y;
		qLine->alphaMod = 0.5;
		qLine->setWidth(width);



		qSurface->position = position+offset;
		qSurface->scale = scale.y;

		qSurface->setWidthHeight(width, height);

		float bit = core->cameraPos.x/300.0f;

		qLine->texOff.x = bit;

		qLine->refreshRepeatTextureToFill();



		if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
		{
			qSurface->alphaMod = 0.5;
		}
	}
	else
	{
		qLine->alpha = qSurface->alpha = 0;

		if (qLine2)
		{
			qLine2->alpha = 0;
		}
	}

	if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
	{
		const float reflectSize = 97;
		const float reflectPos = (game->waterLevel.x - core->cameraPos.y)
			+ (game->waterLevel.x - core->screenCenter.y) / 3;
		const float reflectOffset = -0.03f;
		const float coordDiv = 768;
		const float v0 = 1 + reflectOffset - (reflectPos * core->globalScale.x) / coordDiv;
		const float v1 = v0 + (reflectSize * core->globalScale.x) / coordDiv;

		upperLeftTextureCoordinates.y = v0 * core->frameBuffer.getHeightP();
		lowerRightTextureCoordinates.y = v1 * core->frameBuffer.getHeightP();

		upperLeftTextureCoordinates.x = 0;
		lowerRightTextureCoordinates.x = core->frameBuffer.getWidthP();
	}
}

void WaterSurfaceRender::render(const RenderState& rs) const
{
	if (game->waterLevel.x > 0)
		Quad::render(rs);
}

void WaterSurfaceRender::onRender(const RenderState& rs) const
{
	if (game->waterLevel == 0) return;
	if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
	{
		dsq->frameBuffer.bindTexture();
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glColor4f(0.4f, 0.7f, 0.8f, 0.2f);
	}

	Quad::onRender(rs);

	glBindTexture(GL_TEXTURE_2D, 0);

	RenderObject::lastTextureApplied = 0;
}
