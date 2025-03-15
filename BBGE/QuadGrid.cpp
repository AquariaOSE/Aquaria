#include "QuadGrid.h"
#include "Texture.h"
#include "RenderBase.h"
#include "Core.h"

QuadGrid::QuadGrid(size_t w, size_t h)
    : pauseLevel(0), _w(w+1), _h(h+1)
    , dirty(true)
    , vbo(GPUBUF_DYNAMIC | GPUBUF_VERTEXBUF)
    , ibo(GPUBUF_STATIC | GPUBUF_INDEXBUF)
    , _points((w+1) * (h+1))
{
    addType(SCO_QUAD_GRID);
    resetUV();
    resetPos(1, 1);
    this->width = 2;
    this->height = 2;
    this->cull = false;
    this->repeatTexture = true;
    _numtris = ibo.initGridIndices_Triangles(w+1, h+1, false, GPUACCESS_DEFAULT);
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

    dirty = true;
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

    dirty = true;
}

void QuadGrid::onRender(const RenderState& rs) const
{
    glColor4f(color.x, color.y, color.z, alpha.x * alphaMod);

    vbo.apply();
    ibo.drawElements(GL_TRIANGLES, _numtris);
}

void QuadGrid::onUpdate(float dt)
{
    const bool interp = texOffset.isInterpolating();
    if(!(pauseLevel < core->particlesPaused))
    {
        texOffset.update(dt);
        RenderObject::onUpdate(dt);
    }

    if(dirty || interp)
        updateVBO();
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

void QuadGrid::updateVBO()
{
    const size_t bytes = _points.size() * sizeof(Point);

    do
    {
        const Point *s = &_points[0];
        Point *p = (Point*)vbo.beginWrite(GPUBUFTYPE_VEC2_TC, bytes, GPUACCESS_DEFAULT);

        for(size_t i = 0; i < _points.size(); ++i, ++p, ++s)
        {
            p->x = s->x;
            p->y = s->y;
            p->u = s->u + texOffset.x;
            p->v = s->v + texOffset.y;
        }
    }
    while(!vbo.commitWrite());
    dirty = false;
}
