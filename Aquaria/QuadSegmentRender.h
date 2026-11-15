#ifndef QUADSEGMENTRENDER_H
#define QUADSEGMENTRENDER_H

#include "RenderObject.h"
#include "VertexBuffer.h"
#include "GameEnums.h"

class QuadSegmentRender : public RenderObject
{
public:
	QuadSegmentRender(PathType pt, bool shortenSegs);
	virtual ~QuadSegmentRender();
protected:
	void onRender(const RenderState& rs) const OVERRIDE;
	void onUpdate(float dt) OVERRIDE;
	size_t writeVBOData(float *p);
	DynamicGPUBuffer vbo;
	size_t _verticesToRender;
	const PathType pathtype;
	const bool _shortenSegs;
};

class CurrentRender : public QuadSegmentRender
{
public:
	CurrentRender();
	virtual ~CurrentRender();
};

class SteamRender : public QuadSegmentRender
{
public:
	SteamRender();
	virtual ~SteamRender();
};


#endif

