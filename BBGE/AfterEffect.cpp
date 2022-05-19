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
#include "RenderBase.h"
#include "Shader.h"


#include <assert.h>

Effect::Effect()
{
	done = false;
	rate = 1;
}

AfterEffectManager::AfterEffectManager(int xDivs, int yDivs)
{
	active = false;
	numEffects = 0;
	bRenderGridPoints = true;
	shaderPipeline.resize(10, 0);

	drawGrid = 0;

	this->xDivs = xDivs;
	this->yDivs = yDivs;

	if (xDivs != 0 && yDivs != 0)
	{
		drawGrid = new Vector * [xDivs];
		for (int i = 0; i < xDivs; i++)
		{
			drawGrid[i] = new Vector [yDivs];
		}
	}

	updateDevice();

	loadShaders();
}

void AfterEffectManager::loadShaders()
{
	deleteShaders();

	// ...Load shaders here...
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
	deleteShaders();
}

void AfterEffectManager::deleteEffects()
{
	for (size_t i = 0; i < effects.size(); i++)
	{
		if (effects[i])
		{
			delete effects[i];
		}
	}
	effects.clear();
	numEffects=0;
	openSpots.clear();
}

void AfterEffectManager::deleteShaders()
{
	for(size_t i = 0; i < shaderPipeline.size(); ++i)
		shaderPipeline[i] = 0;

	for(size_t i = 0; i < loadedShaders.size(); ++i)
	{
		if(loadedShaders[i])
		{
			delete loadedShaders[i];
			loadedShaders[i] = 0;
		}
	}
}

void AfterEffectManager::unloadShaders()
{
	for(size_t i = 0; i < loadedShaders.size(); ++i)
		if(loadedShaders[i])
			loadedShaders[i]->unload();
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

	for (size_t i = 0; i < effects.size(); i++)
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
	openSpots.push_back(id);
}

