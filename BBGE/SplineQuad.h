#ifndef BBGE_SPLINEQUAD_H
#define BBGE_SPLINEQUAD_H

#include "SpineQuad.h"
#include "tbsp.hh"


class SplineQuad : public SpineQuad
{
public:
	SplineQuad(size_t nodes, float width, float offsPerc);
	virtual ~SplineQuad();

	virtual void onUpdate(float dt) OVERRIDE;

	bool init(size_t controlpoints, bool interpolate, unsigned deg = 3);

	// read/write control points
	inline       Vector& operator()(size_t idx)       { return _ctrlp[idx]; }
	inline const Vector& operator()(size_t idx) const { return _ctrlp[idx]; }
	inline size_t numcp() const { return _ctrlp.size(); }


	void updateSpline();

protected:
	std::vector<Vector> _ctrlp; // CONTROL POINTS: x, y: position; z: width multiplier
	std::vector<float> _knots;

	unsigned _degree;

	struct InterpData
	{
		tbsp::Interpolator<float> interp;
		std::vector<float> storage;
		std::vector<float> tmp;
		std::vector<Vector> resultCtrlp;
	};
	InterpData *_idata;
};

#endif

