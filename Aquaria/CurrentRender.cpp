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
#include "Path.h"
#include "Game.h"

#include "../BBGE/AfterEffect.h"

CurrentRender::CurrentRender() : RenderObject()
{
	cull = false;

	setTexture("Particles/Current");
	repeatTexture = true;
	rippleDelay = 2;
}

void CurrentRender::onRender(const RenderState& rs) const
{
	// note: Leave cull_face disabled!?
	//glDisable(GL_CULL_FACE);
	//int qs = 0;
	for (Path *p = game->getFirstPathOfType(PATH_CURRENT); p; p = p->nextOfType)
	{
		if (p->active)
		{



			int w2 = p->rect.getWidth()/2;



			if (true)
			{
				int sz = p->nodes.size()-1;
				for (int n = 0; n < sz; n++)
				{
					PathNode *n1 = &p->nodes[n];
					PathNode *n2 = &p->nodes[n+1];
					Vector p1 = n1->position;
					Vector p2 = n2->position;
					Vector diff = p2-p1;
					Vector d = diff;
					d.setLength2D(p->rect.getWidth());
					p1 -= d*0.75f;
					p2 += d*0.75f;
					diff = p2 - p1;



					if (!diff.isZero())
					{
						Vector pl = diff.getPerpendicularLeft();
						Vector pr = diff.getPerpendicularRight();
						pl.setLength2D(w2);
						pr.setLength2D(w2);

						Vector p15 = p1 + diff * 0.25f;
						Vector p25 = p2 - diff * 0.25f;
						Vector r1 = p1+pl;
						Vector r2 = p1+pr;
						Vector r3 = p15+pl;
						Vector r4 = p15+pr;
						Vector r5 = p25+pl;
						Vector r6 = p25+pr;
						Vector r7 = p2+pl;
						Vector r8 = p2+pr;
						float len = diff.getLength2D();
						float texScale = len/256.0f;



						if (isTouchingLine(p1, p2, dsq->screenCenter, dsq->cullRadius+p->rect.getWidth()/2.0f))
						{


							glBegin(GL_QUAD_STRIP);
								glColor4f(1,1,1,0);
								glTexCoord2f((0)*texScale+p->animOffset, 0);
								glVertex2f(r1.x, r1.y);

								glTexCoord2f((0)*texScale+p->animOffset, 1);
								glVertex2f(r2.x, r2.y);

								glColor4f(1,1,1,p->amount);
								glTexCoord2f((0+0.25f)*texScale+p->animOffset, 0);
								glVertex2f(r3.x, r3.y);

								glTexCoord2f((0+0.25f)*texScale+p->animOffset, 1);
								glVertex2f(r4.x, r4.y);

								glColor4f(1,1,1,p->amount);
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
			else
			{
				int sz = p->nodes.size()-1;
				for (int n = 0; n < sz; n++)
				{
					PathNode *n1 = &p->nodes[n];
					PathNode *n2 = &p->nodes[n+1];
					Vector p1 = n1->position;
					Vector p2 = n2->position;
					Vector diff = p2-p1;
					Vector pl = diff.getPerpendicularLeft();
					Vector pr = diff.getPerpendicularRight();
					pl.setLength2D(w2);
					pr.setLength2D(w2);
					Vector r1 = p1+pl;
					Vector r2 = p2+pl;
					Vector r3 = p2+pr;
					Vector r4 = p1+pr;
					float len = diff.getLength2D();
					float texScale = len/256.0f;

					if (isTouchingLine(p1, p2, dsq->screenCenter, dsq->cullRadius))
					{

						glBegin(GL_QUADS);
							if (n==0)
								glColor4f(1,1,1,0);
							else
								glColor4f(1,1,1,alpha.x);
							glTexCoord2f((0+p->animOffset)*texScale, 0);
							glVertex2f(r1.x, r1.y);

							if (n==sz-1)
								glColor4f(1,1,1,0);
							else
								glColor4f(1,1,1,alpha.x);

							glTexCoord2f((1+p->animOffset)*texScale, 0);
							glVertex2f(r2.x, r2.y);

							glTexCoord2f((1+p->animOffset)*texScale, 1);
							glVertex2f(r3.x, r3.y);

							if (n==0)
								glColor4f(1,1,1,0);
							else
								glColor4f(1,1,1,alpha.x);
							glTexCoord2f((0+p->animOffset)*texScale, 1);
							glVertex2f(r4.x, r4.y);
						glEnd();
					}
				}
			}
		}


	}



}

