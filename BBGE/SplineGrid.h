#ifndef BBGE_SPLINEGRID_H
#define BBGE_SPLINEGRID_H

#include <vector>
#include "Vector.h"
#include "glm/glm.hpp"
#include "Quad.h"

class SplineGridCtrlPoint : public Quad
{
public:
	SplineGridCtrlPoint();
	virtual void onUpdate(float dt) OVERRIDE;
	Vector getSplinePosition() const;

	static SplineGridCtrlPoint *movingPoint;
};

class SplineGrid : public Quad
{
public:
	typedef Vector value_type;

	SplineGrid();
	~SplineGrid();

	// # of control points on each axis
	void resize(size_t w, size_t h, size_t xres, size_t yres);
	void recalc();
	void resetControlPoints();

	virtual void onRender(const RenderState& rs) const OVERRIDE;
	virtual void onUpdate(float dt) OVERRIDE;

private:

	SplineGridCtrlPoint *createControlPoint(size_t x, size_t y);

	unsigned degreeX, degreeY;

	std::vector<float> knotsX, knotsY;
	std::vector<value_type> tmp; // X-axis temporary values

	size_t _cpx, _cpy; // # of control points
	size_t _xres; // resolution for x-axis eval before doing 2D expansion
	size_t _yres;

	std::vector<SplineGridCtrlPoint*> ctrlp;
};


#endif
