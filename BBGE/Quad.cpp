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
#include <assert.h>

Vector Quad::renderBorderColor = Vector(1,1,1);

Quad::Quad(const std::string &tex, const Vector &pos)
: RenderObject()
{
	initQuad();
	position = pos;
	setTexture(tex);
}



void Quad::setSegs(int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo)
{
	deleteGrid();
	if (x == 0 || y == 0)
	{
		doUpdateGrid = false;
	}
	else
	{
		doUpdateGrid = true;
		this->drawGridOffsetX = dgox;
		this->drawGridOffsetY = dgoy;
		this->drawGridModX = dgmx;
		this->drawGridModY = dgmy;
		this->drawGridTimeMultiplier = dgtm;
		drawGridOut = dgo;
		createGrid(x, y);
	}

	gridTimer = 0;
}

void Quad::createGrid(int xd, int yd)
{
	drawGrid.init(xd, yd);
	resetGrid();
	Vector *dg = drawGrid.data();
	for(size_t i = 0; i < drawGrid.linearsize(); ++i)
		dg[i].z = 1.0f;
}

void Quad::setDrawGridAlpha(size_t x, size_t y, float alpha)
{
	if (x < drawGrid.width() && y < drawGrid.height())
	{
		drawGrid(x, y).z = alpha;
	}
}

void Quad::setStripPoints(bool vert, const Vector *points, size_t n)
{
	if (drawGrid.empty()) return;
	resetGrid();

	const float mul = float(n);

	if (!vert) // horz
	{
		const size_t xmax = std::min(drawGrid.width(), n);
		for (size_t y = 0; y < drawGrid.height(); y++)
		{
			Vector *row = drawGrid.row(y);
			for (size_t x = 0; x < xmax; x++)
				row[x] += points[x] * mul;
		}
	}
	else
	{
		const size_t ymax = std::min(drawGrid.height(), n);
		for (size_t x = 0; x < drawGrid.width(); x++)
			for (size_t y = 0; y < ymax; y++)
				drawGrid(x, y) += points[y] * mul;
	}
}

void Quad::resetGrid()
{
	const float yMulF = 1.0f / (float)(drawGrid.height()-1);
	const float xMulF = 1.0f / (float)(drawGrid.width()-1);

	for (size_t y = 0; y < drawGrid.height(); y++)
	{
		Vector *row = drawGrid.row(y);
		const float yval = float(y)*yMulF-0.5f;
		for (size_t x = 0; x < drawGrid.width(); x++)
		{
			row[x].x = float(x)*xMulF-0.5f;
			row[x].y = yval;
		}
	}
}

void Quad::initQuad()
{
	repeatToFillScale = Vector(1,1);
	gridType = GRID_WAVY;
	gridTimer = 0;

	doUpdateGrid = false;

	autoWidth = autoHeight = 0;

	renderBorder = false;
	renderCenter = true;
	width = 2; height = 2;



	upperLeftTextureCoordinates = Vector(0,0);
	lowerRightTextureCoordinates = Vector(1,1);
	renderQuad = true;

}

Quad::Quad() : RenderObject()
{
	addType(SCO_QUAD);
	borderAlpha = 0.5;

	initQuad();


}

void Quad::deleteGrid()
{
	drawGrid.clear();
}

