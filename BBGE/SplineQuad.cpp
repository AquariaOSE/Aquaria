// Line SpineQuad, but spine points are smoothly generated from control points

#include "SplineQuad.h"


SplineQuad::SplineQuad(size_t nodes, float width, float offsPerc)
	: SpineQuad(nodes, width, offsPerc)
	, _degree(0)
	, _idata(NULL)
{
	addType(SCO_SPLINEQUAD);
	_ctrlp.resize(nodes);
}

SplineQuad::~SplineQuad()
{
	delete _idata;
}

void SplineQuad::onUpdate(float dt)
{
	updateSpline();
	SpineQuad::onUpdate(dt);
}


bool SplineQuad::init(size_t controlpoints, bool interpolate, unsigned deg)
{
	bool ok = true;
	_degree = deg;
	_ctrlp.resize(controlpoints);

	if(deg > 0)
	{
		size_t k = tbsp__getNumKnots(controlpoints, deg);
		_knots.resize(k);
		tbsp::fillKnotVector(&_knots[0], controlpoints, deg, 0.0f, 1.0f);

		if(interpolate && !_idata)
			_idata = new InterpData;
		else if(!interpolate && _idata)
		{
			delete _idata;
			_idata = NULL;
		}

		if(_idata)
		{
			const size_t np = _points.size();
			const size_t ssz = tbsp__getInterpolatorStorageSize(controlpoints, np);
			const size_t tsz = tbsp__getInterpolatorRefreshTempSize(controlpoints, np);
			_idata->storage.resize(ssz);
			_idata->tmp.resize(tsz);
			_idata->resultCtrlp.resize(controlpoints);
			ok = _idata->interp.init(&_idata->storage[0], np, controlpoints) && ok;
			ok = _idata->interp.refresh(tsz ? &_idata->tmp[0] : NULL, &_knots[0], deg) && ok;
		}
	}

	return ok;
}

void SplineQuad::updateSpline()
{
	if(_degree > 0)
	{
		Vector *cp = &_ctrlp[0];
		size_t numcp = _ctrlp.size();

		// If interpolating the actually used control points, do that first. This is expensive.
		if(_idata)
		{
			cp = &_idata->resultCtrlp[0];
			numcp = _idata->interp.generateControlPoints(cp, (Vector*)NULL, &_ctrlp[0]);
		}

		Vector *work = (Vector*)alloca(_degree);
		tbsp::evalRange(&_points[0], _points.size(), work, &_knots[0], cp, numcp, _degree, 0.0f, 1.0f);
	}
	else
	{
		size_t sz = std::min(_points.size(), _ctrlp.size());
		memcpy(&_points[0], &_ctrlp[0], sz * sizeof(Vector));
	}
	updateVBO();
}
