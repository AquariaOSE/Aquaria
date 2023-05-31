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
#include "RenderBase.h"
#include "../BBGE/AfterEffect.h"
#include "Game.h"

SteamRender::SteamRender() : RenderObject()
{
	cull = false;

	alpha = 0.7f;
	setTexture("Particles/Steam");
	repeatTexture = true;
	rippleDelay = 2;
	setBlendType(BLEND_ADD);
}

void SteamRender::onRender(const RenderState& rs) const
{
	if(!game) return;


	for (Path *p = game->getFirstPathOfType(PATH_STEAM); p; p = p->nextOfType)
	{
		if (p->active)
		{

			int w2 = p->rect.getWidth()/2;

			if (true)
			{
				const int sz = p->nodes.size()-1;
				for (int n = 0; n < sz; n++)
				{
					const PathNode *n1 = &p->nodes[n];
					const PathNode *n2 = &p->nodes[n+1];
					const Vector p1 = n1->position;

					const Vector p2 = n2->position;
					Vector diff = p2-p1;
					if (!diff.isZero())
					{
						Vector pl = diff.getPerpendicularLeft();
						Vector pr = diff.getPerpendicularRight();
						pl.setLength2D(w2);
						pr.setLength2D(w2);

						if (isTouchingLine(p1, p2, dsq->screenCenter, dsq->cullRadius + p->rect.getWidth()/2.0f))
						{
							const Vector p15 = n1->position + diff * 0.25f;
							const Vector p25 = n2->position - diff * 0.25f;
							const Vector r1 = p1+pl;
							const Vector r2 = p1+pr;
							const Vector r3 = p15+pl;
							const Vector r4 = p15+pr;
							const Vector r5 = p25+pl;
							const Vector r6 = p25+pr;
							const Vector r7 = p2+pl;
							const Vector r8 = p2+pr;
							const float len = diff.getLength2D();
							const float texScale = len/256.0f;


							glBegin(GL_QUAD_STRIP);
								glColor4f(1,1,1,0);
								glTexCoord2f((0)*texScale+p->animOffset, 0);
								glVertex2f(r1.x, r1.y);

								glTexCoord2f((0)*texScale+p->animOffset, 1);
								glVertex2f(r2.x, r2.y);

								glColor4f(1,1,1,alpha.x);
								glTexCoord2f((0+0.25f)*texScale+p->animOffset, 0);
								glVertex2f(r3.x, r3.y);

								glTexCoord2f((0+0.25f)*texScale+p->animOffset, 1);
								glVertex2f(r4.x, r4.y);

								glColor4f(1,1,1,alpha.x);
								glTexCoord2f((1-0.25f)*texScale+p->animOffset, 0);
								glVertex2f(r5.x, r5.y);

								glTexCoord2f((1-0.25f)*texScale+p->animOffset, 1);
								glVertex2f(r6.x, r6.y);

								glColor4f(1,1,1,0);
								glTexCoord2f((1)*texScale+p->animOffset, 0);
								glVertex2f(r7.x, r7.y);

								glTexCoord2f((1)*texScale+p->animOffset, 1);
								glVertex2f(r8.x, r8.y);
							glEnd();
						}
					}
				}
			}
		}
	}


}

