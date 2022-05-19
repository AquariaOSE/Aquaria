#include "QuadGrid.h"
#include "Texture.h"
#include "RenderBase.h"
#include "Core.h"

QuadGrid::QuadGrid(size_t w, size_t h)
    : pauseLevel(0), _w(w+1), _h(h+1)
{
    addType(SCO_QUAD_GRID);
    _points.resize((w+1) * (h+1));
    resetUV();
    resetPos(1, 1);
    this->width = 2;
    this->height = 2;
    this->cull = false;
    this->repeatTexture = true;
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
    const float incX = xmul / float(quadsX());
    const float incY = ymul / float(quadsY());
    const size_t NX = pointsX();

    float v = 0;

    // go over points
    for(size_t y = 0; y < _h; ++y)
    {
        Point *row = &_points[y * NX];
        float u = 0;
        for(size_t x = 0; x < NX; ++x)
        {
            row[x].u = u;
            row[x].v = v;
            u += incX;
        }
        v += incY;
    }
}

void QuadGrid::resetPos(float w, float h, float xoffs, float yoffs)
{
    const float dx = w / float(quadsX());
    const float dy = h / float(quadsY());
    const size_t NX = pointsX();

    float yy = yoffs;
    // go over points
    for(size_t y = 0; y < _h; ++y, yy +=  dy)
    {
        Point * const row = &_points[y * NX];
        float xx = xoffs;
        for(size_t x = 0; x < NX; ++x, xx += dx)
        {
            row[x].x = xx;
            row[x].y = yy;
        }
    }
}

static inline void drawOnePoint(const QuadGrid::Point& p, float ox, float oy)
{
    glTexCoord2f(p.u + ox, p.v + oy);
    glVertex2f(p.x, p.y);
}


void QuadGrid::onRender(const RenderState& rs) const
{
    glColor4f(color.x, color.y, color.z, alpha.x * alphaMod);

    const float ox = texOffset.x;
    const float oy = texOffset.y;
    const size_t NX = pointsX();

    // go over grids
    const size_t W = quadsX();
    const size_t H = quadsY();
    for(size_t y = 0; y < H; ++y)
    {
        const Point * const row0 = &_points[y * NX];
        const Point * const row1 = row0 + NX;
        for(size_t x = 0; x < W; ++x)
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

void QuadGrid::onSetTexture() // same as Quad::setTexture()
{
    if (texture)
    {
        width = this->texture->width;
        height = this->texture->height;
    }
    else
    {
        width = 64;
        height = 64;
    }
}
