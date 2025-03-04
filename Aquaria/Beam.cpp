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

#include "Beam.h"
#include "Game.h"
#include "RenderBase.h"

#include "../BBGE/MathFunctions.h"

Beam::Beams Beam::beams;

Beam::Beam(Vector pos, float angle, unsigned maxrangeTiles) : Quad()
	, gpubuf(GPUBUF_VERTEXBUF | GPUBUF_DYNAMIC)
{
	addType(SCO_BEAM);
	cull = false;

	this->angle = angle;
	position = pos;

	setTexture("beam");
	beams.push_back(this);

	setBlendType(BLEND_ADD);
	alpha = 0;
	alpha.interpolateTo(1, 0.1f);
	trace();
	damageData.damageType = DT_ENEMY_BEAM;
	damageData.damage = 0.5f;

	beamWidth = 16;
	maxrange = maxrangeTiles;

	trace();
}

void Beam::setBeamWidth(float w)
{
	beamWidth = w;
}

void Beam::setDamage(float dmg)
{
	damageData.damage = dmg;
}

void Beam::setFirer(Entity *e)
{
	damageData.attacker = e;
}

void Beam::onEndOfLife()
{
	beams.remove(this);
}

void Beam::killAllBeams()
{
	Beams beamsToDelete = beams; // copy
	for (Beams::iterator it = beamsToDelete.begin(); it != beamsToDelete.end(); it++)
		if(Beam *s = *it)
			s->safeKill();

	beams.clear();
}

void Beam::trace()
{
	float angle = MathFunctions::toRadians(this->angle);


	const Vector mov(sinf(angle), cosf(angle));
	TileVector t(position);
	Vector startTile(t.x, t.y);

	unsigned moves = 0;
	while (!game->isObstructed(TileVector((int)startTile.x, (int)startTile.y)))
	{
		startTile += mov;
		moves++;
		if (moves > maxrange)
			break;
	}
	t = TileVector((int)startTile.x, (int)startTile.y);
	Vector wend = t.worldVector();
	endPos = position + mov * (wend - position).getLength2D();


	Vector diff = endPos - position;
	Vector side = diff;

	side.setLength2D(beamWidth*2);
	const Vector L = side.getPerpendicularLeft(); // sides, base
	const Vector R = side.getPerpendicularRight();
	const Vector Le = L + diff; // sides, far end
	const Vector Re = R + diff;

	const size_t bytes = 4 * 4 * sizeof(float); // 4 vertices, each xyuv
	do
	{
		float *p = (float*)gpubuf.beginWrite(GPUBUFTYPE_VEC2_TC, bytes, GPUACCESS_DEFAULT);

		*p++ = L.x;
		*p++ = L.y;
		*p++ = 0;
		*p++ = 0;

		*p++ = Le.x;
		*p++ = Le.y;
		*p++ = 1;
		*p++ = 0;

		*p++ = R.x;
		*p++ = R.y;
		*p++ = 0;
		*p++ = 1;

		*p++ = Re.x;
		*p++ = Re.y;
		*p++ = 1;
		*p++ = 1;
	}
	while(!gpubuf.commitWrite());
}

void Beam::onRender(const RenderState& rs) const
{
	gpubuf.apply();
	core->getDefaultQuadGrid()->getIndexBuf().drawElements(GL_TRIANGLES, 6); // 6 verts = 2 tris
}

void Beam::onUpdate(float dt)
{
	if (game->isPaused()) return;

	Quad::onUpdate(dt);

	if (alpha.x > 0.5f)
	{
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != damageData.attacker && e->isDamageTarget(damageData.damageType))
			{
				if (isTouchingLine(position, endPos, e->position, beamWidth + e->collideRadius))
				{
					e->damage(damageData);
				}
			}
		}
	}
}

