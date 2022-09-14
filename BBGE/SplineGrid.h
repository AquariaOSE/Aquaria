#ifndef BBGE_SPLINEGRID_H
#define BBGE_SPLINEGRID_H

#include <vector>
#include "Vector.h"
#include "glm/glm.hpp"
#include "Quad.h"
#include "Interpolators.h"

enum SplineType
{
	SPLINE_BSPLINE,
	SPLINE_COSINE,
};

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
	void resize(size_t w, size_t h, size_t xres, size_t yres, unsigned deg);
	void recalc();

	void resetControlPoints();

	virtual void onRender(const RenderState& rs) const OVERRIDE;
	virtual void onUpdate(float dt) OVERRIDE;

private:

	SplineGridCtrlPoint *createControlPoint(size_t x, size_t y);

	std::vector<SplineGridCtrlPoint*> ctrlp;
	unsigned deg;
	BSpline2D bsp;
};


#endif
