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
    : _cpx(0), _cpy(0), _degx(0), _degy(0), _tmin(0), _tmax(0), _ext(NULL)
{
}

BSpline2D::~BSpline2D()
{
    if(_ext)
    {
        _ext->~Extended();
        free(_ext);
    }
}

BSpline2D::BSpline2D(const BSpline2D& o)
    : _cpx(o._cpx), _cpy(o._cpy), _degx(o._degx), _degy(o._degy)
    , _tmin(o._tmin), _tmax(o._tmax)
    , knotsX(o.knotsX), knotsY(o.knotsY)
    , _ext(NULL) // VERY important
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


    const size_t maxCp = std::max(cx, cy);

    const size_t interpStorageSizeX = tbsp__getInterpolatorStorageSize(cx, cx);
    const size_t interpStorageSizeY = tbsp__getInterpolatorStorageSize(cy, cy);
    const size_t interpRefreshTempSize = tbsp__getInterpolatorRefreshTempSize(maxCp, maxCp);
    const size_t interpStorageNeeded = interpStorageSizeX + interpStorageSizeY;

    if(_ext && _ext->capacity < interpStorageNeeded)
    {
        _ext->~Extended();
        free(_ext);
        _ext = NULL;
    }

    if(!_ext)
    {
        void *extmem = malloc(sizeof(Extended) + sizeof(float) * interpStorageNeeded);
        Extended *ext = new (extmem) Extended;
        ext->capacity = interpStorageNeeded;
        _ext = ext;
    }

    if(_ext)
    {
        // Some extra temp memory is required during init, but can be discarded right afterward
        std::vector<float> interptmp(interpRefreshTempSize);

        float *mx = _ext->floats();
        float *my = mx + interpStorageSizeX;

        _ext->interp.x.init(mx,  cx, cx);
        _ext->interp.x.refresh(&interptmp[0], &knotsX[0], degx);

        _ext->interp.y.init(my,  cy, cy);
        _ext->interp.y.refresh(&interptmp[0], &knotsY[0], degy);

        _ext->tmp2d.init(cx, cy);
    }
}

void BSpline2D::recalc(Vector* dst, size_t xres, size_t yres, const Vector *controlpoints)
{
    if(_ext)
    {
        const size_t maxCp = std::max(_cpx, _cpy);

        Array2d<Vector>& tmp2d = _ext->tmp2d;
        std::vector<Vector> tmpcp(maxCp), tmpin(maxCp);

        // FIXME: should have in/out stride in the generator function

        // y direction first
        for(size_t x = 0; x < _cpx; ++x)
        {
            const Vector *src = &controlpoints[x];
            for(size_t i = 0; i < _cpy; ++i, src += _cpx)
                tmpin[i] = *src;

            _ext->interp.y.generateControlPoints<Vector>(&tmpcp[0], NULL, &tmpin[0]);

            for(size_t y = 0; y < _cpy; ++y)
                tmp2d(x, y) = tmpcp[y];
        }

        // x direction
        for(size_t y = 0; y < _cpy; ++y)
        {
            Vector *row = tmp2d.row(y);
            memcpy(&tmpin[0], row, sizeof(Vector) * _cpx);
            _ext->interp.x.generateControlPoints<Vector>(row, NULL, &tmpin[0]);
        }

        controlpoints = tmp2d.data();
    }


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
