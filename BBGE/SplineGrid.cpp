#include "SplineGrid.h"
#include "RenderBase.h"
#include "Core.h"

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
    return Vector(position.x / p->width, position.y / p->height, 1.0f);
}

void SplineGridCtrlPoint::onUpdate(float dt)
{
    const bool lmb = core->mouse.buttons.left;

    if(lmb)
    {
        if(!movingPoint && isCoordinateInside(core->mouse.position))
            movingPoint = this;
    }
    else
        movingPoint = NULL;

    if(movingPoint == this)
    {
        SplineGrid *p = (SplineGrid*)getParent();
        const Vector parentPos = p->getWorldPosition();
        position = core->mouse.position - parentPos;
        p->recalc();
    }
}

SplineGrid::SplineGrid()
    : deg(0)
{
    setWidthHeight(128, 128);
    renderQuad = true;
    renderBorder = true;
}

SplineGrid::~SplineGrid()
{
}

void SplineGrid::resize(size_t w, size_t h, size_t xres, size_t yres, unsigned deg)
{
    size_t oldcpx = bsp.ctrlX();
    size_t oldcpy = bsp.ctrlY();

    this->createGrid(xres, yres);

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

    bsp.resize(w, h, deg, deg, -1.0f, 1.0f);

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
}

void SplineGrid::recalc()
{
    for(size_t i = 0; i < ctrlp.size(); ++i)
        bsp.controlpoints[i] = ctrlp[i]->getSplinePosition();
    bsp.recalc(drawGrid.data(), drawGrid.width(), drawGrid.height());

}


void SplineGrid::resetControlPoints()
{
    const size_t cpx = bsp.ctrlX();
    const size_t cpy = bsp.ctrlY();
    const float dx = width / float(cpx - 1);
    const float dy = height / float(cpy - 1);
    float yy = height * -0.5f;
    for(size_t y = 0; y < cpy; ++y, yy += dy)
    {
        float xx = width * -0.5f;
        SplineGridCtrlPoint **row = &ctrlp[y * cpx];
        for(size_t x = 0; x < cpx; ++x, xx += dx)
            row[x]->position = Vector(xx, yy);
    }
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
    glColor4f(0.0f, 1.0f, 0.3f, 0.3f);

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


