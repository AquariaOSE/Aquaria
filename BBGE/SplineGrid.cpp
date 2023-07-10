#include "SplineGrid.h"
#include "RenderBase.h"
#include "Core.h"
#include "RenderGrid.h"

SplineGridCtrlPoint *SplineGridCtrlPoint::movingPoint;

SplineGridCtrlPoint::SplineGridCtrlPoint()
{
    setTexture("gui/open-menu");
    setWidthHeight(16, 16);
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
    : wasModified(false), deg(0)
{
    setWidthHeight(128, 128);
    renderQuad = true;
    renderBorder = true;
}

SplineGrid::~SplineGrid()
{
}

RenderGrid *SplineGrid::resize(size_t w, size_t h, size_t xres, size_t yres, unsigned degx, unsigned degy)
{
    size_t oldcpx = bsp.ctrlX();
    size_t oldcpy = bsp.ctrlY();

    RenderGrid *ret = this->createGrid(xres, yres);

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

    recalc();

    return ret;
}

void SplineGrid::recalc()
{
    exportControlPoints(&bsp.controlpoints[0]);
    if(grid)
    {
        bsp.recalc(grid->data(), grid->width(), grid->height());
        wasModified = true;
    }
}

void SplineGrid::exportControlPoints(Vector* controlpoints)
{
    for(size_t i = 0; i < ctrlp.size(); ++i)
        controlpoints[i] = ctrlp[i]->getSplinePosition();
}

void SplineGrid::importControlPoints(const Vector* controlpoints)
{
    for(size_t i = 0; i < ctrlp.size(); ++i)
        ctrlp[i]->setSplinePosition(controlpoints[i]);
    recalc();
}


void SplineGrid::resetControlPoints()
{
    bsp.reset();
    importControlPoints(&bsp.controlpoints[0]);
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
    glColor4f(0.0f, 0.3f, 1.0f, 0.3f);

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
}


