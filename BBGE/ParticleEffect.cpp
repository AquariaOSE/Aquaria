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
#include "Particles.h"
#include "SimpleIStringStream.h"

ParticleEffect::ParticleEffect() : RenderObject()
{
	addType(SCO_PARTICLE_EFFECT);
	running = false;
	waitForParticles = true;
	effectLife = -1;
	die = false;
	effectLifeCounter = -1;
	cull = false;
}

void ParticleEffect::setDie(bool v)
{
	die = v;
}

void ParticleEffect::load(const std::string &name)
{
	particleManager->loadParticleEffectFromBank(name, this);
}

void ParticleEffect::transfer(ParticleEffect *pe)
{
	pe->effectLife = this->effectLife;
	pe->clearEmitters();
	pe->name = this->name;

	for (Emitters::iterator i = emitters.begin(); i != emitters.end(); i++)
	{
		Emitter *e = pe->addNewEmitter();
		e->data = (*i)->data;
		e->setTexture(e->data.texture);

	}
}

Emitter *ParticleEffect::addNewEmitter()
{
	Emitter *e = new Emitter(this);
	emitters.push_back(e);
	addChild(e, PM_POINTER);
	return e;
}

void ParticleEffect::clearEmitters()
{
	for (Emitters::iterator i = emitters.begin(); i != emitters.end(); i++)
	{
		(*i)->destroy();
		delete *i;
	}
	emitters.clear();
	children.clear();
}

