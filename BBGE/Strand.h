#ifndef SEGMENTED_STRAND_H
#define SEGMENTED_STRAND_H

#include "RenderObject.h"
#include "VertexBuffer.h"

class Strand : public RenderObject
{
public:
	Strand(const Vector &position, size_t segs, float dist=32);
protected:
	void onUpdate(float dt) OVERRIDE;
	void onRender(const RenderState& rs) const OVERRIDE;

private:
	void updatePoints();

	DynamicGPUBuffer gpubuf;
	std::vector<Vector> points;
	float dist;
};


#endif
