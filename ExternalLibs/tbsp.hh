/* Tiny B-spline evaluation library

License:
Public domain, WTFPL, CC0 or your favorite permissive license; whatever is available in your country.

Dependencies:
- Requires C++98 without libc

Origin:
https://github.com/fgenesis/tinypile


--- Example usage: ---

enum { DEGREE = 3 }; // Cubic
// You can interpolate anything as long as it supports element addition and scalar multiplication
struct Point { ... operator+(Point) and operator*(float) overloaded ... };
const Point ps[N] = {...}; // Control points for the spline

float knots[tbsp__getNumKnots(N, DEGREE)]; // knot vector; used for evaluating the spline
Point tmp[DEGREE]; // Temporary working memory must be provided by the caller.
                   // This is just a tiny array. Must have as many elements as the degree of the spline.

// This must be done once for each B-spline; the spline is then defined by the knot vector.
// In particular, this inits a knot vector with end points [L..R],
// ie. the spline will interpolate values for t = [L..R].
// (You can use any boundary values, eg. [-4..+5], but [0..1] is the most common)
tbsp::fillKnotVector(knots, N, DEGREE, L, R);

// Evaluate the spline at point t
// Returns ps[0] if t <= L; ps[N-1] if t >= R; otherwise an interpolated point
Point p = tbsp::evalOne(tmp, knots, ps, N, DEGREE, t);

// Evaluate A points between t=0.2 .. t=0.5, equidistantly spaced, and write to a[].
// (If you have multiple points to evaluate, this is faster than multiple evalOne() calls)
Point a[A];
tbsp::evalRange(out, A, tmp, knots, ps, N, DEGREE, 0.2f, 0.5f);
*/

#pragma once

#include <stddef.h> // size_t


#ifndef TBSP_ASSERT
#  include <assert.h>
#  define TBSP_ASSERT(x) assert(x)
#endif

// ---- B-Spline eval part begin ----

