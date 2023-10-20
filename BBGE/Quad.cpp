/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "Quad.h"
#include "Core.h"
#include "RenderBase.h"
#include "RenderGrid.h"
#include <assert.h>

Quad::Quad(const std::string &tex, const Vector &pos)
: RenderObject()
{
	initQuad();
	renderBorderColor = Vector(1,1,1);
	position = pos;
	setTexture(tex);
}

Quad::Quad() : RenderObject()
{
	initQuad();
}

Quad::~Quad()
{
	deleteGrid();
}

void Quad::initQuad()
{
	addType(SCO_QUAD);
	borderAlpha = 0.5;
	repeatToFillScale = Vector(1,1);

	autoWidth = autoHeight = 0;

	renderBorder = false;
	renderCenter = true;
	width = 2; height = 2;
	texcoords.setStandard();

	renderQuad = true;
	grid = NULL;
}

void Quad::deleteGrid()
{
	delete grid;
	grid = NULL;
}

void Quad::destroy()
{
	deleteGrid();
	RenderObject::destroy();
}



DynamicRenderGrid *Quad::setSegs(int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo)
{
	DynamicRenderGrid *g = createGrid(x, y);
	if(g)
		g->setSegs(dgox, dgoy, dgmx, dgmy, dgtm, dgo);
	return g;
}

DynamicRenderGrid *Quad::createGrid(int xd, int yd)
{
	delete grid;
	grid = NULL;
	if(xd && yd)
	{
		grid = new DynamicRenderGrid();
		grid->init(xd, yd, texcoords);
	}
	return grid;
}

void Quad::setDrawGridAlpha(size_t x, size_t y, float alpha)
{
	if(grid)
		grid->setAlpha(x, y, alpha);
}

void Quad::setStripPoints(bool vert, const Vector *points, size_t n)
{
	if(grid)
		grid->setStripPoints(vert, points, n);
}

void Quad::resetGrid()
{
	if(grid)
		grid->reset();
}

void Quad::_renderBorder(const RenderState& rs, Vector color, float borderalpha) const
{
	glBindTexture(GL_TEXTURE_2D, 0);

	glColor4f(color.x, color.y, color.z, borderalpha*alpha.x*alphaMod);

	core->getDefaultQuadBorderBuf()->apply();

	if (rs.forceRenderCenter || renderCenter)
	{
		glPointSize(16);
		glDrawArrays(GL_POINTS, 4, 1);
	}

	glLineWidth(2);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	RenderObject::lastTextureApplied = 0;
}



bool Quad::isCoordinateInside(Vector coord, int minSize) const
{
	Vector realscale = getRealScale();
	int hw = fabsf((width)*realscale.x)*0.5f;
	int hh = fabsf((height)*realscale.y)*0.5f;
	if (hw < minSize)
		hw = minSize;
	if (hh < minSize)
		hh = minSize;

	Vector pos = getRealPosition();

	if (coord.x >= pos.x - hw && coord.x <= pos.x + hw)
	{
		if (coord.y >= pos.y - hh && coord.y <= pos.y + hh)
		{
			return true;
		}
	}
	return false;
}

bool Quad::isCoordinateInsideWorld(const Vector &coord, int minSize) const
{
	int hw = fabsf((width)*getRealScale().x)*0.5f;
	int hh = fabsf((height)*getRealScale().y)*0.5f;
	if (hw < minSize)
		hw = minSize;
	if (hh < minSize)
		hh = minSize;

	Vector pos = getWorldPosition();
	if (coord.x >= pos.x + offset.x - hw && coord.x <= pos.x + offset.x + hw)
	{
		if (coord.y >= pos.y + offset.y - hh && coord.y <= pos.y + offset.y + hh)
		{
			return true;
		}
	}
	return false;
}

bool Quad::isCoordinateInsideWorldRect(const Vector &coord, int w, int h) const
{
	int hw = w*0.5f;
	int hh = h*0.5f;

	Vector pos = getWorldPosition();
	if (coord.x >= pos.x + offset.x - hw && coord.x <= pos.x + offset.x + hw)
	{
		if (coord.y >= pos.y + offset.y - hh && coord.y <= pos.y + offset.y + hh)
		{
			return true;
		}
	}
	return false;
}

