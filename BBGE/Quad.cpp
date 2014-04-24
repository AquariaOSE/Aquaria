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

#include <assert.h>

Vector Quad::renderBorderColor = Vector(1,1,1);

Quad::Quad(const std::string &tex, const Vector &pos)
: RenderObject()
{
	initQuad();
	position = pos;
	setTexture(tex);
}

/*
void Quad::initDefaultVBO()
{
}

void Quad::shutdownDefaultVBO()
{
}
*/

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

void Quad::setStrip(const std::vector<Vector> &st)
{
	resetStrip();
	for (int i = 0; i < st.size(); i++)
	{
		if (i < strip.size())
		{
			strip[i].x += st[i].x;
			strip[i].y += st[i].y;
		}
	}
}

void Quad::createGrid(int xd, int yd)
{
	deleteGrid();
	
	xDivs = xd;
	yDivs = yd;
	
	drawGrid = new Vector * [xDivs];
	for (int i = 0; i < xDivs; i++)
	{
		drawGrid[i] = new Vector [yDivs];
		for (int j = 0; j < yDivs; j++)
		{
			drawGrid[i][j].z = 1;
		}
	}
	
	resetGrid();
}

void Quad::setDrawGridAlpha(int x, int y, float alpha)
{
	if (x < xDivs && x >= 0 && y < yDivs && y >= 0)
	{
		drawGrid[x][y].z = alpha;
	}
}

