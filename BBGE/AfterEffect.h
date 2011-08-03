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
#ifndef __after_effect__
#define __after_effect__

#include "Core.h"


class Effect
{
public:
	Effect();
	virtual void go(){}
	virtual void update(float dt, Vector ** drawGrid, int xDivs, int yDivs){}
	bool done;
	Vector position;
protected:
	float rate;
};

class ShockEffect : public Effect
{
public:
	ShockEffect(Vector position, Vector originalCenter, float nAmplitude, float nAmpDecay, float nFrequency,float nWaveLength, float timeMultiplier=1.0) : Effect()
	{
		this->originalCenter = originalCenter;
		this->position = position;
		amplitude = nAmplitude;
		frequency = nFrequency;
		waveLength = nWaveLength;
		rate = nAmpDecay;
		currentDistance = 0;
		this->timeMultiplier = timeMultiplier;
	}
	float timeMultiplier;
	//void go();
	void update(float dt, Vector ** drawGrid, int xDivs, int yDivs);

	float waveLength;
	float amplitude;
	float frequency;

	Vector centerPoint;
	Vector originalCenter;

	float currentDistance;
};

class RippleEffect : public Effect
{
public:
	RippleEffect();
	void update(float dt, Vector ** drawGrid, int xDivs, int yDivs);
	float time;
};

enum ActiveShader
{
	AS_NONE			= 0,
	AS_BLUR			,
	AS_BW			,	
	AS_WASHOUT		,
	AS_MOTIONBLUR	,
	AS_GLOW
};

class AfterEffectManager
{
public:
	AfterEffectManager(int xDivs, int yDivs);
	~AfterEffectManager();
	void addEffect(Effect *e);
	void destroyEffect(int id);
	void update(float dt);

	void clear();
	void deleteEffects();

	void resetGrid();

	void capture();
	void render();
	void renderGrid();
	void renderGridPoints();

	void loadShaders();

	void unloadDevice();
	void reloadDevice();

	std::vector<Effect*> effects;
	std::queue<int> openSpots;

	bool active;

	void setActiveShader(ActiveShader as);

#ifdef BBGE_BUILD_OPENGL
	GLuint texture;
#endif

	bool bRenderGridPoints;

	int numEffects;
	int xDivs, yDivs;
	int screenWidth, screenHeight;
	int textureWidth, textureHeight;

	Shader blurShader, bwShader, washoutShader, motionBlurShader, glowShader;

	Vector ** drawGrid;	

	ActiveShader activeShader;
};


#endif


