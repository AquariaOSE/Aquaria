#include "Interpolators.h"
#include <math.h>

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
