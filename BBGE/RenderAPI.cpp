#include "RenderAPI.h"
#include "Base.h"

#include "RenderBase.h"

namespace RenderAPI {

void BufferBase::destroy()
{
    deleteBuffer(&_bufid);
}

void BufferBase::upload(Hint usage)
{
    updateBuffer(&_bufid, _data, _bytes, usage);
}


// ----- GL backend starts here -----
// TODO: move backend to separate file once finalized

struct BlendParams
{
	GLenum src, dst;
};
static const BlendParams s_blendParams[] =
{
	{ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA },
	{ GL_SRC_ALPHA, GL_ONE },
	{ GL_ZERO, GL_SRC_ALPHA },
	{ GL_ZERO, GL_SRC_COLOR },
};

static const GLenum s_primTypes[] =
{
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_QUADS,
	GL_QUAD_STRIP
};


static void renderNonIndexed(const ObjectData& d)
{
	switch(d.layout)
	{
		case RAPI_LAYOUT_2D:
		{
			::glColor4fv(&d.color.r);
			const size_t n = d.verts->bytes() / sizeof(float);
			const float *p = (const float*)d.verts->data();
			const float * const end = p + n;
			for( ; p < end; p += 4) // Vertex2D
			{
				::glTexCoord2f(p[0], p[1]);
				::glVertex2f(p[2], p[3]);
			}
		}
		break;

		case RAPI_LAYOUT_2D_COLOR:
		{
			const size_t n = d.verts->bytes() / sizeof(float);
			const float *p = (const float*)d.verts->data();
			const float * const end = p + n;
			for( ; p < end; p += 8) // Vertex2DColor
			{
				::glColor4fv(&p[4]);
				::glTexCoord2f(p[0], p[1]);
				::glVertex2f(p[2], p[3]);
			}
		}
		break;
	}
}

static void renderIndexed(const ObjectData& d)
{
	switch(d.layout)
	{
		case RAPI_LAYOUT_2D:
		{
			::glColor4fv(&d.color.r);
			const size_t n = d.indices->bytes() / sizeof(unsigned short);
			const unsigned short *idx = (const unsigned short*)d.indices->data();
			const float * const pbase = (const float*)d.verts->data();
			const unsigned short * const end = idx + n;
			for( ; idx < end; ++idx)
			{
				const float *p = pbase + *idx;
				::glTexCoord2f(p[0], p[1]);
				::glVertex2f(p[2], p[3]);
			}
		}
		break;

		case RAPI_LAYOUT_2D_COLOR:
		{
			const size_t n = d.indices->bytes() / sizeof(unsigned short);
			const unsigned short *idx = (const unsigned short*)d.indices->data();
			const float * const pbase = (const float*)d.verts->data();
			const unsigned short * const end = idx + n;
			for( ; idx < end; ++idx)
			{
				const float *p = pbase + *idx;
				::glColor4fv(&p[4]);
				::glTexCoord2f(p[0], p[1]);
				::glVertex2f(p[2], p[3]);
			}
		}
		break;
	}
}

void render(const ObjectData* objs, size_t n)
{
	::glPushMatrix();
    char lastblend = BLEND_DISABLED - 1; // always fail the first check
	unsigned lasttex = unsigned(-1);
    const ObjectData* const end = objs + n;
    for( ; objs < end; ++objs)
    {
        const ObjectData& d = *objs;
        if(lastblend != d.blend)
        {
			lastblend = d.blend;
            unsigned ublend = unsigned(int(d.blend)); // this underflows if BLEND_DISABLED
            if (ublend < Countof(s_blendParams))
			{
				::glEnable(GL_BLEND);
				const BlendParams& bp = s_blendParams[ublend];
				::glBlendFunc(bp.src, bp.dst);
			}
			else
			{
				::glDisable(GL_BLEND);
				::glDisable(GL_ALPHA_TEST);
			}
        }


		::glLoadMatrixf(d.pmat);


		const unsigned prim = d.prim;
		switch(prim)
		{
			case RAPI_PRIM_POINTS:
				::glPointSize(d.u.linewidth);
				break;
			case RAPI_PRIM_LINES:
			case RAPI_PRIM_LINE_STRIP:
				::glLineWidth(d.u.linewidth);
				break;

			default:
				if(d.u.texid != lasttex)
				{
					lasttex = d.u.texid;
					::glBindTexture(GL_TEXTURE_2D, d.u.texid);
				}
		}

		::glBegin(s_primTypes[prim]);
		if(!d.indices)
			renderNonIndexed(d);
		else
			renderIndexed(d);
		::glEnd();
    }
	::glPopMatrix();
}

void updateBuffer(unsigned* pbufid, const void* data, size_t bytes, RenderAPI::BufferBase::Hint usage)
{
}

void deleteBuffer(unsigned* pbufid)
{
}

}
