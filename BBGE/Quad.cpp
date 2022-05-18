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
		gridTimer = 0;
		xDivs = 0;
		yDivs = 0;
		doUpdateGrid = false;
	}
	else
	{
		this->drawGridOffsetX = dgox;
		this->drawGridOffsetY = dgoy;
		this->drawGridModX = dgmx;
		this->drawGridModY = dgmy;
		this->drawGridTimeMultiplier = dgtm;
		drawGridOut = dgo;
		xDivs = x;
		yDivs = y;

		createGrid(x, y);

		gridTimer = 0;

		doUpdateGrid = true;
	}
}

void Quad::createStrip(bool vert, int num)
{
	strip.resize(num);
	stripVert = vert;
	resetStrip();
}

void Quad::createGrid(int xd, int yd)
{
	deleteGrid();

	xDivs = xd;
	yDivs = yd;

	drawGrid = new Vector * [xDivs];
	for (size_t i = 0; i < xDivs; i++)
	{
		drawGrid[i] = new Vector [yDivs];
		for (size_t j = 0; j < yDivs; j++)
		{
			drawGrid[i][j].z = 1;
		}
	}

	resetGrid();
}

void Quad::setDrawGridAlpha(size_t x, size_t y, float alpha)
{
	if (x < xDivs && y < yDivs)
	{
		drawGrid[x][y].z = alpha;
	}
}

void Quad::setGridPoints(bool vert, const std::vector<Vector> &points)
{
	if (!drawGrid) return;
	resetGrid();
	for (size_t i = 0; i < points.size(); i++)
	{
		if (!vert) // horz
		{
			for (size_t y = 0; y < yDivs; y++)
			{
				for (size_t x = 0; x < xDivs; x++)
				{
					if (x < points.size())
					{
						drawGrid[x][y] += points[x];
					}
				}
			}
		}
		else
		{
			for (size_t x = 0; x < xDivs; x++)
			{
				for (size_t y = 0; y < yDivs; y++)
				{
					if (y < points.size())
					{
						drawGrid[x][y] += points[y];
					}
				}
			}
		}
	}
}

float Quad::getStripSegmentSize()
{
	return (1.0f/(float(strip.size())));
}

void Quad::resetStrip()
{
	if (!stripVert)
	{
		for (size_t i = 0; i < strip.size(); i++)
		{

			float v = (i/(float(strip.size())));
			strip[i].x = v;
			strip[i].y = 0;
		}
	}
	else
	{
		errorLog("VERTICAL STRIP NOT SUPPORTED ^_-");
	}
}

void Quad::resetGrid()
{
	for (size_t i = 0; i < xDivs; i++)
	{
		for (size_t j = 0; j < yDivs; j++)
		{
			drawGrid[i][j].x = i/(float)(xDivs-1)-0.5f;
			drawGrid[i][j].y = j/(float)(yDivs-1)-0.5f;
		}
	}
}

void Quad::initQuad()
{
	repeatToFillScale = Vector(1,1);
	gridType = GRID_WAVY;
	gridTimer = 0;
	xDivs = 0;
	yDivs = 0;

	doUpdateGrid = false;

	autoWidth = autoHeight = 0;



	repeatingTextureToFill = false;

	drawGrid = 0;

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
	if (drawGrid)
	{
		for (size_t i = 0; i < xDivs; i++)
		{
			delete[] drawGrid[i];
		}
		delete[] drawGrid;
		drawGrid = 0;
	}
}

void Quad::destroy()
{
	deleteGrid();
	RenderObject::destroy();
}

bool Quad::isCoordinateInside(Vector coord, int minSize)
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

bool Quad::isCoordinateInsideWorld(const Vector &coord, int minSize)
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

bool Quad::isCoordinateInsideWorldRect(const Vector &coord, int w, int h)
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
		size_t hx = xDivs/2;
		for (size_t x = 0; x < xDivs; x++)
		{
			float yoffset = x * drawGridOffsetY;
			float addY = 0;
			if (drawGridModY != 0)
				addY = cosf(gridTimer+yoffset)*drawGridModY;
			for (size_t y = 0; y < yDivs; y++)
			{
				float xoffset = y * drawGridOffsetX;
				if (drawGridModX != 0)
				{
					float addX = (sinf(gridTimer+xoffset)*drawGridModX);
					if (drawGridOut && x < hx)
						drawGrid[x][y].x += addX;
					else
						drawGrid[x][y].x -= addX;
				}
				drawGrid[x][y].y += addY;
			}
		}
	}
}

