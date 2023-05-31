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
#include "Web.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "RenderBase.h"

Web::Webs Web::webs;

Web::Web() : RenderObject()
{
	addType(SCO_WEB);
	webs.push_back(this);
	cull = false;
	parentEntity = 0;
	existence = 0;
}

void Web::setParentEntity(Entity *e)
{
	parentEntity = e;
}

void Web::killAllWebs()
{
	Webs websToDelete = webs; // copy
	for (Webs::iterator it = websToDelete.begin(); it != websToDelete.end(); it++)
		if(Web *s = *it)
			s->safeKill();

	webs.clear();
}

void Web::onEndOfLife()
{
	RenderObject::onEndOfLife();
	webs.remove(this);
}

void Web::setExistence(float t)
{
	existence = t;
}

int Web::addPoint(const Vector &point)
{
	points.push_back(point);
	return points.size()-1;
}

void Web::setPoint(size_t pt, const Vector &v)
{
	if (pt >= points.size()) return;
	points[pt] = v;
}

Vector Web::getPoint(size_t pt) const
{
	Vector v;
	if ( pt < points.size())
		v = points[pt];
	return v;
}

int Web::getNumPoints()
{
	return points.size();
}

void Web::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (!game->isPaused())
	{
		if (existence > 0)
		{
			existence -= dt;
			if (existence <= 0)
			{
				existence = 0;
				setLife(1);
				setDecayRate(1);
				fadeAlphaWithLife = 1;
				dsq->sound->playSfx("spiderweb");
			}
		}

		if (game->avatar && game->avatar->isInputEnabled())
		{
			FOR_ENTITIES(i)
			{
				bool hit = false;
				Entity *e = (*i);
				if ((e->getEntityType() == ET_ENEMY || e->getEntityType() == ET_AVATAR) && e->isDamageTarget(DT_ENEMY_WEB))
				{
					if (parentEntity != e && points.size() > 0)
					{
						for (size_t j = 0; j < points.size()-1; j++)
						{
							if (game->collideCircleVsLine(e, points[j], points[j+1], 4))
							{
								hit = true;
								break;
							}
						}
					}
				}
				if (hit)
				{

					e->vel /= float(e->getv(EV_WEBSLOW)) * dt;
				}
			}
		}
	}
}

void Web::onRender(const RenderState& rs) const
{
	glBindTexture(GL_TEXTURE_2D, 0);


	glLineWidth(4);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_LINES);
	if(points.size() > 0) {
		for (size_t i = 0; i < points.size()-1; i++) {
			glColor4f(1, 1, 1, 0.5f*alpha.x);
			glVertex3f(points[i].x, points[i].y, 0);
			glColor4f(1, 1, 1, 0.5f*alpha.x);
			glVertex3f(points[i+1].x, points[i+1].y, 0);
		}
	}
	glEnd();
}
