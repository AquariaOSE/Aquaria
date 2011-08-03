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
#include "AfterEffect.h"
//#include "math.h"

#include <assert.h>

Effect::Effect()
{
	done = false;
	rate = 1;
}

AfterEffectManager::AfterEffectManager(int xDivs, int yDivs)
{
	active = false;
	activeShader = AS_NONE;
	numEffects = 0;
	bRenderGridPoints = true;

	screenWidth = core->getWindowWidth();
	screenHeight = core->getWindowHeight();

	this->xDivs = 0;
	this->yDivs = 0;

	drawGrid = 0;
	
#ifdef BBGE_BUILD_OPENGL


	this->xDivs = xDivs;
	this->yDivs = yDivs;
	//cameraPointer = nCameraPointer;

	//Asssuming the resolutions values are > 256 and < 2048
	//Set the texture heights and widths
	if (core->frameBuffer.isInited())
	{
		textureWidth = core->frameBuffer.getWidth();
		textureHeight = core->frameBuffer.getHeight();
	}
	else
	{
		if (screenWidth <= 512)
			textureWidth = 512;
		else if (screenWidth <= 1024)
			textureWidth = 1024;
		else
			textureWidth = 2048;
		if (screenHeight <= 512)
			textureHeight = 512;
		else if (screenHeight <= 1024)
			textureHeight = 1024;
		else
			textureHeight = 2048;
	}

	//create our texture
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	
#endif
	//BuildMip();

	if (xDivs != 0 && yDivs != 0)
	{
		drawGrid = new Vector * [xDivs];
		for (int i = 0; i < xDivs; i++)
		{
			drawGrid[i] = new Vector [yDivs];
		}
	}

	loadShaders();
}

void AfterEffectManager::loadShaders()
{
	/*
	blurShader.load("data/shaders/stan.vert", "data/shaders/blur.frag");
	bwShader.load("data/shaders/stan.vert", "data/shaders/bw.frag");
	washoutShader.load("data/shaders/stan.vert", "data/shaders/washout.frag");
	//motionBlurShader.load("data/shaders/stan.vert", "data/shaders/hoblur.frag");
	motionBlurShader.load("data/shaders/stan.vert", "data/shaders/blur.frag");
	glowShader.load("data/shaders/stan.vert", "data/shaders/glow.frag");
	*/
}

AfterEffectManager::~AfterEffectManager()
{
	if (drawGrid)
	{
		int i;
		for (i = 0; i < xDivs; i++)
		{
			delete[] drawGrid[i];
		}	
		delete[] drawGrid;
	}
	deleteEffects();
}

void AfterEffectManager::deleteEffects()
{
	for (int i = 0; i < effects.size(); i++)
	{
		if (effects[i])
		{
			delete effects[i];
		}
	}
	effects.clear();
	numEffects=0;
	while (!openSpots.empty())
		openSpots.pop();
}

void AfterEffectManager::clear()
{
	deleteEffects();
	resetGrid();
}

void AfterEffectManager::update(float dt)
{
	if (core->particlesPaused) return;	

	resetGrid();

	if (core->frameBuffer.isInited())
		active = true;
	else
		active = false;

	for (int i = 0; i < effects.size(); i++)
	{		
		Effect *e = effects[i];
		if (e)
		{
			active = true;
			e->update(dt, drawGrid, xDivs, yDivs);
			if (e->done)
			{
				numEffects--;
				destroyEffect(i);
			}
		}
	}
}


void AfterEffectManager::resetGrid()
{
	for (int i = 0; i < xDivs; i++)
	{
		for (int j = 0; j < yDivs; j++)
		{
			drawGrid[i][j].x = i/(float)(xDivs-1);
			drawGrid[i][j].y = j/(float)(yDivs-1);
		}
	}
}

void AfterEffectManager::destroyEffect(int id)
{
	delete effects[id];
	effects[id] = 0;
	openSpots.push(id);
}