void AfterEffectManager::render() const
{
	assert(core->frameBuffer.isInited());

	glPushMatrix();

	glDisable (GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	core->frameBuffer.endCapture();
	glTranslatef(core->cameraPos.x, core->cameraPos.y, 0);
	glScalef(core->invGlobalScale, core->invGlobalScale,0);

	glColor4f(1,1,1,1);
	renderGrid();

	glPopMatrix();
}

void AfterEffectManager::renderGrid() const
{

	int firstShader = -1;
	int lastShader = -1;
	Shader *activeShader = 0;
	for (size_t i = 0; i < shaderPipeline.size(); ++i)
	{
		if(shaderPipeline[i] && shaderPipeline[i]->isLoaded())
		{
			if(firstShader < 0)
			{
				firstShader = i;
				activeShader = shaderPipeline[i];
			}
			lastShader = i;
		}
	}

	float percentX, percentY;
	percentX = (float)screenWidth/(float)textureWidth;
	percentY = (float)screenHeight/(float)textureHeight;

	int vw = core->getVirtualWidth();
	int vh = core->getVirtualHeight();
	int offx = -core->getVirtualOffX();
	int offy = -core->getVirtualOffY();

	core->frameBuffer.bindTexture();

	if(activeShader)
	{
		activeShader->bind();
		activeShader->setInt("tex", 0);

		if(firstShader != lastShader)
			backupBuffer.startCapture();
	}


	for (int i = 0; i < (xDivs-1); i++)
	{
		for (int j = 0; j < (yDivs-1); j++)
		{
			glBegin(GL_QUADS);

				glTexCoord2f(i/(float)(xDivs-1)*percentX,  1*percentY-(j)/(float)(yDivs-1)*percentY);


				glVertex2f(offx + vw*drawGrid[i][j].x,		offy + vh*drawGrid[i][j].y);
				glTexCoord2f(i/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);


				glVertex2f(offx + vw*drawGrid[i][j+1].x,		offy + vh*drawGrid[i][j+1].y);
				glTexCoord2f((i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);


				glVertex2f(offx + vw*drawGrid[i+1][j+1].x,	offy + vh*drawGrid[i+1][j+1].y);
				glTexCoord2f((i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j)/(float)(yDivs-1)*percentY);


				glVertex2f(offx + vw*drawGrid[i+1][j].x,		offy + vh*drawGrid[i+1][j].y);
			glEnd();
		}
	}

	if (activeShader)
		activeShader->unbind();

	if(firstShader != lastShader)
	{
		// From here on: secondary shader passes.
		// We just outputted to the backup buffer...
		const FrameBuffer *fbIn = &core->frameBuffer;
		const FrameBuffer *fbOut = &backupBuffer;


		for(int i = firstShader + 1; i <= lastShader; ++i)
		{
			activeShader = shaderPipeline[i];
			if(!(activeShader && activeShader->isLoaded()))
				continue;

			// Swap and exchange framebuffers. The old output buffer serves as texture input for the other one
			fbOut->endCapture();
			std::swap(fbIn, fbOut);
			fbIn->bindTexture();

			// If this is the last pass, do not render to a frame buffer again
			if(i != lastShader)
				fbOut->startCapture();

			activeShader->bind();
			activeShader->setInt("tex", 0);

			// note that offx, offy are negative here!
			glBegin(GL_QUADS);
				glTexCoord2d(0.0f, 0.0f);
				glVertex3f(offx, vh+offy,  0.0f);
				glTexCoord2d(percentX, 0.0f);
				glVertex3f( vw+offx, vh+offy,  0.0f);
				glTexCoord2d(percentX, percentY);
				glVertex3f( vw+offx,  offy,  0.0f);
				glTexCoord2d(0.0f, percentY);
				glVertex3f(offx,  offy,  0.0f);
			glEnd();

			activeShader->unbind();
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

	//glDisable(GL_TEXTURE_2D);
	RenderObject::lastTextureApplied = 0;
	glBindTexture(GL_TEXTURE_2D, 0);

	//bwShader.unbind();
	//glActiveTextureARB(GL_TEXTURE0_ARB);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//if (bRenderGridPoints)
	//	renderGridPoints();
}

void AfterEffectManager::renderGridPoints() const
{
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
}

void AfterEffectManager::unloadDevice()
{
	backupBuffer.unloadDevice();
	unloadShaders();
}

void AfterEffectManager::_updateScreenSize()
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
		textureWidth = screenWidth;
		sizePowerOf2Texture(textureWidth);
		textureHeight = screenHeight;
		sizePowerOf2Texture(textureHeight);
	}
}

void AfterEffectManager::updateDevice()
{
	_updateScreenSize();
	backupBuffer.init(-1, -1, true);
}

void AfterEffectManager::reloadDevice()
{
	_updateScreenSize();

	backupBuffer.reloadDevice();

	for (size_t i = 0; i < loadedShaders.size(); ++i)
	{
		if (Shader *sh = loadedShaders[i])
		{
			sh->reload();
			if (!sh->isLoaded())
			{
				debugLog("AfterEffect::reloadDevice(): Failed to reload shader");
				loadedShaders[i] = 0;
				for(size_t j = 0; j < shaderPipeline.size(); ++j)
					if(sh == shaderPipeline[j])
						shaderPipeline[j] = 0;
				delete sh;
			}
		}
	}
}

void AfterEffectManager::addEffect(Effect *e)
{
	if (!openSpots.empty())
	{
		int i = openSpots.back();
		openSpots.pop_back();
		effects[i] = e;
	}
	else
	{
		effects.push_back(e);
	}
	numEffects++;

	Vector base(0,0,0);

	e->position.x /= screenWidth;
	e->position.y /= screenHeight;
}


void ShockEffect::update(float dt, Vector ** drawGrid, int xDivs, int yDivs)
{
	dt *= timeMultiplier;
	Effect::update(dt, drawGrid, xDivs, yDivs);

	centerPoint = position;
	centerPoint -= ((core->screenCenter-originalCenter)*core->globalScale.x)/core->width;
	amplitude-=dt*rate;
	currentDistance+=dt*frequency;

	float	distFromCamp = 4;
	float adjWaveLength = waveLength/distFromCamp;
	float adjAmplitude = amplitude/distFromCamp;

	if (amplitude < 0)
		done=true;

	for (int i = 1; i < (xDivs-1); i++)
	{
		for (int j = 1; j < (yDivs-1); j++)
		{
			float xDist = (centerPoint.x - drawGrid[i][j].x)/.75;
			float yDist = centerPoint.y - drawGrid[i][j].y;

			float tDist = sqrtf(xDist*xDist+yDist*yDist);

			if (tDist < currentDistance*adjWaveLength)
			{
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



	time += dt*0.5f;
	float amp = 0.002f;
	for (int i = 0; i < (xDivs-1); i++)
	{
		for (int j = 0; j < (yDivs-1); j++)
		{
			float offset = i/float(xDivs) + (core->screenCenter.x/float(core->width)/2) +j/float(xDivs) + (core->screenCenter.y/float(core->height)/2);

			drawGrid[i][j].x += sinf((time+offset)*7.5f)*(amp*0.5f);
			drawGrid[i][j].y += cosf((time+offset)*7.5f)*amp;
		}
	}
}

int AfterEffectManager::loadShaderFile(const char *vert, const char *frag)
{
	Shader *sh = new Shader();
	sh->load(vert, frag);
	if(!sh->isLoaded())
	{
		delete sh;
		return 0;
	}
	return _insertShader(sh);
}

int AfterEffectManager::loadShaderSrc(const char *vert, const char *frag)
{
	Shader *sh = new Shader();
	sh->loadSrc(vert, frag);
	if(!sh->isLoaded())
	{
		delete sh;
		return 0;
	}
	return _insertShader(sh);
}

Shader *AfterEffectManager::getShaderPtr(int handle)
{
	size_t idx = handle - 1;
	return idx  < loadedShaders.size() ? loadedShaders[idx] : 0;
}

void AfterEffectManager::setShaderPipelineSize(size_t size)
{
	shaderPipeline.resize(size, 0);
}

bool AfterEffectManager::setShaderPipelinePos(int handle, size_t pos)
{
	if(pos < shaderPipeline.size())
	{
		shaderPipeline[pos] = getShaderPtr(handle);
		return true;
	}
	return false;
}

// returns handle (= index + 1)
int AfterEffectManager::_insertShader(Shader *sh)
{
	for(size_t i = 0; i < loadedShaders.size(); ++i)
	{
		if(!loadedShaders[i])
		{
			loadedShaders[i] = sh;
			return i+1;
		}
	}
	loadedShaders.push_back(sh);
	return loadedShaders.size();
}

void AfterEffectManager::deleteShader(int handle)
{
	Shader *sh = getShaderPtr(handle);
	if(!sh)
		return;

	for(size_t i = 0; i < shaderPipeline.size(); ++i)
		if(shaderPipeline[i] == sh)
			shaderPipeline[i] = 0;

	size_t idx = handle - 1;
	loadedShaders[idx] = 0;
	delete sh;
}

