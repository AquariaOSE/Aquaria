#ifndef BBGE_QUADGRID_H
#define BBGE_QUADGRID_H

#include <vector>
#include "RenderObject.h"

/*
This class is an extension for Quad, where coordinates and UV coords can be set freely.
Yes, the Quad class also has a grid function that's used for quad strips, segmented animation and so on, but this is different.

TODO:
In *most* cases a full Quad (as it's implemented right now) isn't needed.
A Quad *should* be just a dumb texture with a center point that can be slapped somewhere and scaled/rotated/etc. Like a normal map tile (Element) without any specials applied.
Only some Elements are worthy of being grid Quads -- those with a wavy tile effect applied.
Same goes for bones. Most bones don't have any strip animation so they don't need what a Quad provides.
Strip anim is just a 2xN or Nx2 grid, btw.

A future goal is to eventually rip all strip/xy-grid/segs functionality out of the Quad, make the Quad dumb,
and make this QuadGrid class derive from Quad (or probably PauseQuad)  instead of RenderObject.
The Bone and Element classes need to be fixed to accompany for this of course but
*/

class QuadGrid : public RenderObject
{
public:
    static QuadGrid *New(size_t w, size_t h);
    virtual ~QuadGrid();

    struct Point
    {
        float x, y, u, v;
    };


    inline Point& operator()(size_t x, size_t y)
    {
        return _points[y * _w + x];
    }

    inline const Point& operator()(size_t x, size_t y) const
    {
        return _points[y * _w + x];
    }

    virtual void onRender(const RenderState& rs) const OVERRIDE;
    virtual void onUpdate(float dt) OVERRIDE;
    virtual void onSetTexture() OVERRIDE;

    void resetUV(float xmul = 1, float ymul = 1);
    void resetPos(float w, float h, float xoffs = 0, float yoffs = 0);

    inline size_t quadsX() const { return _w - 1; }
    inline size_t quadsY() const { return _h - 1; }

    inline size_t pointsX() const { return _w; }
    inline size_t pointsY() const { return _h; }


public:
    InterpolatedVector texOffset;
    int pauseLevel;


private:
    QuadGrid(size_t w, size_t h);
    const size_t _w, _h; // number of points in each direction (2x3 quads => 3x4 grid points)
    std::vector<Point> _points;
};


#endif
