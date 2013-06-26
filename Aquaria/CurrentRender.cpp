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

#include "../BBGE/AfterEffect.h"

CurrentRender::CurrentRender() : RenderObject()
{
	cull = false;
	//alpha = 0.2f;
	setTexture("Particles/Current");
	texture->repeat = true;
	rippleDelay = 2;
}

void CurrentRender::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	/*
	rippleDelay -= dt;
	if (rippleDelay < 0)
	{
		for (int i = 0; i < dsq->game->paths.size()-1; i++)
		{
			Path *p = dsq->game->paths[i];
			for (int n = 0; n < p->nodes.size()-1; i++)
			{
				PathNode *n1 = &p->nodes[n];
				PathNode *n2 = &p->nodes[n+1];
				Vector diff = n2->position - n1->position;
				Vector pos = n1->position + diff*p->animOffset;
				// spawn effect at pos
				if (core->afterEffectManager)
					core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),pos,0.04,0.06,15,0.2f));
			}
		}
		rippleDelay = 1.0;
	}
	*/
}

void CurrentRender::onRender()
{
#ifdef BBGE_BUILD_OPENGL
	// note: Leave cull_face disabled!?
	//glDisable(GL_CULL_FACE);
	//int qs = 0;
	for (Path *p = dsq->game->getFirstPathOfType(PATH_CURRENT); p; p = p->nextOfType)
	{
		if (p->active)
		{

			/*
			std::ostringstream os;
			os << "animOffset: " << p->animOffset;
			debugLog(os.str());
			*/

			int w2 = p->rect.getWidth()/2;

			/*
			if (false)
			{
				float offset = 0;
				glBegin(GL_QUAD_STRIP);
				int sz = p->nodes.size();
				float len = 0;


				float totalLength = 0;
				for (int n = 0; n < sz-1; n++)
				{
					totalLength += (p->nodes[n+1].position - p->nodes[n].position).getLength2D();
				}

				float texScale = totalLength/256.0f;

				Vector p1, p2, diff, pl, pr;
				for (int n = 0; n < sz; n++)
				{
					PathNode *n1 = &p->nodes[n];
					p1 = n1->position;
					if (n == sz-1)
					{
						PathNode *n2 = &p->nodes[n-1];
						p2 = n2->position;
						diff = p1-p2;
					}
					else
					{
						PathNode *n2 = &p->nodes[n+1];
						p2 = n2->position;
						diff = p2-p1;
					}
					len = diff.getLength2D();
					float add = len/totalLength;
					//texScale = len/totalLength;

					pl = diff.getPerpendicularLeft();
					pr = diff.getPerpendicularRight();

					pl.setLength2D(w2);
					pr.setLength2D(w2);
					Vector r1 = p1+pl;

					Vector r4 = p1+pr;


					if (n == 0 || n == sz-1)
					{
						glColor4f(1, 1, 1, 0);
					}
					else
					{
						glColor4f(1, 1, 1, 1);
					}
					//(0+p->animOffset)*texScale +
					glTexCoord2f((offset)*texScale+p->animOffset, 0);
					glVertex2f(r1.x, r1.y);

					glTexCoord2f((offset)*texScale+p->animOffset, 1);
					//(0+p->animOffset)*texScale +
					glVertex2f(r4.x, r4.y);

					offset += add;

				}
				glEnd();
			}
			else
			*/

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

					//bool edge = false;

					/*
					if (n == 0)
					{
						p1 -= diff*0.25f;
						edge = true;
					}

					if (n == sz-1)
					{
						p2 += diff*0.25f;
						edge = true;
					}

					diff = p2-p1;
					*/

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
						//float texScale2 = texScale;

						/*
						if (edge)
							texScale *= 2;
						*/

						/*
						if (edge)
							texScale2 *= 4;
						*/

						if (isTouchingLine(p1, p2, dsq->screenCenter, dsq->cullRadius+p->rect.getWidth()/2.0f))
						{
							//qs++;

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
						//qs++;
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
		//glEnd();

	}
	//glEnable(GL_CULL_FACE);

	/*
	std::ostringstream os;
	os << "current quads: " << qs;
	debugLog(os.str());
	*/

#endif

}