void Quad::setGridPoints(bool vert, const std::vector<Vector> &points)
{
	if (!drawGrid) return;
	resetGrid();
	for (int i = 0; i < points.size(); i++)
	{
		if (!vert) // horz
		{
			for (int y = 0; y < yDivs; y++)
			{
				for (int x = 0; x < xDivs; x++)
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
			for (int x = 0; x < xDivs; x++)
			{
				for (int y = 0; y < yDivs; y++)
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
		for (int i = 0; i < strip.size(); i++)
		{
			//float v = (i/(float)(strip.size()-1))-0.5f;
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
	for (int i = 0; i < xDivs; i++)
	{
		for (int j = 0; j < yDivs; j++)
		{
			drawGrid[i][j].x = i/(float)(xDivs-1)-0.5f;
			drawGrid[i][j].y = j/(float)(yDivs-1)-0.5f;
		}
	}
}

void Quad::spawnChildClone(float t)
{
	if (!this->texture) return;
	Quad *q = new Quad;
	q->setTexture(this->texture->name);
	q->setLife(t+0.1f);
	q->setDecayRate(1);
	q->width = this->width;
	q->height = this->height;
	q->alpha = 1;
	q->alpha.interpolateTo(0, t);
	if (isfh())
		q->flipHorizontal();
	q->position = this->position;
	q->followCamera = this->followCamera;
	q->scale = this->scale;
	q->offset = this->offset;
	q->blendType = this->blendType;

	//q->parentManagedPointer = true;
	//q->renderBeforeParent = false;
	core->getTopStateData()->addRenderObject(q, this->layer);
	//addChild(q);
}
/*
smoothly transition to texture
by creating a copy of the current quad on top and fading it out
*/
void Quad::setTextureSmooth(const std::string &texture, float t)
{
	if (this->texture && !this->texture->name.empty())
	{
		spawnChildClone(t);
		//core->getTopStateData()->addRenderObject(q, this->layer);
	}
	this->setTexture(texture);
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

	//debugLog("Quad::initQuad()");

	repeatingTextureToFill = false;
	
	drawGrid = 0;

	renderBorder = false;
	renderCenter = true;
	width = 2; height = 2;
	//llalpha = Vector(1);
	//lralpha = Vector(1);
	//ulalpha = Vector(1);
	//uralpha = Vector(1);
	//oriented = false;
	upperLeftTextureCoordinates = Vector(0,0);
	lowerRightTextureCoordinates = Vector(1,1);
	renderQuad = true;
	//debugLog("End Quad::initQuad()");
}

Quad::Quad() : RenderObject()
{
	addType(SCO_QUAD);
	borderAlpha = 0.5;
	//debugLog("Quad::Quad()");
	initQuad();
	//debugLog("End Quad::Quad()");
	//textureSize = Vector(1,1);
}

void Quad::deleteGrid()
{
	if (drawGrid)
	{
		for (int i = 0; i < xDivs; i++)
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
	//if (xDivs == 0 && yDivs == 0) return;
	if (!doUpdateGrid) return;

	if (gridType == GRID_WAVY)
	{
		gridTimer += dt * drawGridTimeMultiplier;
		resetGrid();
		int hx = xDivs/2;
		for (int x = 0; x < xDivs; x++)
		{
			float yoffset = x * drawGridOffsetY;
			float addY = 0;
			if (drawGridModY != 0)
				addY = cosf(gridTimer+yoffset)*drawGridModY;
			for (int y = 0; y < yDivs; y++)
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

#ifdef BBGE_BUILD_OPENGL
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

	/*
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	*/
	glBegin(GL_QUADS);
	float u0 = baseX;
	float u1 = u0 + incX;
	for (int i = 0; i < (xDivs-1); i++, u0 = u1, u1 += incX)
	{
		float v0 = 1 - percentY + baseY;
		float v1 = v0 + incY;
		for (int j = 0; j < (yDivs-1); j++, v0 = v1, v1 += incY)
		{
			if (drawGrid[i][j].z != 0 || drawGrid[i][j+1].z != 0 || drawGrid[i+1][j].z != 0 || drawGrid[i+1][j+1].z != 0)
			{

				glColor4f(red, green, blue, alpha*drawGrid[i][j].z);
				glTexCoord2f(u0, v0);
					//glMultiTexCoord2fARB(GL_TEXTURE0_ARB, u0-baseX, v0-baseY);
					//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);
				glVertex2f(w*drawGrid[i][j].x,		h*drawGrid[i][j].y);
				//
				glColor4f(red, green, blue, alpha*drawGrid[i][j+1].z);
				glTexCoord2f(u0, v1);
					//glMultiTexCoord2fARB(GL_TEXTURE0_ARB, u0-baseX, v1-baseY);
					//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,(float)(screenHeight/(yDivs-1))/16);
				glVertex2f(w*drawGrid[i][j+1].x,		h*drawGrid[i][j+1].y);
				//
				glColor4f(red, green, blue, alpha*drawGrid[i+1][j+1].z);
				glTexCoord2f(u1, v1);
					//glMultiTexCoord2fARB(GL_TEXTURE0_ARB, u1-baseX, v1-baseY);
					//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,(float)(screenHeight/(yDivs-1))/16);
				glVertex2f(w*drawGrid[i+1][j+1].x,	h*drawGrid[i+1][j+1].y);
				//
				glColor4f(red, green, blue, alpha*drawGrid[i+1][j].z);
				glTexCoord2f(u1, v0);
					//glMultiTexCoord2fARB(GL_TEXTURE0_ARB, u1-baseX, v0-baseY);
					//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,0);
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
			for (int i = 0; i < (xDivs-1); i++)
			{
				for (int j = 0; j < (yDivs-1); j++)
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
#endif
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

#ifdef BBGE_BUILD_OPENGL

	float _w2 = width/2.0f;
	float _h2 = height/2.0f;

	if (!strip.empty())
	{
		//glDisable(GL_BLEND);gggg
		//glDisable(GL_CULL_FACE);

		const float texBits = 1.0f / (strip.size()-1);

		glBegin(GL_QUAD_STRIP);

		if (!stripVert)
		{
			for (int i = 0; i < strip.size(); i++)
			{
				glTexCoord2f(texBits*i, 0);
				glVertex2f(strip[i].x*width-_w2,  strip[i].y*_h2*10 - _h2);
				glTexCoord2f(texBits*i, 1);
				glVertex2f(strip[i].x*width-_w2,  strip[i].y*_h2*10 + _h2);
			}
		}
		glEnd();

		//glEnable(GL_CULL_FACE);
		glBindTexture( GL_TEXTURE_2D, 0 );
		glColor4f(1,0,0,1);
		glPointSize(64);

		glBegin(GL_POINTS);
		for (int i = 0; i < strip.size(); i++)
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

#endif
#ifdef BBGE_BUILD_DIRECTX
	//core->setColor(color.x, color.y, color.z, alpha.x);
	//if (!children.empty() || useDXTransform)
	if (true)
	{
		if (this->texture)
		{
			if (upperLeftTextureCoordinates.x != 0 || upperLeftTextureCoordinates.y != 0
				|| lowerRightTextureCoordinates.x != 1 || lowerRightTextureCoordinates.y != 1)
			{
				//core->blitD3DEx(this->texture->d3dTexture, fontDrawSize/2, fontDrawSize/2, u, v-ybit, u+xbit, v+ybit-ybit);
				core->blitD3DEx(this->texture->d3dTexture, width, height, upperLeftTextureCoordinates.x, upperLeftTextureCoordinates.y, lowerRightTextureCoordinates.x, lowerRightTextureCoordinates.y);
			}
			else
				core->blitD3D(this->texture->d3dTexture, width, height);
		}
		else
		{
			core->blitD3D(0, width, height);
		}
	}
	else
	{
		if (this->texture)
			core->blitD3DPreTrans(this->texture->d3dTexture, position.x+offset.x, position.y+offset.y, width*scale.x, width.y*scale.y);
		else
			core->blitD3DPreTrans(0, position.x+offset.x, position.y+offset.y, width*scale.x, width.y*scale.y);
	}

	/*
	if (this->texture)
	{
		core->getD3DSprite()->Begin(D3DXSPRITE_ALPHABLEND);
		D3DXVECTOR2 scaling((1.0f/float(this->texture->width))*width*scale.x,
			(1.0f/float(this->texture->height))*height*scale.y);
		if (isfh())
			scaling.x = -scaling.x;
		D3DXVECTOR2 spriteCentre=D3DXVECTOR2((this->texture->width/2), (this->texture->height/2));
		///scale.x
		//D3DXVECTOR2 trans=D3DXVECTOR2(position.x, position.y);


		if (blendType == BLEND_DEFAULT)
		{
			core->getD3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			core->getD3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		}
		else
		{
			core->getD3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			core->getD3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		}

		D3DXVECTOR2 rotationCentre = spriteCentre;
		D3DXVECTOR2 trans=D3DXVECTOR2(position.x,position.y) - spriteCentre;
		if (followCamera != 1)
		{
			trans.x -= core->cameraPos.x;
			trans.y -= core->cameraPos.y;
		}
		D3DXMATRIX mat, scale, final;
		//D3DXVECTOR2 centre = trans + spriteCentre;
		float rotation = (this->rotation.z*PI)/180.0f;
		//D3DXVECTOR2 scaling((1.0f/float(this->texture->width))*width*scale.x,(1.0f/float(this->texture->height))*height*scale.y);

		//D3DXVECTOR2 scaling(1,1);
		const D3DCOLOR d3dColor=D3DCOLOR_ARGB(int(alpha.x*255), int(color.x*255), int(color.y*255), int(color.z*255));
		//const D3DCOLOR d3dColor=D3DCOLOR_ARGB(int(alpha.x*255), int(color.x*255), int(color.y*255), int(color.z*255));
		FLOAT scalingRotation = 0;
		//D3DXMatrixTransformation2D(&mat,NULL,0.0,&scaling,&spriteCentre,rotation,&trans);
		D3DXMatrixTransformation2D(&mat,
			&spriteCentre,
			scalingRotation,
			&scaling,
			&spriteCentre,
			rotation,
			&trans
		);

		if (followCamera != 1)
		{
			D3DXMatrixScaling(&scale,core->globalScale.x*core->globalResolutionScale.x,core->globalScale.y*core->globalResolutionScale.y,1);
			D3DXMatrixMultiply(&final, &mat, &scale);

			core->getD3DSprite()->SetTransform(&final);
		}
		else
		{
			D3DXMatrixScaling(&scale,core->globalResolutionScale.x,core->globalResolutionScale.y,1);
			D3DXMatrixMultiply(&final, &mat, &scale);
			core->getD3DSprite()->SetTransform(&final);
		}


		//mat = scale * mat;

		if (this->texture)
		{
			core->getD3DSprite()->Draw(this->texture->d3dTexture,NULL,NULL,NULL,d3dColor);//0xFFFFFFFF);//d3dColor);
			core->getD3DSprite()->End();
		}
		else
		{
			core->getD3DSprite()->End();
			D3DRECT rect;
			rect.x1 = trans.x - this->width/2;
			rect.x2 = trans.x + this->width/2;
			rect.y1 = trans.y - this->height/2;
			rect.y2 = trans.y + this->height/2;
			core->getD3DDevice()->Clear(1,&rect,D3DCLEAR_TARGET,d3dColor,0,0);
		}
		//core->getD3DSprite()->End();
	}
	*/

#endif
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
}

PauseQuad::PauseQuad() : Quad(), pauseLevel(0)
{
	addType(SCO_PAUSEQUAD);
}

void PauseQuad::onUpdate(float dt)
{
	if (core->particlesPaused <= pauseLevel)
	{
		Quad::onUpdate(dt);
	}
}

