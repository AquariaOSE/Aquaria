#include "TileRender.h"
#include "RenderBase.h"
#include "Core.h"
#include "Tileset.h"

TileRender::TileRender(const TileStorage& tiles)
	: storage(tiles)
{
}

TileRender::~TileRender()
{
}

void TileRender::onRender(const RenderState& rs) const
{
	// prepare. get parallax scroll factors
	const RenderObjectLayer& rl = core->renderObjectLayers[this->layer];
	Vector M = rl.followCameraMult; // affected by parallaxLock
	const float F = rl.followCamera;

	// Formula from RenderObject::getFollowCameraPosition() and optimized for speed
	const Vector C = core->screenCenter;
	const Vector M1 = Vector(1,1) - M;
	const Vector T = C * (1 - F);

	unsigned lastTexRepeat = false;
	unsigned lastTexId = 0;
	BlendType blend = BLEND_DEFAULT; // TODO: influenced by efx
	const bool renderBorders = true; // TODO: when layer selected in editor

	for(size_t i = 0; i < storage.tiles.size(); ++i)
	{
		const TileData& tile = storage.tiles[i];
		const Vector tilepos(tile.x, tile.y);
		const Vector tmp = T + (F * tilepos);
		const Vector pos = tilepos * M1 + (tmp * M); // lerp, used to select whether to use original v or parallax-corrected v

		rs.gpu.setBlend(blend);

		ElementTemplate * const et = tile.et;
		if(Texture * const tex = et->tex.content())
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
			glBindTexture(GL_TEXTURE_2D, 0); // unlikely

		glPushMatrix();
		glTranslatef(pos.x, pos.y, pos.z);

		glRotatef(tile.rotation, 0, 0, 1);
		if(tile.flags & TILEFLAG_FH)
			glRotatef(180, 0, 1, 0);

		// this is only relevant in editor mode and is always 0 otherwise
		glTranslatef(tile.beforeScaleOffset.x, tile.beforeScaleOffset.y, tile.beforeScaleOffset.z);

		glScalef(tile.scale.x, tile.scale.y, 1);

		// TODO: only need to do this when prev. tile had different alpha
		{
			const float alpha = 1; // TODO: via efx
			Vector col = rs.color;
			glColor4f(col.x, col.y, col.z, rs.alpha*alpha);
		}

		const float _w2 = float(int(et->w)) * 0.5f;
		const float _h2 = float(int(et->h)) * 0.5f;

		// render texture
		{
			const Vector upperLeftTextureCoordinates(et->tu1, et->tv1);
			const Vector lowerRightTextureCoordinates(et->tu2, et->tv2);

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

		if(renderBorders)
		{
			lastTexId = 0;
			glBindTexture(GL_TEXTURE_2D, 0);

			Vector color(0.5f,0.5f,0.5f); // TODO: (1,1,1) when selected


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
}
