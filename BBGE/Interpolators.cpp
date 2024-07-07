#include "Interpolators.h"
#include <math.h>

// usually one would expect that a bspline goes from t=0 to t=1.
// here, splines eval between these -0.5 .. +0.5.
// this way 0 is perfectly in the center (which is a nice property to have)
// but more importantly the Quad::drawGrid in its default state
// has values in -0.5..+0.5 in its initial state.
// So we follow the same here, that the spline produces values
// in -0.5..+0.5 in its initial state.
static const float TMIN = -0.5f;
static const float TMAX = 0.5f;

CosineInterpolator::CosineInterpolator()
{
}

void CosineInterpolator::clear()
{
    pxy.clear();
}

void CosineInterpolator::setPoints(const Vector* p, size_t n)
{
    pxy.resize(n);
    for(size_t i = 0; i < n; ++i)
    {
        pxy[i].first  = p[i].x;
        pxy[i].second = p[i].y;
    }


    std::sort(pxy.begin(), pxy.end());
}

float CosineInterpolator::operator()(float x) const
{
    if (x < pxy[0].first)
        return pxy[0].second;

    size_t N = pxy.size() - 1;
    for (size_t i = 0; i < N; ++i)
    {
        // TODO: binary search
        if (pxy[i + 1].first > x)
        {
            float xfactor = (x - pxy[i].first) / (pxy[i + 1].first - pxy[i].first);
            float yfactor = (1.0f - cos(xfactor * 3.141596f)) * 0.5f;
            return yfactor * pxy[i + 1].second + (1 - yfactor) * pxy[i].second;
        }
    }

    return pxy[N].second;
}

void CosineInterpolator::interpolateRange(float *ys, const float* xs, size_t n)
{
    size_t i = 0, k = 0;
    for( ; k < n; ++k)
    {
        const float x = xs[k];

        while(x < pxy[i].first)
        {
            ++i;
            if(i >= pxy.size())
                goto tail;
        }

        ys[k] = pxy[i].second;
    }
    return;

tail:
    const float v = pxy.back().second;
    for( ; k < n; ++k)
        ys[k] = v;
}

BSpline2D::BSpline2D()
    : _cpx(0), _cpy(0), _degx(0), _degy(0), _tmin(0), _tmax(0)
{
}

void BSpline2D::resize(size_t cx, size_t cy, unsigned degx, unsigned degy)
{
    const float tmin = TMIN;
    const float tmax = TMAX;
    knotsX.resize(tbsp__getNumKnots(cx, degx));
    knotsY.resize(tbsp__getNumKnots(cy, degy));
    tbsp::fillKnotVector<float>(&knotsX[0], cx, degx, tmin, tmax);
    tbsp::fillKnotVector<float>(&knotsY[0], cy, degy, tmin, tmax);
    _cpx = cx;
    _cpy = cy;
    _degx = degx;
    _degy = degy;
    _tmin = tmin;
    _tmax = tmax;
}

void BSpline2D::recalc(Vector* dst, size_t xres, size_t yres, const Vector *controlpoints)
{
    const unsigned maxDeg = std::max(_degx, _degy);

    std::vector<Vector> tmpv;
    size_t tmpn = (yres * _cpx) + maxDeg;
    size_t tmpsz = tmpn * sizeof(Vector);
    Vector *tmp;
    if(tmpsz < 17*1024)
        tmp = (Vector*)alloca(tmpsz);
    else
    {
        tmpv.resize(tmpn);
        tmp = &tmpv[0];
    }
    // tmp[] layout: leftmost part: entries to hold the matrix as it's being built;
    //               rightmost part: maxDeg entries as workmem for the deBoor eval
    Vector *work = tmp + (tmpn - maxDeg);

    // Each column -> Y-axis interpolation
    for(size_t x = 0; x < _cpx; ++x)
    {
        const Vector *srccol = &controlpoints[x];
        Vector *dstcol = &tmp[x];
        tbsp::evalRange(dstcol, yres, &work[0], &knotsY[0], srccol, _cpy, _degy, _tmin, _tmax, _cpx, _cpx);
    }

    // Each row -> X-axis interpolation
    for(size_t y = 0; y < yres; ++y)
    {
        const Vector *srcrow = &tmp[y * _cpx];
        tbsp::evalRange(dst, xres, &work[0], &knotsX[0], srcrow, _cpx, _degx, _tmin, _tmax);
        dst += xres;
    }
}

