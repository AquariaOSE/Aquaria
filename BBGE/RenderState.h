#ifndef BBGE_RENDERSTATE_H
#define BBGE_RENDERSTATE_H

#include "Vector.h"
#include "EngineEnums.h"

struct CombinedRenderAndGPUState;

// Only once of these exists at any time.
// It stores the known GPU state so that we don't need so many futile state changes
struct GPUState
{
    friend struct CombinedRenderAndGPUState;
    GPUState();

    void setBlend(BlendType bt);

private:
    BlendType _blendType;
};

// The RenderState is passed through the scene graph as each layer is rendered
// TODO: what needs to end up here? matrix stack too?
struct RenderState
{
    GPUState& gpu;

    Vector color;
    Vector scale;
    float alpha;
    int pass;

    bool forceRenderBorder;
    bool forceRenderCenter;
    Vector renderBorderColor;
    float renderBorderAlpha;

protected:
    RenderState(GPUState& gpu);
};

struct CombinedRenderAndGPUState : public RenderState
{
    GPUState gpu;
    CombinedRenderAndGPUState() : RenderState(gpu) {}
};



#endif
