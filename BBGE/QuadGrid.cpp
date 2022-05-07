#include "QuadGrid.h"
#include "Texture.h"
#include "RenderBase.h"
#include "Core.h"

QuadGrid::QuadGrid(size_t w, size_t h)
    : pauseLevel(0), _w(w), _h(h)
{
    addType(SCO_QUAD_GRID);
    _points.resize((w+1) * (h+1));
    resetUV();
}

QuadGrid* QuadGrid::New(size_t w, size_t h)
{
    return w && h ? new QuadGrid(w, h) : NULL;
}

QuadGrid::~QuadGrid()
{
}

void QuadGrid::resetUV(float xmul, float ymul)
{
    const float incX = 1.0f / float(_w + 1);
    const float incY = 1.0f / float(_h + 1);

    float u0 = 0, u1 = incX, v0 = 0, v1 = incY;

    // go over points, so <= to compare boundaries
    for(size_t y = 0; y <= _h; ++y, v0 = v1, v1 += incY)
    {
        Point *row = &_points[y * _w];
        for(size_t x = 0; x <= _w; ++x, u0 = u1, u1 += incX)
        {
            row[x].u = u0;
            row[y].v = v0;
        }
    }
}

static inline void drawOnePoint(const QuadGrid::Point& p, float ox, float oy)
{
    glTexCoord2f(p.u + ox, p.v + oy);
    glVertex2f(p.x, p.y);
}


void QuadGrid::onRender()
{
    glBindTexture(GL_TEXTURE_2D, texture->textures[0]);
    glColor4f(color.x, color.y, color.z, alpha.x * alphaMod);

    const float ox = texOffset.x;
    const float oy = texOffset.y;

    const size_t NX = _w + 1;

    // go over grids, so < to compare boundaries
    for(size_t y = 0; y < _h; ++y)
    {
        Point * const row0 = &_points[y * NX];
        Point * const row1 = row0 + NX;
        for(size_t x = 0; x < _w; ++x)
        {
            glBegin(GL_QUADS);
            drawOnePoint(row0[x],   ox, oy);
            drawOnePoint(row0[x+1], ox, oy);
            drawOnePoint(row1[x+1], ox, oy);
            drawOnePoint(row1[x],   ox, oy);
            glEnd();
        }
    }
}

void QuadGrid::onUpdate(float dt)
{
    if(pauseLevel < core->particlesPaused)
        return;

    texOffset.update(dt);

    RenderObject::onUpdate(dt);
}