void Quad::renderGrid(const RenderState& rs) const
{
	RenderState rx(rs);
	rx.color = rs.color * this->color;
	rx.alpha = rs.alpha * this->alpha.x * this->alphaMod;

	grid->render(rx);

	// debug points
	if (RenderObject::renderCollisionShape)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		grid->renderDebugPoints(rs);
		RenderObject::lastTextureApplied = 0;
	}
}


void Quad::repeatTextureToFill(bool on)
{
	repeatTexture = on;
	updateTexCoords();

}

void Quad::setRepeatScale(const Vector& repscale)
{
	repeatToFillScale = repscale;
	updateTexCoords();
}

void Quad::setRepeatOffset(const Vector& repoffs)
{
	texOff = repoffs;
	updateTexCoords();
}

void Quad::onRender(const RenderState& rs) const
{
	if (!renderQuad) return;


	// previously, vertical flip was implemented by texcoord inversion.
	// ie. child objects were never affected. not sure if this is desirable behavior but it'll stay like
	// this for now. should probably make it affect children as well.
	glPushMatrix();
	if(_fv)
		glRotatef(180, 1, 0, 0);

	glScalef(width, height, 1);

	if (!grid)
	{
		core->getDefaultQuadGrid()->render(rs);
	}
	else
	{
		renderGrid(rs);
	}

	if(renderBorder)
		_renderBorder(rs, renderBorderColor, borderAlpha);
	else if(rs.forceRenderBorder)
		_renderBorder(rs, rs.renderBorderColor, rs.renderBorderAlpha);

	glPopMatrix();
}

void Quad::updateTexCoords()
{
	if (repeatTexture && texture)
	{
		texcoords.u1 = texOff.x;
		texcoords.v1 = texOff.y;
		texcoords.u2 = (width*scale.x*repeatToFillScale.x)/texture->width + texOff.x;
		texcoords.v2 = (height*scale.y*repeatToFillScale.y)/texture->height + texOff.y;

		if(!grid)
		{
			createGrid(2, 2)->gridType = GRID_UNDEFINED;
		}
		grid->setTexCoords(texcoords);
	}
	else
	{
		texcoords.setStandard();
		// don't delete when a wavy effect is going on, for example
		if(grid && grid->gridType == GRID_UNDEFINED)
			deleteGrid();
	}
}

void Quad::reloadDevice()
{
	RenderObject::reloadDevice();
}

void Quad::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (autoWidth == AUTO_VIRTUALWIDTH)
		width = core->getVirtualWidth();
	else if (autoWidth == AUTO_VIRTUALHEIGHT)
		width = core->getVirtualHeight();

	if (autoHeight == AUTO_VIRTUALWIDTH)
		height = core->getVirtualWidth();
	else if (autoHeight == AUTO_VIRTUALHEIGHT)
		height = core->getVirtualHeight();

	if (grid && alpha.x > 0 && alphaMod > 0)
	{
		grid->update(dt);
	}
}

void Quad::setWidthHeight(float w, float h)
{
	if (h == -1)
		height = w;
	else
		height = h;
	width = w;
}

void Quad::setWidth(float w)
{
	width = w;
}

void Quad::setHeight(float h)
{
	height = h;
}

void Quad::onSetTexture()
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

PauseQuad::PauseQuad() : Quad(), pauseLevel(0), positionSnapTo(0)
{
	addType(SCO_PAUSEQUAD);
}

PauseQuad::~PauseQuad()
{
}

void PauseQuad::onUpdate(float dt)
{
	if (positionSnapTo)
		this->position = *positionSnapTo;

	if (core->particlesPaused <= pauseLevel)
	{
		Quad::onUpdate(dt);
	}
}

void PauseQuad::setPositionSnapTo(InterpolatedVector *positionSnapTo)
{
	this->positionSnapTo = positionSnapTo;
}

CollideQuad::CollideQuad()
	: collideRadius(0)
{
	addType(SCO_COLLIDE_QUAD);
}

CollideQuad::~CollideQuad()
{
}

void CollideQuad::renderCollision(const RenderState& rs) const
{
	if (collideRadius > 0)
	{
		glPushMatrix();
		glLoadIdentity();
		core->setupRenderPositionAndScale();
		glBindTexture(GL_TEXTURE_2D, 0);
		glTranslatef(position.x+offset.x, position.y+offset.y, 0);

		glTranslatef(internalOffset.x, internalOffset.y, 0);
		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1,0,0,0.5);
		drawCircle(collideRadius, 8);
		glDisable(GL_BLEND);
		glTranslatef(offset.x, offset.y,0);
		glPopMatrix();
	}
}

