#include "SplineGrid.h"
#include "tbsp.hh"
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
    Vector pos = position;
    return Vector(pos.x / p->width, pos.y / p->height);
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
        SplineGridCtrlPoint *p = (SplineGridCtrlPoint*)getParent();
        const Vector parentPos = p->getWorldPosition();
        position = core->mouse.position - parentPos;
    }
}

SplineGrid::SplineGrid()
    : degreeX(3), degreeY(3), _cpx(0), _cpy(0), _xres(0), _yres(0)
{
    setWidthHeight(128, 128);
    //renderQuad = false;
}

SplineGrid::~SplineGrid()
{
}

void SplineGrid::resize(size_t w, size_t h, size_t xres, size_t yres)
{

    knotsX.resize(tbsp__getNumKnots(w, degreeX));
    knotsY.resize(tbsp__getNumKnots(h, degreeY));
    tmp.resize(xres * h);
    tbsp::fillKnotVector<float>(&knotsX[0], w, degreeX, 0.0f, 1.0f);
    tbsp::fillKnotVector<float>(&knotsY[0], h, degreeY, 0.0f, 1.0f);

    this->createGrid(xres, yres);

    std::vector<SplineGridCtrlPoint*> oldp;
    ctrlp.swap(oldp);
    ctrlp.resize(w * h);

    // move any old points over that fit within the new size
    {
        const size_t cw = std::min(_cpx, w);
        const size_t ch = std::min(_cpy, h);
        for(size_t y = 0; y < ch; ++y)
            for(size_t x = 0; x < cw; ++x)
            {
                SplineGridCtrlPoint *& ref = oldp[y * _cpx + x];
                ctrlp[y * w + x] = ref;
                ref = NULL;
            }
    }

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

    _cpx = w;
    _cpy = h;
    _xres = xres;
    _yres = yres;
}

void SplineGrid::recalc()
{
    std::vector<Vector> px(_cpx);
    std::vector<Vector> work(std::max(degreeX, degreeY));
    for(size_t y = 0; y < _cpy; ++y)
    {
        for(size_t x = 0; x < _cpx; ++x)
            px[x] = ctrlp[y * _cpx + x]->getSplinePosition();

        Vector *dst = &tmp[y * _xres];
        tbsp::evalRange<float, Vector>(dst, _xres, &work[0], &knotsX[0], &px[0], px.size(), degreeX, 0.0f, 1.0f);
    }
}

void SplineGrid::resetControlPoints()
{
    const float dx = width / float(_cpx + 1);
    const float dy = height / float(_cpy + 1);
    float yy = height * -0.5f;
    for(size_t y = 0; y < _cpy; ++y, yy += dy)
    {
        float xx = width * -0.5f;
        SplineGridCtrlPoint **row = &ctrlp[y * _cpx];
        for(size_t x = 0; x < _cpx; ++x, xx += dx)
            row[x]->position = Vector(xx, yy);
    }
}

SplineGridCtrlPoint* SplineGrid::createControlPoint(size_t x, size_t y)
{
    assert(x < _cpx && y < _cpy);
    const float w2 = width * -0.5f;
    const float h2 = height * -0.5f;
    SplineGridCtrlPoint *cp = new SplineGridCtrlPoint();
    cp->position.x = w2 + float(x) / float(_cpx + 1);
    cp->position.y = h2 + float(y) / float(_cpy + 1);
    this->addChild(cp, PM_POINTER);
    return cp;
}

void SplineGrid::onUpdate(float dt)
{
    Quad::onUpdate(dt);

    recalc(); // HACK
}

void SplineGrid::onRender(const RenderState& rs) const
{
    Quad::onRender(rs);

    glBindTexture(GL_TEXTURE_2D, 0);

    const Vector wh(width, height);

    glLineWidth(2);
    glColor4f(0.0f, 1.0f, 0.3f, 0.3f);

    // X axis
    for(size_t y = 0; y < _cpy; ++y)
    {
        glBegin(GL_LINE_STRIP);
        const SplineGridCtrlPoint * const *row = &ctrlp[y * _cpx];
        for(size_t x = 0; x < _cpx; ++x)
        {
            const Vector p = row[x]->position;
            glVertex2f(p.x, p.y);
        }
        glEnd();
    }

    // Y axis
    for(size_t x = 0; x < _cpx; ++x)
    {
        glBegin(GL_LINE_STRIP);
        for(size_t y = 0; y < _cpy; ++y)
        {
            const Vector p = ctrlp[y * _cpx + x]->position;
            glVertex2f(p.x, p.y);
        }
        glEnd();
    }




    glColor4f(0.0f, 0.2f, 1.0f, 1.0f);
	glPointSize(3);
	glBegin(GL_POINTS);

    for(size_t y = 0; y < _cpy; ++y)
    {
        const value_type *line = &tmp[y * _xres];
        for(size_t x = 0; x < _xres; ++x)
        {
            Vector pos = line[x] * wh;
            glVertex2f(pos.x, pos.y);
        }
    }
    glEnd();
}


