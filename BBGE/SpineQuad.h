#ifndef BBGE_SPINEQUAD_H
#define BBGE_SPINEQUAD_H

#include "Quad.h"

class SpineQuad : public RenderObject
{
public:
	SpineQuad(size_t nodes, float width, float offsPerc);
	virtual ~SpineQuad();

	void updateVBO();
	void updateIBO();
	void setTextureFlip(bool flip) { _texfh = flip; }

	void onRender(const RenderState& rs) const OVERRIDE;

	std::vector<Vector> points; // x, y: position; z: width multiplier


protected:
	float defaultWidth;
	float spineOffsPerc; // 0..1
	bool _texfh;


	size_t trisToDraw;
	DynamicGPUBuffer ibo, vbo;
};




#endif
