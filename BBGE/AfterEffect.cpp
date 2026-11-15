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
	shaderPipeline.resize(10, 0);

	this->xDivs = xDivs;
	this->yDivs = yDivs;

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

void AfterEffectManager::beginCapture()
{
	assert(core->frameBuffer.isInited());
	core->frameBuffer.pushCapture(0);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void AfterEffectManager::endCapture()
{
	core->frameBuffer.popCapture();
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

	const size_t N = effects.size();

	if(!N && !active)
		return;

	resetGrid();

	bool isactive = false;
	for (size_t i = 0; i < N; i++)
	{
		Effect *e = effects[i];
		if (e)
		{
			isactive = true;
			e->update(dt, grid.array2d(), xDivs, yDivs);
			if (e->done)
			{
				numEffects--;
				destroyEffect(i);
			}
		}
	}

	grid.updateVBO();

	// FIXME: active if FBO is there and (effects exist or shaders exist)
	active = isactive && core->frameBuffer.isInited();
}


void AfterEffectManager::resetGrid()
{
	grid.reset01();
}

void AfterEffectManager::destroyEffect(int id)
{
	delete effects[id];
	effects[id] = 0;
	openSpots.push_back(id);
}

void AfterEffectManager::render(const RenderState& rs, unsigned fboPageWithImage) const
{
	assert(core->frameBuffer.isInited());

	glPushMatrix();

	rs.gpu.setBlend(BLEND_DISABLED);

	glTranslatef(core->cameraPos.x, core->cameraPos.y, 0);
	glScalef(core->invGlobalScale, core->invGlobalScale,0);

	glColor4f(1,1,1,1);
	renderGrid(rs, fboPageWithImage);

	glPopMatrix();
}

void AfterEffectManager::renderGrid(const RenderState& rs, unsigned fbPage) const
{
	size_t firstShader, lastShader;
	Shader *activeShader = NULL;
	for (size_t i = 0; i < shaderPipeline.size(); ++i)
	{
		if(shaderPipeline[i] && shaderPipeline[i]->isLoaded())
		{
			if(!activeShader)
			{
				firstShader = i;
				activeShader = shaderPipeline[i];
			}
			lastShader = i;
		}
	}

	// Disable blending so we don't need to clear the framebuffers
	rs.gpu.setBlend(BLEND_DISABLED);

	int vw = core->getVirtualWidth();
	int vh = core->getVirtualHeight();
	int offx = -core->getVirtualOffX();
	int offy = -core->getVirtualOffY();

	const FrameBuffer * const fb = &core->frameBuffer;

	// STARTING POINT: game image was just rendered into fb(0), use that as starting point
	fb->bindTexture(fbPage);

	if(activeShader)
	{
		activeShader->setInt("tex", 0);
		activeShader->bind();

		// Unless this is the last pass, render to fb(1)
		if(firstShader != lastShader)
			fb->pushCapture(1 - fbPage);
	}

	// verts are in 0..1, transform so that we cover the entire screen
	glTranslatef(offx, offy, 0);
	glScalef(vw, vh, 1);

	if(active)
	{
		grid.render(rs);
		//renderGridPoints(rs);
	}
	else
		core->blitQuad.render(rs);

	if (activeShader)
	{
		activeShader->unbind();

		if(firstShader != lastShader)
		{
			// From here on: secondary shader passes.
			// We just outputted to the backup buffer...

			unsigned pageOut = 1 - fbPage;

			for(size_t i = firstShader + 1; i <= lastShader; ++i)
			{
				unsigned pageIn = 1 - pageOut;
				activeShader = shaderPipeline[i];
				if(!(activeShader && activeShader->isLoaded()))
					continue;

				// Swap and exchange framebuffers. The old output buffer serves as texture input for the other one
				pageOut = pageIn;

				// If this is the last pass, do not render to a frame buffer again
				if(i == lastShader)
					fb->popCapture();
				else
					fb->replaceCapture(pageOut);

				fb->bindTexture(pageIn);

				activeShader->bind();
				activeShader->setInt("tex", 0);

				core->blitQuad.render(rs);

				activeShader->unbind();
			}
		}
	}

	RenderObject::lastTextureApplied = 0;
	glBindTexture(GL_TEXTURE_2D, 0);
}

void AfterEffectManager::renderGridPoints(const RenderState& rs) const
{
	grid.renderDebugPoints(rs);
}

void AfterEffectManager::unloadDevice()
{
	grid.dropBuffers();
	unloadShaders();
}

void AfterEffectManager::_updateScreenSize()
{
	screenWidth = core->getWindowWidth();
	screenHeight = core->getWindowHeight();

	grid.setTexCoords(core->blitQuad.getTexCoords());
}

void AfterEffectManager::updateDevice()
{
	_updateScreenSize();
	_initGrid();
}

void AfterEffectManager::reloadDevice()
{
	_updateScreenSize();

	_initGrid();

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

	e->position.x /= screenWidth;
	e->position.y /= screenHeight;
}


void ShockEffect::update(float dt, Array2d<Vector>& grid, int xDivs, int yDivs)
{
	dt *= timeMultiplier;
	Effect::update(dt, grid, xDivs, yDivs);

	const Vector centerPoint = position - ((core->screenCenter-originalCenter)*core->globalScale.x)/core->width;
	amplitude-=dt*rate;
	currentDistance+=dt*frequency;

	const float distFromCamp = 4;
	const float adjWaveLength = waveLength/distFromCamp;
	const float adjAmplitude = amplitude/distFromCamp;

	const float invAdjVaveLen = -1.0f / adjWaveLength;
	const float dist = currentDistance*adjWaveLength;

	if (amplitude < 0)
		done=true;

	for (int y = 1; y < (yDivs-1); y++)
	{
		Vector *row = grid.row(y);
		for (int x = 1; x < (xDivs-1); x++)
		{
			float xDist = (centerPoint.x - row[x].x)/.75f; // factor for 4:3 internal resolution
			float yDist = centerPoint.y - row[x].y;

			float tDist = sqrtf(xDist*xDist+yDist*yDist);

			if (tDist < dist)
			{
				const float a = tDist * invAdjVaveLen + currentDistance;
				row[x].x += adjAmplitude*sinf(a)*.75f;
				row[x].y += adjAmplitude*cosf(a);
			}
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

void AfterEffectManager::_initGrid()
{
	if(xDivs && yDivs)
		grid.init(xDivs, yDivs);
	else
		grid.dropBuffers();
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

