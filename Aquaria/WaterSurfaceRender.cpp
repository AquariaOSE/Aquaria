#include "WaterSurfaceRender.h"
#include <sstream>
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "RenderBase.h"


WaterSurfaceRender::WaterSurfaceRender() : Quad()
{
	cull = false;
	this->texcoordOverride = true;

	if (dsq->useFrameBuffer)
	{
		setSegs(4, 32, 0.5f, 0.5f, -0.005f, 0, 5, 1);
	}

	this->renderBorder = false;
	this->borderAlpha = 1;

	qLine = new Quad("water/water-line", Vector(0,0));
	qLine->repeatTextureToFill(1);
	qLine->cull = false;
	game->addRenderObject(qLine, LR_WATERSURFACE2);

	qSurface = new Quad("missingimage", Vector(0,0));
	qSurface->cull = false;
	qSurface->repeatTextureToFill(1);
	game->addRenderObject(qSurface, LR_WATERSURFACE2);

	shareColorWithChildren = 0;
	shareAlphaWithChildren = 0;
}

WaterSurfaceRender::~WaterSurfaceRender()
{
}

void WaterSurfaceRender::onEndOfLife()
{
	if(qLine)
		qLine->safeKill();
	if(qSurface)
		qSurface->safeKill();

	qLine = NULL;
	qSurface = NULL;
}

void WaterSurfaceRender::prepareRender()
{
	bool fbEffectVisible = false;
	const float vw = core->getVirtualWidth();
	const float vh = core->getVirtualHeight();
	if (game->waterLevel.x > 0)
	{
		qLine->alpha = qSurface->alpha = 1;

		position.x = core->screenCenter.x;
		position.y = game->waterLevel.x;

		width = vw * core->invGlobalScale;
		height = vh * 0.4f; // how wide the water line gets in total


		float dist = (core->screenCenter.y - position.y);
		// 0.5 makes it align right as the top of the water surface touches the top of the screen
		// < 0.5 makes the surface line appear flat before it reaches the top of the screen
		const float topAlignFactor = 0.48f;
		float maxdist = core->invGlobalScale * vh * topAlignFactor;
		if (dist > 0)
		{
			if (dist > maxdist)
				scale.y = 0;
			else
				scale.y = 1.0f-(dist/maxdist);
		}

		offset.y = height*scale.y*0.5f; // top boundary of quad must touch water surface

		qLine->position = position + offset * 2; // water line is at bottom side of the quad
		qLine->position.y -= qLine->height * 0.25f; // adjust to align with quad boundaries
		qLine->alphaMod = 0.5f;
		qLine->setWidth(width);

		qSurface->position = position+offset;
		qSurface->scale = scale.y;

		qSurface->setWidthHeight(width, height);

		float bit = core->cameraPos.x/300.0f;
		Vector texoff = qLine->getRepeatOffset();
		texoff.x = bit;
		qLine->setRepeatOffset(texoff);


		if (dsq->useFrameBuffer && dsq->frameBuffer.isInited())
		{
			qSurface->alphaMod = 0.5f;
			fbEffectVisible = this->isRectPartiallyOnScreen();
		}
	}
	else
	{
		qLine->alpha = qSurface->alpha = 0;
	}

	if (fbEffectVisible)
	{
		const Vector effpos = Vector(0, game->waterLevel.x);
		const Vector reflectSurface = core->getWindowPosition(effpos);
		// Reflect from the water surface seam to xx game units, going up
		const Vector reflectLimit = core->getWindowPosition(effpos - Vector(0, 250));

		const float coordMul = 1.0f / vh;

		const float v0 = 1 - (reflectSurface.y * coordMul);
		const float v1 = 1 - (reflectLimit.y * coordMul);

		texcoords.u1 = 0;
		texcoords.u2 = core->frameBuffer.getWidthP();

		const float hperc = core->frameBuffer.getHeightP();
		texcoords.v1 = v0 * hperc;
		texcoords.v2 = std::min(1.0f, v1) * hperc; // the min() here is not to let texcoords go out of bounds. this is st

		color = Vector(0.4f,0.9f,1.0f);
		alpha = 0.75f;
	}
	else
	{
		texcoords.setStandard();
		color = Vector(0.4f, 0.7f, 0.8f);
		alpha = 0.2f;
	}
	//debugLog(fbEffectVisible ? "ws on screen" : "ws not vis");
	this->fbEffectVisible = fbEffectVisible;
	this->renderBorder = RenderObject::renderCollisionShape;
	if(this->renderBorder)
	{
		this->renderBorderColor = fbEffectVisible ? Vector(1,0,1) : Vector(0,0.5f,1);
	}

	grid->setTexCoords(texcoords);
}

static void quadBlit(const RenderState& rs, unsigned tex)
{
	rs.gpu.setBlend(BLEND_DISABLED);
	glBindTexture(GL_TEXTURE_2D, tex);
	glColor4f(1,1,1,1);

	glPushMatrix();
	glLoadIdentity();
	glScalef(core->globalResolutionScale.x, core->globalResolutionScale.y, core->globalResolutionScale.z);

	int vw = core->getVirtualWidth();
	int vh = core->getVirtualHeight();
	int offx = -core->getVirtualOffX();
	int offy = -core->getVirtualOffY();

	// verts are in 0..1, transform so that we cover the entire screen
	glTranslatef(offx, offy, 0);
	glScalef(vw, vh, 1);

	core->blitQuad.render(rs);
	glPopMatrix();
}

void WaterSurfaceRender::render(const RenderState& rs) const
{
	if (game->waterLevel.x > 0)
	{
		if (fbEffectVisible)
		{
			// Everything up until now was rendered to the first page; flip it
			const unsigned curpage = core->frameBuffer.getCurrentPage();
			const unsigned oldtex = core->frameBuffer.getTextureID(curpage);
			const unsigned newtex = core->frameBuffer.getTextureID(curpage + 1);

			core->frameBuffer.replaceCapture(curpage + 1);

			// This appears to have problems with some intel drivers; the fallback path is good enough
			/*if(glCopyImageSubDataEXT)
				glCopyImageSubDataEXT(
					oldtex, GL_TEXTURE_2D, 0, 0, 0, 0,
					newtex, GL_TEXTURE_2D, 0, 0, 0, 0,
					core->width, core->height, 1
				);
			else*/
				quadBlit(rs, oldtex);
		}

		Quad::render(rs);
	}
}

void WaterSurfaceRender::onRender(const RenderState& rs) const
{
	if (!fbEffectVisible || game->waterLevel.x == 0) return;


	core->frameBuffer.bindTexture(core->frameBuffer.getCurrentPage() - 1);

	Quad::onRender(rs);

	glBindTexture(GL_TEXTURE_2D, 0);

	RenderObject::lastTextureApplied = 0;
}
