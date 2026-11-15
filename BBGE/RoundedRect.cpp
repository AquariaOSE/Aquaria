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
#include "RoundedRect.h"
#include "Core.h"
#include "TTFFont.h"
#include "RenderBase.h"

#include <assert.h>

enum
{
	RR_STEPS = 10,

	// center, sides, four corners
	RR_NUM_QUADS = 1 + 2 + 4*RR_STEPS,

	RR_NUM_VERTS = 4 * RR_NUM_QUADS,
	RR_NUM_INDICES = 6 * RR_NUM_QUADS
};


RoundedRect::RoundedRect(int w, int h, int radius)
	: RenderObject()
	, vbo(GPUBUF_STATIC | GPUBUF_VERTEXBUF)
	, ibo(GPUBUF_STATIC | GPUBUF_INDEXBUF)

{
	alphaMod = 0.75;
	color = 0;

	cull = false;

	followCamera = 1;

	setWidthHeight(w, h, radius);
}

void RoundedRect::setWidthHeight(int w, int h, int radius)
{
	this->radius = radius;
	width = w-radius*2;
	height = h-radius*2;

	updateVBO();
}

void RoundedRect::show()
{
	if (alpha.x == 0)
	{
		const float t = 0.1f;
		alpha = 0;
		alpha.interpolateTo(1, t);
		scale = Vector(0.5f, 0.5f);
		scale.interpolateTo(Vector(1,1), t);
	}
}

void RoundedRect::hide()
{
	const float t = 0.1f;
	alpha = 1.0;
	alpha.interpolateTo(0, t);
	scale = Vector(1, 1);
	scale.interpolateTo(Vector(0.5f,0.5f), t);
}

void RoundedRect::onRender(const RenderState& rs) const
{
	vbo.apply();
	ibo.drawElements(GL_TRIANGLES, RR_NUM_INDICES);
}

struct V2
{
	float x, y;
};
struct SQuad
{
	V2 v[4];
};

struct QuadAdder
{
	float *p;
	unsigned short *i;
	unsigned idx;
	QuadAdder(float *p, unsigned short *i) : p(p), i(i), idx(0) {}
	void add(const SQuad& q)
	{
		for(size_t k = 0; k < 4; ++k)
		{
			*p++ = q.v[k].x;
			*p++ = q.v[k].y;
		}

		*i++ = idx;
		*i++ = idx + 1;
		*i++ = idx + 2;

		*i++ = idx + 1;
		*i++ = idx + 2;
		*i++ = idx + 3;

		idx += 4;
	}
};

void RoundedRect::updateVBO()
{
	const size_t vbytes = RR_NUM_VERTS * 2 * sizeof(float);
	const size_t ibytes = RR_NUM_INDICES * sizeof(unsigned short);

	const float w2 = width * 0.5f;
	const float h2 = height * 0.5f;

	const float iter = PI_HALF / RR_STEPS;

	bool ok;
	do
	{
		float *p = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2, vbytes, GPUACCESS_DEFAULT);
		unsigned short *idxs = (unsigned short*)ibo.beginWrite(GPUBUFTYPE_U16, ibytes, GPUACCESS_DEFAULT);
		QuadAdder qa(p, idxs);

		float angle = 0;


		for (unsigned i = 0; i < RR_STEPS; ++i, angle+=iter)
		{
			// top right
			{
				float x1 = sinf(angle)*radius, y1 = -cosf(angle)*radius;
				float x2 = sinf(angle+iter)*radius, y2 = -cosf(angle+iter)*radius;
				SQuad q =
				{{
					{ w2 + x1, -h2 + 0 },
					{ w2 + x2, -h2 + 0 },
					{ w2 + x1, -h2 + y1 },
					{ w2 + x2, -h2 + y2 }
				}};
				qa.add(q);
			}
			// top left
			{
				float x1 = -sinf(angle)*radius, y1 = -cosf(angle)*radius;
				float x2 = -sinf(angle+iter)*radius, y2 = -cosf(angle+iter)*radius;
				SQuad q =
				{{
					{ -w2 + x1, -h2 + 0 },
					{ -w2 + x2, -h2 + 0 },
					{ -w2 + x1, -h2 + y1 },
					{ -w2 + x2, -h2 + y2 }
				}};
				qa.add(q);
			}
			{
				float x1 = sinf(angle)*radius, y1 = cosf(angle)*radius;
				float x2 = sinf(angle+iter)*radius, y2 = cosf(angle+iter)*radius;
				SQuad q =
				{{
					{ w2 + x1, h2 + 0 },
					{ w2 + x2, h2 + 0 },
					{ w2 + x1, h2 + y1 },
					{ w2 + x2, h2 + y2 }
				}};
				qa.add(q);
			}
			{
				float x1 = -sinf(angle)*radius, y1 = cosf(angle)*radius;
				float x2 = -sinf(angle+iter)*radius, y2 = cosf(angle+iter)*radius;
				SQuad q =
				{{
					{ -w2 + x1, h2 + 0 },
					{ -w2 + x2, h2 + 0 },
					{ -w2 + x1, h2 + y1 },
					{ -w2 + x2, h2 + y2 }
				}};
				qa.add(q);
			}
		}

		//middle, top, btm
		{
			SQuad q =
			{{
				{ w2, h2 + radius },
				{ -w2, h2 + radius },
				{ w2, -h2 - radius },
				{ -w2, -h2 - radius }

			}};
			qa.add(q);
		}

		// left
		{
			SQuad q =
			{{
				{ -w2 - radius, h2 },
				{ -w2, h2 },
				{ -w2 - radius, -h2 },
				{ -w2, -h2 }
			}};
			qa.add(q);
		}

		// right
		{
			SQuad q =
			{{
				{ w2 + radius, h2 },
				{ w2, h2 },
				{ w2 + radius, -h2 },
				{ w2, -h2 },
			}};
			qa.add(q);
		}

		ok = vbo.commitWriteExact(qa.p) && ibo.commitWriteExact(qa.i);
	}
	while(!ok);
}
