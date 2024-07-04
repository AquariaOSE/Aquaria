#ifndef BBGE_SPLINEGRID_H
#define BBGE_SPLINEGRID_H

#include <vector>
#include "Vector.h"
#include "glm/glm.hpp"
#include "Quad.h"
#include "Interpolators.h"


class SplineGridCtrlPoint : public Quad
{
public:
	SplineGridCtrlPoint();
	virtual ~SplineGridCtrlPoint();
	virtual void onUpdate(float dt) OVERRIDE;
	Vector getSplinePosition() const;
	void setSplinePosition(Vector pos);

	static SplineGridCtrlPoint *movingPoint;
};

class SplineGrid : public Quad
{
public:
	typedef Vector value_type;

	SplineGrid();
	virtual ~SplineGrid();

	// # of control points on each axis
	DynamicRenderGrid *resize(size_t w, size_t h, size_t xres, size_t yres, unsigned degx, unsigned degy);
	void recalc();
	void exportControlPoints(Vector *controlpoints);
	void importControlPoints(const Vector *controlpoints);
	void resetControlPoints();

	void setPointScale(const float scale);
	float getPointScale() const { return pointscale; }


	virtual void onRender(const RenderState& rs) const OVERRIDE;
	virtual void onUpdate(float dt) OVERRIDE;

	BSpline2D& getSpline() { return bsp; }
	const BSpline2D& getSpline() const { return bsp; }

	bool wasModified; // to be checked/reset by external code

private:

	SplineGridCtrlPoint *createControlPoint(size_t x, size_t y);

	std::vector<SplineGridCtrlPoint*> ctrlp;
	unsigned deg;
	BSpline2DWithPoints bsp;
	float pointscale;
};


#endif