namespace tbsp {

// These should be constexpr, but we want to stay C++98-compatible
#define tbsp__getNumKnots(points, degree) ((points) + (degree) + 1)
#define tbsp__getKnotVectorAllocSize(K, points, degree) (sizeof(K) * tbsp__getNumKnots((points), (degree)))

namespace detail {

// returns index of first element strictly less than t
template<typename K>
static size_t findKnotIndexOffs(K val, const K *p, size_t n)
{
    // Binary search to find leftmost element that is < val
    size_t L = 0;
    size_t R = n;
    size_t m;
    while(L < R)
    {
        m = (L + R) / 2u;
        if(p[m] < val)
            L = m + 1;
        else
            R = m;
    }
    return L;
}

template<typename K>
static inline size_t findKnotIndex(K val, const K *knots, size_t n, size_t degree)
{
    TBSP_ASSERT(n > degree);
    TBSP_ASSERT(val < knots[n - degree - 1]); // beyond right end? should have been caught by caller

    // skip endpoints
    return degree + findKnotIndexOffs(val, knots + degree, n - degree);
}

template<typename K>
static void genKnotsUniform(K *knots, size_t nn, K mink, K maxk)
{
    const K m = (maxk - mink) / K(nn + 1);
    for(size_t i = 0; i < nn; ++i)
        knots[i] = mink + K(i+1) * m;
}

template<typename K, typename T>
static T deBoor(T *work, const T *src, const K *knots, const size_t r, const size_t k, const K t, size_t inputStride)
{
    T last = src[0]; // init so that it works correctly even with degree == 0
    for(size_t worksize = k; worksize > 1; --worksize)
    {
        const size_t j = k - worksize + 1; // iteration number, starting with 1, going up to k
        const size_t tmp = r - k + 1 + j;
        for(size_t w = 0, wr = 0; w < worksize - 1; ++w, wr += inputStride)
        {
            const size_t i = w + tmp;
            const K ki = knots[i];
            TBSP_ASSERT(ki <= t);
            const K div = knots[i+k-j] - ki;
            TBSP_ASSERT(div > 0);
            const K a = (t - ki) / div;
            const K a1 = K(1) - a;
            work[w] = last = (src[wr] * a1) + (src[wr + inputStride] * a); // lerp
        }
        src = work; // done writing the initial data to work, now use that as input for further iterations
        inputStride = 1;
    }
    return last;
}

} // end namespace detail
//--------------------------------------

template<typename K>
static size_t fillKnotVector(K *knots, size_t points, size_t degree, K mink, K maxk)
{
    const size_t n = points - 1;
    if(n < degree) // lower degree if not enough points
        degree = n;
    TBSP_ASSERT(n >= degree);

    const size_t ep = degree + 1; // ep knots on each end
    const size_t ne = n - degree; // non-endpoint knots in the middle

    // endpoint interpolation, beginning
    for(size_t i = 0; i < ep; ++i)
        *knots++ = mink;

    // TODO: allow more parametrizations
    detail::genKnotsUniform(knots, ne, mink, maxk);
    knots += ne;

    // endpoint interpolation, end
    for(size_t i = 0; i < ep; ++i)
        *knots++ = maxk;

    return degree;
}

// evaluate single point at t
template<typename K, typename T>
static T evalOne(T *work, const K *knots, const T *points, size_t numpoints, size_t degree, K t)
{
    if(t < knots[0])
        return points[0]; // left out-of-bounds

    if(numpoints - 1 < degree)
        degree = numpoints - 1;

    const size_t numknots = tbsp__getNumKnots(numpoints, degree);
    const K maxknot = knots[numknots - 1];
    if(t < maxknot)
    {
        const size_t r = detail::findKnotIndex(t, knots, numknots, degree);
        TBSP_ASSERT(r >= degree);
        const size_t k = degree + 1;
        TBSP_ASSERT(r + k < numknots); // check that the copy below stays in bounds

        const T* const src = &points[r - degree];
        return detail::deBoor(work, src, knots, r, k, t);
    }

    return points[numpoints - 1]; // right out-of-bounds
}

// evaluate numdst points in range [tmin..tmax], equally spaced
template<typename K, typename T>
static void evalRange(T *dst, size_t numdst, T *work, const K *knots, const T *points, size_t numpoints, size_t degree, K tmin, K tmax, size_t inputStride = 1, size_t outputStride = 1)
{
    TBSP_ASSERT(tmin <= tmax);
    if(numpoints - 1 < degree)
        degree = numpoints - 1;

    const size_t numknots = tbsp__getNumKnots(numpoints, degree);
    size_t r = detail::findKnotIndex(tmin, knots, numknots, degree);
    TBSP_ASSERT(r >= degree);
    const size_t k = degree + 1;
    TBSP_ASSERT(r + k < numknots); // check that the copy below stays in bounds

    const K step = (tmax - tmin) / K(numdst - 1);
    K t = tmin;
    const size_t maxidx = numknots - k;


    size_t i = 0;

    // left out-of-bounds
    for( ; i < numdst && t < knots[0]; ++i, t += step)
    {
        *dst = points[0];
        dst += outputStride;
    }

    // actually interpolated points
    const K maxknot = knots[numknots - 1];
    for( ; i < numdst && t < maxknot; ++i, t += step)
    {
        while(r < maxidx && knots[r+1] < t) // find new index; don't need to do binary search again
            ++r;

        const T* const src = &points[(r - degree) * inputStride];
        *dst = detail::deBoor(work, src, knots, r, k, t, inputStride);
        dst += outputStride;
    }

    // right out-of-bounds
    if(i < numdst)
    {
        T last =  points[(numpoints - 1) * inputStride];
        for( ; i < numdst; ++i)
        {
            *dst = last;
            dst += outputStride;
        }
    }
}


} // end namespace tbsp