void AfterEffectManager::capture()
{
	/*
#ifdef BBGE_BUILD_OPENGL
	glBindTexture(GL_TEXTURE_2D,texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenWidth, screenHeight);
#endif
	*/
	if (core->frameBuffer.isInited())
	{
		core->frameBuffer.endCapture();
		//core->enable2D(core->pixelScale);
	}
	else
	{
#ifdef BBGE_BUILD_OPENGL
		glBindTexture(GL_TEXTURE_2D,texture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenWidth, screenHeight);
#endif
	}
//glDisable(GL_TEXTURE_2D);

}
void AfterEffectManager::render()
{
	//if (!core->frameBuffer.isInited() && numEffects==0) return;	
#ifdef BBGE_BUILD_OPENGL
	if (active || core->frameBuffer.isInited())
	{
		glPushMatrix();
		//glDisable(GL_BLEND);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDisable (GL_ALPHA_TEST);
		glDisable(GL_BLEND);

		capture();
		glTranslatef(core->cameraPos.x, core->cameraPos.y, 0);
		glScalef(core->invGlobalScale, core->invGlobalScale,0);
		/*
		static float angle=0;
		angle += 0.03f;
		*/
		//glRotatef(angle, 0, 0, 1);
		//glColor4f(1,1,1,0.75);
		glColor4f(1,1,1,1);
		if (core->mode == Core::MODE_2D)
		{
			renderGrid();
		//	renderGridPoints();
		}
		glPopMatrix();
	}
	else
	{
		glPushMatrix();
		//glDisable(GL_BLEND);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDisable (GL_ALPHA_TEST);
		glDisable(GL_BLEND);

		capture();
		glTranslatef(core->cameraPos.x, core->cameraPos.y, 0);
		glScalef(core->invGlobalScale, core->invGlobalScale,0);
		/*
		static float angle;
		angle += 0.03f;
		*/
		//glRotatef(angle, 0, 0, 1);
		//glColor4f(1,1,1,0.75);
		glColor4f(1,1,1,1);
		if (core->mode == Core::MODE_2D)
		{
			renderGrid();
		//	renderGridPoints();
		}
		glPopMatrix();
	}
	/*
	else
	{
		core->frameBuffer.endCapture();
	}
	*/
#endif
}

void AfterEffectManager::setActiveShader(ActiveShader as)
{
	activeShader = as;
}

