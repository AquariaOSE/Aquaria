#ifndef BBGE_SPINEQUAD_H
#define BBGE_SPINEQUAD_H

#include "Quad.h"

class SpineQuad : public RenderObject
{
public:
	// When offsPerc = 0, the spine goes along the center.
	// When in [-1 .. 1] it goes along either edge
	SpineQuad(size_t nodes, float width, float offsPerc);
	virtual ~SpineQuad();


	void updateVBO();
	void updateIBO();
	void setTextureFlip(bool flip) { _texfh = flip; }

	virtual void onRender(const RenderState& rs) const OVERRIDE;

	std::vector<Vector> _points; // x, y: position; z: width multiplier


protected:
	void updateVBO(const Vector *points, size_t n);
	float defaultWidth;
	float spineOffsPerc; // 0..1
	bool _texfh;
	bool pointsAreInWorldCoordSystem;


	size_t trisToDraw;
	DynamicGPUBuffer ibo, vbo;
};




#endif
