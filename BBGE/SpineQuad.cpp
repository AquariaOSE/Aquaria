#include "SpineQuad.h"
#include "RenderBase.h"
#include "Core.h"

SpineQuad::SpineQuad(size_t nodes, float width, float offsPerc)
	: RenderObject()
	, defaultWidth(width)
	, spineOffsPerc(offsPerc)
	, _texfh(false)
	, trisToDraw(0)
	, vbo(GPUBUF_DYNAMIC | GPUBUF_VERTEXBUF)
	, ibo(GPUBUF_STATIC | GPUBUF_INDEXBUF)

{
	points.resize(nodes);
	updateIBO();
}

SpineQuad::~SpineQuad()
{
}

void SpineQuad::onRender(const RenderState& rs) const
{
	vbo.apply();
	ibo.drawElements(GL_TRIANGLE_STRIP, trisToDraw);

	if(RenderObject::renderCollisionShape)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		RenderObject::lastTextureApplied = 0;
		glPointSize(2);
		glColor3f(1,0,1);
		glDrawArrays(GL_POINTS, 0, vbo.size() / (sizeof(float) * 4));

		//ibo.drawElements(GL_LINE_STRIP, ibo.size() / sizeof(unsigned short)); // strips wireframe
	}
}

void SpineQuad::updateIBO()
{
	const size_t N = points.size();
	const size_t space = (4 + (4 * N)) * sizeof(short);

	do
	{
		unsigned short * const begin = (unsigned short*)ibo.beginWrite(GPUBUFTYPE_U16, space, GPUACCESS_DEFAULT);
		unsigned short *p = begin;

		// Top strip first
		for(size_t i = 0; i < N; ++i)
		{
			*p++ = (unsigned short)(i * 3);
			*p++ = (unsigned short)(i * 3 + 1);
		}

		// Top triangle at the far end
		*p++ = (unsigned short)(N * 3);

		// Degenerate linker triangle to bottom strip
		*p++ = (unsigned short)(N * 3);
		*p++ = 2;

		// Continue with bottom strip
		for(size_t i = 0; i < N; ++i)
		{
			*p++ = (unsigned short)(i * 3 + 2);
			*p++ = (unsigned short)(i * 3 + 1);
		}

		// Bottom triangle at the far end
		*p++ = (unsigned short)(N * 3 + 1);

		assert(((char*)p - (char*)begin) == space);
	}
	while(!ibo.commitWrite());

	trisToDraw = (N+1)*2 * 2;
}

void SpineQuad::updateVBO()
{
	bool fh = _fh;
	const size_t N = points.size();
	const float texBits = 1.0f / (N-1);
	const Vector mul = !fh ? Vector(1, 1) : Vector(-1, -1);

	const float u0 = !fh ? 0.0f : 1.0f;
	const float u1 = 1.0f - u0;
	const float umid = u0 + u1 * spineOffsPerc;
	const float v0 = 0;
	const float v1 = 1;

	const Vector wp = this->getWorldPositionAndRotation();

	/* This is the draw pattern: The spine goes across the center line but may be biased towards either side
	* A, B, C... are points[].
	* o is computed from the perpendiculars of two adjacent points
	* numbers are the vertex index
	a---o---b---o---c---o--...y---o---z <- N*3+1
	0  /3\     /6\     /9\ .     / \  |
	| /   \   /   \   /   \.    /   \ |
	|/     \ /     \ /     .   /     \|
	A-------B-------C------...Y-------Z <- (N-1)*3+1
	1\     /4\     /7\     .   \     /|
	| \   /   \   /   \   /.    \   / |
	|  \ /     \ /     \ / .     \ /  |
	a---o---b---o---c---o--...y---o---z <- N*3+2
	2   5       8
	To compute:
	- Take the perpendiculars of A and B (points a, b, c) to either side
	- Their halfway point becomes a triangle corner
	*/

	Vector plprev(NoInit), prprev(NoInit);
	do
	{
		// 3 verts per hair node plus 2 corners at the end, each vertex is float xy+uv

		const size_t space = ((N * 3) + 2) * (2*2) * sizeof(float);
		float * const begin = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2_TC, space, GPUACCESS_DEFAULT);
		float *p = begin;

		const float lenleft = defaultWidth * spineOffsPerc;
		const float lenright = defaultWidth - lenleft;

		// top, A, bottom (left edge)
		{
			Vector p0 = points[0];
			Vector p1 = points[1];
			Vector diffVec = p1 - p0;
			Vector pl = diffVec.getPerpendicularLeft();
			pl.setLength2D(lenleft);
			Vector pr = diffVec.getPerpendicularRight();
			pr.setLength2D(lenright);

			// top
			*p++ = p0.x + pl.x;
			*p++ = p0.y + pl.y;
			*p++ = u0;
			*p++ = v0;

			// A
			*p++ = p0.x;
			*p++ = p0.y;
			*p++ = umid;
			*p++ = v0;

			// bottom
			*p++ = p0.x + pr.x;
			*p++ = p0.y + pr.y;
			*p++ = u1;
			*p++ = v0;

			plprev = pl;
			prprev = pr;
		}
		/* Middle; one segment of:
		o---...    <-- n
		 \         .
		  \        .
		   \       .
		    B---...<- n+1
		   /       .
		  /        .
		 /         .
		o---...    <- n+2  */
		for(size_t i = 1; i < N; ++i)
		{
			Vector p0 = points[i-1];
			Vector p1 = points[i];

			Vector diffVec = p1 - p0;
			Vector pl = diffVec.getPerpendicularLeft();
			pl.setLength2D(lenleft);
			Vector pr = diffVec.getPerpendicularRight();
			pr.setLength2D(lenright);
			const float v = texBits * float(i);
			const float vhalf = texBits * (float(i) - 0.5f);

			Vector ph = (p0 + p1) * 0.5f;
			Vector halfl = ph + (pl + plprev) * 0.5f;
			Vector halfr = ph + (pr + prprev) * 0.5f;

			// (A is added in prev iteration)

			// top
			*p++ = halfl.x;
			*p++ = halfl.y;
			*p++ = u0;
			*p++ = vhalf;

			// B
			*p++ = p1.x;
			*p++ = p1.y;
			*p++ = umid;
			*p++ = v;

			// bottom
			*p++ = halfr.x;
			*p++ = halfr.y;
			*p++ = u1;
			*p++ = vhalf;

			plprev = pl;
			prprev = pr;
		}
		/* End part (Z and o's were already added in last complete segment)
		o---z   <- n
		 \  |
		  \ |
		   \|
		    Z
		   /|
		  / |
		 /  |
		o---z   <- n+1
		(And the parpendiculars were already computed) */
		{
			Vector p0 = points[N-1];
			// top
			*p++ = p0.x + plprev.x;
			*p++ = p0.y + plprev.y;
			*p++ = u0;
			*p++ = v1;

			// bottom
			*p++ = p0.x + prprev.x;
			*p++ = p0.y + prprev.y;
			*p++ = u1;
			*p++ = v1;
		}

		assert(((char*)p - (char*)begin) == space);
	}
	while(!vbo.commitWrite());
}