void AfterEffectManager::renderGrid()
{
#ifdef BBGE_BUILD_OPENGL
	//glBindTexture(GL_TEXTURE_2D, texture);
	if (core->frameBuffer.isInited())
		core->frameBuffer.bindTexture();
	else
		glBindTexture(GL_TEXTURE_2D, texture);

	
	
	//bwShader.bind();
	Shader *activeShader=0;
	if (core->frameBuffer.isInited())
	{
		switch(this->activeShader)
		{
		case AS_BLUR:
			activeShader = &blurShader;
		break;
		case AS_BW:
			activeShader = &bwShader;
		break;
		case AS_WASHOUT:
			activeShader = &washoutShader;
		break;
		case AS_MOTIONBLUR:
			activeShader = &motionBlurShader;
		break;
		case AS_GLOW:
			activeShader = &glowShader;
		break;
		}
	}

	if (activeShader)
		activeShader->bind();

	screenWidth = core->getWindowWidth();
	screenHeight = core->getWindowHeight();

	/*
	float percentX, percentY;
	percentX = 1;
	percentY = 0.5;
	*/

	/*
	percentX = (float)textureWidth/(float)screenWidth;
	percentY = (float)textureHeight/(float)screenHeight;
	*/

	float percentX, percentY;
	percentX = (float)screenWidth/(float)textureWidth;
	percentY = (float)screenHeight/(float)textureHeight;


		/*
		if (screenWidth <= textureWidth)
		{
			percentX = (float)screenWidth/(float)textureWidth;
			percentY = (float)screenHeight/(float)textureHeight;
		}
		else
		{
			percentY = 10;
		}
		*/


	int vw = core->getVirtualWidth();
	int vh = core->getVirtualHeight();
	int offx = -core->getVirtualOffX();
	int offy = -core->getVirtualOffY();

	if (core->mode == Core::MODE_2D)
	{
		//float div = xDivs;
		for (int i = 0; i < (xDivs-1); i++)
		{
			for (int j = 0; j < (yDivs-1); j++)
			{				
				glBegin(GL_QUADS);		
					//glColor3f(i/div, i/div, i/div);
					glTexCoord2f(i/(float)(xDivs-1)*percentX,  1*percentY-(j)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,i/(float)(xDivs-1)*percentX,  1*percentY-(j)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);
					glVertex2f(offx + vw*drawGrid[i][j].x,		offy + vh*drawGrid[i][j].y);
					glTexCoord2f(i/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,i/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,(float)(screenHeight/(yDivs-1))/16);
					glVertex2f(offx + vw*drawGrid[i][j+1].x,		offy + vh*drawGrid[i][j+1].y);
					glTexCoord2f((i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,(i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,(float)(screenHeight/(yDivs-1))/16);			
					glVertex2f(offx + vw*drawGrid[i+1][j+1].x,	offy + vh*drawGrid[i+1][j+1].y);
					glTexCoord2f((i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,(i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,0);			
					glVertex2f(offx + vw*drawGrid[i+1][j].x,		offy + vh*drawGrid[i+1][j].y);				
				glEnd();
			}
		}

		// uncomment to render grid points
		/*
		glBindTexture(GL_TEXTURE_2D, 0);
		glPointSize(2);
		glColor4f(1, 0, 0, 0.5);
		for (int i = 0; i < (xDivs-1); i++)
		{
			for (int j = 0; j < (yDivs-1); j++)
			{				
				glBegin(GL_POINTS);		
					//glColor3f(i/div, i/div, i/div);
					glTexCoord2f(i/(float)(xDivs-1)*percentX,  1*percentY-(j)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,i/(float)(xDivs-1)*percentX,  1*percentY-(j)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);
					glVertex2f(800*drawGrid[i][j].x,		600*drawGrid[i][j].y);
					glTexCoord2f(i/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,i/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,(float)(screenHeight/(yDivs-1))/16);
					glVertex2f(800*drawGrid[i][j+1].x,		600*drawGrid[i][j+1].y);
					glTexCoord2f((i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,(i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,(float)(screenHeight/(yDivs-1))/16);			
					glVertex2f(800*drawGrid[i+1][j+1].x,	600*drawGrid[i+1][j+1].y);
					glTexCoord2f((i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,(i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j)/(float)(yDivs-1)*percentY);	
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,0);			
					glVertex2f(800*drawGrid[i+1][j].x,		600*drawGrid[i+1][j].y);				
				glEnd();
			}
		}	
		*/
	}

	//glDisable(GL_TEXTURE_2D);
	RenderObject::lastTextureApplied = 0;
	glBindTexture(GL_TEXTURE_2D, 0);
	
	if (activeShader)
		activeShader->unbind();

	//bwShader.unbind();
	//glActiveTextureARB(GL_TEXTURE0_ARB);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	
	//if (bRenderGridPoints)
	//	renderGridPoints();
#endif
}

void AfterEffectManager::renderGridPoints()
{
#ifdef BBGE_BUILD_OPENGL
	glColor4f(0.0f,0.0f,0.0f,1.0f);
	for (int i = 0; i < (xDivs); i++)
	{
		for (int j = 0; j < (yDivs); j++)
		{
		glBegin(GL_QUADS);
			glVertex2f(screenWidth*drawGrid[i][j].x-3,	screenHeight*drawGrid[i][j].y-3);
			glVertex2f(screenWidth*drawGrid[i][j].x-3,	screenHeight*drawGrid[i][j].y+3);
			glVertex2f(screenWidth*drawGrid[i][j].x+3,	screenHeight*drawGrid[i][j].y+3);
			glVertex2f(screenWidth*drawGrid[i][j].x+3,	screenHeight*drawGrid[i][j].y-3);
		glEnd();
		}
	}
#endif
}

void AfterEffectManager::unloadDevice()
{
	if (texture)
		glDeleteTextures(1,&texture);
}

void AfterEffectManager::reloadDevice()
{
	screenWidth = core->getWindowWidth();
	screenHeight = core->getWindowHeight();

	if (core->frameBuffer.isInited())
	{
		textureWidth = core->frameBuffer.getWidth();
		textureHeight = core->frameBuffer.getHeight();
	}
	else
	{
		if (screenWidth <= 1024)
			textureWidth = 1024;
		else
			textureWidth = 2048;
		if (screenHeight <= 1024)
			textureHeight = 1024;
		else
			textureHeight = 2048;
	}

	//create our texture
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void AfterEffectManager::addEffect(Effect *e)
{

	if (!openSpots.empty())
	{
		int i = openSpots.front();
		openSpots.pop();
		effects[i] = e;
	}
	else
	{
		effects.push_back(e);
	}
	numEffects++;
	//float lowest = 9999;
	Vector base(0,0,0);
	//Vector *newPos = &base;
	//Vector *v;
	e->position.x /= screenWidth;
	//e->position.x *= xDivs;
	e->position.y /= screenHeight;
	//e->position.y *= yDivs;

	/*
	for (int x = 1; x < xDivs-1; x++)
	{
		for (int y = 1; y < yDivs-1; y++)
		{
			v = &drawGrid[x][y];
			float dist = (v->x - e->position.x)*(v->x - e->position.x)+(v->y - e->position.y)*(v->y - e->position.y);
			if (dist < lowest)
			{
				lowest = dist;
				newPos = &drawGrid[x][y];
			}
		}
	}
	e->position = Vector(newPos->x, newPos->y, newPos->z);
	*/

}


void ShockEffect::update(float dt, Vector ** drawGrid, int xDivs, int yDivs)
{
	dt *= timeMultiplier;
	Effect::update(dt, drawGrid, xDivs, yDivs);
	//GLdouble sx, sy,sz;
	/*
	gluProject(position.x,position.y,position.z,
		nCameraPointer->modelMatrix,nCameraPointer->projMatrix,nCameraPointer->viewport,
		&sx,&sy,&sz); // Find out where the light is on the screen.
	centerPoint.Set(sx/(float)nCameraPointer->viewport[2],1-sy/(float)nCameraPointer->viewport[3],sz);

  */
	centerPoint = position;
	centerPoint -= ((core->screenCenter-originalCenter)*core->globalScale.x)/core->width;
	//centerPoint = position/xDivs;
	//centerPoint = drawGrid[xDivs/2][yDivs/2];
	float xDist,yDist,tDist;


	amplitude-=dt*rate;
	currentDistance+=dt*frequency;


	//float distFromCamp =(core->cameraPos - position).getLength2D();//v3dDist(nCameraPointer->pos, position);
	//if (distFromCamp < 4)
	float	distFromCamp = 4;

	float adjWaveLength = waveLength/distFromCamp;
	float adjAmplitude = amplitude/distFromCamp;

	if (amplitude < 0)
		done=true;

	for (int i = 1; i < (xDivs-1); i++)
	{
		for (int j = 1; j < (yDivs-1); j++)
		{
			/*
			Vector p = getNearestPointOnLine(centerPoint, centerPoint + Vector(-200, -200), Vector(drawGrid[i][j].x*core->width, drawGrid[i][j].y*core->height));
			
			p.x /= core->width;
			p.y /= core->height;
			*/
			
			xDist = (centerPoint.x - drawGrid[i][j].x)/.75;
			yDist = centerPoint.y - drawGrid[i][j].y;
			
			/*
			xDist = (p.x - drawGrid[i][j].x)/.75;
			yDist = p.y - drawGrid[i][j].y;
			*/

			//xDist = 1;
			//yDist = 2;
			tDist = sqrtf(xDist*xDist+yDist*yDist);

			//drawGrid[i][j].x += (rand()%100)/10000.0f;
			//drawGrid[i][j].y += (rand()%100)/10000.0f;


			if (tDist < currentDistance*adjWaveLength)
			{
				//drawGrid[i][j].x += rand()%50;
				//drawGrid[i][j].y += rand()%50;
				drawGrid[i][j].x += adjAmplitude*sinf(-tDist/adjWaveLength+currentDistance)*.75f;
				drawGrid[i][j].y += adjAmplitude*cosf(-tDist/adjWaveLength+currentDistance);
			}
		}
	}
}


RippleEffect::RippleEffect() : Effect()
{
	time = 0;
}

void RippleEffect::update(float dt, Vector ** drawGrid, int xDivs, int yDivs)
{
	/*
	// whole screen roll
	time += dt;
	float amp = 0.01;
	for (int i = 0; i < (xDivs-1); i++)
	{
		for (int j = 0; j < (yDivs-1); j++)
		{
			float offset = +i/float(xDivs) +j/float(xDivs);
			//drawGrid[i][j].x += sinf(time+offset)*amp;
			drawGrid[i][j].y += cosf((time+offset)*2.5f)*amp;
		}
	}
	*/
	time += dt*0.5f;
	float amp = 0.002;
	for (int i = 0; i < (xDivs-1); i++)
	{
		for (int j = 0; j < (yDivs-1); j++)
		{
			float offset = i/float(xDivs) + (core->screenCenter.x/float(core->width)/2) +j/float(xDivs) + (core->screenCenter.y/float(core->height)/2);
			//drawGrid[i][j].x += sinf(time+offset)*amp;
			drawGrid[i][j].x += sinf((time+offset)*7.5f)*(amp*0.5f);
			drawGrid[i][j].y += cosf((time+offset)*7.5f)*amp;
		}
	}
}