void Quad::destroy()
{
	deleteGrid();
	RenderObject::destroy();
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

void Quad::updateGrid(float dt)
{
	if (!doUpdateGrid) return;

	if (gridType == GRID_WAVY)
	{
		gridTimer += dt * drawGridTimeMultiplier;
		resetGrid();
		size_t hx = drawGrid.width()/2;
		for (size_t x = 0; x < drawGrid.width(); x++)
		{
			float yoffset = x * drawGridOffsetY;
			float addY = 0;
			if (drawGridModY != 0)
				addY = cosf(gridTimer+yoffset)*drawGridModY;
			for (size_t y = 0; y < drawGrid.height(); y++)
			{
				float xoffset = y * drawGridOffsetX;
				if (drawGridModX != 0)
				{
					float addX = (sinf(gridTimer+xoffset)*drawGridModX);
					if (drawGridOut && x < hx)
						drawGrid(x,y).x += addX;
					else
						drawGrid(x,y).x -= addX;
				}
				drawGrid(x,y).y += addY;
			}
		}
	}
}

void Quad::renderGrid(const RenderState& rs) const
{
	if (drawGrid.width() < 2 || drawGrid.height() < 2)
		return;

	const float percentX = fabsf(this->lowerRightTextureCoordinates.x - this->upperLeftTextureCoordinates.x);
	const float percentY = fabsf(this->upperLeftTextureCoordinates.y - this->lowerRightTextureCoordinates.y);

	const float baseX =
		(lowerRightTextureCoordinates.x < upperLeftTextureCoordinates.x)
		? lowerRightTextureCoordinates.x : upperLeftTextureCoordinates.x;
	const float baseY =
		(lowerRightTextureCoordinates.y < upperLeftTextureCoordinates.y)
		? lowerRightTextureCoordinates.y : upperLeftTextureCoordinates.y;

	const size_t NX = drawGrid.width()-1;
	const size_t NY = drawGrid.height()-1;

	// NOTE: These are used to avoid repeated expensive divide operations,
	// but they may cause rounding error of around 1 part per million,
	// which could in theory cause minor graphical glitches with broken
	// OpenGL implementations.  --achurch
	const float incX = percentX / float(NX);
	const float incY = percentY / float(NY);

	const float w = this->getWidth();
	const float h = this->getHeight();

	const float red   = rs.color.x * this->color.x;
	const float green = rs.color.y * this->color.y;
	const float blue  = rs.color.z * this->color.z;
	const float alpha = rs.alpha * this->alpha.x * this->alphaMod;


	glBegin(GL_QUADS);
	float u0 = baseX;
	float u1 = u0 + incX;
	for (size_t x = 0; x < NX; x++, u0 = u1, u1 += incX)
	{
		float v0 = 1 - percentY + baseY;
		float v1 = v0 + incY;
		for (size_t y = 0; y < NY; y++, v0 = v1, v1 += incY)
		{
			if (drawGrid(x,y).z != 0 || drawGrid(x,y+1).z != 0 || drawGrid(x+1,y).z != 0 || drawGrid(x+1,y+1).z != 0)
			{

				glColor4f(red, green, blue, alpha*drawGrid(x,y).z);
				glTexCoord2f(u0, v0);


				glVertex2f(w*drawGrid(x,y).x,		h*drawGrid(x,y).y);

				glColor4f(red, green, blue, alpha*drawGrid(x,y+1).z);
				glTexCoord2f(u0, v1);


				glVertex2f(w*drawGrid(x,y+1).x,		h*drawGrid(x,y+1).y);

				glColor4f(red, green, blue, alpha*drawGrid(x+1,y+1).z);
				glTexCoord2f(u1, v1);


				glVertex2f(w*drawGrid(x+1,y+1).x,	h*drawGrid(x+1,y+1).y);

				glColor4f(red, green, blue, alpha*drawGrid(x+1,y).z);
				glTexCoord2f(u1, v0);


				glVertex2f(w*drawGrid(x+1,y).x,		h*drawGrid(x+1,y).y);
			}
		}
	}
	glEnd();

	// debug points
	if (RenderObject::renderCollisionShape)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glPointSize(2);
		glColor3f(1,0,0);
		glBegin(GL_POINTS);
			for (size_t x = 0; x < NX; x++)
			{
				for (size_t y = 0; y < NY; y++)
				{
					glVertex2f(w*drawGrid(x,y).x,		h*drawGrid(x,y).y);
					glVertex2f(w*drawGrid(x,y+1).x,		h*drawGrid(x,y+1).y);
					glVertex2f(w*drawGrid(x+1,y+1).x,	h*drawGrid(x+1,y+1).y);
					glVertex2f(w*drawGrid(x+1,y).x,		h*drawGrid(x+1,y).y);
				}
			}
		glEnd();
		if (texture)
			glBindTexture(GL_TEXTURE_2D, texture->textures[0]);
	}
}

