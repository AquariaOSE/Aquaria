#ifndef BBGE_INTERPOLATORS_H
#define BBGE_INTERPOLATORS_H

#include <algorithm> // std::pair
#include <vector>
#include "Vector.h"
#include "tbsp.hh"
#include "DataStructures.h"

enum SplineType
{
	INTERPOLATOR_BSPLINE,
	INTERPOLATOR_COSINE,
	INTERPOLATOR_BSPLINE_EXT
};

class CosineInterpolator
{
public:
    CosineInterpolator();
    void clear();
    void setPoints(const Vector *p, size_t n);
    float operator()(float x) const;
    void interpolateRange(float *ys, const float *xs, size_t n);

private:

    std::vector<std::pair<float, float> > pxy;
};

class BSpline2D
{
public:
    BSpline2D();

    // # of control points on each axis
    void resize(size_t cx, size_t cy, unsigned degx, unsigned degy);
    void recalc(Vector *dst, size_t xres, size_t yres, const Vector *controlpoints);

    void reset(Vector *controlpoints);

    inline size_t ctrlX() const { return _cpx; }
    inline size_t ctrlY() const { return _cpy; }

    inline unsigned degX() const { return _degx; }
    inline unsigned degY() const { return _degy; }

    inline const float *getKnotsX() const { return &knotsX[0]; }
    inline const float *getKnotsY() const { return &knotsY[0]; }

private:

    size_t _cpx, _cpy; // # of control points
    unsigned _degx, _degy;
    float _tmin, _tmax;
    std::vector<float> knotsX, knotsY;
};

class BSpline2DWithPoints : public BSpline2D
{
public:
    void resize(size_t cx, size_t cy, unsigned degx, unsigned degy);
    void recalc(Vector *dst, size_t xres, size_t yres);

    void reset();

    std::vector<Vector> controlpoints;

    inline Vector& controlpoint(size_t x, size_t y)
    {
        return controlpoints[y * ctrlX() + x];
    }
};

class BSpline2DControlPointGenerator
{
public:
    bool resize(size_t cx, size_t cy);

    bool refresh(const float *knotsx, const float *knotsy, unsigned degx, unsigned degy);

    Vector *generateControlPoints(const Vector *points2d);

private:
    Array2d<Vector> cp2d;
    struct
    {
        tbsp::Interpolator<float> x, y;
    } interp;
    std::vector<float> floats;
    std::vector<Vector> vectmp;
};

class BSpline2DControlPointGeneratorWithPoints : public BSpline2DControlPointGenerator
{
public:
    bool resize(size_t cx, size_t cy);
    Vector *generateControlPoints();
    std::vector<Vector> designpoints;
};

#endif
