#include "RenderState.h"
#include "Base.h"
#include "RenderBase.h"
#include "RenderObject.h"


RenderState::RenderState(GPUState &gpu)
    : gpu(gpu), color(1,1,1), scale(1,1), alpha(1), pass(RenderObject::RENDER_ALL)
	, forceRenderBorder(false), forceRenderCenter(false), renderBorderAlpha(1)
{
}


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


GPUState::GPUState()
	: _blendType(BLEND_DISABLED)
{
	setBlend(BLEND_DEFAULT);
}

void GPUState::setBlend(BlendType bt)
{
	compile_assert(Countof(s_blendParams) == _BLEND_MAXSIZE);

	if(_blendType == bt)
		return;

	_blendType = bt;
	if (unsigned(bt) < _BLEND_MAXSIZE)
	{
		glEnable(GL_BLEND);
		const BlendParams& bp = s_blendParams[bt];
		glBlendFunc(bp.src, bp.dst);
	}
	else
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}
}

void GPUState::invalidateBlend()
{
	_blendType = _BLEND_MAXSIZE;
}