// TODO: add minx/y, maxx/y params?
// this should NOT be tmin?! probably?
void BSpline2D::reset(Vector* controlpoints)
{
    const float dx = (_tmax - _tmin) / float(_cpx - 1);
    const float dy = (_tmax - _tmin) / float(_cpy - 1);
    float yy = _tmin;
    for(size_t y = 0; y < _cpy; ++y, yy += dy)
    {
        float xx = _tmin;
        for(size_t x = 0; x < _cpx; ++x, xx += dx)
            *controlpoints++ = Vector(xx, yy);
    }
}

void BSpline2DWithPoints::resize(size_t cx, size_t cy, unsigned degx, unsigned degy)
{
    controlpoints.resize(cx * cy);
    BSpline2D::resize(cx, cy, degx, degy);
}

void BSpline2DWithPoints::recalc(Vector* dst, size_t xres, size_t yres)
{
    BSpline2D::recalc(dst, xres, yres, &controlpoints[0]);
}

void BSpline2DWithPoints::reset()
{
    BSpline2D::reset(&controlpoints[0]);
}

BSpline2DControlPointGenerator::BSpline2DControlPointGenerator(size_t cx, size_t cy)
{
    const size_t interpStorageSizeX = tbsp__getInterpolatorStorageSize(cx, cx);
    const size_t interpStorageSizeY = tbsp__getInterpolatorStorageSize(cy, cy);
    const size_t interpStorageNeeded = interpStorageSizeX + interpStorageSizeY;
    floats.resize(interpStorageNeeded);

    interp.x.init(&floats[0], cx, cx);
    interp.y.init(&floats[interpStorageSizeX], cy, cy);
    cp2d.init(cx, cy);

    const size_t maxcp = std::max(cx, cy);
    vectmp.resize(maxcp);
}

void BSpline2DControlPointGenerator::refresh(const float* knotsx, const float* knotsy, unsigned degx, unsigned degy)
{
    const size_t maxcp = vectmp.size();
    const size_t tmpn = tbsp__getInterpolatorRefreshTempSize(maxcp, maxcp);
    const size_t tmpsz = tmpn * sizeof(float);
    float *tmp;
    std::vector<float> tmpv;
    if(tmpsz < 17*1024)
        tmp = (float*)alloca(tmpsz);
    else
    {
        tmpv.resize(tmpn);
        tmp = &tmpv[0];
    }

    interp.x.refresh(tmp, knotsx, degx);
    interp.y.refresh(tmp, knotsy, degy);
}

Vector* BSpline2DControlPointGenerator::generateControlPoints(const Vector *points2d)
{
    const size_t cpx = interp.x.getNumInputPoints();
    const size_t cpy = interp.x.getNumInputPoints();

    // y direction first
    for(size_t x = 0; x < cpx; ++x)
    {
        const Vector *src = &points2d[x];
        for(size_t y = 0; y < cpy; ++y, src += cpx)
            vectmp[y] = *src;

        // solve in-place
        interp.y.generateControlPoints<Vector>(&vectmp[0], NULL, &vectmp[0]);

        for(size_t y = 0; y < cpy; ++y)
            cp2d(x, y) = vectmp[y];
    }

    // x direction
    for(size_t y = 0; y < cpy; ++y)
    {
        Vector *row = cp2d.row(y);
        // solve in-place
        interp.x.generateControlPoints<Vector>(row, NULL, row);
    }

    return cp2d.data();
}


BSpline2DControlPointGeneratorWithPoints::BSpline2DControlPointGeneratorWithPoints(size_t cx, size_t cy)
    : BSpline2DControlPointGenerator(cx, cy)
    , designpoints(cx * cy)
{
}

Vector* BSpline2DControlPointGeneratorWithPoints::generateControlPoints()
{
    return BSpline2DControlPointGenerator::generateControlPoints(&designpoints[0]);
}
