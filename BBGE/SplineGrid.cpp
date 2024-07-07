#include "SplineGrid.h"

#include <assert.h>

#include "RenderBase.h"
#include "Core.h"
#include "RenderGrid.h"
#include "SkeletalSprite.h"


SplineGridCtrlPoint *SplineGridCtrlPoint::movingPoint;

SplineGridCtrlPoint::SplineGridCtrlPoint()
{
    setTexture("gui/open-menu");
    setWidthHeight(8, 8);
}

SplineGridCtrlPoint::~SplineGridCtrlPoint()
{
}

Vector SplineGridCtrlPoint::getSplinePosition() const
{
    SplineGridCtrlPoint *p = (SplineGridCtrlPoint*)getParent();
    // always return alpha == 1
    // points within the quad result in in -0.5 .. +0.5 on both axes
    return Vector(position.x / p->width, position.y / p->height, 1.0f);
}

void SplineGridCtrlPoint::setSplinePosition(Vector pos)
{
    SplineGridCtrlPoint *p = (SplineGridCtrlPoint*)getParent();
    position.x = pos.x * p->width;
    position.y = pos.y * p->height;
}

void SplineGridCtrlPoint::onUpdate(float dt)
{
    const bool lmb = core->mouse.buttons.left;

    // FIXME: selected point tracking should be done by the parent

    Vector cur = core->mouse.position;
    // bool wouldpick = isCoordinateInside(cur); // doesn't work
    Vector wp = getWorldPosition();
    const bool wouldpick = (cur - wp).isLength2DIn(10);

    if(wouldpick)
        color = Vector(1,0,0);
    else
        color = Vector(1,1,1);

    if(lmb)
    {
        if(!movingPoint && wouldpick)
        {
            movingPoint = this;
        }
    }
    else if(movingPoint)
    {
        movingPoint->color = Vector(1,1,1);
        movingPoint = NULL;
    }

    if(movingPoint == this)
    {
        SplineGrid *p = (SplineGrid*)getParent();
        const Vector parentPos = p->getWorldPosition();
        const Vector invscale = Vector(1.0f / p->scale.x, 1.0f / p->scale.y);
        Vector newpos = (cur - parentPos) * invscale;
        if(position != newpos)
        {
            position = newpos;
            p->recalc();
        }
    }
}

SplineGrid::SplineGrid()
    : wasModified(false), deg(0), pointscale(1), _assistMode(true)
{
    setWidthHeight(128, 128);
    renderQuad = true;
    renderBorder = true;
    renderBorderColor = Vector(0.5f, 0.5f, 0.5f);
}

SplineGrid::~SplineGrid()
{
}

DynamicRenderGrid *SplineGrid::resize(size_t w, size_t h, size_t xres, size_t yres, unsigned degx, unsigned degy)
{
    if(!cpgen.resize(w, h))
        return NULL;

    size_t oldcpx = bsp.ctrlX();
    size_t oldcpy = bsp.ctrlY();

    DynamicRenderGrid *ret = this->createGrid(xres, yres);
    ret->gridType = GRID_INTERP;

    std::vector<SplineGridCtrlPoint*> oldp;
    ctrlp.swap(oldp);
    ctrlp.resize(w * h);

    // move any old points over that fit within the new size
    {
        const size_t cw = std::min(oldcpx, w);
        const size_t ch = std::min(oldcpy, h);
        for(size_t y = 0; y < ch; ++y)
            for(size_t x = 0; x < cw; ++x)
            {
                SplineGridCtrlPoint *& ref = oldp[y * oldcpx + x];
                ctrlp[y * w + x] = ref;
                ref = NULL;
            }
    }

    bsp.resize(w, h, degx, degy);

    // kill any excess points
    for(size_t i = 0; i < oldp.size(); ++i)
        if(oldp[i])
            oldp[i]->safeKill();

    // extend until all points are there
    for(size_t y = 0; y < h; ++y)
        for(size_t x = 0; x < w; ++x)
        {
            SplineGridCtrlPoint *& ref = ctrlp[y * w + x];
            if(!ref)
                ref = createControlPoint(x, y);
        }

    if(!cpgen.refresh(bsp.getKnotsX(), bsp.getKnotsY(), bsp.degX(), bsp.degY()))
        return NULL;

    recalc();

    return ret;
}

void SplineGrid::recalc()
{
    if(_assistMode)
    {
        exportGridPoints(&cpgen.designpoints[0]);
        _generateControlPointsFromDesignPoints();
    }
    else
    {
        exportGridPoints(&bsp.controlpoints[0]);
        bsp.recalc(&cpgen.designpoints[0], bsp.ctrlX(), bsp.ctrlY());
    }

    if(grid)
    {
        bsp.recalc(grid->dataRW(), grid->width(), grid->height());
        wasModified = true;
    }
}

void SplineGrid::exportGridPoints(Vector* pdst) const
{
    for(size_t i = 0; i < ctrlp.size(); ++i)
        pdst[i] = ctrlp[i]->getSplinePosition();
}

void SplineGrid::importGridPoints(const Vector* psrc)
{
    for(size_t i = 0; i < ctrlp.size(); ++i)
        ctrlp[i]->setSplinePosition(psrc[i]);
}

