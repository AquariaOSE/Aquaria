#include "TileRender.h"
#include "RenderBase.h"
#include "Core.h"
#include "Tileset.h"
#include "RenderGrid.h"
#include "RenderObject.h"


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


void TileRender::onRender(const RenderState& rs) const
{
	// prepare. get parallax scroll factors
	const RenderObjectLayer& rl = core->renderObjectLayers[this->layer];
	const Vector M = rl.followCameraMult; // affected by parallaxLock
	const float F = rl.followCamera;
	const bool parallax = rl.followCamera > 0;

	// Formula from RenderObject::getFollowCameraPosition() and optimized for speed
	const Vector C = core->screenCenter;
	const Vector M1 = Vector(1,1) - M;
	const Vector T = C * (1 - F);

	unsigned lastTexRepeat = false;
	unsigned lastTexId = 0;

	const bool renderBorders = this->renderBorders;
	//bool mustSetColor = false;

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
		if(const Texture * const tex = et->tex.content())
		{
			unsigned texid = tex->gltexid;
			unsigned rep = tile.flags & TILEFLAG_REPEAT;
			if(texid != lastTexId || rep != lastTexRepeat)
			{
				lastTexId = texid;
				lastTexRepeat = rep;
				tex->apply(!!rep);
			}
		}
		else
		{
			lastTexId = 0;
			glBindTexture(GL_TEXTURE_2D, 0); // unlikely
		}

		glPushMatrix();
		glTranslatef(pos.x, pos.y, pos.z);

		glRotatef(tile.rotation, 0, 0, 1);
		if(tile.flags & TILEFLAG_FH) // TODO: This is not necessary! Since we have no children, flipped texcoords are fine
			glRotatef(180, 0, 1, 0);

		// this is only relevant in editor mode and is always 0 otherwise
		glTranslatef(tile.beforeScaleOffsetX, tile.beforeScaleOffsetY, 0);

		glScalef(tile.scalex, tile.scaley, 1);
		//glScalef(tile.scalex * et->w, tile.scaley * et->h, 1); // TODO use this + fixed verts

		BlendType blend = BLEND_DEFAULT;
		float alpha = rs.alpha;
		RenderGrid *grid = NULL;
		const TileEffectData * const eff = tile.eff;
		if(eff)
		{
			grid = eff->grid;
			alpha *= eff->alpha.x;
			blend = eff->blend;
		}

		rs.gpu.setBlend(blend);

		// TODO: only need to do this when prev. tile had different alpha
		glColor4f(rs.color.x, rs.color.y, rs.color.z, alpha);

		const Vector upperLeftTextureCoordinates(et->tu1, et->tv1);
		const Vector lowerRightTextureCoordinates(et->tu2, et->tv2);

		if(!grid)
		{
			const float _w2 = et->w * 0.5f;
			const float _h2 = et->h * 0.5f;

			glBegin(GL_QUADS);
			{
				glTexCoord2f(upperLeftTextureCoordinates.x, 1.0f-upperLeftTextureCoordinates.y);
				glVertex2f(-_w2, +_h2);

				glTexCoord2f(lowerRightTextureCoordinates.x, 1.0f-upperLeftTextureCoordinates.y);
				glVertex2f(+_w2, +_h2);

				glTexCoord2f(lowerRightTextureCoordinates.x, 1.0f-lowerRightTextureCoordinates.y);
				glVertex2f(+_w2, -_h2);

				glTexCoord2f(upperLeftTextureCoordinates.x, 1.0f-lowerRightTextureCoordinates.y);
				glVertex2f(-_w2, -_h2);
			}
			glEnd();
		}
		else
		{
			glPushMatrix();
			glScalef(et->w, et->h, 1);

			RenderState rx(rs);
			rx.alpha = alpha;
			grid->render(rx, upperLeftTextureCoordinates, lowerRightTextureCoordinates);
			if (RenderObject::renderCollisionShape)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				grid->renderDebugPoints(rs);
				lastTexId = 0;
			}

			glPopMatrix();
		}

		if(renderBorders)
		{
			lastTexId = 0;
			glBindTexture(GL_TEXTURE_2D, 0);

			float c = (tile.flags & TILEFLAG_SELECTED) ? 1.0f : 0.5f;
			Vector color(c,c,c);
			color *= getTagColor(tile.tag);
			const float _w2 = et->w * 0.5f;
			const float _h2 = et->h * 0.5f;

			glColor4f(color.x, color.y, color.z, 1.0f);
			glPointSize(16);
			glBegin(GL_POINTS);
				glVertex2f(0,0);
			glEnd();

			glLineWidth(2);
			glBegin(GL_LINE_STRIP);
				glVertex2f(_w2, _h2);
				glVertex2f(_w2, -_h2);
				glVertex2f(-_w2, -_h2);
				glVertex2f(-_w2, _h2);
				glVertex2f(_w2, _h2);
			glEnd();
		}


		glPopMatrix();
	}

	RenderObject::lastTextureApplied = lastTexId;
	RenderObject::lastTextureRepeat = !!lastTexRepeat;
}

void TileRender::onUpdate(float dt)
{
	//this->position = core->screenCenter;
}
