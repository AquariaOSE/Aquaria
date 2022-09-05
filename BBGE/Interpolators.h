#ifndef BBGE_INTERPOLATORS_H
#define BBGE_INTERPOLATORS_H

#include <algorithm> // std::pair
#include <vector>
#include "Vector.h"

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

#endif