void SplineGrid::importKeyframe(const BoneKeyframe* bk)
{
    const size_t numcp = bsp.ctrlX() * bsp.ctrlY();
    assert(bk->controlpoints.size() == numcp);

    bsp.controlpoints = bk->controlpoints;

    if(_assistMode)
    {
        // given control points, generate spline points (which are later caculated back into control points)
        bsp.recalc(&cpgen.designpoints[0], bsp.ctrlX(), bsp.ctrlY());
        importGridPoints(&cpgen.designpoints[0]);
    }
    else
        importGridPoints(&bk->controlpoints[0]);

    recalc();
}

void SplineGrid::exportKeyframe(BoneKeyframe* bk) const
{
    const size_t numcp = bsp.ctrlX() * bsp.ctrlY();
    assert(bk->controlpoints.size() == numcp);

    bk->controlpoints = bsp.controlpoints;
}

void SplineGrid::resetControlPoints()
{
    bsp.reset();

    importGridPoints(&bsp.controlpoints[0]);

    // This pushes the bspline controlpoints outwards so that all spline points line up as one would expect.
    // If this weren't done, the tile's texture would be pulled inwards (more with increasing dimension);
    // as if the tile was a piece of plastic foil that's seen too much heat.
    //if(_assistMode) // ALWAYS DO THIS!!
    {
        cpgen.designpoints = bsp.controlpoints;
        _generateControlPointsFromDesignPoints();
    }

    recalc();
}

void SplineGrid::_generateControlPointsFromDesignPoints()
{
    const Vector *cp = cpgen.generateControlPoints();
    memcpy(&bsp.controlpoints[0], cp, bsp.controlpoints.size() * sizeof(*cp));
}

SplineGridCtrlPoint* SplineGrid::createControlPoint(size_t x, size_t y)
{
    const size_t cpx = bsp.ctrlX();
    const size_t cpy = bsp.ctrlY();
    assert(x < cpx && y < cpy);
    const Vector wh(width, height);
    const Vector pos01(float(x) / float(cpx-1), float(y) / float(cpy-1));
    SplineGridCtrlPoint *cp = new SplineGridCtrlPoint();
    cp->position = (pos01 - Vector(0.5f, 0.5f)) * wh;
    cp->scale.x = pointscale;
    cp->scale.y = pointscale;
    this->addChild(cp, PM_POINTER);
    return cp;
}

void SplineGrid::onUpdate(float dt)
{
    Quad::onUpdate(dt);
}

void SplineGrid::onRender(const RenderState& rs) const
{
    Quad::onRender(rs);

    glBindTexture(GL_TEXTURE_2D, 0);

    const Vector wh2(width * 0.5f, height * 0.5f);

    glLineWidth(2);
    if(_assistMode)
        glColor4f(0.0f, 0.3f, 1.0f, 0.4f);
    else
        glColor4f(0.0f, 0.0f, 0.0f, 0.4f);

    const size_t cpx = bsp.ctrlX();
    const size_t cpy = bsp.ctrlY();

    // X axis
    for(size_t y = 0; y < cpy; ++y)
    {
        glBegin(GL_LINE_STRIP);
        const SplineGridCtrlPoint * const *row = &ctrlp[y * cpx];
        for(size_t x = 0; x < cpx; ++x)
        {
            const Vector p = row[x]->position;
            glVertex2f(p.x, p.y);
        }
        glEnd();
    }

    // Y axis
    for(size_t x = 0; x < cpx; ++x)
    {
        glBegin(GL_LINE_STRIP);
        for(size_t y = 0; y < cpy; ++y)
        {
            const Vector p = ctrlp[y * cpx + x]->position;
            glVertex2f(p.x, p.y);
        }
        glEnd();
    }

    const Vector *psrc = _assistMode
        ? &bsp.controlpoints[0]
        : &cpgen.designpoints[0];

    if(RenderObject::renderCollisionShape)
    {
        glLineWidth(1);
        glColor4f(1.0f, 0.3f, 0.3f, 0.7f);
        glPushMatrix();
        glScalef(width, height, 1);

        // X axis
        for(size_t y = 0; y < cpy; ++y)
        {
            glBegin(GL_LINE_STRIP);
            const Vector *row = &psrc[y * cpx];
            for(size_t x = 0; x < cpx; ++x)
            {
                const Vector p = row[x];
                glVertex2f(p.x, p.y);
            }
            glEnd();
        }

        // Y axis
        for(size_t x = 0; x < cpx; ++x)
        {
            glBegin(GL_LINE_STRIP);
            for(size_t y = 0; y < cpy; ++y)
            {
                const Vector p = psrc[y * cpx + x];
                glVertex2f(p.x, p.y);
            }
            glEnd();
        }

        glPopMatrix();
    }
}

void SplineGrid::setPointScale(const float scale)
{
    pointscale = scale;
    for(size_t i = 0; i < ctrlp.size(); ++i)
    {
        ctrlp[i]->scale.x = scale;
        ctrlp[i]->scale.y = scale;
    }
}

void SplineGrid::setAssist(bool on)
{
    if(on == _assistMode)
        return;

    if(on)
        importGridPoints(&cpgen.designpoints[0]);
    else
        importGridPoints(&bsp.controlpoints[0]);

    _assistMode = on;
    recalc();
}