void Quad::renderGrid()
{
	if (xDivs < 2 || yDivs < 2)
		return;

	const float percentX = fabsf(this->lowerRightTextureCoordinates.x - this->upperLeftTextureCoordinates.x);
	const float percentY = fabsf(this->upperLeftTextureCoordinates.y - this->lowerRightTextureCoordinates.y);

	const float baseX =
		(lowerRightTextureCoordinates.x < upperLeftTextureCoordinates.x)
		? lowerRightTextureCoordinates.x : upperLeftTextureCoordinates.x;
	const float baseY =
		(lowerRightTextureCoordinates.y < upperLeftTextureCoordinates.y)
		? lowerRightTextureCoordinates.y : upperLeftTextureCoordinates.y;

	// NOTE: These are used to avoid repeated expensive divide operations,
	// but they may cause rounding error of around 1 part per million,
	// which could in theory cause minor graphical glitches with broken
	// OpenGL implementations.  --achurch
	const float incX = percentX / (float)(xDivs-1);
	const float incY = percentY / (float)(yDivs-1);

	const float w = this->getWidth();
	const float h = this->getHeight();

	const float red   = this->color.x;
	const float green = this->color.y;
	const float blue  = this->color.z;
	const float alpha = this->alpha.x * this->alphaMod;


	glBegin(GL_QUADS);
	float u0 = baseX;
	float u1 = u0 + incX;
	for (size_t i = 0; i < (xDivs-1); i++, u0 = u1, u1 += incX)
	{
		float v0 = 1 - percentY + baseY;
		float v1 = v0 + incY;
		for (size_t j = 0; j < (yDivs-1); j++, v0 = v1, v1 += incY)
		{
			if (drawGrid[i][j].z != 0 || drawGrid[i][j+1].z != 0 || drawGrid[i+1][j].z != 0 || drawGrid[i+1][j+1].z != 0)
			{

				glColor4f(red, green, blue, alpha*drawGrid[i][j].z);
				glTexCoord2f(u0, v0);


				glVertex2f(w*drawGrid[i][j].x,		h*drawGrid[i][j].y);

				glColor4f(red, green, blue, alpha*drawGrid[i][j+1].z);
				glTexCoord2f(u0, v1);


				glVertex2f(w*drawGrid[i][j+1].x,		h*drawGrid[i][j+1].y);

				glColor4f(red, green, blue, alpha*drawGrid[i+1][j+1].z);
				glTexCoord2f(u1, v1);


				glVertex2f(w*drawGrid[i+1][j+1].x,	h*drawGrid[i+1][j+1].y);

				glColor4f(red, green, blue, alpha*drawGrid[i+1][j].z);
				glTexCoord2f(u1, v0);


				glVertex2f(w*drawGrid[i+1][j].x,		h*drawGrid[i+1][j].y);
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
			if(xDivs > 0 && yDivs > 0)
			for (size_t i = 0; i < (xDivs-1); i++)
			{
				for (size_t j = 0; j < (yDivs-1); j++)
				{
					glVertex2f(w*drawGrid[i][j].x,		h*drawGrid[i][j].y);
					glVertex2f(w*drawGrid[i][j+1].x,		h*drawGrid[i][j+1].y);
					glVertex2f(w*drawGrid[i+1][j+1].x,	h*drawGrid[i+1][j+1].y);
					glVertex2f(w*drawGrid[i+1][j].x,		h*drawGrid[i+1][j].y);
				}
			}
		glEnd();
		if (texture)
			glBindTexture(GL_TEXTURE_2D, texture->textures[0]);
	}
}

void Quad::repeatTextureToFill(bool on)
{
	repeatingTextureToFill = on;
	repeatTexture = on;
	refreshRepeatTextureToFill();

}

void Quad::onRender()
{
	if (!renderQuad) return;


	float _w2 = width/2.0f;
	float _h2 = height/2.0f;

	if (!strip.empty())
	{



		const float texBits = 1.0f / (strip.size()-1);

		glBegin(GL_QUAD_STRIP);

		if (!stripVert)
		{
			for (size_t i = 0; i < strip.size(); i++)
			{
				glTexCoord2f(texBits*i, 0);
				glVertex2f(strip[i].x*width-_w2,  strip[i].y*_h2*10 - _h2);
				glTexCoord2f(texBits*i, 1);
				glVertex2f(strip[i].x*width-_w2,  strip[i].y*_h2*10 + _h2);
			}
		}
		glEnd();


		glBindTexture( GL_TEXTURE_2D, 0 );
		glColor4f(1,0,0,1);
		glPointSize(64);

		glBegin(GL_POINTS);
		for (size_t i = 0; i < strip.size(); i++)
		{
			glVertex2f((strip[i].x*width)-_w2, strip[i].y*height);
		}
		glEnd();
	}
	else
	{
		if (!drawGrid)
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
			renderGrid();
		}
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
	if (repeatingTextureToFill && texture)
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

	if (drawGrid && alpha.x > 0 && alphaMod > 0)
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