void ParticleEffect::bankLoad(const std::string &file, const std::string &path)
{
	std::string usef = path + file + ".txt";

	name = file;
	stringToLower(name);

	clearEmitters();

	usef = adjustFilenameCase(usef);
	debugLog(usef);
	char *buffer = readFile(usef.c_str());
	if (!buffer)
	{
		debugLog("Can't read " + usef);
		return;
	}

	SimpleIStringStream inf(buffer, SimpleIStringStream::TAKE_OVER);
	std::string token, tmp;
	int state=0;
	Emitter *currentEmitter = 0;
	while (inf >> token)
	{

		if (token == "[Emitter]")
		{
			state = 1;
			currentEmitter = addNewEmitter();
			continue;
		}
		if (token == "[Color]")
		{
			state = 2;
			continue;
		}
		if (token == "[Number]")
		{
			state = 3;
			continue;
		}
		if (token == "[Alpha]")
		{
			state = 4;
			continue;
		}
		if (token == "[Rotation]")
		{
			state = 5;
			continue;
		}
		if (token == "[Scale]")
		{
			state = 6;
			continue;
		}

		if (state == 2 && currentEmitter)
		{
			float t, x, y, z;

			SimpleIStringStream is(token);
			is >> t;
			inf >> x >> y >> z;
			currentEmitter->data.color.ensureData();
			currentEmitter->data.color.data->path.addPathNode(Vector(x,y,z), t);
			currentEmitter->data.color.startPath(currentEmitter->data.life);


		}
		if (state == 3 && currentEmitter)
		{
			float t, num;
			SimpleIStringStream is(token);
			is >> t;
			inf >> num;
			currentEmitter->data.number.ensureData();
			currentEmitter->data.number.data->path.addPathNode(num, t);
			currentEmitter->data.number.startPath(currentEmitter->data.life);


		}
		if (state == 4 && currentEmitter)
		{
			float t, num;
			SimpleIStringStream is(token);
			is >> t;
			inf >> num;
			currentEmitter->data.alpha.ensureData();
			currentEmitter->data.alpha.data->path.addPathNode(num, t);
			currentEmitter->data.alpha.startPath(currentEmitter->data.life);


		}
		if (state == 5 && currentEmitter)
		{
			float t, num;
			SimpleIStringStream is(token);
			is >> t;
			inf >> num;
			currentEmitter->data.rotation.ensureData();
			currentEmitter->data.rotation.data->path.addPathNode(Vector(0,0,num), t);
			currentEmitter->data.rotation.startPath(currentEmitter->data.life);


		}
		if (state == 6 && currentEmitter)
		{
			float t, sx, sy;
			SimpleIStringStream is(token);
			is >> t;
			inf >> sx >> sy;
			currentEmitter->data.scale.ensureData();
			currentEmitter->data.scale.data->path.addPathNode(Vector(sx, sy), t);
			currentEmitter->data.scale.startPath(currentEmitter->data.life);


		}


		if (token == "EmitterLife")
		{
			inf >> tmp;
			inf >> effectLife;
			continue;
		}
		if (token == "EmitterScale")
		{
			inf >> tmp;
			inf >> scale.x;
			scale.y = scale.x;
			continue;
		}
		if (token == "EmitterUpdateCull")
		{
			inf >> tmp;
			inf >> updateCull;
			continue;
		}

		// subs
		if (currentEmitter)
		{

			if (token == "SpawnLocal")
			{
				inf >> tmp;
				inf >> currentEmitter->data.spawnLocal;
				continue;
			}
			else if (token == "Texture")
			{
				inf >> tmp;
				inf >> currentEmitter->data.texture;
			}
			else if (token == "RandomScale")
			{
				inf >> tmp;
				inf >> currentEmitter->data.randomScale1 >> currentEmitter->data.randomScale2;
			}
			else if (token == "RandomAlphaMod")
			{
				inf >> tmp;
				inf >> currentEmitter->data.randomAlphaMod1 >> currentEmitter->data.randomAlphaMod2;
			}
			else if (token == "RandomSpawnRadius")
			{
				inf >> tmp;
				inf >> currentEmitter->data.randomSpawnRadius;
			}
			else if (token == "RandomSpawnMod")
			{
				inf >> tmp;
				inf >> currentEmitter->data.randomSpawnMod.x >> currentEmitter->data.randomSpawnMod.y;
			}
			else if (token == "RandomSpawnRadiusRange")
			{
				inf >> tmp;
				inf >> currentEmitter->data.randomSpawnRadiusRange;
			}
			else if (token == "RandomVelocityMagnitude")
			{
				inf >> tmp;
				inf >> currentEmitter->data.randomVelocityMagnitude;
			}
			else if (token == "CopyParentRotation")
			{
				inf >> tmp;
				inf >> currentEmitter->data.copyParentRotation;
			}
			else if (token == "CopyParentFlip")
			{
				inf >> tmp;
				inf >> currentEmitter->data.copyParentFlip;
			}
			else if (token == "JustOne")
			{
				inf >> tmp;
				inf >> currentEmitter->data.justOne;
			}
			else if (token == "SpawnTimeOffset")
			{
				inf >> tmp;
				inf >> currentEmitter->data.spawnTimeOffset;
			}
			else if (token == "RandomRotationRange")
			{
				inf >> tmp;
				inf >> currentEmitter->data.randomRotationRange;
			}
			else if (token == "InitialVelocity")
			{
				inf >> tmp;
				inf >> currentEmitter->data.initialVelocity.x >> currentEmitter->data.initialVelocity.y;
			}
			else if (token == "Influenced")
			{
				inf >> tmp;
				inf >> currentEmitter->data.influenced;
			}
			else if (token == "DeathPrt")
			{
				inf >> tmp;
				inf >> currentEmitter->data.deathPrt;
			}
			else if (token == "Gravity")
			{
				inf >> tmp;
				inf >> currentEmitter->data.gravity.x >> currentEmitter->data.gravity.y;
			}
			else if (token == "PauseLevel")
			{
				inf >> tmp;
				inf >> currentEmitter->data.pauseLevel;
				//errorLog("read in pauseLevel");
			}
			else if (token == "FlipH")
			{
				inf >> tmp;
				inf >> currentEmitter->data.flipH;
			}
			else if (token == "FlipV")
			{
				inf >> tmp;
				inf >> currentEmitter->data.flipV;
			}
			else if (token == "Blend")
			{
				inf >> tmp;
				std::string blendType;
				inf >> blendType;
				if (blendType == "Add")
					currentEmitter->data.blendType = BLEND_ADD;
				else if (blendType == "Sub")
					currentEmitter->data.blendType = BLEND_SUB;
			}
			else if (token == "Width")
			{
				inf >> tmp;
				inf >> currentEmitter->data.width;
			}
			else if (token == "Height")
			{
				inf >> tmp;
				inf >> currentEmitter->data.height;
			}
			else if (token == "Life")
			{
				inf >> tmp;
				inf >> currentEmitter->data.life;
			}
			else if (token == "Suck")
			{
				inf >> tmp;
				inf >> currentEmitter->data.suckIndex >> currentEmitter->data.suckStr;
			}
			else if (token == "InheritColor")
			{
				inf >> tmp;
				inf >> currentEmitter->data.inheritColor;
			}
			else if (token == "InheritAlpha")
			{
				inf >> tmp;
				inf >> currentEmitter->data.inheritAlpha;
			}
		}
	}
}

void ParticleEffect::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);



	if (effectLifeCounter == 0)
	{
		if (waitForParticles)
		{
			// extra loop, could be combined above later
			int c=0,e=0;
			for (Emitters::iterator i = emitters.begin(); i != emitters.end(); i++)
			{
				if ((*i)->isEmpty())
				{
					e++;
				}
				c++;
			}
			if (c == e)
			{
				if (die)
					safeKill();
			}
		}
		else
		{
			if (die)
				safeKill();
		}

	}

	if (effectLifeCounter != -1 && running)
	{
		effectLifeCounter -= dt;
		if (effectLifeCounter <= 0)
		{
			effectLifeCounter = 0;
			stop();
		}
	}
}

// stop the particle effect, let the particles all die off before we delete ourselves
void ParticleEffect::killParticleEffect()
{
	effectLifeCounter = 0.0001f;
	die = true;
	//stop();
}

void ParticleEffect::start()
{
	effectLifeCounter = effectLife;
	running = true;

	for (Emitters::iterator i = emitters.begin(); i != emitters.end(); i++)
	{
		(*i)->start();
	}
}

void ParticleEffect::stop()
{
	running = false;

	for (Emitters::iterator i = emitters.begin(); i != emitters.end(); i++)
	{
		(*i)->stop();
	}
}
