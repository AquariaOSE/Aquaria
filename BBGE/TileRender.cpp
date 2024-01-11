#include "TileRender.h"
#include "RenderBase.h"
#include "Core.h"
#include "Tileset.h"
#include "RenderGrid.h"
#include "RenderObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct StaticData
{
	glm::mat4 fh, fv;
	StaticData()
		: fh(glm::rotate(glm::mat4(), 180.0f, glm::vec3(0,1,0)))
		, fv(glm::rotate(glm::mat4(), 180.0f, glm::vec3(1,0,0)))
	{
	}
};

static const StaticData staticdata;


TileRender::TileRender(const TileStorage& tiles)
	: storage(tiles), renderBorders(false)
{
	this->cull = false;
	this->neverFollowCamera = true;
}

TileRender::~TileRender()
{
}


// shamelessly ripped from paint.net default palette
static const Vector s_tagColors[] =
{
	/* 0 */ Vector(0.5f, 0.5f, 0.5f),
	/* 1 */ Vector(1,0,0),
	/* 2 */ Vector(1, 0.415686f, 0),
	/* 3 */ Vector(1,0.847059f, 0),
	/* 4 */ Vector(0.298039f,1,0),
	/* 5 */ Vector(0,1,1),
	/* 6 */ Vector(0,0.580392,1),
	/* 7 */ Vector(0,0.149020f,1),
	/* 8 */ Vector(0.282353f,0,1),
	/* 9 */ Vector(0.698039f,0,1),

	/* 10 */ Vector(1,0,1), // anything outside of the pretty range
};

static inline const Vector& getTagColor(int tag)
{
	const unsigned idx = std::min<unsigned>(unsigned(tag), Countof(s_tagColors)-1);
	return s_tagColors[idx];

}

Vector TileRender::GetTagColor(int tag)
{
	return getTagColor(tag);
}


void TileRender::onRender(const RenderState& rs) const
{
	if(storage.tiles.empty())
		return;

	if(renderBorders)
		glLineWidth(2);

	RenderState rx(rs);

	// FIXME: this is such a hack. remove this once we pass matrices explicitly
	glm::mat4 originalMat;
	glGetFloatv(GL_MODELVIEW_MATRIX, &originalMat[0][0]);

	// prepare. get parallax scroll factors
	const RenderObjectLayer& rl = core->renderObjectLayers[this->layer];
	const Vector M = rl.followCameraMult; // affected by parallaxLock
	const float F = rl.followCamera;
	const bool parallax = rl.followCamera > 0;

	// Formula from RenderObject::getFollowCameraPosition() and optimized for speed
	const Vector C = core->screenCenter;
	const Vector M1 = Vector(1,1) - M;
	const Vector T = C * (1 - F);

	unsigned lastTexId = 0;

	const bool renderExtras = renderBorders || RenderObject::renderCollisionShape;

	const TileEffectData *prevEff = ((TileEffectData*)NULL)+1; // initial value is different from anything else

	for(size_t i = 0; i < storage.tiles.size(); ++i)
	{
		const TileData& tile = storage.tiles[i];
		if(tile.flags & (TILEFLAG_HIDDEN | TILEFLAG_EDITOR_HIDDEN))
			continue;

		Vector pos(tile.x, tile.y);
		if(parallax)
		{
			const Vector tmp = T + (F * pos);
			pos = pos * M1 + (tmp * M); // lerp, used to select whether to use original v or parallax-corrected v
		}

		const ElementTemplate * const et = tile.et;
		const float sw = et->w * tile.scalex;
		const float sh = et->h * tile.scaley;

		// adapted from RenderObject::isOnScreen()
		{
			const float cullRadiusSqr = ((sw*sw + sh*sh) * core->invGlobalScaleSqr) + core->cullRadiusSqr;
			if ((pos - core->cullCenter).getSquaredLength2D() >= cullRadiusSqr)
				continue;
		}

		if(const Texture * const tex = et->tex.content())
		{
			unsigned texid = tex->gltexid;
			if(texid != lastTexId)
			{
				lastTexId = texid;
				tex->apply();
			}
		}
		else
		{
			lastTexId = 0;
			glBindTexture(GL_TEXTURE_2D, 0); // unlikely
		}

		float alpha = rs.alpha;
		const TileEffectData * const eff = tile.eff;
		if(eff != prevEff) // effects between tiles are often shared so this works not only for NULL
		{
			prevEff = eff;
			BlendType blend = BLEND_DEFAULT;

			if(eff)
			{
				alpha *= eff->alpha.x;
				blend = eff->blend;
			}

			rs.gpu.setBlend(blend);
			glColor4f(rs.color.x, rs.color.y, rs.color.z, alpha);
		}

		glm::mat4 m = glm::translate(originalMat, glm::vec3(pos.x, pos.y, pos.z));

		// HACK: Due to a renderer bug in older versions, vertical flip is ignored
		// when a grid-based tile effect is applied.
		// Maps were designed with the bug present so we need to replicate it,
		// otherwise things won't look correct.
		unsigned effflag = tile.flags;
		if(eff && eff->grid)
			effflag &= ~TILEFLAG_FV;

		float effrot = tile.rotation;

		// both set? that's effectively a rotation by 180°
		if ((effflag & (TILEFLAG_FH | TILEFLAG_FV)) == (TILEFLAG_FH | TILEFLAG_FV))
			effrot += 180;

		m = glm::rotate(m, effrot, glm::vec3(0,0,1));


		switch(effflag & (TILEFLAG_FH | TILEFLAG_FV))
		{
			case TILEFLAG_FH:
				m *= staticdata.fh;
				break;

			case TILEFLAG_FV:
				m *= staticdata.fv;
				break;

			default: ; // both or none set, nothing to do
		}

		m = glm::scale(m, glm::vec3(sw, sh, 1));

		glLoadMatrixf(&m[0][0]);


		const RenderGrid *grid = tile.getGrid();
		if(!grid)
			grid = core->getDefaultQuadGrid();
		rx.alpha = alpha;
		grid->render(rx);


		if(renderExtras)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			lastTexId = 0;
			prevEff = ((TileEffectData*)NULL)+1;

			rs.gpu.setBlend(BLEND_DEFAULT);

			if(grid != core->getDefaultQuadGrid() && RenderObject::renderCollisionShape)
			{
				grid->renderDebugPoints(rs);
			}

			if(renderBorders)
			{
				float c = (tile.flags & TILEFLAG_SELECTED) ? 1.0f : 0.5f;
				Vector color(c,c,c);
				color *= getTagColor(tile.tag);

				glColor4f(color.x, color.y, color.z, 1.0f);
				core->getDefaultQuadBorderBuf()->apply();
				glPointSize(16);
				glDrawArrays(GL_POINTS, 4, 1);
				glDrawArrays(GL_LINE_LOOP, 0, 4);
			}
		}
	}

	glLoadMatrixf(&originalMat[0][0]);

	RenderObject::lastTextureApplied = lastTexId;
}

void TileRender::onUpdate(float dt)
{
	//this->position = core->screenCenter;
}
