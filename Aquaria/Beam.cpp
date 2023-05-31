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

Beam::Beam(Vector pos, float angle) : Quad()
{
	addType(SCO_BEAM);
	cull = false;
	trace();

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


	Vector mov(sinf(angle), cosf(angle));
	TileVector t(position);
	Vector startTile(t.x, t.y);



	int moves = 0;
	while (!game->isObstructed(TileVector(startTile.x, startTile.y)))
	{
		startTile += mov;
		moves++;
		if (moves > 1000)
			break;
	}
	t = TileVector(startTile.x, startTile.y);
	endPos = t.worldVector();



}

void Beam::onRender(const RenderState& rs) const
{

	Vector diff = endPos - position;
	Vector side = diff;

	side.setLength2D(beamWidth*2);
	Vector sideLeft = side.getPerpendicularLeft();
	Vector sideRight = side.getPerpendicularRight();

	glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(sideLeft.x, sideLeft.y);
		glTexCoord2f(1, 0);
		glVertex2f(sideLeft.x+diff.x, sideLeft.y+diff.y);
		glTexCoord2f(1, 1);
		glVertex2f(sideRight.x+diff.x, sideRight.y+diff.y);
		glTexCoord2f(0, 1);
		glVertex2f(sideRight.x, sideRight.y);
	glEnd();
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