void Quad::repeatTextureToFill(bool on)
{
	repeatTexture = on;
	refreshRepeatTextureToFill();

}

void Quad::onRender(const RenderState& rs) const
{
	if (!renderQuad) return;

	const float _w2 = width*0.5f;
	const float _h2 = height*0.5f;

	if (drawGrid.empty())
	{
		glBegin(GL_QUADS);
		{
			glTexCoord2f(upperLeftTextureCoordinates.x, 1.0f-upperLeftTextureCoordinates.y);
			glVertex2f(-_w2, +_h2);

			glTexCoord2f(lowerRightTextureCoordinates.x, 1.0f-upperLeftTextureCoordinates.y);
			glVertex2f(+_w2, +_h2);

			glTexCoord2f(lowerRightTextureCoordinates.x, 1.0f-lowerRightTextureCoordinates.y);
			glVertex2f(+_w2, -_h2);

			glTexCoord2f(upperLeftTextureCoordinates.x, 1.0f-lowerRightTextureCoordinates.y);
			glVertex2f(-_w2, -_h2);
		}
		glEnd();
	}
	else
	{
		renderGrid(rs);
	}

	if (renderBorder)
	{
		glLineWidth(2);

		glBindTexture(GL_TEXTURE_2D, 0);

		glColor4f(renderBorderColor.x, renderBorderColor.y, renderBorderColor.z, borderAlpha*alpha.x*alphaMod);

		if (renderCenter)
		{
			glPointSize(16);
			glBegin(GL_POINTS);
				glVertex2f(0,0);
			glEnd();
		}

		glColor4f(renderBorderColor.x, renderBorderColor.y, renderBorderColor.z, 1*alpha.x*alphaMod);
		glBegin(GL_LINES);
			glVertex2f(-_w2, _h2);
			glVertex2f(_w2, _h2);
			glVertex2f(_w2, -_h2);
			glVertex2f(_w2, _h2);
			glVertex2f(-_w2, -_h2);
			glVertex2f(-_w2, _h2);
			glVertex2f(-_w2, -_h2);
			glVertex2f(_w2, -_h2);
		glEnd();
		RenderObject::lastTextureApplied = 0;
	}

}


void Quad::flipHorizontal()
{
	RenderObject::flipHorizontal();
}

void Quad::flipVertical()
{
	if (!_fv)
	{
		lowerRightTextureCoordinates.y = 0;
		upperLeftTextureCoordinates.y = 1;
	}
	else
	{
		lowerRightTextureCoordinates.y = 1;
		upperLeftTextureCoordinates.y = 0;
	}
	RenderObject::flipVertical();
}

void Quad::refreshRepeatTextureToFill()
{
	if (repeatTexture && texture)
	{
		upperLeftTextureCoordinates.x = texOff.x;
		upperLeftTextureCoordinates.y = texOff.y;
		lowerRightTextureCoordinates.x = (width*scale.x*repeatToFillScale.x)/texture->width + texOff.x;
		lowerRightTextureCoordinates.y = (height*scale.y*repeatToFillScale.y)/texture->height + texOff.y;
	}
	else
	{
		if (fabsf(lowerRightTextureCoordinates.x) > 1 || fabsf(lowerRightTextureCoordinates.y)>1)
			lowerRightTextureCoordinates = Vector(1,1);
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


	refreshRepeatTextureToFill();

	lowerRightTextureCoordinates.update(dt);
	upperLeftTextureCoordinates.update(dt);

	if (!drawGrid.empty() && alpha.x > 0 && alphaMod > 0)
	{
		updateGrid(dt);
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

