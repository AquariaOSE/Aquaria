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
#include "Gradient.h"
#include "Core.h"
#include "RenderBase.h"

Gradient::Gradient() : RenderObject()
	, vbo(GPUBUF_VERTEXBUF | GPUBUF_DYNAMIC)
{
	autoWidth = autoHeight = 0;
}

void Gradient::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (autoWidth == AUTO_VIRTUALWIDTH)
	{
		scale.x = core->getVirtualWidth();
	}

	if (autoHeight == AUTO_VIRTUALHEIGHT)
	{
		scale.y = core->getVirtualHeight();
	}
}

void Gradient::makeVertical(Vector c1, Vector c2)
{
	ulc0 = c1;
	ulc1 = c1;
	ulc2 = c2;
	ulc3 = c2;
	updateVBO();
}

void Gradient::makeHorizontal(Vector c1, Vector c2)
{
	ulc0 = c1;
	ulc1 = c2;
	ulc2 = c2;
	ulc3 = c1;
	updateVBO();
}

void Gradient::updateVBO()
{
	const size_t bytes = 4 * 6 * sizeof(float);
	do
	{
		float *p = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2_RGBA, bytes, GPUACCESS_DEFAULT);
		Vector c = _NoInit();

		*p++ = 0.5f; *p++ = 0.5f;
		c = ulc3;
		*p++ = c.x; *p++ = c.y; *p++ = c.z; *p++ = alpha.x;

		*p++ = -0.5f; *p++ = 0.5f;
		c = ulc2;
		*p++ = c.x; *p++ = c.y; *p++ = c.z; *p++ = alpha.x;

		*p++ = 0.5f; *p++ = -0.5f;
		c = ulc0;
		*p++ = c.x; *p++ = c.y; *p++ = c.z; *p++ = alpha.x;

		*p++ = -0.5f; *p++ = -0.5f;
		c = ulc1;
		*p++ = c.x; *p++ = c.y; *p++ = c.z; *p++ = alpha.x;

	}
	while(!vbo.commitWrite());

}

void Gradient::onRender(const RenderState& rs) const
{
	vbo.apply();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

